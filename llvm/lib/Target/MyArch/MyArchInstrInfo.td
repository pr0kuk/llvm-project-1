//===----------------------------------------------------------------------===//
//
// This file contains the MyArch implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MyArch profiles and nodes
//===----------------------------------------------------------------------===//

def SDT_MyArchCall         : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_MyArchCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32>,
                                            SDTCisVT<1, i32> ]>;
def SDT_MyArchCallSeqEnd   : SDCallSeqEnd<[ SDTCisVT<0, i32>,
                                          SDTCisVT<1, i32> ]>;

def SDT_MyArchBrCC : SDTypeProfile<0, 4, [ SDTCisSameAs<0, 1>,
                                         SDTCisVT<2, OtherVT>,
                                         SDTCisVT<3, OtherVT>]>;

// These are target-independent nodes, but have target-specific formats.
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_MyArchCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_MyArchCallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def MyArch_ret      : SDNode<"MyArchISD::RET", SDTNone,
                            [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def MyArch_call     : SDNode<"MyArchISD::CALL", SDT_MyArchCall,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                             SDNPVariadic]>;

def MyArch_brcc     : SDNode<"MyArchISD::BR_CC", SDT_MyArchBrCC,
                            [SDNPHasChain]>;

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "MyArchInstrFormats.td"

//===----------------------------------------------------------------------===//
// Pseudo Instructions
//===----------------------------------------------------------------------===//

let Defs = [R2], Uses = [R2] in {
def ADJCALLSTACKDOWN : MyArchPseudoInst<(outs), (ins i32imm:$amt, i32imm:$amt2),
                               "# ADJCALLSTACKDOWN $amt, $amt2",
                               [(callseq_start timm:$amt, timm:$amt2)]>;
def ADJCALLSTACKUP : MyArchPseudoInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                            "# ADJCALLSTACKUP $amt1",
                            [(callseq_end timm:$amt1, timm:$amt2)]>;
}

def IsOrAdd: PatFrag<(ops node:$A, node:$B), (or node:$A, node:$B), [{
  return isOrEquivalentToAdd(N);
}]>;

//===----------------------------------------------------------------------===//
// MyArch Operand, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

class ImmAsmOperand<string prefix, int width, string suffix> : AsmOperandClass {
  let Name = prefix # "Imm" # width # suffix;
  let RenderMethod = "addImmOperands";
  let DiagnosticType = !strconcat("Invalid", Name);
}

class SImmAsmOperand<int width, string suffix = "">
    : ImmAsmOperand<"S", width, suffix> {
}

class UImmAsmOperand<int width, string suffix = "">
    : ImmAsmOperand<"U", width, suffix> {
}

def MyArchm16 : Operand<i32>, ImmLeaf<i32, [{return isInt<16>(Imm);}]> {
  let ParserMatchClass = SImmAsmOperand<16>;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperand<16>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_SIMM16";
  let OperandNamespace = "MyArchOp";
}

def uimm16 : Operand<i32>, ImmLeaf<i32, [{return isUInt<16>(Imm);}]> {
  let ParserMatchClass = UImmAsmOperand<16>;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeUImmOperand<16>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isUInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_UIMM16";
  let OperandNamespace = "MyArchOp";
}

def btarget16 : Operand<OtherVT> {
  let ParserMatchClass = SImmAsmOperand<16>;
  let PrintMethod = "printBranchOperand";
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperand<16>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_PCREL";
}

// TODO: sym methods
def ctarget16 : Operand<i32> {
  let ParserMatchClass = SImmAsmOperand<16>;
  let PrintMethod = "printBranchOperand";
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperand<16>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_PCREL";
}

def LI16 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint64_t)N->getZExtValue() & 0xffff,
                                   SDLoc(N), MVT::i32);
}]>;

def HI16 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint64_t)N->getZExtValue() >> 16, SDLoc(N),
                                   MVT::i32);
}]>;

// def LiImm16AsmOperand : AsmOperandClass { let Name = "LiImm16"; }
def i32li16u : Operand<i32>, PatLeaf<(i32 imm), [{
    return ((N->getZExtValue() & 0xFFFFUL) == N->getZExtValue());}], LI16> {
  // let ParserMatchClass = LiImm16AsmOperand;
}

// def HiImm16AsmOperand : AsmOperandClass { let Name = "HiImm16"; }
def i32hi16 : Operand<i32>, PatLeaf<(i32 imm), [{
    // i32hi16 predicate - true if the 32-bit immediate has only leftmost 16
    // bits set.
    return ((N->getZExtValue() & 0xFFFF0000UL) == N->getZExtValue());}], HI16> {
  // let ParserMatchClass = HiImm16AsmOperand;
  // let PrintMethod = "printHi16ImmOperand";
}

