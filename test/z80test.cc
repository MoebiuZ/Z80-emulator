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


#include "z80test.h"

#define FLAG_S	(1 << 7)
#define FLAG_Z	(1 << 6)
#define FLAG_Y	(1 << 5)
#define FLAG_H	(1 << 4)
#define FLAG_X	(1 << 3)
#define FLAG_PV	(1 << 2)
#define FLAG_N	(1 << 1)
#define FLAG_C	(1 << 0)

#define COND_Z	0
#define COND_NZ	1
#define COND_C	2
#define COND_NC	3
#define COND_M	4
#define COND_P	5
#define COND_PE	6
#define COND_PO	7


using namespace std;


Z80Test::Z80Test() { }

ZBYTE Z80Test::IOReadCallback(ZWORD addr) {
	current_case->io.push_back(Z80Test::IOACCESS());
	current_case->io.back().prpw = 0;
	current_case->io.back().address = addr;
	current_case->io.back().data = addr >> 8;
	return addr >> 8;
}


void Z80Test::IOWriteCallback(ZWORD addr, ZBYTE data) {
	current_case->io.push_back(Z80Test::IOACCESS());
	current_case->io.back().prpw = 1;
	current_case->io.back().address = addr;
	current_case->io.back().data = data;
}


void Z80Test::Init() {
	emul = new Z80();

	emul->SetIOReadCallback(std::bind(&Z80Test::IOReadCallback, this, std::placeholders::_1));
	emul->SetIOWriteCallback(std::bind(&Z80Test::IOWriteCallback, this, std::placeholders::_1, std::placeholders::_2));

	// FUSE Z80 test suite, with some added tests and modifyed for undocumented effects
	cases_result = LoadTestCases("./test/tests.in");
	cases_expected = LoadTestCases("./test/tests.expected");
}


int Z80Test::TestAll() {
	int result;
	unsigned int exec_tstates;

	for (unsigned int i = 0; i < cases_result.size(); i++) {

		Z80Test::current_case = &cases_result[i];

		exec_tstates = SetupTest(i);
			
		emul->ExecuteTStates(exec_tstates);

		result = CheckResult(i);

		if (result) {
			failed.push_back(i);
			notpassed++;
		} else {
			passed++;
		}
	}


	for (unsigned int i = 0; i < failed.size(); i++) {
		ShowFailed(failed[i]);
	}

	cout << "PASSED TESTS: " << dec << passed << "\nFAILED TESTS: " << dec << notpassed << endl;


	return notpassed; // If 0, Unit Test passes
}



int Z80Test::CheckResult(int pos) {

	if (emul->reg.w.af != 		cases_expected[pos].reg.w.af) return 1;
	if (emul->reg.w.bc != 		cases_expected[pos].reg.w.bc) return 1;
	if (emul->reg.w.de != 		cases_expected[pos].reg.w.de) return 1;
	if (emul->reg.w.hl != 		cases_expected[pos].reg.w.hl) return 1;
	if (emul->alt_reg.w.af != 	cases_expected[pos].alt_reg.w.af) return 1;
	if (emul->alt_reg.w.bc != 	cases_expected[pos].alt_reg.w.bc) return 1;
	if (emul->alt_reg.w.de != 	cases_expected[pos].alt_reg.w.de) return 1;
	if (emul->alt_reg.w.hl != 	cases_expected[pos].alt_reg.w.hl) return 1;
	if (emul->reg.w.ix != 		cases_expected[pos].reg.w.ix) return 1;
	if (emul->reg.w.iy != 		cases_expected[pos].reg.w.iy) return 1;
	if (emul->reg.w.sp != 		cases_expected[pos].reg.w.sp) return 1;
	if (emul->pc != 			cases_expected[pos].pc) return 1;
	if (emul->reg.b.i !=		cases_expected[pos].reg.b.i) return 1;
	if (emul->reg.b.r !=		cases_expected[pos].reg.b.r) return 1;
	if (emul->iff1 !=			cases_expected[pos].iff1) return 1;
	if (emul->iff2 !=			cases_expected[pos].iff2) return 1;
	if (emul->im !=				cases_expected[pos].im) return 1;
	if (emul->halted !=			cases_expected[pos].halted) return 1;

	if (emul->tstates != 		cases_expected[pos].tstates) return 1;

	if (DiffMem(pos)) return 1;

	if (cases_result[pos].io.size() != cases_expected[pos].io.size()) {
		return 1;
	}
	for (unsigned int i = 0; i < cases_expected[pos].io.size(); i++) {
		if (cases_result[pos].io[i].prpw != cases_expected[pos].io[i].prpw) {
			return 1;
		}
		if (cases_result[pos].io[i].address != cases_expected[pos].io[i].address) {
			return 1;
		}
		if (cases_result[pos].io[i].data != cases_expected[pos].io[i].data) {
			return 1;
		}
	}

	return 0;
}


