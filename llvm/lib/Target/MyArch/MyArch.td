//===----------------------------------------------------------------------===//
// This is the top level entry point for the MyArch target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// MyArch Subtarget features
//===----------------------------------------------------------------------===//

//def FeatureDummy
//    : SubtargetFeature<"dummy", "Dummy", "true",
//                       "Dummy feature">;

//===----------------------------------------------------------------------===//
// Target-dependent interfaces
//===----------------------------------------------------------------------===//

include "MyArchRegisterInfo.td"

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "MyArchCallingConv.td"

include "MyArchInstrInfo.td"
def MyArchInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// MyArch processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;
//def : Proc<"dummy-st", []>;

// Will generate MyArchGenAsmWrite.inc included by MyArchInstPrinter.cpp, contents
//  as follows,
// void MyArchInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *MyArchInstPrinter::getRegisterName(unsigned RegNo) {...}
def MyArch : Target {
  let InstructionSet = MyArchInstrInfo;
}
