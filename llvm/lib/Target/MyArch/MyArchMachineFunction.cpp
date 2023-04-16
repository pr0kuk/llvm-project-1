//===-- MyArchMachineFunctionInfo.cpp - Private data used for MyArch ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MyArchMachineFunction.h"
#if CH >= CH3_1

#if CH >= CH3_2
#include "MCTargetDesc/MyArchBaseInfo.h"
#endif
#include "MyArchInstrInfo.h"
#include "MyArchSubtarget.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

bool FixGlobalBaseReg;

MyArchFunctionInfo::~MyArchFunctionInfo() {}

#if CH >= CH6_1
bool MyArchFunctionInfo::globalBaseRegFixed() const {
  return FixGlobalBaseReg;
}

bool MyArchFunctionInfo::globalBaseRegSet() const {
  return GlobalBaseReg;
}

unsigned MyArchFunctionInfo::getGlobalBaseReg() {
  return GlobalBaseReg = MyArch::GP;
}
#endif

#if CH >= CH3_5
void MyArchFunctionInfo::createEhDataRegsFI() {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  for (int I = 0; I < 2; ++I) {
    const TargetRegisterClass &RC = MyArch::CPURegsRegClass;

    EhDataRegFI[I] = MF.getFrameInfo().CreateStackObject(
        TRI.getSpillSize(RC), TRI.getSpillAlign(RC), false);
  }
}
#endif

#if CH >= CH9_2
MachinePointerInfo MyArchFunctionInfo::callPtrInfo(const char *ES) {
  return MachinePointerInfo(MF.getPSVManager().getExternalSymbolCallEntry(ES));
}

MachinePointerInfo MyArchFunctionInfo::callPtrInfo(const GlobalValue *GV) {
  return MachinePointerInfo(MF.getPSVManager().getGlobalValueCallEntry(GV));
}
#endif

void MyArchFunctionInfo::anchor() { }

#endif // #if CH >= CH3_1
