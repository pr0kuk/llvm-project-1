add_llvm_component_group(MyArch)

set(LLVM_TARGET_DEFINITIONS MyArch.td)

# Generate MyArchGenRegisterInfo.inc and MyArchGenInstrInfo.inc which included by
#  your hand code C++ files.
# MyArchGenRegisterInfo.inc came from MyArchRegisterInfo.td, MyArchGenInstrInfo.inc
#  came from MyArchInstrInfo.td.
tablegen(LLVM MyArchGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MyArchGenInstrInfo.inc -gen-instr-info)

# MyArchCommonTableGen must be defined
add_public_tablegen_target(MyArchCommonTableGen)

# MyArchCodeGen should match with LLVMBuild.txt MyArchCodeGen
add_llvm_target(MyArchCodeGen
  MyArchTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  MyArchDesc
  MyArchInfo
  SelectionDAG
  Support
  Target
  GlobalISel

  ADD_TO_COMPONENT
  MyArch
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
