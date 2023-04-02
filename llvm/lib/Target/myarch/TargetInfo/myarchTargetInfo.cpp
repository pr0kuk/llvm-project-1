#include "myarch.h"
#include "llvm/IR/Module.h"
#include "TargetInfo/myarchTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getThemyarchTarget() {
  static Target ThemyarchTarget;
  return ThemyarchTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializemyarchTargetInfo() {
  RegisterTarget<Triple::myarch, false> X(getThemyarchTarget(), "myarch", "myarch (32-bit myarchulator arch)", "myarch");
}
