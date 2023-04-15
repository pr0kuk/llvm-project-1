//===-- MyArchTargetStreamer.h - MyArch Target Streamer ------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArchTARGETSTREAMER_H
#define LLVM_LIB_TARGET_MyArch_MyArchTARGETSTREAMER_H

// #include "MyArchConfig.h"
#if CH >= CH5_1

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {

class MyArchTargetStreamer : public MCTargetStreamer {
public:
  MyArchTargetStreamer(MCStreamer &S);
};

// This part is for ascii assembly output
class MyArchTargetAsmStreamer : public MyArchTargetStreamer {
  formatted_raw_ostream &OS;

public:
  MyArchTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
};

}

#endif // #if CH >= CH5_1
#endif