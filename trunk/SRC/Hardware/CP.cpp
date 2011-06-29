// all fifo-related stuff here :
// CP - command processor, PE - pixel engine,
// PI fifo - processor interface fifo.
#include "dolphin.h"

/*/
    Thats how I understand GC graphics fifo logic.

    There are two fifos : CPU (PI) and GP (CP). Fifos can run in two modes:
    immediate and multibuffer. CPU fifo is used together with GP fifo in
    multibuffer mode to imitate N64 double-buffered display lists, for easy 
    N64's games porting.

    CP registers are used to control GP fifo flow and PI registers are used
    for CPU fifo.

    Details of GP Fifo
    ------------------

    Important detail : GP command proccessor (which is inside ArtX GP logic, 
    not 'CP' interface) always reading data by 32-byte chunks. Im pretty sure,
    that it doesnt matter, when sent 32-byte "packet" is teared in middle
    of it, as example 'float' vertex X-position data. GP will stalls all
    commands execution, until it get full set of current primitive or its
    parameters. I dont think, that it has cache for big primitives, like 
    triangle-strips, its just stalls, until dont get whole current vertex 
    data to render. But Im sure, emulators can collect whole single primitive
    in accumulation buffer to draw it all at once - it will be much faster :)

    The 'read idle' and 'command idle' bits are showing GP state. When GP
    have no new data to read - 'read idle' is set. And when GP command processor
    is not busy by drawing primitive, or executing command, 'command idle' is
    set. When both bits are cleared, then GP is free and ready for action.
    Note : 'read idle' can be set in breakpoint condition, when GP stalls
    until breakpoint is not cleared.

    If 'GP read' bit enabled in CP control register, GP logic starts to read
    fifo data by 32-byte chunks, using CP read pointer. When CP read pointer
    wraps 'top' address, it automatically changes back to 'base'. No interrupts
    generated

    If 'GP link' bit enabled in CP CR, following features are become enabled :
    breakpoint, low watermark and high watermark. These features are used only
    in immediate ('linked') mode.

    Conditions for immediate fifo mode :

    Breakpoint : if CP read pointer reaches breakpoint address, CP generates
    breakpoint interrupt. GP stops reading fifo data, until breakpoint is not
    cleared. Actually this condition can be set even in multibuffer mode, but 
    there is no reason to use it.

    Low watermark : if CP read pointer reaches low watermark address, CP
    generates low watermark interrupt.

    High watermark : if CP write pointer reaches high watermark address, CP
    generates high watermark interrupt.

    Watermarks are used to sync GP and CPU. When high watermark is asserted,
    CPU is stopping to write any commands/data to fifo, until GP reading it.
    And when GP reaches low watermark, CPU will continue to write data. It is
    also used to protect fifo data to be overwritten by CPU. On real hardware
    GP reading will never overdrive CPU writing, so they work finely together
    in single circular fifo buffer.

    When CP write pointer wraps its 'top' address, it automatically changes 
    back to 'base'. No interrupts generated. Actually CP write pointer is dummy
    in fifo processing, because GP can only read data, and its used only to
    calculate CP fifo "stride" = wrptr - rdptr. I believe, CP write pointer
    is always the same as PI write pointer.

    All pointers should be 32-byte aligned, when set.

    Details of CPU Fifo
    -------------------

    CPU Fifo is used only to write data. The application of CPU fifo is to
    create display lists, while GP is executing others from CP fifo (in
    multibuffer mode).

    When CPU fifo wraps the top fifo buffer, special 'wrap' bit is setting
    in PI write pointer register. When CPU writes any new fifo data after
    wrap, this bit is cleared. No interrupts generated.

    All pointers should be 32-byte aligned, when set.

    Application of write gather buffer
    ----------------------------------

    Write gather buffer is used to collect and then quickly blast 32-byte
    memory chunks, during any CPU writes. Thats why all fifo's are aligned
    to 32-byte boundary. See YAGCD or any other Gekko manuals for more
    information on it.

    There are two applications for gather buffer. In first case it is used
    together with PI fifo, otherwise it is assigned to be "redirected" and
    used for other purposes. When gather buffer is not redirected, its
    pointed to 0xCC008000. Since CPU cannot read PI write pointer register
    directly, this address is implemented to be equal as PI write pointer.
    You can think, that if you writing any data to 0xCC008000 you're writing
    to PI fifo.

    Fifo emulation
    --------------

/*/

