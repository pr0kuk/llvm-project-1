#ifndef LLVM_LIB_TARGET_myarch_myarchTARGETSTREAMER_H
#define LLVM_LIB_TARGET_myarch_myarchTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class myarchTargetStreamer : public MCTargetStreamer {
public:
  myarchTargetStreamer(MCStreamer &S);
  ~myarchTargetStreamer() override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_myarch_myarchTARGETSTREAMER_H