// Addressing modes
def AddrFI : ComplexPattern<iPTR, 1, "SelectAddrFI", [frameindex], []>;
def BaseAddr : ComplexPattern<iPTR, 1, "SelectBaseAddr">;

class PatGpr<SDPatternOperator OpNode, MyArchInst Inst>
    : Pat<(OpNode GPR:$rs1), (Inst GPR:$rs1)>;
class PatGprGpr<SDPatternOperator OpNode, MyArchInst Inst>
    : Pat<(OpNode GPR:$rs1, GPR:$rs2), (Inst GPR:$rs1, GPR:$rs2)>;
class PatGprImm<SDPatternOperator OpNode, MyArchInst Inst, ImmLeaf ImmType>
    : Pat<(i32 (OpNode (i32 GPR:$rs1), ImmType:$imm)),
          (Inst GPR:$rs1, ImmType:$imm)>;

class PatBcc<CondCode Cond, MyArchInst Inst>
    : Pat<(MyArch_brcc GPR:$rs1, GPR:$rs2, Cond, bb:$imm16),
          (Inst GPR:$rs1, GPR:$rs2, btarget16:$imm16)>;

//===----------------------------------------------------------------------===//
// Instructions specific format
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isBranch = 1, isTerminator = 1 in
class BranchCC<bits<8> op, string opcodestr>
    : MyArchInst<op, (outs), (ins GPR:$rs1, GPR:$rs2, btarget16:$imm16),
              !strconcat(opcodestr, " $rs1 $rs2 $imm16"), []>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ArithmRR<bits<8> op, string opcodestr>
    : MyArchInst<op, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
              !strconcat(opcodestr, " $rd $rs1 $rs2"), []>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ArithmRI<bits<8> op, string opcodestr>
    : MyArchInst<op, (outs GPR:$rd), (ins GPR:$rs1, MyArchm16:$imm),
              !strconcat(opcodestr, " $rd $rs1 $imm"), []>;

//===----------------------------------------------------------------------===//
// Instruction definition
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MyArch Instructions
//===----------------------------------------------------------------------===//

// TODO: check B* flags
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isBarrier = 1, isBranch = 1, isTerminator = 1 in
def B : MyArchInst<0x0, (outs), (ins btarget16:$imm16), "B $imm16",
                 [(br bb:$imm16)]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCall = 1, isBranch = 1, isTerminator = 1 in
def BR : MyArchInst<0x0, (outs), (ins GPR:$rs1), "BR $rs1",
                  [(brind GPR:$rs1)]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCall = 1, isBranch = 1, isTerminator = 1 in
def BL : MyArchInst<0x0, (outs GPR:$rd), (ins ctarget16:$imm16), "BL $rd $imm16",
                  []>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCall = 1, isBranch = 1, isTerminator = 1 in
def BLR : MyArchInst<0x0, (outs GPR:$rd), (ins GPR:$rs1),
                  "BLR $rd $rs1", []>;

let isBarrier = 1, isReturn = 1, isTerminator = 1 in
def PseudoRET : MyArchPseudoInst<(outs), (ins), "# PseudoRET",
                [(MyArch_ret)]>,
                PseudoInstExpansion<(BR R1)>;

let isCall = 1, Defs = [R1] in
def PseudoCALL : MyArchPseudoInst<(outs), (ins ctarget16:$func), "# PseudoCALL",
                 [(MyArch_call tglobaladdr:$func)]>,
                 PseudoInstExpansion<(BL R1, ctarget16:$func)>;

let isCall = 1, Defs = [R1] in
def PseudoCALLInd : MyArchPseudoInst<(outs), (ins GPR:$rs1), "# PseudoCALLInd",
                    [(MyArch_call GPR:$rs1)]>,
                    PseudoInstExpansion<(BLR R1, GPR:$rs1)>;

def BEQ  : BranchCC<0x0, "B.EQ">;
def BNE  : BranchCC<0x0, "B.NE">;
def BLE  : BranchCC<0x0, "B.LE">;
def BGT  : BranchCC<0x0, "B.GT">;
def BLEU  : BranchCC<0x0, "B.LEU">;
def BGTU  : BranchCC<0x0, "B.GTU">;

def : PatBcc<SETEQ, BEQ>;
def : PatBcc<SETNE, BNE>;
def : PatBcc<SETLE, BLE>;
def : PatBcc<SETGT, BGT>;
def : PatBcc<SETULE, BLEU>;
def : PatBcc<SETUGT, BGTU>;

