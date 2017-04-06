# -*- coding: utf-8 -*-


'''
TODO: Fix tstates on ED table mirrored instructions

'''


import json
import re
from collections import OrderedDict

spec_reg = [ "I", "R" ]
byte_reg = [ "A", "B", "C", "D", "E", "F", "H", "L", "IXH", "IXL", "IYH", "IYL", "I", "R" , "A'", "F'", "B'", "C'", "D'", "E'" ]
word_reg = [ "DE", "HL", "AF", "AF'", "BC", "IY", "IX", "SP" ]
alt_reg = [ "AF'", "BC'", "DE'", "HL'"]
indirect = [ "(HL)", "(SP)", "(BC)", "(DE)", "(IY)", "(IX)" ]
offset = [ "(IX+d)", "(IY+d)" ]
condition = [ "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" ]



'''
//   0     1     2     3     4     5     6     7     8     9     10    11    12    13   14    15    16    17
// ------------------------------------------------------------------------------------------------------------
//   AF |  BC |  DE |  HL |  IX |  IY |  SP |  IR |  WZ |
//   A  |  F  |  B  |  C  |  D  |  E  |  H  |  L  | IXH | IXL | IYH | IHL |  S  |  P  |  I  |  R  |  W  |  Z  |
//   00 |  08 |  10 |  18 |  20 |  28 |  30 |  38 |
//   Z  |  NZ |  CC |  NC |  M  |  P  |  PE |  PO |
//  (AF)| (BC)| (DE)| (HL)| (IX)| (IY)| (SP)| (IR)| (WZ)|
'''

ops = [ ("AF",0), ("BC",1), ("DE",2), ("HL",3), ("IX",4), ("IY",5), ("SP",6), ("IR", 7), ("WZ",8),
		("AF'",0), ("BC'",1), ("DE'",2), ("HL'",3), ("IX'",4), ("IY'",5), ("SP'",6), ("IR'", 7), ("WZ'",8),
		("(AF)",0), ("(BC)",1), ("(DE)",2), ("(HL)",3), ("(IX)",4), ("(IY)",5), ("(SP)",6), ("(IR)", 7), ("(WZ)",8),
		("A",0),  ("F",1),  ("B",2),  ("C",3),  ("D",4),  ("E",5),  ("H",6),  ("L",7),   ("IXH",8), ("IXL",9), ("IYH",10), ("IYL",11), ("S",12), ("P",13), ("I",14), ("R",15), ("W",16), ("Z",17),
		("00",0x00), ("08",0x08), ("10",0x10), ("18",0x18), ("20",0x20), ("28",0x28), ("30",0x30), ("38", 0x38),
		("Z",0),  ("NZ",1), ("CC",2), ("NC",3), ("M",4),  ("P",5),  ("PE",6), ("PO", 7),
		("0",0),  ("1",1),  ("2",2),  ("3",3),  ("4",4),  ("5",5),  ("6",6),  ("7", 7),
		("(IX+d)", 4), ("(IY+d)",5),
		("nn",0), ("n",0),  ("(nn)",0), ("(n)",0), ("d",0), ("(C)",0)
]


ops = dict(ops)


already = []

pointer_tables = []
operand_tables = []
endian_tables = []


sorted_declarations = []
sorted_functions = []

switch_tables = []



with open('instructions.json') as data_file:
		data = json.load(data_file, object_pairs_hook=OrderedDict)




