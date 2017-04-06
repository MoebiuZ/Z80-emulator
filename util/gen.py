'''
  Nostalgic Z80 emulator
  Copyright (c) 2016, Antonio Rodriguez <@MoebiuZ>
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
  http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
'''


## TODO: The code is a mess, needs some rework

import json
import re
from collections import OrderedDict

byte_reg = [ "A", "B", "C", "D", "E", "F", "H", "L", "IXH", "IXL", "IYH", "IYL" ]
alt_w_reg = [ "AF'", "BC'", "DE'", "HL'" ]
word_reg = [ "DE", "HL", "AF", "AF'", "BC", "IY", "IX", "SP" ]
addr_w_reg = [ "(HL)", "(SP)", "(BC)", "(DE)", "(IY)", "(IX)" ]
addr_w_reg_dest = [ "d(HL)", "d(SP)", "d(BC)", "d(DE)", "d(IY)", "d(IX)" ]
disp_reg = [ "(IX+d)", "(IY+d)" ]
cond_reg = [ "C_NZ", "C_Z", "C_NC", "C_C", "C_PO", "C_PE", "C_P", "C_M" ]
alt_cond_reg = [ "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" ]
number_reg = ["0", "1", "2", "3", "4", "5", "6", "7" ]
spec_reg = [ "R", "I"]
hex_reg = [ "00", "08", "10", "18", "20", "28", "30", "38"]

# (nn) nn

with open('instructions.json') as data_file:
		data = json.load(data_file, object_pairs_hook=OrderedDict)



already = []

arrays = []
sorted_declarations = []
sorted_functions = []



def genName(mnem):

	mnem = re.sub(" ", ur"_", mnem)
	mnem = re.sub(",", ur"_", mnem)
	mnem = re.sub("->", ur"_", mnem)
	mnem = re.sub("\+d", ur"d", mnem)
	mnem = re.sub("\((.+)\)", ur"a\1", mnem)
	mnem = re.sub("'", "alt", mnem)

	return str(mnem)


def genArg(instruction):

	if instruction in cond_reg:
		return str(instruction.lower())
	elif instruction in byte_reg:
		return "reg.b." + str(instruction.lower())
	elif instruction in alt_w_reg:
		return "alt_reg.w." + str(re.sub("(\S+?)\'", "\\1", instruction.lower()))
	elif instruction in word_reg:
		if instruction == "SP":
			return str(instruction.lower())
		else:
			return "reg.w." + str(instruction.lower())

	elif instruction in addr_w_reg:
		if instruction == "SP":
			return "READBYTE(" + str(re.sub("[\(\)]", "", instruction.lower())) + ")"
		else:
			return "READBYTE(reg.w." + str(re.sub("[\(\)]", "", instruction.lower())) + ")"

	elif instruction in addr_w_reg_dest:
		if instruction == "SP":
			return str(re.sub("d\((\S+?)\)", "\\1", instruction.lower()))
		else:
			return "reg.w." + str(re.sub("d\((\S+?)\)", "\\1", instruction.lower()))

	elif instruction in disp_reg:
		return "reg.w." + str(re.sub("[\(\)\+d]", "", instruction.lower()))
		#return "READBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction.lower())) + " + READBYTE(pc++))"
	elif instruction == "n":
		return "READBYTE(pc++)"
	elif instruction == "nn":
		return "READWORD(pc)"
	elif instruction in number_reg:
		return str(instruction)
	elif instruction in spec_reg:
		return str(instruction).lower()
	elif instruction in hex_reg:
		return str(instruction)



