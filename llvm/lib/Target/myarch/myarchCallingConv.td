//===----------------------------------------------------------------------===//
// This describes the calling conventions for myarch architecture.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// myarch Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_myarch : CallingConv<[
  // i32 return values
  CCIfType<[i32, i64], CCAssignToReg<[R9, R10, R11, R12]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i64], CCAssignToStack<2, 1>>,
  CCIfType<[i32], CCAssignToStack<1, 1>>
]>;

//===----------------------------------------------------------------------===//
// myarch Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_myarch : CallingConv<[
  // Promote i8/i16 arguments to i32.
  CCIfType<[i8, i16], CCPromoteToType<i32>>,

  // The first 4 integer arguments are passed in integer registers.
  CCIfType<[i32, i64], CCAssignToReg<[R9, R10, R11, R12]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i64], CCAssignToStack<2, 1>>,
  CCIfType<[i32], CCAssignToStack<1, 1>>
]>;

def CSR_myarch : CalleeSavedRegs<(add R0, R1, R3, R5, R6, R7, R8)>;
