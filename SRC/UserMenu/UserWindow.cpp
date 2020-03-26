// main window controls and creation.
// main window consist from 3 parts : main menu, selector and statusbar.
// selector is active only when emulator is in Idle state (not running);
// statusbar is used to show current emulator state and performance.
// last note : DO NOT USE WINDOWS API CODE IN OTHER SUB-SYSTEMS!!
#include "dolphin.h"

// all important data is placed here
UserWindow wnd;

// ---------------------------------------------------------------------------
// statusbar

// set default values of statusbar parts
static void ResetStatusBar()
{
    SetStatusText(STATUS_ENUM::Progress,  _T("Idle"));
    SetStatusText(STATUS_ENUM::Fps,       _T(""));
    SetStatusText(STATUS_ENUM::Timing,    _T(""));
    SetStatusText(STATUS_ENUM::Time,      _T(""));
}

// create status bar window
static void CreateStatusBar()
{
    int parts[] = { 0, 360, 420, 480, -1 };

    if(wnd.hMainWindow == NULL) return;

    // create window
    wnd.hStatusWindow = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE,
        NULL,
        wnd.hMainWindow,
        ID_STATUS_BAR
    );
    assert(wnd.hStatusWindow);

    // depart statusbar
    SendMessage( wnd.hStatusWindow, 
                 SB_SETPARTS, 
                 (WPARAM)sizeof(parts) / sizeof(int), 
                 (LPARAM)parts);

    // set default values
    ResetStatusBar();
}

// change text in specified statusbar part
void SetStatusText(STATUS_ENUM sbPart, const TCHAR *text, bool post)
{
    if(wnd.hStatusWindow == NULL) return;
    if(post)
    {
        PostMessage(wnd.hStatusWindow, SB_SETTEXT, (WPARAM)(sbPart), (LPARAM)text);
    }
    else
    {
        SendMessage(wnd.hStatusWindow, SB_SETTEXT, (WPARAM)(sbPart), (LPARAM)text);
    }
}

// get text of statusbar part
TCHAR *GetStatusText(STATUS_ENUM sbPart)
{
    static TCHAR sbText[256] = { 0, };

    if(wnd.hStatusWindow == NULL) return NULL;

    SendMessage(wnd.hStatusWindow, SB_GETTEXT, (WPARAM)(sbPart), (LPARAM)sbText);
    return sbText;
}

void StartProgress(int range, int delta)
{
    StopProgress();

    RECT rect;
    SendMessage(wnd.hStatusWindow, SB_GETRECT, 1, (LPARAM)&rect);
    int cyVScroll = GetSystemMetrics(SM_CYVSCROLL);

    wnd.hProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 
        rect.left, rect.top + abs(rect.bottom - rect.top) / 2 - cyVScroll / 2, rect.right, cyVScroll, 
        wnd.hStatusWindow, NULL, GetModuleHandle(NULL), 0);
    if(wnd.hProgress == NULL) return;
    
    SendMessage(wnd.hProgress, PBM_SETPOS, 0, 0);
    SendMessage(wnd.hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, range));
    SendMessage(wnd.hProgress, PBM_SETSTEP, delta, 0);
}

void StepProgress()
{
    if(wnd.hProgress)
    {
        SendMessage(wnd.hProgress, PBM_STEPIT, 0, 0);
    }
}

void StopProgress()
{
    if(wnd.hProgress)
    {
        DestroyWindow(wnd.hProgress);
        wnd.hProgress = NULL;
    }
}

// ---------------------------------------------------------------------------
// recent files list (moved from UserLoader - it was bad place there)

#define MAX_RECENT  5   // if you want to increase, you must also add new ID_FILE_RECENT_*

// returns -1 if not found
static int GetMenuItemIndex(HMENU hMenu, char *item)
{
    int     index = 0;
    char    buf[256];

    while(index < GetMenuItemCount(hMenu))
    {
        if(GetMenuStringA(hMenu, index, buf, sizeof(buf)-1, MF_BYPOSITION))
        {
            if(!strcmp(item, buf)) return index;
        }
        index++;
    }
    return -1;
}

