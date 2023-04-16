//===-- MyArchInstrInfo.cpp - MyArch Instruction Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the MyArch implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "MyArchInstrInfo.h"
#if CH >= CH3_1

#include "MyArchTargetMachine.h"
#include "MyArchMachineFunction.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MyArchGenInstrInfo.inc"

// Pin the vtable to this file.
void MyArchInstrInfo::anchor() {}

//@MyArchInstrInfo {
MyArchInstrInfo::MyArchInstrInfo(const MyArchSubtarget &STI)
    : 
#if CH >= CH9_2
      MyArchGenInstrInfo(MyArch::ADJCALLSTACKDOWN, MyArch::ADJCALLSTACKUP),
#endif
      Subtarget(STI) {}

const MyArchInstrInfo *MyArchInstrInfo::create(MyArchSubtarget &STI) {
  return llvm::createMyArchSEInstrInfo(STI);
}

#if CH >= CH3_5 //1
MachineMemOperand *
MyArchInstrInfo::GetMemOperand(MachineBasicBlock &MBB, int FI,
                             MachineMemOperand::Flags Flags) const {

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  return MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                 Flags, MFI.getObjectSize(FI),
                                 MFI.getObjectAlign(FI));
}
#endif

//@GetInstSizeInBytes {
/// Return the number of bytes of code the specified instruction may be.
unsigned MyArchInstrInfo::GetInstSizeInBytes(const MachineInstr &MI) const {
//@GetInstSizeInBytes - body
  switch (MI.getOpcode()) {
  default:
    return MI.getDesc().getSize();
#if CH >= CH11_2
  case  TargetOpcode::INLINEASM: {       // Inline Asm: Variable size.
    const MachineFunction *MF = MI.getParent()->getParent();
    const char *AsmStr = MI.getOperand(0).getSymbolName();
    return getInlineAsmLength(AsmStr, *MF->getTarget().getMCAsmInfo());
  }
#endif
  }
}

#endif // #if CH >= CH3_1