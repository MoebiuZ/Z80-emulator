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

#define FLAG_S		(1 << 7)
#define FLAG_Z		(1 << 6)
#define FLAG_Y		(1 << 5)
#define FLAG_H		(1 << 4)
#define FLAG_X		(1 << 3)
#define FLAG_PV	(1 << 2)
#define FLAG_N		(1 << 1)
#define FLAG_C		(1 << 0)

#define COND_Z		0
#define COND_NZ	1
#define COND_C		2
#define COND_NC	3
#define COND_M		4
#define COND_P		5
#define COND_PE	6
#define COND_PO	7


#define REG(val)  reg.registers[val]
#define PAIR(val) reg.pairs[val]
#define ALT_PAIR(val) alt_reg.pairs[val]
#define OP1 a_set[i_set][op].operand1
#define OP2 a_set[i_set][op].operand2
#define OP3 a_set[i_set][op].operand3


#define GETFLAG(flag) (reg.b.f & flag)
#define SETFLAG(flag) reg.b.f |= flag
#define RESETFLAG(flag) reg.b.f &= ~(flag)
#define MODFLAG(flag, value) reg.b.f ^= (-(value) ^ reg.b.f) & flag


#define ADJUSTUNDOCFLAGS(val) \
{ \
	MODFLAG(FLAG_Y, (val & FLAG_Y) != 0);  \
	MODFLAG(FLAG_X, (val & FLAG_X) != 0);  \
}


int Z80::Condition(ZBYTE cond) {
	switch (cond) {
		case COND_Z: return GETFLAG(FLAG_Z);
		case COND_NZ: return !GETFLAG(FLAG_Z);
		case COND_C: return GETFLAG(FLAG_C);
		case COND_NC: return !GETFLAG(FLAG_C);
		case COND_M: return GETFLAG(FLAG_S);
		case COND_P: return !GETFLAG(FLAG_S);
		case COND_PE: return GETFLAG(FLAG_PV);
		case COND_PO: return !GETFLAG(FLAG_PV);
		default: return !GETFLAG(FLAG_PV);
	}
}




#define CP(value) \
{ \
	ZWORD result, carry, halfcarry; \
	result = reg.b.a - value; \
	halfcarry = reg.b.a ^ result ^ value; \
	reg.b.f = FLAG_N | (halfcarry & FLAG_H); \
	reg.b.f |= SZ_table[result & 0xff] & (FLAG_S | FLAG_Z); \
	halfcarry &= 0x0180; \
	reg.b.f |= V_table[halfcarry >> 7]; \
	carry = halfcarry >> 8; \
	reg.b.f |= carry; \
}


#define CPHL(result) \
{ \
	ZBYTE val = READBYTE(reg.w.hl); \
	result = reg.b.a - val; \
	CP(val); \
	ADJUSTUNDOCFLAGS(val); \
}

#define INC(value) \
{ \
	ZWORD halfcarry; \
	halfcarry = value ^ (value + 1); \
	value++; \
	reg.b.f = (reg.b.f & FLAG_C) | (halfcarry & FLAG_H) | SZ_table[value & 0xff] | V_table[(halfcarry >> 7) & 0x03]; \
}

#define DEC(value) \
{ \
	ZWORD halfcarry; \
	halfcarry = value ^ (value - 1); \
	value--; \
	reg.b.f = (reg.b.f & FLAG_C) | FLAG_N | (halfcarry & FLAG_H) | SZ_table[value & 0xff] | V_table[(halfcarry >> 7) & 0x03]; \
  }

