//===-- MyArchInstrInfo.cpp - MyArch Instruction Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the MyArch implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "MyArchInstrInfo.h"
#include "MCTargetDesc/MyArchMatInt.h"
#include "MyArch.h"
#include "MyArchSubtarget.h"
#include "MyArchTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GEN_CHECK_COMPRESS_INSTR
#include "MyArchGenCompressInstEmitter.inc"

#define GET_INSTRINFO_CTOR_DTOR
#include "MyArchGenInstrInfo.inc"

MyArchInstrInfo::MyArchInstrInfo(MyArchSubtarget &STI)
    : MyArchGenInstrInfo(MyArch::ADJCALLSTACKDOWN, MyArch::ADJCALLSTACKUP),
      STI(STI) {}

unsigned MyArchInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                             int &FrameIndex) const {
  switch (MI.getOpcode()) {
  default:
    return 0;
  case MyArch::LB:
  case MyArch::LBU:
  case MyArch::LH:
  case MyArch::LHU:
  case MyArch::FLH:
  case MyArch::LW:
  case MyArch::FLW:
  case MyArch::LWU:
  case MyArch::LD:
  case MyArch::FLD:
    break;
  }

  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }

  return 0;
}

unsigned MyArchInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                            int &FrameIndex) const {
  switch (MI.getOpcode()) {
  default:
    return 0;
  case MyArch::SB:
  case MyArch::SH:
  case MyArch::SW:
  case MyArch::FSH:
  case MyArch::FSW:
  case MyArch::SD:
  case MyArch::FSD:
    break;
  }

  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }

  return 0;
}

void MyArchInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 const DebugLoc &DL, MCRegister DstReg,
                                 MCRegister SrcReg, bool KillSrc) const {
  if (MyArch::GPRRegClass.contains(DstReg, SrcReg)) {
    BuildMI(MBB, MBBI, DL, get(MyArch::ADDI), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addImm(0);
    return;
  }

  // FPR->FPR copies and VR->VR copies.
  unsigned Opc;
  bool IsScalableVector = false;
  if (MyArch::FPR16RegClass.contains(DstReg, SrcReg))
    Opc = MyArch::FSGNJ_H;
  else if (MyArch::FPR32RegClass.contains(DstReg, SrcReg))
    Opc = MyArch::FSGNJ_S;
  else if (MyArch::FPR64RegClass.contains(DstReg, SrcReg))
    Opc = MyArch::FSGNJ_D;
  else if (MyArch::VRRegClass.contains(DstReg, SrcReg)) {
    Opc = MyArch::PseudoVMV1R_V;
    IsScalableVector = true;
  } else if (MyArch::VRM2RegClass.contains(DstReg, SrcReg)) {
    Opc = MyArch::PseudoVMV2R_V;
    IsScalableVector = true;
  } else if (MyArch::VRM4RegClass.contains(DstReg, SrcReg)) {
    Opc = MyArch::PseudoVMV4R_V;
    IsScalableVector = true;
  } else if (MyArch::VRM8RegClass.contains(DstReg, SrcReg)) {
    Opc = MyArch::PseudoVMV8R_V;
    IsScalableVector = true;
  } else
    llvm_unreachable("Impossible reg-to-reg copy");

  if (IsScalableVector)
    BuildMI(MBB, MBBI, DL, get(Opc), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  else
    BuildMI(MBB, MBBI, DL, get(Opc), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addReg(SrcReg, getKillRegState(KillSrc));
}

void MyArchInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator I,
                                         Register SrcReg, bool IsKill, int FI,
                                         const TargetRegisterClass *RC,
                                         const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOStore,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  unsigned Opcode;
  if (MyArch::GPRRegClass.hasSubClassEq(RC))
    Opcode = TRI->getRegSizeInBits(MyArch::GPRRegClass) == 32 ?
             MyArch::SW : MyArch::SD;
  else if (MyArch::FPR16RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FSH;
  else if (MyArch::FPR32RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FSW;
  else if (MyArch::FPR64RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FSD;
  else
    llvm_unreachable("Can't store this register to stack slot");

  BuildMI(MBB, I, DL, get(Opcode))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void MyArchInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator I,
                                          Register DstReg, int FI,
                                          const TargetRegisterClass *RC,
                                          const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  unsigned Opcode;
  if (MyArch::GPRRegClass.hasSubClassEq(RC))
    Opcode = TRI->getRegSizeInBits(MyArch::GPRRegClass) == 32 ?
             MyArch::LW : MyArch::LD;
  else if (MyArch::FPR16RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FLH;
  else if (MyArch::FPR32RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FLW;
  else if (MyArch::FPR64RegClass.hasSubClassEq(RC))
    Opcode = MyArch::FLD;
  else
    llvm_unreachable("Can't load this register from stack slot");

  BuildMI(MBB, I, DL, get(Opcode), DstReg)
    .addFrameIndex(FI)
    .addImm(0)
    .addMemOperand(MMO);
}

void MyArchInstrInfo::movImm(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            const DebugLoc &DL, Register DstReg, uint64_t Val,
                            MachineInstr::MIFlag Flag) const {
  MachineFunction *MF = MBB.getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  bool IsRV64 = MF->getSubtarget<MyArchSubtarget>().is64Bit();
  Register SrcReg = MyArch::X0;
  Register Result = MRI.createVirtualRegister(&MyArch::GPRRegClass);
  unsigned Num = 0;

  if (!IsRV64 && !isInt<32>(Val))
    report_fatal_error("Should only materialize 32-bit constants for RV32");

  MyArchMatInt::InstSeq Seq;
  MyArchMatInt::generateInstSeq(Val, IsRV64, Seq);
  assert(Seq.size() > 0);

  for (MyArchMatInt::Inst &Inst : Seq) {
    // Write the final result to DstReg if it's the last instruction in the Seq.
    // Otherwise, write the result to the temp register.
    if (++Num == Seq.size())
      Result = DstReg;

    if (Inst.Opc == MyArch::LUI) {
      BuildMI(MBB, MBBI, DL, get(MyArch::LUI), Result)
          .addImm(Inst.Imm)
          .setMIFlag(Flag);
    } else {
      BuildMI(MBB, MBBI, DL, get(Inst.Opc), Result)
          .addReg(SrcReg, RegState::Kill)
          .addImm(Inst.Imm)
          .setMIFlag(Flag);
    }
    // Only the first instruction has X0 as its source.
    SrcReg = Result;
  }
}

// The contents of values added to Cond are not examined outside of
// MyArchInstrInfo, giving us flexibility in what to push to it. For MyArch, we
// push BranchOpcode, Reg1, Reg2.
static void parseCondBranch(MachineInstr &LastInst, MachineBasicBlock *&Target,
                            SmallVectorImpl<MachineOperand> &Cond) {
  // Block ends with fall-through condbranch.
  assert(LastInst.getDesc().isConditionalBranch() &&
         "Unknown conditional branch");
  Target = LastInst.getOperand(2).getMBB();
  Cond.push_back(MachineOperand::CreateImm(LastInst.getOpcode()));
  Cond.push_back(LastInst.getOperand(0));
  Cond.push_back(LastInst.getOperand(1));
}

static unsigned getOppositeBranchOpcode(int Opc) {
  switch (Opc) {
  default:
    llvm_unreachable("Unrecognized conditional branch");
  case MyArch::BEQ:
    return MyArch::BNE;
  case MyArch::BNE:
    return MyArch::BEQ;
  case MyArch::BLT:
    return MyArch::BGE;
  case MyArch::BGE:
    return MyArch::BLT;
  case MyArch::BLTU:
    return MyArch::BGEU;
  case MyArch::BGEU:
    return MyArch::BLTU;
  }
}

bool MyArchInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const {
  TBB = FBB = nullptr;
  Cond.clear();

  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end() || !isUnpredicatedTerminator(*I))
    return false;

  // Count the number of terminators and find the first unconditional or
  // indirect branch.
  MachineBasicBlock::iterator FirstUncondOrIndirectBr = MBB.end();
  int NumTerminators = 0;
  for (auto J = I.getReverse(); J != MBB.rend() && isUnpredicatedTerminator(*J);
       J++) {
    NumTerminators++;
    if (J->getDesc().isUnconditionalBranch() ||
        J->getDesc().isIndirectBranch()) {
      FirstUncondOrIndirectBr = J.getReverse();
    }
  }

  // If AllowModify is true, we can erase any terminators after
  // FirstUncondOrIndirectBR.
  if (AllowModify && FirstUncondOrIndirectBr != MBB.end()) {
    while (std::next(FirstUncondOrIndirectBr) != MBB.end()) {
      std::next(FirstUncondOrIndirectBr)->eraseFromParent();
      NumTerminators--;
    }
    I = FirstUncondOrIndirectBr;
  }

  // We can't handle blocks that end in an indirect branch.
  if (I->getDesc().isIndirectBranch())
    return true;

  // We can't handle blocks with more than 2 terminators.
  if (NumTerminators > 2)
    return true;

  // Handle a single unconditional branch.
  if (NumTerminators == 1 && I->getDesc().isUnconditionalBranch()) {
    TBB = getBranchDestBlock(*I);
    return false;
  }

  // Handle a single conditional branch.
  if (NumTerminators == 1 && I->getDesc().isConditionalBranch()) {
    parseCondBranch(*I, TBB, Cond);
    return false;
  }

  // Handle a conditional branch followed by an unconditional branch.
  if (NumTerminators == 2 && std::prev(I)->getDesc().isConditionalBranch() &&
      I->getDesc().isUnconditionalBranch()) {
    parseCondBranch(*std::prev(I), TBB, Cond);
    FBB = getBranchDestBlock(*I);
    return false;
  }

  // Otherwise, we can't handle this.
  return true;
}

unsigned MyArchInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                      int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return 0;

  if (!I->getDesc().isUnconditionalBranch() &&
      !I->getDesc().isConditionalBranch())
    return 0;

  // Remove the branch.
  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();

  I = MBB.end();

  if (I == MBB.begin())
    return 1;
  --I;
  if (!I->getDesc().isConditionalBranch())
    return 1;

  // Remove the branch.
  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();
  return 2;
}

// Inserts a branch into the end of the specific MachineBasicBlock, returning
// the number of instructions inserted.
unsigned MyArchInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  if (BytesAdded)
    *BytesAdded = 0;

  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 3 || Cond.size() == 0) &&
         "MyArch branch conditions have two components!");

  // Unconditional branch.
  if (Cond.empty()) {
    MachineInstr &MI = *BuildMI(&MBB, DL, get(MyArch::PseudoBR)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    return 1;
  }

  // Either a one or two-way conditional branch.
  unsigned Opc = Cond[0].getImm();
  MachineInstr &CondMI =
      *BuildMI(&MBB, DL, get(Opc)).add(Cond[1]).add(Cond[2]).addMBB(TBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(CondMI);

  // One-way conditional branch.
  if (!FBB)
    return 1;

  // Two-way conditional branch.
  MachineInstr &MI = *BuildMI(&MBB, DL, get(MyArch::PseudoBR)).addMBB(FBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(MI);
  return 2;
}

unsigned MyArchInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                              MachineBasicBlock &DestBB,
                                              const DebugLoc &DL,
                                              int64_t BrOffset,
                                              RegScavenger *RS) const {
  assert(RS && "RegScavenger required for long branching");
  assert(MBB.empty() &&
         "new block should be inserted for expanding unconditional branch");
  assert(MBB.pred_size() == 1);

  MachineFunction *MF = MBB.getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();

  if (!isInt<32>(BrOffset))
    report_fatal_error(
        "Branch offsets outside of the signed 32-bit range not supported");

  // FIXME: A virtual register must be used initially, as the register
  // scavenger won't work with empty blocks (SIInstrInfo::insertIndirectBranch
  // uses the same workaround).
  Register ScratchReg = MRI.createVirtualRegister(&MyArch::GPRRegClass);
  auto II = MBB.end();

  MachineInstr &MI = *BuildMI(MBB, II, DL, get(MyArch::PseudoJump))
                          .addReg(ScratchReg, RegState::Define | RegState::Dead)
                          .addMBB(&DestBB, MyArchII::MO_CALL);

  RS->enterBasicBlockEnd(MBB);
  unsigned Scav = RS->scavengeRegisterBackwards(MyArch::GPRRegClass,
                                                MI.getIterator(), false, 0);
  MRI.replaceRegWith(ScratchReg, Scav);
  MRI.clearVirtRegs();
  RS->setRegUsed(Scav);
  return 8;
}

bool MyArchInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert((Cond.size() == 3) && "Invalid branch condition!");
  Cond[0].setImm(getOppositeBranchOpcode(Cond[0].getImm()));
  return false;
}

MachineBasicBlock *
MyArchInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  assert(MI.getDesc().isBranch() && "Unexpected opcode!");
  // The branch target is always the last operand.
  int NumOp = MI.getNumExplicitOperands();
  return MI.getOperand(NumOp - 1).getMBB();
}

bool MyArchInstrInfo::isBranchOffsetInRange(unsigned BranchOp,
                                           int64_t BrOffset) const {
  unsigned XLen = STI.getXLen();
  // Ideally we could determine the supported branch offset from the
  // MyArchII::FormMask, but this can't be used for Pseudo instructions like
  // PseudoBR.
  switch (BranchOp) {
  default:
    llvm_unreachable("Unexpected opcode!");
  case MyArch::BEQ:
  case MyArch::BNE:
  case MyArch::BLT:
  case MyArch::BGE:
  case MyArch::BLTU:
  case MyArch::BGEU:
    return isIntN(13, BrOffset);
  case MyArch::JAL:
  case MyArch::PseudoBR:
    return isIntN(21, BrOffset);
  case MyArch::PseudoJump:
    return isIntN(32, SignExtend64(BrOffset + 0x800, XLen));
  }
}

unsigned MyArchInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  unsigned Opcode = MI.getOpcode();

  switch (Opcode) {
  default: {
    if (MI.getParent() && MI.getParent()->getParent()) {
      const auto MF = MI.getMF();
      const auto &TM = static_cast<const MyArchTargetMachine &>(MF->getTarget());
      const MCRegisterInfo &MRI = *TM.getMCRegisterInfo();
      const MCSubtargetInfo &STI = *TM.getMCSubtargetInfo();
      const MyArchSubtarget &ST = MF->getSubtarget<MyArchSubtarget>();
      if (isCompressibleInst(MI, &ST, MRI, STI))
        return 2;
    }
    return get(Opcode).getSize();
  }
  case TargetOpcode::EH_LABEL:
  case TargetOpcode::IMPLICIT_DEF:
  case TargetOpcode::KILL:
  case TargetOpcode::DBG_VALUE:
    return 0;
  // These values are determined based on MyArchExpandAtomicPseudoInsts,
  // MyArchExpandPseudoInsts and MyArchMCCodeEmitter, depending on where the
  // pseudos are expanded.
  case MyArch::PseudoCALLReg:
  case MyArch::PseudoCALL:
  case MyArch::PseudoJump:
  case MyArch::PseudoTAIL:
  case MyArch::PseudoLLA:
  case MyArch::PseudoLA:
  case MyArch::PseudoLA_TLS_IE:
  case MyArch::PseudoLA_TLS_GD:
    return 8;
  case MyArch::PseudoAtomicLoadNand32:
  case MyArch::PseudoAtomicLoadNand64:
    return 20;
  case MyArch::PseudoMaskedAtomicSwap32:
  case MyArch::PseudoMaskedAtomicLoadAdd32:
  case MyArch::PseudoMaskedAtomicLoadSub32:
    return 28;
  case MyArch::PseudoMaskedAtomicLoadNand32:
    return 32;
  case MyArch::PseudoMaskedAtomicLoadMax32:
  case MyArch::PseudoMaskedAtomicLoadMin32:
    return 44;
  case MyArch::PseudoMaskedAtomicLoadUMax32:
  case MyArch::PseudoMaskedAtomicLoadUMin32:
    return 36;
  case MyArch::PseudoCmpXchg32:
  case MyArch::PseudoCmpXchg64:
    return 16;
  case MyArch::PseudoMaskedCmpXchg32:
    return 32;
  case TargetOpcode::INLINEASM:
  case TargetOpcode::INLINEASM_BR: {
    const MachineFunction &MF = *MI.getParent()->getParent();
    const auto &TM = static_cast<const MyArchTargetMachine &>(MF.getTarget());
    return getInlineAsmLength(MI.getOperand(0).getSymbolName(),
                              *TM.getMCAsmInfo());
  }
  }
}

bool MyArchInstrInfo::isAsCheapAsAMove(const MachineInstr &MI) const {
  const unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
  default:
    break;
  case MyArch::FSGNJ_D:
  case MyArch::FSGNJ_S:
    // The canonical floating-point move is fsgnj rd, rs, rs.
    return MI.getOperand(1).isReg() && MI.getOperand(2).isReg() &&
           MI.getOperand(1).getReg() == MI.getOperand(2).getReg();
  case MyArch::ADDI:
  case MyArch::ORI:
  case MyArch::XORI:
    return (MI.getOperand(1).isReg() &&
            MI.getOperand(1).getReg() == MyArch::X0) ||
           (MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0);
  }
  return MI.isAsCheapAsAMove();
}

