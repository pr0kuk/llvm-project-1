//===- IntrinsicsMyArch.td - Defines RISCV intrinsics -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the MyArch-specific intrinsics.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Atomics

// Atomic Intrinsics have multiple versions for different access widths, which
// all follow one of the following signatures (depending on how many arguments
// they require). We carefully instantiate only specific versions of these for
// specific integer widths, rather than using `llvm_anyint_ty`.
//
// In fact, as these intrinsics take `llvm_anyptr_ty`, the given names are the
// canonical names, and the intrinsics used in the code will have a name
// suffixed with the pointer type they are specialised for (denoted `<p>` in the
// names below), in order to avoid type conflicts.

let TargetPrefix = "myarch" in {

  // T @llvm.<name>.T.<p>(any*, T, T, T imm);
  class MaskedAtomicRMWFourArg<LLVMType itype>
      : Intrinsic<[itype], [llvm_anyptr_ty, itype, itype, itype],
                  [IntrArgMemOnly, NoCapture<ArgIndex<0>>, ImmArg<ArgIndex<3>>]>;
  // T @llvm.<name>.T.<p>(any*, T, T, T, T imm);
  class MaskedAtomicRMWFiveArg<LLVMType itype>
      : Intrinsic<[itype], [llvm_anyptr_ty, itype, itype, itype, itype],
                  [IntrArgMemOnly, NoCapture<ArgIndex<0>>, ImmArg<ArgIndex<4>>]>;

  // We define 32-bit and 64-bit variants of the above, where T stands for i32
  // or i64 respectively:
  multiclass MaskedAtomicRMWFourArgIntrinsics {
    // i32 @llvm.<name>.i32.<p>(any*, i32, i32, i32 imm);
    def _i32 : MaskedAtomicRMWFourArg<llvm_i32_ty>;
    // i64 @llvm.<name>.i32.<p>(any*, i64, i64, i64 imm);
    def _i64 : MaskedAtomicRMWFourArg<llvm_i64_ty>;
  }

  multiclass MaskedAtomicRMWFiveArgIntrinsics {
    // i32 @llvm.<name>.i32.<p>(any*, i32, i32, i32, i32 imm);
    def _i32 : MaskedAtomicRMWFiveArg<llvm_i32_ty>;
    // i64 @llvm.<name>.i64.<p>(any*, i64, i64, i64, i64 imm);
    def _i64 : MaskedAtomicRMWFiveArg<llvm_i64_ty>;
  }

  // @llvm.myarch.masked.atomicrmw.*.{i32,i64}.<p>(...)
  defm int_myarch_masked_atomicrmw_xchg : MaskedAtomicRMWFourArgIntrinsics;
  defm int_myarch_masked_atomicrmw_add : MaskedAtomicRMWFourArgIntrinsics;
  defm int_myarch_masked_atomicrmw_sub : MaskedAtomicRMWFourArgIntrinsics;
  defm int_myarch_masked_atomicrmw_nand : MaskedAtomicRMWFourArgIntrinsics;
  // Signed min and max need an extra operand to do sign extension with.
  defm int_myarch_masked_atomicrmw_max : MaskedAtomicRMWFiveArgIntrinsics;
  defm int_myarch_masked_atomicrmw_min : MaskedAtomicRMWFiveArgIntrinsics;
  // Unsigned min and max don't need the extra operand.
  defm int_myarch_masked_atomicrmw_umax : MaskedAtomicRMWFourArgIntrinsics;
  defm int_myarch_masked_atomicrmw_umin : MaskedAtomicRMWFourArgIntrinsics;

  // @llvm.myarch.masked.cmpxchg.{i32,i64}.<p>(...)
  defm int_myarch_masked_cmpxchg : MaskedAtomicRMWFiveArgIntrinsics;

} // TargetPrefix = "myarch"

//===----------------------------------------------------------------------===//
// Vectors

class MyArchVIntrinsic {
  // These intrinsics may accept illegal integer values in their llvm_any_ty
  // operand, so they have to be extended. If set to zero then the intrinsic
  // does not have any operand that must be extended.
  Intrinsic IntrinsicID = !cast<Intrinsic>(NAME);
  bits<4> ExtendOperand = 0;
}

