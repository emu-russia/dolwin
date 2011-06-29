// PPC definitions and structures for interpreter and recompiler cores

// GC bus clock is running on 1/3 of CPU clock, and timer is
// running on 1/4 of bus clock (1/12 of CPU clock)
#define CPU_CORE_CLOCK  486000000u  // 486 mhz (its not 485, stop bugging me!)
#define CPU_BUS_CLOCK   (CPU_CORE_CLOCK / 3)
#define CPU_TIMER_CLOCK (CPU_BUS_CLOCK / 4)
#define CPU_MAX_DELAY   12

// CPU core enumeration
enum CPU_CORE
{
    CPU_INTERPRETER = 1,
    CPU_RECOMPILER,
};

// ---------------------------------------------------------------------------
// opcode decoding ("op" representing current opcode, to simplify macros)

#define RD          ((op >> 21) & 0x1f)
#define RS          RD
#define RA          ((op >> 16) & 0x1f)
#define RB          ((op >> 11) & 0x1f)
#define RC          ((op >>  6) & 0x1f)
#define SIMM        ((s32)(s16)(u16)op)
#define UIMM        (op & 0xffff)
#define CRFD        ((op >> 23) & 7)
#define CRFS        ((op >> 18) & 7)
#define CRBD        ((op >> 21) & 0x1f)
#define CRBA        ((op >> 16) & 0x1f)
#define CRBB        ((op >> 11) & 0x1f)
#define BO(n)       ((bo >> (4-n)) & 1)
#define BI          RA
#define SH          RB
#define MB          ((op >> 6) & 0x1f)
#define ME          ((op >> 1) & 0x1f)
#define CRM         ((op >> 12) & 0xff)
#define FM          ((op >> 17) & 0xff)

// fast R*-field register addressing
#define RRD         GPR[RD]
#define RRS         GPR[RS]
#define RRA         GPR[RA]
#define RRB         GPR[RB]
#define RRC         GPR[RC]

// ---------------------------------------------------------------------------
// registers and their bitfields

// name registers
#define GPR     cpu.gpr
#define SPR     cpu.spr
#define SR      cpu.sr
#define FPRU(n) (cpu.fpr[n].uval)
#define FPRD(n) (cpu.fpr[n].dbl)
#define SP      (GPR[1])
#define SDA1    (GPR[13])
#define SDA2    (GPR[2])
#define XER     (SPR[1])
#define LR      (SPR[8])
#define CTR     (SPR[9])
#define DSISR   (SPR[18])
#define DAR     (SPR[19])
#define DEC     (SPR[22])
#define SDR1    (SPR[25])
#define SRR0    (SPR[26])
#define SRR1    (SPR[27])
#define SPRG0   (SPR[272])
#define SPRG1   (SPR[273])
#define SPRG2   (SPR[274])
#define SPRG3   (SPR[275])
#define EAR     (SPR[282])
#define PVR     (SPR[287])
#define IBAT0U  (SPR[528])
#define IBAT0L  (SPR[529])
#define IBAT1U  (SPR[530])
#define IBAT1L  (SPR[531])
#define IBAT2U  (SPR[532])
#define IBAT2L  (SPR[533])
#define IBAT3U  (SPR[534])
#define IBAT3L  (SPR[535])
#define DBAT0U  (SPR[536])
#define DBAT0L  (SPR[537])
#define DBAT1U  (SPR[538])
#define DBAT1L  (SPR[539])
#define DBAT2U  (SPR[540])
#define DBAT2L  (SPR[541])
#define DBAT3U  (SPR[542])
#define DBAT3L  (SPR[543])
#define HID0    (SPR[1008])
#define HID1    (SPR[1009])
#define IABR    (SPR[1010])
#define DABR    (SPR[1013])
#define CR      cpu.cr
#define MSR     cpu.msr
#define FPSCR   cpu.fpscr
#define TBR     cpu.tb.sval
#define UTBR    cpu.tb.uval
#define PC      cpu.pc

// BAT fields
#define BATBEPI(batu)   (batu >> 17)
#define BATBL(batu)     ((batu >> 2) & 0x7ff)
#define BATBRPN(batl)   (batl >> 17)

