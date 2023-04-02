#include "MyArchMCTargetDesc.h"
#include "TargetInfo/MyArchTargetInfo.h"
#include "MyArchInfo.h"
#include "MyArchInstPrinter.h"
#include "MyArchMCAsmInfo.h"
#include "MyArchTargetStreamer.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "MyArchGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "MyArchGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MyArchGenRegisterInfo.inc"

static MCInstrInfo *createMyArchMCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitMyArchMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createMyArchMCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  InitMyArchMCRegisterInfo(X, MyArch::R1);
  return X;
}

static MCSubtargetInfo *createMyArchMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  return createMyArchMCSubtargetInfoImpl(TT, CPU, /*TuneCPU=*/CPU, FS);
}

static MCAsmInfo *createMyArchMCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new MyArchMCAsmInfo(TT);
  MCRegister SP = MRI.getDwarfRegNum(MyArch::R2, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
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

MyArchTargetStreamer::MyArchTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}
MyArchTargetStreamer::~MyArchTargetStreamer() = default;

static MCTargetStreamer *createTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter *InstPrint,
                                                 bool isVerboseAsm) {
  return new MyArchTargetStreamer(S);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchTargetMC() {
  // Register the MC asm info.
  Target &TheMyArchTarget = getTheMyArchTarget();
  RegisterMCAsmInfoFn X(TheMyArchTarget, createMyArchMCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheMyArchTarget, createMyArchMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheMyArchTarget, createMyArchMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheMyArchTarget,
                                          createMyArchMCSubtargetInfo);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(TheMyArchTarget, createMyArchMCInstPrinter);

  TargetRegistry::RegisterAsmTargetStreamer(TheMyArchTarget,
                                            createTargetAsmStreamer);
}
