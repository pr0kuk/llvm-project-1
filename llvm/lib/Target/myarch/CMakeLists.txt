add_llvm_component_group(myarch)

set(LLVM_TARGET_DEFINITIONS myarch.td)

# Generate myarchGenRegisterInfo.inc and myarchGenInstrInfo.inc which included by
#  your hand code C++ files.
# myarchGenRegisterInfo.inc came from myarchRegisterInfo.td, myarchGenInstrInfo.inc
#  came from myarchInstrInfo.td.
tablegen(LLVM myarchGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM myarchGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM myarchGenCallingConv.inc -gen-callingconv)
tablegen(LLVM myarchGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM myarchGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM myarchGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM myarchGenDAGISel.inc -gen-dag-isel)

#tablegen(LLVM myarchGenDisassemblerTables.inc -gen-disassembler)

# myarchCommonTableGen must be defined
add_public_tablegen_target(myarchCommonTableGen)

# myarchCodeGen should match with LLVMBuild.txt myarchCodeGen
add_llvm_target(myarchCodeGen
  myarchAsmPrinter.cpp
  myarchFrameLowering.cpp
  myarchInstrInfo.cpp
  myarchISelDagToDag.cpp
  myarchISelLowering.cpp
  myarchMachineFunctionInfo.cpp
  myarchMCInstLower.cpp
  myarchRegisterInfo.cpp
  myarchSubtarget.cpp
  myarchTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  myarchDesc
  myarchInfo
  SelectionDAG
  Support
  Target
  GlobalISel

  ADD_TO_COMPONENT
  myarch
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