int Z80Test::SetupTest(int pos) {
		emul->memory = memory;

		emul->Reset();
		emul->reg.w.af = 		cases_result[pos].reg.w.af;
		emul->reg.w.bc = 		cases_result[pos].reg.w.bc;
		emul->reg.w.de = 		cases_result[pos].reg.w.de;
		emul->reg.w.hl = 		cases_result[pos].reg.w.hl;
		emul->alt_reg.w.af = 	cases_result[pos].alt_reg.w.af;
		emul->alt_reg.w.bc = 	cases_result[pos].alt_reg.w.bc;
		emul->alt_reg.w.de = 	cases_result[pos].alt_reg.w.de;
		emul->alt_reg.w.hl = 	cases_result[pos].alt_reg.w.hl;
		emul->reg.w.ix = 		cases_result[pos].reg.w.ix;
		emul->reg.w.iy = 		cases_result[pos].reg.w.iy;
		emul->reg.w.sp = 		cases_result[pos].reg.w.sp;
		emul->reg.w.wz = 		0x0000;
		emul->pc = 				cases_result[pos].pc;
		emul->reg.b.i =			cases_result[pos].reg.b.i;
		emul->reg.b.r =			cases_result[pos].reg.b.r;
		emul->iff1 =			cases_result[pos].iff1;
		emul->iff2 =			cases_result[pos].iff2;
		emul->im =				cases_result[pos].im;
		emul->halted =			cases_result[pos].halted;

		FillMemory();
		for (unsigned int j = 0; j < cases_result[pos].mem.size(); j++) {
			for (unsigned int k = 0; k < cases_result[pos].mem[j].data.size(); k++) {
				memory[cases_result[pos].mem[j].address + k] = cases_result[pos].mem[j].data[k];
			}
		}
		memcpy(initial_memory, memory, 0xffff + 1);

		return cases_expected[pos].tstates;
}


int Z80Test::DiffMem(int pos) {

	for (unsigned int i = 0; i < cases_expected[pos].mem.size(); i++) {
		for (unsigned int j = 0; j < cases_expected[pos].mem[i].data.size(); j++) {
			initial_memory[cases_expected[pos].mem[i].address + j] = cases_expected[pos].mem[i].data[j];
		}
	}

	for (unsigned int i = 0; i < 0xffff + 1; i++) {
		if (memory[i] != initial_memory[i]) {
			return 1;
		}
	}

	return 0;
}



