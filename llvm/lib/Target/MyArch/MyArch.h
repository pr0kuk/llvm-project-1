//===-- MyArch.h - Top-level interface for MyArch -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// RISC-V back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArch_H
#define LLVM_LIB_TARGET_MyArch_MyArch_H

#include "MCTargetDesc/MyArchBaseInfo.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class MyArchRegisterBankInfo;
class MyArchSubtarget;
class MyArchTargetMachine;
class AsmPrinter;
class FunctionPass;
class InstructionSelector;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class PassRegistry;

void LowerMyArchMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                    const AsmPrinter &AP);
bool LowerMyArchMachineOperandToMCOperand(const MachineOperand &MO,
                                         MCOperand &MCOp, const AsmPrinter &AP);

FunctionPass *createMyArchISelDag(MyArchTargetMachine &TM);

FunctionPass *createMyArchMergeBaseOffsetOptPass();
void initializeMyArchMergeBaseOffsetOptPass(PassRegistry &);

FunctionPass *createMyArchExpandPseudoPass();
void initializeMyArchExpandPseudoPass(PassRegistry &);

FunctionPass *createMyArchExpandAtomicPseudoPass();
void initializeMyArchExpandAtomicPseudoPass(PassRegistry &);

FunctionPass *createMyArchCleanupVSETVLIPass();
void initializeMyArchCleanupVSETVLIPass(PassRegistry &);

InstructionSelector *createMyArchInstructionSelector(const MyArchTargetMachine &,
                                                    MyArchSubtarget &,
                                                    MyArchRegisterBankInfo &);
}

#endif
