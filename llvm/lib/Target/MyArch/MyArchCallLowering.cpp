//===-- MyArchCallLowering.cpp - Call lowering -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file implements the lowering of LLVM calls to machine code calls for
/// GlobalISel.
//
//===----------------------------------------------------------------------===//

#include "MyArchCallLowering.h"
#include "MyArchISelLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

using namespace llvm;

MyArchCallLowering::MyArchCallLowering(const MyArchTargetLowering &TLI)
    : CallLowering(&TLI) {}

bool MyArchCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                    const Value *Val, ArrayRef<Register> VRegs,
                                    FunctionLoweringInfo &FLI) const {

  MachineInstrBuilder Ret = MIRBuilder.buildInstrNoInsert(MyArch::PseudoRET);

  if (Val != nullptr) {
    return false;
  }
  MIRBuilder.insertInstr(Ret);
  return true;
}

bool MyArchCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                             const Function &F,
                                             ArrayRef<ArrayRef<Register>> VRegs,
                                             FunctionLoweringInfo &FLI) const {

  if (F.arg_empty())
    return true;

  return false;
}

bool MyArchCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                  CallLoweringInfo &Info) const {
  return false;
}
