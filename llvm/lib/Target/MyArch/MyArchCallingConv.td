//===-- MyArchCallingConv.td - Calling Conventions for MyArch --*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for MyArch architecture.
//===----------------------------------------------------------------------===//

/// CCIfSubtarget - Match if the current subtarget has a feature F.
class CCIfSubtarget<string F, CCAction A>:
  CCIf<!strconcat("State.getTarget().getSubtarget<MyArchSubtarget>().", F), A>;

//#if CH >= CH3_4 1
def RetCC_MyArchEABI : CallingConv<[
  // i32 are returned in registers V0, V1, A0, A1
  CCIfType<[i32], CCAssignToReg<[V0, V1, A0, A1]>>
]>;
//#endif

//#if CH >= CH3_4 2
def RetCC_MyArch : CallingConv<[
  CCDelegateTo<RetCC_MyArchEABI>
]>;
//#endif

def CSR_O32 : CalleeSavedRegs<(add LR, FP,
                                   (sequence "S%u", 1, 0))>;