static void SetRecentEntry(int index, char *str)
{
    char var[256];
    sprintf_s (var, sizeof(var), USER_RECENT, index);
    SetConfigString(var, str);
}

static char *GetRecentEntry(int index)
{
    char var[256];
    sprintf_s (var, sizeof(var), USER_RECENT, index);
    return GetConfigString(var, "");
}

void UpdateRecentMenu(HWND hwnd)
{
    HMENU   hMainMenu;
    HMENU   hFileMenu;
    HMENU   hReloadMenu;
    UINT    idx;

    // search for required menu sub-item
    hMainMenu = GetMenu(hwnd);
    if(hMainMenu == NULL) return;

    idx = GetMenuItemIndex(hMainMenu, "&File");             // take care about it
    if(idx < 0) return;
    hFileMenu = GetSubMenu(hMainMenu, idx);
    if(hFileMenu == NULL) return;

    idx = GetMenuItemIndex(hFileMenu, "&Reopen\tCtrl+R");   // take care about it
    if(idx < 0) return;
    hReloadMenu = GetSubMenu(hFileMenu, idx);
    if(hReloadMenu == NULL) return;

    // clear recent list
    while(GetMenuItemCount(hReloadMenu))
    {
        DeleteMenu(hReloadMenu, 0, MF_BYPOSITION);
    }

    // if no recent, add empty
    if(GetConfigInt(USER_RECENT_NUM, USER_RECENT_NUM_DEFAULT) == 0)
    {
        AppendMenuA(hReloadMenu, MF_GRAYED | MF_STRING, ID_FILE_RECENT_1, "None");
    }
    else
    {
        char buf[256];
        int RecentNum = GetConfigInt(USER_RECENT_NUM, USER_RECENT_NUM_DEFAULT);

        for(int i=0, n = RecentNum; i<RecentNum; i++, n--)
        {
            sprintf_s(buf, sizeof(buf), "%s", FileShortName(GetRecentEntry(n), 3));
            AppendMenuA(hReloadMenu, MF_STRING, ID_FILE_RECENT_1+i, buf);
        }
    }

    DrawMenuBar(hwnd);
}

void AddRecentFile(char *path)
{
    int n;
    int RecentNum = GetConfigInt(USER_RECENT_NUM, USER_RECENT_NUM_DEFAULT);

    // check if item already present in list
    for(n=1; n<=RecentNum; n++)
    {
        if(!_stricmp(path, GetRecentEntry(n)))
        {
            // place old recent to the top
            // and move upper recents down
            char old[256];
            sprintf_s (old, sizeof(old), "%s", GetRecentEntry(n));
            for(n=n+1; n<=RecentNum; n++)
            {
                SetRecentEntry(n-1, GetRecentEntry(n));
            }
            SetRecentEntry(RecentNum, old);
            UpdateRecentMenu(wnd.hMainWindow);
            return;
        }
    }

    // increase amount of recent files
    RecentNum++;
    if(RecentNum > MAX_RECENT)
    {
        // move list up
        for(n=1; n<MAX_RECENT; n++)
        {
            SetRecentEntry(n, GetRecentEntry(n+1));
        }
        RecentNum = 5;
    }
    SetConfigInt(USER_RECENT_NUM, RecentNum);

    // add new entry
    SetRecentEntry(RecentNum, path);
    UpdateRecentMenu(wnd.hMainWindow);
}

// index = 1..max
void LoadRecentFile(int index)
{
    int RecentNum = GetConfigInt(USER_RECENT_NUM, USER_RECENT_NUM_DEFAULT);
    char *path = GetRecentEntry((RecentNum+1) - index);
    LoadFile(path);
}

// ---------------------------------------------------------------------------
// window actions, on emu start/stop (skip "static" functions, see OnMainWindow*)
// tip : this is nice place for your windows-related init code

