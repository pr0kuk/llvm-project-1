#ifndef LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchMCASMINFO_H
#define LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class myarchMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit myarchMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_myarch_MCTARGETDESC_myarchMCASMINFO_H
