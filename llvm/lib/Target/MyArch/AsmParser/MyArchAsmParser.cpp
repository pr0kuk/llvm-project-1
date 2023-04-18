//===-- MyArchAsmParser.cpp - Parse MyArch assembly to MCInst instructions --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MyArchAsmBackend.h"
#include "MCTargetDesc/MyArchBaseInfo.h"
#include "MCTargetDesc/MyArchInstPrinter.h"
#include "MCTargetDesc/MyArchMCExpr.h"
#include "MCTargetDesc/MyArchMCTargetDesc.h"
#include "MCTargetDesc/MyArchMatInt.h"
#include "MCTargetDesc/MyArchTargetStreamer.h"
#include "TargetInfo/MyArchTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MyArchAttributes.h"
#include "llvm/Support/TargetRegistry.h"

#include <limits>

using namespace llvm;

#define DEBUG_TYPE "myarch-asm-parser"

// Include the auto-generated portion of the compress emitter.
#define GEN_COMPRESS_INSTR
#include "MyArchGenCompressInstEmitter.inc"

STATISTIC(MyArchNumInstrsCompressed,
          "Number of RISC-V Compressed instructions emitted");

namespace {
struct MyArchOperand;

struct ParserOptionsSet {
  bool IsPicEnabled;
};

class MyArchAsmParser : public MCTargetAsmParser {
  SmallVector<FeatureBitset, 4> FeatureBitStack;

  SmallVector<ParserOptionsSet, 4> ParserOptionsStack;
  ParserOptionsSet ParserOptions;

  SMLoc getLoc() const { return getParser().getTok().getLoc(); }
  bool isRV64() const { return getSTI().hasFeature(MyArch::Feature64Bit); }
  bool isRV32E() const { return getSTI().hasFeature(MyArch::FeatureRV32E); }

  MyArchTargetStreamer &getTargetStreamer() {
    MCTargetStreamer &TS = *getParser().getStreamer().getTargetStreamer();
    return static_cast<MyArchTargetStreamer &>(TS);
  }

  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

  bool generateImmOutOfRangeError(OperandVector &Operands, uint64_t ErrorInfo,
                                  int64_t Lower, int64_t Upper, Twine Msg);

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  bool ParseDirective(AsmToken DirectiveID) override;

  // Helper to actually emit an instruction to the MCStreamer. Also, when
  // possible, compression of the instruction is performed.
  void emitToStreamer(MCStreamer &S, const MCInst &Inst);

  // Helper to emit a combination of LUI, ADDI(W), and SLLI instructions that
  // synthesize the desired immedate value into the destination register.
  void emitLoadImm(MCRegister DestReg, int64_t Value, MCStreamer &Out);

  // Helper to emit a combination of AUIPC and SecondOpcode. Used to implement
  // helpers such as emitLoadLocalAddress and emitLoadAddress.
  void emitAuipcInstPair(MCOperand DestReg, MCOperand TmpReg,
                         const MCExpr *Symbol, MyArchMCExpr::VariantKind VKHi,
                         unsigned SecondOpcode, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "lla" used in PC-rel addressing.
  void emitLoadLocalAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la" used in GOT/PC-rel addressing.
  void emitLoadAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la.tls.ie" used in initial-exec TLS
  // addressing.
  void emitLoadTLSIEAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la.tls.gd" used in global-dynamic TLS
  // addressing.
  void emitLoadTLSGDAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo load/store instruction with a symbol.
  void emitLoadStoreSymbol(MCInst &Inst, unsigned Opcode, SMLoc IDLoc,
                           MCStreamer &Out, bool HasTmpReg);

  // Helper to emit pseudo sign/zero extend instruction.
  void emitPseudoExtend(MCInst &Inst, bool SignExtend, int64_t Width,
                        SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo vmsge{u}.vx instruction.
  void emitVMSGE(MCInst &Inst, unsigned Opcode, SMLoc IDLoc, MCStreamer &Out);

  // Checks that a PseudoAddTPRel is using x4/tp in its second input operand.
  // Enforcing this using a restricted register class for the second input
  // operand of PseudoAddTPRel results in a poor diagnostic due to the fact
  // 'add' is an overloaded mnemonic.
  bool checkPseudoAddTPRel(MCInst &Inst, OperandVector &Operands);

  // Check instruction constraints.
  bool validateInstruction(MCInst &Inst, OperandVector &Operands);

  /// Helper for processing MC instructions that have been successfully matched
  /// by MatchAndEmitInstruction. Modifications to the emitted instructions,
  /// like the expansion of pseudo instructions (e.g., "li"), can be performed
  /// in this method.
  bool processInstruction(MCInst &Inst, SMLoc IDLoc, OperandVector &Operands,
                          MCStreamer &Out);

// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "MyArchGenAsmMatcher.inc"

  OperandMatchResultTy parseCSRSystemRegister(OperandVector &Operands);
  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseRegister(OperandVector &Operands,
                                     bool AllowParens = false);
  OperandMatchResultTy parseMemOpBaseReg(OperandVector &Operands);
  OperandMatchResultTy parseAtomicMemOp(OperandVector &Operands);
  OperandMatchResultTy parseOperandWithModifier(OperandVector &Operands);
  OperandMatchResultTy parseBareSymbol(OperandVector &Operands);
  OperandMatchResultTy parseCallSymbol(OperandVector &Operands);
  OperandMatchResultTy parsePseudoJumpSymbol(OperandVector &Operands);
  OperandMatchResultTy parseJALOffset(OperandVector &Operands);
  OperandMatchResultTy parseVTypeI(OperandVector &Operands);
  OperandMatchResultTy parseMaskReg(OperandVector &Operands);

  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);

  bool parseDirectiveOption();
  bool parseDirectiveAttribute();

  void setFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (!(getSTI().getFeatureBits()[Feature])) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

  bool getFeatureBits(uint64_t Feature) {
    return getSTI().getFeatureBits()[Feature];
  }

  void clearFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (getSTI().getFeatureBits()[Feature]) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

  void pushFeatureBits() {
    assert(FeatureBitStack.size() == ParserOptionsStack.size() &&
           "These two stacks must be kept synchronized");
    FeatureBitStack.push_back(getSTI().getFeatureBits());
    ParserOptionsStack.push_back(ParserOptions);
  }

  bool popFeatureBits() {
    assert(FeatureBitStack.size() == ParserOptionsStack.size() &&
           "These two stacks must be kept synchronized");
    if (FeatureBitStack.empty())
      return true;

    FeatureBitset FeatureBits = FeatureBitStack.pop_back_val();
    copySTI().setFeatureBits(FeatureBits);
    setAvailableFeatures(ComputeAvailableFeatures(FeatureBits));

    ParserOptions = ParserOptionsStack.pop_back_val();

    return false;
  }

  std::unique_ptr<MyArchOperand> defaultMaskRegOp() const;

public:
  enum MyArchMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "MyArchGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

  static bool classifySymbolRef(const MCExpr *Expr,
                                MyArchMCExpr::VariantKind &Kind);

  MyArchAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                 const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    Parser.addAliasForDirective(".half", ".2byte");
    Parser.addAliasForDirective(".hword", ".2byte");
    Parser.addAliasForDirective(".word", ".4byte");
    Parser.addAliasForDirective(".dword", ".8byte");
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));

    auto ABIName = StringRef(Options.ABIName);
    if (ABIName.endswith("f") &&
        !getSTI().getFeatureBits()[MyArch::FeatureStdExtF]) {
      errs() << "Hard-float 'f' ABI can't be used for a target that "
                "doesn't support the F instruction set extension (ignoring "
                "target-abi)\n";
    } else if (ABIName.endswith("d") &&
               !getSTI().getFeatureBits()[MyArch::FeatureStdExtD]) {
      errs() << "Hard-float 'd' ABI can't be used for a target that "
                "doesn't support the D instruction set extension (ignoring "
                "target-abi)\n";
    }

    const MCObjectFileInfo *MOFI = Parser.getContext().getObjectFileInfo();
    ParserOptions.IsPicEnabled = MOFI->isPositionIndependent();
  }
};

/// MyArchOperand - Instances of this class represent a parsed machine
/// instruction
struct MyArchOperand : public MCParsedAsmOperand {

  enum class KindTy {
    Token,
    Register,
    Immediate,
    SystemRegister,
    VType,
  } Kind;

  bool IsRV64;

  struct RegOp {
    MCRegister RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct SysRegOp {
    const char *Data;
    unsigned Length;
    unsigned Encoding;
    // FIXME: Add the Encoding parsed fields as needed for checks,
    // e.g.: read/write or user/supervisor/machine privileges.
  };

  struct VTypeOp {
    unsigned Val;
  };

  SMLoc StartLoc, EndLoc;
  union {
    StringRef Tok;
    RegOp Reg;
    ImmOp Imm;
    struct SysRegOp SysReg;
    struct VTypeOp VType;
  };

  MyArchOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
  MyArchOperand(const MyArchOperand &o) : MCParsedAsmOperand() {
    Kind = o.Kind;
    IsRV64 = o.IsRV64;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
    case KindTy::Register:
      Reg = o.Reg;
      break;
    case KindTy::Immediate:
      Imm = o.Imm;
      break;
    case KindTy::Token:
      Tok = o.Tok;
      break;
    case KindTy::SystemRegister:
      SysReg = o.SysReg;
      break;
    case KindTy::VType:
      VType = o.VType;
      break;
    }
  }

  bool isToken() const override { return Kind == KindTy::Token; }
  bool isReg() const override { return Kind == KindTy::Register; }
  bool isV0Reg() const {
    return Kind == KindTy::Register && Reg.RegNum == MyArch::V0;
  }
  bool isImm() const override { return Kind == KindTy::Immediate; }
  bool isMem() const override { return false; }
  bool isSystemRegister() const { return Kind == KindTy::SystemRegister; }
  bool isVType() const { return Kind == KindTy::VType; }

  bool isGPR() const {
    return Kind == KindTy::Register &&
           MyArchMCRegisterClasses[MyArch::GPRRegClassID].contains(Reg.RegNum);
  }

  static bool evaluateConstantImm(const MCExpr *Expr, int64_t &Imm,
                                  MyArchMCExpr::VariantKind &VK) {
    if (auto *RE = dyn_cast<MyArchMCExpr>(Expr)) {
      VK = RE->getKind();
      return RE->evaluateAsConstant(Imm);
    }

    if (auto CE = dyn_cast<MCConstantExpr>(Expr)) {
      VK = MyArchMCExpr::VK_MyArch_None;
      Imm = CE->getValue();
      return true;
    }

    return false;
  }

