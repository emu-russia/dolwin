#pragma pack(1)

// GX fifo scratch buffer (attached to PI fifo, when 0x0C008000)
#define GX_FIFO         0x0C008000

// CP registers. CPU accessing CP regs by 16-bit reads and writes
#define CP_SR           0x0C000000      // status register
#define CP_CR           0x0C000002      // control register
#define CP_CLR          0x0C000004      // clear register
#define CP_TEX          0x0C000006      // something used for TEX units setup
#define CP_BASE         0x0C000020      // GP fifo base
#define CP_TOP          0x0C000024      // GP fifo top
#define CP_HIWMARK      0x0C000028      // GP fifo high watermark
#define CP_LOWMARK      0x0C00002C      // GP fifo low watermark
#define CP_CNT          0x0C000030      // GP fifo read-write pointer distance (stride)
#define CP_WRPTR        0x0C000034      // GP fifo write pointer
#define CP_RDPTR        0x0C000038      // GP fifo read pointer
#define CP_BPPTR        0x0C00003C      // GP fifo breakpoint

// PE registers. CPU accessing PE regs by 16-bit reads and writes
#define PE_ZCR          0x0C001000      // z configuration
#define PE_ACR          0x0C001002      // alpha/blender configuration
#define PE_ALPHA_DST    0x0C001004      // destination alpha
#define PE_ALPHA_MODE   0x0C001006      // alpha mode
#define PE_ALPHA_READ   0x0C001008      // alpha read mode?
#define PE_SR           0x0C00100A      // status register
#define PE_TOKEN        0x0C00100E      // last token value

// PI fifo registers. CPU accessing them by 32-bit reads and writes
#define PI_BASE         0x0C00300C      // CPU fifo base
#define PI_TOP          0x0C003010      // CPU fifo top
#define PI_WRPTR        0x0C003014      // CPU fifo write pointer

// CP status register mask layout
#define CP_SR_OVF       (1 << 0)
#define CP_SR_UVF       (1 << 1)
#define CP_SR_RD_IDLE   (1 << 2)
#define CP_SR_CMD_IDLE  (1 << 3)
#define CP_SR_BPINT     (1 << 4)

// CP control register mask layout
#define CP_CR_RDEN      (1 << 0)
#define CP_CR_BPCLR     (1 << 1)
#define CP_CR_OVFEN     (1 << 2)
#define CP_CR_UVFEN     (1 << 3)
#define CP_CR_LINK      (1 << 4)
#define CP_CR_BPEN      (1 << 5)

// CP clear register mask layout
#define CP_CLR_OVFCLR   (1 << 0)
#define CP_CLR_UVFCLR   (1 << 1)

// PE status register
#define PE_SR_DONE      (1 << 0)
#define PE_SR_TOKEN     (1 << 1)
#define PE_SR_DONEMSK   (1 << 2)
#define PE_SR_TOKENMSK  (1 << 3)

// PI wrap bit
#define PI_WRPTR_WRAP   (1 << 27)

// CP registers
typedef struct CPRegs
{
    u16     sr;         // status
    u16     cr;         // control
    u32     base, top;
    u32     lomark, himark;
    u32     cnt;
    u32     wrptr, rdptr, bpptr;
} CPRegs;

// PE registers
typedef struct PERegs
{
    u16     sr;         // status register
    u16     token;      // last token
} PERegs;

// PI registers
typedef struct PIRegs
{
    u32     base;
    u32     top;
    u32     wrptr;      // also WRAP bit
} PIRegs;

// ---------------------------------------------------------------------------
// hardware API

// CP, PE and PI fifo state (registers and other data)
typedef struct FifoControl
{
    CPRegs      cp;     // command processor registers
    PERegs      pe;     // pixel engine registers
    PIRegs      pi;     // processor (CPU) fifo regs
    
    long    drawdone;   // events
    long    token;
    u32     done_num;   // number of drawdone (PE_FINISH) events
    s64     time;       // fifo update time

    BOOL    gxpoll;     // 1: poll controllers after GX draw done
} FifoControl;

extern  FifoControl fifo;

void    CPOpen();
void    CPUpdate();

#pragma pack()
