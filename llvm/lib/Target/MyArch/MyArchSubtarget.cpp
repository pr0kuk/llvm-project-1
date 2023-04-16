//===-- MyArchSubtarget.cpp - MyArch Subtarget Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the MyArch specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "MyArchSubtarget.h"

#if CH >= CH3_1
#include "MyArchMachineFunction.h"
#include "MyArch.h"
#include "MyArchRegisterInfo.h"

#include "MyArchTargetMachine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "MyArch-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MyArchGenSubtargetInfo.inc"

#if CH >= CH4_1 //1
static cl::opt<bool> EnableOverflowOpt
                ("MyArch-enable-overflow", cl::Hidden, cl::init(false),
                 cl::desc("Use trigger overflow instructions add and sub \
                 instead of non-overflow instructions addu and subu"));
#endif

#if CH >= CH6_1 //1
static cl::opt<bool> UseSmallSectionOpt
                ("MyArch-use-small-section", cl::Hidden, cl::init(false),
                 cl::desc("Use small section. Only work when -relocation-model="
                 "static. pic always not use small section."));

static cl::opt<bool> ReserveGPOpt
                ("MyArch-reserve-gp", cl::Hidden, cl::init(false),
                 cl::desc("Never allocate $gp to variable"));

static cl::opt<bool> NoCploadOpt
                ("MyArch-no-cpload", cl::Hidden, cl::init(false),
                 cl::desc("No issue .cpload"));

bool MyArchReserveGP;
bool MyArchNoCpload;
#endif

extern bool FixGlobalBaseReg;

void MyArchSubtarget::anchor() { }

//@1 {
MyArchSubtarget::MyArchSubtarget(const Triple &TT, StringRef CPU,
                             StringRef FS, bool little, 
                             const MyArchTargetMachine &_TM) :
//@1 }
  // MyArchGenSubtargetInfo will display features by llc -march=MyArch -mcpu=help
  MyArchGenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS),
  IsLittle(little), TM(_TM), TargetTriple(TT), TSInfo(),
      InstrInfo(
          MyArchInstrInfo::create(initializeSubtargetDependencies(CPU, FS, TM))),
      FrameLowering(MyArchFrameLowering::create(*this)),
      TLInfo(MyArchTargetLowering::create(TM, *this)) {

#if CH >= CH4_1 //2
  EnableOverflow = EnableOverflowOpt;
#endif
#if CH >= CH6_1 //2
  // Set UseSmallSection.
  UseSmallSection = UseSmallSectionOpt;
  MyArchReserveGP = ReserveGPOpt;
  MyArchNoCpload = NoCploadOpt;
#ifdef ENABLE_GPRESTORE
  if (!TM.isPositionIndependent() && !UseSmallSection && !MyArchReserveGP)
    FixGlobalBaseReg = false;
  else
#endif
    FixGlobalBaseReg = true;
#endif //#if CH >= CH6_1
}

bool MyArchSubtarget::isPositionIndependent() const {
  return TM.isPositionIndependent();
}

MyArchSubtarget &
MyArchSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                               const TargetMachine &TM) {
  if (TargetTriple.getArch() == Triple::MyArch || TargetTriple.getArch() == Triple::MyArchel) {
    if (CPU.empty() || CPU == "generic") {
      CPU = "MyArch32II";
    }
    else if (CPU == "help") {
      CPU = "";
      return *this;
    }
    else if (CPU != "MyArch32I" && CPU != "MyArch32II") {
      CPU = "MyArch32II";
    }
  }
  else {
    errs() << "!!!Error, TargetTriple.getArch() = " << TargetTriple.getArch()
           <<  "CPU = " << CPU << "\n";
    exit(0);
  }
  
  if (CPU == "MyArch32I")
    MyArchArchVersion = MyArch32I;
  else if (CPU == "MyArch32II")
    MyArchArchVersion = MyArch32II;

  if (isMyArch32I()) {
    HasCmp = true;
    HasSlt = false;
  }
  else if (isMyArch32II()) {
    HasCmp = false;
    HasSlt = true;
  }
  else {
    errs() << "-mcpu must be empty(default:MyArch32II), MyArch32I or MyArch32II" << "\n";
  }

  // Parse features string.
  ParseSubtargetFeatures(CPU, /*TuneCPU*/ CPU, FS);
  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPU);

  return *this;
}

bool MyArchSubtarget::abiUsesSoftFloat() const {
//  return TM->Options.UseSoftFloat;
  return true;
}

const MyArchABIInfo &MyArchSubtarget::getABI() const { return TM.getABI(); }

#endif // #if CH >= CH3_1