  // True if operand is a symbol with no modifiers, or a constant with no
  // modifiers and isShiftedInt<N-1, 1>(Op).
  template <int N> bool isBareSimmNLsb0() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    bool IsValid;
    if (!IsConstantImm)
      IsValid = MyArchAsmParser::classifySymbolRef(getImm(), VK);
    else
      IsValid = isShiftedInt<N - 1, 1>(Imm);
    return IsValid && VK == MyArchMCExpr::VK_MyArch_None;
  }

  // Predicate methods for AsmOperands defined in MyArchInstrInfo.td

  bool isBareSymbol() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return MyArchAsmParser::classifySymbolRef(getImm(), VK) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isCallSymbol() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return MyArchAsmParser::classifySymbolRef(getImm(), VK) &&
           (VK == MyArchMCExpr::VK_MyArch_CALL ||
            VK == MyArchMCExpr::VK_MyArch_CALL_PLT);
  }

  bool isPseudoJumpSymbol() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return MyArchAsmParser::classifySymbolRef(getImm(), VK) &&
           VK == MyArchMCExpr::VK_MyArch_CALL;
  }

  bool isTPRelAddSymbol() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return MyArchAsmParser::classifySymbolRef(getImm(), VK) &&
           VK == MyArchMCExpr::VK_MyArch_TPREL_ADD;
  }

  bool isCSRSystemRegister() const { return isSystemRegister(); }

  bool isVTypeI() const { return isVType(); }

  /// Return true if the operand is a valid for the fence instruction e.g.
  /// ('iorw').
  bool isFenceArg() const {
    if (!isImm())
      return false;
    const MCExpr *Val = getImm();
    auto *SVal = dyn_cast<MCSymbolRefExpr>(Val);
    if (!SVal || SVal->getKind() != MCSymbolRefExpr::VK_None)
      return false;

    StringRef Str = SVal->getSymbol().getName();
    // Letters must be unique, taken from 'iorw', and in ascending order. This
    // holds as long as each individual character is one of 'iorw' and is
    // greater than the previous character.
    char Prev = '\0';
    for (char c : Str) {
      if (c != 'i' && c != 'o' && c != 'r' && c != 'w')
        return false;
      if (c <= Prev)
        return false;
      Prev = c;
    }
    return true;
  }

  /// Return true if the operand is a valid floating point rounding mode.
  bool isFRMArg() const {
    if (!isImm())
      return false;
    const MCExpr *Val = getImm();
    auto *SVal = dyn_cast<MCSymbolRefExpr>(Val);
    if (!SVal || SVal->getKind() != MCSymbolRefExpr::VK_None)
      return false;

    StringRef Str = SVal->getSymbol().getName();

    return MyArchFPRndMode::stringToRoundingMode(Str) != MyArchFPRndMode::Invalid;
  }

  bool isImmXLenLI() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (VK == MyArchMCExpr::VK_MyArch_LO || VK == MyArchMCExpr::VK_MyArch_PCREL_LO)
      return true;
    // Given only Imm, ensuring that the actually specified constant is either
    // a signed or unsigned 64-bit number is unfortunately impossible.
    return IsConstantImm && VK == MyArchMCExpr::VK_MyArch_None &&
           (isRV64() || (isInt<32>(Imm) || isUInt<32>(Imm)));
  }

  bool isUImmLog2XLen() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != MyArchMCExpr::VK_MyArch_None)
      return false;
    return (isRV64() && isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenNonZero() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != MyArchMCExpr::VK_MyArch_None)
      return false;
    if (Imm == 0)
      return false;
    return (isRV64() && isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenHalf() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != MyArchMCExpr::VK_MyArch_None)
      return false;
    return (isRV64() && isUInt<5>(Imm)) || isUInt<4>(Imm);
  }

  bool isUImm5() const {
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isUInt<5>(Imm) && VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm5() const {
    if (!isImm())
      return false;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<5>(Imm) && VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm6() const {
    if (!isImm())
      return false;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<6>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm6NonZero() const {
    if (!isImm())
      return false;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<6>(Imm) && (Imm != 0) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isCLUIImm() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm != 0) &&
           (isUInt<5>(Imm) || (Imm >= 0xfffe0 && Imm <= 0xfffff)) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isUImm7Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<5, 2>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isUImm8Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<6, 2>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isUImm8Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<5, 3>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm9Lsb0() const { return isBareSimmNLsb0<9>(); }

  bool isUImm9Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<6, 3>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isUImm10Lsb00NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<8, 2>(Imm) && (Imm != 0) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm12() const {
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm)
      IsValid = MyArchAsmParser::classifySymbolRef(getImm(), VK);
    else
      IsValid = isInt<12>(Imm);
    return IsValid && ((IsConstantImm && VK == MyArchMCExpr::VK_MyArch_None) ||
                       VK == MyArchMCExpr::VK_MyArch_LO ||
                       VK == MyArchMCExpr::VK_MyArch_PCREL_LO ||
                       VK == MyArchMCExpr::VK_MyArch_TPREL_LO);
  }

  bool isSImm12Lsb0() const { return isBareSimmNLsb0<12>(); }

  bool isSImm13Lsb0() const { return isBareSimmNLsb0<13>(); }

  bool isSImm10Lsb0000NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm != 0) && isShiftedInt<6, 4>(Imm) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isUImm20LUI() const {
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm) {
      IsValid = MyArchAsmParser::classifySymbolRef(getImm(), VK);
      return IsValid && (VK == MyArchMCExpr::VK_MyArch_HI ||
                         VK == MyArchMCExpr::VK_MyArch_TPREL_HI);
    } else {
      return isUInt<20>(Imm) && (VK == MyArchMCExpr::VK_MyArch_None ||
                                 VK == MyArchMCExpr::VK_MyArch_HI ||
                                 VK == MyArchMCExpr::VK_MyArch_TPREL_HI);
    }
  }

  bool isUImm20AUIPC() const {
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm) {
      IsValid = MyArchAsmParser::classifySymbolRef(getImm(), VK);
      return IsValid && (VK == MyArchMCExpr::VK_MyArch_PCREL_HI ||
                         VK == MyArchMCExpr::VK_MyArch_GOT_HI ||
                         VK == MyArchMCExpr::VK_MyArch_TLS_GOT_HI ||
                         VK == MyArchMCExpr::VK_MyArch_TLS_GD_HI);
    } else {
      return isUInt<20>(Imm) && (VK == MyArchMCExpr::VK_MyArch_None ||
                                 VK == MyArchMCExpr::VK_MyArch_PCREL_HI ||
                                 VK == MyArchMCExpr::VK_MyArch_GOT_HI ||
                                 VK == MyArchMCExpr::VK_MyArch_TLS_GOT_HI ||
                                 VK == MyArchMCExpr::VK_MyArch_TLS_GD_HI);
    }
  }

  bool isSImm21Lsb0JAL() const { return isBareSimmNLsb0<21>(); }

  bool isImmZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm == 0) && VK == MyArchMCExpr::VK_MyArch_None;
  }

  bool isSImm5Plus1() const {
    if (!isImm())
      return false;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<5>(Imm - 1) &&
           VK == MyArchMCExpr::VK_MyArch_None;
  }

  /// getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }
  /// True if this operand is for an RV64 instruction
  bool isRV64() const { return IsRV64; }

  unsigned getReg() const override {
    assert(Kind == KindTy::Register && "Invalid type access!");
    return Reg.RegNum.id();
  }

  StringRef getSysReg() const {
    assert(Kind == KindTy::SystemRegister && "Invalid type access!");
    return StringRef(SysReg.Data, SysReg.Length);
  }

  const MCExpr *getImm() const {
    assert(Kind == KindTy::Immediate && "Invalid type access!");
    return Imm.Val;
  }

  StringRef getToken() const {
    assert(Kind == KindTy::Token && "Invalid type access!");
    return Tok;
  }

  unsigned getVType() const {
    assert(Kind == KindTy::VType && "Invalid type access!");
    return VType.Val;
  }

  void print(raw_ostream &OS) const override {
    auto RegName = [](unsigned Reg) {
      if (Reg)
        return MyArchInstPrinter::getRegisterName(Reg);
      else
        return "noreg";
    };

    switch (Kind) {
    case KindTy::Immediate:
      OS << *getImm();
      break;
    case KindTy::Register:
      OS << "<register " << RegName(getReg()) << ">";
      break;
    case KindTy::Token:
      OS << "'" << getToken() << "'";
      break;
    case KindTy::SystemRegister:
      OS << "<sysreg: " << getSysReg() << '>';
      break;
    case KindTy::VType:
      OS << "<vtype: ";
      MyArchVType::printVType(getVType(), OS);
      OS << '>';
      break;
    }
  }

  static std::unique_ptr<MyArchOperand> createToken(StringRef Str, SMLoc S,
                                                   bool IsRV64) {
    auto Op = std::make_unique<MyArchOperand>(KindTy::Token);
    Op->Tok = Str;
    Op->StartLoc = S;
    Op->EndLoc = S;
    Op->IsRV64 = IsRV64;
    return Op;
  }

  static std::unique_ptr<MyArchOperand> createReg(unsigned RegNo, SMLoc S,
                                                 SMLoc E, bool IsRV64) {
    auto Op = std::make_unique<MyArchOperand>(KindTy::Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
    Op->EndLoc = E;
    Op->IsRV64 = IsRV64;
    return Op;
  }

  static std::unique_ptr<MyArchOperand> createImm(const MCExpr *Val, SMLoc S,
                                                 SMLoc E, bool IsRV64) {
    auto Op = std::make_unique<MyArchOperand>(KindTy::Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    Op->IsRV64 = IsRV64;
    return Op;
  }

  static std::unique_ptr<MyArchOperand>
  createSysReg(StringRef Str, SMLoc S, unsigned Encoding, bool IsRV64) {
    auto Op = std::make_unique<MyArchOperand>(KindTy::SystemRegister);
    Op->SysReg.Data = Str.data();
    Op->SysReg.Length = Str.size();
    Op->SysReg.Encoding = Encoding;
    Op->StartLoc = S;
    Op->IsRV64 = IsRV64;
    return Op;
  }

  static std::unique_ptr<MyArchOperand> createVType(unsigned VTypeI, SMLoc S,
                                                   bool IsRV64) {
    auto Op = std::make_unique<MyArchOperand>(KindTy::VType);
    Op->VType.Val = VTypeI;
    Op->StartLoc = S;
    Op->IsRV64 = IsRV64;
    return Op;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    assert(Expr && "Expr shouldn't be null!");
    int64_t Imm = 0;
    MyArchMCExpr::VariantKind VK = MyArchMCExpr::VK_MyArch_None;
    bool IsConstant = evaluateConstantImm(Expr, Imm, VK);

    if (IsConstant)
      Inst.addOperand(MCOperand::createImm(Imm));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    addExpr(Inst, getImm());
  }

  void addFenceArgOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    // isFenceArg has validated the operand, meaning this cast is safe
    auto SE = cast<MCSymbolRefExpr>(getImm());

    unsigned Imm = 0;
    for (char c : SE->getSymbol().getName()) {
      switch (c) {
      default:
        llvm_unreachable("FenceArg must contain only [iorw]");
      case 'i': Imm |= MyArchFenceField::I; break;
      case 'o': Imm |= MyArchFenceField::O; break;
      case 'r': Imm |= MyArchFenceField::R; break;
      case 'w': Imm |= MyArchFenceField::W; break;
      }
    }
    Inst.addOperand(MCOperand::createImm(Imm));
  }

  void addCSRSystemRegisterOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(SysReg.Encoding));
  }

  void addVTypeIOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(getVType()));
  }

  // Returns the rounding mode represented by this MyArchOperand. Should only
  // be called after checking isFRMArg.
  MyArchFPRndMode::RoundingMode getRoundingMode() const {
    // isFRMArg has validated the operand, meaning this cast is safe.
    auto SE = cast<MCSymbolRefExpr>(getImm());
    MyArchFPRndMode::RoundingMode FRM =
        MyArchFPRndMode::stringToRoundingMode(SE->getSymbol().getName());
    assert(FRM != MyArchFPRndMode::Invalid && "Invalid rounding mode");
    return FRM;
  }

  void addFRMArgOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(getRoundingMode()));
  }
};
} // end anonymous namespace.

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "MyArchGenAsmMatcher.inc"

