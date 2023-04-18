//===-- MyArchELFStreamer.cpp - MyArch ELF Target Streamer Methods ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides MyArch specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "MyArchELFStreamer.h"
#include "MyArchAsmBackend.h"
#include "MyArchBaseInfo.h"
#include "MyArchMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/MyArchAttributes.h"

using namespace llvm;

// This part is for ELF object output.
MyArchTargetELFStreamer::MyArchTargetELFStreamer(MCStreamer &S,
                                               const MCSubtargetInfo &STI)
    : MyArchTargetStreamer(S), CurrentVendor("myarch") {
  MCAssembler &MCA = getStreamer().getAssembler();
  const FeatureBitset &Features = STI.getFeatureBits();
  auto &MAB = static_cast<MyArchAsmBackend &>(MCA.getBackend());
  MyArchABI::ABI ABI = MAB.getTargetABI();
  assert(ABI != MyArchABI::ABI_Unknown && "Improperly initialised target ABI");

  unsigned EFlags = MCA.getELFHeaderEFlags();

  if (Features[MyArch::FeatureStdExtC])
    EFlags |= ELF::EF_MyArch_RVC;

  switch (ABI) {
  case MyArchABI::ABI_ILP32:
  case MyArchABI::ABI_LP64:
    break;
  case MyArchABI::ABI_ILP32F:
  case MyArchABI::ABI_LP64F:
    EFlags |= ELF::EF_MyArch_FLOAT_ABI_SINGLE;
    break;
  case MyArchABI::ABI_ILP32D:
  case MyArchABI::ABI_LP64D:
    EFlags |= ELF::EF_MyArch_FLOAT_ABI_DOUBLE;
    break;
  case MyArchABI::ABI_ILP32E:
    EFlags |= ELF::EF_MyArch_RVE;
    break;
  case MyArchABI::ABI_Unknown:
    llvm_unreachable("Improperly initialised target ABI");
  }

  MCA.setELFHeaderEFlags(EFlags);
}

MCELFStreamer &MyArchTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer &>(Streamer);
}

void MyArchTargetELFStreamer::emitDirectiveOptionPush() {}
void MyArchTargetELFStreamer::emitDirectiveOptionPop() {}
void MyArchTargetELFStreamer::emitDirectiveOptionPIC() {}
void MyArchTargetELFStreamer::emitDirectiveOptionNoPIC() {}
void MyArchTargetELFStreamer::emitDirectiveOptionRVC() {}
void MyArchTargetELFStreamer::emitDirectiveOptionNoRVC() {}
void MyArchTargetELFStreamer::emitDirectiveOptionRelax() {}
void MyArchTargetELFStreamer::emitDirectiveOptionNoRelax() {}

void MyArchTargetELFStreamer::emitAttribute(unsigned Attribute, unsigned Value) {
  setAttributeItem(Attribute, Value, /*OverwriteExisting=*/true);
}

void MyArchTargetELFStreamer::emitTextAttribute(unsigned Attribute,
                                               StringRef String) {
  setAttributeItem(Attribute, String, /*OverwriteExisting=*/true);
}

void MyArchTargetELFStreamer::emitIntTextAttribute(unsigned Attribute,
                                                  unsigned IntValue,
                                                  StringRef StringValue) {
  setAttributeItems(Attribute, IntValue, StringValue,
                    /*OverwriteExisting=*/true);
}

void MyArchTargetELFStreamer::finishAttributeSection() {
  if (Contents.empty())
    return;

  if (AttributeSection) {
    Streamer.SwitchSection(AttributeSection);
  } else {
    MCAssembler &MCA = getStreamer().getAssembler();
    AttributeSection = MCA.getContext().getELFSection(
        ".myarch.attributes", ELF::SHT_MyArch_ATTRIBUTES, 0);
    Streamer.SwitchSection(AttributeSection);

    Streamer.emitInt8(ELFAttrs::Format_Version);
  }

  // Vendor size + Vendor name + '\0'
  const size_t VendorHeaderSize = 4 + CurrentVendor.size() + 1;

  // Tag + Tag Size
  const size_t TagHeaderSize = 1 + 4;

  const size_t ContentsSize = calculateContentSize();

  Streamer.emitInt32(VendorHeaderSize + TagHeaderSize + ContentsSize);
  Streamer.emitBytes(CurrentVendor);
  Streamer.emitInt8(0); // '\0'

  Streamer.emitInt8(ELFAttrs::File);
  Streamer.emitInt32(TagHeaderSize + ContentsSize);

  // Size should have been accounted for already, now
  // emit each field as its type (ULEB or String).
  for (AttributeItem item : Contents) {
    Streamer.emitULEB128IntValue(item.Tag);
    switch (item.Type) {
    default:
      llvm_unreachable("Invalid attribute type");
    case AttributeType::Numeric:
      Streamer.emitULEB128IntValue(item.IntValue);
      break;
    case AttributeType::Text:
      Streamer.emitBytes(item.StringValue);
      Streamer.emitInt8(0); // '\0'
      break;
    case AttributeType::NumericAndText:
      Streamer.emitULEB128IntValue(item.IntValue);
      Streamer.emitBytes(item.StringValue);
      Streamer.emitInt8(0); // '\0'
      break;
    }
  }

  Contents.clear();
}

size_t MyArchTargetELFStreamer::calculateContentSize() const {
  size_t Result = 0;
  for (AttributeItem item : Contents) {
    switch (item.Type) {
    case AttributeType::Hidden:
      break;
    case AttributeType::Numeric:
      Result += getULEB128Size(item.Tag);
      Result += getULEB128Size(item.IntValue);
      break;
    case AttributeType::Text:
      Result += getULEB128Size(item.Tag);
      Result += item.StringValue.size() + 1; // string + '\0'
      break;
    case AttributeType::NumericAndText:
      Result += getULEB128Size(item.Tag);
      Result += getULEB128Size(item.IntValue);
      Result += item.StringValue.size() + 1; // string + '\0';
      break;
    }
  }
  return Result;
}