FifoControl fifo;

// ---------------------------------------------------------------------------
// fifo

static void DONE_INT()
{
    fifo.done_num++; vi.frames++;
    if(fifo.done_num == 1)
    {
        SetStatusText(STATUS_PROGRESS, "First GX access", 1);
        vi.xfb = 0;     // disable VI output
    }
    DBReport(GREEN "PE_DONE (frame:%u)", fifo.done_num);

    if(fifo.gxpoll) SIPoll();

    fifo.pe.sr |= PE_SR_DONE;
    if(fifo.pe.sr & PE_SR_DONEMSK)
    {
        PIAssertInt(PI_INTERRUPT_PE_FINISH);
    }
}

static void TOKEN_INT()
{
    vi.frames++;
    DBReport(GREEN "PE_TOKEN (%04X)", fifo.pe.token);
    
    if(fifo.gxpoll) SIPoll();

    fifo.pe.sr |= PE_SR_TOKEN;
    if(fifo.pe.sr & PE_SR_TOKENMSK)
    {
        PIAssertInt(PI_INTERRUPT_PE_TOKEN);
    }
}

static void CP_BREAK()
{
    fifo.cp.sr |= CP_SR_BPINT;
    DolwinReport("BPOINT!");
    PIAssertInt(PI_INTERRUPT_CP);
}

// count PI write pointer, CP pointer, check for breakpoint
static void fifo_flow(u32 nbytes)
{
    // PI fifo flow
    fifo.pi.wrptr &= ~PI_WRPTR_WRAP;
    fifo.pi.wrptr += nbytes;
    if(fifo.pi.wrptr >= fifo.pi.top)
    {
        fifo.pi.wrptr  = fifo.pi.base;
        fifo.pi.wrptr |= PI_WRPTR_WRAP;
    }

    // CP fifo flow (dummy)
    fifo.cp.wrptr = fifo.pi.wrptr;
    fifo.cp.cnt ^= 31;

    // breakpoint check
    if(fifo.cp.cr & CP_CR_BPEN)
    {
        if((fifo.cp.wrptr & ~31) == (fifo.cp.bpptr & ~31))
        {
            CP_BREAK();
        }
    }

    // draw events

    // "draw done"
    if(fifo.drawdone)
    {
        fifo.drawdone = 0;
        DONE_INT();
    }

    // "draw sync"
    if(fifo.token)
    {
        fifo.token = 0;
        TOKEN_INT();
    }
}

// ---------------------------------------------------------------------------
// registers

//
// pixel engine status register (0x100a)
//

static void __fastcall write_pe_sr(u32 addr, u32 data)
{
    // clear interrupts
    if(fifo.pe.sr & PE_SR_DONE)
    {
        fifo.pe.sr &= ~PE_SR_DONE;
        PIClearInt(PI_INTERRUPT_PE_FINISH);
    }
    if(fifo.pe.sr & PE_SR_TOKEN)
    {
        fifo.pe.sr &= ~PE_SR_TOKEN;
        PIClearInt(PI_INTERRUPT_PE_TOKEN);
    }

    // set mask bits
    if(data & PE_SR_DONEMSK) fifo.pe.sr |= PE_SR_DONEMSK;
    else fifo.pe.sr &= ~PE_SR_DONEMSK;
    if(data & PE_SR_TOKENMSK) fifo.pe.sr |= PE_SR_TOKENMSK;
    else fifo.pe.sr &= ~PE_SR_TOKENMSK;
}
static void __fastcall read_pe_sr(u32 addr, u32 *reg)  { *reg = fifo.pe.sr; }

static void __fastcall read_pe_token(u32 addr, u32 *reg) { *reg = fifo.pe.token; }

//
// command processor
//

// control and status registers

static void __fastcall read_cp_sr(u32 addr, u32 *reg)
{
    // GP is always ready for action
    fifo.cp.sr |= (CP_SR_RD_IDLE | CP_SR_CMD_IDLE);

    *reg = fifo.cp.sr;
}