#define RLC(adjFlags, val) \
 { \
	MODFLAG(FLAG_C, (val & 0x80) != 0); \
	val <<= 1; \
	val |= (ZBYTE) GETFLAG(FLAG_C); \
	if (adjFlags) { \
		reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	} \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define RL(adjFlags, val) \
{ \
	int CY = GETFLAG(FLAG_C); \
	MODFLAG(FLAG_C, (val & 0x80) != 0); \
	val <<= 1; \
	val |= (ZBYTE) CY; \
	if (adjFlags) { \
		reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	} \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define RRC(adjFlags, val) \
{ \
	MODFLAG(FLAG_C, (val & 0x01) != 0); \
	val >>= 1; \
	val |= ((ZBYTE) GETFLAG(FLAG_C) << 7); \
	if (adjFlags) { \
		reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	} \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define RR(adjFlags, val) \
{ \
	int CY = GETFLAG(FLAG_C); \
	MODFLAG(FLAG_C, (val & 0x01)); \
	val >>= 1; \
	val |= (CY << 7); \
	if (adjFlags) { \
		reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	} \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define SLA(val) \
{ \
	MODFLAG(FLAG_C, (val & 0x80) != 0); \
	val <<= 1; \
	reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define SLL(val) \
{ \
	MODFLAG(FLAG_C, (val & 0x80) != 0); \
	val <<= 1; \
	val |= 1; \
	reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define SRA(val) \
{ \
	int b = val & 0x80; \
	MODFLAG(FLAG_C, (val & 0x01) != 0); \
	val >>= 1; \
	val |= b; \
	reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}


#define SRL(val) \
{ \
	MODFLAG(FLAG_C, (val & 0x01) != 0); \
	val >>= 1; \
	reg.b.f = SZP_table[val] | GETFLAG(FLAG_C); \
	RESETFLAG(FLAG_H | FLAG_N); \
	ADJUSTUNDOCFLAGS(val); \
}




void Z80::ADC_RR_RR() {
	ZDWORD result, carry, halfcarry;

	reg.w.wz = PAIR(OP1) + 1; // first register before operation

	result = PAIR(OP1) + PAIR(OP2) + (reg.b.f & FLAG_C);

	halfcarry = PAIR(OP1) ^ PAIR(OP2) ^ result;
	carry = result >> 16;

	reg.b.f = result & 0xffff ? (result >> 8) & (FLAG_S | FLAG_X | FLAG_Y) : FLAG_Z;
	reg.b.f |= (halfcarry >> 8) & FLAG_H;
	reg.b.f |= V_table[halfcarry >> 15];
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result >> 8);
	PAIR(OP1) = result;

}


void Z80::ADC_R_HL() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(reg.w.hl);
	
	result = reg.b.a + value + (reg.b.f & FLAG_C);
	halfcarry = reg.b.a ^ value ^ result;

	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result;
}

void Z80::ADC_R_n() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(pc++);
	
	result = reg.b.a + value + (reg.b.f & FLAG_C);
	halfcarry = reg.b.a ^ value ^ result;

	carry = result >> 8;

	reg.b.f = (halfcarry & FLAG_H)
	 | SZ_table[result & 0xff]
	 | V_table[halfcarry >> 7]
	 | carry;
	 
	ADJUSTUNDOCFLAGS(result);

	reg.b.a = result;

}

void Z80::ADC_R_R() {
	ZWORD result, carry, halfcarry;
	
	result = reg.b.a + REG(OP2) + (reg.b.f & FLAG_C);
	halfcarry = reg.b.a ^ REG(OP2) ^ result;

	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result;
}

void Z80::ADC_R_off() {
	ZWORD result, carry, halfcarry;
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP2) + offset);
	
	result = reg.b.a + value + (reg.b.f & FLAG_C);
	halfcarry = reg.b.a ^ value ^ result;

	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;

	reg.b.a = result;
	ADJUSTUNDOCFLAGS(result);
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::ADD_RR_RR() {
	ZDWORD result, carry, halfcarry;

	reg.w.wz = PAIR(OP1) + 1; // first register before operation

	result = PAIR(OP1) + PAIR(OP2);

	halfcarry = PAIR(OP1) ^ PAIR(OP2) ^ result;
	carry = result >> 16;

	reg.b.f = reg.b.f & (FLAG_S | FLAG_Z | FLAG_PV);
	reg.b.f |= (result >> 8) & (FLAG_X | FLAG_Y);
	reg.b.f |= (halfcarry >> 8) & FLAG_H;
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result >> 8);
	PAIR(OP1) = result;

}

void Z80::ADD_R_HL() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(reg.w.hl);

	result = reg.b.a + value;

	halfcarry = reg.b.a ^ value ^ result;
	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	REG(OP1) = result;
}

void Z80::ADD_R_n() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(pc);
	
	result = REG(OP1) + value;

	halfcarry = REG(OP1) ^ value ^ result;
	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;

	ADJUSTUNDOCFLAGS(result);

	REG(OP1) = result;
	pc++;
}

void Z80::ADD_R_R() {
	ZWORD result, carry, halfcarry;
	
	result = reg.b.a + REG(OP2);

	halfcarry = reg.b.a ^ REG(OP2) ^ result;
	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	REG(OP1) = result;
}

void Z80::ADD_R_off() {
	ZWORD result, carry, halfcarry;
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP2) + offset);

	result = reg.b.a + value;

	halfcarry = reg.b.a ^ value ^ result;
	carry = result >> 8;

	reg.b.f = halfcarry & FLAG_H;
	reg.b.f |= SZ_table[result & 0xff];
	reg.b.f |= V_table[halfcarry >> 7];
	reg.b.f |= carry;

	reg.b.a = result;
	ADJUSTUNDOCFLAGS(result);	
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::AND_HL() {
	reg.b.a &= READBYTE(reg.w.hl);
	reg.b.f = SZP_table[reg.b.a] | FLAG_H;
}

void Z80::AND_R() {
	reg.b.a &= REG(OP1);
	reg.b.f = SZP_table[reg.b.a] | FLAG_H;
}

