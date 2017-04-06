/*
 * Nostalgic Z80 emulator
 * Copyright (c) 2016, Antonio Rodriguez <@MoebiuZ>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org\licenses\LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the reg.w.specific language governing permissions and
 * limitations under the License.
 */


#include "z80.h"


#define CHECKJUMP() \
{ \
	tstates_counter = a_set[i_set][op].tstates_nojmp; \
	mcycles_counter = a_set[i_set][op].mcycles_nojmp; \
	last_mcycle_tstates = a_set[i_set][op].last_mc_tstates_nojmp; \
	switch (a_set[i_set][op].checkjump) { \
		case 1: \
			if (Condition(a_set[i_set][op].operand1)) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
		case 2: \
			if (reg.b.b - 1) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
		case 3:	\
			if ((reg.w.bc - 1) != 0) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
		case 4:	\
			if ((reg.w.bc - 1) != 0 && (reg.b.a - READBYTE(reg.w.hl)) != 0) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
		case 5:	\
			if ((reg.b.b - 1) != 0 && (reg.b.a - READBYTE(reg.w.hl)) != 0) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
		case 6:	\
			if ((reg.b.b - 1) != 0) { \
				will_jump = 1; \
				tstates_counter = a_set[i_set][op].tstates; \
				mcycles_counter = a_set[i_set][op].mcycles; \
				last_mcycle_tstates = a_set[i_set][op].last_mc_tstates; \
			} \
			break; \
	} \
}


Z80::Z80() {
	SetIOReadCallback(std::bind(&Z80::IOReadPlaceholder, this, std::placeholders::_1));
	SetIOWriteCallback(std::bind(&Z80::IOWritePlaceholder, this, std::placeholders::_1, std::placeholders::_2));
#ifdef __Z80MEMCALLBACKS__
	SetMemReadCallback(std::bind(&Z80::MemReadPlaceholder, this, std::placeholders::_1));
	SetMemWriteCallback(std::bind(&Z80::MemWritePlaceholder, this, std::placeholders::_1, std::placeholders::_2));
#endif

#ifndef __Z80_BIG_ENDIAN__
	CorrectEndianess();
#endif
}


Z80::~Z80() {}


void Z80::CorrectEndianess() {

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 256; j++) {
		
			if ((endianess_fix[j + (256*i)] & 0x01) == 1) {
				if (a_set[i][j].operand1 & 0x01) {
					a_set[i][j].operand1--;
				} else {
					a_set[i][j].operand1++;
				}
			}

			if ((endianess_fix[j + (256*i)] & 0x02) == 2) {
				if (a_set[i][j].operand2 & 0x01) {
					a_set[i][j].operand2--;
				} else {
					a_set[i][j].operand2++;
				}
			}

			if ((endianess_fix[j + (256*i)] & 0x04) == 4) {
				if (a_set[i][j].operand3 & 0x01) {
					a_set[i][j].operand3--;
				} else {
					a_set[i][j].operand3++;
				}
			}
		}
	}
}


void Z80::NMI() {
	nmi = 1;
}


void Z80::IRQ0() {
	im = 0;
}


void Z80::IRQ1() {
	im = 1;
}


void Z80::IRQ2(ZBYTE vector) {
	im = 2;
	irq_vector = vector;
}


unsigned int Z80::isHalted() {
	return halted;
}

void Z80::SetIOReadCallback(std::function<ZBYTE(ZWORD)> cb) { 
	IOReadCallback = cb; 
}



void Z80::SetIOWriteCallback(std::function<void(ZWORD, ZBYTE)> cb) { 
	IOWriteCallback = cb; 
}



unsigned int Z80::ExecuteInstruction() {

	Run();
	(this->*current_instruction)();

	return mcycles_counter * 4; // Fix this

}



unsigned int Z80::ExecuteTStates(unsigned int ts) {

	tstates = 0;

	// TODO: If 3 or lest tstates, it will execute the first mcycle of next instruction
	while (tstates < ts) {
	
		switch (tstates_counter) {
			case 0:
				Run();
				break;

			case 1:
				(this->*current_instruction)();
				will_jump = 0;
				tstates++;
				tstates_counter = 0;
				break;

			default:
				tstates++;
				tstates_counter--;
				break;
		}
	}

	return tstates;
}



unsigned int Z80::ExecuteMCycle() {

	tstates = 0;
		
	switch (mcycles_counter) {
		case 0:
			Run();
			break;

		case 1:
			(this->*current_instruction)();
			will_jump = 0;
			tstates += last_mcycle_tstates;
			mcycles_counter = 0;
			break;

		default:
			tstates += 4;
			mcycles_counter--;
			break;
	}

	return tstates;
}



inline void Z80::Run() {
		
	// TODO: Handle stray DD and FD

			
	if (!nmi && !irq) {
			
		defer_irq = 0;

		if (!halted) {
			//mcycles_counter = 0;
			//tstates_counter = 0;

			op = READBYTE(pc++);

			reg.b.r++;

			switch (op) {
				case 0xCB:
					i_set = 1;
					reg.b.r++;
					op = READBYTE(pc++);

					current_instruction = cb_instructions[op];
					CHECKJUMP();
					break;
					
				case 0xED:
					i_set = 2;
					ED_Exec();
					break;
					
				case 0xDD:
					i_set = 3;
					DD_Exec();
					break;
					
				case 0xFD:
					i_set = 4;
					FD_Exec();
					break;

				default:
					i_set = 0;
					current_instruction = main_instructions[op];
					CHECKJUMP();
					break;
			}

		} else {
			// Execute NOPs while halted
			last_mcycle_tstates = tstates_counter = 4;
			mcycles_counter = 1;

			current_instruction = main_instructions[0x00];
		}

	} else if (nmi) {
		NonMaskableInterrupt();
	} else if (irq && !defer_irq && iff1) {
		MaskableInterrupt();
	}
}



