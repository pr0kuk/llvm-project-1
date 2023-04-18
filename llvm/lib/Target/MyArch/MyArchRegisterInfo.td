//===-- MyArchRegisterInfo.td - RISC-V Register defs --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the RISC-V register files
//===----------------------------------------------------------------------===//

let Namespace = "MyArch" in {
class MyArchReg<bits<5> Enc, string n, list<string> alt = []> : Register<n> {
  let HWEncoding{4-0} = Enc;
  let AltNames = alt;
}

class MyArchReg16<bits<5> Enc, string n, list<string> alt = []> : Register<n> {
  let HWEncoding{4-0} = Enc;
  let AltNames = alt;
}

def sub_16 : SubRegIndex<16>;
class MyArchReg32<MyArchReg16 subreg> : Register<""> {
  let HWEncoding{4-0} = subreg.HWEncoding{4-0};
  let SubRegs = [subreg];
  let SubRegIndices = [sub_16];
  let AsmName = subreg.AsmName;
  let AltNames = subreg.AltNames;
}

// Because MyArchReg64 register have AsmName and AltNames that alias with their
// 16/32-bit sub-register, MyArchAsmParser will need to coerce a register number
// from a MyArchReg16/MyArchReg32 to the equivalent MyArchReg64 when appropriate.
def sub_32 : SubRegIndex<32>;
class MyArchReg64<MyArchReg32 subreg> : Register<""> {
  let HWEncoding{4-0} = subreg.HWEncoding{4-0};
  let SubRegs = [subreg];
  let SubRegIndices = [sub_32];
  let AsmName = subreg.AsmName;
  let AltNames = subreg.AltNames;
}

class MyArchRegWithSubRegs<bits<5> Enc, string n, list<Register> subregs,
                          list<string> alt = []>
      : RegisterWithSubRegs<n, subregs> {
  let HWEncoding{4-0} = Enc;
  let AltNames = alt;
}

def ABIRegAltName : RegAltNameIndex;

def sub_vrm1_0 : SubRegIndex<64,  -1>;
def sub_vrm1_1 : SubRegIndex<64,  -1>;
def sub_vrm1_2 : SubRegIndex<64,  -1>;
def sub_vrm1_3 : SubRegIndex<64,  -1>;
def sub_vrm1_4 : SubRegIndex<64,  -1>;
def sub_vrm1_5 : SubRegIndex<64,  -1>;
def sub_vrm1_6 : SubRegIndex<64,  -1>;
def sub_vrm1_7 : SubRegIndex<64,  -1>;
def sub_vrm2_0 : SubRegIndex<128, -1>;
def sub_vrm2_1 : SubRegIndex<128, -1>;
def sub_vrm2_2 : SubRegIndex<128, -1>;
def sub_vrm2_3 : SubRegIndex<128, -1>;
def sub_vrm4_0 : SubRegIndex<256, -1>;
def sub_vrm4_1 : SubRegIndex<256, -1>;

} // Namespace = "MyArch"

// Integer registers
// CostPerUse is set higher for registers that may not be compressible as they
// are not part of GPRC, the most restrictive register class used by the
// compressed instruction set. This will influence the greedy register
// allocator to reduce the use of registers that can't be encoded in 16 bit
// instructions. This affects register allocation even when compressed
// instruction isn't targeted, we see no major negative codegen impact.