static MCRegister convertFPR64ToFPR16(MCRegister Reg) {
  assert(Reg >= MyArch::F0_D && Reg <= MyArch::F31_D && "Invalid register");
  return Reg - MyArch::F0_D + MyArch::F0_H;
}

static MCRegister convertFPR64ToFPR32(MCRegister Reg) {
  assert(Reg >= MyArch::F0_D && Reg <= MyArch::F31_D && "Invalid register");
  return Reg - MyArch::F0_D + MyArch::F0_F;
}

unsigned MyArchAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                    unsigned Kind) {
  MyArchOperand &Op = static_cast<MyArchOperand &>(AsmOp);
  if (!Op.isReg())
    return Match_InvalidOperand;

  MCRegister Reg = Op.getReg();
  bool IsRegFPR64 =
      MyArchMCRegisterClasses[MyArch::FPR64RegClassID].contains(Reg);
  bool IsRegFPR64C =
      MyArchMCRegisterClasses[MyArch::FPR64CRegClassID].contains(Reg);

  // As the parser couldn't differentiate an FPR32 from an FPR64, coerce the
  // register from FPR64 to FPR32 or FPR64C to FPR32C if necessary.
  if ((IsRegFPR64 && Kind == MCK_FPR32) ||
      (IsRegFPR64C && Kind == MCK_FPR32C)) {
    Op.Reg.RegNum = convertFPR64ToFPR32(Reg);
    return Match_Success;
  }
  // As the parser couldn't differentiate an FPR16 from an FPR64, coerce the
  // register from FPR64 to FPR16 if necessary.
  if (IsRegFPR64 && Kind == MCK_FPR16) {
    Op.Reg.RegNum = convertFPR64ToFPR16(Reg);
    return Match_Success;
  }
  return Match_InvalidOperand;
}

bool MyArchAsmParser::generateImmOutOfRangeError(
    OperandVector &Operands, uint64_t ErrorInfo, int64_t Lower, int64_t Upper,
    Twine Msg = "immediate must be an integer in the range") {
  SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
  return Error(ErrorLoc, Msg + " [" + Twine(Lower) + ", " + Twine(Upper) + "]");
}

static std::string MyArchMnemonicSpellCheck(StringRef S,
                                          const FeatureBitset &FBS,
                                          unsigned VariantID = 0);

bool MyArchAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) {
  MCInst Inst;
  FeatureBitset MissingFeatures;

  auto Result =
    MatchInstructionImpl(Operands, Inst, ErrorInfo, MissingFeatures,
                         MatchingInlineAsm);
  switch (Result) {
  default:
    break;
  case Match_Success:
    if (validateInstruction(Inst, Operands))
      return true;
    return processInstruction(Inst, IDLoc, Operands, Out);
  case Match_MissingFeature: {
    assert(MissingFeatures.any() && "Unknown missing features!");
    bool FirstFeature = true;
    std::string Msg = "instruction requires the following:";
    for (unsigned i = 0, e = MissingFeatures.size(); i != e; ++i) {
      if (MissingFeatures[i]) {
        Msg += FirstFeature ? " " : ", ";
        Msg += getSubtargetFeatureName(i);
        FirstFeature = false;
      }
    }
    return Error(IDLoc, Msg);
  }
  case Match_MnemonicFail: {
    FeatureBitset FBS = ComputeAvailableFeatures(getSTI().getFeatureBits());
    std::string Suggestion = MyArchMnemonicSpellCheck(
      ((MyArchOperand &)*Operands[0]).getToken(), FBS);
    return Error(IDLoc, "unrecognized instruction mnemonic" + Suggestion);
  }
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");

      ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  }

  // Handle the case when the error message is of specific type
  // other than the generic Match_InvalidOperand, and the
  // corresponding operand is missing.
  if (Result > FIRST_TARGET_MATCH_RESULT_TY) {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U && ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");
  }

  switch(Result) {
  default:
    break;
  case Match_InvalidImmXLenLI:
    if (isRV64()) {
      SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
      return Error(ErrorLoc, "operand must be a constant 64-bit integer");
    }
    return generateImmOutOfRangeError(Operands, ErrorInfo,
                                      std::numeric_limits<int32_t>::min(),
                                      std::numeric_limits<uint32_t>::max());
  case Match_InvalidImmZero: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "immediate must be zero");
  }
  case Match_InvalidUImmLog2XLen:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 6) - 1);
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
  case Match_InvalidUImmLog2XLenNonZero:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 1, (1 << 6) - 1);
    return generateImmOutOfRangeError(Operands, ErrorInfo, 1, (1 << 5) - 1);
  case Match_InvalidUImmLog2XLenHalf:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 4) - 1);
  case Match_InvalidUImm5:
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
  case Match_InvalidSImm5:
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 4),
                                      (1 << 4) - 1);
  case Match_InvalidSImm6:
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 5),
                                      (1 << 5) - 1);
  case Match_InvalidSImm6NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 5), (1 << 5) - 1,
        "immediate must be non-zero in the range");
  case Match_InvalidCLUIImm:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 1, (1 << 5) - 1,
        "immediate must be in [0xfffe0, 0xfffff] or");
  case Match_InvalidUImm7Lsb00:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 7) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidUImm8Lsb00:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 8) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidUImm8Lsb000:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 8) - 8,
        "immediate must be a multiple of 8 bytes in the range");
  case Match_InvalidSImm9Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 8), (1 << 8) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidUImm9Lsb000:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 9) - 8,
        "immediate must be a multiple of 8 bytes in the range");
  case Match_InvalidUImm10Lsb00NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 4, (1 << 10) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidSImm10Lsb0000NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 9), (1 << 9) - 16,
        "immediate must be a multiple of 16 bytes and non-zero in the range");
  case Match_InvalidSImm12:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 11), (1 << 11) - 1,
        "operand must be a symbol with %lo/%pcrel_lo/%tprel_lo modifier or an "
        "integer in the range");
  case Match_InvalidSImm12Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 11), (1 << 11) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidSImm13Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 12), (1 << 12) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidUImm20LUI:
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 20) - 1,
                                      "operand must be a symbol with "
                                      "%hi/%tprel_hi modifier or an integer in "
                                      "the range");
  case Match_InvalidUImm20AUIPC:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 20) - 1,
        "operand must be a symbol with a "
        "%pcrel_hi/%got_pcrel_hi/%tls_ie_pcrel_hi/%tls_gd_pcrel_hi modifier or "
        "an integer in the range");
  case Match_InvalidSImm21Lsb0JAL:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 20), (1 << 20) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidCSRSystemRegister: {
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 12) - 1,
                                      "operand must be a valid system register "
                                      "name or an integer in the range");
  }
  case Match_InvalidFenceArg: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be formed of letters selected in-order from 'iorw'");
  }
  case Match_InvalidFRMArg: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be a valid floating point rounding mode mnemonic");
  }
  case Match_InvalidBareSymbol: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a bare symbol name");
  }
  case Match_InvalidPseudoJumpSymbol: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a valid jump target");
  }
  case Match_InvalidCallSymbol: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a bare symbol name");
  }
  case Match_InvalidTPRelAddSymbol: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a symbol with %tprel_add modifier");
  }
  case Match_InvalidVTypeI: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be "
        "e[8|16|32|64|128|256|512|1024],m[1|2|4|8|f2|f4|f8],[ta|tu],[ma|mu]");
  }
  case Match_InvalidVMaskRegister: {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be v0.t");
  }
  case Match_InvalidSImm5Plus1: {
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 4) + 1,
                                      (1 << 4),
                                      "immediate must be in the range");
  }
  }

  llvm_unreachable("Unknown match type detected!");
}

// Attempts to match Name as a register (either using the default name or
// alternative ABI names), setting RegNo to the matching register. Upon
// failure, returns true and sets RegNo to 0. If IsRV32E then registers
// x16-x31 will be rejected.
static bool matchRegisterNameHelper(bool IsRV32E, MCRegister &RegNo,
                                    StringRef Name) {
  RegNo = MatchRegisterName(Name);
  // The 16-/32- and 64-bit FPRs have the same asm name. Check that the initial
  // match always matches the 64-bit variant, and not the 16/32-bit one.
  assert(!(RegNo >= MyArch::F0_H && RegNo <= MyArch::F31_H));
  assert(!(RegNo >= MyArch::F0_F && RegNo <= MyArch::F31_F));
  // The default FPR register class is based on the tablegen enum ordering.
  static_assert(MyArch::F0_D < MyArch::F0_H, "FPR matching must be updated");
  static_assert(MyArch::F0_D < MyArch::F0_F, "FPR matching must be updated");
  if (RegNo == MyArch::NoRegister)
    RegNo = MatchRegisterAltName(Name);
  if (IsRV32E && RegNo >= MyArch::X16 && RegNo <= MyArch::X31)
    RegNo = MyArch::NoRegister;
  return RegNo == MyArch::NoRegister;
}

