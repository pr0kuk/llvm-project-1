//===-- MyArch.td - Describe the MyArch Target Machine ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//if CH >= CH3_1 1
// Without this will have error: 'MyArch32I' is not a recognized processor for 
//  this target (ignoring processor)
//===----------------------------------------------------------------------===//
// MyArch Subtarget features                                                    //
//===----------------------------------------------------------------------===//


//def FeatureChapter3_1  : SubtargetFeature<"ch3_1", "HasChapterDummy", "true",
//                                "Enable Chapter instructions.">;
//def FeatureChapter3_2  : SubtargetFeature<"ch3_2", "HasChapterDummy", "true",
//                                "Enable Chapter instructions.">;
//def FeatureChapter3_3  : SubtargetFeature<"ch3_3", "HasChapterDummy", "true",
//                                "Enable Chapter instructions.">;
/*def FeatureChapter3_4  : SubtargetFeature<"ch3_4", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter3_5  : SubtargetFeature<"ch3_5", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter4_1  : SubtargetFeature<"ch4_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter4_2  : SubtargetFeature<"ch4_2", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter5_1  : SubtargetFeature<"ch5_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter6_1  : SubtargetFeature<"ch6_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter7_1  : SubtargetFeature<"ch7_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter8_1  : SubtargetFeature<"ch8_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter8_2  : SubtargetFeature<"ch8_2", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter9_1  : SubtargetFeature<"ch9_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter9_2  : SubtargetFeature<"ch9_2", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter9_3  : SubtargetFeature<"ch9_3", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter10_1 : SubtargetFeature<"ch10_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter11_1 : SubtargetFeature<"ch11_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter11_2 : SubtargetFeature<"ch11_2", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapter12_1 : SubtargetFeature<"ch12_1", "HasChapterDummy", "true",
                                "Enable Chapter instructions.">;
def FeatureChapterAll  : SubtargetFeature<"chall", "HasChapterDummy", "true",
                                "Enable Chapter instructions.",
                                [FeatureChapter3_1, FeatureChapter3_2, 
                                 FeatureChapter3_3, FeatureChapter3_4, 
                                 FeatureChapter3_5, 
                                 FeatureChapter4_1, FeatureChapter4_2, 
                                 FeatureChapter5_1, FeatureChapter6_1, 
                                 FeatureChapter7_1, FeatureChapter8_1, 
                                 FeatureChapter8_2, FeatureChapter9_1, 
                                 FeatureChapter9_2, FeatureChapter9_3, 
                                 FeatureChapter10_1, 
                                 FeatureChapter11_1, FeatureChapter11_2, 
                                 FeatureChapter12_1]>;


def FeatureCmp         : SubtargetFeature<"cmp", "HasCmp", "true",
                                "Enable 'cmp' instructions.">;
def FeatureSlt         : SubtargetFeature<"slt", "HasSlt", "true",
                                "Enable 'slt' instructions.">;
def FeatureMyArch32I     : SubtargetFeature<"MyArch32I", "MyArchArchVersion", 
                                "MyArch32I", "MyArch32I ISA Support",
                                [FeatureCmp, FeatureChapterAll]>;
def FeatureMyArch32II    : SubtargetFeature<"MyArch32II", "MyArchArchVersion",                      
                               "MyArch32II", "MyArch32II ISA Support (slt)",
                                [FeatureCmp, FeatureSlt, FeatureChapterAll]>;
#endif
*/
//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//
include "llvm/Target/Target.td"
include "MyArchSchedule.td"
include "MyArchRegisterInfo.td"
//include "MyArchOther.td"
include "MyArchInstrInfo.td"

//#if CH >= CH3_1 2
include "MyArchCallingConv.td"  
//#endif

def MyArchInstrInfo : InstrInfo;
//===----------------------------------------------------------------------===//
// MyArch processors supported.
//===----------------------------------------------------------------------===//

//#if CH >= CH3_1 3
class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, MyArchGenericItineraries, Features>;

def : Proc<"MyArch32I",  [FeatureMyArch32I]>;
def : Proc<"MyArch32II", [FeatureMyArch32II]>;
// Above make MyArchGenSubtargetInfo.inc set feature bit as the following order
// enum {
//   FeatureCmp =  1ULL << 0,
//   FeatureMyArch32I =  1ULL << 1,
//   FeatureMyArch32II =  1ULL << 2,
//   FeatureSlt =  1ULL << 3
// };
//#endif

//#if CH >= CH11_1 1
def MyArchAsmParser : AsmParser {
  let ShouldEmitMatchRegisterName = 0;
}

def MyArchAsmParserVariant : AsmParserVariant {
  int Variant = 0;

  // Recognize hard coded registers.
  string RegisterPrefix = "$";
}
//#endif

// Will generate MyArchGenAsmWrite.inc included by MyArchInstPrinter.cpp, contents 
//  as follows,
// void MyArchInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *MyArchInstPrinter::getRegisterName(unsigned RegNo) {...}
def MyArch : Target {
// def MyArchInstrInfo : InstrInfo as before.
  let InstructionSet = MyArchInstrInfo;
//#if CH >= CH11_1 2
  let AssemblyParsers = [MyArchAsmParser];
  let AssemblyParserVariants = [MyArchAsmParserVariant];
//#endif
}
