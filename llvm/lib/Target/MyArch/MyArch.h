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

// #include "MyArchConfig.h"
#include "MCTargetDesc/MyArchMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class MyArchTargetMachine;
  class FunctionPass;

#if CH >= CH9_3
#ifdef ENABLE_GPRESTORE
  FunctionPass *createMyArchEmitGPRestorePass(MyArchTargetMachine &TM);
#endif
#endif //#if CH >= CH9_3
#if CH >= CH8_2 //1
  FunctionPass *createMyArchDelaySlotFillerPass(MyArchTargetMachine &TM);
#endif
#if CH >= CH8_2 //2
  FunctionPass *createMyArchDelJmpPass(MyArchTargetMachine &TM);
#endif
#if CH >= CH8_2 //3
  FunctionPass *createMyArchLongBranchPass(MyArchTargetMachine &TM);
#endif

} // end namespace llvm;

#endif
