//===-- MyArchELFObjectWriter.cpp - MyArch ELF Writer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MyArchFixupKinds.h"
#include "MCTargetDesc/MyArchMCExpr.h"
#include "MCTargetDesc/MyArchMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class MyArchELFObjectWriter : public MCELFObjectTargetWriter {
public:
  MyArchELFObjectWriter(uint8_t OSABI, bool Is64Bit);

  ~MyArchELFObjectWriter() override;

  // Return true if the given relocation must be with a symbol rather than
  // section plus offset.
  bool needsRelocateWithSymbol(const MCSymbol &Sym,
                               unsigned Type) const override {
    // TODO: this is very conservative, update once RISC-V psABI requirements
    //       are clarified.
    return true;
  }

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
};
}

MyArchELFObjectWriter::MyArchELFObjectWriter(uint8_t OSABI, bool Is64Bit)
    : MCELFObjectTargetWriter(Is64Bit, OSABI, ELF::EM_MyArch,
                              /*HasRelocationAddend*/ true) {}

MyArchELFObjectWriter::~MyArchELFObjectWriter() {}

unsigned MyArchELFObjectWriter::getRelocType(MCContext &Ctx,
                                            const MCValue &Target,
                                            const MCFixup &Fixup,
                                            bool IsPCRel) const {
  const MCExpr *Expr = Fixup.getValue();
  // Determine the type of the relocation
  unsigned Kind = Fixup.getTargetKind();
  if (Kind >= FirstLiteralRelocationKind)
    return Kind - FirstLiteralRelocationKind;
  if (IsPCRel) {
    switch (Kind) {
    default:
      Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
      return ELF::R_MyArch_NONE;
    case FK_Data_4:
    case FK_PCRel_4:
      return ELF::R_MyArch_32_PCREL;
    case MyArch::fixup_myarch_pcrel_hi20:
      return ELF::R_MyArch_PCREL_HI20;
    case MyArch::fixup_myarch_pcrel_lo12_i:
      return ELF::R_MyArch_PCREL_LO12_I;
    case MyArch::fixup_myarch_pcrel_lo12_s:
      return ELF::R_MyArch_PCREL_LO12_S;
    case MyArch::fixup_myarch_got_hi20:
      return ELF::R_MyArch_GOT_HI20;
    case MyArch::fixup_myarch_tls_got_hi20:
      return ELF::R_MyArch_TLS_GOT_HI20;
    case MyArch::fixup_myarch_tls_gd_hi20:
      return ELF::R_MyArch_TLS_GD_HI20;
    case MyArch::fixup_myarch_jal:
      return ELF::R_MyArch_JAL;
    case MyArch::fixup_myarch_branch:
      return ELF::R_MyArch_BRANCH;
    case MyArch::fixup_myarch_rvc_jump:
      return ELF::R_MyArch_RVC_JUMP;
    case MyArch::fixup_myarch_rvc_branch:
      return ELF::R_MyArch_RVC_BRANCH;
    case MyArch::fixup_myarch_call:
      return ELF::R_MyArch_CALL;
    case MyArch::fixup_myarch_call_plt:
      return ELF::R_MyArch_CALL_PLT;
    }
  }

  switch (Kind) {
  default:
    Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
    return ELF::R_MyArch_NONE;
  case FK_Data_1:
    Ctx.reportError(Fixup.getLoc(), "1-byte data relocations not supported");
    return ELF::R_MyArch_NONE;
  case FK_Data_2:
    Ctx.reportError(Fixup.getLoc(), "2-byte data relocations not supported");
    return ELF::R_MyArch_NONE;
  case FK_Data_4:
    if (Expr->getKind() == MCExpr::Target &&
        cast<MyArchMCExpr>(Expr)->getKind() == MyArchMCExpr::VK_MyArch_32_PCREL)
      return ELF::R_MyArch_32_PCREL;
    return ELF::R_MyArch_32;
  case FK_Data_8:
    return ELF::R_MyArch_64;
  case FK_Data_Add_1:
    return ELF::R_MyArch_ADD8;
  case FK_Data_Add_2:
    return ELF::R_MyArch_ADD16;
  case FK_Data_Add_4:
    return ELF::R_MyArch_ADD32;
  case FK_Data_Add_8:
    return ELF::R_MyArch_ADD64;
  case FK_Data_Add_6b:
    return ELF::R_MyArch_SET6;
  case FK_Data_Sub_1:
    return ELF::R_MyArch_SUB8;
  case FK_Data_Sub_2:
    return ELF::R_MyArch_SUB16;
  case FK_Data_Sub_4:
    return ELF::R_MyArch_SUB32;
  case FK_Data_Sub_8:
    return ELF::R_MyArch_SUB64;
  case FK_Data_Sub_6b:
    return ELF::R_MyArch_SUB6;
  case MyArch::fixup_myarch_hi20:
    return ELF::R_MyArch_HI20;
  case MyArch::fixup_myarch_lo12_i:
    return ELF::R_MyArch_LO12_I;
  case MyArch::fixup_myarch_lo12_s:
    return ELF::R_MyArch_LO12_S;
  case MyArch::fixup_myarch_tprel_hi20:
    return ELF::R_MyArch_TPREL_HI20;
  case MyArch::fixup_myarch_tprel_lo12_i:
    return ELF::R_MyArch_TPREL_LO12_I;
  case MyArch::fixup_myarch_tprel_lo12_s:
    return ELF::R_MyArch_TPREL_LO12_S;
  case MyArch::fixup_myarch_tprel_add:
    return ELF::R_MyArch_TPREL_ADD;
  case MyArch::fixup_myarch_relax:
    return ELF::R_MyArch_RELAX;
  case MyArch::fixup_myarch_align:
    return ELF::R_MyArch_ALIGN;
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createMyArchELFObjectWriter(uint8_t OSABI, bool Is64Bit) {
  return std::make_unique<MyArchELFObjectWriter>(OSABI, Is64Bit);
}