// select sort method (Options->View->Sort by)
static void SelectSort()
{
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_1, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_2, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_3, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_4, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_5, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_6, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_7, MF_BYCOMMAND | MF_UNCHECKED);
    
    switch(GetConfigInt(USER_SORTVIEW, USER_SORTVIEW_DEFAULT))
    {
        case SELECTOR_SORT_DEFAULT:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_1, MF_BYCOMMAND | MF_CHECKED);
            break;
        case SELECTOR_SORT_FILENAME:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_2, MF_BYCOMMAND | MF_CHECKED);
            break;
        case SELECTOR_SORT_TITLE:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_3, MF_BYCOMMAND | MF_CHECKED);
            break;
        case SELECTOR_SORT_SIZE:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_4, MF_BYCOMMAND | MF_CHECKED);
            break;
        case SELECTOR_SORT_ID:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_5, MF_BYCOMMAND | MF_CHECKED);
            break;
        case SELECTOR_SORT_COMMENT:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_6, MF_BYCOMMAND | MF_CHECKED);
            break;
        default:
            CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SORTBY_7, MF_BYCOMMAND | MF_CHECKED);
    }
}

// change Swap Controls
void ModifySwapControls(BOOL state)
{
    if(state)       // opened
    {
        SetMenuItemText(wnd.hMainMenu, ID_FILE_COVER, L"&Close Cover");
        EnableMenuItem(wnd.hMainMenu, ID_FILE_CHANGEDVD, MF_BYCOMMAND | MF_ENABLED);
    }
    else            // closed
    {
        SetMenuItemText(wnd.hMainMenu, ID_FILE_COVER, L"&Open Cover");
        EnableMenuItem(wnd.hMainMenu, ID_FILE_CHANGEDVD, MF_BYCOMMAND | MF_GRAYED);
    }
}

// set menu selector-related controls state
void ModifySelectorControls(bool active)
{
    if(active)
    {
        SetMenuItemText(wnd.hMainMenu, ID_OPTIONS_VIEW_DISABLE, L"&Disable Selector");
        EnableMenuItem(wnd.hMainMenu, ID_FILE_EDITINFO, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(wnd.hMainMenu, ID_FILE_REFRESH, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem( GetSubMenu(GetSubMenu(wnd.hMainMenu, 2), 1),    // Sort By..
                        5, MF_BYPOSITION | MF_ENABLED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_FILEFILTER, MF_BYCOMMAND | MF_ENABLED);
    }
    else
    {
        SetMenuItemText(wnd.hMainMenu, ID_OPTIONS_VIEW_DISABLE, L"&Enable Selector");
        EnableMenuItem(wnd.hMainMenu, ID_FILE_EDITINFO, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(wnd.hMainMenu, ID_FILE_REFRESH, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem( GetSubMenu(GetSubMenu(wnd.hMainMenu, 2), 1),    // Sort By..
                        5, MF_BYPOSITION | MF_GRAYED);
        EnableMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_FILEFILTER, MF_BYCOMMAND | MF_GRAYED);
    }
}

// called once, during main window creation
static void OnMainWindowCreate(HWND hwnd)
{
    // save handlers
    wnd.hMainWindow = hwnd;
    wnd.hMainMenu = GetMenu(wnd.hMainWindow);

    // run once ?
    if(GetConfigInt(USER_RUNONCE, USER_RUNONCE_DEFAULT))
        CheckMenuItem(wnd.hMainMenu, ID_RUN_ONCE, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(wnd.hMainMenu, ID_RUN_ONCE, MF_BYCOMMAND | MF_UNCHECKED);

    // allow patches ?
    if(GetConfigInt(USER_PATCH, USER_PATCH_DEFAULT))
        CheckMenuItem(wnd.hMainMenu, ID_ALLOW_PATCHES, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(wnd.hMainMenu, ID_ALLOW_PATCHES, MF_BYCOMMAND | MF_UNCHECKED);

    // debugger enabled ?
    emu.doldebug = GetConfigInt(USER_DOLDEBUG, USER_DOLDEBUG_DEFAULT);
    CheckMenuItem(wnd.hMainMenu, ID_DEBUG_CONSOLE, MF_BYCOMMAND | MF_UNCHECKED);
    if(emu.doldebug)
    {
        DBOpen();            
        CheckMenuItem(wnd.hMainMenu, ID_DEBUG_CONSOLE, MF_BYCOMMAND | MF_CHECKED);
    }

    // load accelerators
    InitCommonControls();

    // always on top (not in debug)
    wnd.ontop = GetConfigInt(USER_ONTOP, USER_ONTOP_DEFAULT) & 1;
    if(wnd.ontop) CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_ALWAYSONTOP, MF_CHECKED);
    else CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_ALWAYSONTOP, MF_UNCHECKED);
    SetAlwaysOnTop(wnd.hMainWindow, wnd.ontop);

    // recent menu
    UpdateRecentMenu(wnd.hMainWindow);

    // dvd swap controls
    ModifySwapControls(0);

    // child windows
    wnd.hPopupMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_POPUP_MENU));
    CreateStatusBar();
    ResizeMainWindow(640, 480-32);  // 32 pixels overscan

    // selector disabled ?
    usel.active = GetConfigInt(USER_SELECTOR, USER_SELECTOR_DEFAULT);
    ModifySelectorControls(usel.active);

    // icon size
    int smallSize = GetConfigInt(USER_SMALLICONS, USER_SMALLICONS_DEFAULT);
    if(smallSize)
    {
        CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_CHECKED);
    }
    else
    {
        CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_CHECKED);
    }

    // select sort method
    SelectSort();

    // enable drop operation
    DragAcceptFiles(wnd.hMainWindow, TRUE);

    // simulate close operation, like we just stopped emu
    OnMainWindowClosed();
}

