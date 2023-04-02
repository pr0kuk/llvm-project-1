#ifndef LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchMCTARGETDESC_H
#define LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"

#include <memory>

namespace llvm {
class Target;
class Triple;

extern Target ThemyarchTarget;

} // End llvm namespace

// Defines symbolic names for myarch registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "myarchGenRegisterInfo.inc"

// Defines symbolic names for the myarch instructions.
#define GET_INSTRINFO_ENUM
#include "myarchGenInstrInfo.inc"

#endif