let RegAltNameIndices = [ABIRegAltName] in {
  def X0  : MyArchReg<0, "x0", ["r0"]>, DwarfRegNum<[0]>;
  let CostPerUse = 1 in {
  def X1  : MyArchReg<1, "x1", ["r1"]>, DwarfRegNum<[1]>;
  def X2  : MyArchReg<2, "x2", ["r2"]>, DwarfRegNum<[2]>;
  def X3  : MyArchReg<3, "x3", ["r3"]>, DwarfRegNum<[3]>;
  def X4  : MyArchReg<4, "x4", ["r4"]>, DwarfRegNum<[4]>;
  def X5  : MyArchReg<5, "x5", ["r5"]>, DwarfRegNum<[5]>;
  def X6  : MyArchReg<6, "x6", ["r6"]>, DwarfRegNum<[6]>;
  def X7  : MyArchReg<7, "x7", ["r7"]>, DwarfRegNum<[7]>;
  }
  def X8  : MyArchReg<8, "x8", ["r8", "fp"]>, DwarfRegNum<[8]>;
  def X9  : MyArchReg<9, "x9", ["r9"]>, DwarfRegNum<[9]>;
  def X10 : MyArchReg<10,"x10", ["r10"]>, DwarfRegNum<[10]>;
  def X11 : MyArchReg<11,"x11", ["r11"]>, DwarfRegNum<[11]>;
  def X12 : MyArchReg<12,"x12", ["r12"]>, DwarfRegNum<[12]>;
  def X13 : MyArchReg<13,"x13", ["r13"]>, DwarfRegNum<[13]>;
  def X14 : MyArchReg<14,"x14", ["r14"]>, DwarfRegNum<[14]>;
  def X15 : MyArchReg<15,"x15", ["r15"]>, DwarfRegNum<[15]>;
  let CostPerUse = 1 in {
  def X16 : MyArchReg<16,"x16", ["r16"]>, DwarfRegNum<[16]>;
  def X17 : MyArchReg<17,"x17", ["r17"]>, DwarfRegNum<[17]>;
  def X18 : MyArchReg<18,"x18", ["r18"]>, DwarfRegNum<[18]>;
  def X19 : MyArchReg<19,"x19", ["r19"]>, DwarfRegNum<[19]>;
  def X20 : MyArchReg<20,"x20", ["r20"]>, DwarfRegNum<[20]>;
  def X21 : MyArchReg<21,"x21", ["r21"]>, DwarfRegNum<[21]>;
  def X22 : MyArchReg<22,"x22", ["r22"]>, DwarfRegNum<[22]>;
  def X23 : MyArchReg<23,"x23", ["r23"]>, DwarfRegNum<[23]>;
  def X24 : MyArchReg<24,"x24", ["r24"]>, DwarfRegNum<[24]>;
  def X25 : MyArchReg<25,"x25", ["r25"]>, DwarfRegNum<[25]>;
  def X26 : MyArchReg<26,"x26", ["r26"]>, DwarfRegNum<[26]>;
  def X27 : MyArchReg<27,"x27", ["r27"]>, DwarfRegNum<[27]>;
  def X28 : MyArchReg<28,"x28", ["r28"]>, DwarfRegNum<[28]>;
  def X29 : MyArchReg<29,"x29", ["r29"]>, DwarfRegNum<[29]>;
  def X30 : MyArchReg<30,"x30", ["r30"]>, DwarfRegNum<[30]>;
  def X31 : MyArchReg<31,"x31", ["r31"]>, DwarfRegNum<[31]>;
  }
}

def XLenVT : ValueTypeByHwMode<[RV32, RV64],
                               [i32,  i64]>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
def GPR : RegisterClass<"MyArch", [XLenVT], 32, (add
    (sequence "X%u", 10, 17),
    (sequence "X%u", 5, 7),
    (sequence "X%u", 28, 31),
    (sequence "X%u", 8, 9),
    (sequence "X%u", 18, 27),
    (sequence "X%u", 0, 4)
  )> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

def GPRX0 : RegisterClass<"MyArch", [XLenVT], 32, (add X0)> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
def GPRNoX0 : RegisterClass<"MyArch", [XLenVT], 32, (add
    (sequence "X%u", 10, 17),
    (sequence "X%u", 5, 7),
    (sequence "X%u", 28, 31),
    (sequence "X%u", 8, 9),
    (sequence "X%u", 18, 27),
    (sequence "X%u", 1, 4)
  )> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

def GPRNoX0X2 : RegisterClass<"MyArch", [XLenVT], 32, (add
    (sequence "X%u", 10, 17),
    (sequence "X%u", 5, 7),
    (sequence "X%u", 28, 31),
    (sequence "X%u", 8, 9),
    (sequence "X%u", 18, 27),
    X1, X3, X4
  )> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

def GPRC : RegisterClass<"MyArch", [XLenVT], 32, (add
    (sequence "X%u", 10, 15),
    (sequence "X%u", 8, 9)
  )> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

// For indirect tail calls, we can't use callee-saved registers, as they are
// restored to the saved value before the tail call, which would clobber a call
// address.
def GPRTC : RegisterClass<"MyArch", [XLenVT], 32, (add
    (sequence "X%u", 5, 7),
    (sequence "X%u", 10, 17),
    (sequence "X%u", 28, 31)
  )> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