// called once, when Dolwin exits to OS
static void OnMainWindowDestroy()
{
    // disable drop operation
    DragAcceptFiles(wnd.hMainWindow, FALSE);

    EMUClose();     // completely close the Dolwin
    exit(1);        // return good
}

// emulation has started - do proper actions
void OnMainWindowOpened()
{
    // disable selector
    CloseSelector();
    ModifySelectorControls(false);
    EnableMenuItem( GetSubMenu(wnd.hMainMenu, GetMenuItemIndex(     // View
                    wnd.hMainMenu, "&Options") ), 1, MF_BYPOSITION | MF_GRAYED );

    // set new title for main window
    char newTitle[1024], gameTitle[64], comment[128];
    char prefix[] = { APPNAME " - Running %s" };
    if(ldat.dvd)
    {
        sprintf_s (gameTitle, sizeof(gameTitle), "%s", ldat.currentFileName);
        sprintf_s (newTitle, sizeof(newTitle), prefix, gameTitle);
    }
    else
    {
        sprintf_s (gameTitle, sizeof(gameTitle), "%s demo", ldat.currentFileName);
        sprintf_s (newTitle, sizeof(newTitle), prefix, gameTitle);
    }
    SetWindowTextA(wnd.hMainWindow, newTitle);

    // user profiler
    OpenProfiler(GetConfigInt(USER_PROFILE, USER_PROFILE_DEFAULT));
}

// emulation stop in progress
void OnMainWindowClosed()
{
    // restore current working directory
    SetCurrentDirectoryA(ldat.cwd);

    // enable selector
    CreateSelector();
    ModifySelectorControls(usel.active);
    EnableMenuItem(GetSubMenu(wnd.hMainMenu, GetMenuItemIndex(     // View
        wnd.hMainMenu, "&Options")), 1, MF_BYPOSITION | MF_ENABLED);

    // set to Idle
    SetWindowText(wnd.hMainWindow, WIN_NAME);
    ResetStatusBar();
}

// ---------------------------------------------------------------------------
// window controls

