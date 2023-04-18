//===-- MyArchMCTargetDesc.cpp - MyArch Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// This file provides MyArch-specific target descriptions.
///
//===----------------------------------------------------------------------===//

#include "MyArchMCTargetDesc.h"
#include "MyArchBaseInfo.h"
#include "MyArchELFStreamer.h"
#include "MyArchInstPrinter.h"
#include "MyArchMCAsmInfo.h"
#include "MyArchTargetStreamer.h"
#include "TargetInfo/MyArchTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "MyArchGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MyArchGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "MyArchGenSubtargetInfo.inc"

using namespace llvm;

static MCInstrInfo *createMyArchMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMyArchMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createMyArchMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMyArchMCRegisterInfo(X, MyArch::X1);
  return X;
}

static MCAsmInfo *createMyArchMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new MyArchMCAsmInfo(TT);

  MCRegister SP = MRI.getDwarfRegNum(MyArch::X2, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCSubtargetInfo *createMyArchMCSubtargetInfo(const Triple &TT,
                                                   StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = TT.isArch64Bit() ? "generic-rv64" : "generic-rv32";
  return createMyArchMCSubtargetInfoImpl(TT, CPUName, /*TuneCPU*/ CPUName, FS);
}

static MCInstPrinter *createMyArchMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new MyArchInstPrinter(MAI, MII, MRI);
}

static MCTargetStreamer *
createMyArchObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  const Triple &TT = STI.getTargetTriple();
  if (TT.isOSBinFormatELF())
    return new MyArchTargetELFStreamer(S, STI);
  return nullptr;
}

static MCTargetStreamer *createMyArchAsmTargetStreamer(MCStreamer &S,
                                                      formatted_raw_ostream &OS,
                                                      MCInstPrinter *InstPrint,
                                                      bool isVerboseAsm) {
  return new MyArchTargetAsmStreamer(S, OS);
}

static MCTargetStreamer *createMyArchNullTargetStreamer(MCStreamer &S) {
  return new MyArchTargetStreamer(S);
}

namespace {

class MyArchMCInstrAnalysis : public MCInstrAnalysis {
public:
  explicit MyArchMCInstrAnalysis(const MCInstrInfo *Info)
      : MCInstrAnalysis(Info) {}

  bool evaluateBranch(const MCInst &Inst, uint64_t Addr, uint64_t Size,
                      uint64_t &Target) const override {
    if (isConditionalBranch(Inst)) {
      int64_t Imm;
      if (Size == 2)
        Imm = Inst.getOperand(1).getImm();
      else
        Imm = Inst.getOperand(2).getImm();
      Target = Addr + Imm;
      return true;
    }

    if (Inst.getOpcode() == MyArch::C_JAL || Inst.getOpcode() == MyArch::C_J) {
      Target = Addr + Inst.getOperand(0).getImm();
      return true;
    }

    if (Inst.getOpcode() == MyArch::JAL) {
      Target = Addr + Inst.getOperand(1).getImm();
      return true;
    }

    return false;
  }
};

} // end anonymous namespace

static MCInstrAnalysis *createMyArchInstrAnalysis(const MCInstrInfo *Info) {
  return new MyArchMCInstrAnalysis(Info);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchTargetMC() {
  for (Target *T : {/*&getTheMyArch32Target(), &getTheMyArch64Target()*/&getTheMyArchTarget()}) {
    TargetRegistry::RegisterMCAsmInfo(*T, createMyArchMCAsmInfo);
    TargetRegistry::RegisterMCInstrInfo(*T, createMyArchMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(*T, createMyArchMCRegisterInfo);
    TargetRegistry::RegisterMCAsmBackend(*T, createMyArchAsmBackend);
    TargetRegistry::RegisterMCCodeEmitter(*T, createMyArchMCCodeEmitter);
    TargetRegistry::RegisterMCInstPrinter(*T, createMyArchMCInstPrinter);
    TargetRegistry::RegisterMCSubtargetInfo(*T, createMyArchMCSubtargetInfo);
    TargetRegistry::RegisterObjectTargetStreamer(
        *T, createMyArchObjectTargetStreamer);
    TargetRegistry::RegisterMCInstrAnalysis(*T, createMyArchInstrAnalysis);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(*T, createMyArchAsmTargetStreamer);
    // Register the null target streamer.
    TargetRegistry::RegisterNullTargetStreamer(*T,
                                               createMyArchNullTargetStreamer);
  }
}
