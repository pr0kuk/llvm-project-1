#ifndef LLVM_LIB_TARGET_myarch_myarch_H
#define LLVM_LIB_TARGET_myarch_myarch_H

#include "MCTargetDesc/myarchMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class myarchTargetMachine;
class FunctionPass;
class myarchSubtarget;
class AsmPrinter;
class InstructionSelector;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class PassRegistry;

bool lowermyarchMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                    AsmPrinter &AP);
bool LowermyarchMachineOperandToMCOperand(const MachineOperand &MO,
                                         MCOperand &MCOp, const AsmPrinter &AP);

FunctionPass *createmyarchISelDag(myarchTargetMachine &TM,
                                CodeGenOpt::Level OptLevel);


namespace myarch {
enum {
  GP = myarch::R0,
  RA = myarch::R1,
  SP = myarch::R2,
  FP = myarch::R3,
  BP = myarch::R4,
};
} // namespace myarch

} // namespace llvm

#endif
