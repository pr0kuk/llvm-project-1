//===-- MyArchAttributeParser.cpp - MyArch Attribute Parser -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/MyArchAttributeParser.h"
#include "llvm/ADT/StringExtras.h"

using namespace llvm;

const MyArchAttributeParser::DisplayHandler
    MyArchAttributeParser::displayRoutines[] = {
        {
            MyArchAttrs::ARCH,
            &ELFAttributeParser::stringAttribute,
        },
        {
            MyArchAttrs::PRIV_SPEC,
            &ELFAttributeParser::integerAttribute,
        },
        {
            MyArchAttrs::PRIV_SPEC_MINOR,
            &ELFAttributeParser::integerAttribute,
        },
        {
            MyArchAttrs::PRIV_SPEC_REVISION,
            &ELFAttributeParser::integerAttribute,
        },
        {
            MyArchAttrs::STACK_ALIGN,
            &MyArchAttributeParser::stackAlign,
        },
        {
            MyArchAttrs::UNALIGNED_ACCESS,
            &MyArchAttributeParser::unalignedAccess,
        }};

Error MyArchAttributeParser::unalignedAccess(unsigned tag) {
  static const char *strings[] = {"No unaligned access", "Unaligned access"};
  return parseStringAttribute("Unaligned_access", tag, makeArrayRef(strings));
}

Error MyArchAttributeParser::stackAlign(unsigned tag) {
  uint64_t value = de.getULEB128(cursor);
  std::string description =
      "Stack alignment is " + utostr(value) + std::string("-bytes");
  printAttribute(tag, value, description);
  return Error::success();
}

Error MyArchAttributeParser::handler(uint64_t tag, bool &handled) {
  handled = false;
  for (unsigned AHI = 0, AHE = array_lengthof(displayRoutines); AHI != AHE;
       ++AHI) {
    if (uint64_t(displayRoutines[AHI].attribute) == tag) {
      if (Error e = (this->*displayRoutines[AHI].routine)(tag))
        return e;
      handled = true;
      break;
    }
  }

  return Error::success();
}
