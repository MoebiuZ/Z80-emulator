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


#ifndef Z80_EMULATOR_H_
#define Z80_EMULATOR_H_

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

#include <stdio.h>
#include <functional>


typedef unsigned char ZBYTE;
typedef unsigned short ZWORD;
typedef unsigned long ZDWORD;

typedef ZBYTE Z80ADDRESSBUS[0xffff + 1];

#ifdef __Z80MEMCALLBACKS__

#define OPCODE(addr) MemReadCallback(addr)
#define READBYTE(addr) MemReadCallback(addr)
#define WRITEBYTE(addr, val) MemWriteCallback(addr, val)
#define READWORD(addr) ReadWord(addr)
#define WRITEWORD(addr, val) WriteWord(addr, val)

#else

#define OPCODE(addr) memory[addr]
#define READBYTE(addr) memory[addr]
#define WRITEBYTE(addr, val) memory[addr] = val
#define READWORD(addr) ((memory[addr + 1] << 8) | memory[addr])
#define WRITEWORD(addr, val) \
{ \
	memory[addr] = val; \
	memory[addr + 1] = val >> 8; \
}

#endif



class Z80 {

#ifdef __Z80TEST__
friend class Z80Test;
#endif

public:

	
	ZBYTE *memory;   	// Pointer to Z80ADDRESSBUS



	Z80();
	~Z80();

	unsigned int ExecuteInstruction();
	unsigned int ExecuteTStates(unsigned int ts);
	unsigned int ExecuteMCycle();

	void NMI();
	void IRQ0();
	void IRQ1();
	void IRQ2(ZBYTE vector);

	unsigned int isHalted();

	void Reset();

	void SetIOReadCallback(std::function<ZBYTE(ZWORD)> cb);
	void SetIOWriteCallback(std::function<void(ZWORD, ZBYTE)> cb);
	#ifdef __Z80MEMCALLBACKS__
	void SetMemReadCallback(std::function<ZBYTE(ZWORD)> cb);
	void SetMemWriteCallback(std::function<void(ZWORD, ZBYTE)> cb);
	#endif
	
	
private:

	std::function<ZBYTE(ZWORD)> IOReadCallback;
	std::function<void(ZWORD, ZBYTE)> IOWriteCallback;

	std::function<ZBYTE(ZWORD)> MemReadCallback;
	std::function<void(ZWORD, ZBYTE)> MemWriteCallback;

	void ED_Exec();
	void FD_Exec();
	void DD_Exec();

	typedef void (Z80::*OPCODES)();
	
	int ioreq = 0;

	unsigned int halted;
	bool nmi; 			// Non-maskable interruption
	bool irq; 			// Maskable interruption

	typedef union {

		struct {
			ZWORD af, bc, de, hl, ix, iy, sp, ir, wz; // WZ is an undocumented internal register (also known as MEMPTR)
		} w;

		struct {
			#ifndef _Z80_BIG_ENDIAN_
			ZBYTE f, a, c, b, e, d, l, h, ixl, ixh, iyl, iyh, p, s, r, i, z, w;
			#else
			ZBYTE a, f, b, c, d, e, h, l, ixh, ixl, iyh, iyl, s, p, i, r, w, z;
			#endif
		} b;

		ZWORD pairs[9];
		ZBYTE registers[18];
	} Z80REGISTERS;


	typedef struct {
		ZBYTE operand1;
		ZBYTE operand2;
		ZBYTE operand3;
 		
		ZBYTE tstates;
		ZBYTE mcycles;
		ZBYTE tstates_nojmp;
		ZBYTE mcycles_nojmp;
		ZBYTE last_mc_tstates;
		ZBYTE last_mc_tstates_nojmp;
		
		ZBYTE checkjump;
	} ARGUMENTS;

	typedef ARGUMENTS ARGUMENT_SETS[256];


	Z80REGISTERS reg, alt_reg;

	ZWORD pc;

	unsigned int iff1;
	unsigned int iff2;
	unsigned int im;

	unsigned int tstates;

	ZBYTE defer_irq;

	ZBYTE irq_vector;

	int const parity[256] = {
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 };



	ZBYTE op;
	int i_set = 0;

	int mcycles_counter = 0;

	int tstates_counter = 0;

	int will_jump = 0;
	int last_mcycle_tstates = 0;
	void (Z80::*current_instruction)();


	void NonMaskableInterrupt();
	void MaskableInterrupt();
	inline void Run();


	void CorrectEndianess();

#ifdef __Z80MEMCALLBACKS__	
	ZWORD ReadWord(ZWORD addr);

	void WriteWord(ZWORD addr, ZWORD val);
#endif

	ZBYTE ReadIO(ZWORD addr);

	void WriteIO(ZWORD addr, ZBYTE val);

	int Condition(ZBYTE cond);


	ZBYTE IOReadPlaceholder(ZWORD addr);
	void IOWritePlaceholder(ZWORD addr, ZBYTE data);

#ifdef __Z80MEMCALLBACKS__
	ZBYTE MemReadPlaceholder(ZWORD addr);
	void MemWritePlaceholder(ZWORD addr, ZBYTE data);
#endif

	const ZBYTE SZ_table[256] = {

		0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
		0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
		0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
		0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
		0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
		0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
		0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 
		0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 
		0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 
		0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 
		0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
		0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
		0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 
		0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 
		0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 
		0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 
		0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8	};


	const ZBYTE SZP_table[256] = {

		0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 
		0x08, 0x0c, 0x0c, 0x08, 0x0c, 0x08, 0x08, 0x0c, 
		0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 
		0x0c, 0x08, 0x08, 0x0c, 0x08, 0x0c, 0x0c, 0x08, 
		0x20, 0x24, 0x24, 0x20, 0x24, 0x20, 0x20, 0x24, 
		0x2c, 0x28, 0x28, 0x2c, 0x28, 0x2c, 0x2c, 0x28, 
		0x24, 0x20, 0x20, 0x24, 0x20, 0x24, 0x24, 0x20, 
		0x28, 0x2c, 0x2c, 0x28, 0x2c, 0x28, 0x28, 0x2c, 
		0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 
		0x0c, 0x08, 0x08, 0x0c, 0x08, 0x0c, 0x0c, 0x08, 
		0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 
		0x08, 0x0c, 0x0c, 0x08, 0x0c, 0x08, 0x08, 0x0c, 
		0x24, 0x20, 0x20, 0x24, 0x20, 0x24, 0x24, 0x20, 
		0x28, 0x2c, 0x2c, 0x28, 0x2c, 0x28, 0x28, 0x2c, 
		0x20, 0x24, 0x24, 0x20, 0x24, 0x20, 0x20, 0x24, 
		0x2c, 0x28, 0x28, 0x2c, 0x28, 0x2c, 0x2c, 0x28, 
		0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 
		0x8c, 0x88, 0x88, 0x8c, 0x88, 0x8c, 0x8c, 0x88, 
		0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 
		0x88, 0x8c, 0x8c, 0x88, 0x8c, 0x88, 0x88, 0x8c, 
		0xa4, 0xa0, 0xa0, 0xa4, 0xa0, 0xa4, 0xa4, 0xa0, 
		0xa8, 0xac, 0xac, 0xa8, 0xac, 0xa8, 0xa8, 0xac, 
		0xa0, 0xa4, 0xa4, 0xa0, 0xa4, 0xa0, 0xa0, 0xa4, 
		0xac, 0xa8, 0xa8, 0xac, 0xa8, 0xac, 0xac, 0xa8, 
		0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 
		0x88, 0x8c, 0x8c, 0x88, 0x8c, 0x88, 0x88, 0x8c, 
		0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 
		0x8c, 0x88, 0x88, 0x8c, 0x88, 0x8c, 0x8c, 0x88, 
		0xa0, 0xa4, 0xa4, 0xa0, 0xa4, 0xa0, 0xa0, 0xa4, 
		0xac, 0xa8, 0xa8, 0xac, 0xa8, 0xac, 0xac, 0xa8, 
		0xa4, 0xa0, 0xa0, 0xa4, 0xa0, 0xa4, 0xa4, 0xa0, 
		0xa8, 0xac, 0xac, 0xa8, 0xac, 0xa8, 0xa8, 0xac };

	const int V_table[4] = { 0, 4, 4, 0, };
	
	const OPCODES main_instructions[256] = {
		&Z80::NOP,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLCA,
		&Z80::EX_RR_altRR,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRCA,
		&Z80::DJNZ,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLA,
		&Z80::JR_d,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRA,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_RR,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::DAA,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_RR_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CPL,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_R,&Z80::INC_RR,&Z80::INC_ind,&Z80::DEC_ind,&Z80::LD_ind_n,&Z80::SCF,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_R_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CCF,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::LD_ind_R,&Z80::LD_ind_R,&Z80::LD_ind_R,&Z80::LD_ind_R,&Z80::LD_ind_R,&Z80::LD_ind_R,&Z80::HALT,&Z80::LD_ind_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_ind,&Z80::LD_R_R,
		&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_HL,&Z80::ADD_R_R,
		&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_HL,&Z80::ADC_R_R,
		&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_HL,&Z80::SUB_R,
		&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_HL,&Z80::SBC_R_R,
		&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_HL,&Z80::AND_R,
		&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_HL,&Z80::XOR_R,
		&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_HL,&Z80::OR_R,
		&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_HL,&Z80::CP_R,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::JP_nn,&Z80::CALL_cond,&Z80::PUSH,&Z80::ADD_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::RET,&Z80::JP_cond,&Z80::NOP,&Z80::CALL_cond,&Z80::CALL,&Z80::ADC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::OUT_n_R,&Z80::CALL_cond,&Z80::PUSH,&Z80::SUB_n,&Z80::RST,
		&Z80::RET_cond,&Z80::EXX,&Z80::JP_cond,&Z80::IN_R_n,&Z80::CALL_cond,&Z80::NOP,&Z80::SBC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::EX_SP_RR,&Z80::CALL_cond,&Z80::PUSH,&Z80::AND_n,&Z80::RST,
		&Z80::RET_cond,&Z80::JP_ind,&Z80::JP_cond,&Z80::EX_RR_RR,&Z80::CALL_cond,&Z80::NOP,&Z80::XOR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::DI,&Z80::CALL_cond,&Z80::PUSH,&Z80::OR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::LD_RR_RR,&Z80::JP_cond,&Z80::EI,&Z80::CALL_cond,&Z80::NOP,&Z80::CP_n,&Z80::RST };


