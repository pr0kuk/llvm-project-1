//===-- MyArchMCInstLower.cpp - Convert MyArch MachineInstr to MCInst ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower MyArch MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "MyArchMCInstLower.h"
#if CH >= CH3_2

#include "MyArchAsmPrinter.h"
#include "MyArchInstrInfo.h"
#include "MCTargetDesc/MyArchBaseInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

MyArchMCInstLower::MyArchMCInstLower(MyArchAsmPrinter &asmprinter)
  : AsmPrinter(asmprinter) {}

void MyArchMCInstLower::Initialize(MCContext* C) {
  Ctx = C;
}

#if CH >= CH6_1 //1
//@LowerSymbolOperand {
MCOperand MyArchMCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                              MachineOperandType MOTy,
                                              unsigned Offset) const {
  MCSymbolRefExpr::VariantKind Kind = MCSymbolRefExpr::VK_None;
  MyArchMCExpr::MyArchExprKind TargetKind = MyArchMCExpr::CEK_None;
  const MCSymbol *Symbol;

  switch(MO.getTargetFlags()) {
  default:                   llvm_unreachable("Invalid target flag!");
  case MyArchII::MO_NO_FLAG:
    break;

// MyArch_GPREL is for llc -march=MyArch -relocation-model=static -MyArch-islinux-
//  format=false (global var in .sdata).
  case MyArchII::MO_GPREL:
    TargetKind = MyArchMCExpr::CEK_GPREL;
    break;

#if CH >= CH9_1 //1
  case MyArchII::MO_GOT_CALL:
    TargetKind = MyArchMCExpr::CEK_GOT_CALL;
    break;
#endif
  case MyArchII::MO_GOT:
    TargetKind = MyArchMCExpr::CEK_GOT;
    break;
// ABS_HI and ABS_LO is for llc -march=MyArch -relocation-model=static (global 
//  var in .data).
  case MyArchII::MO_ABS_HI:
    TargetKind = MyArchMCExpr::CEK_ABS_HI;
    break;
  case MyArchII::MO_ABS_LO:
    TargetKind = MyArchMCExpr::CEK_ABS_LO;
    break;
#if CH >= CH12_1
  case MyArchII::MO_TLSGD:
    TargetKind = MyArchMCExpr::CEK_TLSGD;
    break;
  case MyArchII::MO_TLSLDM:
    TargetKind = MyArchMCExpr::CEK_TLSLDM;
    break;
  case MyArchII::MO_DTP_HI:
    TargetKind = MyArchMCExpr::CEK_DTP_HI;
    break;
  case MyArchII::MO_DTP_LO:
    TargetKind = MyArchMCExpr::CEK_DTP_LO;
    break;
  case MyArchII::MO_GOTTPREL:
    TargetKind = MyArchMCExpr::CEK_GOTTPREL;
    break;
  case MyArchII::MO_TP_HI:
    TargetKind = MyArchMCExpr::CEK_TP_HI;
    break;
  case MyArchII::MO_TP_LO:
    TargetKind = MyArchMCExpr::CEK_TP_LO;
    break;
#endif
  case MyArchII::MO_GOT_HI16:
    TargetKind = MyArchMCExpr::CEK_GOT_HI16;
    break;
  case MyArchII::MO_GOT_LO16:
    TargetKind = MyArchMCExpr::CEK_GOT_LO16;
    break;
  }

  switch (MOTy) {
  case MachineOperand::MO_GlobalAddress:
    Symbol = AsmPrinter.getSymbol(MO.getGlobal());
    Offset += MO.getOffset();
    break;

#if CH >= CH8_1
  case MachineOperand::MO_MachineBasicBlock:
    Symbol = MO.getMBB()->getSymbol();
    break;

  case MachineOperand::MO_BlockAddress:
    Symbol = AsmPrinter.GetBlockAddressSymbol(MO.getBlockAddress());
    Offset += MO.getOffset();
    break;
#endif

#if CH >= CH9_1 //2
  case MachineOperand::MO_ExternalSymbol:
    Symbol = AsmPrinter.GetExternalSymbolSymbol(MO.getSymbolName());
    Offset += MO.getOffset();
    break;
#endif

#if CH >= CH8_1
  case MachineOperand::MO_JumpTableIndex:
    Symbol = AsmPrinter.GetJTISymbol(MO.getIndex());
    break;
#endif

  default:
    llvm_unreachable("<unknown operand type>");
  }

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Kind, *Ctx);

  if (Offset) {
    // Assume offset is never negative.
    assert(Offset > 0);
    Expr = MCBinaryExpr::createAdd(Expr, MCConstantExpr::create(Offset, *Ctx),
                                   *Ctx);
  }

  if (TargetKind != MyArchMCExpr::CEK_None)
    Expr = MyArchMCExpr::create(TargetKind, Expr, *Ctx);

  return MCOperand::createExpr(Expr);

}
//@LowerSymbolOperand }
#endif // if CH >= CH6_1 //1