void Z80::AND_off() {
	char offset = READBYTE(pc++);
	reg.b.a &= READBYTE(PAIR(OP1) + offset);
	reg.b.f = SZP_table[reg.b.a] | FLAG_H;
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::AND_n() {
	reg.b.a &= READBYTE(pc++);
	reg.b.f = SZP_table[reg.b.a] | FLAG_H;
}

void Z80::BIT_n_HL() {
	ZBYTE result;
	result = READBYTE(reg.w.hl) & (1 << OP1);

	if (result) {
		reg.b.f = reg.b.f & FLAG_C;
	} else {
		reg.b.f |= FLAG_Z | FLAG_PV | (reg.b.f & FLAG_C);
	}

	reg.b.f |= result & FLAG_S;
	reg.b.f |= reg.b.w & (FLAG_X | FLAG_Y);
	reg.b.f |= FLAG_H;
	reg.b.f &= ~(FLAG_N);
}

void Z80::BIT_n_R() {
	ZBYTE result;
	result = REG(OP2) & (1 << OP1);
	
	if (result) {
		reg.b.f = reg.b.f & FLAG_C;
	} else {
		reg.b.f = FLAG_Z | FLAG_PV | (reg.b.f & FLAG_C);
	}

	reg.b.f |= result & FLAG_S;
	reg.b.f |= REG(OP2) & (FLAG_X | FLAG_Y);
	reg.b.f |= FLAG_H;
	reg.b.f &= ~(FLAG_N);
}

void Z80::BIT_n_off() {

	char offset = READBYTE(pc++ - 1);
	ZWORD addr = PAIR(OP2) + offset;

	ZBYTE result = READBYTE(addr) & (1 << OP1);

	if (result) {
		reg.b.f = reg.b.f & FLAG_C;
	} else {
		reg.b.f = FLAG_Z | FLAG_PV | (reg.b.f & FLAG_C);
	}

	reg.b.f |= result & FLAG_S;
	reg.b.f |= (addr >> 8) & (FLAG_X | FLAG_Y);
	reg.b.f |= FLAG_H;
	reg.b.f &= ~(FLAG_N);

	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::CALL() {
	ZWORD addr = READWORD(pc);
	pc += 2;
	/* Push */
	reg.w.sp -= 2;
	WRITEWORD(reg.w.sp, pc);
	/* Push */
	pc = addr;
	reg.w.wz = pc;
}

void Z80::CALL_cond() {
	ZWORD addr = READWORD(pc);
	pc += 2;
	if (Condition(OP1)) {
		/* Push */
		reg.w.sp -= 2;
		WRITEWORD(reg.w.sp, pc);
		/* Push */
		pc = addr;
	}
	reg.w.wz = pc;
}

void Z80::CCF() {
	MODFLAG(FLAG_H, GETFLAG(FLAG_C));
	MODFLAG(FLAG_C, (1 - (ZBYTE) GETFLAG(FLAG_C) != 0));
	RESETFLAG(FLAG_N);
	ADJUSTUNDOCFLAGS(reg.b.a);
}

void Z80::CPD() {
	int carry_before = GETFLAG(FLAG_C);

	ZBYTE result;

	CPHL(result);

	if(GETFLAG(FLAG_H)) {
		result--;
	}
	reg.w.hl--;
	reg.w.bc--;
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	if (carry_before) {
		SETFLAG(FLAG_C);
	} else {
		RESETFLAG(FLAG_C);
	}
	MODFLAG(FLAG_Y, result & (1 << 1));
	MODFLAG(FLAG_X, result & (1 << 3));
	reg.w.wz = reg.w.wz - 1;
}

void Z80::CPDR() {
	int carry_before = GETFLAG(FLAG_C);

	ZBYTE value;

	CPHL(value);

	if(GETFLAG(FLAG_H)) {
		value--;
	}
	reg.w.hl--;
	reg.w.bc--;
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	if (carry_before) {
		SETFLAG(FLAG_C);
	} else {
		RESETFLAG(FLAG_C);
	}
	MODFLAG(FLAG_Y, value & (1 << 1));
	MODFLAG(FLAG_X, value & (1 << 3));
	if (reg.b.a == READBYTE(reg.w.hl) || reg.w.bc == 1) {
		reg.w.wz = reg.w.wz + 1;
	} else {
		reg.w.wz = pc - 1;
	}
	if (reg.w.bc != 0 && !GETFLAG(FLAG_Z)) {
		pc -= 2;
	}
}

void Z80::CPI() {
	int carry = GETFLAG(FLAG_C);

	ZBYTE value;

	CPHL(value);
	
	if(GETFLAG(FLAG_H)) {
		value--;
	}
	reg.w.hl++;
	reg.w.bc--;
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	MODFLAG(FLAG_C, carry);
	MODFLAG(FLAG_Y, value & (1 << 1));
	MODFLAG(FLAG_X, value & (1 << 3));
	reg.w.wz = reg.w.wz + 1;
}

void Z80::CPIR() {
	int carry = GETFLAG(FLAG_C);

	ZBYTE value;

	CPHL(value);

	if(GETFLAG(FLAG_H)) {
		value--;
	}
	reg.w.hl++;
	reg.w.bc--;
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	MODFLAG(FLAG_C, carry);
	MODFLAG(FLAG_Y, value & (1 << 1));
	MODFLAG(FLAG_X, value & (1 << 3));
	if (reg.b.a == READBYTE(reg.w.hl) || reg.w.bc == 1) {
		reg.w.wz = reg.w.wz + 1;
	} else {
		reg.w.wz = pc + 1;
	}
	if (reg.w.bc != 0 && !GETFLAG(FLAG_Z)) {
		pc -= 2;
	}
}

void Z80::CPL() {
	reg.b.a = ~reg.b.a;
	SETFLAG(FLAG_H | FLAG_N);
	ADJUSTUNDOCFLAGS(reg.b.a);
}

void Z80::CP_HL() {
	ZBYTE val = READBYTE(reg.w.hl);
	CP(val);
	ADJUSTUNDOCFLAGS(val);
}

void Z80::CP_R() {
	CP(REG(OP1));
	ADJUSTUNDOCFLAGS(REG(OP1));
}

void Z80::CP_n() {
	ZBYTE value = READBYTE(pc++);
	CP(value);
	ADJUSTUNDOCFLAGS(value);
}

void Z80::CP_off() {
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	CP(value);
	ADJUSTUNDOCFLAGS(value);
	reg.w.wz = PAIR(OP1) + offset;
}


void Z80::DAA() {
	int correction_factor = 0x00;
	int carry = 0;

	if (reg.b.a > 0x99 || GETFLAG(FLAG_C)) {
		correction_factor |= 0x60;
		carry = 1;
	}

	if ((reg.b.a & 0x0f) > 9 || GETFLAG(FLAG_H)) {
		correction_factor |= 0x06;
	}

	int a_before = reg.b.a;
	if (GETFLAG(FLAG_N)) {
		reg.b.a -= correction_factor;
	} else {
		reg.b.a += correction_factor;
	}

	MODFLAG(FLAG_H, (a_before ^ reg.b.a) & 0x10);
	MODFLAG(FLAG_C, carry);
	MODFLAG(FLAG_S, (reg.b.a & 0x80)!= 0);
	MODFLAG(FLAG_Z, (reg.b.a == 0));
	MODFLAG(FLAG_PV, parity[reg.b.a]);
	ADJUSTUNDOCFLAGS(reg.b.a);
}


void Z80::DEC_R() {
	DEC(REG(OP1));
}

void Z80::DEC_RR() {
	PAIR(OP1)--;
}

void Z80::DEC_ind() {
	ZBYTE value = READBYTE(PAIR(OP1));
	DEC(value);
	WRITEBYTE(PAIR(OP1), value);
}

void Z80::DEC_off() {
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	DEC(value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::DI() {
	iff1 = iff2 = 0;
	defer_irq = 1;
}

void Z80::DJNZ() {
	char offset = READBYTE(pc++);
	reg.b.b--;
	if (reg.b.b) {
		pc += offset;
		reg.w.wz = pc;
	}
}

void Z80::EI() {
	iff1 = iff2 = 1;
	defer_irq = 1;
}

void Z80::EXX() {
	ZWORD tmp = reg.w.bc;
	reg.w.bc = alt_reg.w.bc;
	alt_reg.w.bc = tmp;
	tmp = reg.w.de;
	reg.w.de = alt_reg.w.de;
	alt_reg.w.de = tmp;
	tmp = reg.w.hl;
	reg.w.hl = alt_reg.w.hl;
	alt_reg.w.hl = tmp;
}

void Z80::EX_RR_altRR() {
	ZWORD tmp = PAIR(OP1);
	PAIR(OP1) = ALT_PAIR(OP2);
	ALT_PAIR(OP2) = tmp;
}

void Z80::EX_RR_RR() {
	ZWORD tmp = PAIR(OP1);
	PAIR(OP1) = PAIR(OP2);
	PAIR(OP2) = tmp;
}

void Z80::EX_SP_RR() {
	ZWORD tmp = READWORD(reg.w.sp);
	WRITEWORD(reg.w.sp, PAIR(OP2));
	PAIR(OP2) = tmp;
	reg.w.wz = PAIR(OP2);
}

void Z80::HALT() {
	halted = 1;
	pc--;
}

void Z80::IM() {
	im = OP1;
}

void Z80::INC_R() {
	ZWORD halfcarry;
	
	halfcarry = REG(OP1) ^ (REG(OP1) + 1);
	REG(OP1)++;

	reg.b.f = (reg.b.f & FLAG_C) | (halfcarry & FLAG_H) | SZ_table[REG(OP1) & 0xff] | V_table[(halfcarry >> 7) & 0x03];
}

void Z80::INC_RR() {
	PAIR(OP1)++;
}

void Z80::INC_ind() {
	ZBYTE value = READBYTE(PAIR(OP1));
	INC(value);
	WRITEBYTE(PAIR(OP1), value);
}

void Z80::INC_off() {
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP1)+ offset);
	INC(value);

	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::IND() {
	ZBYTE value = ReadIO(reg.w.bc);
	WRITEBYTE(reg.w.hl, value);
	reg.w.hl--;
	reg.w.wz = reg.w.bc - 1; // Before Dec() reg B
	DEC(reg.b.b);
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	int flagvalue = value + ((reg.b.c - 1) & 0xff);
	MODFLAG(FLAG_H, flagvalue > 0xff);
	MODFLAG(FLAG_C, flagvalue > 0xff);
	MODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);
}

void Z80::INDR() {
	ZBYTE value = ReadIO(reg.w.bc);
	WRITEBYTE(reg.w.hl, value);
	reg.w.hl--;
	reg.w.wz = reg.w.bc - 1; // Before Dec() reg B
	DEC(reg.b.b);
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	int flagvalue = value + ((reg.b.c - 1) & 0xff);
	MODFLAG(FLAG_H, flagvalue > 0xff);
	MODFLAG(FLAG_C, flagvalue > 0xff);
	MODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);
	if (reg.b.b != 0) {
		pc -= 2;
	}
}

void Z80::INI() {
	ZBYTE value = ReadIO(reg.w.bc);
	WRITEBYTE(reg.w.hl, value);
	reg.w.hl++;
	reg.w.wz = reg.w.bc + 1; // Before Dec() reg B
	DEC(reg.b.b);
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	int flagvalue = value + ((reg.b.c + 1) & 0xff);
	MODFLAG(FLAG_H, flagvalue > 0xff);
	MODFLAG(FLAG_C, flagvalue > 0xff);
	MODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);
}

