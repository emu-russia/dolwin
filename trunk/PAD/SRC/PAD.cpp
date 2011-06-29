// Plugin API for emulator
#include "PAD.h"

PAD pad;

// ---------------------------------------------------------------------------
// run-time initializations

// dll entrypoint
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReasonForCall, LPVOID lpUnknown)
{
    if(dwReasonForCall == DLL_PROCESS_ATTACH)
    {
        pad.inst = (HINSTANCE)hModule;
    }
    return TRUE;
}

// make sure, that emulator is using correct PAD plugin
void RegisterPlugin(PluginData * plugData)
{
    // save main window handler
    pad.hwndParent = (HWND *)plugData->display;

    plugData->type = DOL_PLUG_PAD;
    plugData->version = "GC Controller Plugin (Windows Keyboard)" " (" PAD_VER ")";
}

// ---------------------------------------------------------------------------
// called when emulation started/stopped (pad controls)

long PADOpen()
{
    pad.padToConfigure = 0;
    PADLoadConfig(NULL);

    pad.padToConfigure = 1;
    PADLoadConfig(NULL);

    pad.padToConfigure = 2;
    PADLoadConfig(NULL);

    pad.padToConfigure = 3;
    PADLoadConfig(NULL);

    return TRUE;    // ok
}

void PADClose()
{
    pad.config[0].plugged = 
    pad.config[1].plugged = 
    pad.config[2].plugged = 
    pad.config[3].plugged = 0;

    KillTimer(*pad.hwndParent, 1);
    FlashWindow(*pad.hwndParent, FALSE);
}

// ---------------------------------------------------------------------------
// process input

#define THRESOLD    87

static void pad_reset_chan(PADState *state)
{
    memset(state, 0, sizeof(PADState));

    state->stickX = state->stickY = -128;
    state->substickX = state->substickY = -128;
}

// collect keyboard buttons in PADState
long PADReadButtons(long padnum, PADState *state)
{
    u16 button = 0;

    pad_reset_chan(state);

    if(!pad.config[padnum].plugged) return 0;

    if(padnum < 4)
    {
        //
        // ===== PAD n =====
        //

        if(pad.config[padnum].vkeys[VKEY_FOR_UP] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_UP]) & 0x80000000) button |= PAD_BUTTON_UP;
        if(pad.config[padnum].vkeys[VKEY_FOR_DOWN] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_DOWN]) & 0x80000000) button |= PAD_BUTTON_DOWN;
        if(pad.config[padnum].vkeys[VKEY_FOR_LEFT] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_LEFT]) & 0x80000000) button |= PAD_BUTTON_LEFT;
        if(pad.config[padnum].vkeys[VKEY_FOR_RIGHT] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_RIGHT]) & 0x80000000) button |= PAD_BUTTON_RIGHT;

        if(pad.config[padnum].vkeys[VKEY_FOR_A] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_A]) & 0x80000000) button |= PAD_BUTTON_A;
        if(pad.config[padnum].vkeys[VKEY_FOR_B] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_B]) & 0x80000000) button |= PAD_BUTTON_B;
        if(pad.config[padnum].vkeys[VKEY_FOR_X] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_X]) & 0x80000000) button |= PAD_BUTTON_X;
        if(pad.config[padnum].vkeys[VKEY_FOR_Y] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_Y]) & 0x80000000) button |= PAD_BUTTON_Y;
        if(pad.config[padnum].vkeys[VKEY_FOR_START] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_START]) & 0x80000000) button |= PAD_BUTTON_START;

        // Note : digital L and R are only set when its analog key is pressed all the way down;
        // this plugin is only supporting the fact, that L/R are pressed.

        if(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERL] != -1)
        {
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERL]) & 0x80000000)
            {
                button |= PAD_TRIGGER_L;
                state->triggerLeft = 255;
            }
        }
        if(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERR] != -1)
        {
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERR]) & 0x80000000)
            {
                button |= PAD_TRIGGER_R;
                state->triggerRight = 255;
            }
        }

        if(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERZ] != -1)
        {
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_TRIGGERZ]) & 0x80000000)
            {
                button |= PAD_TRIGGER_Z;
            }
        }

        if(pad.config[padnum].vkeys[VKEY_FOR_XUP50] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XUP50]) & 0x80000000) state->stickY += THRESOLD / 2;
        if(pad.config[padnum].vkeys[VKEY_FOR_XUP100] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XUP100]) & 0x80000000) state->stickY += THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_XDOWN50] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XDOWN50]) & 0x80000000) state->stickY += -THRESOLD / 2;
        if(pad.config[padnum].vkeys[VKEY_FOR_XDOWN100] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XDOWN100]) & 0x80000000) state->stickY += -THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_XRIGHT50] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XRIGHT50]) & 0x80000000) state->stickX += THRESOLD / 2;
        if(pad.config[padnum].vkeys[VKEY_FOR_XRIGHT100] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XRIGHT100]) & 0x80000000) state->stickX += THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_XLEFT50] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XLEFT50]) & 0x80000000) state->stickX += -THRESOLD / 2;
        if(pad.config[padnum].vkeys[VKEY_FOR_XLEFT100] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_XLEFT100]) & 0x80000000) state->stickX += -THRESOLD;

        if(pad.config[padnum].vkeys[VKEY_FOR_CXUP] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_CXUP]) & 0x80000000) state->substickY += THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_CXDOWN] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_CXDOWN]) & 0x80000000) state->substickY += -THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_CXRIGHT] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_CXRIGHT]) & 0x80000000) state->substickX += THRESOLD;
        if(pad.config[padnum].vkeys[VKEY_FOR_CXLEFT] != -1)
            if(GetAsyncKeyState(pad.config[padnum].vkeys[VKEY_FOR_CXLEFT]) & 0x80000000) state->substickX += -THRESOLD;
    } else return 0;

    state->button = button;

    return 1;
}

static VOID CALLBACK TimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime)
{
    FlashWindow(*pad.hwndParent, TRUE);
}

// controller motor. 0 returned, if rumble is not supported by PAD.
// see one of PAD_MOTOR* for allowed commands.
long PADSetRumble(long padnum, long cmd)
{
    if(pad.config[padnum].plugged)
    {
        pad.rumbleFlag[padnum] = cmd;

        if(cmd == PAD_MOTOR_RUMBLE)
        {
            SetTimer(*pad.hwndParent, 1, 50, TimerProc);
        }
        else
        {
            KillTimer(*pad.hwndParent, 1);
            FlashWindow(*pad.hwndParent, FALSE);
        }
        return TRUE;
    }
    else return FALSE;
}

// ---------------------------------------------------------------------------
// save/load (currently there is nothing to save)

// savestate (flag:0 to load, flag:1 to save)
void PADSaveLoad(long flag, char *filename)
{
    // empty
}