// resize client area to fit in given width and height
void ResizeMainWindow(int width, int height)
{
    RECT rc;
    int x, y, w, h;

    GetWindowRect(wnd.hMainWindow, &rc);

    // left-upper corner
    x = rc.left;
    y = rc.top;

    // calculate adjustment
    rc.left = 0;
    rc.top = 0;
    rc.right = width;
    rc.bottom = height;
    AdjustWindowRect(&rc, WIN_STYLE, 1);

    // width and height
    w = rc.right - rc.left;
    h = rc.bottom - rc.top + GetSystemMetrics(SM_CYCAPTION) + 9;

    // adjust by statusbar height
    if(IsWindow(wnd.hStatusWindow))
    {
        GetWindowRect(wnd.hStatusWindow, &rc);
        h += (WORD)(rc.bottom - rc.top);
    }

    // move window
    MoveWindow(wnd.hMainWindow, x, y, w, h, TRUE);
    SendMessage(wnd.hMainWindow, WM_SIZE, 0, 0);
}

// main window procedure : "return 0" to leave, "break" to continue DefWindowProc()
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FILE *f;
    char *name;
    int recent;

    switch(msg)
    {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // creation/destroy messages 

        case WM_CREATE:
            OnMainWindowCreate(hwnd);
            return 0;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            OnMainWindowDestroy();
            return 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // window controls

        case WM_COMMAND:
            // load recent file
            if(LOWORD(wParam) >= ID_FILE_RECENT_1 &&
               LOWORD(wParam) <= ID_FILE_RECENT_5)
            {
                LoadRecentFile(LOWORD(wParam) - ID_FILE_RECENT_1 + 1);
                EMUClose();
                EMUOpen();
            }
            else switch(LOWORD(wParam))
            {
                // load DVD/executable (START)
                case ID_FILE_LOAD:
                    if((name = UI::FileOpen(hwnd)) != nullptr)
                    {
loadFile:
                        LoadFile(name);
                        EMUClose();
                        EMUOpen();
                    }
                    return 0;

                // reload last opened file (RESET)
                case ID_FILE_RELOAD:
                    recent = GetConfigInt(USER_RECENT_NUM, USER_RECENT_NUM_DEFAULT);
                    if(recent > 0)
                    {
                        LoadRecentFile(1);
                        EMUClose();
                        EMUOpen();
                    }
                    return 0;

                // unload file (STOP)
                case ID_FILE_UNLOAD:
                    EMUClose();
                    return 0;

                // load bootrom
                case ID_FILE_IPLMENU:
                    LoadFile("Bootrom");
                    EMUClose();
                    EMUOpen();
                    return 0;

                // open/close DVD lid
                case ID_FILE_COVER:
                    if(DIGetCoverState())   // close lid
                    {
                        DICloseCover();
                        ModifySwapControls(0);
                    }
                    else                    // open lid
                    {
                        DIOpenCover();
                        ModifySwapControls(1);
                    }
                    return 0;

                // set new current DVD image
                case ID_FILE_CHANGEDVD:
                    if((name = UI::FileOpen(hwnd, UI::FileType::Dvd)) != nullptr && DIGetCoverState())
                    {
                        if(!_stricmp(name, ldat.currentFile)) return 0;  // same
                        if(DVDSetCurrent(name) == FALSE) return 0;      // bad

                        // close lid
                        DICloseCover();
                        ModifySwapControls(0);
                    }
                    return 0;

                // exit to OS
                case ID_FILE_EXIT:
                    DestroyWindow(hwnd);
                    return 0;

                // settings dialog
                case ID_OPTIONS_SETTINGS:
                    OpenSettingsDialog(hwnd, GetModuleHandle(NULL));
                    return 0;

                // always on top
                case ID_OPTIONS_ALWAYSONTOP:
                    wnd.ontop ^= 1;
                    SetConfigInt(USER_ONTOP, wnd.ontop & 1);
                    if(wnd.ontop) CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_CHECKED);
                    else CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_UNCHECKED);
                    SetAlwaysOnTop(hwnd, wnd.ontop);
                    return 0;

                // refresh view
                case ID_FILE_REFRESH:
                    UpdateSelector();
                    return 0;

                // disable the view
                case ID_OPTIONS_VIEW_DISABLE:
                    if(usel.active)
                    {
                        CloseSelector();
                        ResetStatusBar();
                        ModifySelectorControls(false);
                        SetConfigInt(USER_SELECTOR, usel.active = FALSE);
                    }
                    else
                    {
                        ModifySelectorControls(true);
                        SetConfigInt(USER_SELECTOR, usel.active = TRUE);
                        CreateSelector();
                    }
                    return 0;

                // change icon size
                case ID_OPTIONS_VIEW_SMALLICONS:
                    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_CHECKED);
                    SetSelectorIconSize(TRUE);
                    return 0;
                case ID_OPTIONS_VIEW_LARGEICONS:
                    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_SMALLICONS, MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(wnd.hMainMenu, ID_OPTIONS_VIEW_LARGEICONS, MF_BYCOMMAND | MF_CHECKED);
                    SetSelectorIconSize(FALSE);
                    return 0;

                // sort files
                case ID_OPTIONS_VIEW_SORTBY_1:
                    SortSelector(SELECTOR_SORT_DEFAULT);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_2:
                    SortSelector(SELECTOR_SORT_FILENAME);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_3:
                    SortSelector(SELECTOR_SORT_TITLE);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_4:
                    SortSelector(SELECTOR_SORT_SIZE);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_5:
                    SortSelector(SELECTOR_SORT_ID);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_6:
                    SortSelector(SELECTOR_SORT_COMMENT);
                    SelectSort();
                    return 0;
                case ID_OPTIONS_VIEW_SORTBY_7:
                    SortSelector(0);    // unsorted
                    SelectSort();
                    return 0;

                // boot!
                case ID_FILE_BOOT:
                {
                    name = usel.selected->name;
                    goto loadFile;
                }

                // edit file filter
                case ID_OPTIONS_VIEW_FILEFILTER:
                {
                    EditFileFilter(hwnd);
                    return 0;
                }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // debug controls

                // multiple instancies on/off
                case ID_RUN_ONCE:
                    if(GetConfigInt(USER_RUNONCE, USER_RUNONCE_DEFAULT))
                    {   // off
                        CheckMenuItem(wnd.hMainMenu, ID_RUN_ONCE, MF_BYCOMMAND | MF_UNCHECKED);
                        SetConfigInt(USER_RUNONCE, FALSE);
                    }
                    else
                    {   // on
                        CheckMenuItem(wnd.hMainMenu, ID_RUN_ONCE, MF_BYCOMMAND | MF_CHECKED);
                        SetConfigInt(USER_RUNONCE, TRUE);
                    }
                    return 0;

                // dump main memory
                case ID_DUMP_RAM:
                    if (mi.ram)
                    {
                        SetStatusText(STATUS_ENUM::Progress, _T("Dumping main memory..."));
                        UI::FileSave(_T("RAM.bin"), mi.ram, RAMSIZE);
                        SetStatusText(STATUS_ENUM::Progress, _T("Main memory dumped in RAM.bin"));
                    }
                    return 0;

                // dump aux. memory
                case ID_DUMP_ARAM:
                    if (ARAM)
                    {
                        SetStatusText(STATUS_ENUM::Progress, _T("Dumping aux. memory..."));
                        UI::FileSave(_T("ARAM.bin"), ARAM, ARAMSIZE);
                        SetStatusText(STATUS_ENUM::Progress, _T("Aux. memory dumped in ARAM.bin"));
                    }
                    return 0;

                // dump OS low memory
                case ID_DUMP_LOMEM:
                    if (mi.ram)
                    {
                        UI::FileSave(_T("lomem.bin"), mi.ram, 0x3100);
                        SetStatusText(STATUS_ENUM::Progress, _T("OS low memory dumped in lomem.bin"));
                    }
                    return 0;

                // enable patches
                case ID_ALLOW_PATCHES:
                    if(GetConfigInt(USER_PATCH, USER_PATCH_DEFAULT))
                    {   // off
                        CheckMenuItem(wnd.hMainMenu, ID_ALLOW_PATCHES, MF_BYCOMMAND | MF_UNCHECKED);
                        SetConfigInt(USER_PATCH, ldat.enablePatch = FALSE);
                    }
                    else
                    {   // on
                        CheckMenuItem(wnd.hMainMenu, ID_ALLOW_PATCHES, MF_BYCOMMAND | MF_CHECKED);
                        SetConfigInt(USER_PATCH, ldat.enablePatch = TRUE);
                    }
                    return 0;

                // load patch data
                case ID_LOAD_PATCH:
                    if((name = UI::FileOpen(hwnd, UI::FileType::Patch)) != nullptr)
                    {
                        UnloadPatch();
                        LoadPatch(name, 0);
                    }
                    return 0;

                // add new patch data
                case ID_ADD_PATCH:
                    if((name = UI::FileOpen(hwnd, UI::FileType::Patch)) != nullptr)
                    {
                        LoadPatch(name, 1);
                    }
                    return 0;

                // open/close debugger
                case ID_DEBUG_CONSOLE:
                    if(!emu.doldebug)
                    {   // open
                        CheckMenuItem(wnd.hMainMenu, ID_DEBUG_CONSOLE, MF_BYCOMMAND | MF_CHECKED);
                        DBOpen();
                        emu.doldebug = TRUE;
                        SetConfigInt(USER_DOLDEBUG, emu.doldebug);
                        SetStatusText(STATUS_ENUM::Progress, _T("Debugger opened"));
                    }
                    else
                    {   // close
                        CheckMenuItem(wnd.hMainMenu, ID_DEBUG_CONSOLE, MF_BYCOMMAND | MF_UNCHECKED);
                        DBClose();
                        emu.doldebug = FALSE;
                        SetConfigInt(USER_DOLDEBUG, emu.doldebug);
                        SetStatusText(STATUS_ENUM::Progress, _T("Debugger closed"));
                    }
                    return 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // options dialogs

                case ID_OPTIONS_CONTROLLERS_PORT1:
                    PADConfigure(0, wnd.hMainWindow);
                    return 0;
                case ID_OPTIONS_CONTROLLERS_PORT2:
                    PADConfigure(1, wnd.hMainWindow);
                    return 0;
                case ID_OPTIONS_CONTROLLERS_PORT3:
                    PADConfigure(2, wnd.hMainWindow);
                    return 0;
                case ID_OPTIONS_CONTROLLERS_PORT4:
                    PADConfigure(3, wnd.hMainWindow);
                    return 0;

                // configure memcard in slot A
                case ID_OPTIONS_MEMCARDS_SLOTA:
                    MemcardConfigure(0, hwnd);
                    return 0;

                // configure memcard in slot B
                case ID_OPTIONS_MEMCARDS_SLOTB:
                    MemcardConfigure(1, hwnd);
                    return 0;

                // configure fonts
                case ID_OPTIONS_BOOTROMFONT:
                    FontConfigure(hwnd);
                    break;

                case ID_HELP_ABOUT:
                    AboutDialog(hwnd);
                    return 0;
            }
            break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // file drag & drop operation

        case WM_DROPFILES:
        {
            char fileName[256];

            strcpy_s(fileName, sizeof(fileName), "");
            DragQueryFileA((HDROP)wParam, 0, fileName, sizeof(fileName));
            DragFinish((HDROP)wParam);

            // extension filter
            if(_stricmp(".dol", strrchr(fileName, '.')) &&
               _stricmp(".elf", strrchr(fileName, '.')) &&
               _stricmp(".bin", strrchr(fileName, '.')) &&
               _stricmp(".iso", strrchr(fileName, '.')) &&
               _stricmp(".gcm", strrchr(fileName, '.')) ) break;

            name = fileName;
            goto loadFile;
        }
        return 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // resizing of main window

        case WM_SIZE:
        {
            // resize status bar window
            if(IsWindow(wnd.hStatusWindow))
            { 
                RECT rm, rs;
                GetWindowRect(hwnd, &rm);
                GetWindowRect(wnd.hStatusWindow, &rs); 
                long sbh = (WORD)(rs.bottom - rs.top);
                MoveWindow(wnd.hStatusWindow, rm.left, rm.top, rm.right - rm.left, rm.bottom - sbh, TRUE);
            }

            // resize selector
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                ResizeSelector(
                    (WORD)(rc.right - rc.left),
                    (WORD)(rc.bottom - rc.top)
                );
            }

            // external video buffer
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                GDIResize(
                    (int)(rc.right - rc.left),
                    (int)(rc.bottom - rc.top)
                );
            }

            return 0;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // selector notification messages

        case WM_NOTIFY:
        {
            if(wParam == ID_SELECTOR)
            {
                NotifySelector((LPNMHDR)lParam);

                // sort rules may change, by clicking on column
                SelectSort();
            }
            return 0;
        }

        case WM_DRAWITEM:
        {
            if(wParam == ID_SELECTOR)
            {
                DrawSelectorItem((LPDRAWITEMSTRUCT)lParam);
            }
            return 0;
        }        
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// self-explanatory.. creates main window and all child windows.
// window size will be set to default 400x300.
HWND CreateMainWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc;

    assert(wnd.hMainWindow == nullptr);

    wc.cbClsExtra    = wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOLWIN_ICON));
    wc.hInstance     = hInstance;
    wc.lpfnWndProc   = WindowProc;
    wc.lpszClassName = _T("GAMECUBECLASS");
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAIN_MENU);
    wc.style         = 0;

    assert(RegisterClass(&wc));

    wnd.hMainWindow = CreateWindowEx(
        0,
        _T("GAMECUBECLASS"), WIN_NAME,
        WIN_STYLE, 
        20, 30,
        400, 300,
        NULL, NULL,
        hInstance, NULL);

    assert(wnd.hMainWindow);

    ShowWindow(wnd.hMainWindow, SW_NORMAL);
    UpdateWindow(wnd.hMainWindow);

    return wnd.hMainWindow;
}