Optional<DestSourcePair>
MyArchInstrInfo::isCopyInstrImpl(const MachineInstr &MI) const {
  if (MI.isMoveReg())
    return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
  switch (MI.getOpcode()) {
  default:
    break;
  case MyArch::ADDI:
    // Operand 1 can be a frameindex but callers expect registers
    if (MI.getOperand(1).isReg() && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0)
      return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
    break;
  case MyArch::FSGNJ_D:
  case MyArch::FSGNJ_S:
    // The canonical floating-point move is fsgnj rd, rs, rs.
    if (MI.getOperand(1).isReg() && MI.getOperand(2).isReg() &&
        MI.getOperand(1).getReg() == MI.getOperand(2).getReg())
      return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
    break;
  }
  return None;
}

bool MyArchInstrInfo::verifyInstruction(const MachineInstr &MI,
                                       StringRef &ErrInfo) const {
  const MCInstrInfo *MCII = STI.getInstrInfo();
  MCInstrDesc const &Desc = MCII->get(MI.getOpcode());

  for (auto &OI : enumerate(Desc.operands())) {
    unsigned OpType = OI.value().OperandType;
    if (OpType >= MyArchOp::OPERAND_FIRST_MyArch_IMM &&
        OpType <= MyArchOp::OPERAND_LAST_MyArch_IMM) {
      const MachineOperand &MO = MI.getOperand(OI.index());
      if (MO.isImm()) {
        int64_t Imm = MO.getImm();
        bool Ok;
        switch (OpType) {
        default:
          llvm_unreachable("Unexpected operand type");
        case MyArchOp::OPERAND_UIMM4:
          Ok = isUInt<4>(Imm);
          break;
        case MyArchOp::OPERAND_UIMM5:
          Ok = isUInt<5>(Imm);
          break;
        case MyArchOp::OPERAND_UIMM12:
          Ok = isUInt<12>(Imm);
          break;
        case MyArchOp::OPERAND_SIMM12:
          Ok = isInt<12>(Imm);
          break;
        case MyArchOp::OPERAND_UIMM20:
          Ok = isUInt<20>(Imm);
          break;
        case MyArchOp::OPERAND_UIMMLOG2XLEN:
          if (STI.getTargetTriple().isArch64Bit())
            Ok = isUInt<6>(Imm);
          else
            Ok = isUInt<5>(Imm);
          break;
        }
        if (!Ok) {
          ErrInfo = "Invalid immediate";
          return false;
        }
      }
    }
  }

  return true;
}

