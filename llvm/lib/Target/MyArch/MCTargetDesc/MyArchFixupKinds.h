//===-- MyArchFixupKinds.h - MyArch Specific Fixup Entries --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchFIXUPKINDS_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

#undef MyArch

namespace llvm {
namespace MyArch {
enum Fixups {
  // fixup_myarch_hi20 - 20-bit fixup corresponding to hi(foo) for
  // instructions like lui
  fixup_myarch_hi20 = FirstTargetFixupKind,
  // fixup_myarch_lo12_i - 12-bit fixup corresponding to lo(foo) for
  // instructions like addi
  fixup_myarch_lo12_i,
  // fixup_myarch_lo12_s - 12-bit fixup corresponding to lo(foo) for
  // the S-type store instructions
  fixup_myarch_lo12_s,
  // fixup_myarch_pcrel_hi20 - 20-bit fixup corresponding to pcrel_hi(foo) for
  // instructions like auipc
  fixup_myarch_pcrel_hi20,
  // fixup_myarch_pcrel_lo12_i - 12-bit fixup corresponding to pcrel_lo(foo) for
  // instructions like addi
  fixup_myarch_pcrel_lo12_i,
  // fixup_myarch_pcrel_lo12_s - 12-bit fixup corresponding to pcrel_lo(foo) for
  // the S-type store instructions
  fixup_myarch_pcrel_lo12_s,
  // fixup_myarch_got_hi20 - 20-bit fixup corresponding to got_pcrel_hi(foo) for
  // instructions like auipc
  fixup_myarch_got_hi20,
  // fixup_myarch_tprel_hi20 - 20-bit fixup corresponding to tprel_hi(foo) for
  // instructions like lui
  fixup_myarch_tprel_hi20,
  // fixup_myarch_tprel_lo12_i - 12-bit fixup corresponding to tprel_lo(foo) for
  // instructions like addi
  fixup_myarch_tprel_lo12_i,
  // fixup_myarch_tprel_lo12_s - 12-bit fixup corresponding to tprel_lo(foo) for
  // the S-type store instructions
  fixup_myarch_tprel_lo12_s,
  // fixup_myarch_tprel_add - A fixup corresponding to %tprel_add(foo) for the
  // add_tls instruction. Used to provide a hint to the linker.
  fixup_myarch_tprel_add,
  // fixup_myarch_tls_got_hi20 - 20-bit fixup corresponding to
  // tls_ie_pcrel_hi(foo) for instructions like auipc
  fixup_myarch_tls_got_hi20,
  // fixup_myarch_tls_gd_hi20 - 20-bit fixup corresponding to
  // tls_gd_pcrel_hi(foo) for instructions like auipc
  fixup_myarch_tls_gd_hi20,
  // fixup_myarch_jal - 20-bit fixup for symbol references in the jal
  // instruction
  fixup_myarch_jal,
  // fixup_myarch_branch - 12-bit fixup for symbol references in the branch
  // instructions
  fixup_myarch_branch,
  // fixup_myarch_rvc_jump - 11-bit fixup for symbol references in the
  // compressed jump instruction
  fixup_myarch_rvc_jump,
  // fixup_myarch_rvc_branch - 8-bit fixup for symbol references in the
  // compressed branch instruction
  fixup_myarch_rvc_branch,
  // fixup_myarch_call - A fixup representing a call attached to the auipc
  // instruction in a pair composed of adjacent auipc+jalr instructions.
  fixup_myarch_call,
  // fixup_myarch_call_plt - A fixup representing a procedure linkage table call
  // attached to the auipc instruction in a pair composed of adjacent auipc+jalr
  // instructions.
  fixup_myarch_call_plt,
  // fixup_myarch_relax - Used to generate an R_MyArch_RELAX relocation type,
  // which indicates the linker may relax the instruction pair.
  fixup_myarch_relax,
  // fixup_myarch_align - Used to generate an R_MyArch_ALIGN relocation type,
  // which indicates the linker should fixup the alignment after linker
  // relaxation.
  fixup_myarch_align,

  // fixup_myarch_invalid - used as a sentinel and a marker, must be last fixup
  fixup_myarch_invalid,
  NumTargetFixupKinds = fixup_myarch_invalid - FirstTargetFixupKind
};
} // end namespace MyArch
} // end namespace llvm

#endif