static void __fastcall write_cp_cr(u32 addr, u32 data)
{
    fifo.cp.cr = (u16)data;

    // clear breakpoint
    if(data & CP_CR_BPCLR)
    {
        fifo.cp.sr &= ~CP_SR_BPINT;
        PIClearInt(PI_INTERRUPT_CP);
    }
}
static void __fastcall read_cp_cr(u32 addr, u32 *reg) { *reg = fifo.cp.cr; }

static void __fastcall write_cp_clr(u32 addr, u32 data)
{
    // clear watermark conditions
    if(data & CP_CLR_OVFCLR)
    {
        fifo.cp.sr &= ~CP_SR_OVF;
        PIClearInt(PI_INTERRUPT_CP);
    }
    if(data & CP_CLR_UVFCLR)
    {
        fifo.cp.sr &= ~CP_SR_UVF;
        PIClearInt(PI_INTERRUPT_CP);
    }
}

// pointers

// show GP fifo configuration
static void printCP()
{
    // fifo modes
    char*md = (fifo.cp.cr & CP_CR_LINK) ? ((char *)"immediate ") : ((char *)"multi-");
    char bp = (fifo.cp.cr & CP_CR_BPEN) ? ('B') : ('-');    // breakpoint
    char lw = (fifo.cp.cr & CP_CR_UVFEN)? ('L') : ('-');    // low-wmark
    char hw = (fifo.cp.cr & CP_CR_OVFEN)? ('H') : ('-');    // high-wmark

    DBReport(CP GREEN "%sfifo configuration:%c%c%c", md, bp, lw, hw);
    DBReport(YEL"   base :%08X", 0x80000000 | fifo.cp.base);
    DBReport(YEL"   top  :%08X", 0x80000000 | fifo.cp.top);
    DBReport(YEL"   low  :%08X", 0x80000000 | fifo.cp.lomark);
    DBReport(YEL"   high :%08X", 0x80000000 | fifo.cp.himark);
    DBReport(YEL"   cnt  :%08X", fifo.cp.cnt);
    DBReport(YEL"   wrptr:%08X", 0x80000000 | fifo.cp.wrptr);
    DBReport(YEL"   rdptr:%08X", 0x80000000 | fifo.cp.rdptr);
}

static void __fastcall read_cp_baseh(u32 addr, u32 *reg)    { *reg = fifo.cp.base >> 16; }
static void __fastcall write_cp_baseh(u32 addr, u32 data)   { fifo.cp.base = data << 16; }
static void __fastcall read_cp_basel(u32 addr, u32 *reg)    { *reg = fifo.cp.base & 0xffff; }
static void __fastcall write_cp_basel(u32 addr, u32 data)   { fifo.cp.base = data & 0xffff; }
static void __fastcall read_cp_toph(u32 addr, u32 *reg)     { *reg = fifo.cp.top >> 16; }
static void __fastcall write_cp_toph(u32 addr, u32 data)    { fifo.cp.top = data << 16; }
static void __fastcall read_cp_topl(u32 addr, u32 *reg)     { *reg = fifo.cp.top & 0xffff; }
static void __fastcall write_cp_topl(u32 addr, u32 data)    { fifo.cp.top = data & 0xffff; }

static void __fastcall read_cp_hmarkh(u32 addr, u32 *reg)   { *reg = fifo.cp.himark >> 16; }
static void __fastcall write_cp_hmarkh(u32 addr, u32 data)  { fifo.cp.himark = data << 16; }
static void __fastcall read_cp_hmarkl(u32 addr, u32 *reg)   { *reg = fifo.cp.himark & 0xffff; }
static void __fastcall write_cp_hmarkl(u32 addr, u32 data)  { fifo.cp.himark = data & 0xffff; }
static void __fastcall read_cp_lmarkh(u32 addr, u32 *reg)   { *reg = fifo.cp.lomark >> 16; }
static void __fastcall write_cp_lmarkh(u32 addr, u32 data)  { fifo.cp.lomark = data << 16; }
static void __fastcall read_cp_lmarkl(u32 addr, u32 *reg)   { *reg = fifo.cp.lomark & 0xffff; }
static void __fastcall write_cp_lmarkl(u32 addr, u32 data)  { fifo.cp.lomark = data & 0xffff; }

