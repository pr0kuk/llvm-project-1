//===- MyArchCleanupVSETVLI.cpp - Cleanup unneeded VSETVLI instructions ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a function pass that removes duplicate vsetvli
// instructions within a basic block.
//
//===----------------------------------------------------------------------===//

#include "MyArch.h"
#include "MyArchSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
using namespace llvm;

#define DEBUG_TYPE "myarch-cleanup-vsetvli"
#define MyArch_CLEANUP_VSETVLI_NAME "MyArch Cleanup VSETVLI pass"

namespace {

class MyArchCleanupVSETVLI : public MachineFunctionPass {
public:
  static char ID;

  MyArchCleanupVSETVLI() : MachineFunctionPass(ID) {
    initializeMyArchCleanupVSETVLIPass(*PassRegistry::getPassRegistry());
  }
  bool runOnMachineFunction(MachineFunction &MF) override;
  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::IsSSA);
  }

  // This pass modifies the program, but does not modify the CFG
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override { return MyArch_CLEANUP_VSETVLI_NAME; }
};

} // end anonymous namespace

char MyArchCleanupVSETVLI::ID = 0;

INITIALIZE_PASS(MyArchCleanupVSETVLI, DEBUG_TYPE,
                MyArch_CLEANUP_VSETVLI_NAME, false, false)

bool MyArchCleanupVSETVLI::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  bool Changed = false;
  MachineInstr *PrevVSETVLI = nullptr;

  for (auto MII = MBB.begin(), MIE = MBB.end(); MII != MIE;) {
    MachineInstr &MI = *MII++;

    if (MI.getOpcode() != MyArch::PseudoVSETVLI &&
        MI.getOpcode() != MyArch::PseudoVSETIVLI) {
      if (PrevVSETVLI &&
          (MI.isCall() || MI.modifiesRegister(MyArch::VL) ||
           MI.modifiesRegister(MyArch::VTYPE))) {
        // Old VL/VTYPE is overwritten.
        PrevVSETVLI = nullptr;
      }
      continue;
    }

    // If we don't have a previous VSET{I}VLI or the VL output isn't dead, we
    // can't remove this VSETVLI.
    if (!PrevVSETVLI || !MI.getOperand(0).isDead()) {
      PrevVSETVLI = &MI;
      continue;
    }

    // If a previous "set vl" instruction opcode is different from this one, we
    // can't differentiate the AVL values.
    if (PrevVSETVLI->getOpcode() != MI.getOpcode()) {
      PrevVSETVLI = &MI;
      continue;
    }

    // The remaining two cases are
    // 1. PrevVSETVLI = PseudoVSETVLI
    //    MI = PseudoVSETVLI
    //
    // 2. PrevVSETVLI = PseudoVSETIVLI
    //    MI = PseudoVSETIVLI
    Register AVLReg;
    bool SameAVL = false;
    if (MI.getOpcode() == MyArch::PseudoVSETVLI) {
      AVLReg = MI.getOperand(1).getReg();
      SameAVL = PrevVSETVLI->getOperand(1).getReg() == AVLReg;
    } else { // MyArch::PseudoVSETIVLI
      SameAVL =
          PrevVSETVLI->getOperand(1).getImm() == MI.getOperand(1).getImm();
    }
    int64_t PrevVTYPEImm = PrevVSETVLI->getOperand(2).getImm();
    int64_t VTYPEImm = MI.getOperand(2).getImm();

    // Does this VSET{I}VLI use the same AVL register/value and VTYPE immediate?
    if (!SameAVL || PrevVTYPEImm != VTYPEImm) {
      PrevVSETVLI = &MI;
      continue;
    }

    // If the AVLReg is X0 we need to look at the output VL of both VSETVLIs.
    if ((MI.getOpcode() == MyArch::PseudoVSETVLI) && (AVLReg == MyArch::X0)) {
      assert((PrevVSETVLI->getOpcode() == MyArch::PseudoVSETVLI) &&
             "Unexpected vsetvli opcode.");
      Register PrevOutVL = PrevVSETVLI->getOperand(0).getReg();
      Register OutVL = MI.getOperand(0).getReg();
      // We can't remove if the previous VSETVLI left VL unchanged and the
      // current instruction is setting it to VLMAX. Without knowing the VL
      // before the previous instruction we don't know if this is a change.
      if (PrevOutVL == MyArch::X0 && OutVL != MyArch::X0) {
        PrevVSETVLI = &MI;
        continue;
      }
    }

    // This VSETVLI is redundant, remove it.
    MI.eraseFromParent();
    Changed = true;
  }

  return Changed;
}

bool MyArchCleanupVSETVLI::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()))
    return false;

  // Skip if the vector extension is not enabled.
  const MyArchSubtarget &ST = MF.getSubtarget<MyArchSubtarget>();
  if (!ST.hasStdExtV())
    return false;

  bool Changed = false;

  for (MachineBasicBlock &MBB : MF)
    Changed |= runOnMachineBasicBlock(MBB);

  return Changed;
}

/// Returns an instance of the Cleanup VSETVLI pass.
FunctionPass *llvm::createMyArchCleanupVSETVLIPass() {
  return new MyArchCleanupVSETVLI();
}
