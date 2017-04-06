/*
 * Nostalgic Z80 emulator
 * Copyright (c) 2016, Antonio Rodriguez <@MoebiuZ>
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
 

#include <stdio.h>
#include <string.h>
#include <iostream> // Using iostream just for fun
#include <fstream>
#include <iomanip>
#include <vector>

#include "z80.h"


using namespace std;


class Z80Test {

public:


	typedef struct MEM {
		ZWORD address;
		vector<int> data;
	} MEM;

	typedef struct IOACCESS {
		int prpw;
		ZWORD address;
		int data;
	} IOACCESS;

	typedef struct TESTCASE {
		string testname;
		Z80::Z80REGISTERS reg;
		Z80::Z80REGISTERS alt_reg;
		ZWORD pc;
		unsigned int iff1;
		unsigned int iff2;
		unsigned int im;
		unsigned int halted;
		unsigned int tstates;
		vector<MEM> mem;
		vector<IOACCESS> io;
	} TESTCASE;


	vector<TESTCASE> cases_result;
	vector<TESTCASE> cases_expected;

	TESTCASE *current_case;

	Z80ADDRESSBUS initial_memory, memory;


	Z80 *emul;

	int passed = 0;
	int notpassed = 0;
	vector<int> failed;


	Z80Test();

	void Init();

	int TestAll();
	int CheckResult(int pos);
	int SetupTest(int pos);
	int DiffMem(int pos);
	void ShowFailed(int pos);
	vector<TESTCASE> LoadTestCases(string file);
	void FillMemory();
	ZBYTE IOReadCallback(ZWORD addr);
	void IOWriteCallback(ZWORD addr, ZBYTE data);

};