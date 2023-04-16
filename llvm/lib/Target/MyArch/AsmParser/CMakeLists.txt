add_llvm_component_library(LLVMMyArchAsmParser
  MyArchAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  MyArchDesc
  MyArchInfo
  Support

  ADD_TO_COMPONENT
  MyArch
  )
