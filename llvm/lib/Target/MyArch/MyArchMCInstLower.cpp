//===-- MyArchMCInstLower.cpp - Convert MyArch MachineInstr to an MCInst ------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower MyArch MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "MyArch.h"
#include "MyArchSubtarget.h"
#include "MCTargetDesc/MyArchMCExpr.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static MCOperand lowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym,
                                    const AsmPrinter &AP) {
  MCContext &Ctx = AP.OutContext;
  MyArchMCExpr::VariantKind Kind;

  switch (MO.getTargetFlags()) {
  default:
    llvm_unreachable("Unknown target flag on GV operand");
  case MyArchII::MO_None:
    Kind = MyArchMCExpr::VK_MyArch_None;
    break;
  case MyArchII::MO_CALL:
    Kind = MyArchMCExpr::VK_MyArch_CALL;
    break;
  case MyArchII::MO_PLT:
    Kind = MyArchMCExpr::VK_MyArch_CALL_PLT;
    break;
  case MyArchII::MO_LO:
    Kind = MyArchMCExpr::VK_MyArch_LO;
    break;
  case MyArchII::MO_HI:
    Kind = MyArchMCExpr::VK_MyArch_HI;
    break;
  case MyArchII::MO_PCREL_LO:
    Kind = MyArchMCExpr::VK_MyArch_PCREL_LO;
    break;
  case MyArchII::MO_PCREL_HI:
    Kind = MyArchMCExpr::VK_MyArch_PCREL_HI;
    break;
  case MyArchII::MO_GOT_HI:
    Kind = MyArchMCExpr::VK_MyArch_GOT_HI;
    break;
  case MyArchII::MO_TPREL_LO:
    Kind = MyArchMCExpr::VK_MyArch_TPREL_LO;
    break;
  case MyArchII::MO_TPREL_HI:
    Kind = MyArchMCExpr::VK_MyArch_TPREL_HI;
    break;
  case MyArchII::MO_TPREL_ADD:
    Kind = MyArchMCExpr::VK_MyArch_TPREL_ADD;
    break;
  case MyArchII::MO_TLS_GOT_HI:
    Kind = MyArchMCExpr::VK_MyArch_TLS_GOT_HI;
    break;
  case MyArchII::MO_TLS_GD_HI:
    Kind = MyArchMCExpr::VK_MyArch_TLS_GD_HI;
    break;
  }

  const MCExpr *ME =
      MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    ME = MCBinaryExpr::createAdd(
        ME, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  if (Kind != MyArchMCExpr::VK_MyArch_None)
    ME = MyArchMCExpr::create(ME, Kind, Ctx);
  return MCOperand::createExpr(ME);
}

bool llvm::LowerMyArchMachineOperandToMCOperand(const MachineOperand &MO,
                                               MCOperand &MCOp,
                                               const AsmPrinter &AP) {
  switch (MO.getType()) {
  default:
    report_fatal_error("LowerMyArchMachineInstrToMCInst: unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return false;
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  case MachineOperand::MO_RegisterMask:
    // Regmasks are like implicit defs.
    return false;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = lowerSymbolOperand(MO, MO.getMBB()->getSymbol(), AP);
    break;
  case MachineOperand::MO_GlobalAddress:
    MCOp = lowerSymbolOperand(MO, AP.getSymbol(MO.getGlobal()), AP);
    break;
  case MachineOperand::MO_BlockAddress:
    MCOp = lowerSymbolOperand(
        MO, AP.GetBlockAddressSymbol(MO.getBlockAddress()), AP);
    break;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = lowerSymbolOperand(
        MO, AP.GetExternalSymbolSymbol(MO.getSymbolName()), AP);
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    MCOp = lowerSymbolOperand(MO, AP.GetCPISymbol(MO.getIndex()), AP);
    break;
  case MachineOperand::MO_JumpTableIndex:
    MCOp = lowerSymbolOperand(MO, AP.GetJTISymbol(MO.getIndex()), AP);
    break;
  }
  return true;
}

static bool lowerMyArchVMachineInstrToMCInst(const MachineInstr *MI,
                                            MCInst &OutMI) {
  const MyArchVPseudosTable::PseudoInfo *RVV =
      MyArchVPseudosTable::getPseudoInfo(MI->getOpcode());
  if (!RVV)
    return false;

  OutMI.setOpcode(RVV->BaseInstr);

  const MachineBasicBlock *MBB = MI->getParent();
  assert(MBB && "MI expected to be in a basic block");
  const MachineFunction *MF = MBB->getParent();
  assert(MF && "MBB expected to be in a machine function");

  const TargetRegisterInfo *TRI =
      MF->getSubtarget<MyArchSubtarget>().getRegisterInfo();
  assert(TRI && "TargetRegisterInfo expected");

  uint64_t TSFlags = MI->getDesc().TSFlags;
  int NumOps = MI->getNumExplicitOperands();

  for (const MachineOperand &MO : MI->explicit_operands()) {
    int OpNo = (int)MI->getOperandNo(&MO);
    assert(OpNo >= 0 && "Operand number doesn't fit in an 'int' type");

    // Skip VL and SEW operands which are the last two operands if present.
    if ((TSFlags & MyArchII::HasVLOpMask) && OpNo == (NumOps - 2))
      continue;
    if ((TSFlags & MyArchII::HasSEWOpMask) && OpNo == (NumOps - 1))
      continue;

    // Skip merge op. It should be the first operand after the result.
    if ((TSFlags & MyArchII::HasMergeOpMask) && OpNo == 1) {
      assert(MI->getNumExplicitDefs() == 1);
      continue;
    }

    MCOperand MCOp;
    switch (MO.getType()) {
    default:
      llvm_unreachable("Unknown operand type");
    case MachineOperand::MO_Register: {
      unsigned Reg = MO.getReg();

      if (MyArch::VRM2RegClass.contains(Reg) ||
          MyArch::VRM4RegClass.contains(Reg) ||
          MyArch::VRM8RegClass.contains(Reg)) {
        Reg = TRI->getSubReg(Reg, MyArch::sub_vrm1_0);
        assert(Reg && "Subregister does not exist");
      } else if (MyArch::FPR16RegClass.contains(Reg)) {
        Reg = TRI->getMatchingSuperReg(Reg, MyArch::sub_16, &MyArch::FPR32RegClass);
        assert(Reg && "Subregister does not exist");
      } else if (MyArch::FPR64RegClass.contains(Reg)) {
        Reg = TRI->getSubReg(Reg, MyArch::sub_32);
        assert(Reg && "Superregister does not exist");
      }

      MCOp = MCOperand::createReg(Reg);
      break;
    }
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::createImm(MO.getImm());
      break;
    }
    OutMI.addOperand(MCOp);
  }

  // Unmasked pseudo instructions need to append dummy mask operand to
  // V instructions. All V instructions are modeled as the masked version.
  if (TSFlags & MyArchII::HasDummyMaskOpMask)
    OutMI.addOperand(MCOperand::createReg(MyArch::NoRegister));

  return true;
}

void llvm::LowerMyArchMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                          const AsmPrinter &AP) {
  if (lowerMyArchVMachineInstrToMCInst(MI, OutMI))
    return;

  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (LowerMyArchMachineOperandToMCOperand(MO, MCOp, AP))
      OutMI.addOperand(MCOp);
  }

  if (OutMI.getOpcode() == MyArch::PseudoReadVLENB) {
    OutMI.setOpcode(MyArch::CSRRS);
    OutMI.addOperand(MCOperand::createImm(
        MyArchSysReg::lookupSysRegByName("VLENB")->Encoding));
    OutMI.addOperand(MCOperand::createReg(MyArch::X0));
    return;
  }

  if (OutMI.getOpcode() == MyArch::PseudoReadVL) {
    OutMI.setOpcode(MyArch::CSRRS);
    OutMI.addOperand(MCOperand::createImm(
        MyArchSysReg::lookupSysRegByName("VL")->Encoding));
    OutMI.addOperand(MCOperand::createReg(MyArch::X0));
    return;
  }
}
