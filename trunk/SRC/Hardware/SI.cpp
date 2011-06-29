// SI - serial interface (only GC "Spec5" controllers atm, by PAD plugin calls).
#include "dolphin.h"

// SI state (registers and other data)
SIControl si;

// IMPORTANT : transfer will never be aborted by communication error, 
// so all ERROR bits/status in SI regs are not used in emulator.

// polling intervals are also not critical. all controllers are polled
// before VI blank (in VI.cpp)

// SI_EXILK is not used (same as EXI clock timing, because of instant EXI transfers)

// Dolwin polling schematics :
/*/
     -----
    | OUT |
    |-----| <
    | INH |  |     ----------       --------------------
    |-----|  |----| PADState |<----| PC device (plugin) |
    | INL |  |     ----------       --------------------
     -----  <
/*/

// Note : digital L and R are only set when its analog key is pressed all the way down;
// Dolwin plugin is only supporting the fact, that L/R are pressed.

// ---------------------------------------------------------------------------
// dispatch command

//
// command data are sending via communication buffer,
// we are parsing command opcode and forming response 
// data packet, written back to the communication buffer
//

static void SICommand(s32 chan, u8 *ptr)
{
    u8  cmd = ptr[0];

    switch(cmd)
    {
        // get device type and status
        case 0x00:
        {
            // 0 : use sub-type
            // 2 : n64 mouse
            // 5 : n64 controller
            // 9 : default gc controller
            ptr[0] = 9;
            ptr[1] = 0;     // sub-type
            ptr[2] = 0;     // always 0 ?
            break;
        }

        // HACK : use it for first time
        case 0x40:
        case 0x42:
            return;

        // unknown, freeloader uses it, when booting
        // case 0x40:

        // read origins
        case 0x41:
            ptr[0] = 0x41;
            ptr[1] = 0;
            ptr[2] = ptr[3] = ptr[4] = ptr[5] = 0x80;
            ptr[6] = ptr[7] = 0x1f;
            break;

        // calibrate
        //case 0x42:

        default:
        {
            DolwinReport(
                "unknown SI command\n" "chan:%i\n" "cmd:%02X\n" "out:%i in:%i\n" "pc:%08X",
                chan, cmd, SI_COMCSR_OUTLEN(SI_COMCSR_REG), SI_COMCSR_INLEN(SI_COMCSR_REG), PC
            );
        }
    }
}

// ---------------------------------------------------------------------------
// register traps

//
// command output buffers are read/write
//

/* ******* CHAN 0 ******* */

static void __fastcall si_wr_out0(u32 addr, u32 data)       // write
{
    si.shdw[0] = data;
    SI_SR_REG |= SI_SR_WRST0;

    // control motor
    if(PADSetRumble)
    {
        if(data == 0x00400000) PADSetRumble(0, PAD_MOTOR_STOP);
        else if(data == 0x00400001) PADSetRumble(0, PAD_MOTOR_RUMBLE);
        else if(data == 0x00400002) PADSetRumble(0, PAD_MOTOR_STOP_HARD);
    }
}

static void __fastcall si_rd_out0(u32 addr, u32 *reg)       // read
{
    *reg = si.out[0];
}

/* ******* CHAN 1 ******* */

static void __fastcall si_wr_out1(u32 addr, u32 data)       // write
{
    si.shdw[1] = data;
    SI_SR_REG |= SI_SR_WRST1;

    // control motor
    if(PADSetRumble)
    {
        if(data == 0x00400000) PADSetRumble(1, PAD_MOTOR_STOP);
        else if(data == 0x00400001) PADSetRumble(1, PAD_MOTOR_RUMBLE);
        else if(data == 0x00400002) PADSetRumble(1, PAD_MOTOR_STOP_HARD);
    }
}

static void __fastcall si_rd_out1(u32 addr, u32 *reg)       // read
{
    *reg = si.out[1];
}

/* ******* CHAN 2 ******* */