// ---------------------------------------------------------------------------
// utilities

// change main window on-top state
void SetAlwaysOnTop(HWND hwnd, BOOL state)
{
    RECT rect;
    HWND ontop[] = { HWND_NOTOPMOST, HWND_TOPMOST };
    GetWindowRect(hwnd, &rect);
    SetWindowPos(
        hwnd, 
        ontop[state], 
        rect.left, rect.top, 
        rect.right - rect.left, rect.bottom - rect.top, 
        SWP_SHOWWINDOW
    );
    UpdateWindow(hwnd);
}

// center the child window into the parent window
void CenterChildWindow(HWND hParent, HWND hChild)
{
   if(IsWindow(hParent) && IsWindow(hChild))
   {
        RECT rp, rc;

        GetWindowRect(hParent, &rp);
        GetWindowRect(hChild, &rc);

        MoveWindow(hChild, 
            rp.left + (rp.right - rp.left - rc.right + rc.left) / 2,
            rp.top + (rp.bottom - rp.top - rc.bottom + rc.top) / 2,
            rc.right - rc.left, rc.bottom - rc.top, TRUE
        );
    }
}

void SetMenuItemText(HMENU hmenu, UINT id, wchar_t *text)
{
    MENUITEMINFO info;

    memset(&info, 0, sizeof(MENUITEMINFO));
    
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask  = MIIM_TYPE;
    info.fType  = MFT_STRING;
    info.dwTypeData = (LPTSTR)text;

    SetMenuItemInfo(hmenu, id, FALSE, &info);
}
