add_llvm_component_library(LLVMMyArchDesc
  MyArchAsmBackend.cpp
  MyArchBaseInfo.cpp
  MyArchELFObjectWriter.cpp
  MyArchInstPrinter.cpp
  MyArchMCAsmInfo.cpp
  MyArchMCCodeEmitter.cpp
  MyArchMCExpr.cpp
  MyArchMCTargetDesc.cpp
  MyArchMatInt.cpp
  MyArchTargetStreamer.cpp
  MyArchELFStreamer.cpp

  LINK_COMPONENTS
  MC
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
)
