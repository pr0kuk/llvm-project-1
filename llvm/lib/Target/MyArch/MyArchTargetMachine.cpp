//===-- MyArchTargetMachine.cpp - Define TargetMachine for MyArch -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about MyArch target spec.
//
//===----------------------------------------------------------------------===//

#include "MyArchTargetMachine.h"
#include "MyArch.h"

#if CH >= CH3_3 //0.5
#include "MyArchSEISelDAGToDAG.h"
#endif
#if CH >= CH3_1
#include "MyArchSubtarget.h"
#include "MyArchTargetObjectFile.h"
#endif
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "MyArch"

extern "C" void LLVMInitializeMyArchTarget() {
#if CH >= CH3_1
  // Register the target.
  //- Big endian Target Machine
  RegisterTargetMachine<MyArchebTargetMachine> X(TheMyArchTarget);
  //- Little endian Target Machine
  RegisterTargetMachine<MyArchelTargetMachine> Y(TheMyArchelTarget);
#endif
}

#if CH >= CH3_1

static std::string computeDataLayout(const Triple &TT, StringRef CPU,
                                     const TargetOptions &Options,
                                     bool isLittle) {
  std::string Ret = "";
  // There are both little and big endian MyArch.
  if (isLittle)
    Ret += "e";
  else
    Ret += "E";

  Ret += "-m:m";

  // Pointers are 32 bit on some ABIs.
  Ret += "-p:32:32";

  // 8 and 16 bit integers only need to have natural alignment, but try to
  // align them to 32 bits. 64 bit integers have natural alignment.
  Ret += "-i8:8:32-i16:16:32-i64:64";

  // 32 bit registers are always available and the stack is at least 64 bit
  // aligned.
  Ret += "-n32-S64";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(bool JIT,
                                           Optional<Reloc::Model> RM) {
  if (!RM.hasValue() || JIT)
    return Reloc::Static;
  return *RM;
}

// DataLayout --> Big-endian, 32-bit pointer/ABI/alignment
// The stack is always 8 byte aligned
// On function prologue, the stack is created by decrementing
// its pointer. Once decremented, all references are done with positive
// offset from the stack/frame pointer, using StackGrowsUp enables
// an easier handling.
// Using CodeModel::Large enables different CALL behavior.
MyArchTargetMachine::MyArchTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     Optional<Reloc::Model> RM,
                                     Optional<CodeModel::Model> CM,
                                     CodeGenOpt::Level OL, bool JIT,
                                     bool isLittle)
  //- Default is big endian
    : LLVMTargetMachine(T, computeDataLayout(TT, CPU, Options, isLittle), TT,
                        CPU, FS, Options, getEffectiveRelocModel(JIT, RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      isLittle(isLittle), TLOF(std::make_unique<MyArchTargetObjectFile>()),
      ABI(MyArchABIInfo::computeTargetABI()),
      DefaultSubtarget(TT, CPU, FS, isLittle, *this) {
  // initAsmInfo will display features by llc -march=MyArch -mcpu=help on 3.7 but
  // not on 3.6
  initAsmInfo();
}

MyArchTargetMachine::~MyArchTargetMachine() {}

void MyArchebTargetMachine::anchor() { }

MyArchebTargetMachine::MyArchebTargetMachine(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT)
    : MyArchTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, JIT, false) {}

void MyArchelTargetMachine::anchor() { }

MyArchelTargetMachine::MyArchelTargetMachine(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT)
    : MyArchTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, JIT, true) {}

const MyArchSubtarget *
MyArchTargetMachine::getSubtargetImpl(const Function &F) const {
  std::string CPU = TargetCPU;
  std::string FS = TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<MyArchSubtarget>(TargetTriple, CPU, FS, isLittle,
                                         *this);
  }
  return I.get();
}

namespace {
//@MyArchPassConfig {
/// MyArch Code Generator Pass Configuration Options.
class MyArchPassConfig : public TargetPassConfig {
public:
  MyArchPassConfig(MyArchTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  MyArchTargetMachine &getMyArchTargetMachine() const {
    return getTM<MyArchTargetMachine>();
  }

  const MyArchSubtarget &getMyArchSubtarget() const {
    return *getMyArchTargetMachine().getSubtargetImpl();
  }
#if CH >= CH12_1 //1
  void addIRPasses() override;
#endif
#if CH >= CH3_3 //1
  bool addInstSelector() override;
#endif
#if CH >= CH8_2 //1
  void addPreEmitPass() override;
#endif
#if CH >= CH9_3 //1
#ifdef ENABLE_GPRESTORE
  void addPreRegAlloc() override;
#endif
#endif //#if CH >= CH9_3 //1
};
} // namespace

TargetPassConfig *MyArchTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MyArchPassConfig(*this, PM);
}

#if CH >= CH12_1 //2
void MyArchPassConfig::addIRPasses() {
  TargetPassConfig::addIRPasses();
  addPass(createAtomicExpandPass());
}
#endif

#if CH >= CH3_3 //2
// Install an instruction selector pass using
// the ISelDag to gen MyArch code.
bool MyArchPassConfig::addInstSelector() {
  addPass(createMyArchSEISelDag(getMyArchTargetMachine(), getOptLevel()));
  return false;
}
#endif

#if CH >= CH9_3 //2
#ifdef ENABLE_GPRESTORE
void MyArchPassConfig::addPreRegAlloc() {
  if (!MyArchReserveGP) {
    // $gp is a caller-saved register.
    addPass(createMyArchEmitGPRestorePass(getMyArchTargetMachine()));
  }
  return;
}
#endif
#endif //#if CH >= CH9_3 //2

#if CH >= CH8_2 //2
// Implemented by targets that want to run passes immediately before
// machine code is emitted. return true if -print-machineinstrs should
// print out the code after the passes.
void MyArchPassConfig::addPreEmitPass() {
  MyArchTargetMachine &TM = getMyArchTargetMachine();
//@8_2 1{
  addPass(createMyArchDelJmpPass(TM));
//@8_2 1}
  addPass(createMyArchDelaySlotFillerPass(TM));
//@8_2 2}
  addPass(createMyArchLongBranchPass(TM));
  return;
}
#endif

#endif // #if CH >= CH3_1