static void __fastcall si_wr_out2(u32 addr, u32 data)       // write
{
    si.shdw[2] = data;
    SI_SR_REG |= SI_SR_WRST2;

    // control motor
    if(PADSetRumble)
    {
        if(data == 0x00400000) PADSetRumble(2, PAD_MOTOR_STOP);
        else if(data == 0x00400001) PADSetRumble(2, PAD_MOTOR_RUMBLE);
        else if(data == 0x00400002) PADSetRumble(2, PAD_MOTOR_STOP_HARD);
    }
}

static void __fastcall si_rd_out2(u32 addr, u32 *reg)       // read
{
    *reg = si.out[2];
}

/* ******* CHAN 3 ******* */

static void __fastcall si_wr_out3(u32 addr, u32 data)       // write
{
    si.shdw[3] = data;
    SI_SR_REG |= SI_SR_WRST3;

    // control motor
    if(PADSetRumble)
    {
        if(data == 0x00400000) PADSetRumble(3, PAD_MOTOR_STOP);
        else if(data == 0x00400001) PADSetRumble(3, PAD_MOTOR_RUMBLE);
        else if(data == 0x00400002) PADSetRumble(3, PAD_MOTOR_STOP_HARD);
    }
}

static void __fastcall si_rd_out3(u32 addr, u32 *reg)       // read
{
    *reg = si.out[3];
}

//
// input buffers are read only
// 0.09 has swapped high-low register traps :) fixed.
//

/* ******* CHAN 0 ******* */

static void __fastcall si_inh0(u32 addr, u32 *reg)          // high
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[0].stickY;
    res |= (u8)si.pad[0].stickX << 8;
    res |= si.pad[0].button << 16;

    // clear RDST mask and interrupt
    SI_SR_REG &= ~SI_SR_RDST0;
    if((SI_SR_REG &
       (SI_SR_RDST0 |
        SI_SR_RDST1 |
        SI_SR_RDST2 |
        SI_SR_RDST3)) == 0)
    {
        SI_COMCSR_REG &= ~SI_COMCSR_RDSTINT;
        PIClearInt(PI_INTERRUPT_SI);
    }

    *reg = res;
}

static void __fastcall si_inl0(u32 addr, u32 *reg)          // low
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[0].triggerRight;
    res |= (u8)si.pad[0].triggerLeft << 8;
    res |= (u8)si.pad[0].substickY << 16;
    res |= (u8)si.pad[0].substickX << 24;

    *reg = res;
}

/* ******* CHAN 1 ******* */

static void __fastcall si_inh1(u32 addr, u32 *reg)          // high
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[1].stickY;
    res |= (u8)si.pad[1].stickX << 8;
    res |= si.pad[1].button << 16;

    // clear RDST mask and interrupt
    SI_SR_REG &= ~SI_SR_RDST1;
    if((SI_SR_REG &
       (SI_SR_RDST0 |
        SI_SR_RDST1 |
        SI_SR_RDST2 |
        SI_SR_RDST3)) == 0)
    {
        SI_COMCSR_REG &= ~SI_COMCSR_RDSTINT;
        PIClearInt(PI_INTERRUPT_SI);
    }

    *reg = res;
}

static void __fastcall si_inl1(u32 addr, u32 *reg)          // low
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[1].triggerRight;
    res |= (u8)si.pad[1].triggerLeft << 8;
    res |= (u8)si.pad[1].substickY << 16;
    res |= (u8)si.pad[1].substickX << 24;

    *reg = res;
}

/* ******* CHAN 2 ******* */

static void __fastcall si_inh2(u32 addr, u32 *reg)          // high
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[2].stickY;
    res |= (u8)si.pad[2].stickX << 8;
    res |= si.pad[2].button << 16;

    // clear RDST mask and interrupt
    SI_SR_REG &= ~SI_SR_RDST2;
    if((SI_SR_REG &
       (SI_SR_RDST0 |
        SI_SR_RDST1 |
        SI_SR_RDST2 |
        SI_SR_RDST3)) == 0)
    {
        SI_COMCSR_REG &= ~SI_COMCSR_RDSTINT;
        PIClearInt(PI_INTERRUPT_SI);
    }

    *reg = res;
}

