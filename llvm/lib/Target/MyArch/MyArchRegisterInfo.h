//===-- MyArchRegisterInfo.h - MyArch Register Information Impl ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the MyArch implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArchREGISTERINFO_H
#define LLVM_LIB_TARGET_MyArch_MyArchREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "MyArchGenRegisterInfo.inc"

namespace llvm {

struct MyArchRegisterInfo : public MyArchGenRegisterInfo {

  MyArchRegisterInfo(unsigned HwMode);

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool isAsmClobberable(const MachineFunction &MF,
                        MCRegister PhysReg) const override;

  bool isConstantPhysReg(MCRegister PhysReg) const override;

  const uint32_t *getNoPreservedMask() const override;

  bool hasReservedSpillSlot(const MachineFunction &MF, Register Reg,
                            int &FrameIdx) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override {
    return true;
  }

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF,
                     unsigned Kind = 0) const override {
    return &MyArch::GPRRegClass;
  }
};
}

#endif
