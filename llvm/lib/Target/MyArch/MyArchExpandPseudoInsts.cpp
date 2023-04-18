//===-- MyArchExpandPseudoInsts.cpp - Expand pseudo instructions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "MyArch.h"
#include "MyArchInstrInfo.h"
#include "MyArchTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define MyArch_EXPAND_PSEUDO_NAME "MyArch pseudo instruction expansion pass"

namespace {

class MyArchExpandPseudo : public MachineFunctionPass {
public:
  const MyArchInstrInfo *TII;
  static char ID;

  MyArchExpandPseudo() : MachineFunctionPass(ID) {
    initializeMyArchExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return MyArch_EXPAND_PSEUDO_NAME; }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAuipcInstPair(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI,
                           MachineBasicBlock::iterator &NextMBBI,
                           unsigned FlagsHi, unsigned SecondOpcode);
  bool expandLoadLocalAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddress(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI,
                         MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSIEAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSGDAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandVSetVL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool expandVMSET_VMCLR(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI, unsigned Opcode);
};

char MyArchExpandPseudo::ID = 0;

bool MyArchExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  TII = static_cast<const MyArchInstrInfo *>(MF.getSubtarget().getInstrInfo());
  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);
  return Modified;
}

bool MyArchExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool MyArchExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  // MyArchInstrInfo::getInstSizeInBytes hard-codes the number of expanded
  // instructions for each pseudo, and must be updated when adding new pseudos
  // or changing existing ones.
  switch (MBBI->getOpcode()) {
  case MyArch::PseudoLLA:
    return expandLoadLocalAddress(MBB, MBBI, NextMBBI);
  case MyArch::PseudoLA:
    return expandLoadAddress(MBB, MBBI, NextMBBI);
  case MyArch::PseudoLA_TLS_IE:
    return expandLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case MyArch::PseudoLA_TLS_GD:
    return expandLoadTLSGDAddress(MBB, MBBI, NextMBBI);
  case MyArch::PseudoVSETVLI:
  case MyArch::PseudoVSETIVLI:
    return expandVSetVL(MBB, MBBI);
  case MyArch::PseudoVMCLR_M_B1:
  case MyArch::PseudoVMCLR_M_B2:
  case MyArch::PseudoVMCLR_M_B4:
  case MyArch::PseudoVMCLR_M_B8:
  case MyArch::PseudoVMCLR_M_B16:
  case MyArch::PseudoVMCLR_M_B32:
  case MyArch::PseudoVMCLR_M_B64:
    // vmclr.m vd => vmxor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, MyArch::VMXOR_MM);
  case MyArch::PseudoVMSET_M_B1:
  case MyArch::PseudoVMSET_M_B2:
  case MyArch::PseudoVMSET_M_B4:
  case MyArch::PseudoVMSET_M_B8:
  case MyArch::PseudoVMSET_M_B16:
  case MyArch::PseudoVMSET_M_B32:
  case MyArch::PseudoVMSET_M_B64:
    // vmset.m vd => vmxnor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, MyArch::VMXNOR_MM);
  }

  return false;
}

bool MyArchExpandPseudo::expandAuipcInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  const MachineOperand &Symbol = MI.getOperand(1);

  MachineBasicBlock *NewMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(MyArch::AUIPC), DestReg)
      .addDisp(Symbol, 0, FlagsHi);
  BuildMI(NewMBB, DL, TII->get(SecondOpcode), DestReg)
      .addReg(DestReg)
      .addMBB(NewMBB, MyArchII::MO_PCREL_LO);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool MyArchExpandPseudo::expandLoadLocalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, MyArchII::MO_PCREL_HI,
                             MyArch::ADDI);
}

bool MyArchExpandPseudo::expandLoadAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  unsigned SecondOpcode;
  unsigned FlagsHi;
  if (MF->getTarget().isPositionIndependent()) {
    const auto &STI = MF->getSubtarget<MyArchSubtarget>();
    SecondOpcode = STI.is64Bit() ? MyArch::LD : MyArch::LW;
    FlagsHi = MyArchII::MO_GOT_HI;
  } else {
    SecondOpcode = MyArch::ADDI;
    FlagsHi = MyArchII::MO_PCREL_HI;
  }
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, FlagsHi, SecondOpcode);
}

bool MyArchExpandPseudo::expandLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  const auto &STI = MF->getSubtarget<MyArchSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? MyArch::LD : MyArch::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, MyArchII::MO_TLS_GOT_HI,
                             SecondOpcode);
}

bool MyArchExpandPseudo::expandLoadTLSGDAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, MyArchII::MO_TLS_GD_HI,
                             MyArch::ADDI);
}

bool MyArchExpandPseudo::expandVSetVL(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MBBI) {
  assert(MBBI->getNumOperands() == 5 && "Unexpected instruction format");

  DebugLoc DL = MBBI->getDebugLoc();

  assert((MBBI->getOpcode() == MyArch::PseudoVSETVLI ||
          MBBI->getOpcode() == MyArch::PseudoVSETIVLI) &&
         "Unexpected pseudo instruction");
  unsigned Opcode;
  if (MBBI->getOpcode() == MyArch::PseudoVSETVLI)
    Opcode = MyArch::VSETVLI;
  else
    Opcode = MyArch::VSETIVLI;
  const MCInstrDesc &Desc = TII->get(Opcode);
  assert(Desc.getNumOperands() == 3 && "Unexpected instruction format");

  Register DstReg = MBBI->getOperand(0).getReg();
  bool DstIsDead = MBBI->getOperand(0).isDead();
  BuildMI(MBB, MBBI, DL, Desc)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .add(MBBI->getOperand(1))  // VL
      .add(MBBI->getOperand(2)); // VType

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool MyArchExpandPseudo::expandVMSET_VMCLR(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          unsigned Opcode) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register DstReg = MBBI->getOperand(0).getReg();
  const MCInstrDesc &Desc = TII->get(Opcode);
  BuildMI(MBB, MBBI, DL, Desc, DstReg)
      .addReg(DstReg, RegState::Undef)
      .addReg(DstReg, RegState::Undef);
  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

} // end of anonymous namespace

INITIALIZE_PASS(MyArchExpandPseudo, "myarch-expand-pseudo",
                MyArch_EXPAND_PSEUDO_NAME, false, false)
namespace llvm {

FunctionPass *createMyArchExpandPseudoPass() { return new MyArchExpandPseudo(); }

} // end of namespace llvm