void Z80Test::ShowFailed(int pos) {

	unsigned int exec_tstates;
	ZBYTE emul_flags, expect_flags;

	exec_tstates = SetupTest(pos);

	while (emul->tstates < exec_tstates) {
		emul->ExecuteTStates(exec_tstates);
	}

	emul_flags = (((emul->reg.w.af << 8) >> 8)& 0x00ff);
	expect_flags = (((cases_expected[pos].reg.w.af << 8) >> 8) & 0x00ff);

	cout	<< "TESTING OPCODE: ";
	cout	<< cases_result[pos].testname << endl << endl;
	cout	<< "    S  Z  Y  H  X P/V N  C  " << endl;
	cout	<< "   ------------------------" << endl;
	cout	<< "T | " << ((emul_flags & FLAG_S) != 0) << " "
			<< " " << ((emul_flags & FLAG_Z) != 0) << " "
			<< " " << ((emul_flags & FLAG_Y) != 0) << " "
			<< " " << ((emul_flags & FLAG_H) != 0) << " "
			<< " " << ((emul_flags & FLAG_X) != 0) << " "
			<< " " << ((emul_flags & FLAG_PV) != 0) << " "
			<< " " << ((emul_flags & FLAG_N) != 0) << " "
			<< " " << ((emul_flags & FLAG_C) != 0) << " |       T = Tested   E = Expected"
			<< endl;
	cout	<< "E | " << ((expect_flags & FLAG_S) != 0) << " "
			<< " " << ((expect_flags & FLAG_Z) != 0) << " "
			<< " " << ((expect_flags & FLAG_Y) != 0) << " "
			<< " " << ((expect_flags & FLAG_H) != 0) << " "
			<< " " << ((expect_flags & FLAG_X) != 0) << " "
			<< " " << ((expect_flags & FLAG_PV) != 0) << " "
			<< " " << ((expect_flags & FLAG_N) != 0) << " "
			<< " " << ((expect_flags & FLAG_C) != 0) << " |"
			<< endl;
	cout	<< "   ------------------------" << endl << endl;

	cout	<< "     PC    SP    AF    BC    DE    HL    AF'   BC'   DE'   HL'   IX    IY" << endl;
	cout	<< "   ------------------------------------------------------------------------" << endl;
	cout	<< internal << setfill('0');
	cout	<< "T | " << hex << setw(4) << emul->pc << "  " << setw(4) << emul->reg.w.sp
			<< "  " << setw(4) << emul->reg.w.af << "  " << setw(4) << emul->reg.w.bc
			<< "  " << setw(4) << emul->reg.w.de << "  " << setw(4) << emul->reg.w.hl
			<< "  " << setw(4) << emul->alt_reg.w.af << "  " << setw(4) << emul->alt_reg.w.bc
			<< "  " << setw(4) << emul->alt_reg.w.de << "  " << setw(4) << emul->alt_reg.w.hl
			<< "  " << setw(4) << emul->reg.w.ix << "  " << setw(4) << emul->reg.w.iy << " |" << endl;
	cout	<< "E | " << hex << setw(4) << cases_expected[pos].pc << "  " << setw(4) << cases_expected[pos].reg.w.sp
			<< "  " << setw(4) << cases_expected[pos].reg.w.af << "  " << setw(4) << cases_expected[pos].reg.w.bc
			<< "  " << setw(4) << cases_expected[pos].reg.w.de << "  " << setw(4) << cases_expected[pos].reg.w.hl
			<< "  " << setw(4) << cases_expected[pos].alt_reg.w.af << "  " << setw(4) << cases_expected[pos].alt_reg.w.bc
			<< "  " << setw(4) << cases_expected[pos].alt_reg.w.de << "  " << setw(4) << cases_expected[pos].alt_reg.w.hl
			<< "  " << setw(4) << cases_expected[pos].reg.w.ix << "  " << setw(4) << cases_expected[pos].reg.w.iy << " |" << endl;
	cout	<< "   ------------------------------------------------------------------------" << endl << endl;
	cout	<< "    I   R   IFF1 IFF2  IM  halted  tstates    " << endl;
	cout	<< "   ----------------------------------------" << endl;
	cout	<< internal << setfill('0');
	cout	<< "T | " << hex << setw(2) << (int) emul->reg.b.i << "  " << setw(2) << (int) emul->reg.b.r
			<< "  " << dec << emul->iff1 << "    " << dec << emul->iff2
			<< "     " << dec << emul->im << "     " << emul->halted;
	cout	<< internal << setfill(' ');				
	cout	<< "       "  << setw(4) << dec << emul->tstates << "  |" << endl;
	cout	<< internal << setfill('0');				
	cout	<< "E | " << hex << setw(2) << (int) cases_expected[pos].reg.b.i << "  " << setw(2) << (int) cases_expected[pos].reg.b.r
			<< "  " << dec << cases_expected[pos].iff1 << "    " << dec << cases_expected[pos].iff2
			<< "     " << dec << cases_expected[pos].im << "     " << cases_expected[pos].halted;
	cout	<< internal << setfill(' ');				
	cout	<< "       " << setw(4) << dec << cases_expected[pos].tstates << "  |" << endl;
	cout	<< "   ----------------------------------------" << endl << endl;

	for (unsigned int i = 0; i < cases_expected[pos].mem.size(); i++) {
		for (unsigned int j = 0; j < cases_expected[pos].mem[i].data.size(); j++) {
			initial_memory[cases_expected[pos].mem[i].address + j] = cases_expected[pos].mem[i].data[j];
		}
	}

	int first = 1;
	for (unsigned int i = 0; i < 0xffff + 1; i++) {
		if (memory[i] != initial_memory[i]) {
			if (first) {
				cout << "Address   T   E" << endl;
				cout << "----------------" << endl;
				first = 0;
			}
			cout << internal << setfill('0');
			cout << setw(4) << hex << i << "      " << setw(2) << hex << (ZWORD) memory[i] << "  " << setw(2) << hex << (ZWORD) initial_memory[i] << endl;
		}
	} 

	for (unsigned int i = 0; i < cases_expected[pos].io.size(); i++) {
		if (cases_expected[pos].io[i].prpw == 0) {
			cout << "PR ";
		} else {
			cout << "PW ";
		}
		cout << setw(4) << hex << cases_expected[pos].io[i].address << " " << hex << cases_expected[pos].io[i].data << endl;
	}
	cout << endl << endl << "____________________________________________" << endl << endl;

}


