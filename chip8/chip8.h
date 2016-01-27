#pragma once

#include <cstdint>
#include <bitset>

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

bool operator==(const Opcode& a, const Opcode& b);

class Chip8 {
	union {
		uint8_t memory[4096];
		struct {
			uint8_t V[16], keys[16], delayTimer, soundTimer, fontData[16*5];
			uint16_t I, stack[16], sp, pc;
			std::bitset<kWidth*kHeight> display;
		} header;

	};

public:
	Chip8() {}
	void execute_instruction();
	void init();
	void load(const char *name);
};

namespace exception {
	struct file_not_found {};
	struct stack_overflow {};
	struct unknown_opcode { Opcode op; };
}