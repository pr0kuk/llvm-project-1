#include "myarchRegisterInfo.h"
#include "myarch.h"
#include "myarchInstrInfo.h"
//#include "myarchMachineFunctionInfo.h"
#include "myarchSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "myarch-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "myarchGenRegisterInfo.inc"

myarchRegisterInfo::myarchRegisterInfo() : myarchGenRegisterInfo(myarch::R1) {}

#if 0
bool myarchRegisterInfo::needsFrameMoves(const MachineFunction &MF) {
  return MF.needsFrameMoves();
}
#endif

const MCPhysReg *
myarchRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_myarch_SaveList;
}

// TODO: check cconv
BitVector myarchRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(myarch::R0);
  Reserved.set(myarch::R1);
  Reserved.set(myarch::R2);
  Reserved.set(myarch::R3);
  return Reserved;
}

bool myarchRegisterInfo::requiresRegisterScavenging(
    const MachineFunction &MF) const {
  return false; // TODO: what for?
}

#if 0
bool myarchRegisterInfo::useFPForScavengingIndex(
    const MachineFunction &MF) const {
  llvm_unreachable("");
}
#endif

// TODO: rewrite!
void myarchRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  int Offset = getFrameLowering(MF)
                   ->getFrameIndexReference(MF, FrameIndex, FrameReg)
                   .getFixed();
  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  if (!isInt<16>(Offset)) {
    llvm_unreachable("");
  }

  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false, false, false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

Register myarchRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? myarch::FP : myarch::SP;
}

const uint32_t *
myarchRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const {
  return CSR_myarch_RegMask;
}
