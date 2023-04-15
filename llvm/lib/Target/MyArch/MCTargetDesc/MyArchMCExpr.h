//===-- MyArchMCExpr.h - MyArch specific MC expression classes ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCEXPR_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCEXPR_H

//#include "MyArchConfig.h"
#if CH >= CH5_1

#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

namespace llvm {

class MyArchMCExpr : public MCTargetExpr {
public:
  enum MyArchExprKind {
    CEK_None,
    CEK_ABS_HI,
    CEK_ABS_LO,
    CEK_CALL_HI16,
    CEK_CALL_LO16,
    CEK_DTP_HI,
    CEK_DTP_LO,
    CEK_GOT,
    CEK_GOTTPREL,
    CEK_GOT_CALL,
    CEK_GOT_DISP,
    CEK_GOT_HI16,
    CEK_GOT_LO16,
    CEK_GPREL,
    CEK_TLSGD,
    CEK_TLSLDM,
    CEK_TP_HI,
    CEK_TP_LO,
    CEK_Special,
  };

private:
  const MyArchExprKind Kind;
  const MCExpr *Expr;

  explicit MyArchMCExpr(MyArchExprKind Kind, const MCExpr *Expr)
    : Kind(Kind), Expr(Expr) {}

public:
  static const MyArchMCExpr *create(MyArchExprKind Kind, const MCExpr *Expr,
                                  MCContext &Ctx);
  static const MyArchMCExpr *create(const MCSymbol *Symbol, 
                                  MyArchMCExpr::MyArchExprKind Kind, MCContext &Ctx);
  static const MyArchMCExpr *createGpOff(MyArchExprKind Kind, const MCExpr *Expr,
                                       MCContext &Ctx);

  /// Get the kind of this expression.
  MyArchExprKind getKind() const { return Kind; }

  /// Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override {
    return getSubExpr()->findAssociatedFragment();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override;

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  bool isGpOff(MyArchExprKind &Kind) const;
  bool isGpOff() const {
    MyArchExprKind Kind;
    return isGpOff(Kind);
  }
};
} // end namespace llvm

#endif // #if CH >= CH5_1

#endif

