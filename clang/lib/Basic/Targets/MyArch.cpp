//===--- MyArch.cpp - Implement MyArch target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements MyArch TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "MyArch.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/TargetParser.h"

using namespace clang;
using namespace clang::targets;

ArrayRef<const char *> MyArchTargetInfo_G::getGCCRegNames() const {
  static const char *const GCCRegNames[] = {
      // Integer registers
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31",

      // Floating point registers
      "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
      "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
      "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
      "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"};
  return llvm::makeArrayRef(GCCRegNames);
}

ArrayRef<TargetInfo::GCCRegAlias> MyArchTargetInfo_G::getGCCRegAliases() const {
  static const TargetInfo::GCCRegAlias GCCRegAliases[] = {
      {{"zero"}, "x0"}, {{"ra"}, "x1"},   {{"sp"}, "x2"},    {{"gp"}, "x3"},
      {{"tp"}, "x4"},   {{"t0"}, "x5"},   {{"t1"}, "x6"},    {{"t2"}, "x7"},
      {{"s0"}, "x8"},   {{"s1"}, "x9"},   {{"a0"}, "x10"},   {{"a1"}, "x11"},
      {{"a2"}, "x12"},  {{"a3"}, "x13"},  {{"a4"}, "x14"},   {{"a5"}, "x15"},
      {{"a6"}, "x16"},  {{"a7"}, "x17"},  {{"s2"}, "x18"},   {{"s3"}, "x19"},
      {{"s4"}, "x20"},  {{"s5"}, "x21"},  {{"s6"}, "x22"},   {{"s7"}, "x23"},
      {{"s8"}, "x24"},  {{"s9"}, "x25"},  {{"s10"}, "x26"},  {{"s11"}, "x27"},
      {{"t3"}, "x28"},  {{"t4"}, "x29"},  {{"t5"}, "x30"},   {{"t6"}, "x31"},
      {{"ft0"}, "f0"},  {{"ft1"}, "f1"},  {{"ft2"}, "f2"},   {{"ft3"}, "f3"},
      {{"ft4"}, "f4"},  {{"ft5"}, "f5"},  {{"ft6"}, "f6"},   {{"ft7"}, "f7"},
      {{"fs0"}, "f8"},  {{"fs1"}, "f9"},  {{"fa0"}, "f10"},  {{"fa1"}, "f11"},
      {{"fa2"}, "f12"}, {{"fa3"}, "f13"}, {{"fa4"}, "f14"},  {{"fa5"}, "f15"},
      {{"fa6"}, "f16"}, {{"fa7"}, "f17"}, {{"fs2"}, "f18"},  {{"fs3"}, "f19"},
      {{"fs4"}, "f20"}, {{"fs5"}, "f21"}, {{"fs6"}, "f22"},  {{"fs7"}, "f23"},
      {{"fs8"}, "f24"}, {{"fs9"}, "f25"}, {{"fs10"}, "f26"}, {{"fs11"}, "f27"},
      {{"ft8"}, "f28"}, {{"ft9"}, "f29"}, {{"ft10"}, "f30"}, {{"ft11"}, "f31"}};
  return llvm::makeArrayRef(GCCRegAliases);
}

bool MyArchTargetInfo_G::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &Info) const {
  switch (*Name) {
  default:
    return false;
  case 'I':
    // A 12-bit signed immediate.
    Info.setRequiresImmediate(-2048, 2047);
    return true;
  case 'J':
    // Integer zero.
    Info.setRequiresImmediate(0);
    return true;
  case 'K':
    // A 5-bit unsigned immediate for CSR access instructions.
    Info.setRequiresImmediate(0, 31);
    return true;
  case 'f':
    // A floating-point register.
    Info.setAllowsRegister();
    return true;
  case 'A':
    // An address that is held in a general-purpose register.
    Info.setAllowsMemory();
    return true;
  }
}