	const OPCODES cb_instructions[256] = {
		&Z80::RLC_R,&Z80::RLC_R,&Z80::RLC_R,&Z80::RLC_R,&Z80::RLC_R,&Z80::RLC_R,&Z80::RLC_HL,&Z80::RLC_R,
		&Z80::RRC_R,&Z80::RRC_R,&Z80::RRC_R,&Z80::RRC_R,&Z80::RRC_R,&Z80::RRC_R,&Z80::RRC_HL,&Z80::RRC_R,
		&Z80::RL_R,&Z80::RL_R,&Z80::RL_R,&Z80::RL_R,&Z80::RL_R,&Z80::RL_R,&Z80::RL_HL,&Z80::RL_R,
		&Z80::RR_R,&Z80::RR_R,&Z80::RR_R,&Z80::RR_R,&Z80::RR_R,&Z80::RR_R,&Z80::RR_HL,&Z80::RR_R,
		&Z80::SLA_R,&Z80::SLA_R,&Z80::SLA_R,&Z80::SLA_R,&Z80::SLA_R,&Z80::SLA_R,&Z80::SLA_HL,&Z80::SLA_R,
		&Z80::SRA_R,&Z80::SRA_R,&Z80::SRA_R,&Z80::SRA_R,&Z80::SRA_R,&Z80::SRA_R,&Z80::SRA_HL,&Z80::SRA_R,
		&Z80::SLL_R,&Z80::SLL_R,&Z80::SLL_R,&Z80::SLL_R,&Z80::SLL_R,&Z80::SLL_R,&Z80::SLL_HL,&Z80::SLL_R,
		&Z80::SRL_R,&Z80::SRL_R,&Z80::SRL_R,&Z80::SRL_R,&Z80::SRL_R,&Z80::SRL_R,&Z80::SRL_HL,&Z80::SRL_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_R,&Z80::BIT_n_HL,&Z80::BIT_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_R,&Z80::RES_n_HL,&Z80::RES_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R,
		&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_R,&Z80::SET_n_HL,&Z80::SET_n_R };


	const OPCODES ed_instructions[256] = {
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::SBC_RR_RR,&Z80::LD_addr_RR,&Z80::NEG,&Z80::RETN,&Z80::IM,&Z80::LD_R_R,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::ADC_RR_RR,&Z80::LD_RR_addr,&Z80::NEG,&Z80::RETI,&Z80::IM,&Z80::LD_R_R,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::SBC_RR_RR,&Z80::LD_addr_RR,&Z80::NEG,&Z80::RETN,&Z80::IM,&Z80::LD_R_spec,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::ADC_RR_RR,&Z80::LD_RR_addr,&Z80::NEG,&Z80::RETI,&Z80::IM,&Z80::LD_R_spec,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::SBC_RR_RR,&Z80::LD_addr_RR,&Z80::NEG,&Z80::RETN,&Z80::IM,&Z80::RRD,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::ADC_RR_RR,&Z80::LD_RR_addr,&Z80::NEG,&Z80::RETI,&Z80::IM,&Z80::RLD,
		&Z80::IN_R_c,&Z80::OUT_c_0,&Z80::SBC_RR_RR,&Z80::LD_addr_RR,&Z80::NEG,&Z80::RETN,&Z80::IM,&Z80::NOP,
		&Z80::IN_R_c,&Z80::OUT_c_R,&Z80::ADC_RR_RR,&Z80::LD_RR_addr,&Z80::NEG,&Z80::RETI,&Z80::IM,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::LDI,&Z80::CPI,&Z80::INI,&Z80::OUTI,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::LDD,&Z80::CPD,&Z80::IND,&Z80::OUTD,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::LDIR,&Z80::CPIR,&Z80::INIR,&Z80::OTIR,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::LDDR,&Z80::CPDR,&Z80::INDR,&Z80::OTDR,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,
		&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP,&Z80::NOP };


	const OPCODES dd_instructions[256] = {
		&Z80::NOP,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLCA,
		&Z80::EX_RR_altRR,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRCA,
		&Z80::DJNZ,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLA,
		&Z80::JR_d,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRA,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_RR,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::DAA,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_RR_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CPL,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_R,&Z80::INC_RR,&Z80::INC_off,&Z80::DEC_off,&Z80::LD_off_n,&Z80::SCF,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_R_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CCF,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::HALT,&Z80::LD_off_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_off,&Z80::ADD_R_R,
		&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_off,&Z80::ADC_R_R,
		&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_off,&Z80::SUB_R,
		&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_off,&Z80::SBC_R_R,
		&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_off,&Z80::AND_R,
		&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_off,&Z80::XOR_R,
		&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_off,&Z80::OR_R,
		&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_off,&Z80::CP_R,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::JP_nn,&Z80::CALL_cond,&Z80::PUSH,&Z80::ADD_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::RET,&Z80::JP_cond,&Z80::NOP,&Z80::CALL_cond,&Z80::CALL,&Z80::ADC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::OUT_n_R,&Z80::CALL_cond,&Z80::PUSH,&Z80::SUB_n,&Z80::RST,
		&Z80::RET_cond,&Z80::EXX,&Z80::JP_cond,&Z80::IN_R_n,&Z80::CALL_cond,&Z80::NOP,&Z80::SBC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::EX_SP_RR,&Z80::CALL_cond,&Z80::PUSH,&Z80::AND_n,&Z80::RST,
		&Z80::RET_cond,&Z80::JP_ind,&Z80::JP_cond,&Z80::EX_RR_RR,&Z80::CALL_cond,&Z80::NOP,&Z80::XOR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::DI,&Z80::CALL_cond,&Z80::PUSH,&Z80::OR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::LD_RR_RR,&Z80::JP_cond,&Z80::EI,&Z80::CALL_cond,&Z80::NOP,&Z80::CP_n,&Z80::RST };


	const OPCODES fd_instructions[256] = {
		&Z80::NOP,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLCA,
		&Z80::EX_RR_altRR,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRCA,
		&Z80::DJNZ,&Z80::LD_RR_nn,&Z80::LD_ind_R,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RLA,
		&Z80::JR_d,&Z80::ADD_RR_RR,&Z80::LD_R_ind,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::RRA,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_RR,&Z80::INC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::DAA,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_RR_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CPL,
		&Z80::JR_cond_d,&Z80::LD_RR_nn,&Z80::LD_addr_R,&Z80::INC_RR,&Z80::INC_off,&Z80::DEC_off,&Z80::LD_off_n,&Z80::SCF,
		&Z80::JR_cond_d,&Z80::ADD_RR_RR,&Z80::LD_R_addr,&Z80::DEC_RR,&Z80::INC_R,&Z80::DEC_R,&Z80::LD_R_n,&Z80::CCF,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::LD_off_R,&Z80::HALT,&Z80::LD_off_R,
		&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_R,&Z80::LD_R_off,&Z80::LD_R_R,
		&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_R,&Z80::ADD_R_off,&Z80::ADD_R_R,
		&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_R,&Z80::ADC_R_off,&Z80::ADC_R_R,
		&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_R,&Z80::SUB_off,&Z80::SUB_R,
		&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_R,&Z80::SBC_R_off,&Z80::SBC_R_R,
		&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_R,&Z80::AND_off,&Z80::AND_R,
		&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_R,&Z80::XOR_off,&Z80::XOR_R,
		&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_R,&Z80::OR_off,&Z80::OR_R,
		&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_R,&Z80::CP_off,&Z80::CP_R,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::JP_nn,&Z80::CALL_cond,&Z80::PUSH,&Z80::ADD_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::RET,&Z80::JP_cond,&Z80::NOP,&Z80::CALL_cond,&Z80::CALL,&Z80::ADC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::OUT_n_R,&Z80::CALL_cond,&Z80::PUSH,&Z80::SUB_n,&Z80::RST,
		&Z80::RET_cond,&Z80::EXX,&Z80::JP_cond,&Z80::IN_R_n,&Z80::CALL_cond,&Z80::NOP,&Z80::SBC_R_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::EX_SP_RR,&Z80::CALL_cond,&Z80::PUSH,&Z80::AND_n,&Z80::RST,
		&Z80::RET_cond,&Z80::JP_ind,&Z80::JP_cond,&Z80::EX_RR_RR,&Z80::CALL_cond,&Z80::NOP,&Z80::XOR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::POP,&Z80::JP_cond,&Z80::DI,&Z80::CALL_cond,&Z80::PUSH,&Z80::OR_n,&Z80::RST,
		&Z80::RET_cond,&Z80::LD_RR_RR,&Z80::JP_cond,&Z80::EI,&Z80::CALL_cond,&Z80::NOP,&Z80::CP_n,&Z80::RST };