vector<Z80Test::TESTCASE> Z80Test::LoadTestCases(string file) {
	ifstream tests;
	vector<TESTCASE> cases;

	tests.open(file);

	cases.push_back(TESTCASE());

	int cnt = 0;
	int addr = 0;
	int databyte;
	char iomode[2];

	while(!tests.eof()) {
			 
		cases.push_back(TESTCASE());

		getline(tests, cases[cnt].testname);
		while (cases[cnt].testname == "") {
			getline(tests, cases[cnt].testname);
		}

		while (tests.peek() == 'P') {
			cases[cnt].io.push_back(IOACCESS());
			tests >> iomode;

			if (iomode[1] == 'W') {
				cases[cnt].io.back().prpw = 1;
			} else {
				cases[cnt].io.back().prpw = 0;
			}

			tests >> hex >> cases[cnt].io.back().address;
			tests >> hex >> cases[cnt].io.back().data;
			tests.get(); // remove \n before next peek()
		}

		ZWORD bi, br;
		tests >> hex >> cases[cnt].reg.w.af >> cases[cnt].reg.w.bc >> cases[cnt].reg.w.de >> cases[cnt].reg.w.hl;
		tests >> hex >> cases[cnt].alt_reg.w.af >> cases[cnt].alt_reg.w.bc >> cases[cnt].alt_reg.w.de >> cases[cnt].alt_reg.w.hl;
		tests >> hex >> cases[cnt].reg.w.ix >> cases[cnt].reg.w.iy >> cases[cnt].reg.w.sp >> cases[cnt].pc;
		tests >> hex >> bi >> br;
		tests >> hex >> cases[cnt].iff1 >> cases[cnt].iff2;
		tests >> dec >> cases[cnt].im >> cases[cnt].halted;
		tests >> cases[cnt].tstates;
		cases[cnt].reg.w.ir = ((bi << 8) | br);

		tests >> hex >> addr;

		while (addr != -1) {
			cases[cnt].mem.push_back(MEM());
			cases[cnt].mem.back().address = addr;

			tests >> hex >> databyte;
			do {
				cases[cnt].mem.back().data.push_back(databyte);
				tests >> hex >> databyte;
			} while (databyte != -1);
			tests >> hex >> addr;
		}
		cnt++;
	}
	
	return cases;
}



void Z80Test::FillMemory() {
		  
	for (int i = 0; i < 0xffff + 1; i += 4) {
		memory[i] = 	0xde; 
		memory[i + 1] = 0xad;
		memory[i + 2] = 0xbe; 
		memory[i + 3] = 0xef;
	}
}



int main() {

	Z80Test *test = new Z80Test();
	test->Init();

	return test->TestAll();

}