def SP : RegisterClass<"MyArch", [XLenVT], 32, (add X2)> {
  let RegInfos = RegInfoByHwMode<
      [RV32,              RV64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;
}

// Floating point registers
let RegAltNameIndices = [ABIRegAltName] in {
  def F0_H  : MyArchReg16<0, "f0", ["ft0"]>, DwarfRegNum<[32]>;
  def F1_H  : MyArchReg16<1, "f1", ["ft1"]>, DwarfRegNum<[33]>;
  def F2_H  : MyArchReg16<2, "f2", ["ft2"]>, DwarfRegNum<[34]>;
  def F3_H  : MyArchReg16<3, "f3", ["ft3"]>, DwarfRegNum<[35]>;
  def F4_H  : MyArchReg16<4, "f4", ["ft4"]>, DwarfRegNum<[36]>;
  def F5_H  : MyArchReg16<5, "f5", ["ft5"]>, DwarfRegNum<[37]>;
  def F6_H  : MyArchReg16<6, "f6", ["ft6"]>, DwarfRegNum<[38]>;
  def F7_H  : MyArchReg16<7, "f7", ["ft7"]>, DwarfRegNum<[39]>;
  def F8_H  : MyArchReg16<8, "f8", ["fs0"]>, DwarfRegNum<[40]>;
  def F9_H  : MyArchReg16<9, "f9", ["fs1"]>, DwarfRegNum<[41]>;
  def F10_H : MyArchReg16<10,"f10", ["fa0"]>, DwarfRegNum<[42]>;
  def F11_H : MyArchReg16<11,"f11", ["fa1"]>, DwarfRegNum<[43]>;
  def F12_H : MyArchReg16<12,"f12", ["fa2"]>, DwarfRegNum<[44]>;
  def F13_H : MyArchReg16<13,"f13", ["fa3"]>, DwarfRegNum<[45]>;
  def F14_H : MyArchReg16<14,"f14", ["fa4"]>, DwarfRegNum<[46]>;
  def F15_H : MyArchReg16<15,"f15", ["fa5"]>, DwarfRegNum<[47]>;
  def F16_H : MyArchReg16<16,"f16", ["fa6"]>, DwarfRegNum<[48]>;
  def F17_H : MyArchReg16<17,"f17", ["fa7"]>, DwarfRegNum<[49]>;
  def F18_H : MyArchReg16<18,"f18", ["fs2"]>, DwarfRegNum<[50]>;
  def F19_H : MyArchReg16<19,"f19", ["fs3"]>, DwarfRegNum<[51]>;
  def F20_H : MyArchReg16<20,"f20", ["fs4"]>, DwarfRegNum<[52]>;
  def F21_H : MyArchReg16<21,"f21", ["fs5"]>, DwarfRegNum<[53]>;
  def F22_H : MyArchReg16<22,"f22", ["fs6"]>, DwarfRegNum<[54]>;
  def F23_H : MyArchReg16<23,"f23", ["fs7"]>, DwarfRegNum<[55]>;
  def F24_H : MyArchReg16<24,"f24", ["fs8"]>, DwarfRegNum<[56]>;
  def F25_H : MyArchReg16<25,"f25", ["fs9"]>, DwarfRegNum<[57]>;
  def F26_H : MyArchReg16<26,"f26", ["fs10"]>, DwarfRegNum<[58]>;
  def F27_H : MyArchReg16<27,"f27", ["fs11"]>, DwarfRegNum<[59]>;
  def F28_H : MyArchReg16<28,"f28", ["ft8"]>, DwarfRegNum<[60]>;
  def F29_H : MyArchReg16<29,"f29", ["ft9"]>, DwarfRegNum<[61]>;
  def F30_H : MyArchReg16<30,"f30", ["ft10"]>, DwarfRegNum<[62]>;
  def F31_H : MyArchReg16<31,"f31", ["ft11"]>, DwarfRegNum<[63]>;

  foreach Index = 0-31 in {
    def F#Index#_F : MyArchReg32<!cast<MyArchReg16>("F"#Index#"_H")>,
      DwarfRegNum<[!add(Index, 32)]>;
  }

  foreach Index = 0-31 in {
    def F#Index#_D : MyArchReg64<!cast<MyArchReg32>("F"#Index#"_F")>,
      DwarfRegNum<[!add(Index, 32)]>;
  }
}

// The order of registers represents the preferred allocation sequence,
// meaning caller-save regs are listed before callee-save.
def FPR16 : RegisterClass<"MyArch", [f16], 16, (add
    (sequence "F%u_H", 0, 7),
    (sequence "F%u_H", 10, 17),
    (sequence "F%u_H", 28, 31),
    (sequence "F%u_H", 8, 9),
    (sequence "F%u_H", 18, 27)
)>;

def FPR32 : RegisterClass<"MyArch", [f32], 32, (add
    (sequence "F%u_F", 0, 7),
    (sequence "F%u_F", 10, 17),
    (sequence "F%u_F", 28, 31),
    (sequence "F%u_F", 8, 9),
    (sequence "F%u_F", 18, 27)
)>;

def FPR32C : RegisterClass<"MyArch", [f32], 32, (add
  (sequence "F%u_F", 10, 15),
  (sequence "F%u_F", 8, 9)
)>;

// The order of registers represents the preferred allocation sequence,
// meaning caller-save regs are listed before callee-save.
def FPR64 : RegisterClass<"MyArch", [f64], 64, (add
    (sequence "F%u_D", 0, 7),
    (sequence "F%u_D", 10, 17),
    (sequence "F%u_D", 28, 31),
    (sequence "F%u_D", 8, 9),
    (sequence "F%u_D", 18, 27)
)>;

def FPR64C : RegisterClass<"MyArch", [f64], 64, (add
  (sequence "F%u_D", 10, 15),
  (sequence "F%u_D", 8, 9)
)>;

// Vector type mapping to LLVM types.
//
// Though the V extension allows that VLEN be as small as 8,
// this approach assumes that VLEN>=64.
// Additionally, the only supported ELEN values are 32 and 64,
// thus `vscale` can be defined as VLEN/64,
// allowing the same types with either ELEN value.
//
//         MF8    MF4     MF2     M1      M2      M4       M8
// i64*    N/A    N/A     N/A     nxv1i64 nxv2i64 nxv4i64  nxv8i64
// i32     N/A    N/A     nxv1i32 nxv2i32 nxv4i32 nxv8i32  nxv16i32
// i16     N/A    nxv1i16 nxv2i16 nxv4i16 nxv8i16 nxv16i16 nxv32i16
// i8      nxv1i8 nxv2i8  nxv4i8  nxv8i8  nxv16i8 nxv32i8  nxv64i8
// double* N/A    N/A     N/A     nxv1f64 nxv2f64 nxv4f64  nxv8f64
// float   N/A    N/A     nxv1f32 nxv2f32 nxv4f32 nxv8f32  nxv16f32
// half    N/A    nxv1f16 nxv2f16 nxv4f16 nxv8f16 nxv16f16 nxv32f16
// * ELEN=64

defvar vint8mf8_t = nxv1i8;
defvar vint8mf4_t = nxv2i8;
defvar vint8mf2_t = nxv4i8;
defvar vint8m1_t = nxv8i8;
defvar vint8m2_t = nxv16i8;
defvar vint8m4_t = nxv32i8;
defvar vint8m8_t = nxv64i8;

defvar vint16mf4_t = nxv1i16;
defvar vint16mf2_t = nxv2i16;
defvar vint16m1_t  = nxv4i16;
defvar vint16m2_t  = nxv8i16;
defvar vint16m4_t  = nxv16i16;
defvar vint16m8_t  = nxv32i16;

defvar vint32mf2_t = nxv1i32;
defvar vint32m1_t  = nxv2i32;
defvar vint32m2_t  = nxv4i32;
defvar vint32m4_t  = nxv8i32;
defvar vint32m8_t  = nxv16i32;

defvar vint64m1_t = nxv1i64;
defvar vint64m2_t = nxv2i64;
defvar vint64m4_t = nxv4i64;
defvar vint64m8_t = nxv8i64;

defvar vfloat16mf4_t = nxv1f16;
defvar vfloat16mf2_t = nxv2f16;
defvar vfloat16m1_t  = nxv4f16;
defvar vfloat16m2_t  = nxv8f16;
defvar vfloat16m4_t  = nxv16f16;
defvar vfloat16m8_t  = nxv32f16;

defvar vfloat32mf2_t = nxv1f32;
defvar vfloat32m1_t  = nxv2f32;
defvar vfloat32m2_t  = nxv4f32;
defvar vfloat32m4_t  = nxv8f32;
defvar vfloat32m8_t  = nxv16f32;

defvar vfloat64m1_t = nxv1f64;
defvar vfloat64m2_t = nxv2f64;
defvar vfloat64m4_t = nxv4f64;
defvar vfloat64m8_t = nxv8f64;

defvar vbool1_t  = nxv64i1;
defvar vbool2_t  = nxv32i1;
defvar vbool4_t  = nxv16i1;
defvar vbool8_t  = nxv8i1;
defvar vbool16_t = nxv4i1;
defvar vbool32_t = nxv2i1;
defvar vbool64_t = nxv1i1;

// There is no need to define register classes for fractional LMUL.
def LMULList {
  list<int> m = [1, 2, 4, 8];
}

//===----------------------------------------------------------------------===//
// Utility classes for segment load/store.
//===----------------------------------------------------------------------===//
// The set of legal NF for LMUL = lmul.
// LMUL == 1, NF = 2, 3, 4, 5, 6, 7, 8
// LMUL == 2, NF = 2, 3, 4
// LMUL == 4, NF = 2
class NFList<int lmul> {
  list<int> L = !cond(!eq(lmul, 1): [2, 3, 4, 5, 6, 7, 8],
                      !eq(lmul, 2): [2, 3, 4],
                      !eq(lmul, 4): [2],
                      !eq(lmul, 8): []);
}

// Generate [start, end) SubRegIndex list.
class SubRegSet<list<SubRegIndex> LIn, int start, int nf, int lmul> {
  list<SubRegIndex> L = !foldl([]<SubRegIndex>,
                               [0, 1, 2, 3, 4, 5, 6, 7],
                               AccList, i,
                               !listconcat(AccList,
                                 !if(!lt(i, nf),
                                   [!cast<SubRegIndex>("sub_vrm" # lmul # "_" # i)],
                                   [])));
}

class IndexSet<int index, int nf, int lmul> {
  list<int> R =
    !foldl([]<int>,
              [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
               13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
               23, 24, 25, 26, 27, 28, 29, 30, 31],
              L, i,
              !listconcat(L,
                          !if(!and(
                                !le(!mul(index, lmul), !mul(i, lmul)),
                                !le(!mul(i, lmul),
                                    !sub(!add(32, !mul(index, lmul)), !mul(nf, lmul)))
                              ), [!mul(i, lmul)], [])));
}

class VRegList<list<dag> LIn, int start, int nf, int lmul> {
  list<dag> L =
    !if(!ge(start, nf),
        LIn,
        !listconcat(
          [!dag(add,
                !foreach(i, IndexSet<start, nf, lmul>.R,
                         !cast<Register>("V" # i # !cond(!eq(lmul, 2): "M2",
                                                         !eq(lmul, 4): "M4",
                                                         true: ""))),
                !listsplat("", !size(IndexSet<start, nf, lmul>.R)))],
          VRegList<LIn, !add(start, 1), nf, lmul>.L));
}

// Vector registers
let RegAltNameIndices = [ABIRegAltName] in {
  foreach Index = 0-31 in {
    def V#Index : MyArchReg<Index, "v"#Index, ["v"#Index]>, DwarfRegNum<[!add(Index, 96)]>;
  }

  foreach Index = [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22,
                   24, 26, 28, 30] in {
    def V#Index#M2 : MyArchRegWithSubRegs<Index, "v"#Index,
                       [!cast<Register>("V"#Index),
                        !cast<Register>("V"#!add(Index, 1))],
                       ["v"#Index]>,
                     DwarfRegAlias<!cast<Register>("V"#Index)> {
      let SubRegIndices = [sub_vrm1_0, sub_vrm1_1];
    }
  }

  foreach Index = [0, 4, 8, 12, 16, 20, 24, 28] in {
    def V#Index#M4 : MyArchRegWithSubRegs<Index, "v"#Index,
                       [!cast<Register>("V"#Index#"M2"),
                        !cast<Register>("V"#!add(Index, 2)#"M2")],
                       ["v"#Index]>,
                     DwarfRegAlias<!cast<Register>("V"#Index)> {
      let SubRegIndices = [sub_vrm2_0, sub_vrm2_1];
    }
  }

  foreach Index = [0, 8, 16, 24] in {
    def V#Index#M8 : MyArchRegWithSubRegs<Index, "v"#Index,
                       [!cast<Register>("V"#Index#"M4"),
                        !cast<Register>("V"#!add(Index, 4)#"M4")],
                       ["v"#Index]>,
                     DwarfRegAlias<!cast<Register>("V"#Index)> {
      let SubRegIndices = [sub_vrm4_0, sub_vrm4_1];
    }
  }

  def VTYPE  : MyArchReg<0, "vtype", ["vtype"]>;
  def VL     : MyArchReg<0, "vl", ["vl"]>;
  def VXSAT  : MyArchReg<0, "vxsat", ["vxsat"]>;
  def VXRM   : MyArchReg<0, "vxrm", ["vxrm"]>;
}

foreach m = [1, 2, 4] in {
  foreach n = NFList<m>.L in {
    def "VN" # n # "M" # m: RegisterTuples<SubRegSet<[], 0, n, m>.L,
                                           VRegList<[], 0, n, m>.L>;
  }
}

class VReg<list<ValueType> regTypes, dag regList, int Vlmul>
  : RegisterClass<"MyArch",
                  regTypes,
                  64, // The maximum supported ELEN is 64.
                  regList> {
  int VLMul = Vlmul;
  int Size = !mul(Vlmul, 64);
}

def VR : VReg<[vint8mf2_t, vint8mf4_t, vint8mf8_t,
               vint16mf2_t, vint16mf4_t, vint32mf2_t,
               vint8m1_t, vint16m1_t, vint32m1_t, vint64m1_t,
               vfloat16mf4_t, vfloat16mf2_t, vfloat16m1_t,
               vfloat32mf2_t, vfloat32m1_t, vfloat64m1_t,
               vbool64_t, vbool32_t, vbool16_t, vbool8_t, vbool4_t,
               vbool2_t, vbool1_t],
           (add (sequence "V%u", 25, 31),
                (sequence "V%u", 8, 24),
                (sequence "V%u", 0, 7)), 1>;

def VRNoV0 : VReg<[vint8mf2_t, vint8mf4_t, vint8mf8_t,
                   vint16mf2_t, vint16mf4_t, vint32mf2_t,
                   vint8m1_t, vint16m1_t, vint32m1_t, vint64m1_t,
                   vfloat16mf4_t, vfloat16mf2_t, vfloat16m1_t,
                   vfloat32mf2_t, vfloat32m1_t, vfloat64m1_t,
                   vbool64_t, vbool32_t, vbool16_t, vbool8_t, vbool4_t,
                   vbool2_t, vbool1_t],
               (add (sequence "V%u", 25, 31),
                    (sequence "V%u", 8, 24),
                    (sequence "V%u", 1, 7)), 1>;

def VRM2 : VReg<[vint8m2_t, vint16m2_t, vint32m2_t, vint64m2_t,
                 vfloat16m2_t, vfloat32m2_t, vfloat64m2_t],
             (add V26M2, V28M2, V30M2, V8M2, V10M2, V12M2, V14M2, V16M2,
                  V18M2, V20M2, V22M2, V24M2, V0M2, V2M2, V4M2, V6M2), 2>;

def VRM2NoV0 : VReg<[vint8m2_t, vint16m2_t, vint32m2_t, vint64m2_t,
                     vfloat16m2_t, vfloat32m2_t, vfloat64m2_t],
                 (add V26M2, V28M2, V30M2, V8M2, V10M2, V12M2, V14M2, V16M2,
                      V18M2, V20M2, V22M2, V24M2, V2M2, V4M2, V6M2), 2>;

def VRM4 : VReg<[vint8m4_t, vint16m4_t, vint32m4_t, vint64m4_t,
                 vfloat16m4_t, vfloat32m4_t, vfloat64m4_t],
             (add V28M4, V8M4, V12M4, V16M4, V20M4, V24M4, V0M4, V4M4), 4>;

def VRM4NoV0 : VReg<[vint8m4_t, vint16m4_t, vint32m4_t, vint64m4_t,
                     vfloat16m4_t, vfloat32m4_t, vfloat64m4_t],
                 (add V28M4, V8M4, V12M4, V16M4, V20M4, V24M4, V4M4), 4>;

def VRM8 : VReg<[vint8m8_t, vint16m8_t, vint32m8_t, vint64m8_t,
                 vfloat16m8_t, vfloat32m8_t, vfloat64m8_t],
             (add V8M8, V16M8, V24M8, V0M8), 8>;

def VRM8NoV0 : VReg<[vint8m8_t, vint16m8_t, vint32m8_t, vint64m8_t,
                     vfloat16m8_t, vfloat32m8_t, vfloat64m8_t],
                 (add V8M8, V16M8, V24M8), 8>;

defvar VMaskVTs = [vbool64_t, vbool32_t, vbool16_t, vbool8_t,
                   vbool4_t, vbool2_t, vbool1_t];

def VMV0 : RegisterClass<"MyArch", VMaskVTs, 64, (add V0)> {
  let Size = 64;
}

foreach m = LMULList.m in {
  foreach nf = NFList<m>.L in {
    def "VRN" # nf # "M" # m : VReg<[untyped],
                               (add !cast<RegisterTuples>("VN" # nf # "M" # m)),
                                    !mul(nf, m)>;
  }
}
