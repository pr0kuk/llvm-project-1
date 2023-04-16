//===-- MyArchMCInstLower.h - Lower MachineInstr to MCInst -------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArchMCINSTLOWER_H
#define LLVM_LIB_TARGET_MyArch_MyArchMCINSTLOWER_H

//#include "MyArchConfig.h"

#if CH >= CH5_1
#include "MCTargetDesc/MyArchMCExpr.h"
#endif
#if CH >= CH3_2
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
  class MCContext;
  class MCInst;
  class MCOperand;
  class MachineInstr;
  class MachineFunction;
  class MyArchAsmPrinter;

//@1 {
/// This class is used to lower an MachineInstr into an MCInst.
class LLVM_LIBRARY_VISIBILITY MyArchMCInstLower {
//@2
  typedef MachineOperand::MachineOperandType MachineOperandType;
  MCContext *Ctx;
  MyArchAsmPrinter &AsmPrinter;
public:
  MyArchMCInstLower(MyArchAsmPrinter &asmprinter);
  void Initialize(MCContext* C);
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;
  MCOperand LowerOperand(const MachineOperand& MO, unsigned offset = 0) const;
#if CH >= CH6_1 //1
  void LowerCPLOAD(SmallVector<MCInst, 4>& MCInsts);
#endif
#if CH >= CH9_3
#ifdef ENABLE_GPRESTORE
  void LowerCPRESTORE(int64_t Offset, SmallVector<MCInst, 4>& MCInsts);
#endif
#endif //#if CH >= CH9_3
#if CH >= CH6_1 //2
private:
  MCOperand LowerSymbolOperand(const MachineOperand &MO,
                               MachineOperandType MOTy, unsigned Offset) const;
#endif
#if CH >= CH8_2 //1
  MCOperand createSub(MachineBasicBlock *BB1, MachineBasicBlock *BB2,
                      MyArchMCExpr::MyArchExprKind Kind) const;
  void lowerLongBranchLUi(const MachineInstr *MI, MCInst &OutMI) const;
  void lowerLongBranchADDiu(const MachineInstr *MI, MCInst &OutMI,
                            int Opcode,
                            MyArchMCExpr::MyArchExprKind Kind) const;
  bool lowerLongBranch(const MachineInstr *MI, MCInst &OutMI) const;
#endif
};
}

#endif // #if CH >= CH3_2

#endif