def looparray(t):

	endianess_fix = []

	switchs = "void Z80::" + re.sub("_instructions",u"_switch",str(t).lower()) + "() {\n"
	switchs += "\tswitch (op) {\n"

	pointers = "\tconst OPCODES " + str(t).lower() + "[256] = {\n"
	#operands = "\tOPERANDS " + re.sub("instructions", ur"operands", str(t).lower()) + "[0xff + 1] {"

	operands = "{ "
	for i in range(0, 256):
		if i % 8 == 0:
			pointers += "\n\t\t"
			operands += "\n\t\t"

		inst = re.split(" |,|->", data[t][i]["mnemonic"])

		pointers += "&Z80::"


		if inst[0] == "ADC":
			if inst[1] in byte_reg:
				if inst[2] in offset:
					fnc_name = "ADC_R_off"

				else:
					if inst[2] == "n":
						fnc_name = "ADC_R_n"

					elif inst[2] == "(HL)":
						fnc_name = "ADC_R_HL"

					else:
						fnc_name = "ADC_R_R"

			elif inst[1] in word_reg:
				fnc_name = "ADC_RR_RR"




		elif inst[0] == "ADD":
			if inst[1] in byte_reg:
				if inst[2] in offset:
					fnc_name = "ADD_R_off"

				else:
					if inst[2] == "n":
						fnc_name = "ADD_R_n"

					elif inst[2] == "(HL)":
						fnc_name = "ADD_R_HL"

					else:
						fnc_name = "ADD_R_R"

			elif inst[1] in word_reg:
				fnc_name = "ADD_RR_RR"



		elif inst[0] == "AND":
			if inst[1] in offset:
				fnc_name = "AND_off"
			elif inst[1] == "(HL)":
				fnc_name = "AND_HL"
			elif inst[1] == "n":
				fnc_name = "AND_n"
			else:
				fnc_name = "AND_R"



		elif inst[0] == "BIT":
			if inst[2] in byte_reg:
				fnc_name = "BIT_n_R"
			elif inst[2] in offset:
				fnc_name = "BIT_n_off"
			elif inst[2] == "(HL)":
				fnc_name = "BIT_n_HL"



		elif inst[0] == "CALL":
			if inst[1] in condition:
				fnc_name = "CALL_cond"
			else:
				fnc_name = "CALL"



		elif inst[0] == "CCF":
			fnc_name = "CCF"



		elif inst[0] == "CP":
			if inst[1] in byte_reg:
				fnc_name = "CP_R"

			elif inst[1] in offset:
				fnc_name = "CP_off"

			elif inst[1] == "(HL)":
				fnc_name = "CP_HL"

			elif inst[1] == "n":
				fnc_name = "CP_n"



		elif inst[0] == "CPD":
			fnc_name = "CPD"



		elif inst[0] == "CPDR":
			fnc_name = "CPDR"



		elif inst[0] == "CPI":
			fnc_name = "CPI"



		elif inst[0] == "CPIR":
			fnc_name = "CPIR"



		elif inst[0] == "CPL":
			fnc_name = "CPL"



		elif inst[0] == "DAA":
			fnc_name = "DAA"



		elif inst[0] == "DEC":
			if inst[1] in byte_reg:
				fnc_name = "DEC_R"

			elif inst[1] in word_reg:
				fnc_name = "DEC_RR"

			elif inst[1] in indirect:
				fnc_name = "DEC_ind"

			elif inst[1] in offset:
				fnc_name = "DEC_off"

			


		elif inst[0] == "DI":
			fnc_name = "DI"



		elif inst[0] == "DJNZ":
			fnc_name = "DJNZ"



		elif inst[0] == "EI":
			fnc_name = "EI"



		elif inst[0] == "EX":
			if inst[1] in word_reg:
				if inst[2] in alt_reg:
					fnc_name = "EX_RR_altRR"
				else:
					fnc_name = "EX_RR_RR"
			else:
				fnc_name = "EX_SP_RR"



		elif inst[0] == "EXX":
			fnc_name = "EXX"



		elif inst[0] == "HALT":
			fnc_name = "HALT"



		elif inst[0] == "IM":
			fnc_name = "IM"



		elif inst[0] == "IN":
			if inst[2] == "(n)":
				fnc_name = "IN_R_n"
			else:
				fnc_name = "IN_R_c"


		elif inst[0] == "INC":
			if inst[1] in byte_reg:
				fnc_name = "INC_R"

			elif inst[1] in word_reg:
				fnc_name = "INC_RR"

			elif inst[1] in indirect:
				fnc_name = "INC_ind"

			elif inst[1] in offset:
				fnc_name = "INC_off"




		elif inst[0] == "IND":
			fnc_name = "IND"



		elif inst[0] == "INDR":
			fnc_name = "INDR"



		elif inst[0] == "INI":
			fnc_name = "INI"



		elif inst[0] == "INIR":
			fnc_name = "INIR"



		elif inst[0] == "JP":
			if inst[1] in word_reg:
				fnc_name = "JP_RR"

			elif inst[1] in indirect:
				fnc_name = "JP_ind"

			elif inst[1] in condition:
				fnc_name = "JP_cond"

			elif inst[1] == "nn":
				fnc_name = "JP_nn"



		elif inst[0] == "JR":
			if inst[1] in condition:
				fnc_name = "JR_cond_d"

			else:
				fnc_name = "JR_d"



		elif inst[0] == "LD":

			if inst[2] in spec_reg:
				fnc_name = "LD_R_spec"

			elif inst[1] in byte_reg:
				if inst[2] in byte_reg:
					fnc_name = "LD_R_R"
				elif inst[2] in indirect:
					fnc_name = "LD_R_ind"
				elif inst[2] in offset:
					fnc_name = "LD_R_off"
				elif inst[2] == "n":
					fnc_name = "LD_R_n"
				elif inst[2] == "(nn)":
					fnc_name = "LD_R_addr"
				elif inst[2] == "(HL)":
					fnc_name = "LD_R_HL"

			elif inst[1] in word_reg:
				if inst[2] in word_reg:
					fnc_name = "LD_RR_RR"
				elif inst[2] == "(nn)":
					fnc_name = "LD_RR_addr"
				elif inst[2] == "nn":
					fnc_name = "LD_RR_nn"

			elif inst[1] in offset:
				if inst[2] in byte_reg:
					fnc_name = "LD_off_R"
				elif inst[2] == "n":
					fnc_name = "LD_off_n"

			elif inst[1] in indirect:
				if inst[2] in byte_reg:
					fnc_name = "LD_ind_R"
				elif inst[2] == "n":
					fnc_name = "LD_ind_n"

			elif inst[1] == "(nn)":
				if inst[2] in byte_reg:
					fnc_name = "LD_addr_R"
				elif inst[2] in word_reg: 
					fnc_name = "LD_addr_RR"

			#elif inst[1] == "(HL)":
			#	fnc_name = "LD_HL_R"




		elif inst[0] == "LDD":
			fnc_name = "LDD"



		elif inst[0] == "LDDR":
			fnc_name = "LDDR"



		elif inst[0] == "LDI":
			fnc_name = "LDI"



		elif inst[0] == "LDIR":
			fnc_name = "LDIR"



		elif inst[0] == "NEG":
			fnc_name = "NEG"



		elif inst[0] == "NOP":
			fnc_name = "NOP"



		elif inst[0] == "OR":
			if inst[1] in offset:
				fnc_name = "OR_off"
			elif inst[1] == "(HL)":
				fnc_name = "OR_HL"
			elif inst[1] == "n":
				fnc_name = "OR_n"
			else:
				fnc_name = "OR_R"



		elif inst[0] == "OTDR":
			fnc_name = "OTDR"



		elif inst[0] == "OTIR":
			fnc_name = "OTIR"



		elif inst[0] == "OUT":
			if inst[1] == "(n)":
				fnc_name = "OUT_n_R"
			else:
				if inst[2] in byte_reg:
					fnc_name = "OUT_c_R"
				else:
					fnc_name = "OUT_c_0"



		elif inst[0] == "OUTD":
			fnc_name = "OUTD"



		elif inst[0] == "OUTI":
			fnc_name = "OUTI"



		elif inst[0] == "POP":
			fnc_name = "POP"



		elif inst[0] == "PUSH":
			fnc_name = "PUSH"



		elif inst[0] == "RES":
			if inst[2] in byte_reg:
				fnc_name = "RES_n_R"

			elif inst[2] in offset:
				if len(inst) == 4:
					fnc_name = "RES_n_off_R"
				else:
					fnc_name = "RES_n_off"

			elif inst[2] == "(HL)":
				fnc_name = "RES_n_HL"



		elif inst[0] == "RET":
			if len(inst) == 1:
				fnc_name = "RET"

			else:
				fnc_name = "RET_cond"



		elif inst[0] == "RETI":
			fnc_name = "RETI"



		elif inst[0] == "RETN":
			fnc_name = "RETN"



		elif inst[0] == "RL":
			if inst[1] in byte_reg:
				fnc_name = "RL_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "RL_off_R"
				else:
					fnc_name = "RL_off"

			elif inst[1] == "(HL)":
				fnc_name = "RL_HL"



		elif inst[0] == "RLA":
			fnc_name = "RLA"



		elif inst[0] == "RLC":
			if inst[1] in byte_reg:
				fnc_name = "RLC_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "RLC_off_R"
				else:
					fnc_name = "RLC_off"

			elif inst[1] == "(HL)":
				fnc_name = "RLC_HL"



		elif inst[0] == "RLCA":
			fnc_name = "RLCA"



		elif inst[0] == "RLD":
			fnc_name = "RLD"



		elif inst[0] == "RR":
			if inst[1] in byte_reg:
				fnc_name = "RR_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "RR_off_R"
				else:
					fnc_name = "RR_off"

			elif inst[1] == "(HL)":
				fnc_name = "RR_HL"



		elif inst[0] == "RRA":
			fnc_name = "RRA"



		elif inst[0] == "RRC":
			if inst[1] in byte_reg:
				fnc_name = "RRC_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "RRC_off_R"
				else:
					fnc_name = "RRC_off"

			elif inst[1] == "(HL)":
				fnc_name = "RRC_HL"



		elif inst[0] == "RRCA":
			fnc_name = "RRCA"



		elif inst[0] == "RRD":
			fnc_name = "RRD"



		elif inst[0] == "RST":
			fnc_name = "RST"



		elif inst[0] == "SBC":
			if inst[1] in byte_reg:
				if inst[2] in offset:
					fnc_name = "SBC_R_off"

				else:
					if inst[2] == "n":
						fnc_name = "SBC_R_n"

					elif inst[2] == "(HL)":
						fnc_name = "SBC_R_HL"

					else:
						fnc_name = "SBC_R_R"

			elif inst[1] in word_reg:
				fnc_name = "SBC_RR_RR"



		elif inst[0] == "SCF":
			fnc_name = "SCF"



		elif inst[0] == "SET":
			if inst[2] in byte_reg:
				fnc_name = "SET_n_R"

			elif inst[2] in offset:
				if len(inst) == 4:
					fnc_name = "SET_n_off_R"
				else:
					fnc_name = "SET_n_off"

			elif inst[2] == "(HL)":
				fnc_name = "SET_n_HL"



		elif inst[0] == "SLA":
			if inst[1] in byte_reg:
				fnc_name = "SLA_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "SLA_off_R"
				else:
					fnc_name = "SLA_off"

			elif inst[1] == "(HL)":
				fnc_name = "SLA_HL"



		elif inst[0] == "SLL":
			if inst[1] in byte_reg:
				fnc_name = "SLL_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "SLL_off_R"
				else:
					fnc_name = "SLL_off"

			elif inst[1] == "(HL)":
				fnc_name = "SLL_HL"



		elif inst[0] == "SRA":
			if inst[1] in byte_reg:
				fnc_name = "SRA_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "SRA_off_R"
				else:
					fnc_name = "SRA_off"

			elif inst[1] == "(HL)":
				fnc_name = "SRA_HL"



		elif inst[0] == "SRL":
			if inst[1] in byte_reg:
				fnc_name = "SRL_R"

			elif inst[1] in offset:
				if len(inst) == 3:
					fnc_name = "SRL_off_R"
				else:
					fnc_name = "SRL_off"

			elif inst[1] == "(HL)":
				fnc_name = "SRL_HL"



		elif inst[0] == "SUB":
			if inst[1] == "n":
				fnc_name = "SUB_n"

			elif inst[1] == "(HL)":
				fnc_name = "SUB_HL"

			elif inst[1] in offset:
				fnc_name = "SUB_off"

			else:
				fnc_name = "SUB_R"



		elif inst[0] == "XOR":
			if inst[1] in offset:
				fnc_name = "XOR_off"
			elif inst[1] == "(HL)":
				fnc_name = "XOR_HL"
			elif inst[1] == "n":
				fnc_name = "XOR_n"
			else:
				fnc_name = "XOR_R"



		elif inst[0] == "---":
			fnc_name = "NOP"




		pointers += fnc_name

		if fnc_name not in already:
			already.append(fnc_name)
			fnc = "\tvoid Z80::" + fnc_name + "() {\n\n\t}"
			declaration = "\tvoid " + fnc_name + "();"
			sorted_declarations.append(declaration)
			sorted_functions.append(fnc)

		operands += "{"

		

		
		tmp = 0
		

		for j in range(1,4):
			
			if len(inst) >= j + 1 and inst[0] != "---":

				operands += str(ops[str(inst[j])]) + ","
				if inst[j] in byte_reg:
					if inst[0] != "DJNZ" and inst[0] != "LDDR" and inst[0] != "LDIR" and inst[0] != "CPDR" and inst[0] != "CPIR" and inst[0] != "INDR" and inst[0] != "INIR" and  inst[0] != "OTDR" or inst[0] != "OTIR" and inst[0] != "JR" and inst[0] != "CALL" and inst[0] != "RET":
						tmp |= 1 << (j - 1)
					
			else:
				operands += "0,"


		endianess_fix.append(tmp)
		


		if inst[0] != "---":

			if inst[0] == "NOP":
				if t == "FD_INSTRUCTIONS" or t == "DD_INSTRUCTIONS": # Extended NOP (4 more tstates)
					operands += str(4 + 4) + ","								# tstates
					operands += str( int(round(float(4 + 4) / 4))) + ","			# mcycles
					operands += str(4 + 4) + ","								# tstates_nojmp
					operands += str( int(round(float(4 + 4) / 4))) + ","			# mcycles_nojmp
				else:
					operands += str(data[t][i]["tstates"]) + ","
					operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
					operands += str(data[t][i]["tstates"]) + ","
					operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","

		#	elif inst[0] == "LD" and ((inst[1] == "HL" and inst[2] == "(nn)") or (inst[1] == "(nn)" and inst[2] == "HL")):
		#		if t == "ED_INSTRUCTIONS": # Mirrored ED opcodes (4 more tstates)
		#			operands += str(int(data[t][i]["tstates"]) + 4) + ","
		#			operands += str( int(round(float(data[t][i]["tstates"] + 4) / 4))) + ","
		#			operands += str(int(data[t][i]["tstates"]) + 4) + ","
		#			operands += str( int(round(float(data[t][i]["tstates"] + 4) / 4))) + ","
		#		else:
		#			operands += str(data[t][i]["tstates"]) + ","
		#			operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
		#			operands += str(data[t][i]["tstates"]) + ","
		#			operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","

			elif inst[0] == "DJNZ" or inst[0] == "LDDR" or inst[0] == "LDIR" or inst[0] == "CPDR" or inst[0] == "CPIR" or inst[0] == "INDR" or inst[0] == "INIR" or inst[0] == "OTDR" or inst[0] == "OTIR" or ((inst[0] == "JR" or inst[0] == "CALL" or inst[0] == "RET") and (len(inst) > 1 and inst[1] in condition)):
				if 'tstates-nojmp' in data[t][i]:
					operands += str(data[t][i]["tstates"]) + ","
					operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
					operands += str(data[t][i]["tstates-nojmp"]) + ","
					operands += str( int(round(float(data[t][i]["tstates-nojmp"]) / 4))) + ","
				else:
					operands += str(data[t][i]["tstates"]) + ","
					operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
					operands += str(data[t][i]["tstates"]) + ","
					operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","

				# All the rest
			else:
				operands += str(data[t][i]["tstates"]) + ","
				operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
				operands += str(data[t][i]["tstates"]) + ","
				operands += str( int(round(float(data[t][i]["tstates"]) / 4))) + ","
				


				
			# last MCycles

	#		operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","

			if inst[0] == "INIR" or inst[0] == "INDR" or inst[0] == "OTIR" or inst[0] == "OTDR" or inst[0] == "CPIR" or inst[0] == "CPDR" or inst[0] == "LDIR" or inst[0] == "LDDR" or inst[0] == "DJNZ" or ((inst[0] == "JR" or inst[0] == "CALL" or inst[0] == "RET") and (len(inst) > 1 and inst[1] in condition)):
				if 'tstates-nojmp' in data[t][i]:
					operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","
					operands += str( data[t][i]["tstates-nojmp"] - ((int(round(float(data[t][i]["tstates-nojmp"]) / 4)) - 1) * 4 )) + ","

				else:
					operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","
					operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","


			else:
				operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","
				operands += str( data[t][i]["tstates"] - ((int(round(float(data[t][i]["tstates"]) / 4)) - 1) * 4 )) + ","


		else:
			if t == "FD_INSTRUCTIONS" or t == "DD_INSTRUCTIONS":
				operands += str(4 + 4) + ","																	# tstates
				operands += str( int(round(float(4) / 4) + 4)) + "," # Extended NOP 							# mcycles
				operands += str(4 + 4) + ","																	# tstates-nojmp
				operands += str( int(round(float(4) / 4) + 4)) + ","											# mcycles_nojmp
				operands += str( data[t][i]["tstates"] - ((int(round(float(4) / 4)) - 1) * 4 ) + 1) + ","		# last_mc_tstates
				operands += str( data[t][i]["tstates"] - ((int(round(float(4) / 4)) - 1) * 4 ) + 1 ) + ","		# last_mc_tstates_nojmp
			else:
				operands += str(4) + ","
				operands += str( int(round(float(4) / 4))) + "," # Extended NOP
				operands += str(4) + ","
				operands += str( int(round(float(4) / 4))) + ","
				operands += str( 4 - ((int(round(float(4) / 4)) - 1) * 4 )) + ","
				operands += str( 4 - ((int(round(float(4) / 4)) - 1) * 4 )) + ","


		# CheckJump & switchs



		if (inst[0] == "JR" or inst[0] == "CALL" or inst[0] == "RET") and (len(inst) > 1 and inst[1] in condition):
			operands += "1"
		

		elif inst[0] == "DJNZ":
			operands += "2"

		elif inst[0] == "LDDR" or inst[0] == "LDIR":
			operands += "3"

		elif inst[0] == "CPDR" or inst[0] == "CPIR":
			operands += "4"

		elif inst[0] == "INDR" or inst[0] == "INIR":
			operands += "5"

		elif inst[0] == "OTDR" or inst[0] == "OTIR":
			operands += "6"

		else:
			operands += "0"			



		operands += "}"

		if inst[0] == "---":
			switchs += "\t\tcase " + str(hex(i)) + ": " + str(hex(i).lstrip('0x')) + "_switch(); break;\n"
		else:
			switchs += "\t\tcase " + str(hex(i)) + ": CHECKJUMP(); " + fnc_name + "(); break;\n"



		if i != 255:
			pointers += ","
			operands += ","
		else:
			pointers += " "
			operands += " "

	pointers += "};\n\n"
	operands += "},\n"

	pointer_tables.append(pointers)
	operand_tables.append(operands)
	endian_tables.append(endianess_fix)
	switchs += "\t}\n}"
	switch_tables.append(switchs)




looparray('MAIN_INSTRUCTIONS')
looparray('CB_INSTRUCTIONS')
looparray('ED_INSTRUCTIONS')
looparray('DD_INSTRUCTIONS')
looparray('FD_INSTRUCTIONS')
looparray('DDCB_INSTRUCTIONS')
looparray('FDCB_INSTRUCTIONS')

for item in pointer_tables:
	print item

print "\n\n"

print "ARGUMENT_SETS a_set[7] = {"
for item in operand_tables:
	print item

print "};"

print "\n\n"


for item in sorted(sorted_declarations):
	print item

print "\n\n"

for item in sorted(sorted_functions):
	print item + "\n"


fix = "BYTE endianess_fix[256 * 7] = {"
#for item in endianess_fix:
for i in range(0, len(endian_tables)):
	for j in range (0, 256):
		if j % 32 == 0:
			fix += "\n\t\t"

		fix += str(endian_tables[i][j]) + ","
	fix += "\n"

fix += "\n};"
print fix



for item in switch_tables:
	print "\n\n"
	print item