static void __fastcall si_inl2(u32 addr, u32 *reg)          // low
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[2].triggerRight;
    res |= (u8)si.pad[2].triggerLeft << 8;
    res |= (u8)si.pad[2].substickY << 16;
    res |= (u8)si.pad[2].substickX << 24;

    *reg = res;
}

/* ******* CHAN 3 ******* */

static void __fastcall si_inh3(u32 addr, u32 *reg)          // high
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[3].stickY;
    res |= (u8)si.pad[3].stickX << 8;
    res |= si.pad[3].button << 16;

    // clear RDST mask and interrupt
    SI_SR_REG &= ~SI_SR_RDST3;
    if((SI_SR_REG &
       (SI_SR_RDST0 |
        SI_SR_RDST1 |
        SI_SR_RDST2 |
        SI_SR_RDST3)) == 0)
    {
        SI_COMCSR_REG &= ~SI_COMCSR_RDSTINT;
        PIClearInt(PI_INTERRUPT_SI);
    }

    *reg = res;
}

static void __fastcall si_inl3(u32 addr, u32 *reg)          // low
{
    u32 res;

    // return swapped joypad values
    res  = (u8)si.pad[3].triggerRight;
    res |= (u8)si.pad[3].triggerLeft << 8;
    res |= (u8)si.pad[3].substickY << 16;
    res |= (u8)si.pad[3].substickX << 24;

    *reg = res;
}

//
// communication buffer access
//

static void __fastcall write_sicom(u32 addr, u32 data)
{
    unsigned ofs = addr & 0x7f;
    u8      *out = (u8 *)&data;

    si.combuf[ofs+0] = out[3];
    si.combuf[ofs+1] = out[2];
    si.combuf[ofs+2] = out[1];
    si.combuf[ofs+3] = out[0];
}

static void __fastcall read_sicom(u32 addr, u32 *reg)
{
    unsigned ofs = addr & 0x7f;
    u8      *in  = (u8 *)reg;

    in[0] = si.combuf[ofs+3];
    in[1] = si.combuf[ofs+2];
    in[2] = si.combuf[ofs+1];
    in[3] = si.combuf[ofs+0];
}

// ---------------------------------------------------------------------------
// si control registers

//
// polling register
//

static void __fastcall write_poll(u32 addr, u32 data) { SI_POLL_REG = data; }
static void __fastcall read_poll(u32 addr, u32 *reg)  { *reg = SI_POLL_REG; }

//
// communication control/status 
//

static void __fastcall write_commcsr(u32 addr, u32 data)
{
    // clear incoming interrupt
    if(data & SI_COMCSR_TCINT)
    {
        SI_COMCSR_REG &= ~SI_COMCSR_TCINT;
        PIClearInt(PI_INTERRUPT_SI);
    }

    // change RDST interrupt mask
    if(data & SI_COMCSR_RDSTINTMSK) SI_COMCSR_REG |= SI_COMCSR_RDSTINTMSK;
    else SI_COMCSR_REG &= ~SI_COMCSR_RDSTINTMSK;

    // commands are executed immediately
    if(data & SI_COMCSR_TSTART)
    {
        // select channel
        s32 chan = SI_COMCSR_CHAN(data);
        SI_COMCSR_REG |= (chan << 1);

        // setup in/out length
        s32 inlen = SI_COMCSR_INLEN(data);
        SI_COMCSR_REG |= (inlen << 8);
        if(inlen == 0) inlen = 128;
        s32 outlen = SI_COMCSR_OUTLEN(data);
        SI_COMCSR_REG |= (outlen << 16);
        if(outlen == 0) outlen = 128;

        // make actual transfer
        SICommand(chan, si.combuf);

        // complete transfer
        SI_COMCSR_REG &= ~SI_COMCSR_TSTART;

        // set completion interrupt
        SI_COMCSR_REG |= SI_COMCSR_TCINT;

        // generate cpu interrupt (if mask allows that)
        if(data & SI_COMCSR_TCINTMSK)
        {
            SI_COMCSR_REG |= SI_COMCSR_TCINTMSK;
            PIAssertInt(PI_INTERRUPT_SI);
        }
        else SI_COMCSR_REG &= ~SI_COMCSR_TCINTMSK;
    }
}

