#ifndef LLVM_LIB_TARGET_MyArch_MyArch_H
#define LLVM_LIB_TARGET_MyArch_MyArch_H

#include "MCTargetDesc/MyArchMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class MyArchTargetMachine;
class FunctionPass;
class MyArchSubtarget;
class AsmPrinter;
class InstructionSelector;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class PassRegistry;

bool lowerMyArchMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                    AsmPrinter &AP);
bool LowerMyArchMachineOperandToMCOperand(const MachineOperand &MO,
                                         MCOperand &MCOp, const AsmPrinter &AP);

FunctionPass *createMyArchISelDag(MyArchTargetMachine &TM,
                                CodeGenOpt::Level OptLevel);


namespace MyArch {
enum {
  GP = MyArch::R0,
  RA = MyArch::R1,
  SP = MyArch::R2,
  FP = MyArch::R3,
  BP = MyArch::R4,
};
} // namespace MyArch

} // namespace llvm

#endif
