add_llvm_component_library(LLVMMyArchDisassembler
  MyArchDisassembler.cpp

  LINK_COMPONENTS
  MCDisassembler
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
  )
