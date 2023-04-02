#include "MyArch.h"
#include "llvm/IR/Module.h"
#include "TargetInfo/MyArchTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheMyArchTarget() {
  static Target TheMyArchTarget;
  return TheMyArchTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchTargetInfo() {
  RegisterTarget<Triple::myarch, false> X(getTheMyArchTarget(), "MyArch", "MyArch (32-bit MyArchulator arch)", "MyArch");
}
