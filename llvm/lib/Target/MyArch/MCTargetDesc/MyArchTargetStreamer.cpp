//===-- MyArchTargetStreamer.cpp - MyArch Target Streamer Methods -----------===//
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

#include "MyArchTargetStreamer.h"
#include "MyArchMCTargetDesc.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MyArchAttributes.h"

using namespace llvm;

MyArchTargetStreamer::MyArchTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

void MyArchTargetStreamer::finish() { finishAttributeSection(); }

void MyArchTargetStreamer::emitDirectiveOptionPush() {}
void MyArchTargetStreamer::emitDirectiveOptionPop() {}
void MyArchTargetStreamer::emitDirectiveOptionPIC() {}
void MyArchTargetStreamer::emitDirectiveOptionNoPIC() {}
void MyArchTargetStreamer::emitDirectiveOptionRVC() {}
void MyArchTargetStreamer::emitDirectiveOptionNoRVC() {}
void MyArchTargetStreamer::emitDirectiveOptionRelax() {}
void MyArchTargetStreamer::emitDirectiveOptionNoRelax() {}
void MyArchTargetStreamer::emitAttribute(unsigned Attribute, unsigned Value) {}
void MyArchTargetStreamer::finishAttributeSection() {}
void MyArchTargetStreamer::emitTextAttribute(unsigned Attribute,
                                            StringRef String) {}
void MyArchTargetStreamer::emitIntTextAttribute(unsigned Attribute,
                                               unsigned IntValue,
                                               StringRef StringValue) {}

void MyArchTargetStreamer::emitTargetAttributes(const MCSubtargetInfo &STI) {
  if (STI.hasFeature(MyArch::FeatureRV32E))
    emitAttribute(MyArchAttrs::STACK_ALIGN, MyArchAttrs::ALIGN_4);
  else
    emitAttribute(MyArchAttrs::STACK_ALIGN, MyArchAttrs::ALIGN_16);

  std::string Arch = "rv32";
  if (STI.hasFeature(MyArch::Feature64Bit))
    Arch = "rv64";
  if (STI.hasFeature(MyArch::FeatureRV32E))
    Arch += "e1p9";
  else
    Arch += "i2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtM))
    Arch += "_m2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtA))
    Arch += "_a2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtF))
    Arch += "_f2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtD))
    Arch += "_d2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtC))
    Arch += "_c2p0";
  if (STI.hasFeature(MyArch::FeatureStdExtB))
    Arch += "_b0p93";
  if (STI.hasFeature(MyArch::FeatureStdExtV))
    Arch += "_v0p10";
  if (STI.hasFeature(MyArch::FeatureExtZfh))
    Arch += "_zfh0p1";
  if (STI.hasFeature(MyArch::FeatureExtZba))
    Arch += "_zba0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbb))
    Arch += "_zbb0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbc))
    Arch += "_zbc0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbe))
    Arch += "_zbe0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbf))
    Arch += "_zbf0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbm))
    Arch += "_zbm0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbp))
    Arch += "_zbp0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbproposedc))
    Arch += "_zbproposedc0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbr))
    Arch += "_zbr0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbs))
    Arch += "_zbs0p93";
  if (STI.hasFeature(MyArch::FeatureExtZbt))
    Arch += "_zbt0p93";
  if (STI.hasFeature(MyArch::FeatureExtZvamo))
    Arch += "_zvamo0p10";
  if (STI.hasFeature(MyArch::FeatureStdExtZvlsseg))
    Arch += "_zvlsseg0p10";

  emitTextAttribute(MyArchAttrs::ARCH, Arch);
}

// This part is for ascii assembly output
MyArchTargetAsmStreamer::MyArchTargetAsmStreamer(MCStreamer &S,
                                               formatted_raw_ostream &OS)
    : MyArchTargetStreamer(S), OS(OS) {}

void MyArchTargetAsmStreamer::emitDirectiveOptionPush() {
  OS << "\t.option\tpush\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionPop() {
  OS << "\t.option\tpop\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionPIC() {
  OS << "\t.option\tpic\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionNoPIC() {
  OS << "\t.option\tnopic\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionRVC() {
  OS << "\t.option\trvc\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionNoRVC() {
  OS << "\t.option\tnorvc\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionRelax() {
  OS << "\t.option\trelax\n";
}

void MyArchTargetAsmStreamer::emitDirectiveOptionNoRelax() {
  OS << "\t.option\tnorelax\n";
}

void MyArchTargetAsmStreamer::emitAttribute(unsigned Attribute, unsigned Value) {
  OS << "\t.attribute\t" << Attribute << ", " << Twine(Value) << "\n";
}

void MyArchTargetAsmStreamer::emitTextAttribute(unsigned Attribute,
                                               StringRef String) {
  OS << "\t.attribute\t" << Attribute << ", \"" << String << "\"\n";
}

void MyArchTargetAsmStreamer::emitIntTextAttribute(unsigned Attribute,
                                                  unsigned IntValue,
                                                  StringRef StringValue) {}

void MyArchTargetAsmStreamer::finishAttributeSection() {}
