//===-- MyArchTargetStreamer.cpp - MyArch Target Streamer Methods -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides MyArch specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "MyArchInstPrinter.h"
#include "MyArchMCTargetDesc.h"
// #include "MyArchTargetObjectFile.h"
#include "MyArchTargetStreamer.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

#if CH >= CH5_1

using namespace llvm;

MyArchTargetStreamer::MyArchTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {
}

MyArchTargetAsmStreamer::MyArchTargetAsmStreamer(MCStreamer &S,
                                             formatted_raw_ostream &OS)
    : MyArchTargetStreamer(S), OS(OS) {}

#endif // #if CH >= CH5_1