// floating point register
typedef union FPREG
{
    f64         dbl;
    u64         uval;
} FPREG;

// time-base
typedef union TBREG
{
    s64         sval;               // for comparsion
    u64         uval;               // for incrementing
    struct
    {
        u32     l;                  // for output
        u32     u;
    };
} TBREG;

#define BIT(n)              (1 << (31-n))

// Machine State Flags
#define MSR_RESERVED        0xFFFA0088
#define MSR_POW             (BIT(13))               // Power management enable
#define MSR_ILE             (BIT(15))               // Exception little-endian mode
#define MSR_EE              (BIT(16))               // External interrupt enable
#define MSR_PR              (BIT(17))               // User privilege level
#define MSR_FP              (BIT(18))               // Floating-point available
#define MSR_ME              (BIT(19))               // Machine check enable
#define MSR_FE0             (BIT(20))               // Floating-point exception mode 0
#define MSR_SE              (BIT(21))               // Single-step trace enable
#define MSR_BE              (BIT(22))               // Branch trace enable
#define MSR_FE1             (BIT(23))               // Floating-point exception mode 1
#define MSR_IP              (BIT(25))               // Exception prefix
#define MSR_IR              (BIT(26))               // Instruction address translation
#define MSR_DR              (BIT(27))               // Data address translation
#define MSR_PM              (BIT(29))               // Performance monitor mode
#define MSR_RI              (BIT(30))               // Recoverable exception
#define MSR_LE              (BIT(31))               // Little-endian mode enable

// ** Gekko specific **

#define PS0(n)      (cpu.fpr[n].dbl)
#define PS1(n)      (cpu.ps1[n].dbl)

// GQR0 reserved by OS (always 0)
// GQR1 reserved by CW compiler
// others are used by OS "fast-cast"
#define GQR         (&SPR[912])
#define GQR0        (GQR[0])
#define GQR1        (GQR[1])
#define GQR2        (GQR[2])            // u8
#define GQR3        (GQR[3])            // u16
#define GQR4        (GQR[4])            // s8
#define GQR5        (GQR[5])            // s16
#define GQR6        (GQR[6])
#define GQR7        (GQR[7])

#define HID2        (SPR[920])
#define WPAR        (SPR[921])          // write-gathering buffer
#define DMAU        (SPR[922])          // locked cache DMA
#define DMAL        (SPR[923])

#define HID2_LSQE   0x80000000          // PS load/store quantization
#define HID2_WPE    0x40000000          // gathering enabled
#define HID2_PSE    0x20000000          // PS-mode
#define HID2_LCE    0x10000000          // cache is locked

#define WPAR_ADDR   0xffffffe0          // accumulation address
#define WPAR_BNE    0x1                 // buffer not empty
#define WPE         (HID2 & HID2_WPE)

#define LSQE        (HID2 & HID2_LSQE)
#define PSE         (HID2 & HID2_PSE)
#define LD_SCALE(n) ((GQR[n] >> 24) & 0x3f)
#define LD_TYPE(n)  ((GQR[n] >> 16) & 7)
#define ST_SCALE(n) ((GQR[n] >>  8) & 0x3f)
#define ST_TYPE(n)  ((GQR[n]      ) & 7)
#define PSW         (op & 0x8000)
#define PSI         ((op >> 12) & 7)

// ---------------------------------------------------------------------------
// exception vectors (physical, because translation is disabled in exceptions)

#define CPU_EXCEPTION_RESET         0x0100
#define CPU_EXCEPTION_MACHINE       0x0200
#define CPU_EXCEPTION_DSI           0x0300
#define CPU_EXCEPTION_ISI           0x0400
#define CPU_EXCEPTION_INTERRUPT     0x0500
#define CPU_EXCEPTION_ALIGN         0x0600
#define CPU_EXCEPTION_PROGRAM       0x0700
#define CPU_EXCEPTION_FPUNAVAIL     0x0800
#define CPU_EXCEPTION_DECREMENTER   0x0900
#define CPU_EXCEPTION_SYSCALL       0x0C00
#define CPU_EXCEPTION_PERFMON       0x0D00
#define CPU_EXCEPTION_IABR          0x1300
#define CPU_EXCEPTION_THERMAL       0x1700

