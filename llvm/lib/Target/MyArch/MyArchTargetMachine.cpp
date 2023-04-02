//===----------------------------------------------------------------------===//
//
// Implements the info about MyArch target spec.
//
//===----------------------------------------------------------------------===//

#include "MyArchTargetMachine.h"
#include "MyArch.h"
//#include "MyArchTargetTransformInfo.h"
#include "TargetInfo/MyArchTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetOptions.h"

#define DEBUG_TYPE "MyArch"

using namespace llvm;

static Reloc::Model getRelocModel(Optional<Reloc::Model> RM) {
  return RM.getValueOr(Reloc::Static);
}

/// MyArchTargetMachine ctor - Create an ILP32 Architecture model
MyArchTargetMachine::MyArchTargetMachine(const Target &T, const Triple &TT,
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

MyArchTargetMachine::~MyArchTargetMachine() = default;

namespace {

/// MyArch Code Generator Pass Configuration Options.
class MyArchPassConfig : public TargetPassConfig {
public:
  MyArchPassConfig(MyArchTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  MyArchTargetMachine &getMyArchTargetMachine() const {
    return getTM<MyArchTargetMachine>();
  }

  bool addInstSelector() override;
  // void addPreEmitPass() override;
  // void addPreRegAlloc() override;
};

} // end anonymous namespace

TargetPassConfig *MyArchTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MyArchPassConfig(*this, PM);
}

bool MyArchPassConfig::addInstSelector() {
  addPass(createMyArchISelDag(getMyArchTargetMachine(), getOptLevel()));
  return false;
}

// void MyArchPassConfig::addPreEmitPass() { llvm_unreachable(""); }

// void MyArchPassConfig::addPreRegAlloc() { llvm_unreachable(""); }

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchTarget() {
  RegisterTargetMachine<MyArchTargetMachine> X(getTheMyArchTarget());
}

#if 0
TargetTransformInfo
MyArchTargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(MyArchTTIImpl(this, F));
}
#endif
