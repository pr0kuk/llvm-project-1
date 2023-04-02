//===----------------------------------------------------------------------===//
// This is the top level entry point for the myarch target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// myarch Subtarget features
//===----------------------------------------------------------------------===//

//def FeatureDummy
//    : SubtargetFeature<"dummy", "Dummy", "true",
//                       "Dummy feature">;

//===----------------------------------------------------------------------===//
// Target-dependent interfaces
//===----------------------------------------------------------------------===//

include "myarchRegisterInfo.td"

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "myarchCallingConv.td"

include "myarchInstrInfo.td"
def myarchInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// myarch processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;
//def : Proc<"dummy-st", []>;

// Will generate myarchGenAsmWrite.inc included by myarchInstPrinter.cpp, contents
//  as follows,
// void myarchInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *myarchInstPrinter::getRegisterName(unsigned RegNo) {...}
def myarch : Target {
  let InstructionSet = myarchInstrInfo;
}
