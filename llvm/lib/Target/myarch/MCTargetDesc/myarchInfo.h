#ifndef LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchINFO_H
#define LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchINFO_H

#include "llvm/MC/MCInstrDesc.h"

namespace llvm {

namespace myarchCC {
enum CondCode {
  EQ,
  NE,
  LE,
  GT,
  LEU,
  GTU,
  INVALID,
};

CondCode getOppositeBranchCondition(CondCode);

enum BRCondCode {
  BREQ = 0x0,
};
} // end namespace myarchCC

namespace myarchOp {
enum OperandType : unsigned {
  OPERAND_SIMM16 = MCOI::OPERAND_FIRST_TARGET,
  OPERAND_UIMM16,
};
} // namespace myarchOp

} // end namespace llvm

#endif