// Return true if get the base operand, byte offset of an instruction and the
// memory width. Width is the size of memory that is being loaded/stored.
bool MyArchInstrInfo::getMemOperandWithOffsetWidth(
    const MachineInstr &LdSt, const MachineOperand *&BaseReg, int64_t &Offset,
    unsigned &Width, const TargetRegisterInfo *TRI) const {
  if (!LdSt.mayLoadOrStore())
    return false;

  // Here we assume the standard RISC-V ISA, which uses a base+offset
  // addressing mode. You'll need to relax these conditions to support custom
  // load/stores instructions.
  if (LdSt.getNumExplicitOperands() != 3)
    return false;
  if (!LdSt.getOperand(1).isReg() || !LdSt.getOperand(2).isImm())
    return false;

  if (!LdSt.hasOneMemOperand())
    return false;

  Width = (*LdSt.memoperands_begin())->getSize();
  BaseReg = &LdSt.getOperand(1);
  Offset = LdSt.getOperand(2).getImm();
  return true;
}

bool MyArchInstrInfo::areMemAccessesTriviallyDisjoint(
    const MachineInstr &MIa, const MachineInstr &MIb) const {
  assert(MIa.mayLoadOrStore() && "MIa must be a load or store.");
  assert(MIb.mayLoadOrStore() && "MIb must be a load or store.");

  if (MIa.hasUnmodeledSideEffects() || MIb.hasUnmodeledSideEffects() ||
      MIa.hasOrderedMemoryRef() || MIb.hasOrderedMemoryRef())
    return false;

  // Retrieve the base register, offset from the base register and width. Width
  // is the size of memory that is being loaded/stored (e.g. 1, 2, 4).  If
  // base registers are identical, and the offset of a lower memory access +
  // the width doesn't overlap the offset of a higher memory access,
  // then the memory accesses are different.
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  const MachineOperand *BaseOpA = nullptr, *BaseOpB = nullptr;
  int64_t OffsetA = 0, OffsetB = 0;
  unsigned int WidthA = 0, WidthB = 0;
  if (getMemOperandWithOffsetWidth(MIa, BaseOpA, OffsetA, WidthA, TRI) &&
      getMemOperandWithOffsetWidth(MIb, BaseOpB, OffsetB, WidthB, TRI)) {
    if (BaseOpA->isIdenticalTo(*BaseOpB)) {
      int LowOffset = std::min(OffsetA, OffsetB);
      int HighOffset = std::max(OffsetA, OffsetB);
      int LowWidth = (LowOffset == OffsetA) ? WidthA : WidthB;
      if (LowOffset + LowWidth <= HighOffset)
        return true;
    }
  }
  return false;
}