void Z80::ED_Exec() {

	op = READBYTE(pc++);

	//if (op == 0xCB || op == 0xDD || op == 0xED || op == 0xFD) {

		// EDCB, EDDD, EDED and EDFD will be ignored because they lie in the inoperative fourth quarter of the ED set
	//	return;
	//}

	reg.b.r++;
	current_instruction = ed_instructions[op];
	CHECKJUMP();
}



void Z80::DD_Exec() {
		
	op = READBYTE(pc++);
	
	switch (op) { // DDCB
		case 0xCB:
			i_set = 5;
			reg.b.r++;
			op = READBYTE(pc++ + 1);
			current_instruction = ddcb_instructions[op];
			CHECKJUMP();
			break;

		case 0xED:	// if ED, DD is ignored
			pc--;
			tstates_counter += 4;
			break;

		case 0xDD:
			pc--;
			tstates_counter += 4;
			break;

		case 0xFD:
			pc--;
			tstates_counter += 4;
			break;

		default:
			reg.b.r++;
			current_instruction = dd_instructions[op];
			CHECKJUMP();
			break;
	}

}


void Z80::FD_Exec() {

	op = READBYTE(pc++);

	switch (op) {
		case 0xCB:  // FDCB
			i_set = 6;
			reg.b.r++;
			op = READBYTE(pc++ + 1);   // 3 and 4 operand are inverted in FDCB
			current_instruction = fdcb_instructions[op];
			CHECKJUMP();
			break;

		case 0xED:		// if ED, FD is ignored
			ED_Exec();
			break;

		case 0xDD:
			DD_Exec();
			break;

		case 0xFD:
			FD_Exec();
			break;

		default:
			reg.b.r++;
			current_instruction = fd_instructions[op];
			CHECKJUMP();
			break;
	
	}
}


void Z80::NonMaskableInterrupt() {

	reg.b.r++;
	halted = 0;
	pc++;

	iff2 = iff1;
	iff1 = 0;
	/* Push */
	reg.w.sp -= 2;
	WRITEWORD(reg.w.sp, pc);
	/* Push */
	pc = 0x0066;
	tstates += 5;
}



void Z80::MaskableInterrupt() {

	ZWORD vector_addr;
	reg.b.r++;
	halted = 0;
	pc++;

	//int_req = 0;

	iff1 = iff2 = 0;
		
	switch (im) {
		case 0:

			// TODO: IM 0 is tricky

		//	tstates += 6;
			break;
		case 1:
			/* Push */
			reg.w.sp -= 2;
			WRITEWORD(reg.w.sp, pc);
			/* Push */
			pc = 0x0038;
		//	tstates += 7;
			break;
		case 2:
			/* Push */
			reg.w.sp -= 2;
			WRITEWORD(reg.w.sp, pc);
			/* Push */
				
			vector_addr = (reg.b.i << 8) | irq_vector;
			pc = READWORD(vector_addr);
			//tstates += 7;
			break;
	}

	tstates += 7; // ACK interrupt
}


	
void Z80::Reset() {
	iff1 = iff2 = im = 0;
	pc = 0x0000;
	reg.w.ir = 0x0000;
	reg.w.sp = 0xffff;
	reg.w.af = 0xffff;

	halted = 0;

	tstates_counter = 0;
	mcycles_counter = 0;
	last_mcycle_tstates = 0;
}


#ifdef __Z80MEMCALLBACKS__
ZWORD Z80::ReadWord(ZWORD addr) {
	ZBYTE lsb = READBYTE(addr);
	ZBYTE msb = READBYTE(addr + 1);
	return msb << 8 | lsb;
}


void Z80::WriteWord(ZWORD addr, ZWORD val) {
	WRITEBYTE(addr, val);
	WRITEBYTE(addr + 1, val >> 8);
}
#endif


ZBYTE Z80::ReadIO(ZWORD addr) {
	ioreq = 1;
	IOReadCallback(addr);
	return addr >> 8;
}


void Z80::WriteIO(ZWORD addr, ZBYTE val) {
	ioreq = 2;
	IOWriteCallback(addr, val);
}



ZBYTE Z80::IOReadPlaceholder(ZWORD addr) {
	printf("IO Read Callback not set\n");
	return addr << 8;
}



void Z80::IOWritePlaceholder(ZWORD addr, ZBYTE data) {
	printf("IO Write Callback not set\n");
}


#ifdef __Z80MEMCALLBACKS__
	
void Z80::SetMemReadCallback(std::function<ZBYTE(ZWORD)> cb) { 
	MemReadCallback = cb; 
}



void Z80::SetMemWriteCallback(std::function<void(ZWORD, ZBYTE)> cb) { 
	MemWriteCallback = cb; 
}



ZBYTE Z80::MemReadPlaceholder(ZWORD addr) {
//	printf("Mem Read Callback not set\n");
	return (ZBYTE) memory[addr];
}



void Z80::MemWritePlaceholder(ZWORD addr, ZBYTE data) {
//	printf("Mem Write Callback not set\n");
	memory[addr] = data;
}

#endif