//===-- MyArch.h - Top-level interface for MyArch representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM MyArch back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArch_H
#define LLVM_LIB_TARGET_MyArch_MyArch_H

#include "MCTargetDesc/MyArchMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class MyArchTargetMachine;
  class FunctionPass;

} // end namespace llvm;

#endif