static void __fastcall read_commcsr(u32 addr, u32 *reg)
{
    *reg = SI_COMCSR_REG;
}

//
// status register
//

static void __fastcall write_sisr(u32 addr, u32 data)
{
    // copy shadow command registers
    if(data & SI_SR_WR)
    {
        si.out[0] = si.shdw[0];
        SI_SR_REG &= ~SI_SR_WRST0;
        si.out[1] = si.shdw[1];
        SI_SR_REG &= ~SI_SR_WRST1;
        si.out[2] = si.shdw[2];
        SI_SR_REG &= ~SI_SR_WRST2;
        si.out[3] = si.shdw[3];
        SI_SR_REG &= ~SI_SR_WRST3;
    }
}

static void __fastcall read_sisr(u32 addr, u32 *reg)
{
    *reg = SI_SR_REG;
}

// 
// EXI clock lock reg (dummy)
//

static void __fastcall write_exilk(u32 addr, u32 data) { si.exilk = data; }
static void __fastcall read_exilk(u32 addr, u32 *reg)  { *reg = si.exilk; }

//
// stubs for fake mode
//

static void __fastcall no_write(u32 addr, u32 data) {}
static void __fastcall no_read(u32 addr, u32 *reg)
{
    if(addr == SI_POLL) *reg = 0xffffffff;
    else *reg = 0;
}

// ---------------------------------------------------------------------------
// polling

void SIPoll()
{
    if(si.fake) return;
    BeginProfilePAD();

    if(SI_POLL_REG & SI_POLL_EN0)
    {
        // update pad input buffer
        BOOL connected = FALSE;
        if(PADReadButtons) connected = PADReadButtons(0, &si.pad[0]);

        // set RDST flag
        if(connected)
        {
            SI_SR_REG |= SI_SR_RDST0;
            SI_COMCSR_REG |= SI_COMCSR_RDSTINT;
        }
    }

    if(SI_POLL_REG & SI_POLL_EN1)
    {
        // update pad input buffer
        BOOL connected = FALSE;
        if(PADReadButtons) connected = PADReadButtons(1, &si.pad[1]);

        // set RDST flag
        if(connected)
        {
            SI_SR_REG |= SI_SR_RDST1;
            SI_COMCSR_REG |= SI_COMCSR_RDSTINT;
        }
    }

    if(SI_POLL_REG & SI_POLL_EN2)
    {
        // update pad input buffer
        BOOL connected = FALSE;
        if(PADReadButtons) connected = PADReadButtons(2, &si.pad[2]);

        // set RDST flag
        if(connected)
        {
            SI_SR_REG |= SI_SR_RDST2;
            SI_COMCSR_REG |= SI_COMCSR_RDSTINT;
        }
    }

    if(SI_POLL_REG & SI_POLL_EN3)
    {
        // update pad input buffer
        BOOL connected = FALSE;
        if(PADReadButtons) connected = PADReadButtons(3, &si.pad[3]);

        // set RDST flag
        if(connected)
        {
            SI_SR_REG |= SI_SR_RDST3;
            SI_COMCSR_REG |= SI_COMCSR_RDSTINT;
        }
    }

    // generate RDST interrupt
    if((SI_COMCSR_REG & SI_COMCSR_RDSTINT) && (SI_COMCSR_REG & SI_COMCSR_RDSTINTMSK))
    {
        // assert processor interrupt
        PIAssertInt(PI_INTERRUPT_SI);
    }

    EndProfilePAD();
}

// ---------------------------------------------------------------------------
// init

