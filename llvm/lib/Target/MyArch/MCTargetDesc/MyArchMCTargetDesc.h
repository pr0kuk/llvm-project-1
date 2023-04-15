#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCTARGETDESC_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"
#include "llvm/MC/MCTargetOptions.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCRegisterInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;
class Triple;

extern Target TheMyArchTarget;

MCCodeEmitter *createMyArchMCCodeEmitterEB(const MCInstrInfo &MCII,
                                         const MCRegisterInfo &MRI,
                                         MCContext &Ctx);
MCCodeEmitter *createMyArchMCCodeEmitterEL(const MCInstrInfo &MCII,
                                         const MCRegisterInfo &MRI,
                                         MCContext &Ctx);

MCAsmBackend *createMyArchAsmBackend(const Target &T,
                                   const MCSubtargetInfo &STI,
                                   const MCRegisterInfo &MRI,
                                   const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createMyArchELFObjectWriter(const Triple &TT);

} // End llvm namespace

// Defines symbolic names for MyArch registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "MyArchGenRegisterInfo.inc"

// Defines symbolic names for the MyArch instructions.
#define GET_INSTRINFO_ENUM
#include "MyArchGenInstrInfo.inc"

#endif
