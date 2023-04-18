//===-- MyArchSubtarget.cpp - MyArch Subtarget Information ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the MyArch specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "MyArchSubtarget.h"
#include "MyArch.h"
#include "MyArchCallLowering.h"
#include "MyArchFrameLowering.h"
#include "MyArchLegalizerInfo.h"
#include "MyArchRegisterBankInfo.h"
#include "MyArchTargetMachine.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "myarch-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MyArchGenSubtargetInfo.inc"

void MyArchSubtarget::anchor() {}

MyArchSubtarget &MyArchSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS, StringRef ABIName) {
  // Determine default and user-specified characteristics
  bool Is64Bit = TT.isArch64Bit();
  std::string CPUName = std::string(CPU);
  std::string TuneCPUName = std::string(TuneCPU);
  if (CPUName.empty())
    CPUName = Is64Bit ? "generic-rv64" : "generic-rv32";
  if (TuneCPUName.empty())
    TuneCPUName = CPUName;
  ParseSubtargetFeatures(CPUName, TuneCPUName, FS);
  if (Is64Bit) {
    XLenVT = MVT::i64;
    XLen = 64;
  }

  TargetABI = MyArchABI::computeTargetABI(TT, getFeatureBits(), ABIName);
  MyArchFeatures::validate(TT, getFeatureBits());
  return *this;
}

MyArchSubtarget::MyArchSubtarget(const Triple &TT, StringRef CPU,
                               StringRef TuneCPU, StringRef FS,
                               StringRef ABIName, const TargetMachine &TM)
    : MyArchGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      UserReservedRegister(MyArch::NUM_TARGET_REGS),
      FrameLowering(initializeSubtargetDependencies(TT, CPU, TuneCPU, FS, ABIName)),
      InstrInfo(*this), RegInfo(getHwMode()), TLInfo(TM, *this) {
  CallLoweringInfo.reset(new MyArchCallLowering(*getTargetLowering()));
  Legalizer.reset(new MyArchLegalizerInfo(*this));

  auto *RBI = new MyArchRegisterBankInfo(*getRegisterInfo());
  RegBankInfo.reset(RBI);
  InstSelector.reset(createMyArchInstructionSelector(
      *static_cast<const MyArchTargetMachine *>(&TM), *this, *RBI));
}

const CallLowering *MyArchSubtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}

InstructionSelector *MyArchSubtarget::getInstructionSelector() const {
  return InstSelector.get();
}

const LegalizerInfo *MyArchSubtarget::getLegalizerInfo() const {
  return Legalizer.get();
}

const RegisterBankInfo *MyArchSubtarget::getRegBankInfo() const {
  return RegBankInfo.get();
}
