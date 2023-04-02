add_llvm_component_library(LLVMMyArchDesc
  MyArchInstPrinter.cpp
  MyArchMCAsmInfo.cpp
  MyArchMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
  )
