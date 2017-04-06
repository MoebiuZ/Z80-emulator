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

#include <stdio.h>
#include <stdlib.h>
#include "z80.h"


class Z80Test {

public:
	Z80 *cpu;
	Z80ADDRESSBUS memory;

	Z80Test();
	~Z80Test();

	void Emulate(char filename[256]);
	void Init();

private:
	ZBYTE IOReadCallback(ZWORD addr);
	void IOWriteCallback(ZWORD addr, ZBYTE data);

	void SystemCall();
	
};