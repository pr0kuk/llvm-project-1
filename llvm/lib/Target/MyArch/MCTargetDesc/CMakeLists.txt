add_llvm_component_library(LLVMMyArchDesc
  MyArchInstPrinter.cpp
  MyArchMCAsmInfo.cpp
  MyArchMCCodeEmitter.cpp
  MyArchMCTargetDesc.cpp
  MyArchAsmBackend.cpp
  MyArchMCExpr.cpp
  MyArchABIInfo.cpp
  MyArchELFObjectWriter.cpp
  MyArchTargetStreamer.cpp
  LINK_COMPONENTS
  MC
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
  )
