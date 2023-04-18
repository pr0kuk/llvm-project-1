//===-- MyArchAttributeParser.h - MyArch Attribute Parser ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_MyArchATTRIBUTEPARSER_H
#define LLVM_SUPPORT_MyArchATTRIBUTEPARSER_H

#include "llvm/Support/ELFAttributeParser.h"
#include "llvm/Support/MyArchAttributes.h"

namespace llvm {
class MyArchAttributeParser : public ELFAttributeParser {
  struct DisplayHandler {
    MyArchAttrs::AttrType attribute;
    Error (MyArchAttributeParser::*routine)(unsigned);
  };
  static const DisplayHandler displayRoutines[];

  Error handler(uint64_t tag, bool &handled) override;

  Error unalignedAccess(unsigned tag);
  Error stackAlign(unsigned tag);

public:
  MyArchAttributeParser(ScopedPrinter *sw)
      : ELFAttributeParser(sw, MyArchAttrs::MyArchAttributeTags, "riscv") {}
  MyArchAttributeParser()
      : ELFAttributeParser(MyArchAttrs::MyArchAttributeTags, "riscv") {}
};

} // namespace llvm

#endif
