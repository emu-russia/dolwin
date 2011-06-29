// interpreter tables setup
#include "dolphin.h"

// opcode tables
void (__fastcall *c_19[2048])(u32 op);      // 19
void (__fastcall *c_31[2048])(u32 op);      // 31
void (__fastcall *c_59[64])(u32 op);        // 59
void (__fastcall *c_63[2048])(u32 op);      // 63
void (__fastcall *c_4 [2048])(u32 op);      // 4

#define OP(name) void __fastcall c_##name##(u32 op)

// not implemented opcode
OP(NI)
{
    char instr[256];
    DISA d;
    
    // disassemble
    d.op = op;
    d.pc = PC;
    dasm(&d);
    sprintf(
        instr, "%08X  <%08X>  (%i, %i)  %-10s %s",
        PC, op, op >> 26, op & 0x7ff, d.opcode, d.operands
    );

    if(emu.doldebug)
    {
        DBHalt( "** CPU ERROR **\n"
                "unimplemented opcode : %s\n\n",
                instr );
    }

    char dump[1024];

    sprintf(dump,
        "CPU STATE : \n"
        "\n"
        "r0 :%08X\tr8 :%08X\tr16:%08X\tr24:%08X\n"
        "sp :%08X\tr9 :%08X\tr17:%08X\tr25:%08X\n"
        "sd2:%08X\tr10:%08X\tr18:%08X\tr26:%08X\n"
        "r3 :%08X\tr11:%08X\tr19:%08X\tr27:%08X\n"
        "r4 :%08X\tr12:%08X\tr20:%08X\tr28:%08X\n"
        "r5 :%08X\tsd1:%08X\tr21:%08X\tr29:%08X\n"
        "r6 :%08X\tr14:%08X\tr22:%08X\tr30:%08X\n"
        "r7 :%08X\tr15:%08X\tr23:%08X\tr31:%08X\n"
        "\n"
        "lr :%08X\tcr :%08X\tdec:%08X\n"
        "pc :%08X\txer:%08X\tctr:%08X\n",
        GPR[ 0], GPR[ 8], GPR[16], GPR[24],
        GPR[ 1], GPR[ 9], GPR[17], GPR[25],
        GPR[ 2], GPR[10], GPR[18], GPR[26],
        GPR[ 3], GPR[11], GPR[19], GPR[27],
        GPR[ 4], GPR[12], GPR[20], GPR[28],
        GPR[ 5], GPR[13], GPR[21], GPR[29],
        GPR[ 6], GPR[14], GPR[22], GPR[30],
        GPR[ 7], GPR[15], GPR[23], GPR[31],
        LR, CR, DEC,
        PC, XER, CTR
    );

    DolwinError( "** CPU ERROR **",
            "unimplemented opcode : %s\n\n"
            "%s\n",
            instr, dump );
}

// switch to extension opcode table
static OP(OP19) { c_19[op & 0x7ff](op); }
static OP(OP31) { c_31[op & 0x7ff](op); }
static OP(OP59) { c_59[op &  0x3f](op); }
static OP(OP63) { c_63[op & 0x7ff](op); }
static OP(OP4)  { c_4 [op & 0x7ff](op); }

// high level call
OP(HL)
{
    // Dolwin module base should be specified in project properties
    void (*pcall)() = (void (*)())(op);

    if(op == 0)
    {
        DBHalt(
            "Something goes wrong in interpreter, \n"
            "program is trying to execute NULL opcode.\n\n"
            "pc:%08X, time:%s", PC, OSTimeFormat(UTBR)
        );
        return;
    }

    pcall();
}

// setup main opcode table
void (__fastcall *c_1[64])(u32 op) = {
 c_HL    , c_NI    , c_NI    , c_TWI   , c_OP4   , c_NI    , c_NI    , c_MULLI,
 c_SUBFIC, c_NI    , c_CMPLI , c_CMPI  , c_ADDIC , c_ADDICD, c_ADDI  , c_ADDIS,
 c_BCX   , c_SC    , c_BX    , c_OP19  , c_RLWIMI, c_RLWINM, c_NI    , c_RLWNM,
 c_ORI   , c_ORIS  , c_XORI  , c_XORIS , c_ANDID , c_ANDISD, c_NI    , c_OP31,
 c_LWZ   , c_LWZU  , c_LBZ   , c_LBZU  , c_STW   , c_STWU  , c_STB   , c_STBU,
 c_LHZ   , c_LHZU  , c_LHA   , c_LHAU  , c_STH   , c_STHU  , c_LMW   , c_STMW,
 c_LFS   , c_LFSU  , c_LFD   , c_LFDU  , c_STFS  , c_STFSU , c_STFD  , c_STFDU,
 c_PSQ_L , c_PSQ_LU, c_NI    , c_OP59  , c_PSQ_ST,c_PSQ_STU, c_NI    , c_OP63
};

