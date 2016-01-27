#include "chip8.h"

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