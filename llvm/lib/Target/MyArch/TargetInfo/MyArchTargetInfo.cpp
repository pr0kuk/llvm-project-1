//===-- MyArchTargetInfo.cpp - MyArch Target Implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/MyArchTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

// Target &llvm::getTheMyArch32Target() {
//   static Target TheMyArch32Target;
//   return TheMyArch32Target;
// }

Target &llvm::getTheMyArchTarget() {
  static Target TheMyArchTarget;
  return TheMyArchTarget;
}

// Target &llvm::getTheMyArch64Target() {
//   static Target TheMyArch64Target;
//   return TheMyArch64Target;
// }

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchTargetInfo() {
  RegisterTarget<Triple::myarch> X(getTheMyArchTarget(), "myarch",
                                    "32-bit MyArch", "MyArch");
                                    //   RegisterTarget<Triple::myarch32> X(getTheMyArch32Target(), "myarch32",
                                    // "32-bit RISC-V", "MyArch");
  // RegisterTarget<Triple::myarch64> Y(getTheMyArch64Target(), "myarch64",
  //                                   "64-bit RISC-V", "MyArch");
}