extern  void (*CPUException)(u32 vector);

/*/ ---------------------------------------------------------------------------

    Exception Notes:
    ----------------

    We support : DSI(?), ISI(?), Interrupt, Alignment(?), Program, FP Unavail,
    Decrementer, Syscall.
    (?) - only in advanced memory mode (MMU), or optionally.

    Common processing :

        SRR0 - PC (address where to resume execution, after 'rfi')
        SRR1 - copied MSR bits.

    MSR processing :

        1. SRR[0, 5-9, 16-31] are copied from MSR.
        2. disable translation - clear MSR[IR] & MSR[DR]
        3. select exception base (not used in emu) :
                0x000nnnnn if MSR[IP] = 0
                0xFFFnnnnn if MSR[IP] = 1
        4. MSR[RI] must be always 1 in emu!
        5. on Interrupt and Decrementer MSR[EE] is cleared also.

--------------------------------------------------------------------------- /*/

// ---------------------------------------------------------------------------
// CPU externals

// CPU memory operations. using MEM* or DB* read/write operations,
// (depending on DOLDEBUG definition in dolphin.h)
extern void (__fastcall *CPUReadByte)(u32 addr, u32 *reg);
extern void (__fastcall *CPUWriteByte)(u32 addr, u32 data);
extern void (__fastcall *CPUReadHalf)(u32 addr, u32 *reg);
extern void (__fastcall *CPUReadHalfS)(u32 addr, u32 *reg);
extern void (__fastcall *CPUWriteHalf)(u32 addr, u32 data);
extern void (__fastcall *CPUReadWord)(u32 addr, u32 *reg);
extern void (__fastcall *CPUWriteWord)(u32 addr, u32 data);
extern void (__fastcall *CPUReadDouble)(u32 addr, u64 *reg);
extern void (__fastcall *CPUWriteDouble)(u32 addr, u64 *data);

// controls
void    CPUInit();
void    CPUFini();
void    CPUOpen();                  // select core, before start
extern  void (*CPUStart)();         // start from PC
void    CPUTick();                  // modify counters

// there is no need in CPUStop

// CPU control/state block (all important data is here)
typedef struct CPUControl
{
    // CPU state (all registers)
    u32         gpr[32];            // general purpose regs
    FPREG       fpr[32], ps1[32];   // floating point regs (fpr=ps0 for paired singles)
    u32         spr[1024];          // special purpose regs
    u32         sr[16];             // segment regs
    u32         cr;                 // condition reg
    u32         msr;                // machine state reg
    u32         fpscr;              // FP status/control reg (rounding only for now)
    u32         pc;                 // program counter
    TBREG       tb;                 // time-base counter (timer)

    s32         bailout;            // update HW, if < 0
    s32         bailtime;           // initial "bailout" value
    s64         one_second;         // one second in timer ticks
    u32         cf;                 // counter factor
    u32         delay, delayVal;    // TBR/DEC update delay (number of instructions)
    BOOL        decreq;             // decrementer exception request

    u32         core;               // see CPU core enumeration
    BOOL        mmx;                // 1: mmx supported and enabled
    BOOL        sse;                // 1: sse supported and enabled
    BOOL        log;                // log EVERY executed opcode
    u32         ops;                // instruction counter (only for debug!)

    // for default interpreter
    BOOL        exception;          // exception pending
    BOOL        branch;             // non-linear PC change
    u32         rotmask[32][32];    // mask for integer rotate opcodes 
    BOOL        RESERVE;            // for lwarx/stwcx.
    u32         RESERVE_ADDR;       // for lwarx/stwcx.
    f32         ldScale[64];        // for paired-single loads
    f32         stScale[64];        // for paired-single stores

    // for default recompiler
    s32         oplvl;              // optimization level
    u8**        groups;             // group allocation table (24 mb)
    u8*         recbuf;             // temporary buffer
    u32         recptr;             // temporary buffer position
    u32         jumprel[4];         // for relative jumps patching
} CPUControl;

extern  CPUControl cpu;
