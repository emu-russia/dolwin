// high level initialization code
#include "dolphin.h"

HLEControl hle;

// ---------------------------------------------------------------------------

void os_ignore() { DBReport(GREEN "High level ignore (pc: %08X, %s)\n", PC, SYMName(PC)); }
void os_ret0()   { GPR[3] = NULL; }
void os_ret1()   { GPR[3] = 1; }
void os_trap()   { PC = LR - 4; DBHalt("High level trap (pc: %08X)!\n", PC); }

// HLE Ignore (you know what are you doing!)
static char *osignore[] = {
    // stubs for audio/DSP
    //"__OSInitAudioSystem"       ,
    //"__AI_SRC_INIT"             ,
    //"DSPSendCommands2__FPUlUlPFUs_v",
    //"DsetupTable__FUlUlUlUlUl"  ,
    //"DsetDolbyDelay__FUlUs"     ,
    //"__AXOutInitDSP"            ,

    // video
    //"VIWaitForRetrace"          ,

    // Terminator
    "HLE_IGNORE",
    NULL
};

// HLE which return 0 as result
static char *osret0[] = {
    "VerifyID"                  ,

    // Terminator
    "HLE_RETURN0",
    NULL
};

// HLE which return 1 as result
static char *osret1[] = {

    // Terminator
    "HLE_RETURN1",
    NULL
};

// HLE Traps (calls, which can cause unpredictable situation)
static char *ostraps[] = {

    // Terminator
    "HLE_TRAP",
    NULL
};

// HLE Calls
static struct OSCalls
{
    char    *name;
    void    (*call)();
} oscalls[] = {
    // Interrupt handling
    { "OSDisableInterrupts"     , OSDisableInterrupts       },
    { "OSEnableInterrupts"      , OSEnableInterrupts        },
    { "OSRestoreInterrupts"     , OSRestoreInterrupts       },

    // Context API
    // its working, but we need better recognition for OSLoadContext
    // minimal set: OSSaveContext, OSLoadContext, __OSContextInit.
    { "OSSetCurrentContext"     , OSSetCurrentContext       },
    { "OSGetCurrentContext"     , OSGetCurrentContext       },
    { "OSSaveContext"           , OSSaveContext             },
    //{ "OSLoadContext"           , OSLoadContext             },
    { "OSClearContext"          , OSClearContext            },
    { "OSInitContext"           , OSInitContext             },
    { "OSLoadFPUContext"        , OSLoadFPUContext          },
    { "OSSaveFPUContext"        , OSSaveFPUContext          },
    { "OSFillFPUContext"        , OSFillFPUContext          },
    { "__OSContextInit"         , __OSContextInit           },

    // Std C
    { "memset"                  , HLE_memset                },
    { "memcpy"                  , HLE_memcpy                },
/*/
    { "cos"                     , HLE_cos                   },
    { "sin"                     , HLE_sin                   },
    { "modf"                    , HLE_modf                  },
    { "frexp"                   , HLE_frexp                 },
    { "ldexp"                   , HLE_ldexp                 },
    { "floor"                   , HLE_floor                 },
    { "ceil"                    , HLE_ceil                  },
/*/

    // Terminator
    { NULL                      , NULL                      }
};

// ---------------------------------------------------------------------------

// wrapper
void HLESetCall(char * name, void (*call)())
{
    SYMSetHighlevel(name, call);
}

void HLEOpen()
{
    DBReport(
        GREEN "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"
        GREEN "Highlevel Initialization.\n"
        GREEN "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n"
    );

    hle.only = GetConfigInt(USER_HLE_ONLY, USER_HLE_ONLY_DEFAULT);
    if(hle.only)
    {
        DBReport(GREEN "emulator is working in HLE mode.\n\n");
        hle.lastHwAssert = GetConfigInt(USER_HW_ASSERT, USER_HW_ASSERT_DEFAULT);
        SetConfigInt(USER_HW_ASSERT, 0);
        HWClose();  // cruel
        HWEnableUpdate(0);
    }

    SYMAddEmulatorSymbols();

    // set high level calls
    s32 n = 0;
    while(osignore[n])
    {
        HLESetCall(osignore[n++], os_ignore);
    } n = 0;
    while(osret0[n])
    {
        HLESetCall(osret0[n++], os_ret0);
    } n = 0;
    while(osret1[n])
    {
        HLESetCall(osret1[n++], os_ret1);
    } n = 0;
    while(ostraps[n])
    {
        HLESetCall(ostraps[n++], os_trap);
    } n = 0;
    while(oscalls[n].name)
    {
        HLESetCall(oscalls[n].name, oscalls[n].call);
        n++;
    }
  
    HLEResetHitrate();

    // init dsp
    DSPOpen();

    // controllers
    PADOpenHLE();

    // Geometry library
    MTXOpen();
}

void HLEClose()
{
    if(hle.only)
    {
        // restore HW_ASSERT value
        SetConfigInt(USER_HW_ASSERT, hle.lastHwAssert);
    }

    SYMKill();

    // shutdown dsp
    DSPClose();
}

void HLEExecuteCallback(u32 entryPoint)
{
    u32 old = LR;
    PC = entryPoint;
    LR = 0;
    while(PC) IPTExecuteOpcode();
    PC = LR = old;
}

void HLEResetHitrate()
{
    // clear hitrate history
    for(int i=0; i<HLE_HITRATE_MAX; i++)
        hle.hitrate[i] = 0;
}

void HLEGetTop10(int toplist[10])
{
    int top[HLE_HITRATE_MAX];
    memcpy(top, hle.hitrate, sizeof(hle.hitrate));

    for(int i=0; i<10; i++)
    {
        int maxval = top[1], maxid = 1;
        for(int id=2; id<HLE_HITRATE_MAX; id++)
        {
            if(top[id] >= maxval && top[id])
            {
                maxval = top[id];
                maxid  = id;
            }
        }
        if(top[maxid] == 0) maxid = 0;
        top[maxid] = 0;
        toplist[i] = maxid;
    }
}

char * HLEGetHitNameByIndex(int idx)
{
    // compiler should build nice jump table for us
    switch(idx)
    {
        case 0: return "No pretender";

        case HLE_OS_DISABLE_INTERRUPTS: return "OSDisableInterrupts";
        case HLE_OS_ENABLE_INTERRUPTS: return "OSEnableInterrupts";
        case HLE_OS_RESTORE_INTERRUPTS: return "OSRestoreInterrupts";

        case HLE_OS_SET_CURRENT_CONTEXT: return "OSSetCurrentContext";
        case HLE_OS_GET_CURRENT_CONTEXT: return "OSGetCurrentContext";
        case HLE_OS_SAVE_CONTEXT: return "OSSaveContext";
        case HLE_OS_LOAD_CONTEXT: return "OSLoadContext";
        case HLE_OS_CLEAR_CONTEXT: return "OSClearContext";
        case HLE_OS_INIT_CONTEXT: return "OSInitContext";

        case HLE_MEMCPY: return "memcpy";
        case HLE_MEMSET: return "memset";

        case HLE_MTX_IDENTITY: return "MTXIdentity";
        case HLE_MTX_COPY: return "MTXCopy";
        case HLE_MTX_CONCAT: return "MTXConcat";
        case HLE_MTX_TRANSPOSE: return "MTXTranspose";
        case HLE_MTX_INVERSE: return "MTXInverse";
        case HLE_MTX_INVXPOSE: return "MTXInvXpose";

        case HLE_PAD_READ: return "PADRead";
    }
    return "Unknown call";
}
