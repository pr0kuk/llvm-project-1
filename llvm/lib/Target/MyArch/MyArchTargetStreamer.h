#ifndef LLVM_LIB_TARGET_MyArch_MyArchTARGETSTREAMER_H
#define LLVM_LIB_TARGET_MyArch_MyArchTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class MyArchTargetStreamer : public MCTargetStreamer {
public:
  MyArchTargetStreamer(MCStreamer &S);
  ~MyArchTargetStreamer() override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_MyArch_MyArchTARGETSTREAMER_H