void MyArchTargetInfo_G::getTargetDefines(const LangOptions &Opts,
                                       MacroBuilder &Builder) const {
  Builder.defineMacro("__ELF__");
  Builder.defineMacro("__myarch");
  //bool Is64Bit = getTriple().getArch() == llvm::Triple::myarch64;
  //Builder.defineMacro("__myarch_xlen", Is64Bit ? "64" : "32");
  Builder.defineMacro("__myarch_xlen", "32");
  StringRef CodeModel = getTargetOpts().CodeModel;
  if (CodeModel == "default")
    CodeModel = "small";

  if (CodeModel == "small")
    Builder.defineMacro("__myarch_cmodel_medlow");
  else if (CodeModel == "medium")
    Builder.defineMacro("__myarch_cmodel_medany");

  StringRef ABIName = getABI();
  if (ABIName == "ilp32f" || ABIName == "lp64f")
    Builder.defineMacro("__myarch_float_abi_single");
  else if (ABIName == "ilp32d" || ABIName == "lp64d")
    Builder.defineMacro("__myarch_float_abi_double");
  else
    Builder.defineMacro("__myarch_float_abi_soft");

  if (ABIName == "ilp32e")
    Builder.defineMacro("__myarch_abi_rve");

  Builder.defineMacro("__myarch_arch_test");
  Builder.defineMacro("__myarch_i", "2000000");

  if (HasM) {
    Builder.defineMacro("__myarch_m", "2000000");
    Builder.defineMacro("__myarch_mul");
    Builder.defineMacro("__myarch_div");
    Builder.defineMacro("__myarch_muldiv");
  }

  if (HasA) {
    Builder.defineMacro("__myarch_a", "2000000");
    Builder.defineMacro("__myarch_atomic");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4");
    //if (Is64Bit)
    //  Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8");
  }

  if (HasF || HasD) {
    Builder.defineMacro("__myarch_f", "2000000");
    Builder.defineMacro("__myarch_flen", HasD ? "64" : "32");
    Builder.defineMacro("__myarch_fdiv");
    Builder.defineMacro("__myarch_fsqrt");
  }

  if (HasD)
    Builder.defineMacro("__myarch_d", "2000000");

  if (HasC) {
    Builder.defineMacro("__myarch_c", "2000000");
    Builder.defineMacro("__myarch_compressed");
  }

  if (HasB) {
    Builder.defineMacro("__myarch_b", "93000");
    Builder.defineMacro("__myarch_bitmanip");
  }

  if (HasV) {
    Builder.defineMacro("__myarch_v", "10000");
    Builder.defineMacro("__myarch_vector");
  }

  if (HasZba)
    Builder.defineMacro("__myarch_zba", "93000");

  if (HasZbb)
    Builder.defineMacro("__myarch_zbb", "93000");

  if (HasZbc)
    Builder.defineMacro("__myarch_zbc", "93000");

  if (HasZbe)
    Builder.defineMacro("__myarch_zbe", "93000");

  if (HasZbf)
    Builder.defineMacro("__myarch_zbf", "93000");

  if (HasZbm)
    Builder.defineMacro("__myarch_zbm", "93000");

  if (HasZbp)
    Builder.defineMacro("__myarch_zbp", "93000");

  if (HasZbproposedc)
    Builder.defineMacro("__myarch_zbproposedc", "93000");

  if (HasZbr)
    Builder.defineMacro("__myarch_zbr", "93000");

  if (HasZbs)
    Builder.defineMacro("__myarch_zbs", "93000");

  if (HasZbt)
    Builder.defineMacro("__myarch_zbt", "93000");

  if (HasZfh)
    Builder.defineMacro("__myarch_zfh", "1000");

  if (HasZvamo)
    Builder.defineMacro("__myarch_zvamo", "10000");

  if (HasZvlsseg)
    Builder.defineMacro("__myarch_zvlsseg", "10000");
}