def looparray(tabla):

	array = "\tOPCODES " + str(tabla).lower() + "[0xff + 1] {"

	for i in range(0, 256):
		if i % 8 == 0:
			array += "\n\t\t"

		tmp = genName(str(data[tabla][i]["mnemonic"]))

		if "---" in tmp:
			array += "&Z80::NOP"
		else:
			array += "&Z80::" + genName(str(data[tabla][i]["mnemonic"]))

		if i != 255:
			array += ", "
		else:
			array += " "



	array += "};\n\n"

	arrays.append(array)

	for i in range(0, 256):

		if data[tabla][i]["mnemonic"] not in already:

			declaration = "\tvoid " + genName(str(data[tabla][i]["mnemonic"])) + "();\t\t// " + str(data[tabla][i]["mnemonic"])

			funcion = "void Z80::" + genName(str(data[tabla][i]["mnemonic"])) + "() {\n"

			

			# TODO: Fix specific cases with tstates-nojmp


			already.append(data[tabla][i]["mnemonic"])

			instruction = re.split(" |,|->", data[tabla][i]["mnemonic"])

			if instruction[0] != "---":

				funcion += "\t#ifdef __Z80ACCURATE__\n"
				#funcion += "\tif (mcycles_counter > 1) {\n"
				#funcion += "\t\ttstates += 4;\n"
				#funcion += "\t\treturn;\n"
				funcion += "\tif (mcycles_counter == 1) {\n"


				funcion += "\t#endif\n\n"


			if instruction[0] == "ADC":

				if instruction[1] in word_reg:
					funcion += "\treg.w.wz = " + str(genArg(instruction[1])) + " + 1; // first register before operation\n"
					funcion += "\tADDWORDWITHCARRY(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"
				elif instruction[1] in byte_reg:
					if instruction[2] in disp_reg:
						funcion += "\tchar offset = READBYTE(pc++);\n"
						funcion += "\tADDWITHCARRY(READBYTE(" + str(genArg(instruction[2])) + " + offset));\n"
						funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
					else:
						if instruction[2] == "n":
							funcion += "\tADDWITHCARRY(READBYTE(pc));\n"
							funcion += "\tpc++;\n"
						else:
							funcion += "\tADDWITHCARRY(" + str(genArg(instruction[2])) + ");\n"
				else: # n
						funcion += "\tADDWITHCARRY(READBYTE(pc));\n"
						funcion += "\tpc++;\n"

				



			elif instruction[0] == "ADD":

				if instruction[1] in word_reg:
					funcion += "\treg.w.wz = " + str(genArg(instruction[1])) + " + 1; // first register before operation\n"
					funcion += "\tADDWORD(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"


				elif instruction[1] in byte_reg:
					if instruction[2] in disp_reg:
						funcion += "\tchar offset = READBYTE(pc++);\n"
						#funcion += "\t" + str(genArg(instruction[1])) + " = Add(READBYTE(" + str(genArg(instruction[2])) + " + offset));\n"
						funcion += "\tADD(READBYTE(" + str(genArg(instruction[2])) + " + offset));\n"
						funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
					elif instruction[2] == "n":
						#funcion += "\t" + str(genArg(instruction[1])) + " = Add(" + str(genArg(instruction[2])) + ");\n"
						funcion += "\tADD(READBYTE(pc));\n"
						funcion += "\tpc++;\n"
					else:
						#funcion += "\t" + str(genArg(instruction[1])) + " = Add(" + str(genArg(instruction[2])) + ");\n"
						funcion += "\tADD(" + str(genArg(instruction[2])) + ");\n"
				else: # n
						#funcion += "\t" + str(genArg(instruction[1])) + " = Add(READBYTE(pc++));\n"
						funcion += "\tADD(READBYTE(pc));\n"
						funcion += "\tpc++;\n"

				



			elif instruction[0] == "AND":
				if instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tAnd(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				else:
					funcion += "\tAnd(" + str(genArg(instruction[1])) + ");\n"

				



			elif instruction[0] == "BIT":
				if instruction[2] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					funcion += "\tWORD addr = " + str(genArg(instruction[2])) + " + offset;\n"
					funcion += "\tBit(" + str(genArg(instruction[1])) + ", READBYTE(addr));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
					funcion += "\tAdjustUndocFlags(reg.b.w); // Same as addr >> 8\n"
				elif instruction[2] in byte_reg:
					funcion += "\tBit(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"
					funcion += "\tAdjustUndocFlags(" + str(genArg(instruction[2])) + ");\n"
					
				elif instruction[2] == "(HL)":
					funcion += "\treg.w.wz = READWORD(reg.w.hl);\n"
					funcion += "\tBit(" + str(genArg(instruction[1])) + ", READBYTE(reg.w.hl));\n"
					
					funcion += "\tAdjustUndocFlags(READBYTE(reg.w.hl));\n"
 					#funcion += "\tAdjustUndocFlags(reg.b.w); // Flags x and y are bits 11 and 13 of WZ\n"

									



			elif instruction[0] == "CALL":
				funcion += "\tWORD addr = READWORD(pc);\n"
				funcion += "\tpc += 2;\n"
				if instruction[1] in alt_cond_reg:
					funcion += "\tif (Condition(" + str(genArg("C_" + instruction[1])) + ")) {\n"
					funcion += "\t\tPush(pc);\n"
					funcion += "\t\tpc = addr;\n"
					funcion += "\t\t#ifndef __Z80ACCURATE__\n"
					funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
					funcion += "\t\t#endif\n"
					funcion += "\t}\n"

				else: # nn
					funcion += "\tPush(pc);\n"
					funcion += "\tpc = addr;\n"

				funcion += "\treg.w.wz = pc;\n"
					



			elif instruction[0] == "CCF":
				funcion += "\tMODFLAG(FLAG_H, GETFLAG(FLAG_C));\n"
				funcion += "\tMODFLAG(FLAG_C, (1 - (BYTE) GETFLAG(FLAG_C) != 0));\n"
				funcion += "\tRESETFLAG(FLAG_N);\n"
				funcion += "\tAdjustUndocFlags(reg.b.a);\n"
				



			elif instruction[0] == "CP":
				if instruction[1] == "(HL)":
					funcion += "\tCP_HL();\n"
				elif instruction[1] in byte_reg:
					funcion += "\tCP(" + str(genArg(instruction[1])) + ");\n"
					funcion += "\tAdjustUndocFlags(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] == "n":
					funcion += "\tBYTE value = READBYTE(pc++);\n"
					funcion += "\tCP(value);\n"
					#funcion += "\tAdjustUndocFlags(value);\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tBYTE value = READBYTE(" + str(genArg(instruction[1])) + " + offset);\n"
					funcion += "\tCP(value);\n"
					funcion += "\tAdjustUndocFlags(value);\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				



			elif instruction[0] == "CPD":
				funcion += "\tint carry = GETFLAG(FLAG_C);\n"
				funcion += "\tBYTE value = CP_HL();\n"
				funcion += "\tif(GETFLAG(FLAG_H)) {\n"
				funcion += "\t\tvalue--;\n"
				funcion += "\t}\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tif (carry) {\n"
				funcion += "\t\tSETFLAG(FLAG_C);\n"
				funcion += "\t} else {\n"
				funcion += "\t\tRESETFLAG(FLAG_C);\n"
				funcion += "\t}\n"
				funcion += "\tMODFLAG(FLAG_Y, value & (1 << 1));\n"
				funcion += "\tMODFLAG(FLAG_X, value & (1 << 3));\n"
				funcion += "\treg.w.wz = reg.w.wz - 1;\n"
				



			elif instruction[0] == "CPDR":
				funcion += "\tint carry = GETFLAG(FLAG_C);\n"
				funcion += "\tBYTE value = CP_HL();\n"
				funcion += "\tif(GETFLAG(FLAG_H)) {\n"
				funcion += "\t\tvalue--;\n"
				funcion += "\t}\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tif (carry) {\n"
				funcion += "\t\tSETFLAG(FLAG_C);\n"
				funcion += "\t} else {\n"
				funcion += "\t\tRESETFLAG(FLAG_C);\n"
				funcion += "\t}\n"
				funcion += "\tMODFLAG(FLAG_Y, value & (1 << 1));\n"
				funcion += "\tMODFLAG(FLAG_X, value & (1 << 3));\n"
				funcion += "\tif (reg.b.a == READBYTE(reg.w.hl) || reg.w.bc == 1) {\n"
				funcion += "\t\treg.w.wz = reg.w.wz + 1;\n"
				funcion += "\t} else {\n"
				funcion += "\t\treg.w.wz = pc + 1;\n"
				funcion += "\t}\n"
				funcion += "\tif (reg.w.bc != 0 && !GETFLAG(FLAG_Z)) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"



			elif instruction[0] == "CPI":
				funcion += "\tint carry = GETFLAG(FLAG_C);\n"
				funcion += "\tBYTE value = CP_HL();\n"
				funcion += "\tif(GETFLAG(FLAG_H)) {\n"
				funcion += "\t\tvalue--;\n"
				funcion += "\t}\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tMODFLAG(FLAG_C, carry);\n"
				funcion += "\tMODFLAG(FLAG_Y, value & (1 << 1));\n"
				funcion += "\tMODFLAG(FLAG_X, value & (1 << 3));\n"
				funcion += "\treg.w.wz = reg.w.wz + 1;\n"



			elif instruction[0] == "CPIR":
				funcion +="\tint carry = GETFLAG(FLAG_C);\n"
				funcion += "\tBYTE value = CP_HL();\n"
				funcion += "\tif(GETFLAG(FLAG_H)) {\n"
				funcion += "\t\tvalue--;\n"
				funcion += "\t}\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tMODFLAG(FLAG_C, carry);\n"
				funcion += "\tMODFLAG(FLAG_Y, value & (1 << 1));\n"
				funcion += "\tMODFLAG(FLAG_X, value & (1 << 3));\n"
				funcion += "\tif (reg.b.a == READBYTE(reg.w.hl) || reg.w.bc == 1) {\n"
				funcion += "\t\treg.w.wz = reg.w.wz - 1;\n"
				funcion += "\t} else {\n"
				funcion += "\t\treg.w.wz = pc + 1;\n"
				funcion += "\t}\n"
				funcion += "\tif (reg.w.bc != 0 && !GETFLAG(FLAG_Z)) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"






			elif instruction[0] == "CPL":
				funcion += "\treg.b.a = ~reg.b.a;\n"
				funcion += "\tSETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tAdjustUndocFlags(reg.b.a);\n"
				



			elif instruction[0] == "DAA":
				funcion += "DecimalAdjustAccumulator();\n"
				



			elif instruction[0] == "DEC":
				if instruction[1] in word_reg:
					funcion += "\t" + str(genArg(instruction[1]))  + "--;\n"
				elif instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Dec(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tWRITEBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset, Dec(READBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] in addr_w_reg_dest:
					funcion += "\tWRITEBYTE(" + str(genArg(instruction[1]))  + ", Dec(" + str(genArg(instruction[1])) + "));\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Dec(READBYTE(reg.w.hl)));\n"

				



			elif instruction[0] == "DI": # Revisar documentacion
				funcion += "\tiff1 = iff2 = 0;\n"
				funcion += "\tdefer_irq = 1;\n"
				



			elif instruction[0] == "DJNZ":
				funcion += "\tchar offset = READBYTE(pc++);\n"
				funcion += "\treg.b.b--;\n"
				funcion += "\tif (reg.b.b) {\n"
				funcion += "\t\tpc += offset;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"
				funcion += "\treg.w.wz = pc;\n"




			elif instruction[0] == "EI": # Revisar documentacion
				funcion += "\tiff1 = iff2 = 1;\n"
				funcion += "\tdefer_irq = 1;\n"
				



			elif instruction[0] == "EX":
				if instruction[1] in word_reg:
					funcion += "\tWORD tmp = " + str(genArg(instruction[1])) + ";\n"
					funcion += "\t"  + str(genArg(instruction[1])) + " = " + str(genArg(instruction[2])) + ";\n"
					funcion += "\t" + str(genArg(instruction[2])) + " = tmp;\n"
				else:
					funcion += "\tWORD tmp = READWORD(sp);\n"
					funcion += "\tWRITEWORD(sp, "  + str(genArg(instruction[2])) + ");\n"
					funcion += "\t" + str(genArg(instruction[2])) + " = tmp;\n"
					funcion += "\treg.w.wz = " + str(genArg(instruction[2])) + ";\n"

				



			elif instruction[0] == "EXX":
				funcion += "\tWORD tmp = reg.w.bc;\n"
				funcion += "\treg.w.bc = alt_reg.w.bc;\n"
				funcion += "\talt_reg.w.bc = tmp;\n"
				funcion += "\ttmp = reg.w.de;\n"
				funcion += "\treg.w.de = alt_reg.w.de;\n"
				funcion += "\talt_reg.w.de = tmp;\n"
				funcion += "\ttmp = reg.w.hl;\n"
				funcion += "\treg.w.hl = alt_reg.w.hl;\n"
				funcion += "\talt_reg.w.hl = tmp;\n"
				



			elif instruction[0] == "HALT":
				funcion += "\thalted = 1;\n"
				funcion += "\tpc--;\n"
				



			elif instruction[0] == "IM":
				funcion += "\tim = " + str(genArg(instruction[1])) + ";\n"
				



			elif instruction[0] == "IN":
				if instruction[2] == "(n)":
					funcion += "\tWORD port = (reg.b.a << 8) | READBYTE(pc++);\n"
					funcion += "\treg.w.wz = (reg.b.a << 8) + port + 1; // reg A before operation\n"
					funcion += "\treg.b.a = ReadIO(port);\n"
				else:
					funcion += "\t" + str(genArg(instruction[1])) + " = ReadIO(reg.w.bc);\n"
					funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
					funcion += "\tAdjustFlagsSZPV(" + str(genArg(instruction[1])) + ");\n"
					funcion += "\tAdjustUndocFlags(" + str(genArg(instruction[1])) + ");\n"
					funcion += "\treg.w.wz = reg.w.bc + 1;\n"

				



			elif instruction[0] == "INC":
				if instruction[1] in word_reg:
					funcion += "\t" + str(genArg(instruction[1]))  + "++;\n"
				elif instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Inc(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tWRITEBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset, Inc(READBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] in addr_w_reg_dest:
					funcion += "\tWRITEBYTE(" + str(genArg(instruction[1]))  + ", Inc(" + str(genArg(instruction[1])) + "));\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Inc(READBYTE(reg.w.hl)));\n"
				



			elif instruction[0] == "IND":
				funcion += "\tBYTE value = ReadIO(reg.w.bc);\n"
				funcion += "\tWRITEBYTE(reg.w.hl, value);\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.wz = reg.w.bc - 1; // Before Dec() reg B\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\tMODFLAG(FLAG_N, (value & 0x80) != 0);\n"
				funcion += "\tint flagvalue = value + ((reg.b.c - 1) & 0xff);\n"
				funcion += "\tMODFLAG(FLAG_H, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);\n"
				



			elif instruction[0] == "INDR":
				funcion += "\tBYTE value = ReadIO(reg.w.bc);\n"
				funcion += "\tWRITEBYTE(reg.w.hl, value);\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.wz = reg.w.bc - 1; // Before Dec() reg B\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\tMODFLAG(FLAG_N, (value & 0x80) != 0);\n"
				funcion += "\tint flagvalue = value + ((reg.b.c - 1) & 0xff);\n"
				funcion += "\tMODFLAG(FLAG_H, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);\n"
				funcion += "\tif (reg.b.b != 0) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"



			elif instruction[0] == "INI":
				funcion += "\tBYTE value = ReadIO(reg.w.bc);\n"
				funcion += "\tWRITEBYTE(reg.w.hl, value);\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.wz = reg.w.bc + 1; // Before Dec() reg B\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\tMODFLAG(FLAG_N, (value & 0x80) != 0);\n"
				funcion += "\tint flagvalue = value + ((reg.b.c + 1) & 0xff);\n"
				funcion += "\tMODFLAG(FLAG_H, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);\n"
				



			elif instruction[0] == "INIR":
				funcion += "\tBYTE value = ReadIO(reg.w.bc);\n"
				funcion += "\tWRITEBYTE(reg.w.hl, value);\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.wz = reg.w.bc + 1; // Before Dec() reg B\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\tMODFLAG(FLAG_N, (value & 0x80) != 0);\n"
				funcion += "\tint flagvalue = value + ((reg.b.c + 1) & 0xff);\n"
				funcion += "\tMODFLAG(FLAG_H, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flagvalue > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flagvalue & 7) ^ reg.b.b]);\n"
				funcion += "\tif (reg.b.b != 0 && !GETFLAG(FLAG_Z)) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"




			elif instruction[0] == "JP":
				if instruction[1] in word_reg:
					funcion += "\tpc = " + str(genArg(instruction[1])) + ";\n"
				if instruction[1] in addr_w_reg:
					funcion += "\tpc = " + str(genArg("d" + instruction[1])) + ";\n"
					funcion += "\treg.w.wz = pc;\n"
				elif instruction[1] in alt_cond_reg:
					funcion += "\tWORD addr = READWORD(pc);\n"
					funcion += "\tpc += 2;\n"
					funcion += "\tif (Condition(" +  str(genArg("C_" + instruction[1])) + ")) {\n"
					funcion += "\t\tpc = addr;\n"
					funcion += "\t}\n"
					funcion += "\treg.w.wz = pc;\n"

				else:  # nn
					funcion += "\tpc = READWORD(pc);\n"
					funcion += "\treg.w.wz = pc;\n"

				



			elif instruction[0] == "JR":
				funcion += "\tint offset = Complement(READBYTE(pc++));\n"
				if instruction[1] in alt_cond_reg:
					funcion += "\tif (Condition(" + str(genArg("C_" + instruction[1])) + ")) {\n"
					funcion += "\t\tpc += offset;\n"
					funcion += "\t\t#ifndef __Z80ACCURATE__\n"
					funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
					funcion += "\t\t#endif\n"
					funcion += "\t}\n"

				else:
					funcion += "\tpc += offset;\n"
				funcion += "\treg.w.wz = pc;\n"
					



			elif instruction[0] == "LD":

				if instruction[1] in byte_reg:
						if instruction[2] == "(nn)":
							funcion += "\tWORD addr = READWORD(pc);\n"
							funcion += "\t" + str(genArg(instruction[1])) + " = READBYTE(addr);\n"
							funcion += "\tpc += 2;\n"
							funcion += "\treg.w.wz = addr + 1;\n"
						elif instruction[2] in spec_reg:
							funcion += "\t" + str(genArg(instruction[1])) + " = " + str(genArg(instruction[2])) +  ";\n"
							funcion += "\tAdjustUndocFlags(" + str(genArg(instruction[1])) + ");\n"
							funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
							funcion += "\tMODFLAG(FLAG_PV, iff2);\n"
							funcion += "\tMODFLAG(FLAG_S, (" + str(genArg(instruction[1])) + " & 0x80) != 0);\n"
							funcion += "\tMODFLAG(FLAG_Z, (" + str(genArg(instruction[1])) + " == 0));\n"
						elif instruction[2] in disp_reg:
							funcion += "\tchar offset = READBYTE(pc++);\n"
							funcion += "\t" + str(genArg(instruction[1])) + " = READBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset);\n"
							funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
						else:
							funcion += "\t" + str(genArg(instruction[1])) + " = " + str(genArg(instruction[2])) + ";\n"
							if instruction[2] == "(BC)" or instruction[2] == "(DE)":
								funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)]", "", instruction[2].lower())) + " + 1;\n"

				elif instruction[1] in word_reg:
					if instruction[2] == "(nn)":
						funcion += "\tWORD addr = READWORD(pc);\n"
						funcion += "\tpc += 2;\n"
						funcion += "\t" + str(genArg(instruction[1])) + " = READWORD(addr);\n"
						if instruction[1] == "BC" or instruction[1] == "DE":
							funcion += "\treg.w.wz = addr + 1;\n"

					else:
						funcion += "\t" + str(genArg(instruction[1])) + " = " + str(genArg(instruction[2])) +  ";\n"
						if instruction[2] == "nn":
							funcion += "\tpc += 2;\n"

				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					if instruction[2] in byte_reg:
						funcion += "\tWRITEBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tBYTE n = READBYTE(pc++);\n"
						funcion += "\tWRITEBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset, n);\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif "d" + instruction[1] in addr_w_reg_dest:
					funcion += "\tWRITEBYTE(" + str(genArg("d" + instruction[1])) + ", " + str(genArg(instruction[2])) +  ");\n"
					if instruction[1] == "(BC)" or instruction[1] == "(DE)":
						funcion += "\treg.b.w = reg.b.a;\n"
						funcion += "\treg.b.z = (reg.w." + str(re.sub("[\(\)]", "", instruction[1].lower())) + " + 1) & 0xff;\n"


				elif instruction[1] in spec_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = " + str(genArg(instruction[2])) +  ";\n"
				elif instruction[1] == "(nn)":
					funcion += "\tWORD addr = READWORD(pc);\n"
					if instruction[2] in byte_reg:
						funcion += "\tWRITEBYTE(addr, " + str(genArg(instruction[2])) +  ");\n"
						funcion += "\treg.b.w = reg.b.a;\n"
						funcion += "\treg.b.z = (addr + 1) & 0xff;\n"
					elif instruction[2] in word_reg:
						funcion += "\tWRITEWORD(addr, " + str(genArg(instruction[2])) +  ");\n"
						funcion += "\treg.w.wz = addr + 1;\n"
					funcion += "\tpc += 2;\n"


				



			elif instruction[0] == "LDD":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\tWRITEBYTE(reg.w.de, value);\n"
				funcion += "\treg.w.de--;\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);\n"
				funcion += "\tMODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				



			elif instruction[0] == "LDDR":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\tWRITEBYTE(reg.w.de, value);\n"
				funcion += "\treg.w.de--;\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);\n"
				funcion += "\tMODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tif (reg.w.bc != 1) {\n"
				funcion += "\t\treg.w.wz = pc + 1;\n"
				funcion += "\t}\n"
				funcion += "\tif (reg.w.bc != 0) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"



			elif instruction[0] == "LDI":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\tWRITEBYTE(reg.w.de, value);\n"
				funcion += "\treg.w.de++;\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);\n"
				funcion += "\tMODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				



			elif instruction[0] == "LDIR":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\tWRITEBYTE(reg.w.de, value);\n"
				funcion += "\treg.w.de++;\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\treg.w.bc--;\n"
				funcion += "\tMODFLAG(FLAG_Y, (reg.b.a + value) & 0x02);\n"
				funcion += "\tMODFLAG(FLAG_X, ((reg.b.a + value) & FLAG_X) != 0);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tMODFLAG(FLAG_PV, reg.w.bc != 0);\n"
				funcion += "\tif (reg.w.bc != 1) {\n"
				funcion += "\t\treg.w.wz = pc + 1;\n"
				funcion += "\t}\n"
				funcion += "\tif (reg.w.bc != 0) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"




			elif instruction[0] == "NEG":
				funcion += "\tBYTE tmp = reg.b.a;\n"
				funcion += "\treg.b.a = 0;\n"
				funcion += "\tSub(tmp);\n"
				funcion += "\tSETFLAG(FLAG_N);\n"
				



			elif instruction[0] == "NOP":
				pass



			elif instruction[0] == "OR":
				if instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tOr(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				else:
					funcion += "\tOr(" + str(genArg(instruction[1])) + ");\n"

				



			elif instruction[0] == "OTDR":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\treg.w.wz = reg.w.bc - 1; // After Dec reg B\n"
				funcion += "\tWriteIO(reg.w.bc, value);\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\tint flag_value = value + reg.b.l;\n"
				funcion += "\tMODFLAG(FLAG_N, value & 0x80);\n"
				funcion += "\tMODFLAG(FLAG_H, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);\n"
				funcion += "\tAdjustUndocFlags(reg.b.b);\n"
				funcion += "\tif (reg.b.b != 0) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"



			elif instruction[0] == "OTIR":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\treg.w.wz = reg.w.bc + 1; // After Dec reg B\n"
				funcion += "\tWriteIO(reg.w.bc, value);\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\tint flag_value = value + reg.b.l;\n"
				funcion += "\tMODFLAG(FLAG_N, value & 0x80);\n"
				funcion += "\tMODFLAG(FLAG_H, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);\n"
				funcion += "\tAdjustUndocFlags(reg.b.b);\n"
				funcion += "\tif (reg.b.b != 0) {\n"
				funcion += "\t\tpc -= 2;\n"
				funcion += "\t\t#ifndef __Z80ACCURATE__\n"
				funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
				funcion += "\t\t#endif\n"
				funcion += "\t}\n"



			elif instruction[0] == "OUT":
				if instruction[1] == "(n)":
					funcion += "\tWORD port = (reg.b.a << 8) | READBYTE(pc++);\n"
					funcion += "\tWriteIO(port, reg.b.a);\n"
					funcion += "\treg.b.w = reg.b.a;\n"
					funcion += "\treg.b.z = (port + 1) & 0xff;\n"
				else:
					if instruction[2] in byte_reg:
						funcion += "\tWriteIO(reg.w.bc, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWriteIO(reg.w.bc, 0);\n"
					funcion += "\treg.w.wz = reg.w.bc + 1;\n"

				



			elif instruction[0] == "OUTD":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\treg.w.wz = reg.w.bc - 1; // After Dec reg B\n"
				funcion += "\tWriteIO(reg.w.bc, value);\n"
				funcion += "\treg.w.hl--;\n"
				funcion += "\tint flag_value = value + reg.b.l;\n"
				funcion += "\tMODFLAG(FLAG_N, value & 0x80);\n"
				funcion += "\tMODFLAG(FLAG_H, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);\n"
				funcion += "\tAdjustUndocFlags(reg.b.b);\n"
				



			elif instruction[0] == "OUTI":
				funcion += "\tBYTE value = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.b = Dec(reg.b.b);\n"
				funcion += "\treg.w.wz = reg.w.bc + 1; // After Dec reg B\n"
				funcion += "\tWriteIO(reg.w.bc, value);\n"
				funcion += "\treg.w.hl++;\n"
				funcion += "\tint flag_value = value + reg.b.l;\n"
				funcion += "\tMODFLAG(FLAG_N, value & 0x80);\n"
				funcion += "\tMODFLAG(FLAG_H, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_C, flag_value > 0xff);\n"
				funcion += "\tMODFLAG(FLAG_PV, parity[(flag_value & 7) ^ reg.b.b]);\n"
				funcion += "\tAdjustUndocFlags(reg.b.b);\n"
				



			elif instruction[0] == "POP":
				funcion += "\t" + str(genArg(instruction[1])) + " = Pop();\n"
				



			elif instruction[0] == "PUSH":
				funcion += "\tPush(" + str(genArg(instruction[1])) + ");\n"
				



			elif instruction[0] == "RES":
				if instruction[2] in byte_reg:
					funcion += "\t" + str(genArg(instruction[2])) + " = RESETBIT(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"
				elif instruction[2] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 4:
						funcion += "\t" + str(genArg(instruction[3])) + " = RESETBIT(" + str(instruction[1]) + ", READBYTE(" + str(genArg(instruction[2])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[2])) + " + offset, " + str(genArg(instruction[3])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[2])) + " + offset, RESETBIT(" + str(instruction[1]) + ", READBYTE(" + str(genArg(instruction[2])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
				elif instruction[2] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, RESETBIT(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + "));\n"
				



			elif instruction[0] == "RET":
				if len(instruction) == 1:
					funcion += "\tpc = Pop();\n"
					
				elif instruction[1] in alt_cond_reg:
					funcion += "\tif (Condition(" + str(genArg("C_" + instruction[1])) + ")) {\n"
					funcion += "\t\tpc = Pop();\n"
					funcion += "\t\t#ifndef __Z80ACCURATE__\n"
					funcion += "\t\ttstates += " + str(data[tabla][i]["tstates"] - data[tabla][i]["tstates-nojmp"]) + ";\n"
					funcion += "\t\t#endif\n"
					funcion += "\t}\n"
				funcion += "\treg.w.wz = pc;\n"


			elif instruction[0] == "RETI":
				funcion += "\tiff1 = iff2;\n"
				funcion += "\tpc = Pop();\n"
				funcion += "\treg.w.wz = pc;\n"
				


			elif instruction[0] == "RETN":
				funcion += "\tiff1 = iff2;"
				funcion += "\tpc = Pop();\n"
				



			elif instruction[0] == "RL":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = RL(1," + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = RL(1, READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, RL(1, READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, RL(1, READBYTE(reg.w.hl)));\n"

				



			elif instruction[0] == "RLA":
				funcion += "\treg.b.a = RL(0, reg.b.a);\n"
				



			elif instruction[0] == "RLC":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = RLC(1," + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = RLC(1, READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, RLC(1, READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, RLC(1, READBYTE(reg.w.hl)));\n"

				



			elif instruction[0] == "RLCA":
				funcion += "\treg.b.a = RLC(0, reg.b.a);\n"
				



			elif instruction[0] == "RLD":
				funcion += "\tBYTE Ah = reg.b.a & 0x0f;\n"
				funcion += "\tBYTE hl = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.a = (reg.b.a & 0xf0) | ((hl & 0xf0) >> 4);\n"
				funcion += "\thl = (hl << 4) | Ah;\n"
				funcion += "\tWRITEBYTE(reg.w.hl, hl);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tAdjustFlagsSZPV(reg.b.a);\n"
				funcion += "\tAdjustUndocFlags(reg.b.a);\n"
				funcion += "\treg.w.wz = reg.w.hl + 1;\n"

				



			elif instruction[0] == "RR":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = RR(1," + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = RR(1, READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, RR(1, READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, RR(1, READBYTE(reg.w.hl)));\n"
				



			elif instruction[0] == "RRA":
				funcion += "\treg.b.a = RR(0, reg.b.a);\n"
				



			elif instruction[0] == "RRC":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = RRC(1," + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = RRC(1, READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, RRC(1, READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, RRC(1, READBYTE(reg.w.hl)));\n"

				



			elif instruction[0] == "RRCA":
				funcion += "\treg.b.a = RRC(0, reg.b.a);\n"
				



			elif instruction[0] == "RRD":
				funcion += "\tBYTE Ah = reg.b.a & 0x0f;\n"
				funcion += "\tBYTE hl = READBYTE(reg.w.hl);\n"
				funcion += "\treg.b.a = (reg.b.a & 0xf0) | (hl & 0x0f);\n"
				funcion += "\thl = (hl >> 4) | (Ah << 4);\n"
				funcion += "\tWRITEBYTE(reg.w.hl, hl);\n"
				funcion += "\tRESETFLAG(FLAG_H | FLAG_N);\n"
				funcion += "\tAdjustFlagsSZPV(reg.b.a);\n"
				funcion += "\tAdjustUndocFlags(reg.b.a);\n"
				funcion += "\treg.w.wz = reg.w.hl + 1;\n"

				



			elif instruction[0] == "RST":
				funcion += "\tPush(pc);\n"
				funcion += "\tpc = 0x0" + str(genArg(instruction[1])) + ";\n"
				funcion += "\treg.w.wz = pc;\n"
				



			elif instruction[0] == "SBC":

				if instruction[1] in word_reg:
					funcion += "\treg.w.wz = " + str(genArg(instruction[1])) + " + 1; // first register before operation\n"
					funcion += "\tSUBWORDWITHCARRY(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"
				elif instruction[1] in byte_reg:
					if instruction[2] in disp_reg:
						funcion += "\tchar offset = READBYTE(pc);\n"
						funcion += "\tpc++;\n"
						funcion += "\tSUBWITHCARRY(READBYTE(reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset));\n"
						funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
					else:
						if instruction[2] == "n":
							funcion += "\tSUBWITHCARRY(READBYTE(pc));\n"
							funcion += "\tpc++;\n"
						else:
							funcion += "\tSUBWITHCARRY(" + str(genArg(instruction[2])) + ");\n"
				else:
					funcion += "\tSUBWITHCARRY(" + str(genArg(instruction[2])) + ");\n"

				



			elif instruction[0] == "SCF":
				funcion += "\tSETFLAG(FLAG_C);\n"
				funcion += "\tRESETFLAG(FLAG_N | FLAG_H);\n"
				funcion += "\tAdjustUndocFlags(reg.b.a);\n"
				



			elif instruction[0] == "SET":
				if instruction[2] in byte_reg:
					funcion += "\t" + str(genArg(instruction[2])) + " = SETBIT(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + ");\n"
				elif instruction[2] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 4:
						funcion += "\t" + str(genArg(instruction[3])) + " = SETBIT(" + str(instruction[1]) + ", READBYTE(" + str(genArg(instruction[2])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[2])) + " + offset, " + str(genArg(instruction[3])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[2])) + " + offset, SETBIT(" + str(instruction[1]) + ", READBYTE(" + str(genArg(instruction[2])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[2].lower())) + " + offset;\n"
				elif instruction[2] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, SETBIT(" + str(genArg(instruction[1])) + ", " + str(genArg(instruction[2])) + "));\n"
				



			elif instruction[0] == "SLA":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Sla(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = Sla(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, Sla(READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Sla(" + str(genArg(instruction[1])) + "));\n"
				



			elif instruction[0] == "SLL":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Sll(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = Sll(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, Sll(READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Sll(" + str(genArg(instruction[1])) + "));\n"
				



			elif instruction[0] == "SRA":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Sra(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = Sra(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, Sra(READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Sra(" + str(genArg(instruction[1])) + "));\n"
				



			elif instruction[0] == "SRL":
				if instruction[1] in byte_reg:
					funcion += "\t" + str(genArg(instruction[1])) + " = Srl(" + str(genArg(instruction[1])) + ");\n"
				elif instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++ - 1);\n"
					if len(instruction) == 3:
						funcion += "\t" + str(genArg(instruction[2])) + " = Srl(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, " + str(genArg(instruction[2])) + ");\n"
					else:
						funcion += "\tWRITEBYTE(" + str(genArg(instruction[1])) + " + offset, Srl(READBYTE(" + str(genArg(instruction[1])) + " + offset)));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				elif instruction[1] == "(HL)":
					funcion += "\tWRITEBYTE(reg.w.hl, Srl(" + str(genArg(instruction[1])) + "));\n"
				



			elif instruction[0] == "SUB":
				if instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tSUB(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				else:
					if instruction[1] == "n":
						funcion += "\tSUB(READBYTE(pc));\n"
						funcion += "\tpc++;\n"
					else:
						funcion += "\tSUB(" + str(genArg(instruction[1])) + ");\n"
				



			elif instruction[0] == "XOR":
				if instruction[1] in disp_reg:
					funcion += "\tchar offset = READBYTE(pc++);\n"
					funcion += "\tXor(READBYTE(" + str(genArg(instruction[1])) + " + offset));\n"
					funcion += "\treg.w.wz = reg.w." + str(re.sub("[\(\)\+d]", "", instruction[1].lower())) + " + offset;\n"
				else:
					funcion += "\tXor(" + str(genArg(instruction[1])) + ");\n"

				



			elif instruction[0] == "---": # REVISAR
				pass


			if instruction[0] != "---":

				funcion += "\n\t#ifdef __Z80ACCURATE__\n"
				funcion += "\t} else {\n"

				if (instruction[0] == "JR" or instruction[0] == "CALL" or instruction[0] == "RET") and (len(instruction) > 1 and instruction[1] in alt_cond_reg):
					funcion += "\t\tif (Condition(" + str(genArg("C_" + instruction[1])) + ")) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"

				elif instruction[0] == "DJNZ":
					funcion += "\t\tif (reg.b.b - 1) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"


				elif instruction[0] == "LDDR" or instruction[0] == "LDIR":
					funcion += "\t\tif ((reg.w.bc - 1) != 0) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"

				elif instruction[0] == "CPDR" or instruction[0] == "CPIR":
					funcion += "\t\tif ((reg.w.bc - 1) != 0 && (reg.b.a - READBYTE(reg.w.hl)) != 0) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"

				elif instruction[0] == "INDR" or instruction[0] == "INIR":
					funcion += "\t\tif ((reg.b.b - 1) != 0 && (reg.b.a - READBYTE(reg.w.hl)) != 0) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"

				elif instruction[0] == "OTDR" or instruction[0] == "OTIR":
					funcion += "\t\tif ((reg.b.b - 1) != 0) {\n"
					funcion += "\t\t\twill_jump = 1;\n"
					funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"
					funcion += "\t\t} else {\n"
					if 'tstates-nojmp' in data[tabla][i]:

						funcion += "\t\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates-nojmp"]) / 4))) + ";\n"
					funcion += "\t\t}\n"

				# All the rest
				else:
					funcion += "\t\tmcycles_counter = mcycles_current = " + str( int(round(float(data[tabla][i]["tstates"]) / 4))) + ";\n"


				


				if instruction[0] == "INIR" or instruction[0] == "INDR" or instruction[0] == "OTIR" or instruction[0] == "OTDR" or instruction[0] == "CPIR" or instruction[0] == "CPDR" or instruction[0] == "LDIR" or instruction[0] == "LDDR" or instruction[0] == "DJNZ" or ((instruction[0] == "JR" or instruction[0] == "CALL" or instruction[0] == "RET") and (len(instruction) > 1 and instruction[1] in alt_cond_reg)):
					funcion += "\t\tif (will_jump) {\n"
					funcion += "\t\t\twill_jump = 0;\n"
					if 'tstates-nojmp' in data[tabla][i]:
						funcion += "\t\t\tlast_mcycle_tstates = " + str( data[tabla][i]["tstates"] - ((int(round(float(data[tabla][i]["tstates"]) / 4)) - 1) * 4 )) + ";\n"
						funcion += "\t\t} else {\n"
						funcion += "\t\tlast_mcycle_tstates = " + str( data[tabla][i]["tstates-nojmp"] - ((int(round(float(data[tabla][i]["tstates-nojmp"]) / 4)) - 1) * 4 )) + ";\n"

					funcion += "\t\t}\n"

				else:
					funcion += "\t\tlast_mcycle_tstates = " + str( data[tabla][i]["tstates"] - (( int(round(float(data[tabla][i]["tstates"]) / 4)) - 1) * 4 )) + ";\n"

				funcion += "\t}\n"

				funcion += "\t#else\n"
				if instruction[0] == "INIR" or instruction[0] == "INDR" or instruction[0] == "OTIR" or instruction[0] == "OTDR" or instruction[0] == "CPIR" or instruction[0] == "CPDR" or instruction[0] == "LDIR" or instruction[0] == "LDDR" or instruction[0] == "DJNZ" or ((instruction[0] == "JR" or instruction[0] == "CALL" or instruction[0] == "RET") and (len(instruction) > 1 and instruction[1] in alt_cond_reg)):
					funcion += "\ttstates += " + str(data[tabla][i]["tstates-nojmp"]) + ";\n"
				else:
					funcion += "\ttstates += " + str(data[tabla][i]["tstates"]) + ";\n"
				funcion += "\t#endif\n"

				funcion += "}\n"

				sorted_declarations.append(declaration)
				sorted_functions.append(funcion)




print '#include "z80.h"\n\n'
looparray('MAIN_INSTRUCTIONS')
looparray('CB_INSTRUCTIONS')
looparray('ED_INSTRUCTIONS')
looparray('DD_INSTRUCTIONS')
looparray('FD_INSTRUCTIONS')
looparray('DDCB_INSTRUCTIONS')
looparray('FDCB_INSTRUCTIONS')

'''
for item in sorted(arrays):
	print item
	'''
'''
for item in sorted(sorted_declarations):
	if "---" in item:
		pass
	else:
		print item
'''

for item in sorted(sorted_functions):
	if "---" in item:
		pass
	else:
		print item



# Prefixes after prefixes undoccumented