	const OPCODES ddcb_instructions[256] = {
		&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off,&Z80::RLC_off_R,
		&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off,&Z80::RRC_off_R,
		&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off,&Z80::RL_off_R,
		&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off,&Z80::RR_off_R,
		&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off,&Z80::SLA_off_R,
		&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off,&Z80::SRA_off_R,
		&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off,&Z80::SLL_off_R,
		&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off,&Z80::SRL_off_R,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R };


	const OPCODES fdcb_instructions[256] = {
		&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off_R,&Z80::RLC_off,&Z80::RLC_off_R,
		&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off_R,&Z80::RRC_off,&Z80::RRC_off_R,
		&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off_R,&Z80::RL_off,&Z80::RL_off_R,
		&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off_R,&Z80::RR_off,&Z80::RR_off_R,
		&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off_R,&Z80::SLA_off,&Z80::SLA_off_R,
		&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off_R,&Z80::SRA_off,&Z80::SRA_off_R,
		&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off_R,&Z80::SLL_off,&Z80::SLL_off_R,
		&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off_R,&Z80::SRL_off,&Z80::SRL_off_R,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,&Z80::BIT_n_off,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off_R,&Z80::RES_n_off,&Z80::RES_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R,
		&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off_R,&Z80::SET_n_off,&Z80::SET_n_off_R };






ARGUMENT_SETS a_set[7] = {
		{ {0,0,0,4,1,4,1,4,4,0},{1,0,0,10,3,10,3,2,2,0},{1,0,0,7,2,7,2,3,3,0},{1,0,0,6,2,6,2,2,2,0},{2,0,0,4,1,4,1,4,4,0},{2,0,0,4,1,4,1,4,4,0},{2,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{3,1,0,11,3,11,3,3,3,0},{0,1,0,7,2,7,2,3,3,0},{1,0,0,6,2,6,2,2,2,0},{3,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,13,3,8,2,5,4,2},{2,0,0,10,3,10,3,2,2,0},{2,0,0,7,2,7,2,3,3,0},{2,0,0,6,2,6,2,2,2,0},{4,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{4,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,12,3,12,3,4,4,0},{3,2,0,11,3,11,3,3,3,0},{0,2,0,7,2,7,2,3,3,0},{2,0,0,6,2,6,2,2,2,0},{5,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{5,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{1,0,0,12,3,7,2,4,3,1},{3,0,0,10,3,10,3,2,2,0},{0,3,0,16,4,16,4,4,4,0},{3,0,0,6,2,6,2,2,2,0},{6,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{6,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,12,3,7,2,4,3,1},{3,3,0,11,3,11,3,3,3,0},{3,0,0,16,4,16,4,4,4,0},{3,0,0,6,2,6,2,2,2,0},{7,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{7,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{6,0,0,10,3,10,3,2,2,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,6,2,6,2,2,2,0},{3,0,0,11,3,11,3,3,3,0},{3,0,0,11,3,11,3,3,3,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{3,6,0,11,3,11,3,3,3,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,6,2,6,2,2,2,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,2,0,4,1,4,1,4,4,0},{2,3,0,4,1,4,1,4,4,0},{2,4,0,4,1,4,1,4,4,0},{2,5,0,4,1,4,1,4,4,0},{2,6,0,4,1,4,1,4,4,0},{2,7,0,4,1,4,1,4,4,0},{2,3,0,7,2,7,2,3,3,0},{2,0,0,4,1,4,1,4,4,0},
		{3,2,0,4,1,4,1,4,4,0},{3,3,0,4,1,4,1,4,4,0},{3,4,0,4,1,4,1,4,4,0},{3,5,0,4,1,4,1,4,4,0},{3,6,0,4,1,4,1,4,4,0},{3,7,0,4,1,4,1,4,4,0},{3,3,0,7,2,7,2,3,3,0},{3,0,0,4,1,4,1,4,4,0},
		{4,2,0,4,1,4,1,4,4,0},{4,3,0,4,1,4,1,4,4,0},{4,4,0,4,1,4,1,4,4,0},{4,5,0,4,1,4,1,4,4,0},{4,6,0,4,1,4,1,4,4,0},{4,7,0,4,1,4,1,4,4,0},{4,3,0,7,2,7,2,3,3,0},{4,0,0,4,1,4,1,4,4,0},
		{5,2,0,4,1,4,1,4,4,0},{5,3,0,4,1,4,1,4,4,0},{5,4,0,4,1,4,1,4,4,0},{5,5,0,4,1,4,1,4,4,0},{5,6,0,4,1,4,1,4,4,0},{5,7,0,4,1,4,1,4,4,0},{5,3,0,7,2,7,2,3,3,0},{5,0,0,4,1,4,1,4,4,0},
		{6,2,0,4,1,4,1,4,4,0},{6,3,0,4,1,4,1,4,4,0},{6,4,0,4,1,4,1,4,4,0},{6,5,0,4,1,4,1,4,4,0},{6,6,0,4,1,4,1,4,4,0},{6,7,0,4,1,4,1,4,4,0},{6,3,0,7,2,7,2,3,3,0},{6,0,0,4,1,4,1,4,4,0},
		{7,2,0,4,1,4,1,4,4,0},{7,3,0,4,1,4,1,4,4,0},{7,4,0,4,1,4,1,4,4,0},{7,5,0,4,1,4,1,4,4,0},{7,6,0,4,1,4,1,4,4,0},{7,7,0,4,1,4,1,4,4,0},{7,3,0,7,2,7,2,3,3,0},{7,0,0,4,1,4,1,4,4,0},
		{3,2,0,7,2,7,2,3,3,0},{3,3,0,7,2,7,2,3,3,0},{3,4,0,7,2,7,2,3,3,0},{3,5,0,7,2,7,2,3,3,0},{3,6,0,7,2,7,2,3,3,0},{3,7,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},
		{0,2,0,4,1,4,1,4,4,0},{0,3,0,4,1,4,1,4,4,0},{0,4,0,4,1,4,1,4,4,0},{0,5,0,4,1,4,1,4,4,0},{0,6,0,4,1,4,1,4,4,0},{0,7,0,4,1,4,1,4,4,0},{0,3,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,2,0,4,1,4,1,4,4,0},{0,3,0,4,1,4,1,4,4,0},{0,4,0,4,1,4,1,4,4,0},{0,5,0,4,1,4,1,4,4,0},{0,6,0,4,1,4,1,4,4,0},{0,7,0,4,1,4,1,4,4,0},{0,3,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,2,0,4,1,4,1,4,4,0},{0,3,0,4,1,4,1,4,4,0},{0,4,0,4,1,4,1,4,4,0},{0,5,0,4,1,4,1,4,4,0},{0,6,0,4,1,4,1,4,4,0},{0,7,0,4,1,4,1,4,4,0},{0,3,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{0,2,0,4,1,4,1,4,4,0},{0,3,0,4,1,4,1,4,4,0},{0,4,0,4,1,4,1,4,4,0},{0,5,0,4,1,4,1,4,4,0},{0,6,0,4,1,4,1,4,4,0},{0,7,0,4,1,4,1,4,4,0},{0,3,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,4,1,4,1,4,4,0},{3,0,0,4,1,4,1,4,4,0},{4,0,0,4,1,4,1,4,4,0},{5,0,0,4,1,4,1,4,4,0},{6,0,0,4,1,4,1,4,4,0},{7,0,0,4,1,4,1,4,4,0},{3,0,0,7,2,7,2,3,3,0},{0,0,0,4,1,4,1,4,4,0},
		{1,0,0,11,3,5,1,3,5,1},{1,0,0,10,3,10,3,2,2,0},{1,0,0,10,3,10,3,2,2,0},{0,0,0,10,3,10,3,2,2,0},{1,0,0,17,4,10,3,5,2,1},{1,0,0,11,3,11,3,3,3,0},{0,0,0,7,2,7,2,3,3,0},{0,0,0,11,3,11,3,3,3,0},
		{0,0,0,11,3,5,1,3,5,1},{0,0,0,10,3,10,3,2,2,0},{0,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,17,4,10,3,5,2,1},{0,0,0,17,4,17,4,5,5,0},{0,0,0,7,2,7,2,3,3,0},{8,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{2,0,0,10,3,10,3,2,2,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{2,0,0,11,3,11,3,3,3,0},{0,0,0,7,2,7,2,3,3,0},{16,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{0,0,0,4,1,4,1,4,4,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{0,0,0,4,1,4,1,4,4,0},{0,0,0,7,2,7,2,3,3,0},{24,0,0,11,3,11,3,3,3,0},
		{7,0,0,11,3,5,1,3,5,1},{3,0,0,10,3,10,3,2,2,0},{7,0,0,10,3,10,3,2,2,0},{6,3,0,19,5,19,5,3,3,0},{7,0,0,17,4,10,3,5,2,1},{3,0,0,11,3,11,3,3,3,0},{0,0,0,7,2,7,2,3,3,0},{32,0,0,11,3,11,3,3,3,0},
		{6,0,0,11,3,5,1,3,5,1},{3,0,0,4,1,4,1,4,4,0},{6,0,0,10,3,10,3,2,2,0},{2,3,0,4,1,4,1,4,4,0},{6,0,0,17,4,10,3,5,2,1},{0,0,0,4,1,4,1,4,4,0},{0,0,0,7,2,7,2,3,3,0},{40,0,0,11,3,11,3,3,3,0},
		{5,0,0,11,3,5,1,3,5,1},{0,0,0,10,3,10,3,2,2,0},{5,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{5,0,0,17,4,10,3,5,2,1},{0,0,0,11,3,11,3,3,3,0},{0,0,0,7,2,7,2,3,3,0},{48,0,0,11,3,11,3,3,3,0},
		{4,0,0,11,3,5,1,3,5,1},{6,3,0,6,2,6,2,2,2,0},{4,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{4,0,0,17,4,10,3,5,2,1},{0,0,0,4,1,4,1,4,4,0},{0,0,0,7,2,7,2,3,3,0},{56,0,0,11,3,11,3,3,3,0} },

		{ {2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{6,0,0,8,2,8,2,4,4,0},{7,0,0,8,2,8,2,4,4,0},{3,0,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,6,0,8,2,8,2,4,4,0},{0,7,0,8,2,8,2,4,4,0},{0,3,0,12,3,12,3,4,4,0},{0,0,0,8,2,8,2,4,4,0},
		{1,2,0,8,2,8,2,4,4,0},{1,3,0,8,2,8,2,4,4,0},{1,4,0,8,2,8,2,4,4,0},{1,5,0,8,2,8,2,4,4,0},{1,6,0,8,2,8,2,4,4,0},{1,7,0,8,2,8,2,4,4,0},{1,3,0,12,3,12,3,4,4,0},{1,0,0,8,2,8,2,4,4,0},
		{2,2,0,8,2,8,2,4,4,0},{2,3,0,8,2,8,2,4,4,0},{2,4,0,8,2,8,2,4,4,0},{2,5,0,8,2,8,2,4,4,0},{2,6,0,8,2,8,2,4,4,0},{2,7,0,8,2,8,2,4,4,0},{2,3,0,12,3,12,3,4,4,0},{2,0,0,8,2,8,2,4,4,0},
		{3,2,0,8,2,8,2,4,4,0},{3,3,0,8,2,8,2,4,4,0},{3,4,0,8,2,8,2,4,4,0},{3,5,0,8,2,8,2,4,4,0},{3,6,0,8,2,8,2,4,4,0},{3,7,0,8,2,8,2,4,4,0},{3,3,0,12,3,12,3,4,4,0},{3,0,0,8,2,8,2,4,4,0},
		{4,2,0,8,2,8,2,4,4,0},{4,3,0,8,2,8,2,4,4,0},{4,4,0,8,2,8,2,4,4,0},{4,5,0,8,2,8,2,4,4,0},{4,6,0,8,2,8,2,4,4,0},{4,7,0,8,2,8,2,4,4,0},{4,3,0,12,3,12,3,4,4,0},{4,0,0,8,2,8,2,4,4,0},
		{5,2,0,8,2,8,2,4,4,0},{5,3,0,8,2,8,2,4,4,0},{5,4,0,8,2,8,2,4,4,0},{5,5,0,8,2,8,2,4,4,0},{5,6,0,8,2,8,2,4,4,0},{5,7,0,8,2,8,2,4,4,0},{5,3,0,12,3,12,3,4,4,0},{5,0,0,8,2,8,2,4,4,0},
		{6,2,0,8,2,8,2,4,4,0},{6,3,0,8,2,8,2,4,4,0},{6,4,0,8,2,8,2,4,4,0},{6,5,0,8,2,8,2,4,4,0},{6,6,0,8,2,8,2,4,4,0},{6,7,0,8,2,8,2,4,4,0},{6,3,0,12,3,12,3,4,4,0},{6,0,0,8,2,8,2,4,4,0},
		{7,2,0,8,2,8,2,4,4,0},{7,3,0,8,2,8,2,4,4,0},{7,4,0,8,2,8,2,4,4,0},{7,5,0,8,2,8,2,4,4,0},{7,6,0,8,2,8,2,4,4,0},{7,7,0,8,2,8,2,4,4,0},{7,3,0,12,3,12,3,4,4,0},{7,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,6,0,8,2,8,2,4,4,0},{0,7,0,8,2,8,2,4,4,0},{0,3,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,2,0,8,2,8,2,4,4,0},{1,3,0,8,2,8,2,4,4,0},{1,4,0,8,2,8,2,4,4,0},{1,5,0,8,2,8,2,4,4,0},{1,6,0,8,2,8,2,4,4,0},{1,7,0,8,2,8,2,4,4,0},{1,3,0,15,4,15,4,3,3,0},{1,0,0,8,2,8,2,4,4,0},
		{2,2,0,8,2,8,2,4,4,0},{2,3,0,8,2,8,2,4,4,0},{2,4,0,8,2,8,2,4,4,0},{2,5,0,8,2,8,2,4,4,0},{2,6,0,8,2,8,2,4,4,0},{2,7,0,8,2,8,2,4,4,0},{2,3,0,15,4,15,4,3,3,0},{2,0,0,8,2,8,2,4,4,0},
		{3,2,0,8,2,8,2,4,4,0},{3,3,0,8,2,8,2,4,4,0},{3,4,0,8,2,8,2,4,4,0},{3,5,0,8,2,8,2,4,4,0},{3,6,0,8,2,8,2,4,4,0},{3,7,0,8,2,8,2,4,4,0},{3,3,0,15,4,15,4,3,3,0},{3,0,0,8,2,8,2,4,4,0},
		{4,2,0,8,2,8,2,4,4,0},{4,3,0,8,2,8,2,4,4,0},{4,4,0,8,2,8,2,4,4,0},{4,5,0,8,2,8,2,4,4,0},{4,6,0,8,2,8,2,4,4,0},{4,7,0,8,2,8,2,4,4,0},{4,3,0,15,4,15,4,3,3,0},{4,0,0,8,2,8,2,4,4,0},
		{5,2,0,8,2,8,2,4,4,0},{5,3,0,8,2,8,2,4,4,0},{5,4,0,8,2,8,2,4,4,0},{5,5,0,8,2,8,2,4,4,0},{5,6,0,8,2,8,2,4,4,0},{5,7,0,8,2,8,2,4,4,0},{5,3,0,15,4,15,4,3,3,0},{5,0,0,8,2,8,2,4,4,0},
		{6,2,0,8,2,8,2,4,4,0},{6,3,0,8,2,8,2,4,4,0},{6,4,0,8,2,8,2,4,4,0},{6,5,0,8,2,8,2,4,4,0},{6,6,0,8,2,8,2,4,4,0},{6,7,0,8,2,8,2,4,4,0},{6,3,0,15,4,15,4,3,3,0},{6,0,0,8,2,8,2,4,4,0},
		{7,2,0,8,2,8,2,4,4,0},{7,3,0,8,2,8,2,4,4,0},{7,4,0,8,2,8,2,4,4,0},{7,5,0,8,2,8,2,4,4,0},{7,6,0,8,2,8,2,4,4,0},{7,7,0,8,2,8,2,4,4,0},{7,3,0,15,4,15,4,3,3,0},{7,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,6,0,8,2,8,2,4,4,0},{0,7,0,8,2,8,2,4,4,0},{0,3,0,15,4,15,4,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,2,0,8,2,8,2,4,4,0},{1,3,0,8,2,8,2,4,4,0},{1,4,0,8,2,8,2,4,4,0},{1,5,0,8,2,8,2,4,4,0},{1,6,0,8,2,8,2,4,4,0},{1,7,0,8,2,8,2,4,4,0},{1,3,0,15,4,15,4,3,3,0},{1,0,0,8,2,8,2,4,4,0},
		{2,2,0,8,2,8,2,4,4,0},{2,3,0,8,2,8,2,4,4,0},{2,4,0,8,2,8,2,4,4,0},{2,5,0,8,2,8,2,4,4,0},{2,6,0,8,2,8,2,4,4,0},{2,7,0,8,2,8,2,4,4,0},{2,3,0,15,4,15,4,3,3,0},{2,0,0,8,2,8,2,4,4,0},
		{3,2,0,8,2,8,2,4,4,0},{3,3,0,8,2,8,2,4,4,0},{3,4,0,8,2,8,2,4,4,0},{3,5,0,8,2,8,2,4,4,0},{3,6,0,8,2,8,2,4,4,0},{3,7,0,8,2,8,2,4,4,0},{3,3,0,15,4,15,4,3,3,0},{3,0,0,8,2,8,2,4,4,0},
		{4,2,0,8,2,8,2,4,4,0},{4,3,0,8,2,8,2,4,4,0},{4,4,0,8,2,8,2,4,4,0},{4,5,0,8,2,8,2,4,4,0},{4,6,0,8,2,8,2,4,4,0},{4,7,0,8,2,8,2,4,4,0},{4,3,0,15,4,15,4,3,3,0},{4,0,0,8,2,8,2,4,4,0},
		{5,2,0,8,2,8,2,4,4,0},{5,3,0,8,2,8,2,4,4,0},{5,4,0,8,2,8,2,4,4,0},{5,5,0,8,2,8,2,4,4,0},{5,6,0,8,2,8,2,4,4,0},{5,7,0,8,2,8,2,4,4,0},{5,3,0,15,4,15,4,3,3,0},{5,0,0,8,2,8,2,4,4,0},
		{6,2,0,8,2,8,2,4,4,0},{6,3,0,8,2,8,2,4,4,0},{6,4,0,8,2,8,2,4,4,0},{6,5,0,8,2,8,2,4,4,0},{6,6,0,8,2,8,2,4,4,0},{6,7,0,8,2,8,2,4,4,0},{6,3,0,15,4,15,4,3,3,0},{6,0,0,8,2,8,2,4,4,0},
		{7,2,0,8,2,8,2,4,4,0},{7,3,0,8,2,8,2,4,4,0},{7,4,0,8,2,8,2,4,4,0},{7,5,0,8,2,8,2,4,4,0},{7,6,0,8,2,8,2,4,4,0},{7,7,0,8,2,8,2,4,4,0},{7,3,0,15,4,15,4,3,3,0},{7,0,0,8,2,8,2,4,4,0} },

		{ {0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{2,0,0,12,3,12,3,4,4,0},{0,2,0,12,3,12,3,4,4,0},{3,1,0,15,4,15,4,3,3,0},{0,1,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{0,0,0,8,2,8,2,4,4,0},{14,0,0,9,2,9,2,5,5,0},
		{3,0,0,12,3,12,3,4,4,0},{0,3,0,12,3,12,3,4,4,0},{3,1,0,15,4,15,4,3,3,0},{1,0,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{0,0,0,8,2,8,2,4,4,0},{15,0,0,9,2,9,2,5,5,0},
		{4,0,0,12,3,12,3,4,4,0},{0,4,0,12,3,12,3,4,4,0},{3,2,0,15,4,15,4,3,3,0},{0,2,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{1,0,0,8,2,8,2,4,4,0},{0,14,0,9,2,9,2,5,5,0},
		{5,0,0,12,3,12,3,4,4,0},{0,5,0,12,3,12,3,4,4,0},{3,2,0,15,4,15,4,3,3,0},{2,0,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{2,0,0,8,2,8,2,4,4,0},{0,15,0,9,2,9,2,5,5,0},
		{6,0,0,12,3,12,3,4,4,0},{0,6,0,12,3,12,3,4,4,0},{3,3,0,15,4,15,4,3,3,0},{0,3,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,18,5,18,5,2,2,0},
		{7,0,0,12,3,12,3,4,4,0},{0,7,0,12,3,12,3,4,4,0},{3,3,0,15,4,15,4,3,3,0},{3,0,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,18,5,18,5,2,2,0},
		{1,0,0,12,3,12,3,4,4,0},{0,0,0,12,3,12,3,4,4,0},{3,6,0,15,4,15,4,3,3,0},{0,6,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{1,0,0,8,2,8,2,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,12,3,12,3,4,4,0},{0,0,0,12,3,12,3,4,4,0},{3,6,0,15,4,15,4,3,3,0},{6,0,0,20,5,20,5,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,14,4,14,4,2,2,0},{2,0,0,8,2,8,2,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,16,4,16,4,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,21,5,16,4,5,4,3},{0,0,0,21,5,16,4,5,4,4},{0,0,0,21,5,16,4,5,4,5},{0,0,0,21,5,16,4,5,4,6},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,21,5,16,4,5,4,3},{0,0,0,21,5,16,4,5,4,4},{0,0,0,21,5,16,4,5,4,5},{0,0,0,21,5,16,4,5,4,6},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0},{0,0,0,4,1,4,1,4,4,0} },

		{ {0,0,0,8,2,8,2,4,4,0},{1,0,0,14,4,14,4,2,2,0},{1,0,0,7,2,7,2,3,3,0},{1,0,0,10,3,10,3,2,2,0},{2,0,0,8,2,8,2,4,4,0},{2,0,0,8,2,8,2,4,4,0},{2,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{4,1,0,15,4,15,4,3,3,0},{0,1,0,7,2,7,2,3,3,0},{1,0,0,10,3,10,3,2,2,0},{3,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{3,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,13,3,8,2,5,4,2},{2,0,0,14,4,14,4,2,2,0},{2,0,0,7,2,7,2,3,3,0},{2,0,0,10,3,10,3,2,2,0},{4,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{4,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,12,3,12,3,4,4,0},{4,2,0,15,4,15,4,3,3,0},{0,2,0,7,2,7,2,3,3,0},{2,0,0,10,3,10,3,2,2,0},{5,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{5,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,0,0,12,3,7,2,4,3,1},{4,0,0,14,4,14,4,2,2,0},{0,4,0,20,5,20,5,4,4,0},{4,0,0,10,3,10,3,2,2,0},{8,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{8,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,12,3,7,2,4,3,1},{4,4,0,15,4,15,4,3,3,0},{4,0,0,20,5,20,5,4,4,0},{4,0,0,10,3,10,3,2,2,0},{9,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{9,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{6,0,0,14,4,14,4,2,2,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,10,3,10,3,2,2,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{4,6,0,15,4,15,4,3,3,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,10,3,10,3,2,2,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,2,0,8,2,8,2,4,4,0},{2,3,0,8,2,8,2,4,4,0},{2,4,0,8,2,8,2,4,4,0},{2,5,0,8,2,8,2,4,4,0},{2,8,0,8,2,8,2,4,4,0},{2,9,0,8,2,8,2,4,4,0},{2,4,0,19,5,19,5,3,3,0},{2,0,0,8,2,8,2,4,4,0},
		{3,2,0,8,2,8,2,4,4,0},{3,3,0,8,2,8,2,4,4,0},{3,4,0,8,2,8,2,4,4,0},{3,5,0,8,2,8,2,4,4,0},{3,8,0,8,2,8,2,4,4,0},{3,9,0,8,2,8,2,4,4,0},{3,4,0,19,5,19,5,3,3,0},{3,0,0,8,2,8,2,4,4,0},
		{4,2,0,8,2,8,2,4,4,0},{4,3,0,8,2,8,2,4,4,0},{4,4,0,8,2,8,2,4,4,0},{4,5,0,8,2,8,2,4,4,0},{4,8,0,8,2,8,2,4,4,0},{4,9,0,8,2,8,2,4,4,0},{4,4,0,19,5,19,5,3,3,0},{4,0,0,8,2,8,2,4,4,0},
		{5,2,0,8,2,8,2,4,4,0},{5,3,0,8,2,8,2,4,4,0},{5,4,0,8,2,8,2,4,4,0},{5,5,0,8,2,8,2,4,4,0},{5,8,0,8,2,8,2,4,4,0},{5,9,0,8,2,8,2,4,4,0},{5,4,0,19,5,19,5,3,3,0},{5,0,0,8,2,8,2,4,4,0},
		{8,2,0,8,2,8,2,4,4,0},{8,3,0,8,2,8,2,4,4,0},{8,4,0,8,2,8,2,4,4,0},{8,5,0,8,2,8,2,4,4,0},{8,8,0,8,2,8,2,4,4,0},{8,9,0,8,2,8,2,4,4,0},{6,4,0,19,5,19,5,3,3,0},{8,0,0,8,2,8,2,4,4,0},
		{9,2,0,8,2,8,2,4,4,0},{9,3,0,8,2,8,2,4,4,0},{9,4,0,8,2,8,2,4,4,0},{9,5,0,8,2,8,2,4,4,0},{9,8,0,8,2,8,2,4,4,0},{9,9,0,8,2,8,2,4,4,0},{7,4,0,19,5,19,5,3,3,0},{9,0,0,8,2,8,2,4,4,0},
		{4,2,0,19,5,19,5,3,3,0},{4,3,0,19,5,19,5,3,3,0},{4,4,0,19,5,19,5,3,3,0},{4,5,0,19,5,19,5,3,3,0},{4,6,0,19,5,19,5,3,3,0},{4,7,0,19,5,19,5,3,3,0},{0,0,0,4,1,4,1,4,4,0},{4,0,0,19,5,19,5,3,3,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,8,0,8,2,8,2,4,4,0},{0,9,0,8,2,8,2,4,4,0},{0,4,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,8,0,8,2,8,2,4,4,0},{0,9,0,8,2,8,2,4,4,0},{0,4,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,8,0,8,2,8,2,4,4,0},{0,9,0,8,2,8,2,4,4,0},{0,4,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,8,0,8,2,8,2,4,4,0},{0,9,0,8,2,8,2,4,4,0},{0,4,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{8,0,0,8,2,8,2,4,4,0},{9,0,0,8,2,8,2,4,4,0},{4,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,0,0,11,3,5,1,3,5,1},{1,0,0,14,4,14,4,2,2,0},{1,0,0,10,3,10,3,2,2,0},{0,0,0,10,3,10,3,2,2,0},{1,0,0,17,4,10,3,5,2,1},{1,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{0,0,0,11,3,11,3,3,3,0},
		{0,0,0,11,3,5,1,3,5,1},{0,0,0,14,4,14,4,2,2,0},{0,0,0,10,3,10,3,2,2,0},{0,0,0,8,5,8,5,1,1,0},{0,0,0,17,4,10,3,5,2,1},{0,0,0,17,4,17,4,5,5,0},{0,0,0,7,2,7,2,3,3,0},{8,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{2,0,0,14,4,14,4,2,2,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{2,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{16,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{0,0,0,8,2,8,2,4,4,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{24,0,0,11,3,11,3,3,3,0},
		{7,0,0,11,3,5,1,3,5,1},{4,0,0,14,4,14,4,2,2,0},{7,0,0,10,3,10,3,2,2,0},{6,4,0,23,6,23,6,3,3,0},{7,0,0,17,4,10,3,5,2,1},{4,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{32,0,0,11,3,11,3,3,3,0},
		{6,0,0,11,3,5,1,3,5,1},{4,0,0,8,2,8,2,4,4,0},{6,0,0,10,3,10,3,2,2,0},{2,3,0,4,1,4,1,4,4,0},{6,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{40,0,0,11,3,11,3,3,3,0},
		{5,0,0,11,3,5,1,3,5,1},{0,0,0,14,4,14,4,2,2,0},{5,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{5,0,0,17,4,10,3,5,2,1},{0,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{48,0,0,11,3,11,3,3,3,0},
		{4,0,0,11,3,5,1,3,5,1},{6,4,0,10,3,10,3,2,2,0},{4,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{4,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{56,0,0,11,3,11,3,3,3,0} },

		{ {0,0,0,8,2,8,2,4,4,0},{1,0,0,14,4,14,4,2,2,0},{1,0,0,7,2,7,2,3,3,0},{1,0,0,10,3,10,3,2,2,0},{2,0,0,8,2,8,2,4,4,0},{2,0,0,8,2,8,2,4,4,0},{2,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,4,1,4,1,4,4,0},{5,1,0,15,4,15,4,3,3,0},{0,1,0,7,2,7,2,3,3,0},{1,0,0,10,3,10,3,2,2,0},{3,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{3,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,13,3,8,2,5,4,2},{2,0,0,14,4,14,4,2,2,0},{2,0,0,7,2,7,2,3,3,0},{2,0,0,10,3,10,3,2,2,0},{4,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{4,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,12,3,12,3,4,4,0},{5,2,0,15,4,15,4,3,3,0},{0,2,0,7,2,7,2,3,3,0},{2,0,0,10,3,10,3,2,2,0},{5,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{5,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,0,0,12,3,7,2,4,3,1},{5,0,0,14,4,14,4,2,2,0},{0,5,0,20,5,20,5,4,4,0},{5,0,0,10,3,10,3,2,2,0},{10,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{10,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,0,0,12,3,7,2,4,3,1},{5,5,0,15,4,15,4,3,3,0},{5,0,0,20,5,20,5,4,4,0},{5,0,0,10,3,10,3,2,2,0},{11,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{11,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{6,0,0,14,4,14,4,2,2,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,10,3,10,3,2,2,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{3,0,0,12,3,7,2,4,3,1},{5,6,0,15,4,15,4,3,3,0},{0,0,0,13,3,13,3,5,5,0},{6,0,0,10,3,10,3,2,2,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,8,2,8,2,4,4,0},{0,0,0,11,3,11,3,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,2,0,8,2,8,2,4,4,0},{2,3,0,8,2,8,2,4,4,0},{2,4,0,8,2,8,2,4,4,0},{2,5,0,8,2,8,2,4,4,0},{2,10,0,8,2,8,2,4,4,0},{2,11,0,8,2,8,2,4,4,0},{2,5,0,19,5,19,5,3,3,0},{2,0,0,8,2,8,2,4,4,0},
		{3,2,0,8,2,8,2,4,4,0},{3,3,0,8,2,8,2,4,4,0},{3,4,0,8,2,8,2,4,4,0},{3,5,0,8,2,8,2,4,4,0},{3,10,0,8,2,8,2,4,4,0},{3,11,0,8,2,8,2,4,4,0},{3,5,0,19,5,19,5,3,3,0},{3,0,0,8,2,8,2,4,4,0},
		{4,2,0,8,2,8,2,4,4,0},{4,3,0,8,2,8,2,4,4,0},{4,4,0,8,2,8,2,4,4,0},{4,5,0,8,2,8,2,4,4,0},{4,10,0,8,2,8,2,4,4,0},{4,11,0,8,2,8,2,4,4,0},{4,5,0,19,5,19,5,3,3,0},{4,0,0,8,2,8,2,4,4,0},
		{5,2,0,8,2,8,2,4,4,0},{5,3,0,8,2,8,2,4,4,0},{5,4,0,8,2,8,2,4,4,0},{5,5,0,8,2,8,2,4,4,0},{5,10,0,8,2,8,2,4,4,0},{5,11,0,8,2,8,2,4,4,0},{5,5,0,19,5,19,5,3,3,0},{5,0,0,8,2,8,2,4,4,0},
		{10,2,0,8,2,8,2,4,4,0},{10,3,0,8,2,8,2,4,4,0},{10,4,0,8,2,8,2,4,4,0},{10,5,0,8,2,8,2,4,4,0},{10,10,0,8,2,8,2,4,4,0},{10,11,0,8,2,8,2,4,4,0},{6,5,0,19,5,19,5,3,3,0},{10,0,0,8,2,8,2,4,4,0},
		{11,2,0,8,2,8,2,4,4,0},{11,3,0,8,2,8,2,4,4,0},{11,4,0,8,2,8,2,4,4,0},{11,5,0,8,2,8,2,4,4,0},{11,10,0,8,2,8,2,4,4,0},{11,11,0,8,2,8,2,4,4,0},{7,5,0,19,5,19,5,3,3,0},{11,0,0,8,2,8,2,4,4,0},
		{5,2,0,19,5,19,5,3,3,0},{5,3,0,19,5,19,5,3,3,0},{5,4,0,19,5,19,5,3,3,0},{5,5,0,19,5,19,5,3,3,0},{5,6,0,19,5,19,5,3,3,0},{5,7,0,19,5,19,5,3,3,0},{0,0,0,4,1,4,1,4,4,0},{5,0,0,19,5,19,5,3,3,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,10,0,8,2,8,2,4,4,0},{0,11,0,8,2,8,2,4,4,0},{0,5,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,10,0,8,2,8,2,4,4,0},{0,11,0,8,2,8,2,4,4,0},{0,5,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,10,0,8,2,8,2,4,4,0},{0,11,0,8,2,8,2,4,4,0},{0,5,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{0,2,0,8,2,8,2,4,4,0},{0,3,0,8,2,8,2,4,4,0},{0,4,0,8,2,8,2,4,4,0},{0,5,0,8,2,8,2,4,4,0},{0,10,0,8,2,8,2,4,4,0},{0,11,0,8,2,8,2,4,4,0},{0,5,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{2,0,0,8,2,8,2,4,4,0},{3,0,0,8,2,8,2,4,4,0},{4,0,0,8,2,8,2,4,4,0},{5,0,0,8,2,8,2,4,4,0},{10,0,0,8,2,8,2,4,4,0},{11,0,0,8,2,8,2,4,4,0},{5,0,0,19,5,19,5,3,3,0},{0,0,0,8,2,8,2,4,4,0},
		{1,0,0,11,3,5,1,3,5,1},{1,0,0,14,4,14,4,2,2,0},{1,0,0,10,3,10,3,2,2,0},{0,0,0,10,3,10,3,2,2,0},{1,0,0,17,4,10,3,5,2,1},{1,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{0,0,0,11,3,11,3,3,3,0},
		{0,0,0,11,3,5,1,3,5,1},{0,0,0,14,4,14,4,2,2,0},{0,0,0,10,3,10,3,2,2,0},{0,0,0,8,5,8,5,1,1,0},{0,0,0,17,4,10,3,5,2,1},{0,0,0,17,4,17,4,5,5,0},{0,0,0,7,2,7,2,3,3,0},{8,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{2,0,0,14,4,14,4,2,2,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{2,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{16,0,0,11,3,11,3,3,3,0},
		{3,0,0,11,3,5,1,3,5,1},{0,0,0,8,2,8,2,4,4,0},{3,0,0,10,3,10,3,2,2,0},{0,0,0,11,3,11,3,3,3,0},{3,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{24,0,0,11,3,11,3,3,3,0},
		{7,0,0,11,3,5,1,3,5,1},{5,0,0,14,4,14,4,2,2,0},{7,0,0,10,3,10,3,2,2,0},{6,5,0,23,6,23,6,3,3,0},{7,0,0,17,4,10,3,5,2,1},{5,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{32,0,0,11,3,11,3,3,3,0},
		{6,0,0,11,3,5,1,3,5,1},{5,0,0,8,2,8,2,4,4,0},{6,0,0,10,3,10,3,2,2,0},{2,3,0,4,1,4,1,4,4,0},{6,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{40,0,0,11,3,11,3,3,3,0},
		{5,0,0,11,3,5,1,3,5,1},{0,0,0,14,4,14,4,2,2,0},{5,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{5,0,0,17,4,10,3,5,2,1},{0,0,0,15,4,15,4,3,3,0},{0,0,0,7,2,7,2,3,3,0},{48,0,0,11,3,11,3,3,3,0},
		{4,0,0,11,3,5,1,3,5,1},{6,5,0,10,3,10,3,2,2,0},{4,0,0,10,3,10,3,2,2,0},{0,0,0,4,1,4,1,4,4,0},{4,0,0,17,4,10,3,5,2,1},{0,0,0,8,5,8,5,1,1,0},{0,0,0,7,2,7,2,3,3,0},{56,0,0,11,3,11,3,3,3,0} },

		{ {4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{4,2,0,23,6,23,6,3,3,0},{4,3,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,6,0,23,6,23,6,3,3,0},{4,7,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},{4,0,0,23,6,23,6,3,3,0},
		{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},{0,4,0,20,5,20,5,4,4,0},
		{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},{1,4,0,20,5,20,5,4,4,0},
		{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},{2,4,0,20,5,20,5,4,4,0},
		{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},{3,4,0,20,5,20,5,4,4,0},
		{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},{4,4,0,20,5,20,5,4,4,0},
		{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},{5,4,0,20,5,20,5,4,4,0},
		{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},{6,4,0,20,5,20,5,4,4,0},
		{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},{7,4,0,20,5,20,5,4,4,0},
		{0,4,2,23,6,23,6,3,3,0},{0,4,3,23,6,23,6,3,3,0},{0,4,4,23,6,23,6,3,3,0},{0,4,5,23,6,23,6,3,3,0},{0,4,6,23,6,23,6,3,3,0},{0,4,7,23,6,23,6,3,3,0},{0,4,0,23,6,23,6,3,3,0},{0,4,0,23,6,23,6,3,3,0},
		{1,4,2,23,6,23,6,3,3,0},{1,4,3,23,6,23,6,3,3,0},{1,4,4,23,6,23,6,3,3,0},{1,4,5,23,6,23,6,3,3,0},{1,4,6,23,6,23,6,3,3,0},{1,4,7,23,6,23,6,3,3,0},{1,4,0,23,6,23,6,3,3,0},{1,4,0,23,6,23,6,3,3,0},
		{2,4,2,23,6,23,6,3,3,0},{2,4,3,23,6,23,6,3,3,0},{2,4,4,23,6,23,6,3,3,0},{2,4,5,23,6,23,6,3,3,0},{2,4,6,23,6,23,6,3,3,0},{2,4,7,23,6,23,6,3,3,0},{2,4,0,23,6,23,6,3,3,0},{2,4,0,23,6,23,6,3,3,0},
		{3,4,2,23,6,23,6,3,3,0},{3,4,3,23,6,23,6,3,3,0},{3,4,4,23,6,23,6,3,3,0},{3,4,5,23,6,23,6,3,3,0},{3,4,6,23,6,23,6,3,3,0},{3,4,7,23,6,23,6,3,3,0},{3,4,0,23,6,23,6,3,3,0},{3,4,0,23,6,23,6,3,3,0},
		{4,4,2,23,6,23,6,3,3,0},{4,4,3,23,6,23,6,3,3,0},{4,4,4,23,6,23,6,3,3,0},{4,4,5,23,6,23,6,3,3,0},{4,4,6,23,6,23,6,3,3,0},{4,4,7,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},
		{5,4,2,23,6,23,6,3,3,0},{5,4,3,23,6,23,6,3,3,0},{5,4,4,23,6,23,6,3,3,0},{5,4,5,23,6,23,6,3,3,0},{5,4,6,23,6,23,6,3,3,0},{5,4,7,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},
		{6,4,2,23,6,23,6,3,3,0},{6,4,3,23,6,23,6,3,3,0},{6,4,4,23,6,23,6,3,3,0},{6,4,5,23,6,23,6,3,3,0},{6,4,6,23,6,23,6,3,3,0},{6,4,7,23,6,23,6,3,3,0},{6,4,0,23,6,23,6,3,3,0},{6,4,0,23,6,23,6,3,3,0},
		{7,4,2,23,6,23,6,3,3,0},{7,4,3,23,6,23,6,3,3,0},{7,4,4,23,6,23,6,3,3,0},{7,4,5,23,6,23,6,3,3,0},{7,4,6,23,6,23,6,3,3,0},{7,4,7,23,6,23,6,3,3,0},{7,4,0,23,6,23,6,3,3,0},{7,4,0,23,6,23,6,3,3,0},
		{0,4,2,23,6,23,6,3,3,0},{0,4,3,23,6,23,6,3,3,0},{0,4,4,23,6,23,6,3,3,0},{0,4,5,23,6,23,6,3,3,0},{0,4,6,23,6,23,6,3,3,0},{0,4,7,23,6,23,6,3,3,0},{0,4,0,23,6,23,6,3,3,0},{0,4,0,23,6,23,6,3,3,0},
		{1,4,2,23,6,23,6,3,3,0},{1,4,3,23,6,23,6,3,3,0},{1,4,4,23,6,23,6,3,3,0},{1,4,5,23,6,23,6,3,3,0},{1,4,6,23,6,23,6,3,3,0},{1,4,7,23,6,23,6,3,3,0},{1,4,0,23,6,23,6,3,3,0},{1,4,0,23,6,23,6,3,3,0},
		{2,4,2,23,6,23,6,3,3,0},{2,4,3,23,6,23,6,3,3,0},{2,4,4,23,6,23,6,3,3,0},{2,4,5,23,6,23,6,3,3,0},{2,4,6,23,6,23,6,3,3,0},{2,4,7,23,6,23,6,3,3,0},{2,4,0,23,6,23,6,3,3,0},{2,4,0,23,6,23,6,3,3,0},
		{3,4,2,23,6,23,6,3,3,0},{3,4,3,23,6,23,6,3,3,0},{3,4,4,23,6,23,6,3,3,0},{3,4,5,23,6,23,6,3,3,0},{3,4,6,23,6,23,6,3,3,0},{3,4,7,23,6,23,6,3,3,0},{3,4,0,23,6,23,6,3,3,0},{3,4,0,23,6,23,6,3,3,0},
		{4,4,2,23,6,23,6,3,3,0},{4,4,3,23,6,23,6,3,3,0},{4,4,4,23,6,23,6,3,3,0},{4,4,5,23,6,23,6,3,3,0},{4,4,6,23,6,23,6,3,3,0},{4,4,7,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},{4,4,0,23,6,23,6,3,3,0},
		{5,4,2,23,6,23,6,3,3,0},{5,4,3,23,6,23,6,3,3,0},{5,4,4,23,6,23,6,3,3,0},{5,4,5,23,6,23,6,3,3,0},{5,4,6,23,6,23,6,3,3,0},{5,4,7,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},
		{6,4,2,23,6,23,6,3,3,0},{6,4,3,23,6,23,6,3,3,0},{6,4,4,23,6,23,6,3,3,0},{6,4,5,23,6,23,6,3,3,0},{6,4,6,23,6,23,6,3,3,0},{6,4,7,23,6,23,6,3,3,0},{6,4,0,23,6,23,6,3,3,0},{6,4,0,23,6,23,6,3,3,0},
		{7,4,2,23,6,23,6,3,3,0},{7,4,3,23,6,23,6,3,3,0},{7,4,4,23,6,23,6,3,3,0},{7,4,5,23,6,23,6,3,3,0},{7,4,6,23,6,23,6,3,3,0},{7,4,7,23,6,23,6,3,3,0},{7,4,0,23,6,23,6,3,3,0},{7,4,0,23,6,23,6,3,3,0} },

		{ {5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{5,2,0,23,6,23,6,3,3,0},{5,3,0,23,6,23,6,3,3,0},{5,4,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,6,0,23,6,23,6,3,3,0},{5,7,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},{5,0,0,23,6,23,6,3,3,0},
		{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},{0,5,0,20,5,20,5,4,4,0},
		{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},{1,5,0,20,5,20,5,4,4,0},
		{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},{2,5,0,20,5,20,5,4,4,0},
		{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},{3,5,0,20,5,20,5,4,4,0},
		{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},{4,5,0,20,5,20,5,4,4,0},
		{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},{5,5,0,20,5,20,5,4,4,0},
		{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},{6,5,0,20,5,20,5,4,4,0},
		{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},{7,5,0,20,5,20,5,4,4,0},
		{0,5,2,23,6,23,6,3,3,0},{0,5,3,23,6,23,6,3,3,0},{0,5,4,23,6,23,6,3,3,0},{0,5,5,23,6,23,6,3,3,0},{0,5,6,23,6,23,6,3,3,0},{0,5,7,23,6,23,6,3,3,0},{0,5,0,23,6,23,6,3,3,0},{0,5,0,23,6,23,6,3,3,0},
		{1,5,2,23,6,23,6,3,3,0},{1,5,3,23,6,23,6,3,3,0},{1,5,4,23,6,23,6,3,3,0},{1,5,5,23,6,23,6,3,3,0},{1,5,6,23,6,23,6,3,3,0},{1,5,7,23,6,23,6,3,3,0},{1,5,0,23,6,23,6,3,3,0},{1,5,0,23,6,23,6,3,3,0},
		{2,5,2,23,6,23,6,3,3,0},{2,5,3,23,6,23,6,3,3,0},{2,5,4,23,6,23,6,3,3,0},{2,5,5,23,6,23,6,3,3,0},{2,5,6,23,6,23,6,3,3,0},{2,5,7,23,6,23,6,3,3,0},{2,5,0,23,6,23,6,3,3,0},{2,5,0,23,6,23,6,3,3,0},
		{3,5,2,23,6,23,6,3,3,0},{3,5,3,23,6,23,6,3,3,0},{3,5,4,23,6,23,6,3,3,0},{3,5,5,23,6,23,6,3,3,0},{3,5,6,23,6,23,6,3,3,0},{3,5,7,23,6,23,6,3,3,0},{3,5,0,23,6,23,6,3,3,0},{3,5,0,23,6,23,6,3,3,0},
		{4,5,2,23,6,23,6,3,3,0},{4,5,3,23,6,23,6,3,3,0},{4,5,4,23,6,23,6,3,3,0},{4,5,5,23,6,23,6,3,3,0},{4,5,6,23,6,23,6,3,3,0},{4,5,7,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},
		{5,5,2,23,6,23,6,3,3,0},{5,5,3,23,6,23,6,3,3,0},{5,5,4,23,6,23,6,3,3,0},{5,5,5,23,6,23,6,3,3,0},{5,5,6,23,6,23,6,3,3,0},{5,5,7,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},
		{6,5,2,23,6,23,6,3,3,0},{6,5,3,23,6,23,6,3,3,0},{6,5,4,23,6,23,6,3,3,0},{6,5,5,23,6,23,6,3,3,0},{6,5,6,23,6,23,6,3,3,0},{6,5,7,23,6,23,6,3,3,0},{6,5,0,23,6,23,6,3,3,0},{6,5,0,23,6,23,6,3,3,0},
		{7,5,2,23,6,23,6,3,3,0},{7,5,3,23,6,23,6,3,3,0},{7,5,4,23,6,23,6,3,3,0},{7,5,5,23,6,23,6,3,3,0},{7,5,6,23,6,23,6,3,3,0},{7,5,7,23,6,23,6,3,3,0},{7,5,0,23,6,23,6,3,3,0},{7,5,0,23,6,23,6,3,3,0},
		{0,5,2,23,6,23,6,3,3,0},{0,5,3,23,6,23,6,3,3,0},{0,5,4,23,6,23,6,3,3,0},{0,5,5,23,6,23,6,3,3,0},{0,5,6,23,6,23,6,3,3,0},{0,5,7,23,6,23,6,3,3,0},{0,5,0,23,6,23,6,3,3,0},{0,5,0,23,6,23,6,3,3,0},
		{1,5,2,23,6,23,6,3,3,0},{1,5,3,23,6,23,6,3,3,0},{1,5,4,23,6,23,6,3,3,0},{1,5,5,23,6,23,6,3,3,0},{1,5,6,23,6,23,6,3,3,0},{1,5,7,23,6,23,6,3,3,0},{1,5,0,23,6,23,6,3,3,0},{1,5,0,23,6,23,6,3,3,0},
		{2,5,2,23,6,23,6,3,3,0},{2,5,3,23,6,23,6,3,3,0},{2,5,4,23,6,23,6,3,3,0},{2,5,5,23,6,23,6,3,3,0},{2,5,6,23,6,23,6,3,3,0},{2,5,7,23,6,23,6,3,3,0},{2,5,0,23,6,23,6,3,3,0},{2,5,0,23,6,23,6,3,3,0},
		{3,5,2,23,6,23,6,3,3,0},{3,5,3,23,6,23,6,3,3,0},{3,5,4,23,6,23,6,3,3,0},{3,5,5,23,6,23,6,3,3,0},{3,5,6,23,6,23,6,3,3,0},{3,5,7,23,6,23,6,3,3,0},{3,5,0,23,6,23,6,3,3,0},{3,5,0,23,6,23,6,3,3,0},
		{4,5,2,23,6,23,6,3,3,0},{4,5,3,23,6,23,6,3,3,0},{4,5,4,23,6,23,6,3,3,0},{4,5,5,23,6,23,6,3,3,0},{4,5,6,23,6,23,6,3,3,0},{4,5,7,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},{4,5,0,23,6,23,6,3,3,0},
		{5,5,2,23,6,23,6,3,3,0},{5,5,3,23,6,23,6,3,3,0},{5,5,4,23,6,23,6,3,3,0},{5,5,5,23,6,23,6,3,3,0},{5,5,6,23,6,23,6,3,3,0},{5,5,7,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},{5,5,0,23,6,23,6,3,3,0},
		{6,5,2,23,6,23,6,3,3,0},{6,5,3,23,6,23,6,3,3,0},{6,5,4,23,6,23,6,3,3,0},{6,5,5,23,6,23,6,3,3,0},{6,5,6,23,6,23,6,3,3,0},{6,5,7,23,6,23,6,3,3,0},{6,5,0,23,6,23,6,3,3,0},{6,5,0,23,6,23,6,3,3,0},
		{7,5,2,23,6,23,6,3,3,0},{7,5,3,23,6,23,6,3,3,0},{7,5,4,23,6,23,6,3,3,0},{7,5,5,23,6,23,6,3,3,0},{7,5,6,23,6,23,6,3,3,0},{7,5,7,23,6,23,6,3,3,0},{7,5,0,23,6,23,6,3,3,0},{7,5,0,23,6,23,6,3,3,0} } };


ZBYTE endianess_fix[256 * 7] = {
		0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,
		0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,2,0,0,0,0,0,1,0,1,0,1,1,1,0,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,2,2,2,2,2,2,0,2,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,1,1,1,1,1,1,0,1,3,3,3,3,3,3,1,3,
		1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,
		0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,2,0,0,0,0,1,0,1,1,1,0,1,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

		1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,
		1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,

		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,2,0,0,0,0,0,3,1,2,0,0,0,0,0,3,1,2,0,0,0,0,0,3,1,2,0,0,0,0,0,3,
		1,2,0,0,0,0,0,0,1,2,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,2,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

		0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,
		0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,2,0,0,0,0,0,1,0,1,0,1,1,1,0,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,2,2,2,2,2,2,0,2,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,1,1,1,1,1,1,0,1,3,3,3,3,3,3,1,3,
		1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,
		0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,2,0,0,0,0,1,0,1,1,1,0,1,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

		0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,0,0,2,0,1,1,1,0,0,0,1,0,1,1,1,0,
		0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,2,0,0,0,0,0,1,0,1,0,1,1,1,0,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,2,2,2,2,2,2,0,2,3,3,3,3,3,3,1,3,
		3,3,3,3,3,3,1,3,3,3,3,3,3,3,1,3,1,1,1,1,1,1,0,1,3,3,3,3,3,3,1,3,
		1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,
		0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,2,0,0,0,0,1,0,1,1,1,0,1,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,

		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,
		4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0,4 };


	void ADC_RR_RR();
	void ADC_R_HL();
	void ADC_R_n();
	void ADC_R_R();
	void ADC_R_off();
	void ADD_RR_RR();
	void ADD_R_HL();
	void ADD_R_n();
	void ADD_R_R();
	void ADD_R_off();
	void AND_HL();
	void AND_R();
	void AND_off();
	void AND_n();
	void BIT_n_HL();
	void BIT_n_R();
	void BIT_n_off();
	void CALL_cond();
	void CALL();
	void CCF();
	void CPD();
	void CPDR();
	void CPI();
	void CPIR();
	void CPL();
	void CP_HL();
	void CP_R();
	void CP_n();
	void CP_off();
	void DAA();
	void DEC_R();
	void DEC_RR();
	void DEC_ind();
	void DEC_off();
	void DI();
	void DJNZ();
	void EI();
	void EXX();
	void EX_RR_altRR();
	void EX_RR_RR();
	void EX_SP_RR();
	void HALT();
	void IM();
	void INC_R();
	void INC_RR();
	void INC_ind();
	void INC_off();
	void IND();
	void INDR();
	void INI();
	void INIR();
	void IN_R_c();
	void IN_R_n();
	void JP_cond();
	void JP_ind();
	void JP_nn();
	void JR_cond_d();
	void JR_d();
	void LDD();
	void LDDR();
	void LDI();
	void LDIR();
	void LD_R_spec();
	void LD_RR_RR();
	void LD_RR_addr();
	void LD_RR_nn();
	void LD_R_R();
	void LD_R_off();
	void LD_R_addr();
	void LD_R_ind();
	void LD_R_n();
	void LD_addr_R();
	void LD_addr_RR();
	void LD_ind_R();
	void LD_ind_n();
	void LD_off_R();
	void LD_off_n();
	void NEG();
	void NOP();
	void OR_HL();
	void OR_R();
	void OR_off();
	void OR_n();
	void OTDR();
	void OTIR();
	void OUTD();
	void OUTI();
	void OUT_c_0();
	void OUT_c_R();
	void OUT_n_R();
	void POP();
	void PUSH();
	void RES_n_HL();
	void RES_n_R();
	void RES_n_off();
	void RES_n_off_R();
	void RET();
	void RETI();
	void RETN();
	void RET_cond();
	void RLA();
	void RLCA();
	void RLC_HL();
	void RLC_R();
	void RLC_off();
	void RLC_off_R();
	void RLD();
	void RL_HL();
	void RL_R();
	void RL_off();
	void RL_off_R();
	void RRA();
	void RRCA();
	void RRC_HL();
	void RRC_R();
	void RRC_off();
	void RRC_off_R();
	void RRD();
	void RR_HL();
	void RR_R();
	void RR_off();
	void RR_off_R();
	void RST();
	void SBC_RR_RR();
	void SBC_R_HL();
	void SBC_R_n();
	void SBC_R_R();
	void SBC_R_off();
	void SCF();
	void SET_n_HL();
	void SET_n_R();
	void SET_n_off();
	void SET_n_off_R();
	void SLA_HL();
	void SLA_R();
	void SLA_off();
	void SLA_off_R();
	void SLL_HL();
	void SLL_R();
	void SLL_off();
	void SLL_off_R();
	void SRA_HL();
	void SRA_R();
	void SRA_off();
	void SRA_off_R();
	void SRL_HL();
	void SRL_R();
	void SRL_off();
	void SRL_off_R();
	void SUB_HL();
	void SUB_R();
	void SUB_n();
	void SUB_off();
	void XOR_HL();
	void XOR_R();
	void XOR_off();
	void XOR_n();

};

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(z80_class) {
	class_<Z80>("Z80")
		.constructor<int, std::string>()
		.function("Execute", &Z80:::Execute)
	// .property("x", &Z80:::getX, &Z80:::setX)
	//.class_function("getStringFromInstance", &Z80:::getStringFromInstance)
	;
}
#endif

#endif