std::pair<unsigned, unsigned>
MyArchInstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  const unsigned Mask = MyArchII::MO_DIRECT_FLAG_MASK;
  return std::make_pair(TF & Mask, TF & ~Mask);
}

ArrayRef<std::pair<unsigned, const char *>>
MyArchInstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
  using namespace MyArchII;
  static const std::pair<unsigned, const char *> TargetFlags[] = {
      {MO_CALL, "myarch-call"},
      {MO_PLT, "myarch-plt"},
      {MO_LO, "myarch-lo"},
      {MO_HI, "myarch-hi"},
      {MO_PCREL_LO, "myarch-pcrel-lo"},
      {MO_PCREL_HI, "myarch-pcrel-hi"},
      {MO_GOT_HI, "myarch-got-hi"},
      {MO_TPREL_LO, "myarch-tprel-lo"},
      {MO_TPREL_HI, "myarch-tprel-hi"},
      {MO_TPREL_ADD, "myarch-tprel-add"},
      {MO_TLS_GOT_HI, "myarch-tls-got-hi"},
      {MO_TLS_GD_HI, "myarch-tls-gd-hi"}};
  return makeArrayRef(TargetFlags);
}
bool MyArchInstrInfo::isFunctionSafeToOutlineFrom(
    MachineFunction &MF, bool OutlineFromLinkOnceODRs) const {
  const Function &F = MF.getFunction();

  // Can F be deduplicated by the linker? If it can, don't outline from it.
  if (!OutlineFromLinkOnceODRs && F.hasLinkOnceODRLinkage())
    return false;

  // Don't outline from functions with section markings; the program could
  // expect that all the code is in the named section.
  if (F.hasSection())
    return false;

  // It's safe to outline from MF.
  return true;
}