static void __fastcall read_cp_cnth(u32 addr, u32 *reg)     { *reg = fifo.cp.cnt >> 16 ; }
static void __fastcall write_cp_cnth(u32 addr, u32 data)    { fifo.cp.cnt = data << 16; }
static void __fastcall read_cp_cntl(u32 addr, u32 *reg)     { *reg = fifo.cp.cnt & 0xffff; }
static void __fastcall write_cp_cntl(u32 addr, u32 data)    { fifo.cp.cnt = data & 0xffff; }

static void __fastcall read_cp_wrptrh(u32 addr, u32 *reg)   { *reg = fifo.cp.wrptr >> 16; }
static void __fastcall write_cp_wrptrh(u32 addr, u32 data)  { fifo.cp.wrptr = data << 16; }
static void __fastcall read_cp_wrptrl(u32 addr, u32 *reg)   { *reg = fifo.cp.wrptr & 0xffff; }
static void __fastcall write_cp_wrptrl(u32 addr, u32 data)  { fifo.cp.wrptr = data & 0xffff; }
static void __fastcall read_cp_rdptrh(u32 addr, u32 *reg)   { *reg = fifo.cp.rdptr >> 16; }
static void __fastcall write_cp_rdptrh(u32 addr, u32 data)  { fifo.cp.rdptr = data << 16; }
static void __fastcall read_cp_rdptrl(u32 addr, u32 *reg)   { *reg = fifo.cp.rdptr & 0xffff; }
static void __fastcall write_cp_rdptrl(u32 addr, u32 data)  { fifo.cp.rdptr = data & 0xffff; }

static void __fastcall read_cp_bpptrh(u32 addr, u32 *reg)   { *reg = fifo.cp.bpptr >> 16; }
static void __fastcall write_cp_bpptrh(u32 addr, u32 data)  { fifo.cp.bpptr = data << 16; }
static void __fastcall read_cp_bpptrl(u32 addr, u32 *reg)   { *reg = fifo.cp.bpptr & 0xffff; }
static void __fastcall write_cp_bpptrl(u32 addr, u32 data)  { fifo.cp.bpptr = data & 0xffff; }

//
// PI fifo (CPU)
//

static void __fastcall read_pi_base(u32 addr, u32 *reg)   { *reg = fifo.pi.base; }
static void __fastcall write_pi_base(u32 addr, u32 data)  { fifo.pi.base = data; }
static void __fastcall read_pi_top(u32 addr, u32 *reg)    { *reg = fifo.pi.top; }
static void __fastcall write_pi_top(u32 addr, u32 data)   { fifo.pi.top = data; }
static void __fastcall read_pi_wrptr(u32 addr, u32 *reg)  { *reg = fifo.pi.wrptr; }
static void __fastcall write_pi_wrptr(u32 addr, u32 data) { fifo.pi.wrptr = data; }

// show PI fifo configuration
static void printPI()
{
    DBReport(PI GREEN "fifo configuration");
    DBReport(YEL"   base :%08X", 0x80000000 | fifo.pi.base);
    DBReport(YEL"   top  :%08X", 0x80000000 | fifo.pi.top);
    DBReport(YEL"   wrptr:%08X", 0x80000000 | fifo.pi.wrptr);
    DBReport(YEL"   wrap :%i", (fifo.pi.wrptr & PI_WRPTR_WRAP) ? (1) : (0));
}

//
// PI fifo buffer (scratch space, for non-redirected fifo)
//

void __fastcall write_fifo8(u32 addr, u32 data)
{
    u8 b = (u8)data;

    BeginProfileGfx();
    GXWriteFifo(&b, 1);
    fifo_flow(1);
    EndProfileGfx();
}

void __fastcall write_fifo16(u32 addr, u32 data)
{
    u16 h = MEMSwapHalf((u16)data);

    BeginProfileGfx();
    GXWriteFifo((u8 *)&h, 2);
    fifo_flow(2);
    EndProfileGfx();
}

void __fastcall write_fifo32(u32 addr, u32 data)
{
    u32 w = MEMSwap(data);

    BeginProfileGfx();
    GXWriteFifo((u8 *)&w, 4);
    fifo_flow(4);
    EndProfileGfx();
}

//
// stubs
//

static void __fastcall no_write(u32 addr, u32 data) {}
static void __fastcall no_read(u32 addr, u32 *reg)  { *reg = 0; }