void SIOpen(BOOL fake)
{
    DBReport(CYAN "SI: Serial interface driver\n");

    si.fake = fake;         // set fake mode ?

    // clear all registers
    memset(&si, 0, sizeof(SIControl));

    // these values are actually written when IPL boots
    // meaning is unknown (some pad command) and no need to be known
    si.out[0] = 
    si.out[1] = 
    si.out[2] = 
    si.out[3] = 0x00400300; // continue polling ?

    // enable polling (for homebrewn), IPL enabling it
    SI_POLL_REG |= (SI_POLL_EN0 | SI_POLL_EN1 | SI_POLL_EN2 | SI_POLL_EN3);

    // update joypad data
    SIPoll();

    // set rumble flags
    for(int i=0; i<4; i++)
        si.rumble[i] = PADSetRumble(i, PAD_MOTOR_STOP);

    // joypads in/out command buffer
    HWSetTrap(32, SI_CHAN0_OUTBUF, si_rd_out0, si_wr_out0);
    HWSetTrap(32, SI_CHAN0_INBUFH, si_inh0, NULL);
    HWSetTrap(32, SI_CHAN0_INBUFL, si_inl0, NULL);
    HWSetTrap(32, SI_CHAN1_OUTBUF, si_rd_out1, si_wr_out1);
    HWSetTrap(32, SI_CHAN1_INBUFH, si_inh1, NULL);
    HWSetTrap(32, SI_CHAN1_INBUFL, si_inl1, NULL);
    HWSetTrap(32, SI_CHAN2_OUTBUF, si_rd_out2, si_wr_out2);
    HWSetTrap(32, SI_CHAN2_INBUFH, si_inh2, NULL);
    HWSetTrap(32, SI_CHAN2_INBUFL, si_inl2, NULL);
    HWSetTrap(32, SI_CHAN3_OUTBUF, si_rd_out3, si_wr_out3);
    HWSetTrap(32, SI_CHAN3_INBUFH, si_inh3, NULL);
    HWSetTrap(32, SI_CHAN3_INBUFL, si_inl3, NULL);
    
    // si control registers
    HWSetTrap(32, SI_POLL  , read_poll   , write_poll);
    HWSetTrap(32, SI_COMCSR, read_commcsr, write_commcsr);
    HWSetTrap(32, SI_SR    , read_sisr   , write_sisr);
    HWSetTrap(32, SI_EXILK , read_exilk  , write_exilk);

    // serial communcation buffer
    for(unsigned ofs=0; ofs<128; ofs+=4)
    {
        HWSetTrap(32, SI_COMBUF | ofs, read_sicom, write_sicom);
    }

    // fake SI for debug
    if(fake)
    {
        HWSetTrap(32, SI_CHAN0_OUTBUF, no_read, no_write);
        HWSetTrap(32, SI_CHAN0_INBUFH, no_read, NULL);
        HWSetTrap(32, SI_CHAN0_INBUFL, no_read, NULL);
        HWSetTrap(32, SI_CHAN1_OUTBUF, no_read, no_write);
        HWSetTrap(32, SI_CHAN1_INBUFH, no_read, NULL);
        HWSetTrap(32, SI_CHAN1_INBUFL, no_read, NULL);
        HWSetTrap(32, SI_CHAN2_OUTBUF, no_read, no_write);
        HWSetTrap(32, SI_CHAN2_INBUFH, no_read, NULL);
        HWSetTrap(32, SI_CHAN2_INBUFL, no_read, NULL);
        HWSetTrap(32, SI_CHAN3_OUTBUF, no_read, no_write);
        HWSetTrap(32, SI_CHAN3_INBUFH, no_read, NULL);
        HWSetTrap(32, SI_CHAN3_INBUFL, no_read, NULL);
        HWSetTrap(32, SI_POLL  , no_read, no_write);
        HWSetTrap(32, SI_COMCSR, no_read, no_write);
        HWSetTrap(32, SI_SR    , no_read, no_write);
        HWSetTrap(32, SI_EXILK , no_read, no_write);
        for(unsigned ofs=0; ofs<128; ofs+=4)
        {
            HWSetTrap(32, SI_COMBUF | ofs, no_read, no_write);
        }
    }
}
