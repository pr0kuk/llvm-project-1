add_llvm_component_group(MyArch)
# set(LLVM_TARGET_DEFINITIONS MyArch.td)
# set(LLVM_TARGET_DEFINITIONS MyArchAsm.td)
# set(LLVM_TARGET_DEFINITIONS MyArchOther.td)

# Generate MyArchGenRegisterInfo.inc and MyArchGenInstrInfo.inc which included by 
#  your hand code C++ files. 
# MyArchGenRegisterInfo.inc came from MyArchRegisterInfo.td, MyArchGenInstrInfo.inc 
#  came from MyArchInstrInfo.td.
tablegen(LLVM MyArchGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MyArchGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MyArchGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM MyArchGenCodeEmitter.inc -gen-emitter)
tablegen(LLVM MyArchGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM MyArchGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM MyArchGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM MyArchGenCallingConv.inc -gen-callingconv)
tablegen(LLVM MyArchGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM MyArchGenMCPseudoLowering.inc -gen-pseudo-lowering)
# tablegen(LLVM MyArchGenAsmMatcher.inc -gen-asm-matcher)


# MyArchCommonTableGen must be defined
add_public_tablegen_target(MyArchCommonTableGen)

# MyArchCodeGen should match with LLVMBuild.txt MyArchCodeGen
add_llvm_target(MyArchCodeGen
  MyArchAnalyzeImmediate.cpp
  MyArchAsmPrinter.cpp
  MyArchMCInstLower.cpp
  #MyArchDelaySlotFiller.cpp
  #MyArchDelUselessJMP.cpp
  #MyArchEmitGPRestore.cpp
  MyArchFrameLowering.cpp
  #MyArchLongBranch.cpp
  MyArchInstrInfo.cpp
  MyArchISelLowering.cpp
  MyArchMachineFunction.cpp
  MyArchMachineFunctionInfo.cpp
  MyArchRegisterInfo.cpp
  #MyArchSEFrameLowering.cpp
  #MyArchSEInstrInfo.cpp
  #MyArchSEISelLowering.cpp
  #MyArchSERegisterInfo.cpp
  MyArchSubtarget.cpp
  # MyArchTargetObjectFile.cpp
  MyArchISelDagToDag.cpp
  #MyArchSEISelDAGToDAG.cpp
  MyArchTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  MyArchDesc
  MyArchInfo
  MyArchAsmParser
  MyArchAsmPrinter
  MyArchDisassembler
  SelectionDAG
  Support
  Target
  GlobalISel

  ADD_TO_COMPONENT
  MyArch
  )

# add_subdirectory(InstPrinter)
#add_subdirectory(Disassembler)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
# add_subdirectory(AsmParser)