static void CreateMCInst(MCInst& Inst, unsigned Opc, const MCOperand& Opnd0,
                         const MCOperand& Opnd1,
                         const MCOperand& Opnd2 = MCOperand()) {
  Inst.setOpcode(Opc);
  Inst.addOperand(Opnd0);
  Inst.addOperand(Opnd1);
  if (Opnd2.isValid())
    Inst.addOperand(Opnd2);
}

#if CH >= CH6_1 //2
// Lower ".cpload $reg" to
//  "lui   $gp, %hi(_gp_disp)"
//  "addiu $gp, $gp, %lo(_gp_disp)"
//  "addu  $gp, $gp, $t9"
void MyArchMCInstLower::LowerCPLOAD(SmallVector<MCInst, 4>& MCInsts) {
  MCOperand GPReg = MCOperand::createReg(MyArch::GP);
  MCOperand T9Reg = MCOperand::createReg(MyArch::T9);
  StringRef SymName("_gp_disp");
  const MCSymbol *Sym = Ctx->getOrCreateSymbol(SymName);
  const MyArchMCExpr *MCSym;

  MCSym = MyArchMCExpr::create(Sym, MyArchMCExpr::CEK_ABS_HI, *Ctx);
  MCOperand SymHi = MCOperand::createExpr(MCSym);
  MCSym = MyArchMCExpr::create(Sym, MyArchMCExpr::CEK_ABS_LO, *Ctx);
  MCOperand SymLo = MCOperand::createExpr(MCSym);

  MCInsts.resize(3);

  CreateMCInst(MCInsts[0], MyArch::LUi, GPReg, SymHi);
  CreateMCInst(MCInsts[1], MyArch::ORi, GPReg, GPReg, SymLo);
  CreateMCInst(MCInsts[2], MyArch::ADD, GPReg, GPReg, T9Reg);
}
#endif

#if CH >= CH9_3
#ifdef ENABLE_GPRESTORE
// Lower ".cprestore offset" to "st $gp, offset($sp)".
void MyArchMCInstLower::LowerCPRESTORE(int64_t Offset,
                                     SmallVector<MCInst, 4>& MCInsts) {
  assert(isInt<32>(Offset) && (Offset >= 0) &&
         "Imm operand of .cprestore must be a non-negative 32-bit value.");

  MCOperand SPReg = MCOperand::createReg(MyArch::SP), BaseReg = SPReg;
  MCOperand GPReg = MCOperand::createReg(MyArch::GP);
  MCOperand ZEROReg = MCOperand::createReg(MyArch::ZERO);

  if (!isInt<16>(Offset)) {
    unsigned Hi = ((Offset + 0x8000) >> 16) & 0xffff;
    Offset &= 0xffff;
    MCOperand ATReg = MCOperand::createReg(MyArch::AT);
    BaseReg = ATReg;

    // lui   at,hi
    // add   at,at,sp
    MCInsts.resize(2);
    CreateMCInst(MCInsts[0], MyArch::LUi, ATReg, ZEROReg, MCOperand::createImm(Hi));
    CreateMCInst(MCInsts[1], MyArch::ADD, ATReg, ATReg, SPReg);
  }

  MCInst St;
  CreateMCInst(St, MyArch::ST, GPReg, BaseReg, MCOperand::createImm(Offset));
  MCInsts.push_back(St);
}
#endif
#endif //#if CH >= CH9_3

