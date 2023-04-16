//===-- MyArchMCTargetDesc.cpp - MyArch Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides MyArch specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "MyArchMCTargetDesc.h"
#if CH >= CH3_2 //1
#include "MyArchInstPrinter.h"
#include "MyArchMCAsmInfo.h"
#endif
#if CH >= CH5_1
#include "MyArchTargetStreamer.h"
#endif
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "MyArchGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "MyArchGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MyArchGenRegisterInfo.inc"

#if CH >= CH3_2 //2
//@1 {
/// Select the MyArch Architecture Feature for the given triple and cpu name.
/// The function will be called at command 'llvm-objdump -d' for MyArch elf input.
static std::string selectMyArchArchFeature(const Triple &TT, StringRef CPU) {
  std::string MyArchArchFeature;
  if (CPU.empty() || CPU == "generic") {
    if (TT.getArch() == Triple::myarch) {
      if (CPU.empty() || CPU == "MyArch32II") {
        MyArchArchFeature = "+MyArch32II";
      }
      else {
        if (CPU == "MyArch32I") {
          MyArchArchFeature = "+MyArch32I";
        }
      }
    }
  }
  return MyArchArchFeature;
}
//@1 }

static MCInstrInfo *createMyArchMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMyArchMCInstrInfo(X); // defined in MyArchGenInstrInfo.inc
  return X;
}

static MCRegisterInfo *createMyArchMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMyArchMCRegisterInfo(X, MyArch::SW); // defined in MyArchGenRegisterInfo.inc
  return X;
}

static MCSubtargetInfo *createMyArchMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  std::string ArchFS = selectMyArchArchFeature(TT,CPU);
  if (!FS.empty()) {
    if (!ArchFS.empty())
      ArchFS = ArchFS + "," + FS.str();
    else
      ArchFS = FS.str();
  }
  return createMyArchMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, ArchFS);
// createMyArchMCSubtargetInfoImpl defined in MyArchGenSubtargetInfo.inc
}

static MCAsmInfo *createMyArchMCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new MyArchMCAsmInfo(TT);

  unsigned SP = MRI.getDwarfRegNum(MyArch::SP, true);
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfaRegister(nullptr, SP);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createMyArchMCInstPrinter(const Triple &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  return new MyArchInstPrinter(MAI, MII, MRI);
}

namespace {

class MyArchMCInstrAnalysis : public MCInstrAnalysis {
public:
  MyArchMCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}
};
}

static MCInstrAnalysis *createMyArchMCInstrAnalysis(const MCInstrInfo *Info) {
  return new MyArchMCInstrAnalysis(Info);
}


#endif

#if CH >= CH5_1 //1
static MCStreamer *createMCStreamer(const Triple &TT, MCContext &Context,
                                    std::unique_ptr<MCAsmBackend> &&MAB,
                                    std::unique_ptr<MCObjectWriter> &&OW,
                                    std::unique_ptr<MCCodeEmitter> &&Emitter,
                                    bool RelaxAll) {
  return createELFStreamer(Context, std::move(MAB), std::move(OW),
                           std::move(Emitter), RelaxAll);;
}

static MCTargetStreamer *createMyArchAsmTargetStreamer(MCStreamer &S,
                                                     formatted_raw_ostream &OS,
                                                     MCInstPrinter *InstPrint,
                                                     bool isVerboseAsm) {
  return new MyArchTargetAsmStreamer(S, OS);
}
#endif

//@2 {
extern "C" void LLVMInitializeMyArchTargetMC() {
#if CH >= CH3_2 //3
  // for (Target *T : {&TheMyArchTarget, &TheMyArchelTarget}) {
    // Register the MC asm info.
    RegisterMCAsmInfoFn X(TheMyArchTarget, createMyArchMCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(TheMyArchTarget, createMyArchMCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(TheMyArchTarget, createMyArchMCRegisterInfo);

#if CH >= CH5_1 //2
     // Register the elf streamer.
    TargetRegistry::RegisterELFStreamer(TheMyArchTarget, createMCStreamer);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(TheMyArchTarget, createMyArchAsmTargetStreamer);

    // Register the asm backend.
    TargetRegistry::RegisterMCAsmBackend(TheMyArchTarget, createMyArchAsmBackend);
#endif

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(TheMyArchTarget,
	                                        createMyArchMCSubtargetInfo);
    // Register the MC instruction analyzer.
    TargetRegistry::RegisterMCInstrAnalysis(TheMyArchTarget, createMyArchMCInstrAnalysis);
    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(TheMyArchTarget,
	                                      createMyArchMCInstPrinter);
  // }
#endif // #if CH >= CH3_2

#if CH >= CH5_1 //3
  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(TheMyArchTarget,
                                        createMyArchMCCodeEmitterEB);
  // TargetRegistry::RegisterMCCodeEmitter(TheMyArchelTarget,
                                        // createMyArchMCCodeEmitterEL);

#endif
}
//@2 }
