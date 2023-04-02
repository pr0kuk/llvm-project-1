#ifndef LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCASMINFO_H
#define LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class MyArchMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit MyArchMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_MyArch_MCTARGETDESC_MyArchMCASMINFO_H