// movli
let isAsCheapAsAMove = 1, isCodeGenOnly = 1, isReMaterializable = 1 in
def MOVLI : MyArchInst<0x0, (outs GPR:$rd), (ins i32li16u:$imm),
                     "MOVli $rd $imm",
                     [(set GPR:$rd, i32li16u:$imm)]>;

// movhi
let isAsCheapAsAMove = 1, isCodeGenOnly = 1, isReMaterializable = 1 in
def MOVHI : MyArchInst<0x0, (outs GPR:$rd), (ins i32hi16:$imm),
                     "MOVhi $rd $imm",
                     [(set GPR:$rd, i32hi16:$imm)]>;

// sti, but not SubtargetInfo!
let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
def STI_: MyArchInst<0x0, (outs), (ins GPR:$rs2, GPR:$rs1, MyArchm16:$imm),
              "STi $rs2 $rs1 $imm",
              []>;
def : Pat<(store (i32 GPR:$rs2), BaseAddr:$rs1),
        (STI_ GPR:$rs2, BaseAddr:$rs1, 0)>;
def : Pat<(store (i32 GPR:$rs2), (add BaseAddr:$rs1, MyArchm16:$imm)),
        (STI_ GPR:$rs2, BaseAddr:$rs1, MyArchm16:$imm)>;
def : Pat<(store (i32 GPR:$rs2), (IsOrAdd AddrFI:$rs1, MyArchm16:$imm)),
        (STI_ GPR:$rs2, AddrFI:$rs1, MyArchm16:$imm)>;

// ldi
let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def LDI: MyArchInst<0x0, (outs GPR:$rd), (ins GPR:$rs1, MyArchm16:$imm),
              "LDi $rd $rs1 $imm",
              []>;
def : Pat<(i32 (load BaseAddr:$rs1)), (LDI BaseAddr:$rs1, 0)>;
def : Pat<(i32 (load (add BaseAddr:$rs1, MyArchm16:$imm))),
          (LDI BaseAddr:$rs1, MyArchm16:$imm)>;
def : Pat<(i32 (load (IsOrAdd AddrFI:$rs1, MyArchm16:$imm))),
          (LDI AddrFI:$rs1, MyArchm16:$imm)>;

def ADD : ArithmRR<0x0, "ADD">;  def : PatGprGpr<add,  ADD>;
def SUB : ArithmRR<0x0, "SUB">;  def : PatGprGpr<sub,  SUB>;
def MUL : ArithmRR<0x0, "MUL">;  def : PatGprGpr<mul,  MUL>;
def DIV : ArithmRR<0x0, "DIV">;  def : PatGprGpr<sdiv, DIV>;
def OR  : ArithmRR<0x0, "OR">;   def : PatGprGpr<or,   OR>;
def AND : ArithmRR<0x0, "AND">;  def : PatGprGpr<and,  AND>;
def XOR : ArithmRR<0x0, "XOR">;  def : PatGprGpr<xor,  XOR>;
def SHRA: ArithmRR<0x0, "SHRA">; def : PatGprGpr<sra,  SHRA>;
def SHR : ArithmRR<0x0, "SHR">;  def : PatGprGpr<srl,  SHR>;
def SHL : ArithmRR<0x0, "SHL">;  def : PatGprGpr<shl,  SHL>;

def ADDI : ArithmRI<0x0, "ADDi">; def : PatGprImm<add, ADDI, MyArchm16>;
def MULI : ArithmRI<0x0, "MULi">; def : PatGprImm<mul, MULI, MyArchm16>;
def ORI  : ArithmRI<0x0, "ORi">;  def : PatGprImm<mul, ORI,  MyArchm16>;

//===----------------------------------------------------------------------===//
// MyArch Other patterns
//===----------------------------------------------------------------------===//

// i16 imm
def : Pat<(i32 i32li16u:$imm), (MOVLI imm:$imm)>;
// i32 imm
def : Pat<(i32 imm:$imm), (ORI (MOVHI (HI16 imm:$imm)), (LI16 imm:$imm))>;

def : Pat<(add (i32 AddrFI:$rs), MyArchm16:$imm16),
          (ADDI (i32 AddrFI:$rs), MyArchm16:$imm16)>;
def : Pat<(IsOrAdd (i32 AddrFI:$rs), MyArchm16:$imm16),
          (ADDI (i32 AddrFI:$rs), MyArchm16:$imm16)>;
