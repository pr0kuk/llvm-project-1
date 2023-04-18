//===-- MyArchRegisterInfo.cpp - MyArch Register Information ------*- C++ -*-===//
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

#include "MyArchRegisterInfo.h"
#include "MyArch.h"
#include "MyArchMachineFunctionInfo.h"
#include "MyArchSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "MyArchGenRegisterInfo.inc"

using namespace llvm;

static_assert(MyArch::X1 == MyArch::X0 + 1, "Register list not consecutive");
static_assert(MyArch::X31 == MyArch::X0 + 31, "Register list not consecutive");
static_assert(MyArch::F1_H == MyArch::F0_H + 1, "Register list not consecutive");
static_assert(MyArch::F31_H == MyArch::F0_H + 31,
              "Register list not consecutive");
static_assert(MyArch::F1_F == MyArch::F0_F + 1, "Register list not consecutive");
static_assert(MyArch::F31_F == MyArch::F0_F + 31,
              "Register list not consecutive");
static_assert(MyArch::F1_D == MyArch::F0_D + 1, "Register list not consecutive");
static_assert(MyArch::F31_D == MyArch::F0_D + 31,
              "Register list not consecutive");
static_assert(MyArch::V1 == MyArch::V0 + 1, "Register list not consecutive");
static_assert(MyArch::V31 == MyArch::V0 + 31, "Register list not consecutive");

MyArchRegisterInfo::MyArchRegisterInfo(unsigned HwMode)
    : MyArchGenRegisterInfo(MyArch::X1, /*DwarfFlavour*/0, /*EHFlavor*/0,
                           /*PC*/0, HwMode) {}

const MCPhysReg *
MyArchRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  auto &Subtarget = MF->getSubtarget<MyArchSubtarget>();
  if (MF->getFunction().getCallingConv() == CallingConv::GHC)
    return CSR_NoRegs_SaveList;
  if (MF->getFunction().hasFnAttribute("interrupt")) {
    if (Subtarget.hasStdExtD())
      return CSR_XLEN_F64_Interrupt_SaveList;
    if (Subtarget.hasStdExtF())
      return CSR_XLEN_F32_Interrupt_SaveList;
    return CSR_Interrupt_SaveList;
  }

  switch (Subtarget.getTargetABI()) {
  default:
    llvm_unreachable("Unrecognized ABI");
  case MyArchABI::ABI_ILP32:
  case MyArchABI::ABI_LP64:
    return CSR_ILP32_LP64_SaveList;
  case MyArchABI::ABI_ILP32F:
  case MyArchABI::ABI_LP64F:
    return CSR_ILP32F_LP64F_SaveList;
  case MyArchABI::ABI_ILP32D:
  case MyArchABI::ABI_LP64D:
    return CSR_ILP32D_LP64D_SaveList;
  }
}

BitVector MyArchRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  const MyArchFrameLowering *TFI = getFrameLowering(MF);
  BitVector Reserved(getNumRegs());

  // Mark any registers requested to be reserved as such
  for (size_t Reg = 0; Reg < getNumRegs(); Reg++) {
    if (MF.getSubtarget<MyArchSubtarget>().isRegisterReservedByUser(Reg))
      markSuperRegs(Reserved, Reg);
  }

  // Use markSuperRegs to ensure any register aliases are also reserved
  markSuperRegs(Reserved, MyArch::X0); // zero
  markSuperRegs(Reserved, MyArch::X2); // sp
  markSuperRegs(Reserved, MyArch::X3); // gp
  markSuperRegs(Reserved, MyArch::X4); // tp
  if (TFI->hasFP(MF))
    markSuperRegs(Reserved, MyArch::X8); // fp
  // Reserve the base register if we need to realign the stack and allocate
  // variable-sized objects at runtime.
  if (TFI->hasBP(MF))
    markSuperRegs(Reserved, MyArchABI::getBPReg()); // bp

  // V registers for code generation. We handle them manually.
  markSuperRegs(Reserved, MyArch::VL);
  markSuperRegs(Reserved, MyArch::VTYPE);
  markSuperRegs(Reserved, MyArch::VXSAT);
  markSuperRegs(Reserved, MyArch::VXRM);

  assert(checkAllSuperRegsMarked(Reserved));
  return Reserved;
}

