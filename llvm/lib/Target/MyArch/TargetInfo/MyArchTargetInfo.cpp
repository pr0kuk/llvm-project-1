//===-- MyArchTargetInfo.cpp - MyArch Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MyArch.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

Target llvm::TheMyArchTarget;

extern "C" void LLVMInitializeMyArchTargetInfo() {
  RegisterTarget<Triple::myarch,
                 /*HasJIT=*/false>
      X(TheMyArchTarget, "MyArch", "MyArch (32-bit simhulator arch)", "MyArch");
}