void Z80::INIR() {
	ZBYTE value = ReadIO(reg.w.bc);
	WRITEBYTE(reg.w.hl, value);
	reg.w.hl++;
	reg.w.wz = reg.w.bc + 1; // Before Dec() reg B
	DEC(reg.b.b);
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	int flagvalue = value + ((reg.b.c + 1) & 0xff);
	MODFLAG(FLAG_H, flagvalue > 0xff);
	MODFLAG(FLAG_C, flagvalue > 0xff);
	MODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);
	if (reg.b.b != 0 && !GETFLAG(FLAG_Z)) {
		pc -= 2;
	}
}

void Z80::IN_R_c() {
	REG(OP1) = ReadIO(reg.w.bc);

	reg.b.f = SZP_table[REG(OP1)] | GETFLAG(FLAG_C);
	RESETFLAG(FLAG_H | FLAG_N);
	ADJUSTUNDOCFLAGS(REG(OP1));
	reg.w.wz = reg.w.bc + 1;
}

void Z80::IN_R_n() {
	ZWORD port = (REG(OP1) << 8) | READBYTE(pc++);
	reg.w.wz = (REG(OP1) << 8) + port + 1; // reg A before operation
	REG(OP1) = ReadIO(port);
}


void Z80::JP_cond() {
	ZWORD addr = READWORD(pc);
	pc += 2;
	if (Condition(OP1)) {
		pc = addr;
	}
	reg.w.wz = pc;
}