/// Return true if has this feature, need to sync with handleTargetFeatures.
bool MyArchTargetInfo_G::hasFeature(StringRef Feature) const {
  //bool Is64Bit = getTriple().getArch() == llvm::Triple::myarch64;
  return llvm::StringSwitch<bool>(Feature)
      .Case("myarch", true)
      //.Case("myarch32", !Is64Bit)
      //.Case("myarch64", Is64Bit)
      .Case("m", HasM)
      .Case("a", HasA)
      .Case("f", HasF)
      .Case("d", HasD)
      .Case("c", HasC)
      .Case("experimental-b", HasB)
      .Case("experimental-v", HasV)
      .Case("experimental-zba", HasZba)
      .Case("experimental-zbb", HasZbb)
      .Case("experimental-zbc", HasZbc)
      .Case("experimental-zbe", HasZbe)
      .Case("experimental-zbf", HasZbf)
      .Case("experimental-zbm", HasZbm)
      .Case("experimental-zbp", HasZbp)
      .Case("experimental-zbproposedc", HasZbproposedc)
      .Case("experimental-zbr", HasZbr)
      .Case("experimental-zbs", HasZbs)
      .Case("experimental-zbt", HasZbt)
      .Case("experimental-zfh", HasZfh)
      .Case("experimental-zvamo", HasZvamo)
      .Case("experimental-zvlsseg", HasZvlsseg)
      .Default(false);
}

/// Perform initialization based on the user configured set of features.
bool MyArchTargetInfo_G::handleTargetFeatures(std::vector<std::string> &Features,
                                           DiagnosticsEngine &Diags) {
  for (const auto &Feature : Features) {
    if (Feature == "+m")
      HasM = true;
    else if (Feature == "+a")
      HasA = true;
    else if (Feature == "+f")
      HasF = true;
    else if (Feature == "+d")
      HasD = true;
    else if (Feature == "+c")
      HasC = true;
    else if (Feature == "+experimental-b")
      HasB = true;
    else if (Feature == "+experimental-v")
      HasV = true;
    else if (Feature == "+experimental-zba")
      HasZba = true;
    else if (Feature == "+experimental-zbb")
      HasZbb = true;
    else if (Feature == "+experimental-zbc")
      HasZbc = true;
    else if (Feature == "+experimental-zbe")
      HasZbe = true;
    else if (Feature == "+experimental-zbf")
      HasZbf = true;
    else if (Feature == "+experimental-zbm")
      HasZbm = true;
    else if (Feature == "+experimental-zbp")
      HasZbp = true;
    else if (Feature == "+experimental-zbproposedc")
      HasZbproposedc = true;
    else if (Feature == "+experimental-zbr")
      HasZbr = true;
    else if (Feature == "+experimental-zbs")
      HasZbs = true;
    else if (Feature == "+experimental-zbt")
      HasZbt = true;
    else if (Feature == "+experimental-zfh")
      HasZfh = true;
    else if (Feature == "+experimental-zvamo")
      HasZvamo = true;
    else if (Feature == "+experimental-zvlsseg")
      HasZvlsseg = true;
  }

  return true;
}

bool MyArchTargetInfo::isValidCPUName(StringRef Name) const {
  return llvm::MyArch::checkCPUKind(llvm::MyArch::parseCPUKind(Name),
                                   /*Is64Bit=*/false);
}

void MyArchTargetInfo::fillValidCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  llvm::MyArch::fillValidCPUArchList(Values, false);
}

bool MyArchTargetInfo::isValidTuneCPUName(StringRef Name) const {
  return llvm::MyArch::checkTuneCPUKind(
      llvm::MyArch::parseTuneCPUKind(Name, false),
      /*Is64Bit=*/false);
}

void MyArchTargetInfo::fillValidTuneCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  llvm::MyArch::fillValidTuneCPUArchList(Values, false);
}

// bool MyArch64TargetInfo::isValidCPUName(StringRef Name) const {
//   return llvm::MyArch::checkCPUKind(llvm::MyArch::parseCPUKind(Name),
//                                    /*Is64Bit=*/true);
// }

// void MyArch64TargetInfo::fillValidCPUList(
//     SmallVectorImpl<StringRef> &Values) const {
//   llvm::MyArch::fillValidCPUArchList(Values, true);
// }

// bool MyArch64TargetInfo::isValidTuneCPUName(StringRef Name) const {
//   return llvm::MyArch::checkTuneCPUKind(
//       llvm::MyArch::parseTuneCPUKind(Name, true),
//       /*Is64Bit=*/true);
// }

// void MyArch64TargetInfo::fillValidTuneCPUList(
//     SmallVectorImpl<StringRef> &Values) const {
//   llvm::MyArch::fillValidTuneCPUArchList(Values, true);
// }
