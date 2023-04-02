add_llvm_component_library(LLVMmyarchDesc
  myarchInstPrinter.cpp
  myarchMCAsmInfo.cpp
  myarchMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  myarchInfo
  Support

  ADD_TO_COMPONENT
  myarch
  )
