#include "myarchMCTargetDesc.h"
#include "TargetInfo/myarchTargetInfo.h"
#include "myarchInfo.h"
#include "myarchInstPrinter.h"
#include "myarchMCAsmInfo.h"
#include "myarchTargetStreamer.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "myarchGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "myarchGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "myarchGenRegisterInfo.inc"

static MCInstrInfo *createmyarchMCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitmyarchMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createmyarchMCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  InitmyarchMCRegisterInfo(X, myarch::R1);
  return X;
}

static MCSubtargetInfo *createmyarchMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  return createmyarchMCSubtargetInfoImpl(TT, CPU, /*TuneCPU=*/CPU, FS);
}

static MCAsmInfo *createmyarchMCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new myarchMCAsmInfo(TT);
  MCRegister SP = MRI.getDwarfRegNum(myarch::R2, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);
  return MAI;
}

static MCInstPrinter *createmyarchMCInstPrinter(const Triple &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  return new myarchInstPrinter(MAI, MII, MRI);
}

myarchTargetStreamer::myarchTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}
myarchTargetStreamer::~myarchTargetStreamer() = default;

static MCTargetStreamer *createTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter *InstPrint,
                                                 bool isVerboseAsm) {
  return new myarchTargetStreamer(S);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializemyarchTargetMC() {
  // Register the MC asm info.
  Target &ThemyarchTarget = getThemyarchTarget();
  RegisterMCAsmInfoFn X(ThemyarchTarget, createmyarchMCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(ThemyarchTarget, createmyarchMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(ThemyarchTarget, createmyarchMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(ThemyarchTarget,
                                          createmyarchMCSubtargetInfo);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(ThemyarchTarget, createmyarchMCInstPrinter);

  TargetRegistry::RegisterAsmTargetStreamer(ThemyarchTarget,
                                            createTargetAsmStreamer);
}