// ---------------------------------------------------------------------------
// init

void CPOpen()
{
    DBReport(CYAN "FIFO: Command processor (for GX)\n");

    // clear registers
    memset(&fifo, 0, sizeof(FifoControl));

    fifo.time = TBR + 100;

    fifo.gxpoll = GetConfigInt(USER_GX_POLL, USER_GX_POLL_DEFAULT);

    // command processor
    HWSetTrap(16, CP_SR         , read_cp_sr, NULL);
    HWSetTrap(16, CP_CR         , read_cp_cr, write_cp_cr);
    HWSetTrap(16, CP_CLR        , NULL, write_cp_clr);
    HWSetTrap(16, CP_TEX        , no_read, no_write);
    HWSetTrap(16, CP_BASE       , read_cp_baseh, write_cp_baseh);
    HWSetTrap(16, CP_BASE + 2   , read_cp_basel, write_cp_basel);
    HWSetTrap(16, CP_TOP        , read_cp_toph, write_cp_toph);
    HWSetTrap(16, CP_TOP + 2    , read_cp_topl, write_cp_topl);
    HWSetTrap(16, CP_HIWMARK    , read_cp_hmarkh, write_cp_hmarkh);
    HWSetTrap(16, CP_HIWMARK + 2, read_cp_hmarkl, write_cp_hmarkl);
    HWSetTrap(16, CP_LOWMARK    , read_cp_lmarkh, write_cp_lmarkh);
    HWSetTrap(16, CP_LOWMARK + 2, read_cp_lmarkl, write_cp_lmarkl);
    HWSetTrap(16, CP_CNT        , read_cp_cnth, write_cp_cnth);
    HWSetTrap(16, CP_CNT + 2    , read_cp_cntl, write_cp_cntl);
    HWSetTrap(16, CP_WRPTR      , read_cp_wrptrh, write_cp_wrptrh);
    HWSetTrap(16, CP_WRPTR + 2  , read_cp_wrptrl, write_cp_wrptrl);
    HWSetTrap(16, CP_RDPTR      , read_cp_rdptrh, write_cp_rdptrh);
    HWSetTrap(16, CP_RDPTR + 2  , read_cp_rdptrl, write_cp_rdptrl);
    HWSetTrap(16, CP_BPPTR      , read_cp_bpptrh, write_cp_bpptrh);
    HWSetTrap(16, CP_BPPTR + 2  , read_cp_bpptrl, write_cp_bpptrl);

    // pixel engine
    HWSetTrap(16, PE_ZCR       , no_read, no_write);
    HWSetTrap(16, PE_ACR       , no_read, no_write);
    HWSetTrap(16, PE_ALPHA_DST , no_read, no_write);
    HWSetTrap(16, PE_ALPHA_MODE, no_read, no_write);
    HWSetTrap(16, PE_ALPHA_READ, no_read, no_write);
    HWSetTrap(16, PE_SR        , read_pe_sr, write_pe_sr);
    HWSetTrap(16, PE_TOKEN     , read_pe_token, NULL);

    // processor interface (CPU fifo)
    HWSetTrap(32, PI_BASE , read_pi_base , write_pi_base);
    HWSetTrap(32, PI_TOP  , read_pi_top  , write_pi_top);
    HWSetTrap(32, PI_WRPTR, read_pi_wrptr, write_pi_wrptr);

    // scratch PI fifo buffer
    HWSetTrap(8 , GX_FIFO  , NULL, write_fifo8);
    HWSetTrap(16, GX_FIFO  , NULL, write_fifo16);
    HWSetTrap(32, GX_FIFO  , NULL, write_fifo32);
    HWSetTrap(8 , GX_FIFO+4, NULL, write_fifo8);
    HWSetTrap(16, GX_FIFO+4, NULL, write_fifo16);
    HWSetTrap(32, GX_FIFO+4, NULL, write_fifo32);
}

void CPUpdate()
{
/*/
    if(TBR < fifo.time) return;
    fifo.time = TBR + 100;

    // "draw done"
    if(fifo.drawdone)
    {
        fifo.drawdone = 0;
        DONE_INT();
    }

    // "draw sync"
    if(fifo.token)
    {
        fifo.token = 0;
        TOKEN_INT();
    }
/*/
}
