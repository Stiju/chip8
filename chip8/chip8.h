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
			uint8_t V[16], keys[16], delay_timer, sound_timer, font_data[16 * 5], display_updated;
			uint16_t I, stack[16], sp, pc;
			std::bitset<kWidth*kHeight> display;
		} header;

	};

public:
	Chip8() {}
	void execute_instruction();
	void init();
	void load(const char *name);
	const auto& get_display() const { return header.display; }
	bool display_updated() {
		if(!header.display_updated) return false;
		header.display_updated = false;
		return true;
	}
};

namespace exception {
	struct file_not_found {};
	struct stack_overflow {};
	struct unknown_opcode { Opcode op; };
}