let TargetPrefix = "myarch" in {
  // We use anyint here but we only support XLen.
  def int_myarch_vsetvli   : Intrinsic<[llvm_anyint_ty],
                           /* AVL */  [LLVMMatchType<0>,
                           /* VSEW */  LLVMMatchType<0>,
                           /* VLMUL */ LLVMMatchType<0>],
                                      [IntrNoMem, IntrHasSideEffects,
                                       ImmArg<ArgIndex<1>>,
                                       ImmArg<ArgIndex<2>>]>;
  def int_myarch_vsetvlimax : Intrinsic<[llvm_anyint_ty],
                            /* VSEW */ [LLVMMatchType<0>,
                            /* VLMUL */ LLVMMatchType<0>],
                                      [IntrNoMem, IntrHasSideEffects,
                                       ImmArg<ArgIndex<0>>,
                                       ImmArg<ArgIndex<1>>]>;

  // For unit stride load
  // Input: (pointer, vl)
  class MyArchUSLoad
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For unit stride fault-only-first load
  // Input: (pointer, vl)
  // Output: (data, vl)
  // NOTE: We model this with default memory properties since we model writing
  // VL as a side effect. IntrReadMem, IntrHasSideEffects does not work.
  class MyArchUSLoadFF
        : Intrinsic<[llvm_anyvector_ty, llvm_anyint_ty],
                    [LLVMPointerType<LLVMMatchType<0>>, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<0>>]>,
                    MyArchVIntrinsic;
  // For unit stride load with mask
  // Input: (maskedoff, pointer, mask, vl)
  class MyArchUSLoadMask
        : Intrinsic<[llvm_anyvector_ty ],
                    [LLVMMatchType<0>,
                     LLVMPointerType<LLVMMatchType<0>>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                     llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrReadMem]>, MyArchVIntrinsic;
  // For unit stride fault-only-first load with mask
  // Input: (maskedoff, pointer, mask, vl)
  // Output: (data, vl)
  // NOTE: We model this with default memory properties since we model writing
  // VL as a side effect. IntrReadMem, IntrHasSideEffects does not work.
  class MyArchUSLoadFFMask
        : Intrinsic<[llvm_anyvector_ty, llvm_anyint_ty],
                    [LLVMMatchType<0>,
                     LLVMPointerType<LLVMMatchType<0>>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                     LLVMMatchType<1>],
                    [NoCapture<ArgIndex<1>>]>, MyArchVIntrinsic;
  // For strided load
  // Input: (pointer, stride, vl)
  class MyArchSLoad
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyint_ty, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For strided load with mask
  // Input: (maskedoff, pointer, stride, mask, vl)
  class MyArchSLoadMask
        : Intrinsic<[llvm_anyvector_ty ],
                    [LLVMMatchType<0>,
                     LLVMPointerType<LLVMMatchType<0>>, llvm_anyint_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<1>>, IntrReadMem]>, MyArchVIntrinsic;
  // For indexed load
  // Input: (pointer, index, vl)
  class MyArchILoad
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyvector_ty, llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For indexed load with mask
  // Input: (maskedoff, pointer, index, mask, vl)
  class MyArchILoadMask
        : Intrinsic<[llvm_anyvector_ty ],
                    [LLVMMatchType<0>,
                     LLVMPointerType<LLVMMatchType<0>>, llvm_anyvector_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrReadMem]>, MyArchVIntrinsic;
  // For unit stride store
  // Input: (vector_in, pointer, vl)
  class MyArchUSStore
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For unit stride store with mask
  // Input: (vector_in, pointer, mask, vl)
  class MyArchUSStoreMask
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                     llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For strided store
  // Input: (vector_in, pointer, stride, vl)
  class MyArchSStore
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyint_ty, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For stride store with mask
  // Input: (vector_in, pointer, stirde, mask, vl)
  class MyArchSStoreMask
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>, llvm_anyint_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For indexed store
  // Input: (vector_in, pointer, index, vl)
  class MyArchIStore
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>,
                     llvm_anyint_ty, llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For indexed store with mask
  // Input: (vector_in, pointer, index, mask, vl)
  class MyArchIStoreMask
        : Intrinsic<[],
                    [llvm_anyvector_ty,
                     LLVMPointerType<LLVMMatchType<0>>, llvm_anyvector_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [NoCapture<ArgIndex<1>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For destination vector type is the same as source vector.
  // Input: (vector_in, vl)
  class MyArchUnaryAANoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is the same as first source vector (with mask).
  // Input: (vector_in, mask, vl)
  class MyArchUnaryAAMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is the same as first and second source vector.
  // Input: (vector_in, vector_in, vl)
  class MyArchBinaryAAANoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is the same as first and second source vector.
  // Input: (vector_in, vector_in, vl)
  class MyArchBinaryAAAMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is the same as first source vector.
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchBinaryAAXNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For destination vector type is the same as first source vector (with mask).
  // Input: (maskedoff, vector_in, vector_in/scalar_in, mask, vl)
  class MyArchBinaryAAXMask
       : Intrinsic<[llvm_anyvector_ty],
                   [LLVMMatchType<0>, LLVMMatchType<0>, llvm_any_ty,
                    LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                   [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 3;
  }
  // For destination vector type is NOT the same as first source vector.
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchBinaryABXNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [llvm_anyvector_ty, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For destination vector type is NOT the same as first source vector (with mask).
  // Input: (maskedoff, vector_in, vector_in/scalar_in, mask, vl)
  class MyArchBinaryABXMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty, llvm_any_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 3;
  }
  // For binary operations with V0 as input.
  // Input: (vector_in, vector_in/scalar_in, V0, vl)
  class MyArchBinaryWithV0
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_any_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                     llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For binary operations with mask type output and V0 as input.
  // Output: (mask type output)
  // Input: (vector_in, vector_in/scalar_in, V0, vl)
  class MyArchBinaryMOutWithV0
        :Intrinsic<[LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>],
                   [llvm_anyvector_ty, llvm_any_ty,
                    LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                    llvm_anyint_ty],
                   [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For binary operations with mask type output.
  // Output: (mask type output)
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchBinaryMOut
        : Intrinsic<[LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>],
                    [llvm_anyvector_ty, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For binary operations with mask type output without mask.
  // Output: (mask type output)
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchCompareNoMask
        : Intrinsic<[LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>],
                    [llvm_anyvector_ty, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For binary operations with mask type output with mask.
  // Output: (mask type output)
  // Input: (maskedoff, vector_in, vector_in/scalar_in, mask, vl)
  class MyArchCompareMask
        : Intrinsic<[LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>],
                    [LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                     llvm_anyvector_ty, llvm_any_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 3;
  }
  // For FP classify operations.
  // Output: (bit mask type output)
  // Input: (vector_in, vl)
  class MyArchClassifyNoMask
        : Intrinsic<[LLVMVectorOfBitcastsToInt<0>],
                    [llvm_anyvector_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For FP classify operations with mask.
  // Output: (bit mask type output)
  // Input: (maskedoff, vector_in, mask, vl)
  class MyArchClassifyMask
        : Intrinsic<[LLVMVectorOfBitcastsToInt<0>],
                    [LLVMVectorOfBitcastsToInt<0>, llvm_anyvector_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For Saturating binary operations.
  // The destination vector type is the same as first source vector.
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchSaturatingBinaryAAXNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem, IntrHasSideEffects]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For Saturating binary operations with mask.
  // The destination vector type is the same as first source vector.
  // Input: (maskedoff, vector_in, vector_in/scalar_in, mask, vl)
  class MyArchSaturatingBinaryAAXMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>, llvm_any_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem, IntrHasSideEffects]>, MyArchVIntrinsic {
    let ExtendOperand = 3;
  }
  // For Saturating binary operations.
  // The destination vector type is NOT the same as first source vector.
  // Input: (vector_in, vector_in/scalar_in, vl)
  class MyArchSaturatingBinaryABXNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [llvm_anyvector_ty, llvm_any_ty, llvm_anyint_ty],
                    [IntrNoMem, IntrHasSideEffects]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For Saturating binary operations with mask.
  // The destination vector type is NOT the same as first source vector (with mask).
  // Input: (maskedoff, vector_in, vector_in/scalar_in, mask, vl)
  class MyArchSaturatingBinaryABXMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty, llvm_any_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem, IntrHasSideEffects]>, MyArchVIntrinsic {
    let ExtendOperand = 3;
  }
  class MyArchTernaryAAAXNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>, llvm_anyint_ty,
                     LLVMMatchType<1>],
                    [IntrNoMem]>, MyArchVIntrinsic;
  class MyArchTernaryAAAXMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>, llvm_anyint_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, LLVMMatchType<1>],
                    [IntrNoMem]>, MyArchVIntrinsic;
  class MyArchTernaryAAXANoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_any_ty, LLVMMatchType<0>,
                     llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  class MyArchTernaryAAXAMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_any_ty, LLVMMatchType<0>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  class MyArchTernaryWideNoMask
        : Intrinsic< [llvm_anyvector_ty],
                     [LLVMMatchType<0>, llvm_any_ty, llvm_anyvector_ty,
                      llvm_anyint_ty],
                     [IntrNoMem] >, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  class MyArchTernaryWideMask
        : Intrinsic< [llvm_anyvector_ty],
                     [LLVMMatchType<0>, llvm_any_ty, llvm_anyvector_ty,
                      LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                     [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }
  // For Reduction ternary operations.
  // For destination vector type is the same as first and third source vector.
  // Input: (vector_in, vector_in, vector_in, vl)
  class MyArchReductionNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty, LLVMMatchType<0>,
                     llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For Reduction ternary operations with mask.
  // For destination vector type is the same as first and third source vector.
  // The mask type come from second source vector.
  // Input: (maskedoff, vector_in, vector_in, vector_in, mask, vl)
  class MyArchReductionMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty, LLVMMatchType<0>,
                     LLVMScalarOrSameVectorWidth<1, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For unary operations with scalar type output without mask
  // Output: (scalar type)
  // Input: (vector_in, vl)
  class MyArchMaskUnarySOutNoMask
        : Intrinsic<[llvm_anyint_ty],
                    [llvm_anyvector_ty, LLVMMatchType<0>],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For unary operations with scalar type output with mask
  // Output: (scalar type)
  // Input: (vector_in, mask, vl)
  class MyArchMaskUnarySOutMask
        : Intrinsic<[llvm_anyint_ty],
                    [llvm_anyvector_ty, LLVMMatchType<1>, LLVMMatchType<0>],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is NOT the same as source vector.
  // Input: (vector_in, vl)
  class MyArchUnaryABNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [llvm_anyvector_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For destination vector type is NOT the same as source vector (with mask).
  // Input: (maskedoff, vector_in, mask, vl)
  class MyArchUnaryABMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty,
                     LLVMScalarOrSameVectorWidth<1, llvm_i1_ty>,
                     llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For unary operations with the same vector type in/out without mask
  // Output: (vector)
  // Input: (vector_in, vl)
  class MyArchUnaryNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For mask unary operations with mask type in/out with mask
  // Output: (mask type output)
  // Input: (mask type maskedoff, mask type vector_in, mask, vl)
  class MyArchMaskUnaryMOutMask
        : Intrinsic<[llvm_anyint_ty],
                    [LLVMMatchType<0>, LLVMMatchType<0>,
                     LLVMMatchType<0>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // Output: (vector)
  // Input: (vl)
  class MyArchNullaryIntrinsic
        : Intrinsic<[llvm_anyvector_ty],
                    [llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For Conversion unary operations.
  // Input: (vector_in, vl)
  class MyArchConversionNoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [llvm_anyvector_ty, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For Conversion unary operations with mask.
  // Input: (maskedoff, vector_in, mask, vl)
  class MyArchConversionMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMMatchType<0>, llvm_anyvector_ty,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [IntrNoMem]>, MyArchVIntrinsic;
  // For atomic operations without mask
  // Input: (base, index, value, vl)
  class MyArchAMONoMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMPointerType<LLVMMatchType<0>>, llvm_anyvector_ty, LLVMMatchType<0>,
                     llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>]>, MyArchVIntrinsic;
  // For atomic operations with mask
  // Input: (base, index, value, mask, vl)
  class MyArchAMOMask
        : Intrinsic<[llvm_anyvector_ty],
                    [LLVMPointerType<LLVMMatchType<0>>, llvm_anyvector_ty, LLVMMatchType<0>,
                     LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>, llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>]>, MyArchVIntrinsic;

  // For unit stride segment load
  // Input: (pointer, vl)
  class MyArchUSSegLoad<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    [LLVMPointerToElt<0>, llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For unit stride segment load with mask
  // Input: (maskedoff, pointer, mask, vl)
  class MyArchUSSegLoadMask<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    !listconcat(!listsplat(LLVMMatchType<0>, nf),
                                [LLVMPointerToElt<0>,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrReadMem]>, MyArchVIntrinsic;

  // For unit stride fault-only-first segment load
  // Input: (pointer, vl)
  // Output: (data, vl)
  // NOTE: We model this with default memory properties since we model writing
  // VL as a side effect. IntrReadMem, IntrHasSideEffects does not work.
  class MyArchUSSegLoadFF<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1)), [llvm_anyint_ty]),
                    [LLVMPointerToElt<0>, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<0>>]>, MyArchVIntrinsic;
  // For unit stride fault-only-first segment load with mask
  // Input: (maskedoff, pointer, mask, vl)
  // Output: (data, vl)
  // NOTE: We model this with default memory properties since we model writing
  // VL as a side effect. IntrReadMem, IntrHasSideEffects does not work.
  class MyArchUSSegLoadFFMask<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1)), [llvm_anyint_ty]),
                    !listconcat(!listsplat(LLVMMatchType<0>, nf),
                     [LLVMPointerToElt<0>,
                      LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                      LLVMMatchType<1>]),
                    [NoCapture<ArgIndex<nf>>]>, MyArchVIntrinsic;

  // For stride segment load
  // Input: (pointer, offset, vl)
  class MyArchSSegLoad<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    [LLVMPointerToElt<0>, llvm_anyint_ty, LLVMMatchType<1>],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For stride segment load with mask
  // Input: (maskedoff, pointer, offset, mask, vl)
  class MyArchSSegLoadMask<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    !listconcat(!listsplat(LLVMMatchType<0>, nf),
                                [LLVMPointerToElt<0>,
                                 llvm_anyint_ty,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 LLVMMatchType<1>]),
                    [NoCapture<ArgIndex<nf>>, IntrReadMem]>, MyArchVIntrinsic;

  // For indexed segment load
  // Input: (pointer, index, vl)
  class MyArchISegLoad<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    [LLVMPointerToElt<0>, llvm_anyvector_ty, llvm_anyint_ty],
                    [NoCapture<ArgIndex<0>>, IntrReadMem]>, MyArchVIntrinsic;
  // For indexed segment load with mask
  // Input: (maskedoff, pointer, index, mask, vl)
  class MyArchISegLoadMask<int nf>
        : Intrinsic<!listconcat([llvm_anyvector_ty], !listsplat(LLVMMatchType<0>,
                                !add(nf, -1))),
                    !listconcat(!listsplat(LLVMMatchType<0>, nf),
                                [LLVMPointerToElt<0>,
                                 llvm_anyvector_ty,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrReadMem]>, MyArchVIntrinsic;

  // For unit stride segment store
  // Input: (value, pointer, vl)
  class MyArchUSSegStore<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>, llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For unit stride segment store with mask
  // Input: (value, pointer, mask, vl)
  class MyArchUSSegStoreMask<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;

  // For stride segment store
  // Input: (value, pointer, offset, vl)
  class MyArchSSegStore<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>, llvm_anyint_ty,
                                 LLVMMatchType<1>]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For stride segment store with mask
  // Input: (value, pointer, offset, mask, vl)
  class MyArchSSegStoreMask<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>, llvm_anyint_ty,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 LLVMMatchType<1>]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;

  // For indexed segment store
  // Input: (value, pointer, offset, vl)
  class MyArchISegStore<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>, llvm_anyvector_ty,
                                 llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;
  // For indexed segment store with mask
  // Input: (value, pointer, offset, mask, vl)
  class MyArchISegStoreMask<int nf>
        : Intrinsic<[],
                    !listconcat([llvm_anyvector_ty],
                                !listsplat(LLVMMatchType<0>, !add(nf, -1)),
                                [LLVMPointerToElt<0>, llvm_anyvector_ty,
                                 LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                 llvm_anyint_ty]),
                    [NoCapture<ArgIndex<nf>>, IntrWriteMem]>, MyArchVIntrinsic;

  multiclass MyArchUSLoad {
    def "int_myarch_" # NAME : MyArchUSLoad;
    def "int_myarch_" # NAME # "_mask" : MyArchUSLoadMask;
  }
  multiclass MyArchUSLoadFF {
    def "int_myarch_" # NAME : MyArchUSLoadFF;
    def "int_myarch_" # NAME # "_mask" : MyArchUSLoadFFMask;
  }
  multiclass MyArchSLoad {
    def "int_myarch_" # NAME : MyArchSLoad;
    def "int_myarch_" # NAME # "_mask" : MyArchSLoadMask;
  }
  multiclass MyArchILoad {
    def "int_myarch_" # NAME : MyArchILoad;
    def "int_myarch_" # NAME # "_mask" : MyArchILoadMask;
  }
  multiclass MyArchUSStore {
    def "int_myarch_" # NAME : MyArchUSStore;
    def "int_myarch_" # NAME # "_mask" : MyArchUSStoreMask;
  }
  multiclass MyArchSStore {
    def "int_myarch_" # NAME : MyArchSStore;
    def "int_myarch_" # NAME # "_mask" : MyArchSStoreMask;
  }

  multiclass MyArchIStore {
    def "int_myarch_" # NAME : MyArchIStore;
    def "int_myarch_" # NAME # "_mask" : MyArchIStoreMask;
  }
  multiclass MyArchUnaryAA {
    def "int_myarch_" # NAME : MyArchUnaryAANoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchUnaryAAMask;
  }
  multiclass MyArchUnaryAB {
    def "int_myarch_" # NAME : MyArchUnaryABNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchUnaryABMask;
  }
  // AAX means the destination type(A) is the same as the first source
  // type(A). X means any type for the second source operand.
  multiclass MyArchBinaryAAX {
    def "int_myarch_" # NAME : MyArchBinaryAAXNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchBinaryAAXMask;
  }
  // ABX means the destination type(A) is different from the first source
  // type(B). X means any type for the second source operand.
  multiclass MyArchBinaryABX {
    def "int_myarch_" # NAME : MyArchBinaryABXNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchBinaryABXMask;
  }
  multiclass MyArchBinaryWithV0 {
    def "int_myarch_" # NAME : MyArchBinaryWithV0;
  }
  multiclass MyArchBinaryMaskOutWithV0 {
    def "int_myarch_" # NAME : MyArchBinaryMOutWithV0;
  }
  multiclass MyArchBinaryMaskOut {
    def "int_myarch_" # NAME : MyArchBinaryMOut;
  }
  multiclass MyArchSaturatingBinaryAAX {
    def "int_myarch_" # NAME : MyArchSaturatingBinaryAAXNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchSaturatingBinaryAAXMask;
  }
  multiclass MyArchSaturatingBinaryABX {
    def "int_myarch_" # NAME : MyArchSaturatingBinaryABXNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchSaturatingBinaryABXMask;
  }
  multiclass MyArchTernaryAAAX {
    def "int_myarch_" # NAME : MyArchTernaryAAAXNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchTernaryAAAXMask;
  }
  multiclass MyArchTernaryAAXA {
    def "int_myarch_" # NAME : MyArchTernaryAAXANoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchTernaryAAXAMask;
  }
  multiclass MyArchCompare {
    def "int_myarch_" # NAME : MyArchCompareNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchCompareMask;
  }
  multiclass MyArchClassify {
    def "int_myarch_" # NAME : MyArchClassifyNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchClassifyMask;
  }
  multiclass MyArchTernaryWide {
    def "int_myarch_" # NAME : MyArchTernaryWideNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchTernaryWideMask;
  }
  multiclass MyArchReduction {
    def "int_myarch_" # NAME : MyArchReductionNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchReductionMask;
  }
  multiclass MyArchMaskUnarySOut {
    def "int_myarch_" # NAME : MyArchMaskUnarySOutNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchMaskUnarySOutMask;
  }
  multiclass MyArchMaskUnaryMOut {
    def "int_myarch_" # NAME : MyArchUnaryNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchMaskUnaryMOutMask;
  }
  multiclass MyArchConversion {
    def "int_myarch_" #NAME :MyArchConversionNoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchConversionMask;
  }
  multiclass MyArchAMO {
    def "int_myarch_" # NAME : MyArchAMONoMask;
    def "int_myarch_" # NAME # "_mask" : MyArchAMOMask;
  }
  multiclass MyArchUSSegLoad<int nf> {
    def "int_myarch_" # NAME : MyArchUSSegLoad<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchUSSegLoadMask<nf>;
  }
  multiclass MyArchUSSegLoadFF<int nf> {
    def "int_myarch_" # NAME : MyArchUSSegLoadFF<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchUSSegLoadFFMask<nf>;
  }
  multiclass MyArchSSegLoad<int nf> {
    def "int_myarch_" # NAME : MyArchSSegLoad<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchSSegLoadMask<nf>;
  }
  multiclass MyArchISegLoad<int nf> {
    def "int_myarch_" # NAME : MyArchISegLoad<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchISegLoadMask<nf>;
  }
  multiclass MyArchUSSegStore<int nf> {
    def "int_myarch_" # NAME : MyArchUSSegStore<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchUSSegStoreMask<nf>;
  }
  multiclass MyArchSSegStore<int nf> {
    def "int_myarch_" # NAME : MyArchSSegStore<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchSSegStoreMask<nf>;
  }
  multiclass MyArchISegStore<int nf> {
    def "int_myarch_" # NAME : MyArchISegStore<nf>;
    def "int_myarch_" # NAME # "_mask" : MyArchISegStoreMask<nf>;
  }

  defm vle : MyArchUSLoad;
  defm vleff : MyArchUSLoadFF;
  defm vse : MyArchUSStore;
  defm vlse: MyArchSLoad;
  defm vsse: MyArchSStore;
  defm vluxei : MyArchILoad;
  defm vloxei : MyArchILoad;
  defm vsoxei : MyArchIStore;
  defm vsuxei : MyArchIStore;

  def int_myarch_vle1 : MyArchUSLoad;
  def int_myarch_vse1 : MyArchUSStore;

  defm vamoswap : MyArchAMO;
  defm vamoadd : MyArchAMO;
  defm vamoxor : MyArchAMO;
  defm vamoand : MyArchAMO;
  defm vamoor : MyArchAMO;
  defm vamomin : MyArchAMO;
  defm vamomax : MyArchAMO;
  defm vamominu : MyArchAMO;
  defm vamomaxu : MyArchAMO;

  defm vadd : MyArchBinaryAAX;
  defm vsub : MyArchBinaryAAX;
  defm vrsub : MyArchBinaryAAX;

  defm vwaddu : MyArchBinaryABX;
  defm vwadd : MyArchBinaryABX;
  defm vwaddu_w : MyArchBinaryAAX;
  defm vwadd_w : MyArchBinaryAAX;
  defm vwsubu : MyArchBinaryABX;
  defm vwsub : MyArchBinaryABX;
  defm vwsubu_w : MyArchBinaryAAX;
  defm vwsub_w : MyArchBinaryAAX;

  defm vzext : MyArchUnaryAB;
  defm vsext : MyArchUnaryAB;

  defm vadc : MyArchBinaryWithV0;
  defm vmadc_carry_in : MyArchBinaryMaskOutWithV0;
  defm vmadc : MyArchBinaryMaskOut;

  defm vsbc : MyArchBinaryWithV0;
  defm vmsbc_borrow_in : MyArchBinaryMaskOutWithV0;
  defm vmsbc : MyArchBinaryMaskOut;

  defm vand : MyArchBinaryAAX;
  defm vor : MyArchBinaryAAX;
  defm vxor : MyArchBinaryAAX;

  defm vsll : MyArchBinaryAAX;
  defm vsrl : MyArchBinaryAAX;
  defm vsra : MyArchBinaryAAX;

  defm vnsrl : MyArchBinaryABX;
  defm vnsra : MyArchBinaryABX;

  defm vmseq : MyArchCompare;
  defm vmsne : MyArchCompare;
  defm vmsltu : MyArchCompare;
  defm vmslt : MyArchCompare;
  defm vmsleu : MyArchCompare;
  defm vmsle : MyArchCompare;
  defm vmsgtu : MyArchCompare;
  defm vmsgt : MyArchCompare;

  defm vminu : MyArchBinaryAAX;
  defm vmin : MyArchBinaryAAX;
  defm vmaxu : MyArchBinaryAAX;
  defm vmax : MyArchBinaryAAX;

  defm vmul : MyArchBinaryAAX;
  defm vmulh : MyArchBinaryAAX;
  defm vmulhu : MyArchBinaryAAX;
  defm vmulhsu : MyArchBinaryAAX;

  defm vdivu : MyArchBinaryAAX;
  defm vdiv : MyArchBinaryAAX;
  defm vremu : MyArchBinaryAAX;
  defm vrem : MyArchBinaryAAX;

  defm vwmul : MyArchBinaryABX;
  defm vwmulu : MyArchBinaryABX;
  defm vwmulsu : MyArchBinaryABX;

  defm vmacc : MyArchTernaryAAXA;
  defm vnmsac : MyArchTernaryAAXA;
  defm vmadd : MyArchTernaryAAXA;
  defm vnmsub : MyArchTernaryAAXA;

  defm vwmaccu  : MyArchTernaryWide;
  defm vwmacc   : MyArchTernaryWide;
  defm vwmaccus : MyArchTernaryWide;
  defm vwmaccsu : MyArchTernaryWide;

  defm vfadd : MyArchBinaryAAX;
  defm vfsub : MyArchBinaryAAX;
  defm vfrsub : MyArchBinaryAAX;

  defm vfwadd : MyArchBinaryABX;
  defm vfwsub : MyArchBinaryABX;
  defm vfwadd_w : MyArchBinaryAAX;
  defm vfwsub_w : MyArchBinaryAAX;

  defm vsaddu : MyArchSaturatingBinaryAAX;
  defm vsadd : MyArchSaturatingBinaryAAX;
  defm vssubu : MyArchSaturatingBinaryAAX;
  defm vssub : MyArchSaturatingBinaryAAX;

  def int_myarch_vmerge : MyArchBinaryWithV0;

  def int_myarch_vmv_v_v : Intrinsic<[llvm_anyvector_ty],
                                    [LLVMMatchType<0>, llvm_anyint_ty],
                                    [IntrNoMem]>, MyArchVIntrinsic;
  def int_myarch_vmv_v_x : Intrinsic<[llvm_anyint_ty],
                                    [LLVMVectorElementType<0>, llvm_anyint_ty],
                                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 1;
  }
  def int_myarch_vfmv_v_f : Intrinsic<[llvm_anyfloat_ty],
                                     [LLVMVectorElementType<0>, llvm_anyint_ty],
                                     [IntrNoMem]>, MyArchVIntrinsic;

  def int_myarch_vmv_x_s : Intrinsic<[LLVMVectorElementType<0>],
                                    [llvm_anyint_ty],
                                    [IntrNoMem]>, MyArchVIntrinsic;
  def int_myarch_vmv_s_x : Intrinsic<[llvm_anyint_ty],
                                    [LLVMMatchType<0>, LLVMVectorElementType<0>,
                                     llvm_anyint_ty],
                                    [IntrNoMem]>, MyArchVIntrinsic {
    let ExtendOperand = 2;
  }

  def int_myarch_vfmv_f_s : Intrinsic<[LLVMVectorElementType<0>],
                                     [llvm_anyfloat_ty],
                                     [IntrNoMem]>, MyArchVIntrinsic;
  def int_myarch_vfmv_s_f : Intrinsic<[llvm_anyfloat_ty],
                                     [LLVMMatchType<0>, LLVMVectorElementType<0>,
                                      llvm_anyint_ty],
                                     [IntrNoMem]>, MyArchVIntrinsic;

  defm vfmul : MyArchBinaryAAX;
  defm vfdiv : MyArchBinaryAAX;
  defm vfrdiv : MyArchBinaryAAX;

  defm vfwmul : MyArchBinaryABX;

  defm vfmacc : MyArchTernaryAAXA;
  defm vfnmacc : MyArchTernaryAAXA;
  defm vfmsac : MyArchTernaryAAXA;
  defm vfnmsac : MyArchTernaryAAXA;
  defm vfmadd : MyArchTernaryAAXA;
  defm vfnmadd : MyArchTernaryAAXA;
  defm vfmsub : MyArchTernaryAAXA;
  defm vfnmsub : MyArchTernaryAAXA;

  defm vfwmacc : MyArchTernaryWide;
  defm vfwnmacc : MyArchTernaryWide;
  defm vfwmsac : MyArchTernaryWide;
  defm vfwnmsac : MyArchTernaryWide;

  defm vfsqrt : MyArchUnaryAA;
  defm vfrsqrt7 : MyArchUnaryAA;
  defm vfrec7 : MyArchUnaryAA;

  defm vfmin : MyArchBinaryAAX;
  defm vfmax : MyArchBinaryAAX;

  defm vfsgnj : MyArchBinaryAAX;
  defm vfsgnjn : MyArchBinaryAAX;
  defm vfsgnjx : MyArchBinaryAAX;

  defm vfclass : MyArchClassify;

  defm vfmerge : MyArchBinaryWithV0;

  defm vslideup : MyArchTernaryAAAX;
  defm vslidedown : MyArchTernaryAAAX;

  defm vslide1up : MyArchBinaryAAX;
  defm vslide1down : MyArchBinaryAAX;
  defm vfslide1up : MyArchBinaryAAX;
  defm vfslide1down : MyArchBinaryAAX;

  defm vrgather : MyArchBinaryAAX;
  defm vrgatherei16 : MyArchBinaryAAX;

  def "int_myarch_vcompress" : MyArchBinaryAAAMask;

  defm vaaddu : MyArchSaturatingBinaryAAX;
  defm vaadd : MyArchSaturatingBinaryAAX;
  defm vasubu : MyArchSaturatingBinaryAAX;
  defm vasub : MyArchSaturatingBinaryAAX;

  defm vsmul : MyArchSaturatingBinaryAAX;

  defm vssrl : MyArchSaturatingBinaryAAX;
  defm vssra : MyArchSaturatingBinaryAAX;

  defm vnclipu : MyArchSaturatingBinaryABX;
  defm vnclip : MyArchSaturatingBinaryABX;

  defm vmfeq : MyArchCompare;
  defm vmfne : MyArchCompare;
  defm vmflt : MyArchCompare;
  defm vmfle : MyArchCompare;
  defm vmfgt : MyArchCompare;
  defm vmfge : MyArchCompare;

  defm vredsum : MyArchReduction;
  defm vredand : MyArchReduction;
  defm vredor : MyArchReduction;
  defm vredxor : MyArchReduction;
  defm vredminu : MyArchReduction;
  defm vredmin : MyArchReduction;
  defm vredmaxu : MyArchReduction;
  defm vredmax : MyArchReduction;

  defm vwredsumu : MyArchReduction;
  defm vwredsum : MyArchReduction;

  defm vfredosum : MyArchReduction;
  defm vfredsum : MyArchReduction;
  defm vfredmin : MyArchReduction;
  defm vfredmax : MyArchReduction;

  defm vfwredsum : MyArchReduction;
  defm vfwredosum : MyArchReduction;

  def int_myarch_vmand: MyArchBinaryAAANoMask;
  def int_myarch_vmnand: MyArchBinaryAAANoMask;
  def int_myarch_vmandnot: MyArchBinaryAAANoMask;
  def int_myarch_vmxor: MyArchBinaryAAANoMask;
  def int_myarch_vmor: MyArchBinaryAAANoMask;
  def int_myarch_vmnor: MyArchBinaryAAANoMask;
  def int_myarch_vmornot: MyArchBinaryAAANoMask;
  def int_myarch_vmxnor: MyArchBinaryAAANoMask;
  def int_myarch_vmclr : MyArchNullaryIntrinsic;
  def int_myarch_vmset : MyArchNullaryIntrinsic;

  defm vpopc : MyArchMaskUnarySOut;
  defm vfirst : MyArchMaskUnarySOut;
  defm vmsbf : MyArchMaskUnaryMOut;
  defm vmsof : MyArchMaskUnaryMOut;
  defm vmsif : MyArchMaskUnaryMOut;

  defm vfcvt_xu_f_v : MyArchConversion;
  defm vfcvt_x_f_v : MyArchConversion;
  defm vfcvt_rtz_xu_f_v : MyArchConversion;
  defm vfcvt_rtz_x_f_v : MyArchConversion;
  defm vfcvt_f_xu_v : MyArchConversion;
  defm vfcvt_f_x_v : MyArchConversion;

  defm vfwcvt_f_xu_v : MyArchConversion;
  defm vfwcvt_f_x_v : MyArchConversion;
  defm vfwcvt_xu_f_v : MyArchConversion;
  defm vfwcvt_x_f_v : MyArchConversion;
  defm vfwcvt_rtz_xu_f_v : MyArchConversion;
  defm vfwcvt_rtz_x_f_v : MyArchConversion;
  defm vfwcvt_f_f_v : MyArchConversion;

  defm vfncvt_f_xu_w : MyArchConversion;
  defm vfncvt_f_x_w : MyArchConversion;
  defm vfncvt_xu_f_w : MyArchConversion;
  defm vfncvt_x_f_w : MyArchConversion;
  defm vfncvt_rtz_xu_f_w : MyArchConversion;
  defm vfncvt_rtz_x_f_w : MyArchConversion;
  defm vfncvt_f_f_w : MyArchConversion;
  defm vfncvt_rod_f_f_w : MyArchConversion;

  // Output: (vector)
  // Input: (mask type input, vl)
  def int_myarch_viota : Intrinsic<[llvm_anyvector_ty],
                                  [LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                   llvm_anyint_ty],
                                  [IntrNoMem]>, MyArchVIntrinsic;
  // Output: (vector)
  // Input: (maskedoff, mask type vector_in, mask, vl)
  def int_myarch_viota_mask : Intrinsic<[llvm_anyvector_ty],
                                       [LLVMMatchType<0>,
                                        LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                        LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                        llvm_anyint_ty],
                                       [IntrNoMem]>, MyArchVIntrinsic;
  // Output: (vector)
  // Input: (vl)
  def int_myarch_vid : MyArchNullaryIntrinsic;

  // Output: (vector)
  // Input: (maskedoff, mask, vl)
  def int_myarch_vid_mask : Intrinsic<[llvm_anyvector_ty],
                                     [LLVMMatchType<0>,
                                      LLVMScalarOrSameVectorWidth<0, llvm_i1_ty>,
                                      llvm_anyint_ty],
                                     [IntrNoMem]>, MyArchVIntrinsic;

  foreach nf = [2, 3, 4, 5, 6, 7, 8] in {
    defm vlseg # nf : MyArchUSSegLoad<nf>;
    defm vlseg # nf # ff : MyArchUSSegLoadFF<nf>;
    defm vlsseg # nf : MyArchSSegLoad<nf>;
    defm vloxseg # nf : MyArchISegLoad<nf>;
    defm vluxseg # nf : MyArchISegLoad<nf>;
    defm vsseg # nf : MyArchUSSegStore<nf>;
    defm vssseg # nf : MyArchSSegStore<nf>;
    defm vsoxseg # nf : MyArchISegStore<nf>;
    defm vsuxseg # nf : MyArchISegStore<nf>;
  }

} // TargetPrefix = "myarch"
