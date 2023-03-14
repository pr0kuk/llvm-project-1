//===-- MyArchMCTargetDesc.h - MyArch Target Descriptions -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides MyArch specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCTARGETDESC_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"

#include <memory>

namespace llvm {
class Target;
class Triple;

extern Target TheMyArchTarget;

} // End llvm namespace

// Defines symbolic names for MyArch registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "MyArchGenRegisterInfo.inc"

// Defines symbolic names for the MyArch instructions.
#define GET_INSTRINFO_ENUM
#include "MyArchGenInstrInfo.inc"

#endif
