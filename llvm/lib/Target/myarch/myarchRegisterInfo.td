//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the myarch register file
//===----------------------------------------------------------------------===//

// For tablegen(... -gen-emitter)  in CMakeLists.txt
class myarchReg<bits<16> Enc, string n> : Register<n> {
  let HWEncoding = Enc;
}

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//
//@ All registers definition
let Namespace = "myarch" in {
  //@ General Purpose Registers
  def R0    : myarchReg<0,  "r0">,    DwarfRegNum<[0]>;  // gp?
  def R1    : myarchReg<1,  "r1">,    DwarfRegNum<[1]>;  // ra
  def R2    : myarchReg<2,  "r2">,    DwarfRegNum<[2]>;  // sp
  def R3    : myarchReg<3,  "r3">,    DwarfRegNum<[3]>;  // fp
  def R4    : myarchReg<4,  "r4">,    DwarfRegNum<[4]>;  // bp
  def R5    : myarchReg<5,  "r5">,    DwarfRegNum<[5]>;  // s0
  def R6    : myarchReg<6,  "r6">,    DwarfRegNum<[6]>;  // s1
  def R7    : myarchReg<7,  "r7">,    DwarfRegNum<[7]>;  // s2
  def R8    : myarchReg<8,  "r8">,    DwarfRegNum<[8]>;  // s3
  def R9    : myarchReg<9,  "r9">,    DwarfRegNum<[9]>;  // a0
  def R10   : myarchReg<10, "r10">,   DwarfRegNum<[10]>; // a1
  def R11   : myarchReg<11, "r11">,   DwarfRegNum<[11]>; // a2
  def R12   : myarchReg<12, "r12">,   DwarfRegNum<[12]>; // a3
  def R13   : myarchReg<13, "r13">,   DwarfRegNum<[13]>; // t0
  def R14   : myarchReg<14, "r14">,   DwarfRegNum<[14]>; // t1
  def R15   : myarchReg<15, "r15">,   DwarfRegNum<[15]>; // t2
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

def GPR : RegisterClass<"myarch", [i32], 32, (add
  R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15)>;
