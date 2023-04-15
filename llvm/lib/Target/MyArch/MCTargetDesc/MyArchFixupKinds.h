//===-- MyArchFixupKinds.h - MyArch Specific Fixup Entries ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchFIXUPKINDS_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchFIXUPKINDS_H

//#include "MyArchConfig.h"

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace MyArch {
  // Although most of the current fixup types reflect a unique relocation
  // one can have multiple fixup types for a given relocation and thus need
  // to be uniquely named.
  //
  // This table *must* be in the save order of
  // MCFixupKindInfo Infos[MyArch::NumTargetFixupKinds]
  // in MyArchAsmBackend.cpp.
  //@Fixups {
  enum Fixups {
    //@ Pure upper 32 bit fixup resulting in - R_MyArch_32.
    fixup_MyArch_32 = FirstTargetFixupKind,

    // Pure upper 16 bit fixup resulting in - R_MyArch_HI16.
    fixup_MyArch_HI16,

    // Pure lower 16 bit fixup resulting in - R_MyArch_LO16.
    fixup_MyArch_LO16,

    // 16 bit fixup for GP offest resulting in - R_MyArch_GPREL16.
    fixup_MyArch_GPREL16,

    // GOT (Global Offset Table)
    // Symbol fixup resulting in - R_MyArch_GOT16.
    fixup_MyArch_GOT,

    

    // resulting in - R_MyArch_GOT_HI16
    fixup_MyArch_GOT_HI16,

    // resulting in - R_MyArch_GOT_LO16
    fixup_MyArch_GOT_LO16,

    // Marker
    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
  };
  //@Fixups }
} // namespace MyArch
} // namespace llvm

#endif // LLVM_MyArch_MyArchFIXUPKINDS_H

