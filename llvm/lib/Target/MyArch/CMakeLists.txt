add_llvm_component_group(MyArch)

set(LLVM_TARGET_DEFINITIONS MyArch.td)

tablegen(LLVM MyArchGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM MyArchGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM MyArchGenCompressInstEmitter.inc -gen-compress-inst-emitter)
tablegen(LLVM MyArchGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM MyArchGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM MyArchGenGlobalISel.inc -gen-global-isel)
tablegen(LLVM MyArchGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MyArchGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM MyArchGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM MyArchGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM MyArchGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MyArchGenSearchableTables.inc -gen-searchable-tables)
tablegen(LLVM MyArchGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MyArchCommonTableGen)

add_llvm_target(MyArchCodeGen
  MyArchAsmPrinter.cpp
  MyArchCallLowering.cpp
  MyArchCleanupVSETVLI.cpp
  MyArchExpandAtomicPseudoInsts.cpp
  MyArchExpandPseudoInsts.cpp
  MyArchFrameLowering.cpp
  MyArchInstrInfo.cpp
  MyArchInstructionSelector.cpp
  MyArchISelDAGToDAG.cpp
  MyArchISelLowering.cpp
  MyArchLegalizerInfo.cpp
  MyArchMCInstLower.cpp
  MyArchMergeBaseOffset.cpp
  MyArchRegisterBankInfo.cpp
  MyArchRegisterInfo.cpp
  MyArchSubtarget.cpp
  MyArchTargetMachine.cpp
  MyArchTargetObjectFile.cpp
  MyArchTargetTransformInfo.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  Core
  CodeGen
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

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