//@LowerOperand {
MCOperand MyArchMCInstLower::LowerOperand(const MachineOperand& MO,
                                        unsigned offset) const {
  MachineOperandType MOTy = MO.getType();

  switch (MOTy) {
  //@2
  default: llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit()) break;
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm() + offset);
#if CH >= CH8_1
  case MachineOperand::MO_MachineBasicBlock:
#endif
#if CH >= CH9_1 //3
  case MachineOperand::MO_ExternalSymbol:
#endif
#if CH >= CH8_1
  case MachineOperand::MO_JumpTableIndex:
  case MachineOperand::MO_BlockAddress:
#endif
#if CH >= CH6_1 //3
  case MachineOperand::MO_GlobalAddress:
//@1
    return LowerSymbolOperand(MO, MOTy, offset);
#endif
  case MachineOperand::MO_RegisterMask:
    break;
 }

  return MCOperand();
}

#if CH >= CH8_2 //1
MCOperand MyArchMCInstLower::createSub(MachineBasicBlock *BB1,
                                     MachineBasicBlock *BB2,
                                     MyArchMCExpr::MyArchExprKind Kind) const {
  const MCSymbolRefExpr *Sym1 = MCSymbolRefExpr::create(BB1->getSymbol(), *Ctx);
  const MCSymbolRefExpr *Sym2 = MCSymbolRefExpr::create(BB2->getSymbol(), *Ctx);
  const MCBinaryExpr *Sub = MCBinaryExpr::createSub(Sym1, Sym2, *Ctx);

  return MCOperand::createExpr(MyArchMCExpr::create(Kind, Sub, *Ctx));
}

void MyArchMCInstLower::
lowerLongBranchLUi(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MyArch::LUi);

  // Lower register operand.
  OutMI.addOperand(LowerOperand(MI->getOperand(0)));

  // Create %hi($tgt-$baltgt).
  OutMI.addOperand(createSub(MI->getOperand(1).getMBB(),
                             MI->getOperand(2).getMBB(),
                             MyArchMCExpr::CEK_ABS_HI));
}

void MyArchMCInstLower::
lowerLongBranchADDiu(const MachineInstr *MI, MCInst &OutMI, int Opcode,
                     MyArchMCExpr::MyArchExprKind Kind) const {
  OutMI.setOpcode(Opcode);

  // Lower two register operands.
  for (unsigned I = 0, E = 2; I != E; ++I) {
    const MachineOperand &MO = MI->getOperand(I);
    OutMI.addOperand(LowerOperand(MO));
  }

  // Create %lo($tgt-$baltgt) or %hi($tgt-$baltgt).
  OutMI.addOperand(createSub(MI->getOperand(2).getMBB(),
                             MI->getOperand(3).getMBB(), Kind));
}

bool MyArchMCInstLower::lowerLongBranch(const MachineInstr *MI,
                                      MCInst &OutMI) const {
  switch (MI->getOpcode()) {
  default:
    return false;
  case MyArch::LONG_BRANCH_LUi:
    lowerLongBranchLUi(MI, OutMI);
    return true;
  case MyArch::LONG_BRANCH_ADDiu:
    lowerLongBranchADDiu(MI, OutMI, MyArch::ADDiu,
                         MyArchMCExpr::CEK_ABS_LO);
    return true;
  }
}
#endif

void MyArchMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
#if CH >= CH8_2 //2
  if (lowerLongBranch(MI, OutMI))
    return;
#endif
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    MCOperand MCOp = LowerOperand(MO);

    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}

#endif // #if CH >= CH3_2

