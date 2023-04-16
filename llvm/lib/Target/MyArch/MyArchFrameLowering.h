//===-- MyArchFrameLowering.h - Define frame lowering for MyArch ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_MyArch_MyArchFRAMELOWERING_H
#define LLVM_LIB_TARGET_MyArch_MyArchFRAMELOWERING_H

// #include "MyArchConfig.h"

#include "MyArch.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
  class MyArchSubtarget;

class MyArchFrameLowering : public TargetFrameLowering {
protected:
  const MyArchSubtarget &STI;

public:
  explicit MyArchFrameLowering(const MyArchSubtarget &sti, unsigned Alignment)
    : TargetFrameLowering(StackGrowsDown, Align(Alignment), 0, Align(Alignment)),
      STI(sti) {
  }

  static const MyArchFrameLowering *create(const MyArchSubtarget &ST);

  bool hasFP(const MachineFunction &MF) const override;

};

/// Create MyArchFrameLowering objects.
const MyArchFrameLowering *createMyArchSEFrameLowering(const MyArchSubtarget &ST);

} // End llvm namespace

#endif