bool MyArchInstrInfo::isMBBSafeToOutlineFrom(MachineBasicBlock &MBB,
                                            unsigned &Flags) const {
  // More accurate safety checking is done in getOutliningCandidateInfo.
  return true;
}

// Enum values indicating how an outlined call should be constructed.
enum MachineOutlinerConstructionID {
  MachineOutlinerDefault
};

outliner::OutlinedFunction MyArchInstrInfo::getOutliningCandidateInfo(
    std::vector<outliner::Candidate> &RepeatedSequenceLocs) const {

  // First we need to filter out candidates where the X5 register (IE t0) can't
  // be used to setup the function call.
  auto CannotInsertCall = [](outliner::Candidate &C) {
    const TargetRegisterInfo *TRI = C.getMF()->getSubtarget().getRegisterInfo();

    C.initLRU(*TRI);
    LiveRegUnits LRU = C.LRU;
    return !LRU.available(MyArch::X5);
  };

  llvm::erase_if(RepeatedSequenceLocs, CannotInsertCall);

  // If the sequence doesn't have enough candidates left, then we're done.
  if (RepeatedSequenceLocs.size() < 2)
    return outliner::OutlinedFunction();

  unsigned SequenceSize = 0;

  auto I = RepeatedSequenceLocs[0].front();
  auto E = std::next(RepeatedSequenceLocs[0].back());
  for (; I != E; ++I)
    SequenceSize += getInstSizeInBytes(*I);

  // call t0, function = 8 bytes.
  unsigned CallOverhead = 8;
  for (auto &C : RepeatedSequenceLocs)
    C.setCallInfo(MachineOutlinerDefault, CallOverhead);

  // jr t0 = 4 bytes, 2 bytes if compressed instructions are enabled.
  unsigned FrameOverhead = 4;
  if (RepeatedSequenceLocs[0].getMF()->getSubtarget()
          .getFeatureBits()[MyArch::FeatureStdExtC])
    FrameOverhead = 2;

  return outliner::OutlinedFunction(RepeatedSequenceLocs, SequenceSize,
                                    FrameOverhead, MachineOutlinerDefault);
}