void Z80::JP_ind() {
	pc = PAIR(OP1);
	//reg.w.wz = pc;
}

void Z80::JP_nn() {
	pc = READWORD(pc);
	reg.w.wz = pc;
}

void Z80::JR_cond_d() {
	int offset;

	ZBYTE value = READBYTE(pc++);
	if ((value & 0x80) == 0) {
		offset = value;
	} else {
		value = ~value;
		value &= 0x7F;
		value++;
		offset = -value;
	}

	//int offset = Complement(READBYTE(pc++));
	if (Condition(OP1)) {
		pc += offset;
		reg.w.wz = pc;
	}
	
}

void Z80::JR_d() {
	int offset;
	
	ZBYTE value = READBYTE(pc++);
	if ((value & 0x80) == 0) {
		offset = value;
	} else {
		value = ~value;
		value &= 0x7F;
		value++;
		offset = -value;
	}
	pc += offset;
	reg.w.wz = pc;
}

void Z80::LDD() {
	ZBYTE value = READBYTE(reg.w.hl);
	WRITEBYTE(reg.w.de, value);
	reg.w.de--;
	reg.w.hl--;
	reg.w.bc--;
	MODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);
	MODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);
	RESETFLAG(FLAG_H | FLAG_N);
	MODFLAG(FLAG_PV, reg.w.bc != 0);
}

void Z80::LDDR() {
	ZBYTE value = READBYTE(reg.w.hl);
	WRITEBYTE(reg.w.de, value);
	reg.w.de--;
	reg.w.hl--;
	reg.w.bc--;
	MODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);
	MODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);
	RESETFLAG(FLAG_H | FLAG_N);
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	if (reg.w.bc != 1) {
		reg.w.wz = pc + 1;
	}
	if (reg.w.bc != 0) {
		pc -= 2;
	}
}

void Z80::LDI() {
	ZBYTE value = READBYTE(reg.w.hl);
	WRITEBYTE(reg.w.de, value);
	reg.w.de++;
	reg.w.hl++;
	reg.w.bc--;
	MODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);
	MODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);
	RESETFLAG(FLAG_H | FLAG_N);
	MODFLAG(FLAG_PV, reg.w.bc != 0);
}

void Z80::LDIR() {
	ZBYTE value = READBYTE(reg.w.hl);
	WRITEBYTE(reg.w.de, value);
	reg.w.de++;
	reg.w.hl++;
	reg.w.bc--;
	MODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);
	MODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);
	RESETFLAG(FLAG_H | FLAG_N);
	MODFLAG(FLAG_PV, reg.w.bc != 0);
	if (reg.w.bc != 1) {
		reg.w.wz = pc + 1;
	}
	if (reg.w.bc != 0) {
		pc -= 2;
	}
}

void Z80::LD_R_spec() {
	REG(OP1) = REG(OP2);
	RESETFLAG(FLAG_H | FLAG_N);
	MODFLAG(FLAG_PV, iff2);
	MODFLAG(FLAG_S, (REG(OP1) & 0x80) != 0);
	MODFLAG(FLAG_Z, (REG(OP1) == 0));
	ADJUSTUNDOCFLAGS(REG(OP1));
}

