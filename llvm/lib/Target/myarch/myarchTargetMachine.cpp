//===----------------------------------------------------------------------===//
//
// Implements the info about myarch target spec.
//
//===----------------------------------------------------------------------===//

#include "myarchTargetMachine.h"
#include "myarch.h"
//#include "myarchTargetTransformInfo.h"
#include "TargetInfo/myarchTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetOptions.h"

#define DEBUG_TYPE "myarch"

using namespace llvm;

static Reloc::Model getRelocModel(Optional<Reloc::Model> RM) {
  return RM.getValueOr(Reloc::Static);
}

/// myarchTargetMachine ctor - Create an ILP32 Architecture model
myarchTargetMachine::myarchTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     Optional<Reloc::Model> RM,
                                     Optional<CodeModel::Model> CM,
                                     CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T,
                        "e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32-"
                        "f32:32:32-i64:32-f64:32-a:0:32-n32",
                        TT, CPU, FS, Options, getRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

myarchTargetMachine::~myarchTargetMachine() = default;

namespace {

/// myarch Code Generator Pass Configuration Options.
class myarchPassConfig : public TargetPassConfig {
public:
  myarchPassConfig(myarchTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  myarchTargetMachine &getmyarchTargetMachine() const {
    return getTM<myarchTargetMachine>();
  }

  bool addInstSelector() override;
  // void addPreEmitPass() override;
  // void addPreRegAlloc() override;
};

} // end anonymous namespace

TargetPassConfig *myarchTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new myarchPassConfig(*this, PM);
}

bool myarchPassConfig::addInstSelector() {
  addPass(createmyarchISelDag(getmyarchTargetMachine(), getOptLevel()));
  return false;
}

// void myarchPassConfig::addPreEmitPass() { llvm_unreachable(""); }

// void myarchPassConfig::addPreRegAlloc() { llvm_unreachable(""); }

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializemyarchTarget() {
  RegisterTargetMachine<myarchTargetMachine> X(getThemyarchTarget());
}

#if 0
TargetTransformInfo
myarchTargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(myarchTTIImpl(this, F));
}
#endif