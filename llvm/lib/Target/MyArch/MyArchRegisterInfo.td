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

// For tablegen(... -gen-emitter)  in CMakeLists.txt
class MyArchReg<bits<16> Enc, string n> : Register<n> {
  let HWEncoding = Enc;
}

// MyArch CPU Registers
class MyArchGPRReg<bits<16> Enc, string n> : MyArchReg<Enc, n>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//
//@ All registers definition
let Namespace = "MyArch" in {
  //@ General Purpose Registers
  def R0    : MyArchGPRReg<0,  "r0">,    DwarfRegNum<[0]>;
  def R1    : MyArchGPRReg<1,  "r1">,    DwarfRegNum<[1]>;
  def R2    : MyArchGPRReg<2,  "r2">,    DwarfRegNum<[2]>;
  def R3    : MyArchGPRReg<3,  "r3">,    DwarfRegNum<[3]>;
  def R4    : MyArchGPRReg<4,  "r4">,    DwarfRegNum<[4]>;
  def R5    : MyArchGPRReg<5,  "r5">,    DwarfRegNum<[5]>;
  def R6    : MyArchGPRReg<6,  "r6">,    DwarfRegNum<[6]>;
  def R7    : MyArchGPRReg<7,  "r7">,    DwarfRegNum<[7]>;
  def R8    : MyArchGPRReg<8,  "r8">,    DwarfRegNum<[8]>;
  def R9    : MyArchGPRReg<9,  "r9">,    DwarfRegNum<[9]>;
  def R10   : MyArchGPRReg<10, "r10">,   DwarfRegNum<[10]>;
  def R11   : MyArchGPRReg<11, "r11">,   DwarfRegNum<[11]>;
  def R12   : MyArchGPRReg<12, "r12">,   DwarfRegNum<[12]>;
  def R13   : MyArchGPRReg<13, "r13">,   DwarfRegNum<[13]>;
  def R14   : MyArchGPRReg<14, "r14">,   DwarfRegNum<[14]>;
  def R15   : MyArchGPRReg<15, "r15">,   DwarfRegNum<[15]>;
  def R16    : MyArchGPRReg<16,  "r16">,    DwarfRegNum<[16]>;
  def R17    : MyArchGPRReg<17,  "r17">,    DwarfRegNum<[17]>;
  def R18    : MyArchGPRReg<18,  "r18">,    DwarfRegNum<[18]>;
  def R19    : MyArchGPRReg<19,  "r19">,    DwarfRegNum<[19]>;
  def R20    : MyArchGPRReg<20,  "r20">,    DwarfRegNum<[20]>;
  def R21    : MyArchGPRReg<21,  "r21">,    DwarfRegNum<[21]>;
  def R22    : MyArchGPRReg<22,  "r22">,    DwarfRegNum<[22]>;
  def R23    : MyArchGPRReg<23,  "r23">,    DwarfRegNum<[23]>;
  def R24    : MyArchGPRReg<24,  "r24">,    DwarfRegNum<[24]>;
  def R25    : MyArchGPRReg<25,  "r25">,    DwarfRegNum<[25]>;
  def R26    : MyArchGPRReg<26,  "r26">,    DwarfRegNum<[26]>;
  def R27    : MyArchGPRReg<27,  "r27">,    DwarfRegNum<[27]>;
  def R28    : MyArchGPRReg<28,  "r28">,    DwarfRegNum<[28]>;
  def R29    : MyArchGPRReg<29,  "r29">,    DwarfRegNum<[29]>;
  def R30    : MyArchGPRReg<30,  "r30">,    DwarfRegNum<[30]>;
  def R31    : MyArchGPRReg<31,  "r31">,    DwarfRegNum<[31]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

def CPURegs : RegisterClass<"MyArch", [i32], 32, (add
  R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31)>;
