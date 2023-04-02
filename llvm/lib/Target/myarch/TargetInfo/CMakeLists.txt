add_llvm_component_library(LLVMmyarchInfo
  myarchTargetInfo.cpp

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  myarch
  )
