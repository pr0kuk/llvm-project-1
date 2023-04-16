//===-- MyArchTargetMachine.h - Define TargetMachine for MyArch -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the MyArch specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MyArch_MyArchTARGETMACHINE_H
#define LLVM_LIB_TARGET_MyArch_MyArchTARGETMACHINE_H

// #include "MyArchConfig.h"
#if CH >= CH3_1

#include "MCTargetDesc/MyArchABIInfo.h"
#include "MyArchSubtarget.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class formatted_raw_ostream;
class MyArchRegisterInfo;

class MyArchTargetMachine : public LLVMTargetMachine {
  bool isLittle;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  // Selected ABI
  MyArchABIInfo ABI;
  MyArchSubtarget DefaultSubtarget;

  mutable StringMap<std::unique_ptr<MyArchSubtarget>> SubtargetMap;
public:
  MyArchTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                    CodeGenOpt::Level OL, bool JIT, bool isLittle);
  ~MyArchTargetMachine() override;

  const MyArchSubtarget *getSubtargetImpl() const {
    return &DefaultSubtarget;
  }

  const MyArchSubtarget *getSubtargetImpl(const Function &F) const override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
  bool isLittleEndian() const { return isLittle; }
  const MyArchABIInfo &getABI() const { return ABI; }
};

/// MyArchebTargetMachine - MyArch32 big endian target machine.
///
class MyArchebTargetMachine : public MyArchTargetMachine {
  virtual void anchor();
public:
  MyArchebTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
};

/// MyArchelTargetMachine - MyArch32 little endian target machine.
///
class MyArchelTargetMachine : public MyArchTargetMachine {
  virtual void anchor();
public:
  MyArchelTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
};
} // End llvm namespace

#endif // #if CH >= CH3_1

#endif