bool MyArchRegisterInfo::isAsmClobberable(const MachineFunction &MF,
                                         MCRegister PhysReg) const {
  return !MF.getSubtarget<MyArchSubtarget>().isRegisterReservedByUser(PhysReg);
}

bool MyArchRegisterInfo::isConstantPhysReg(MCRegister PhysReg) const {
  return PhysReg == MyArch::X0;
}

const uint32_t *MyArchRegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}

// Frame indexes representing locations of CSRs which are given a fixed location
// by save/restore libcalls.
static const std::map<unsigned, int> FixedCSRFIMap = {
  {/*ra*/  MyArch::X1,   -1},
  {/*s0*/  MyArch::X8,   -2},
  {/*s1*/  MyArch::X9,   -3},
  {/*s2*/  MyArch::X18,  -4},
  {/*s3*/  MyArch::X19,  -5},
  {/*s4*/  MyArch::X20,  -6},
  {/*s5*/  MyArch::X21,  -7},
  {/*s6*/  MyArch::X22,  -8},
  {/*s7*/  MyArch::X23,  -9},
  {/*s8*/  MyArch::X24,  -10},
  {/*s9*/  MyArch::X25,  -11},
  {/*s10*/ MyArch::X26,  -12},
  {/*s11*/ MyArch::X27,  -13}
};

bool MyArchRegisterInfo::hasReservedSpillSlot(const MachineFunction &MF,
                                             Register Reg,
                                             int &FrameIdx) const {
  const auto *RVFI = MF.getInfo<MyArchMachineFunctionInfo>();
  if (!RVFI->useSaveRestoreLibCalls(MF))
    return false;

  auto FII = FixedCSRFIMap.find(Reg);
  if (FII == FixedCSRFIMap.end())
    return false;

  FrameIdx = FII->second;
  return true;
}

void MyArchRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                            int SPAdj, unsigned FIOperandNum,
                                            RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const MyArchInstrInfo *TII = MF.getSubtarget<MyArchSubtarget>().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  int Offset = getFrameLowering(MF)
                   ->getFrameIndexReference(MF, FrameIndex, FrameReg)
                   .getFixed() +
               MI.getOperand(FIOperandNum + 1).getImm();

  if (!isInt<32>(Offset)) {
    report_fatal_error(
        "Frame offsets outside of the signed 32-bit range not supported");
  }

  MachineBasicBlock &MBB = *MI.getParent();
  bool FrameRegIsKill = false;

  if (!isInt<12>(Offset)) {
    assert(isInt<32>(Offset) && "Int32 expected");
    // The offset won't fit in an immediate, so use a scratch register instead
    // Modify Offset and FrameReg appropriately
    Register ScratchReg = MRI.createVirtualRegister(&MyArch::GPRRegClass);
    TII->movImm(MBB, II, DL, ScratchReg, Offset);
    BuildMI(MBB, II, DL, TII->get(MyArch::ADD), ScratchReg)
        .addReg(FrameReg)
        .addReg(ScratchReg, RegState::Kill);
    Offset = 0;
    FrameReg = ScratchReg;
    FrameRegIsKill = true;
  }

  MI.getOperand(FIOperandNum)
      .ChangeToRegister(FrameReg, false, false, FrameRegIsKill);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

Register MyArchRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? MyArch::X8 : MyArch::X2;
}

const uint32_t *
MyArchRegisterInfo::getCallPreservedMask(const MachineFunction & MF,
                                        CallingConv::ID CC) const {
  auto &Subtarget = MF.getSubtarget<MyArchSubtarget>();

  if (CC == CallingConv::GHC)
    return CSR_NoRegs_RegMask;
  switch (Subtarget.getTargetABI()) {
  default:
    llvm_unreachable("Unrecognized ABI");
  case MyArchABI::ABI_ILP32:
  case MyArchABI::ABI_LP64:
    return CSR_ILP32_LP64_RegMask;
  case MyArchABI::ABI_ILP32F:
  case MyArchABI::ABI_LP64F:
    return CSR_ILP32F_LP64F_RegMask;
  case MyArchABI::ABI_ILP32D:
  case MyArchABI::ABI_LP64D:
    return CSR_ILP32D_LP64D_RegMask;
  }
}
