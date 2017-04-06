/*
 * Nostalgic Z80 emulator
 * Copyright (c) 2016, Antonio Rodriguez <@MoebiuZ>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "zextest.h"


#define Z80_CPU_SPEED		   4000000   /* In Hz. */
#define CYCLES_PER_STEP		 (Z80_CPU_SPEED / 50)
#define MAXIMUM_STRING_LENGTH   100


Z80Test::Z80Test() { }

Z80Test::~Z80Test() {
	delete cpu;
}

void Z80Test::Init() {
	cpu = new Z80();
	cpu->memory = memory;
	cpu->SetIOReadCallback(std::bind(&Z80Test::IOReadCallback, this, std::placeholders::_1));
	cpu->SetIOWriteCallback(std::bind(&Z80Test::IOWriteCallback, this, std::placeholders::_1, std::placeholders::_2));
}



void Z80Test::Emulate(char *filename) {
	FILE			*file;
	long			l;
	
	double		  total;

	for (int i = 0; i < 0xffff + 1; i++) {
		cpu->memory[i] = 0x00; 
	}

	if ((file = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "Can't open file!\n");
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	l = ftell(file);

	fseek(file, 0, SEEK_SET);
	fread(memory + 0x100, 1, l, file);

	fclose(file);


	cpu->Reset();

	memory[0] = 0xd3; // OUT N, A
	memory[1] = 0x00;

	memory[5] = 0xdb; // IN A, N
	memory[6] = 0x00;
	memory[7] = 0xc9; // RET

	cpu->pc = 0x100;
		
	total = 0.0;
		
	
	while (1) {
		total += cpu->ExecuteMCycle();
	
		if (cpu->isHalted() != 0) {
			break;
		}
	}


	printf("\n%.0f cycle(s) emulated.\n" 
				"For a Z80 running at %.2fMHz, "
				"that would be %d second(s) or %.2f hour(s).\n",
				total,
				Z80_CPU_SPEED / 1000000.0,
				(int) (total / Z80_CPU_SPEED),
				total / ((double) 3600 * Z80_CPU_SPEED));
}




ZBYTE Z80Test::IOReadCallback(ZWORD addr) {
	SystemCall();
	return addr << 8;
}
	
void Z80Test::IOWriteCallback(ZWORD addr, ZBYTE data) {
	SystemCall();
	cpu->halted = 1;
}

// Emulate CP/M bdos call 5 functions 2 (output character on screen) and 9
// (output $-terminated string to screen).

void Z80Test::SystemCall() {
	if (cpu->reg.b.c == 2) {
		printf("%c", cpu->reg.b.e);
	} else if (cpu->reg.b.c == 9) {
		int	 i, c;

		for (i = cpu->reg.w.de, c = 0; memory[i] != '$'; i++) {
			printf("%c", memory[i & 0xffff]);
				if (c++ > MAXIMUM_STRING_LENGTH) {
					fprintf(stderr,	"String to print is too long!\n");
					exit(1);
				}
		}

	}
}


int main (void) {

	Z80Test *test = new Z80Test();

	test->Init();

	printf("Testing documented instructions and effects (zexdoc.com)\n");
	test->Emulate((char *) "./test/zexdoc.com");

	printf("\n\nTesting undocumented instructions and effects (zexall.com)\n");
	test->Emulate((char *) "./test/zexall.com");
		
	return 0;
}