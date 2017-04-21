[![Build Status](https://travis-ci.org/MoebiuZ/Z80-emulator.svg?branch=master)](https://travis-ci.org/MoebiuZ/Z80-emulator)
<a href="https://scan.coverity.com/projects/moebiuz-z80-emulator">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/12495/badge.svg"/>
</a>

# Zilog Z80 emulator

A Z80 emulator written in C++ just for fun.

**WARNING** Interruptions are not completely implemented and this readme are only personal notes.


# Features

* Undocumented opcodes

# Usage

Z80 cpu = new Z80();
cpu->SetIOReadCallback();
cpu->SetIOWriteCallback();


Z80ADDRESSBUS memory;
cpu->memory = memory;


cpu->SetMemReadCallback();
cpu->SetMemWriteCallback();


cpu->Reset(); // Resets the Z80

cpu->Execute();   // Will execute an instruction

cpu->ExecuteMCycle(); // Will execute an M-Cycle (4 T-States)

cpu->ExecuteTStates(num_tstates); // Will execute n T-States


cpu->NMI();


cpu->IRQ0();
cpu->IRQ1();
cpu->IRQ2();
cpu->IRQ3();


cpu->isHalted();


# NOTES

T-States and M-Cycles

# References

[Z80 CPU User Manual - Zilog](http://www.zilog.com/appnotes_download.php?FromPage=DirectLink&dn=UM0080&ft=User%20Manual&f=YUhSMGNEb3ZMM2QzZHk1NmFXeHZaeTVqYjIwdlpHOWpjeTk2T0RBdlZVMHdNRGd3TG5Ca1pnPT0=)

[Z80 Family CPU User Manual](http://www.z80.info/zip/z80cpu_um.pdf)

[The Undocumented Z80 Documented](http://www.z80.info/zip/z80-documented.pdf)

[Z80 Flag Affection](http://www.z80.info/z80sflag.htm)

[Interrupt Behaviour of the Z80 CPU](http://www.z80.info/interrup.htm)

[MEMPTR, esoteric register of the ZiLOG Z80 CPU](http://www.grimware.org/lib/exe/fetch.php/documentations/devices/z80/z80.memptr.eng.txt)
