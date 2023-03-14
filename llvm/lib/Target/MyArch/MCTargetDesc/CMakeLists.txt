add_llvm_component_library(LLVMMyArchDesc
  MyArchMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
  )