bool MyArchAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                   SMLoc &EndLoc) {
  if (tryParseRegister(RegNo, StartLoc, EndLoc) != MatchOperand_Success)
    return Error(StartLoc, "invalid register name");
  return false;
}

OperandMatchResultTy MyArchAsmParser::tryParseRegister(unsigned &RegNo,
                                                      SMLoc &StartLoc,
                                                      SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  RegNo = 0;
  StringRef Name = getLexer().getTok().getIdentifier();

  if (matchRegisterNameHelper(isRV32E(), (MCRegister &)RegNo, Name))
    return MatchOperand_NoMatch;

  getParser().Lex(); // Eat identifier token.
  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseRegister(OperandVector &Operands,
                                                   bool AllowParens) {
  SMLoc FirstS = getLoc();
  bool HadParens = false;
  AsmToken LParen;

  // If this is an LParen and a parenthesised register name is allowed, parse it
  // atomically.
  if (AllowParens && getLexer().is(AsmToken::LParen)) {
    AsmToken Buf[2];
    size_t ReadCount = getLexer().peekTokens(Buf);
    if (ReadCount == 2 && Buf[1].getKind() == AsmToken::RParen) {
      HadParens = true;
      LParen = getParser().getTok();
      getParser().Lex(); // Eat '('
    }
  }

  switch (getLexer().getKind()) {
  default:
    if (HadParens)
      getLexer().UnLex(LParen);
    return MatchOperand_NoMatch;
  case AsmToken::Identifier:
    StringRef Name = getLexer().getTok().getIdentifier();
    MCRegister RegNo;
    matchRegisterNameHelper(isRV32E(), RegNo, Name);

    if (RegNo == MyArch::NoRegister) {
      if (HadParens)
        getLexer().UnLex(LParen);
      return MatchOperand_NoMatch;
    }
    if (HadParens)
      Operands.push_back(MyArchOperand::createToken("(", FirstS, isRV64()));
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    getLexer().Lex();
    Operands.push_back(MyArchOperand::createReg(RegNo, S, E, isRV64()));
  }

  if (HadParens) {
    getParser().Lex(); // Eat ')'
    Operands.push_back(MyArchOperand::createToken(")", getLoc(), isRV64()));
  }

  return MatchOperand_Success;
}

OperandMatchResultTy
MyArchAsmParser::parseCSRSystemRegister(OperandVector &Operands) {
  SMLoc S = getLoc();
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Exclaim:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String: {
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;

    auto *CE = dyn_cast<MCConstantExpr>(Res);
    if (CE) {
      int64_t Imm = CE->getValue();
      if (isUInt<12>(Imm)) {
        auto SysReg = MyArchSysReg::lookupSysRegByEncoding(Imm);
        // Accept an immediate representing a named or un-named Sys Reg
        // if the range is valid, regardless of the required features.
        Operands.push_back(MyArchOperand::createSysReg(
            SysReg ? SysReg->Name : "", S, Imm, isRV64()));
        return MatchOperand_Success;
      }
    }

    Twine Msg = "immediate must be an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  case AsmToken::Identifier: {
    StringRef Identifier;
    if (getParser().parseIdentifier(Identifier))
      return MatchOperand_ParseFail;

    auto SysReg = MyArchSysReg::lookupSysRegByName(Identifier);
    if (!SysReg)
      SysReg = MyArchSysReg::lookupSysRegByAltName(Identifier);
    // Accept a named Sys Reg if the required features are present.
    if (SysReg) {
      if (!SysReg->haveRequiredFeatures(getSTI().getFeatureBits())) {
        Error(S, "system register use requires an option to be enabled");
        return MatchOperand_ParseFail;
      }
      Operands.push_back(MyArchOperand::createSysReg(
          Identifier, S, SysReg->Encoding, isRV64()));
      return MatchOperand_Success;
    }

    Twine Msg = "operand must be a valid system register name "
                "or an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  case AsmToken::Percent: {
    // Discard operand with modifier.
    Twine Msg = "immediate must be an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  }

  return MatchOperand_NoMatch;
}

OperandMatchResultTy MyArchAsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Dot:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Exclaim:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String:
  case AsmToken::Identifier:
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;
    break;
  case AsmToken::Percent:
    return parseOperandWithModifier(Operands);
  }

  Operands.push_back(MyArchOperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy
MyArchAsmParser::parseOperandWithModifier(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);

  if (getLexer().getKind() != AsmToken::Percent) {
    Error(getLoc(), "expected '%' for operand modifier");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat '%'

  if (getLexer().getKind() != AsmToken::Identifier) {
    Error(getLoc(), "expected valid identifier for operand modifier");
    return MatchOperand_ParseFail;
  }
  StringRef Identifier = getParser().getTok().getIdentifier();
  MyArchMCExpr::VariantKind VK = MyArchMCExpr::getVariantKindForName(Identifier);
  if (VK == MyArchMCExpr::VK_MyArch_Invalid) {
    Error(getLoc(), "unrecognized operand modifier");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat the identifier
  if (getLexer().getKind() != AsmToken::LParen) {
    Error(getLoc(), "expected '('");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat '('

  const MCExpr *SubExpr;
  if (getParser().parseParenExpression(SubExpr, E)) {
    return MatchOperand_ParseFail;
  }

  const MCExpr *ModExpr = MyArchMCExpr::create(SubExpr, VK, getContext());
  Operands.push_back(MyArchOperand::createImm(ModExpr, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseBareSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getLexer().getKind() != AsmToken::Identifier)
    return MatchOperand_NoMatch;

  StringRef Identifier;
  AsmToken Tok = getLexer().getTok();

  if (getParser().parseIdentifier(Identifier))
    return MatchOperand_ParseFail;

  if (Identifier.consume_back("@plt")) {
    Error(getLoc(), "'@plt' operand not valid for instruction");
    return MatchOperand_ParseFail;
  }

  MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);

  if (Sym->isVariable()) {
    const MCExpr *V = Sym->getVariableValue(/*SetUsed=*/false);
    if (!isa<MCSymbolRefExpr>(V)) {
      getLexer().UnLex(Tok); // Put back if it's not a bare symbol.
      return MatchOperand_NoMatch;
    }
    Res = V;
  } else
    Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());

  MCBinaryExpr::Opcode Opcode;
  switch (getLexer().getKind()) {
  default:
    Operands.push_back(MyArchOperand::createImm(Res, S, E, isRV64()));
    return MatchOperand_Success;
  case AsmToken::Plus:
    Opcode = MCBinaryExpr::Add;
    break;
  case AsmToken::Minus:
    Opcode = MCBinaryExpr::Sub;
    break;
  }

  const MCExpr *Expr;
  if (getParser().parseExpression(Expr))
    return MatchOperand_ParseFail;
  Res = MCBinaryExpr::create(Opcode, Res, Expr, getContext());
  Operands.push_back(MyArchOperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseCallSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getLexer().getKind() != AsmToken::Identifier)
    return MatchOperand_NoMatch;

  // Avoid parsing the register in `call rd, foo` as a call symbol.
  if (getLexer().peekTok().getKind() != AsmToken::EndOfStatement)
    return MatchOperand_NoMatch;

  StringRef Identifier;
  if (getParser().parseIdentifier(Identifier))
    return MatchOperand_ParseFail;

  MyArchMCExpr::VariantKind Kind = MyArchMCExpr::VK_MyArch_CALL;
  if (Identifier.consume_back("@plt"))
    Kind = MyArchMCExpr::VK_MyArch_CALL_PLT;

  MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);
  Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());
  Res = MyArchMCExpr::create(Res, Kind, getContext());
  Operands.push_back(MyArchOperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy
MyArchAsmParser::parsePseudoJumpSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getParser().parseExpression(Res))
    return MatchOperand_ParseFail;

  if (Res->getKind() != MCExpr::ExprKind::SymbolRef ||
      cast<MCSymbolRefExpr>(Res)->getKind() ==
          MCSymbolRefExpr::VariantKind::VK_PLT) {
    Error(S, "operand must be a valid jump target");
    return MatchOperand_ParseFail;
  }

  Res = MyArchMCExpr::create(Res, MyArchMCExpr::VK_MyArch_CALL, getContext());
  Operands.push_back(MyArchOperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseJALOffset(OperandVector &Operands) {
  // Parsing jal operands is fiddly due to the `jal foo` and `jal ra, foo`
  // both being acceptable forms. When parsing `jal ra, foo` this function
  // will be called for the `ra` register operand in an attempt to match the
  // single-operand alias. parseJALOffset must fail for this case. It would
  // seem logical to try parse the operand using parseImmediate and return
  // NoMatch if the next token is a comma (meaning we must be parsing a jal in
  // the second form rather than the first). We can't do this as there's no
  // way of rewinding the lexer state. Instead, return NoMatch if this operand
  // is an identifier and is followed by a comma.
  if (getLexer().is(AsmToken::Identifier) &&
      getLexer().peekTok().is(AsmToken::Comma))
    return MatchOperand_NoMatch;

  return parseImmediate(Operands);
}

OperandMatchResultTy MyArchAsmParser::parseVTypeI(OperandVector &Operands) {
  SMLoc S = getLoc();
  if (getLexer().getKind() != AsmToken::Identifier)
    return MatchOperand_NoMatch;

  // Parse "e8,m1,t[a|u],m[a|u]"
  StringRef Name = getLexer().getTok().getIdentifier();
  if (!Name.consume_front("e"))
    return MatchOperand_NoMatch;
  unsigned Sew;
  if (Name.getAsInteger(10, Sew))
    return MatchOperand_NoMatch;
  if (!MyArchVType::isValidSEW(Sew))
    return MatchOperand_NoMatch;
  getLexer().Lex();

  if (!getLexer().is(AsmToken::Comma))
    return MatchOperand_NoMatch;
  getLexer().Lex();

  Name = getLexer().getTok().getIdentifier();
  if (!Name.consume_front("m"))
    return MatchOperand_NoMatch;
  // "m" or "mf"
  bool Fractional = Name.consume_front("f");
  unsigned Lmul;
  if (Name.getAsInteger(10, Lmul))
    return MatchOperand_NoMatch;
  if (!MyArchVType::isValidLMUL(Lmul, Fractional))
    return MatchOperand_NoMatch;
  getLexer().Lex();

  if (!getLexer().is(AsmToken::Comma))
    return MatchOperand_NoMatch;
  getLexer().Lex();

  Name = getLexer().getTok().getIdentifier();
  // ta or tu
  bool TailAgnostic;
  if (Name == "ta")
    TailAgnostic = true;
  else if (Name == "tu")
    TailAgnostic = false;
  else
    return MatchOperand_NoMatch;
  getLexer().Lex();

  if (!getLexer().is(AsmToken::Comma))
    return MatchOperand_NoMatch;
  getLexer().Lex();

  Name = getLexer().getTok().getIdentifier();
  // ma or mu
  bool MaskAgnostic;
  if (Name == "ma")
    MaskAgnostic = true;
  else if (Name == "mu")
    MaskAgnostic = false;
  else
    return MatchOperand_NoMatch;
  getLexer().Lex();

  if (getLexer().getKind() != AsmToken::EndOfStatement)
    return MatchOperand_NoMatch;

  unsigned SewLog2 = Log2_32(Sew / 8);
  unsigned LmulLog2 = Log2_32(Lmul);
  MyArchVSEW VSEW = static_cast<MyArchVSEW>(SewLog2);
  MyArchVLMUL VLMUL =
      static_cast<MyArchVLMUL>(Fractional ? 8 - LmulLog2 : LmulLog2);

  unsigned VTypeI =
      MyArchVType::encodeVTYPE(VLMUL, VSEW, TailAgnostic, MaskAgnostic);
  Operands.push_back(MyArchOperand::createVType(VTypeI, S, isRV64()));

  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseMaskReg(OperandVector &Operands) {
  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::Identifier:
    StringRef Name = getLexer().getTok().getIdentifier();
    if (!Name.consume_back(".t")) {
      Error(getLoc(), "expected '.t' suffix");
      return MatchOperand_ParseFail;
    }
    MCRegister RegNo;
    matchRegisterNameHelper(isRV32E(), RegNo, Name);

    if (RegNo == MyArch::NoRegister)
      return MatchOperand_NoMatch;
    if (RegNo != MyArch::V0)
      return MatchOperand_NoMatch;
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    getLexer().Lex();
    Operands.push_back(MyArchOperand::createReg(RegNo, S, E, isRV64()));
  }

  return MatchOperand_Success;
}

OperandMatchResultTy
MyArchAsmParser::parseMemOpBaseReg(OperandVector &Operands) {
  if (getLexer().isNot(AsmToken::LParen)) {
    Error(getLoc(), "expected '('");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat '('
  Operands.push_back(MyArchOperand::createToken("(", getLoc(), isRV64()));

  if (parseRegister(Operands) != MatchOperand_Success) {
    Error(getLoc(), "expected register");
    return MatchOperand_ParseFail;
  }

  if (getLexer().isNot(AsmToken::RParen)) {
    Error(getLoc(), "expected ')'");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat ')'
  Operands.push_back(MyArchOperand::createToken(")", getLoc(), isRV64()));

  return MatchOperand_Success;
}

OperandMatchResultTy MyArchAsmParser::parseAtomicMemOp(OperandVector &Operands) {
  // Atomic operations such as lr.w, sc.w, and amo*.w accept a "memory operand"
  // as one of their register operands, such as `(a0)`. This just denotes that
  // the register (in this case `a0`) contains a memory address.
  //
  // Normally, we would be able to parse these by putting the parens into the
  // instruction string. However, GNU as also accepts a zero-offset memory
  // operand (such as `0(a0)`), and ignores the 0. Normally this would be parsed
  // with parseImmediate followed by parseMemOpBaseReg, but these instructions
  // do not accept an immediate operand, and we do not want to add a "dummy"
  // operand that is silently dropped.
  //
  // Instead, we use this custom parser. This will: allow (and discard) an
  // offset if it is zero; require (and discard) parentheses; and add only the
  // parsed register operand to `Operands`.
  //
  // These operands are printed with MyArchInstPrinter::printAtomicMemOp, which
  // will only print the register surrounded by parentheses (which GNU as also
  // uses as its canonical representation for these operands).
  std::unique_ptr<MyArchOperand> OptionalImmOp;

  if (getLexer().isNot(AsmToken::LParen)) {
    // Parse an Integer token. We do not accept arbritrary constant expressions
    // in the offset field (because they may include parens, which complicates
    // parsing a lot).
    int64_t ImmVal;
    SMLoc ImmStart = getLoc();
    if (getParser().parseIntToken(ImmVal,
                                  "expected '(' or optional integer offset"))
      return MatchOperand_ParseFail;

    // Create a MyArchOperand for checking later (so the error messages are
    // nicer), but we don't add it to Operands.
    SMLoc ImmEnd = getLoc();
    OptionalImmOp =
        MyArchOperand::createImm(MCConstantExpr::create(ImmVal, getContext()),
                                ImmStart, ImmEnd, isRV64());
  }

  if (getLexer().isNot(AsmToken::LParen)) {
    Error(getLoc(), OptionalImmOp ? "expected '(' after optional integer offset"
                                  : "expected '(' or optional integer offset");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat '('

  if (parseRegister(Operands) != MatchOperand_Success) {
    Error(getLoc(), "expected register");
    return MatchOperand_ParseFail;
  }

  if (getLexer().isNot(AsmToken::RParen)) {
    Error(getLoc(), "expected ')'");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat ')'

  // Deferred Handling of non-zero offsets. This makes the error messages nicer.
  if (OptionalImmOp && !OptionalImmOp->isImmZero()) {
    Error(OptionalImmOp->getStartLoc(), "optional integer offset must be 0",
          SMRange(OptionalImmOp->getStartLoc(), OptionalImmOp->getEndLoc()));
    return MatchOperand_ParseFail;
  }

  return MatchOperand_Success;
}

/// Looks at a token type and creates the relevant operand from this
/// information, adding to Operands. If operand was parsed, returns false, else
/// true.
bool MyArchAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic) {
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy Result =
      MatchOperandParserImpl(Operands, Mnemonic, /*ParseForAllFeatures=*/true);
  if (Result == MatchOperand_Success)
    return false;
  if (Result == MatchOperand_ParseFail)
    return true;

  // Attempt to parse token as a register.
  if (parseRegister(Operands, true) == MatchOperand_Success)
    return false;

  // Attempt to parse token as an immediate
  if (parseImmediate(Operands) == MatchOperand_Success) {
    // Parse memory base register if present
    if (getLexer().is(AsmToken::LParen))
      return parseMemOpBaseReg(Operands) != MatchOperand_Success;
    return false;
  }

  // Finally we have exhausted all options and must declare defeat.
  Error(getLoc(), "unknown operand");
  return true;
}

bool MyArchAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {
  // Ensure that if the instruction occurs when relaxation is enabled,
  // relocations are forced for the file. Ideally this would be done when there
  // is enough information to reliably determine if the instruction itself may
  // cause relaxations. Unfortunately instruction processing stage occurs in the
  // same pass as relocation emission, so it's too late to set a 'sticky bit'
  // for the entire file.
  if (getSTI().getFeatureBits()[MyArch::FeatureRelax]) {
    auto *Assembler = getTargetStreamer().getStreamer().getAssemblerPtr();
    if (Assembler != nullptr) {
      MyArchAsmBackend &MAB =
          static_cast<MyArchAsmBackend &>(Assembler->getBackend());
      MAB.setForceRelocs();
    }
  }

  // First operand is token for instruction
  Operands.push_back(MyArchOperand::createToken(Name, NameLoc, isRV64()));

  // If there are no more operands, then finish
  if (getLexer().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (parseOperand(Operands, Name))
    return true;

  // Parse until end of statement, consuming commas between operands
  unsigned OperandIdx = 1;
  while (getLexer().is(AsmToken::Comma)) {
    // Consume comma token
    getLexer().Lex();

    // Parse next operand
    if (parseOperand(Operands, Name))
      return true;

    ++OperandIdx;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool MyArchAsmParser::classifySymbolRef(const MCExpr *Expr,
                                       MyArchMCExpr::VariantKind &Kind) {
  Kind = MyArchMCExpr::VK_MyArch_None;

  if (const MyArchMCExpr *RE = dyn_cast<MyArchMCExpr>(Expr)) {
    Kind = RE->getKind();
    Expr = RE->getSubExpr();
  }

  MCValue Res;
  MCFixup Fixup;
  if (Expr->evaluateAsRelocatable(Res, nullptr, &Fixup))
    return Res.getRefKind() == MyArchMCExpr::VK_MyArch_None;
  return false;
}

bool MyArchAsmParser::ParseDirective(AsmToken DirectiveID) {
  // This returns false if this function recognizes the directive
  // regardless of whether it is successfully handles or reports an
  // error. Otherwise it returns true to give the generic parser a
  // chance at recognizing it.
  StringRef IDVal = DirectiveID.getString();

  if (IDVal == ".option")
    return parseDirectiveOption();
  else if (IDVal == ".attribute")
    return parseDirectiveAttribute();

  return true;
}

bool MyArchAsmParser::parseDirectiveOption() {
  MCAsmParser &Parser = getParser();
  // Get the option token.
  AsmToken Tok = Parser.getTok();
  // At the moment only identifiers are supported.
  if (Tok.isNot(AsmToken::Identifier))
    return Error(Parser.getTok().getLoc(),
                 "unexpected token, expected identifier");

  StringRef Option = Tok.getIdentifier();

  if (Option == "push") {
    getTargetStreamer().emitDirectiveOptionPush();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    pushFeatureBits();
    return false;
  }

  if (Option == "pop") {
    SMLoc StartLoc = Parser.getTok().getLoc();
    getTargetStreamer().emitDirectiveOptionPop();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    if (popFeatureBits())
      return Error(StartLoc, ".option pop with no .option push");

    return false;
  }

  if (Option == "rvc") {
    getTargetStreamer().emitDirectiveOptionRVC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    setFeatureBits(MyArch::FeatureStdExtC, "c");
    return false;
  }

  if (Option == "norvc") {
    getTargetStreamer().emitDirectiveOptionNoRVC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    clearFeatureBits(MyArch::FeatureStdExtC, "c");
    return false;
  }

  if (Option == "pic") {
    getTargetStreamer().emitDirectiveOptionPIC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    ParserOptions.IsPicEnabled = true;
    return false;
  }

  if (Option == "nopic") {
    getTargetStreamer().emitDirectiveOptionNoPIC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    ParserOptions.IsPicEnabled = false;
    return false;
  }

  if (Option == "relax") {
    getTargetStreamer().emitDirectiveOptionRelax();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    setFeatureBits(MyArch::FeatureRelax, "relax");
    return false;
  }

  if (Option == "norelax") {
    getTargetStreamer().emitDirectiveOptionNoRelax();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    clearFeatureBits(MyArch::FeatureRelax, "relax");
    return false;
  }

  // Unknown option.
  Warning(Parser.getTok().getLoc(),
          "unknown option, expected 'push', 'pop', 'rvc', 'norvc', 'relax' or "
          "'norelax'");
  Parser.eatToEndOfStatement();
  return false;
}

/// parseDirectiveAttribute
///  ::= .attribute expression ',' ( expression | "string" )
///  ::= .attribute identifier ',' ( expression | "string" )
bool MyArchAsmParser::parseDirectiveAttribute() {
  MCAsmParser &Parser = getParser();
  int64_t Tag;
  SMLoc TagLoc;
  TagLoc = Parser.getTok().getLoc();
  if (Parser.getTok().is(AsmToken::Identifier)) {
    StringRef Name = Parser.getTok().getIdentifier();
    Optional<unsigned> Ret =
        ELFAttrs::attrTypeFromString(Name, MyArchAttrs::MyArchAttributeTags);
    if (!Ret.hasValue()) {
      Error(TagLoc, "attribute name not recognised: " + Name);
      return false;
    }
    Tag = Ret.getValue();
    Parser.Lex();
  } else {
    const MCExpr *AttrExpr;

    TagLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(AttrExpr))
      return true;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(AttrExpr);
    if (check(!CE, TagLoc, "expected numeric constant"))
      return true;

    Tag = CE->getValue();
  }

  if (Parser.parseToken(AsmToken::Comma, "comma expected"))
    return true;

  StringRef StringValue;
  int64_t IntegerValue = 0;
  bool IsIntegerValue = true;

  // RISC-V attributes have a string value if the tag number is odd
  // and an integer value if the tag number is even.
  if (Tag % 2)
    IsIntegerValue = false;

  SMLoc ValueExprLoc = Parser.getTok().getLoc();
  if (IsIntegerValue) {
    const MCExpr *ValueExpr;
    if (Parser.parseExpression(ValueExpr))
      return true;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(ValueExpr);
    if (!CE)
      return Error(ValueExprLoc, "expected numeric constant");
    IntegerValue = CE->getValue();
  } else {
    if (Parser.getTok().isNot(AsmToken::String))
      return Error(Parser.getTok().getLoc(), "expected string constant");

    StringValue = Parser.getTok().getStringContents();
    Parser.Lex();
  }

  if (Parser.parseToken(AsmToken::EndOfStatement,
                        "unexpected token in '.attribute' directive"))
    return true;

  if (Tag == MyArchAttrs::ARCH) {
    StringRef Arch = StringValue;
    if (Arch.consume_front("rv32"))
      clearFeatureBits(MyArch::Feature64Bit, "64bit");
    else if (Arch.consume_front("rv64"))
      setFeatureBits(MyArch::Feature64Bit, "64bit");
    else
      return Error(ValueExprLoc, "bad arch string " + Arch);

    // .attribute arch overrides the current architecture, so unset all
    // currently enabled extensions
    clearFeatureBits(MyArch::FeatureRV32E, "e");
    clearFeatureBits(MyArch::FeatureStdExtM, "m");
    clearFeatureBits(MyArch::FeatureStdExtA, "a");
    clearFeatureBits(MyArch::FeatureStdExtF, "f");
    clearFeatureBits(MyArch::FeatureStdExtD, "d");
    clearFeatureBits(MyArch::FeatureStdExtC, "c");
    clearFeatureBits(MyArch::FeatureStdExtB, "experimental-b");
    clearFeatureBits(MyArch::FeatureStdExtV, "experimental-v");
    clearFeatureBits(MyArch::FeatureExtZfh, "experimental-zfh");
    clearFeatureBits(MyArch::FeatureExtZba, "experimental-zba");
    clearFeatureBits(MyArch::FeatureExtZbb, "experimental-zbb");
    clearFeatureBits(MyArch::FeatureExtZbc, "experimental-zbc");
    clearFeatureBits(MyArch::FeatureExtZbe, "experimental-zbe");
    clearFeatureBits(MyArch::FeatureExtZbf, "experimental-zbf");
    clearFeatureBits(MyArch::FeatureExtZbm, "experimental-zbm");
    clearFeatureBits(MyArch::FeatureExtZbp, "experimental-zbp");
    clearFeatureBits(MyArch::FeatureExtZbproposedc, "experimental-zbproposedc");
    clearFeatureBits(MyArch::FeatureExtZbr, "experimental-zbr");
    clearFeatureBits(MyArch::FeatureExtZbs, "experimental-zbs");
    clearFeatureBits(MyArch::FeatureExtZbt, "experimental-zbt");
    clearFeatureBits(MyArch::FeatureExtZvamo, "experimental-zvamo");
    clearFeatureBits(MyArch::FeatureStdExtZvlsseg, "experimental-zvlsseg");

    while (!Arch.empty()) {
      bool DropFirst = true;
      if (Arch[0] == 'i')
        clearFeatureBits(MyArch::FeatureRV32E, "e");
      else if (Arch[0] == 'e')
        setFeatureBits(MyArch::FeatureRV32E, "e");
      else if (Arch[0] == 'g') {
        clearFeatureBits(MyArch::FeatureRV32E, "e");
        setFeatureBits(MyArch::FeatureStdExtM, "m");
        setFeatureBits(MyArch::FeatureStdExtA, "a");
        setFeatureBits(MyArch::FeatureStdExtF, "f");
        setFeatureBits(MyArch::FeatureStdExtD, "d");
      } else if (Arch[0] == 'm')
        setFeatureBits(MyArch::FeatureStdExtM, "m");
      else if (Arch[0] == 'a')
        setFeatureBits(MyArch::FeatureStdExtA, "a");
      else if (Arch[0] == 'f')
        setFeatureBits(MyArch::FeatureStdExtF, "f");
      else if (Arch[0] == 'd') {
        setFeatureBits(MyArch::FeatureStdExtF, "f");
        setFeatureBits(MyArch::FeatureStdExtD, "d");
      } else if (Arch[0] == 'c') {
        setFeatureBits(MyArch::FeatureStdExtC, "c");
      } else if (Arch[0] == 'b') {
        setFeatureBits(MyArch::FeatureStdExtB, "experimental-b");
      } else if (Arch[0] == 'v') {
        setFeatureBits(MyArch::FeatureStdExtV, "experimental-v");
      } else if (Arch[0] == 's' || Arch[0] == 'x' || Arch[0] == 'z') {
        StringRef Ext =
            Arch.take_until([](char c) { return ::isdigit(c) || c == '_'; });
        if (Ext == "zba")
          setFeatureBits(MyArch::FeatureExtZba, "experimental-zba");
        else if (Ext == "zbb")
          setFeatureBits(MyArch::FeatureExtZbb, "experimental-zbb");
        else if (Ext == "zbc")
          setFeatureBits(MyArch::FeatureExtZbc, "experimental-zbc");
        else if (Ext == "zbe")
          setFeatureBits(MyArch::FeatureExtZbe, "experimental-zbe");
        else if (Ext == "zbf")
          setFeatureBits(MyArch::FeatureExtZbf, "experimental-zbf");
        else if (Ext == "zbm")
          setFeatureBits(MyArch::FeatureExtZbm, "experimental-zbm");
        else if (Ext == "zbp")
          setFeatureBits(MyArch::FeatureExtZbp, "experimental-zbp");
        else if (Ext == "zbproposedc")
          setFeatureBits(MyArch::FeatureExtZbproposedc,
                         "experimental-zbproposedc");
        else if (Ext == "zbr")
          setFeatureBits(MyArch::FeatureExtZbr, "experimental-zbr");
        else if (Ext == "zbs")
          setFeatureBits(MyArch::FeatureExtZbs, "experimental-zbs");
        else if (Ext == "zbt")
          setFeatureBits(MyArch::FeatureExtZbt, "experimental-zbt");
        else if (Ext == "zfh")
          setFeatureBits(MyArch::FeatureExtZfh, "experimental-zfh");
        else if (Ext == "zvamo")
          setFeatureBits(MyArch::FeatureExtZvamo, "experimental-zvamo");
        else if (Ext == "zvlsseg")
          setFeatureBits(MyArch::FeatureStdExtZvlsseg, "experimental-zvlsseg");
        else
          return Error(ValueExprLoc, "bad arch string " + Ext);
        Arch = Arch.drop_until([](char c) { return ::isdigit(c) || c == '_'; });
        DropFirst = false;
      } else
        return Error(ValueExprLoc, "bad arch string " + Arch);

      if (DropFirst)
        Arch = Arch.drop_front(1);
      int major = 0;
      int minor = 0;
      Arch.consumeInteger(10, major);
      Arch.consume_front("p");
      Arch.consumeInteger(10, minor);
      Arch = Arch.drop_while([](char c) { return c == '_'; });
    }
  }

  if (IsIntegerValue)
    getTargetStreamer().emitAttribute(Tag, IntegerValue);
  else {
    if (Tag != MyArchAttrs::ARCH) {
      getTargetStreamer().emitTextAttribute(Tag, StringValue);
    } else {
      std::string formalArchStr = "rv32";
      if (getFeatureBits(MyArch::Feature64Bit))
        formalArchStr = "rv64";
      if (getFeatureBits(MyArch::FeatureRV32E))
        formalArchStr = (Twine(formalArchStr) + "e1p9").str();
      else
        formalArchStr = (Twine(formalArchStr) + "i2p0").str();

      if (getFeatureBits(MyArch::FeatureStdExtM))
        formalArchStr = (Twine(formalArchStr) + "_m2p0").str();
      if (getFeatureBits(MyArch::FeatureStdExtA))
        formalArchStr = (Twine(formalArchStr) + "_a2p0").str();
      if (getFeatureBits(MyArch::FeatureStdExtF))
        formalArchStr = (Twine(formalArchStr) + "_f2p0").str();
      if (getFeatureBits(MyArch::FeatureStdExtD))
        formalArchStr = (Twine(formalArchStr) + "_d2p0").str();
      if (getFeatureBits(MyArch::FeatureStdExtC))
        formalArchStr = (Twine(formalArchStr) + "_c2p0").str();
      if (getFeatureBits(MyArch::FeatureStdExtB))
        formalArchStr = (Twine(formalArchStr) + "_b0p93").str();
      if (getFeatureBits(MyArch::FeatureStdExtV))
        formalArchStr = (Twine(formalArchStr) + "_v0p10").str();
      if (getFeatureBits(MyArch::FeatureExtZfh))
        formalArchStr = (Twine(formalArchStr) + "_zfh0p1").str();
      if (getFeatureBits(MyArch::FeatureExtZba))
        formalArchStr = (Twine(formalArchStr) + "_zba0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbb))
        formalArchStr = (Twine(formalArchStr) + "_zbb0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbc))
        formalArchStr = (Twine(formalArchStr) + "_zbc0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbe))
        formalArchStr = (Twine(formalArchStr) + "_zbe0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbf))
        formalArchStr = (Twine(formalArchStr) + "_zbf0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbm))
        formalArchStr = (Twine(formalArchStr) + "_zbm0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbp))
        formalArchStr = (Twine(formalArchStr) + "_zbp0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbproposedc))
        formalArchStr = (Twine(formalArchStr) + "_zbproposedc0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbr))
        formalArchStr = (Twine(formalArchStr) + "_zbr0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbs))
        formalArchStr = (Twine(formalArchStr) + "_zbs0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZbt))
        formalArchStr = (Twine(formalArchStr) + "_zbt0p93").str();
      if (getFeatureBits(MyArch::FeatureExtZvamo))
        formalArchStr = (Twine(formalArchStr) + "_zvamo0p10").str();
      if (getFeatureBits(MyArch::FeatureStdExtZvlsseg))
        formalArchStr = (Twine(formalArchStr) + "_zvlsseg0p10").str();

      getTargetStreamer().emitTextAttribute(Tag, formalArchStr);
    }
  }

  return false;
}

void MyArchAsmParser::emitToStreamer(MCStreamer &S, const MCInst &Inst) {
  MCInst CInst;
  bool Res = compressInst(CInst, Inst, getSTI(), S.getContext());
  if (Res)
    ++MyArchNumInstrsCompressed;
  S.emitInstruction((Res ? CInst : Inst), getSTI());
}

void MyArchAsmParser::emitLoadImm(MCRegister DestReg, int64_t Value,
                                 MCStreamer &Out) {
  MyArchMatInt::InstSeq Seq;
  MyArchMatInt::generateInstSeq(Value, isRV64(), Seq);

  MCRegister SrcReg = MyArch::X0;
  for (MyArchMatInt::Inst &Inst : Seq) {
    if (Inst.Opc == MyArch::LUI) {
      emitToStreamer(
          Out, MCInstBuilder(MyArch::LUI).addReg(DestReg).addImm(Inst.Imm));
    } else {
      emitToStreamer(
          Out, MCInstBuilder(Inst.Opc).addReg(DestReg).addReg(SrcReg).addImm(
                   Inst.Imm));
    }

    // Only the first instruction has X0 as its source.
    SrcReg = DestReg;
  }
}

void MyArchAsmParser::emitAuipcInstPair(MCOperand DestReg, MCOperand TmpReg,
                                       const MCExpr *Symbol,
                                       MyArchMCExpr::VariantKind VKHi,
                                       unsigned SecondOpcode, SMLoc IDLoc,
                                       MCStreamer &Out) {
  // A pair of instructions for PC-relative addressing; expands to
  //   TmpLabel: AUIPC TmpReg, VKHi(symbol)
  //             OP DestReg, TmpReg, %pcrel_lo(TmpLabel)
  MCContext &Ctx = getContext();

  MCSymbol *TmpLabel = Ctx.createNamedTempSymbol("pcrel_hi");
  Out.emitLabel(TmpLabel);

  const MyArchMCExpr *SymbolHi = MyArchMCExpr::create(Symbol, VKHi, Ctx);
  emitToStreamer(
      Out, MCInstBuilder(MyArch::AUIPC).addOperand(TmpReg).addExpr(SymbolHi));

  const MCExpr *RefToLinkTmpLabel =
      MyArchMCExpr::create(MCSymbolRefExpr::create(TmpLabel, Ctx),
                          MyArchMCExpr::VK_MyArch_PCREL_LO, Ctx);

  emitToStreamer(Out, MCInstBuilder(SecondOpcode)
                          .addOperand(DestReg)
                          .addOperand(TmpReg)
                          .addExpr(RefToLinkTmpLabel));
}

void MyArchAsmParser::emitLoadLocalAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load local address pseudo-instruction "lla" is used in PC-relative
  // addressing of local symbols:
  //   lla rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  emitAuipcInstPair(DestReg, DestReg, Symbol, MyArchMCExpr::VK_MyArch_PCREL_HI,
                    MyArch::ADDI, IDLoc, Out);
}

void MyArchAsmParser::emitLoadAddress(MCInst &Inst, SMLoc IDLoc,
                                     MCStreamer &Out) {
  // The load address pseudo-instruction "la" is used in PC-relative and
  // GOT-indirect addressing of global symbols:
  //   la rdest, symbol
  // expands to either (for non-PIC)
  //   TmpLabel: AUIPC rdest, %pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  // or (for PIC)
  //   TmpLabel: AUIPC rdest, %got_pcrel_hi(symbol)
  //             Lx rdest, %pcrel_lo(TmpLabel)(rdest)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  unsigned SecondOpcode;
  MyArchMCExpr::VariantKind VKHi;
  if (ParserOptions.IsPicEnabled) {
    SecondOpcode = isRV64() ? MyArch::LD : MyArch::LW;
    VKHi = MyArchMCExpr::VK_MyArch_GOT_HI;
  } else {
    SecondOpcode = MyArch::ADDI;
    VKHi = MyArchMCExpr::VK_MyArch_PCREL_HI;
  }
  emitAuipcInstPair(DestReg, DestReg, Symbol, VKHi, SecondOpcode, IDLoc, Out);
}

void MyArchAsmParser::emitLoadTLSIEAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load TLS IE address pseudo-instruction "la.tls.ie" is used in
  // initial-exec TLS model addressing of global symbols:
  //   la.tls.ie rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %tls_ie_pcrel_hi(symbol)
  //             Lx rdest, %pcrel_lo(TmpLabel)(rdest)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  unsigned SecondOpcode = isRV64() ? MyArch::LD : MyArch::LW;
  emitAuipcInstPair(DestReg, DestReg, Symbol, MyArchMCExpr::VK_MyArch_TLS_GOT_HI,
                    SecondOpcode, IDLoc, Out);
}

void MyArchAsmParser::emitLoadTLSGDAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load TLS GD address pseudo-instruction "la.tls.gd" is used in
  // global-dynamic TLS model addressing of global symbols:
  //   la.tls.gd rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %tls_gd_pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  emitAuipcInstPair(DestReg, DestReg, Symbol, MyArchMCExpr::VK_MyArch_TLS_GD_HI,
                    MyArch::ADDI, IDLoc, Out);
}

void MyArchAsmParser::emitLoadStoreSymbol(MCInst &Inst, unsigned Opcode,
                                         SMLoc IDLoc, MCStreamer &Out,
                                         bool HasTmpReg) {
  // The load/store pseudo-instruction does a pc-relative load with
  // a symbol.
  //
  // The expansion looks like this
  //
  //   TmpLabel: AUIPC tmp, %pcrel_hi(symbol)
  //             [S|L]X    rd, %pcrel_lo(TmpLabel)(tmp)
  MCOperand DestReg = Inst.getOperand(0);
  unsigned SymbolOpIdx = HasTmpReg ? 2 : 1;
  unsigned TmpRegOpIdx = HasTmpReg ? 1 : 0;
  MCOperand TmpReg = Inst.getOperand(TmpRegOpIdx);
  const MCExpr *Symbol = Inst.getOperand(SymbolOpIdx).getExpr();
  emitAuipcInstPair(DestReg, TmpReg, Symbol, MyArchMCExpr::VK_MyArch_PCREL_HI,
                    Opcode, IDLoc, Out);
}

void MyArchAsmParser::emitPseudoExtend(MCInst &Inst, bool SignExtend,
                                      int64_t Width, SMLoc IDLoc,
                                      MCStreamer &Out) {
  // The sign/zero extend pseudo-instruction does two shifts, with the shift
  // amounts dependent on the XLEN.
  //
  // The expansion looks like this
  //
  //    SLLI rd, rs, XLEN - Width
  //    SR[A|R]I rd, rd, XLEN - Width
  MCOperand DestReg = Inst.getOperand(0);
  MCOperand SourceReg = Inst.getOperand(1);

  unsigned SecondOpcode = SignExtend ? MyArch::SRAI : MyArch::SRLI;
  int64_t ShAmt = (isRV64() ? 64 : 32) - Width;

  assert(ShAmt > 0 && "Shift amount must be non-zero.");

  emitToStreamer(Out, MCInstBuilder(MyArch::SLLI)
                          .addOperand(DestReg)
                          .addOperand(SourceReg)
                          .addImm(ShAmt));

  emitToStreamer(Out, MCInstBuilder(SecondOpcode)
                          .addOperand(DestReg)
                          .addOperand(DestReg)
                          .addImm(ShAmt));
}

void MyArchAsmParser::emitVMSGE(MCInst &Inst, unsigned Opcode, SMLoc IDLoc,
                               MCStreamer &Out) {
  if (Inst.getNumOperands() == 3) {
    // unmasked va >= x
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x
    //  expansion: vmslt{u}.vx vd, va, x; vmnand.mm vd, vd, vd
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addReg(MyArch::NoRegister));
    emitToStreamer(Out, MCInstBuilder(MyArch::VMNAND_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0)));
  } else if (Inst.getNumOperands() == 4) {
    // masked va >= x, vd != v0
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x, v0.t
    //  expansion: vmslt{u}.vx vd, va, x, v0.t; vmxor.mm vd, vd, v0
    assert(Inst.getOperand(0).getReg() != MyArch::V0 &&
           "The destination register should not be V0.");
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addOperand(Inst.getOperand(3)));
    emitToStreamer(Out, MCInstBuilder(MyArch::VMXOR_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addReg(MyArch::V0));
  } else if (Inst.getNumOperands() == 5) {
    // masked va >= x, vd == v0
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x, v0.t, vt
    //  expansion: vmslt{u}.vx vt, va, x;  vmandnot.mm vd, vd, vt
    assert(Inst.getOperand(0).getReg() == MyArch::V0 &&
           "The destination register should be V0.");
    assert(Inst.getOperand(1).getReg() != MyArch::V0 &&
           "The temporary vector register should not be V0.");
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addOperand(Inst.getOperand(3))
                            .addOperand(Inst.getOperand(4)));
    emitToStreamer(Out, MCInstBuilder(MyArch::VMANDNOT_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1)));
  }
}

bool MyArchAsmParser::checkPseudoAddTPRel(MCInst &Inst,
                                         OperandVector &Operands) {
  assert(Inst.getOpcode() == MyArch::PseudoAddTPRel && "Invalid instruction");
  assert(Inst.getOperand(2).isReg() && "Unexpected second operand kind");
  if (Inst.getOperand(2).getReg() != MyArch::X4) {
    SMLoc ErrorLoc = ((MyArchOperand &)*Operands[3]).getStartLoc();
    return Error(ErrorLoc, "the second input operand must be tp/x4 when using "
                           "%tprel_add modifier");
  }

  return false;
}

std::unique_ptr<MyArchOperand> MyArchAsmParser::defaultMaskRegOp() const {
  return MyArchOperand::createReg(MyArch::NoRegister, llvm::SMLoc(),
                                 llvm::SMLoc(), isRV64());
}

bool MyArchAsmParser::validateInstruction(MCInst &Inst,
                                         OperandVector &Operands) {
  const MCInstrDesc &MCID = MII.get(Inst.getOpcode());
  unsigned Constraints =
      (MCID.TSFlags & MyArchII::ConstraintMask) >> MyArchII::ConstraintShift;
  if (Constraints == MyArchII::NoConstraint)
    return false;

  unsigned DestReg = Inst.getOperand(0).getReg();
  // Operands[1] will be the first operand, DestReg.
  SMLoc Loc = Operands[1]->getStartLoc();
  if (Constraints & MyArchII::VS2Constraint) {
    unsigned CheckReg = Inst.getOperand(1).getReg();
    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the source vector register group.");
  }
  if ((Constraints & MyArchII::VS1Constraint) && (Inst.getOperand(2).isReg())) {
    unsigned CheckReg = Inst.getOperand(2).getReg();
    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the source vector register group.");
  }
  if ((Constraints & MyArchII::VMConstraint) && (DestReg == MyArch::V0)) {
    // vadc, vsbc are special cases. These instructions have no mask register.
    // The destination register could not be V0.
    unsigned Opcode = Inst.getOpcode();
    if (Opcode == MyArch::VADC_VVM || Opcode == MyArch::VADC_VXM ||
        Opcode == MyArch::VADC_VIM || Opcode == MyArch::VSBC_VVM ||
        Opcode == MyArch::VSBC_VXM || Opcode == MyArch::VFMERGE_VFM ||
        Opcode == MyArch::VMERGE_VIM || Opcode == MyArch::VMERGE_VVM ||
        Opcode == MyArch::VMERGE_VXM)
      return Error(Loc, "The destination vector register group cannot be V0.");

    // Regardless masked or unmasked version, the number of operands is the
    // same. For example, "viota.m v0, v2" is "viota.m v0, v2, NoRegister"
    // actually. We need to check the last operand to ensure whether it is
    // masked or not.
    unsigned CheckReg = Inst.getOperand(Inst.getNumOperands() - 1).getReg();
    assert((CheckReg == MyArch::V0 || CheckReg == MyArch::NoRegister) &&
           "Unexpected register for mask operand");

    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the mask register.");
  }
  return false;
}

bool MyArchAsmParser::processInstruction(MCInst &Inst, SMLoc IDLoc,
                                        OperandVector &Operands,
                                        MCStreamer &Out) {
  Inst.setLoc(IDLoc);

  switch (Inst.getOpcode()) {
  default:
    break;
  case MyArch::PseudoLI: {
    MCRegister Reg = Inst.getOperand(0).getReg();
    const MCOperand &Op1 = Inst.getOperand(1);
    if (Op1.isExpr()) {
      // We must have li reg, %lo(sym) or li reg, %pcrel_lo(sym) or similar.
      // Just convert to an addi. This allows compatibility with gas.
      emitToStreamer(Out, MCInstBuilder(MyArch::ADDI)
                              .addReg(Reg)
                              .addReg(MyArch::X0)
                              .addExpr(Op1.getExpr()));
      return false;
    }
    int64_t Imm = Inst.getOperand(1).getImm();
    // On RV32 the immediate here can either be a signed or an unsigned
    // 32-bit number. Sign extension has to be performed to ensure that Imm
    // represents the expected signed 64-bit number.
    if (!isRV64())
      Imm = SignExtend64<32>(Imm);
    emitLoadImm(Reg, Imm, Out);
    return false;
  }
  case MyArch::PseudoLLA:
    emitLoadLocalAddress(Inst, IDLoc, Out);
    return false;
  case MyArch::PseudoLA:
    emitLoadAddress(Inst, IDLoc, Out);
    return false;
  case MyArch::PseudoLA_TLS_IE:
    emitLoadTLSIEAddress(Inst, IDLoc, Out);
    return false;
  case MyArch::PseudoLA_TLS_GD:
    emitLoadTLSGDAddress(Inst, IDLoc, Out);
    return false;
  case MyArch::PseudoLB:
    emitLoadStoreSymbol(Inst, MyArch::LB, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLBU:
    emitLoadStoreSymbol(Inst, MyArch::LBU, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLH:
    emitLoadStoreSymbol(Inst, MyArch::LH, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLHU:
    emitLoadStoreSymbol(Inst, MyArch::LHU, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLW:
    emitLoadStoreSymbol(Inst, MyArch::LW, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLWU:
    emitLoadStoreSymbol(Inst, MyArch::LWU, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoLD:
    emitLoadStoreSymbol(Inst, MyArch::LD, IDLoc, Out, /*HasTmpReg=*/false);
    return false;
  case MyArch::PseudoFLH:
    emitLoadStoreSymbol(Inst, MyArch::FLH, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoFLW:
    emitLoadStoreSymbol(Inst, MyArch::FLW, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoFLD:
    emitLoadStoreSymbol(Inst, MyArch::FLD, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoSB:
    emitLoadStoreSymbol(Inst, MyArch::SB, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoSH:
    emitLoadStoreSymbol(Inst, MyArch::SH, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoSW:
    emitLoadStoreSymbol(Inst, MyArch::SW, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoSD:
    emitLoadStoreSymbol(Inst, MyArch::SD, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoFSH:
    emitLoadStoreSymbol(Inst, MyArch::FSH, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoFSW:
    emitLoadStoreSymbol(Inst, MyArch::FSW, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoFSD:
    emitLoadStoreSymbol(Inst, MyArch::FSD, IDLoc, Out, /*HasTmpReg=*/true);
    return false;
  case MyArch::PseudoAddTPRel:
    if (checkPseudoAddTPRel(Inst, Operands))
      return true;
    break;
  case MyArch::PseudoSEXT_B:
    emitPseudoExtend(Inst, /*SignExtend=*/true, /*Width=*/8, IDLoc, Out);
    return false;
  case MyArch::PseudoSEXT_H:
    emitPseudoExtend(Inst, /*SignExtend=*/true, /*Width=*/16, IDLoc, Out);
    return false;
  case MyArch::PseudoZEXT_H:
    emitPseudoExtend(Inst, /*SignExtend=*/false, /*Width=*/16, IDLoc, Out);
    return false;
  case MyArch::PseudoZEXT_W:
    emitPseudoExtend(Inst, /*SignExtend=*/false, /*Width=*/32, IDLoc, Out);
    return false;
  case MyArch::PseudoVMSGEU_VX:
  case MyArch::PseudoVMSGEU_VX_M:
  case MyArch::PseudoVMSGEU_VX_M_T:
    emitVMSGE(Inst, MyArch::VMSLTU_VX, IDLoc, Out);
    return false;
  case MyArch::PseudoVMSGE_VX:
  case MyArch::PseudoVMSGE_VX_M:
  case MyArch::PseudoVMSGE_VX_M_T:
    emitVMSGE(Inst, MyArch::VMSLT_VX, IDLoc, Out);
    return false;
  case MyArch::PseudoVMSGE_VI:
  case MyArch::PseudoVMSLT_VI: {
    // These instructions are signed and so is immediate so we can subtract one
    // and change the opcode.
    int64_t Imm = Inst.getOperand(2).getImm();
    unsigned Opc = Inst.getOpcode() == MyArch::PseudoVMSGE_VI ? MyArch::VMSGT_VI
                                                             : MyArch::VMSLE_VI;
    emitToStreamer(Out, MCInstBuilder(Opc)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addImm(Imm - 1)
                            .addOperand(Inst.getOperand(3)));
    return false;
  }
  case MyArch::PseudoVMSGEU_VI:
  case MyArch::PseudoVMSLTU_VI: {
    int64_t Imm = Inst.getOperand(2).getImm();
    // Unsigned comparisons are tricky because the immediate is signed. If the
    // immediate is 0 we can't just subtract one. vmsltu.vi v0, v1, 0 is always
    // false, but vmsle.vi v0, v1, -1 is always true. Instead we use
    // vmsne v0, v1, v1 which is always false.
    if (Imm == 0) {
      unsigned Opc = Inst.getOpcode() == MyArch::PseudoVMSGEU_VI
                         ? MyArch::VMSEQ_VV
                         : MyArch::VMSNE_VV;
      emitToStreamer(Out, MCInstBuilder(Opc)
                              .addOperand(Inst.getOperand(0))
                              .addOperand(Inst.getOperand(1))
                              .addOperand(Inst.getOperand(1))
                              .addOperand(Inst.getOperand(3)));
    } else {
      // Other immediate values can subtract one like signed.
      unsigned Opc = Inst.getOpcode() == MyArch::PseudoVMSGEU_VI
                         ? MyArch::VMSGTU_VI
                         : MyArch::VMSLEU_VI;
      emitToStreamer(Out, MCInstBuilder(Opc)
                              .addOperand(Inst.getOperand(0))
                              .addOperand(Inst.getOperand(1))
                              .addImm(Imm - 1)
                              .addOperand(Inst.getOperand(3)));
    }

    return false;
  }
  }

  emitToStreamer(Out, Inst);
  return false;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMyArchAsmParser() {
  //RegisterMCAsmParser<MyArchAsmParser> X(getTheMyArch32Target());
  RegisterMCAsmParser<MyArchAsmParser> X(getTheMyArchTarget());
 
 // RegisterMCAsmParser<MyArchAsmParser> Y(getTheMyArch64Target());
}