outliner::InstrType
MyArchInstrInfo::getOutliningType(MachineBasicBlock::iterator &MBBI,
                                 unsigned Flags) const {
  MachineInstr &MI = *MBBI;
  MachineBasicBlock *MBB = MI.getParent();
  const TargetRegisterInfo *TRI =
      MBB->getParent()->getSubtarget().getRegisterInfo();

  // Positions generally can't safely be outlined.
  if (MI.isPosition()) {
    // We can manually strip out CFI instructions later.
    if (MI.isCFIInstruction())
      return outliner::InstrType::Invisible;

    return outliner::InstrType::Illegal;
  }

  // Don't trust the user to write safe inline assembly.
  if (MI.isInlineAsm())
    return outliner::InstrType::Illegal;

  // We can't outline branches to other basic blocks.
  if (MI.isTerminator() && !MBB->succ_empty())
    return outliner::InstrType::Illegal;

  // We need support for tail calls to outlined functions before return
  // statements can be allowed.
  if (MI.isReturn())
    return outliner::InstrType::Illegal;

  // Don't allow modifying the X5 register which we use for return addresses for
  // these outlined functions.
  if (MI.modifiesRegister(MyArch::X5, TRI) ||
      MI.getDesc().hasImplicitDefOfPhysReg(MyArch::X5))
    return outliner::InstrType::Illegal;

  // Make sure the operands don't reference something unsafe.
  for (const auto &MO : MI.operands())
    if (MO.isMBB() || MO.isBlockAddress() || MO.isCPI())
      return outliner::InstrType::Illegal;

  // Don't allow instructions which won't be materialized to impact outlining
  // analysis.
  if (MI.isMetaInstruction())
    return outliner::InstrType::Invisible;

  return outliner::InstrType::Legal;
}

void MyArchInstrInfo::buildOutlinedFrame(
    MachineBasicBlock &MBB, MachineFunction &MF,
    const outliner::OutlinedFunction &OF) const {

  // Strip out any CFI instructions
  bool Changed = true;
  while (Changed) {
    Changed = false;
    auto I = MBB.begin();
    auto E = MBB.end();
    for (; I != E; ++I) {
      if (I->isCFIInstruction()) {
        I->removeFromParent();
        Changed = true;
        break;
      }
    }
  }

  MBB.addLiveIn(MyArch::X5);

  // Add in a return instruction to the end of the outlined frame.
  MBB.insert(MBB.end(), BuildMI(MF, DebugLoc(), get(MyArch::JALR))
      .addReg(MyArch::X0, RegState::Define)
      .addReg(MyArch::X5)
      .addImm(0));
}

MachineBasicBlock::iterator MyArchInstrInfo::insertOutlinedCall(
    Module &M, MachineBasicBlock &MBB, MachineBasicBlock::iterator &It,
    MachineFunction &MF, const outliner::Candidate &C) const {

  // Add in a call instruction to the outlined function at the given location.
  It = MBB.insert(It,
                  BuildMI(MF, DebugLoc(), get(MyArch::PseudoCALLReg), MyArch::X5)
                      .addGlobalAddress(M.getNamedValue(MF.getName()), 0,
                                        MyArchII::MO_CALL));
  return It;
}
