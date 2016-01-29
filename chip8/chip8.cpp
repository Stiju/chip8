#include "chip8.h"

#include <random>
#include <fstream>

const uint8_t kFontData[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80,
};

const uint16_t kEntryPoint = 0x200;

uint8_t random() {
	static std::mt19937 randomEngine{};
	static std::uniform_int_distribution<> gen(0, 255);
	return static_cast<uint8_t>(gen(randomEngine));
}

bool operator==(const Opcode& a, const Opcode& b) {
	return a.opcode == b.opcode;
}

void Chip8::execute_instruction() {
	Opcode op = memory[header.pc] << 8 | memory[header.pc + 1];
	header.pc += 2;
	uint8_t &Vx = header.V[op.data.x], &Vy = header.V[op.data.y], &V0 = header.V[0], &VF = header.V[0xF];
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
		case 0x5: VF = Vy <= Vx; Vx -= Vy; break;
		case 0x6: VF = Vx & 1; Vx >>= 1; break;
		case 0x7: VF = Vx <= Vy; Vx = Vy - Vx; break;
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
		VF = 0;
		for(uint16_t y = 0; y < op.data.n; ++y) {
			uint8_t sprite = memory[header.I + y];
			for(uint16_t x = 0; x < 8; ++x) {
				if(sprite & (0x80 >> x)) {
					uint16_t i = ((Vx + x) % kWidth) + ((Vy + y) % kHeight) * kWidth;
					if(header.display[i] == 1) {
						VF = 1;
					}
					header.display[i] = header.display[i] ^ 1;
				}
			}
		}
		header.display_updated = true;
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
		case 0x07: Vx = header.delay_timer; break;
		case 0x0A: {
			bool key_pressed = false;
			for(uint8_t i = 0; i < 16; ++i) {
				if(header.keys[i] != 0) {
					Vx = i;
					key_pressed = true;
					break;
				}
			}
			if(!key_pressed) {
				header.pc -= 2;
				return;
			}
		} break;
		case 0x15: header.delay_timer = Vx; break;
		case 0x18: header.sound_timer = Vx; break;
		case 0x1E: header.I += Vx; break;
		case 0x29: header.I = static_cast<uint16_t>(header.font_data - memory) + Vx * 5; break;
		case 0x33:
			memory[header.I] = Vx / 100;
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
	if(header.delay_timer > 0) header.delay_timer--;
	if(header.sound_timer > 0) header.sound_timer--;
}

void Chip8::init() {
	memset(memory, 0, sizeof(memory));
	header.pc = kEntryPoint;
	memcpy(header.font_data, kFontData, sizeof(kFontData));
	header.display_updated = true;
}

void Chip8::load(const char *name) {
	init();
	std::ifstream file{name, std::ios::binary};
	if(!file.is_open()) {
		throw exception::file_not_found{};
	}
	file.read(reinterpret_cast<char*>(&memory[kEntryPoint]), sizeof(memory) - kEntryPoint);
}