void Z80::LD_R_off() {
	char offset = READBYTE(pc++);
	REG(OP1) = READBYTE(PAIR(OP2) + offset);
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::LD_RR_RR() {
	PAIR(OP1) = PAIR(OP2);
}

void Z80::LD_RR_addr() {
	ZWORD addr = READWORD(pc);
	pc += 2;
	PAIR(OP1) = READWORD(addr);
	reg.w.wz = addr + 1;
}

void Z80::LD_RR_nn() {
	PAIR(OP1) = READWORD(pc);
	pc += 2;
}

void Z80::LD_R_R() {
	REG(OP1) = REG(OP2);
}

void Z80::LD_R_addr() {
	ZWORD addr = READWORD(pc);
	REG(OP1) = READBYTE(addr);
	pc += 2;
	reg.w.wz = addr + 1;
}

void Z80::LD_R_ind() {
	REG(OP1) = READBYTE(PAIR(OP2));

	if (OP2 == 1 || OP2 == 2) {
		reg.w.wz = PAIR(OP2) + 1;
	}
}

void Z80::LD_R_n() {
	REG(OP1) = READBYTE(pc++);
}


void Z80::LD_addr_R() {
	ZWORD addr = READWORD(pc);
	WRITEBYTE(addr, REG(OP2));
	reg.b.w = REG(OP1);
	reg.b.z = READBYTE(addr + 1) & 0xff;
	pc += 2;
}

void Z80::LD_addr_RR() {
	ZWORD addr = READWORD(pc);
	WRITEWORD(addr, PAIR(OP2));
	reg.w.wz = addr + 1;
	pc += 2;
}

void Z80::LD_ind_R() {
	WRITEBYTE(PAIR(OP1), REG(OP2));

	if (OP1 == 1 || OP1 == 2) {
		reg.b.w = (PAIR(OP1) + 1) & 0xff;
		reg.b.z = REG(OP2);
	}
}

void Z80::LD_ind_n() {
	WRITEBYTE(PAIR(OP1), READBYTE(pc++));
}

void Z80::LD_off_R() {
	char offset = READBYTE(pc++);
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::LD_off_n() {
	char offset = READBYTE(pc++);
	WRITEBYTE(PAIR(OP1) + offset, READBYTE(pc++));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::NEG() {
	ZBYTE tmp = reg.b.a;
	reg.b.a = 0;


	ZWORD result, carry, halfcarry;

	result = reg.b.a - tmp;

	halfcarry = reg.b.a ^ result ^ tmp;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;

	reg.b.a = result;
	
	SETFLAG(FLAG_N);
}


void Z80::NOP() {}

void Z80::OR_HL() {
	reg.b.a |= READBYTE(reg.w.hl);
	reg.b.f = SZP_table[reg.b.a];
}

void Z80::OR_R() {
	reg.b.a |= REG(OP1);
	reg.b.f = SZP_table[reg.b.a];
}

void Z80::OR_off() {
	char offset = READBYTE(pc++);
	reg.b.a |= READBYTE(PAIR(OP1) + offset);
	reg.b.f = SZP_table[reg.b.a];
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::OR_n(){
	reg.b.a |= READBYTE(pc++);
	reg.b.f = SZP_table[reg.b.a];
}

void Z80::OTDR() {
	ZBYTE value = READBYTE(reg.w.hl);
	DEC(reg.b.b);
	reg.w.wz = reg.w.bc - 1; // After Dec reg B
	WriteIO(reg.w.bc, value);
	reg.w.hl--;
	int flag_value = value + reg.b.l;
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	MODFLAG(FLAG_H, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_C, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);
	ADJUSTUNDOCFLAGS(reg.b.b);
	if (reg.b.b != 0) {
		pc -= 2;
	}			
}

void Z80::OTIR() {
	ZBYTE value = READBYTE(reg.w.hl);
	DEC(reg.b.b);
	reg.w.wz = reg.w.bc + 1; // After Dec reg B
	WriteIO(reg.w.bc, value);
	reg.w.hl++;
	int flag_value = value + reg.b.l;
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	MODFLAG(FLAG_H, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_C, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);
	ADJUSTUNDOCFLAGS(reg.b.b);
	if (reg.b.b != 0) {
		pc -= 2;
	}
}

void Z80::OUTD() {
	ZBYTE value = READBYTE(reg.w.hl);
	DEC(reg.b.b);
	reg.w.wz = reg.w.bc - 1; // After Dec reg B
	WriteIO(reg.w.bc, value);
	reg.w.hl--;
	int flag_value = value + reg.b.l;
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	MODFLAG(FLAG_H, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_C, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);
	ADJUSTUNDOCFLAGS(reg.b.b);
}

void Z80::OUTI() {
	ZBYTE value = READBYTE(reg.w.hl);
	DEC(reg.b.b);
	reg.w.wz = reg.w.bc + 1; // After Dec reg B
	WriteIO(reg.w.bc, value);
	reg.w.hl++;
	int flag_value = value + reg.b.l;
	MODFLAG(FLAG_N, (value & 0x80) != 0);
	MODFLAG(FLAG_H, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_C, (flag_value > 0xff) != 0);
	MODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);
	ADJUSTUNDOCFLAGS(reg.b.b);
}

void Z80::OUT_c_0() {
	WriteIO(reg.w.bc, 0);
	reg.w.wz = reg.w.bc + 1;
}

void Z80::OUT_c_R() {
	WriteIO(reg.w.bc, REG(OP2));
	reg.w.wz = reg.w.bc + 1;
}

void Z80::OUT_n_R() {
	ZWORD port = (REG(OP2) << 8) | READBYTE(pc++);
	WriteIO(port, REG(OP2));
	reg.b.w = REG(OP2);
	reg.b.z = (port + 1) & 0xff;
}

void Z80::POP() {
	ZWORD value = READWORD(reg.w.sp);
	reg.w.sp += 2;
	PAIR(OP1) = value;
}

void Z80::PUSH() {
	/* Push */
	reg.w.sp -= 2;
	WRITEWORD(reg.w.sp, PAIR(OP1));
	/* Push */
}

void Z80::RES_n_HL() {
	WRITEBYTE(reg.w.hl, (READBYTE(reg.w.hl) & ~(1 << OP1)) );
}

void Z80::RES_n_R() {
	REG(OP2) = (REG(OP2) & ~(1 << OP1));
}

void Z80::RES_n_off() {
	char offset = READBYTE(pc++ - 1);
	WRITEBYTE(PAIR(OP2) + offset, (READBYTE(PAIR(OP2) + offset) & ~(1 << OP1)));
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::RES_n_off_R() {
	char offset = READBYTE(pc++ - 1);
	REG(OP3) = (READBYTE(PAIR(OP2) + offset) & ~(1 << OP1));
	WRITEBYTE(PAIR(OP2) + offset, REG(OP3));
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::RET() {
	ZWORD value = READWORD(reg.w.sp);
	reg.w.sp += 2;
	pc = value;
	reg.w.wz = pc;
}

void Z80::RETI() {
	iff1 = iff2;
	
	ZWORD value = READWORD(reg.w.sp);
	reg.w.sp += 2;
	pc = value;
	reg.w.wz = pc;
}

void Z80::RETN() {
	iff1 = iff2;
	ZWORD value = READWORD(reg.w.sp);
	reg.w.sp += 2;	
	pc = value;
}

void Z80::RET_cond() {
	if (Condition(OP1)) {
		ZWORD value = READWORD(reg.w.sp);
		reg.w.sp += 2;
		pc = value;
	}
	reg.w.wz = pc;
}

void Z80::RLA() {
	RL(0, reg.b.a);
}

void Z80::RLCA() {
	RLC(0, reg.b.a);
}

void Z80::RLC_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	RLC(1, value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::RLC_R() {
	RLC(1, REG(OP1));
}

void Z80::RLC_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RLC(1, value)
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RLC_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RLC(1, value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RLD() {
	ZBYTE Ah = reg.b.a & 0x0f;
	ZBYTE hl = READBYTE(reg.w.hl);
	reg.b.a = (reg.b.a & 0xf0) | ((hl & 0xf0) >> 4);
	hl = (hl << 4) | Ah;
	WRITEBYTE(reg.w.hl, hl);

	reg.b.f = SZP_table[reg.b.a] | GETFLAG(FLAG_C);
	RESETFLAG(FLAG_H | FLAG_N);
	ADJUSTUNDOCFLAGS(reg.b.a);
	reg.w.wz = reg.w.hl + 1;
}

void Z80::RL_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	RL(1, value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::RL_R() {
	RL(1, REG(OP1));
}

void Z80::RL_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RL(1, value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RL_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RL(1, value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RRA() {
	RR(0, reg.b.a);
}

void Z80::RRCA() {
	RRC(0, reg.b.a);
}

void Z80::RRC_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	RRC(1, value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::RRC_R() {
	RRC(1, REG(OP1));
}

void Z80::RRC_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RRC(1, value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = reg.w.ix + offset;
}

void Z80::RRC_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RRC(1, value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RRD() {
	ZBYTE Ah = reg.b.a & 0x0f;
	ZBYTE hl = READBYTE(reg.w.hl);
	reg.b.a = (reg.b.a & 0xf0) | (hl & 0x0f);
	hl = (hl >> 4) | (Ah << 4);
	WRITEBYTE(reg.w.hl, hl);
	reg.b.f = SZP_table[reg.b.a] | GETFLAG(FLAG_C);
	RESETFLAG(FLAG_H | FLAG_N);
	ADJUSTUNDOCFLAGS(reg.b.a);
	reg.w.wz = reg.w.hl + 1;
}

void Z80::RR_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	RR(1, value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::RR_R() {
	RR(1, REG(OP1));
}

void Z80::RR_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RR(1, value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = reg.w.ix + offset;
}

void Z80::RR_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	RR(1, value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::RST() {
	/* Push */
	reg.w.sp -= 2;
	WRITEWORD(reg.w.sp, pc);
	/* Push */
	pc = OP1;
	reg.w.wz = pc;
}

void Z80::SBC_RR_RR() {
		ZDWORD result, halfcarry;

		reg.w.wz = PAIR(OP1) + 1; // first register before operation
		
		result = PAIR(OP1) - PAIR(OP2) - (reg.b.f & FLAG_C);

		halfcarry = PAIR(OP1) ^ PAIR(OP2) ^ result;

		reg.b.f = FLAG_N;
		reg.b.f |= result & 0xffff ? (result >> 8) & (FLAG_S | FLAG_X | FLAG_Y) : FLAG_Z;
		reg.b.f |= (halfcarry >> 8) & FLAG_H;
		halfcarry &= 0x018000;
		reg.b.f |= V_table[halfcarry >> 15];
		reg.b.f |= halfcarry >> 16;
	ADJUSTUNDOCFLAGS(result >> 8);
		PAIR(OP1) = result;
}

void Z80::SBC_R_HL() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(reg.w.hl);

	result = reg.b.a - value - (reg.b.f & FLAG_C);

	halfcarry = reg.b.a ^ value ^ result;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result & 0xff;
}

void Z80::SBC_R_n() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(pc);

	result = reg.b.a - value - (reg.b.f & FLAG_C);

	halfcarry = reg.b.a ^ value ^ result;
	
	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;

	reg.b.a = result & 0xff;
	ADJUSTUNDOCFLAGS(result);

	pc++;
}

void Z80::SBC_R_R() {
	ZWORD result, carry, halfcarry;

	result = reg.b.a - REG(OP2) - (reg.b.f & FLAG_C);

	halfcarry = reg.b.a ^ REG(OP2) ^ result;
		
	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result & 0xff;
}

void Z80::SBC_R_off() {
	ZWORD result, carry, halfcarry;
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP2) + offset);

	result = reg.b.a - value - (reg.b.f & FLAG_C);

	halfcarry = reg.b.a ^ value ^ result;
	
	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;

	reg.b.a = result & 0xff;
	ADJUSTUNDOCFLAGS(result);
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::SCF() {
	SETFLAG(FLAG_C);
	RESETFLAG(FLAG_N | FLAG_H);
	ADJUSTUNDOCFLAGS(reg.b.a);
}

void Z80::SET_n_HL() {
	WRITEBYTE(reg.w.hl, (READBYTE(reg.w.hl) | (1 << OP1)));
}

void Z80::SET_n_R() {
	REG(OP2) |= (1 << OP1);
}

void Z80::SET_n_off() {
	char offset = READBYTE(pc++ - 1);
	WRITEBYTE(PAIR(OP2) + offset, (READBYTE(PAIR(OP2) + offset) | (1 << OP1)));
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::SET_n_off_R() {
	char offset = READBYTE(pc++ - 1);
	REG(OP3) = (READBYTE(PAIR(OP2) + offset) | (1 << OP1));
	WRITEBYTE(PAIR(OP2) + offset, REG(OP3));
	reg.w.wz = PAIR(OP2) + offset;
}

void Z80::SLA_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	SLA(value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::SLA_R() {
	SLA(REG(OP1));
}

void Z80::SLA_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SLA(value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SLA_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SLA(value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SLL_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	SLL(value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::SLL_R() {
	SLL(REG(OP1));
}

void Z80::SLL_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SLL(value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SLL_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SLL(value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SRA_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	SRA(value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::SRA_R() {
	SRA(REG(OP1));
}

void Z80::SRA_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SRA(value);
	WRITEBYTE(PAIR(OP1) + offset, value);
}

void Z80::SRA_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SRA(value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
}

void Z80::SRL_HL() {
	ZBYTE value = READBYTE(reg.w.hl);
	SRL(value);
	WRITEBYTE(reg.w.hl, value);
}

void Z80::SRL_R() {
	SRL(REG(OP1));
}

void Z80::SRL_off() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SRL(value);
	WRITEBYTE(PAIR(OP1) + offset, value);
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SRL_off_R() {
	char offset = READBYTE(pc++ - 1);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);
	SRL(value);
	REG(OP2) = value;
	WRITEBYTE(PAIR(OP1) + offset, REG(OP2));
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::SUB_HL() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(reg.w.hl);

	result = reg.b.a - value;

	halfcarry = reg.b.a ^ value ^ result;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result & 0xff;
}

void Z80::SUB_R() {
	ZWORD result, carry, halfcarry;

	result = reg.b.a - REG(OP1);

	halfcarry = reg.b.a ^ REG(OP1) ^ result;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;
	ADJUSTUNDOCFLAGS(result);
	reg.b.a = result & 0xff;
}

void Z80::SUB_n() {
	ZWORD result, carry, halfcarry;
	ZBYTE value = READBYTE(pc);

	result = reg.b.a - value;

	halfcarry = reg.b.a ^ value ^ result;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;

	reg.b.a = result & 0xff;

	ADJUSTUNDOCFLAGS(result);
	pc++;
}


void Z80::SUB_off() {
	ZWORD result, carry, halfcarry;
	char offset = READBYTE(pc++);
	ZBYTE value = READBYTE(PAIR(OP1) + offset);

	result = reg.b.a - value;

	halfcarry = reg.b.a ^ value ^ result;

	reg.b.f = FLAG_N | (halfcarry & FLAG_H);
	reg.b.f |= SZ_table[result & 0xff];
	halfcarry &= 0x0180;
	reg.b.f |= V_table[halfcarry >> 7];
	carry = halfcarry >> 8;
	reg.b.f |= carry;

	reg.b.a = result & 0xff;

	ADJUSTUNDOCFLAGS(result);
	reg.w.wz = PAIR(OP1) + offset;
	
}

void Z80::XOR_HL() {
	reg.b.a ^= READBYTE(reg.w.hl);
	reg.b.f = SZP_table[reg.b.a];
}

void Z80::XOR_R() {
	reg.b.a ^= REG(OP1);
	reg.b.f = SZP_table[reg.b.a];
}

void Z80::XOR_off() {
	char offset = READBYTE(pc++);
	reg.b.a ^= READBYTE(PAIR(OP1) + offset);
	reg.b.f = SZP_table[reg.b.a];
	reg.w.wz = PAIR(OP1) + offset;
}

void Z80::XOR_n() {
	reg.b.a ^= READBYTE(pc++);
	reg.b.f = SZP_table[reg.b.a];
}