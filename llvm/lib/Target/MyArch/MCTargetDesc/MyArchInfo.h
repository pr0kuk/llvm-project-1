#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchINFO_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchINFO_H

#include "llvm/MC/MCInstrDesc.h"

namespace llvm {

namespace MyArchCC {
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
} // end namespace MyArchCC

namespace MyArchOp {
enum OperandType : unsigned {
  OPERAND_SIMM16 = MCOI::OPERAND_FIRST_TARGET,
  OPERAND_UIMM16,
};
} // namespace MyArchOp

} // end namespace llvm

#endif