// setup extension tables
void IPTInitTables()
{
    int i;
    u8 scale;

    // build rotate mask table
    for(int mb=0; mb<32; mb++)
    {
        for(int me=0; me<32; me++)
        {
            u32 mask = ((u32)-1 >> mb) ^ ((me >= 31) ? 0 : ((u32)-1) >> (me + 1));
            cpu.rotmask[mb][me] = (mb > me) ? (~mask) : (mask);
        }
    }

    // build paired-single load scale
    for(scale=0; scale<64; scale++)
    {
        int factor;
        if(scale & 0x20)    // -32 ... -1
        {
            factor = -32 + (scale & 0x1f);
        }
        else                // 0 ... 31
        {
            factor = 0 + (scale & 0x1f);
        }
        cpu.ldScale[scale] = powf(2, -1.0f * (f32)factor);
    }

    // build paired-single store scale
    for(scale=0; scale<64; scale++)
    {
        int factor;
        if(scale & 0x20)    // -32 ... -1
        {
            factor = -32 + (scale & 0x1f);
        }
        else                // 0 ... 31
        {
            factor = 0 + (scale & 0x1f);
        }
        cpu.stScale[scale] = powf(2, +1.0f * (f32)factor);
    }

    // set all tables to default "not implemented" opcode
    for(i=0; i<2048; i++)
    {
        if(i < 64) c_59[i] = c_NI;
        c_19[i] = c_31[i] = c_63[i] = c_NI;
        c_4[i] = c_NI;
    }

    // "19" extension
    c_19[   0] = c_MCRF;
    c_19[  32] = c_BCLR;
    c_19[  33] = c_BCLRL;
    c_19[  66] = c_CRNOR;
    c_19[ 100] = c_RFI;
    c_19[ 258] = c_CRANDC;
    c_19[ 300] = c_ISYNC;
    c_19[ 386] = c_CRXOR;
    c_19[ 450] = c_CRNAND;
    c_19[ 514] = c_CRAND;
    c_19[ 578] = c_CREQV;
    c_19[ 834] = c_CRORC;
    c_19[ 898] = c_CROR;
    c_19[1056] = c_BCCTR;
    c_19[1057] = c_BCCTRL;

    // "31" extension
    c_31[   0] = c_CMP;
    c_31[   8] = c_TW;
    c_31[  16] = c_SUBFC;
    c_31[  17] = c_SUBFCD;
    c_31[  20] = c_ADDC;
    c_31[  21] = c_ADDCD;
    c_31[  22] = c_MULHWU;
    c_31[  23] = c_MULHWUD;
    c_31[  38] = c_MFCR;
    c_31[  40] = c_LWARX;
    c_31[  46] = c_LWZX;
    c_31[  48] = c_SLW;
    c_31[  49] = c_SLWD;
    c_31[  52] = c_CNTLZW;
    c_31[  53] = c_CNTLZWD;
    c_31[  56] = c_AND;
    c_31[  57] = c_ANDD;
    c_31[  64] = c_CMPL;
    c_31[  80] = c_SUBF;
    c_31[  81] = c_SUBFD;
    c_31[ 108] = c_DCBST;
    c_31[ 110] = c_LWZUX;
    c_31[ 120] = c_ANDC;
    c_31[ 121] = c_ANDCD;
    c_31[ 150] = c_MULHW;
    c_31[ 151] = c_MULHWD;
    c_31[ 166] = c_MFMSR;
    c_31[ 172] = c_DCBF;
    c_31[ 174] = c_LBZX;
    c_31[ 208] = c_NEG;
    c_31[ 209] = c_NEGD;
    c_31[ 238] = c_LBZUX;
    c_31[ 248] = c_NOR;
    c_31[ 249] = c_NORD;
    c_31[ 272] = c_SUBFE;
    c_31[ 273] = c_SUBFED;
    c_31[ 276] = c_ADDE;
    c_31[ 277] = c_ADDED;
    c_31[ 288] = c_MTCRF;
    c_31[ 292] = c_MTMSR;
    c_31[ 301] = c_STWCXD;
    c_31[ 302] = c_STWX;
    c_31[ 366] = c_STWUX;
    c_31[ 400] = c_SUBFZE;
    c_31[ 401] = c_SUBFZED;
    c_31[ 404] = c_ADDZE;
    c_31[ 405] = c_ADDZED;
    c_31[ 420] = c_MTSR;
    c_31[ 430] = c_STBX;
    c_31[ 464] = c_SUBFME;
    c_31[ 465] = c_SUBFMED;
    c_31[ 468] = c_ADDME;
    c_31[ 469] = c_ADDMED;
    c_31[ 470] = c_MULLW;
    c_31[ 471] = c_MULLWD;
    c_31[ 484] = c_MTSRIN;
    c_31[ 492] = c_DCBTST;
    c_31[ 494] = c_STBUX;
    c_31[ 532] = c_ADD;
    c_31[ 533] = c_ADDD;
    c_31[ 556] = c_DCBT;
    c_31[ 558] = c_LHZX;
    c_31[ 568] = c_EQV;
    c_31[ 569] = c_EQVD;
    c_31[ 612] = c_TLBIE;
    c_31[ 622] = c_LHZUX;
    c_31[ 632] = c_XOR;
    c_31[ 633] = c_XORD;
    c_31[ 678] = c_MFSPR;
    c_31[ 686] = c_LHAX;
    c_31[ 742] = c_MFTB;
    c_31[ 750] = c_LHAUX;
    c_31[ 814] = c_STHX;
    c_31[ 824] = c_ORC;
    c_31[ 825] = c_ORCD;
    c_31[ 878] = c_STHUX;
    c_31[ 888] = c_OR;
    c_31[ 889] = c_ORD;
    c_31[ 918] = c_DIVWU;
    c_31[ 919] = c_DIVWUD;
    c_31[ 934] = c_MTSPR;
    c_31[ 940] = c_DCBI;
    c_31[ 952] = c_NAND;
    c_31[ 953] = c_NANDD;
    c_31[ 982] = c_DIVW;
    c_31[ 983] = c_DIVWD;
    c_31[1024] = c_MCRXR;
    c_31[1044] = c_ADDCO;
    c_31[1045] = c_ADDCOD;
    c_31[1068] = c_LWBRX;
    c_31[1070] = c_LFSX;
    c_31[1072] = c_SRW;
    c_31[1073] = c_SRWD;
    c_31[1132] = c_TLBSYNC;
    c_31[1134] = c_LFSUX;
    c_31[1190] = c_MFSR;
    c_31[1194] = c_LSWI;
    c_31[1196] = c_SYNC;
    c_31[1198] = c_LFDX;
    c_31[1262] = c_LFDUX;
    c_31[1318] = c_MFSRIN;
    c_31[1324] = c_STWBRX;
    c_31[1326] = c_STFSX;
    c_31[1390] = c_STFSUX;
    c_31[1450] = c_STSWI;
    c_31[1454] = c_STFDX;
    c_31[1518] = c_STFDUX;
    c_31[1556] = c_ADDO;
    c_31[1557] = c_ADDOD;
    c_31[1580] = c_LHBRX;
    c_31[1584] = c_SRAW;
    c_31[1585] = c_SRAWD;
    c_31[1648] = c_SRAWI;
    c_31[1649] = c_SRAWID;
    c_31[1708] = c_EIEIO;
    c_31[1836] = c_STHBRX;
    c_31[1844] = c_EXTSH;
    c_31[1845] = c_EXTSHD;
    c_31[1908] = c_EXTSB;
    c_31[1909] = c_EXTSBD;
    c_31[1964] = c_ICBI;
    c_31[1966] = c_STFIWX;
    c_31[2028] = c_DCBZ;

    // "59" extension
    c_59[  36] = c_FDIVS;
    c_59[  37] = c_FDIVSD;
    c_59[  40] = c_FSUBS;
    c_59[  41] = c_FSUBSD;
    c_59[  42] = c_FADDS;
    c_59[  43] = c_FADDSD;
    c_59[  48] = c_FRES;
    c_59[  49] = c_FRESD;
    c_59[  50] = c_FMULS;
    c_59[  51] = c_FMULSD;
    c_59[  56] = c_FMSUBS;
    c_59[  57] = c_FMSUBSD;
    c_59[  58] = c_FMADDS;
    c_59[  59] = c_FMADDSD;
    c_59[  60] = c_FNMSUBS;
    c_59[  61] = c_FNMSUBSD;
    c_59[  62] = c_FNMADDS;
    c_59[  63] = c_FNMADDSD;

    // "63" extension
    c_63[   0] = c_FCMPU;
    c_63[  24] = c_FRSP;
    c_63[  25] = c_FRSPD;
    c_63[  28] = c_FCTIW;
    c_63[  29] = c_FCTIWD;
    c_63[  30] = c_FCTIWZ;
    c_63[  31] = c_FCTIWZD;
    c_63[  36] = c_FDIV;
    c_63[  37] = c_FDIVD;
    c_63[  40] = c_FSUB;
    c_63[  41] = c_FSUBD;
    c_63[  42] = c_FADD;
    c_63[  43] = c_FADDD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 46] = c_FSEL;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 47] = c_FSELD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 50] = c_FMUL;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 51] = c_FMULD;
    c_63[  52] = c_FRSQRTE;
    c_63[  53] = c_FRSQRTED;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 56] = c_FMSUB;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 57] = c_FMSUBD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 58] = c_FMADD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 59] = c_FMADDD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 60] = c_FNMSUB;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 61] = c_FNMSUBD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 62] = c_FNMADD;
    for(i = 0; i < 32; i++) c_63[(i << 6) | 63] = c_FNMADDD;
    c_63[  64] = c_FCMPO;
    c_63[  76] = c_MTFSB1;
    c_63[  77] = c_MTFSB1D;
    c_63[  80] = c_FNEG;
    c_63[  81] = c_FNEGD;
    c_63[ 128] = c_MCRFS;
    c_63[ 140] = c_MTFSB0;
    c_63[ 141] = c_MTFSB0D;
    c_63[ 144] = c_FMR;
    c_63[ 145] = c_FMRD;
    c_63[ 272] = c_FNABS;
    c_63[ 273] = c_FNABSD;
    c_63[ 528] = c_FABS;
    c_63[ 529] = c_FABSD;
    c_63[1166] = c_MFFS;
    c_63[1167] = c_MFFSD;
    c_63[1422] = c_MTFSF;
    c_63[1423] = c_MTFSFD;

    // "4" extension
    c_4[   0] = c_PS_CMPU0;
    for(i = 0; i < 16; i++) c_4[(i << 7) | 12] = c_PSQ_LX;
    for(i = 0; i < 16; i++) c_4[(i << 7) | 14] = c_PSQ_STX;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 20] = c_PS_SUM0;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 22] = c_PS_SUM1;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 24] = c_PS_MULS0;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 26] = c_PS_MULS1;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 28] = c_PS_MADDS0;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 30] = c_PS_MADDS1;
    c_4[  36] = c_PS_DIV;
    c_4[  40] = c_PS_SUB;
    c_4[  42] = c_PS_ADD;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 46] = c_PS_SEL;
    c_4[  48] = c_PS_RES;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 50] = c_PS_MUL;
    c_4[  52] = c_PS_RSQRTE;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 56] = c_PS_MSUB;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 58] = c_PS_MADD;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 60] = c_PS_NMSUB;
    for(i = 0; i < 32; i++) c_4[(i << 6) | 62] = c_PS_NMADD;
    c_4[  64] = c_PS_CMPO0;
    for(i = 0; i < 16; i++) c_4[(i << 7) | 76] = c_PSQ_LUX;
    for(i = 0; i < 16; i++) c_4[(i << 7) | 78] = c_PSQ_STUX;
    c_4[  80] = c_PS_NEG;
    c_4[ 144] = c_PS_MR;
    c_4[ 128] = c_PS_CMPU1;
    c_4[ 192] = c_PS_CMPO1;
    c_4[1056] = c_PS_MERGE00;
    c_4[1120] = c_PS_MERGE01;
    c_4[1184] = c_PS_MERGE10;
    c_4[1248] = c_PS_MERGE11;
    c_4[2028] = c_DCBZ_L;
}
