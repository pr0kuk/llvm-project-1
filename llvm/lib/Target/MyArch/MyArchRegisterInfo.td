//===-- MyArchRegisterInfo.td - MyArch Register defs -----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the MyArch register file
//===----------------------------------------------------------------------===//


// We have banks of 16 registers each.
class MyArchReg<bits<16> Enc, string n> : Register<n> {
  // For tablegen(... -gen-emitter)  in CMakeLists.txt
  let HWEncoding = Enc;
  
  let Namespace = "MyArch";
}

// MyArch CPU Registers
class MyArchGPRReg<bits<16> Enc, string n> : MyArchReg<Enc, n>;

// Co-processor 0 Registers
class MyArchC0Reg<bits<16> Enc, string n> : MyArchReg<Enc, n>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//
// The register string, such as "9" or "gp" will show on "llvm-objdump -d"
//@ All registers definition
let Namespace = "MyArch" in {
  //@ General Purpose Registers
  def ZERO : MyArchGPRReg<0,  "zero">, DwarfRegNum<[0]>;
  def AT   : MyArchGPRReg<1,  "1">,    DwarfRegNum<[1]>;
  def V0   : MyArchGPRReg<2,  "2">,    DwarfRegNum<[2]>;
  def V1   : MyArchGPRReg<3,  "3">,    DwarfRegNum<[3]>;
  def A0   : MyArchGPRReg<4,  "4">,    DwarfRegNum<[4]>;
  def A1   : MyArchGPRReg<5,  "5">,    DwarfRegNum<[5]>;
  def T9   : MyArchGPRReg<6,  "t9">,   DwarfRegNum<[6]>;
  def T0   : MyArchGPRReg<7,  "7">,    DwarfRegNum<[7]>;
  def T1   : MyArchGPRReg<8,  "8">,    DwarfRegNum<[8]>;
  def S0   : MyArchGPRReg<9,  "9">,    DwarfRegNum<[9]>;
  def S1   : MyArchGPRReg<10, "10">,   DwarfRegNum<[10]>;
  def GP   : MyArchGPRReg<11, "gp">,   DwarfRegNum<[11]>;
  def FP   : MyArchGPRReg<12, "fp">,   DwarfRegNum<[12]>;
  def SP   : MyArchGPRReg<13, "sp">,   DwarfRegNum<[13]>;
  def LR   : MyArchGPRReg<14, "lr">,   DwarfRegNum<[14]>;
  def SW   : MyArchGPRReg<15, "sw">,   DwarfRegNum<[15]>;
//  def MAR  : Register< 16, "mar">,  DwarfRegNum<[16]>;
//  def MDR  : Register< 17, "mdr">,  DwarfRegNum<[17]>;

//#if CH >= CH4_1 1
  // Hi/Lo registers number and name
  def HI   : MyArchReg<0, "ac0">, DwarfRegNum<[18]>;
  def LO   : MyArchReg<0, "ac0">, DwarfRegNum<[19]>;
//#endif
  def PC   : MyArchC0Reg<0, "pc">,  DwarfRegNum<[20]>;
  def EPC  : MyArchC0Reg<1, "epc">, DwarfRegNum<[21]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

def CPURegs : RegisterClass<"MyArch", [i32], 32, (add
  // Reserved
  ZERO, AT, 
  // Return Values and Arguments
  V0, V1, A0, A1, 
  // Not preserved across procedure calls
  T9, T0, T1,
  // Callee save
  S0, S1,
  // Reserved
  GP, FP, 
  SP, LR, SW)>;

//#if CH >= CH4_1 2
// Hi/Lo Registers class
def HILO   : RegisterClass<"MyArch", [i32], 32, (add HI, LO)>;
//#endif

//@Status Registers class
def SR     : RegisterClass<"MyArch", [i32], 32, (add SW)>;

//@Co-processor 0 Registers class
def C0Regs : RegisterClass<"MyArch", [i32], 32, (add PC, EPC)>;
//def GPROut : RegisterClass<"MyArch", [i32], 32, (add (sub CPURegs, SW))>;
