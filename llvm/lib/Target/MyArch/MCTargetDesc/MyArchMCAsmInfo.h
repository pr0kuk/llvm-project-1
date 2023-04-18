//===-- MyArchMCAsmInfo.h - MyArch Asm Info ----------------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the MyArchMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCASMINFO_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class MyArchMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit MyArchMCAsmInfo(const Triple &TargetTriple);

  const MCExpr *getExprForFDESymbol(const MCSymbol *Sym, unsigned Encoding,
                                    MCStreamer &Streamer) const override;
};

} // namespace llvm

#endif
