#include <bitset>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

#include <Windows.h>

using namespace std::chrono_literals;

const int kWidth = 64, kHeight = 32;

struct Opcode {
	union {
		uint16_t opcode;
		uint16_t nnn : 12;
		uint16_t nn : 8;
		struct {
			uint16_t n : 4;
			uint16_t y : 4;
			uint16_t x : 4;
			uint16_t code : 4;
		} data;
	};
	Opcode() = default;
	Opcode(uint16_t value) : opcode(value) {}
};

bool operator==(const Opcode& a, const Opcode& b) {
	return a.opcode == b.opcode;
}

namespace exception {
	struct file_not_found {};
	struct stack_overflow {};
	struct unknown_opcode { Opcode op; };
}

uint8_t random() {
	static std::mt19937 randomEngine{};
	static std::uniform_int_distribution<> gen(0, 255);
	return static_cast<uint8_t>(gen(randomEngine));
}

class Chip8 {
	union {
		uint8_t memory[4096];
		struct {
			uint8_t V[16], keys[16], delayTimer, soundTimer;
			uint16_t I;
			uint16_t stack[16];
			uint16_t sp, pc;
			std::bitset<kWidth*kHeight> display;
		} header;

	};

public:
	Chip8() {}
	void execute_instruction() {
		Opcode op = memory[header.pc] << 8 | memory[header.pc + 1];
		header.pc += 2;
		uint8_t &Vx = header.V[op.data.x], &Vy = header.V[op.data.y], &V0 = header.V[0], &VF = header.V[0xF];
		//printf("%#06x\n", op.opcode);
		bool drawn = false;
		switch(op.data.code) {
		case 0x0:
			switch(op.opcode) {
			case 0x00E0: header.display.reset(); break;
			case 0x00EE: header.pc = header.stack[--header.sp]; break;
			default: throw exception::unknown_opcode{op};
			}
			break;
		case 0x1:
			header.pc = op.nnn;
			break;
		case 0x2:
			header.stack[header.sp++] = header.pc;
			header.pc = op.nnn;
			break;
		case 0x3:
			if(Vx == op.nn) header.pc += 2;
			break;
		case 0x4:
			if(Vx != op.nn) header.pc += 2;
			break;
		case 0x5:
			if(Vx == Vy) header.pc += 2;
			break;
		case 0x6:
			Vx = op.nn;
			break;
		case 0x7:
			Vx += static_cast<uint8_t>(op.nn);
			break;
		case 0x8:
			switch(op.data.n) {
			case 0x0: Vx = Vy; break;
			case 0x1: Vx |= Vy; break;
			case 0x2: Vx &= Vy; break;
			case 0x3: Vx ^= Vy; break;
			case 0x4: Vx += Vy; VF = Vy > Vx; break;
			case 0x5: VF = Vy > Vx; Vx -= Vy; break;
			case 0x6: VF = Vx & 1; Vx >>= 1; break;
			case 0x7: VF = Vx > Vy; Vx = Vy - Vx; break;
			case 0xe: VF = Vx >> 7; Vx <<= 1; break;
			default: throw exception::unknown_opcode{op};
			}
			break;
		case 0x9:
			if(Vx != Vy) header.pc += 2;
			break;
		case 0xA:
			header.I = op.nnn;
			break;
		case 0xB:
			header.pc = V0 + op.nnn;
			break;
		case 0xC:
			Vx = random() & op.nn;
			break;
		case 0xD:
		{
			VF = 0;
			static std::bitset<kWidth*kHeight> changed;
			changed.reset();
			for(uint16_t y = 0; y < op.data.n; ++y) {
				uint8_t sprite = memory[header.I + y];
				for(uint16_t x = 0; x < 8; ++x) {
					if(sprite & (0x80 >> x)) {
						uint16_t i = Vx + x + (Vy + y) * kWidth;
						if(header.display[i] == 1) {
							VF = 1;
						}
						header.display[i] = header.display[i] ^ 1;
						changed[i] = changed[i] ^ 1;
					}
				}
			}

			static HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleCursorPosition(hStdout, {0,0});
			for(short y = 0; y < kHeight; ++y) {
				for(short x = 0; x < kWidth; ++x) {
					if(changed[x + y*kWidth]) {
						SetConsoleCursorPosition(hStdout, {x,y});
						printf("%c", header.display[x + y *kWidth] ? '#' : ' ');
					}
				}
			}
			drawn = true;
		}
		break;
		case 0xE:
			switch(op.nn) {
			case 0x9E: if(header.keys[Vx] != 0) header.pc += 2; break;
			case 0xA1: if(header.keys[Vx] == 0) header.pc += 2; break;
			default: throw exception::unknown_opcode{op};
			}
			break;
		case 0xF:
			switch(op.nn) {
			case 0x07: Vx = header.delayTimer; break;
			case 0x0A: break;
			case 0x15: header.delayTimer = Vx; break;
			case 0x18: header.soundTimer = Vx; break;
			case 0x1E: header.I += Vx; break;
			case 0x29: header.I = Vx * 5; break;
			case 0x33:
				memory[header.I] = Vx;
				memory[header.I + 1] = (Vx / 10) % 10;
				memory[header.I + 2] = Vx % 10;
				break;
			case 0x55: for(int i = 0; i <= op.data.x; ++i) memory[header.I + i] = header.V[i]; break;
			case 0x65: for(int i = 0; i <= op.data.x; ++i) header.V[i] = memory[header.I + i]; break;
			default: throw exception::unknown_opcode{op};
			}
			break;
		default: throw exception::unknown_opcode{op};
		}
		if(drawn || header.delayTimer > 0 || header.soundTimer > 0) std::this_thread::sleep_for(16ms);
		if(header.delayTimer > 0) header.delayTimer--;
		if(header.soundTimer > 0) header.soundTimer--;
	}

	void load(const char *name) {
		memset(memory, 0, sizeof(memory));
		std::ifstream file{name, std::ios::binary};
		if(!file.is_open()) {
			throw exception::file_not_found{};
		}
		file.read(reinterpret_cast<char*>(&memory[0x200]), 0xE00);
		header.pc = 0x200;
	}
};

int main(int argc, char *argv[]) try {
	system("cls");
	if(argc < 2) {
		return 0;
	}
	Chip8 chip8;
	chip8.load(argv[1]);
	for(;;) {
		chip8.execute_instruction();
	}
} catch(const exception::file_not_found &) {
	printf("file not found\n");
} catch(const exception::unknown_opcode &ex) {
	printf("unknown opcode: %#06x\n", ex.op.opcode);
}