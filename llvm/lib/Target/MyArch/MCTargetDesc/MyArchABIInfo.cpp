//===---- MyArchABIInfo.cpp - Information about MyArch ABI's ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//#include "MyArchConfig.h"

#include "MyArchABIInfo.h"
#include "MyArchRegisterInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/CommandLine.h"
// #include "MyArchGenRegisterInfo.inc"
#include "MyArch.h"
// #include "MyArchRegisterInfo.h"
using namespace llvm;

static cl::opt<bool>
EnableMyArchS32Calls("MyArch-s32-calls", cl::Hidden,
                    cl::desc("MyArch S32 call: use stack only to pass arguments.\
                    "), cl::init(false));

namespace {
static const MCPhysReg O32IntRegs[4] = {MyArch::A0, MyArch::A1};
static const MCPhysReg S32IntRegs = {};
}

const ArrayRef<MCPhysReg> MyArchABIInfo::GetByValArgRegs() const {
  if (IsO32())
    return makeArrayRef(O32IntRegs);
  if (IsS32())
    return makeArrayRef(S32IntRegs);
  llvm_unreachable("Unhandled ABI");
}

const ArrayRef<MCPhysReg> MyArchABIInfo::GetVarArgRegs() const {
  if (IsO32())
    return makeArrayRef(O32IntRegs);
  if (IsS32())
    return makeArrayRef(S32IntRegs);
  llvm_unreachable("Unhandled ABI");
}

unsigned MyArchABIInfo::GetCalleeAllocdArgSizeInBytes(CallingConv::ID CC) const {
  if (IsO32())
    return CC != 0;
  if (IsS32())
    return 0;
  llvm_unreachable("Unhandled ABI");
}

MyArchABIInfo MyArchABIInfo::computeTargetABI() {
  MyArchABIInfo abi(ABI::Unknown);

  if (EnableMyArchS32Calls)
    abi = ABI::S32;
  else
    abi = ABI::O32;
  // Assert exactly one ABI was chosen.
  assert(abi.ThisABI != ABI::Unknown);

  return abi;
}

unsigned MyArchABIInfo::GetStackPtr() const {
  return MyArch::SP;
}

unsigned MyArchABIInfo::GetFramePtr() const {
  return MyArch::FP;
}

unsigned MyArchABIInfo::GetNullPtr() const {
  return MyArch::ZERO;
}

unsigned MyArchABIInfo::GetEhDataReg(unsigned I) const {
  static const unsigned EhDataReg[] = {
    MyArch::A0, MyArch::A1
  };

  return EhDataReg[I];
}

int MyArchABIInfo::EhDataRegSize() const {
  if (ThisABI == ABI::S32)
    return 0;
  else
    return 2;
}
