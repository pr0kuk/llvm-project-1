//===-- MyArch.td - Describe the MyArch Target Machine ----*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the MyArch target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Target-dependent interfaces
//===----------------------------------------------------------------------===//

include "MyArchRegisterInfo.td"

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "MyArchInstrInfo.td"
def MyArchInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// MyArch processors supported.
//===----------------------------------------------------------------------===//

// Will generate MyArchGenAsmWrite.inc included by MyArchInstPrinter.cpp, contents
//  as follows,
// void MyArchInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *MyArchInstPrinter::getRegisterName(unsigned RegNo) {...}
def MyArch : Target {
  let InstructionSet = MyArchInstrInfo;
}

