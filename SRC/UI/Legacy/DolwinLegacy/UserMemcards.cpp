#include "pch.h"

int um_num;
BOOL um_filechanged;

TCHAR Memcard_filename[2][0x1000];
bool SyncSave;
bool Memcard_Connected[2];

/* Memcard ids by number of blocks */
#define MEMCARD_ID_64       (0x0004)
#define MEMCARD_ID_128      (0x0008)
#define MEMCARD_ID_256      (0x0010)
#define MEMCARD_ID_512      (0x0020)
#define MEMCARD_ID_1024     (0x0040)
#define MEMCARD_ID_2048     (0x0080)

/* Memcard ids by number of usable blocks */
#define MEMCARD_ID_59       MEMCARD_ID_64
#define MEMCARD_ID_123      MEMCARD_ID_128
#define MEMCARD_ID_251      MEMCARD_ID_256
#define MEMCARD_ID_507      MEMCARD_ID_512
#define MEMCARD_ID_1019     MEMCARD_ID_1024 
#define MEMCARD_ID_2043     MEMCARD_ID_2048

#define MEMCARD_ERASEBYTE 0x00
//#define MEMCARD_ERASEBYTE 0xFF

#define Num_Memcard_ValidSizes 6

const uint32_t Memcard_ValidSizes[Num_Memcard_ValidSizes] = {
    0x00080000, //524288 bytes , // Memory Card 59
    0x00100000, //1048576 bytes , // Memory Card 123
    0x00200000, //2097152 bytes , // Memory Card 251
    0x00400000, //4194304 bytes , // Memory Card 507
    0x00800000, //8388608 bytes , // Memory Card 1019
    0x01000000  //16777216 bytes , // Memory Card 2043
};

#define Memcard_BlockSize       8192
#define Memcard_BlockSize_log2  13 // just to make shifts instead of mult. or div.
#define Memcard_SectorSize      512
#define Memcard_SectorSize_log2 9 // just to make shifts instead of mult. or div.
#define Memcard_PageSize        128
#define Memcard_PageSize_log2   7 // just to make shifts instead of mult. or div.

/*
 * Creates a new memcard file
 * memcard_id should be one of the following:
 * MEMCARD_ID_64       (0x0004)
 * MEMCARD_ID_128      (0x0008)
 * MEMCARD_ID_256      (0x0010)
 * MEMCARD_ID_512      (0x0020)
 * MEMCARD_ID_1024     (0x0040)
 * MEMCARD_ID_2048     (0x0080)
 */
bool    MCCreateMemcardFile(const TCHAR *path, uint16_t memcard_id) {
    FILE * newfile;
    uint32_t b, blocks;
    uint8_t newfile_buffer[Memcard_BlockSize];

    switch (memcard_id) {
    case MEMCARD_ID_64:
    case MEMCARD_ID_128:
    case MEMCARD_ID_256:
    case MEMCARD_ID_512:
    case MEMCARD_ID_1024:
    case MEMCARD_ID_2048:
        /* 17 = Mbits to byte conversion */
        blocks = ((uint32_t)memcard_id) << (17 - Memcard_BlockSize_log2); 
        break;
    default:
        UI::DolwinError (_T("Memcard Error"), _T("Wrong card id for creating file."));
        return false;
    }

    newfile = nullptr;
    _tfopen_s(&newfile, path, _T("wb")) ;

	if (newfile == NULL) {
        UI::DolwinError ( _T("Memcard Error"), _T("Error while trying to create memcard file."));
		return false;
	}

    memset(newfile_buffer, MEMCARD_ERASEBYTE, Memcard_BlockSize);
    for (b = 0; b < blocks; b++) {
        if (fwrite (newfile_buffer, Memcard_BlockSize, 1, newfile) != 1) {
            UI::DolwinError( _T("Memcard Error"), _T("Error while trying to write memcard file."));

			fclose (newfile);
            return false;
        }
    }

    fclose (newfile);
    return true;
}

static TCHAR *NewMemcardFileProc(HWND hwnd, TCHAR * lastDir)
{
    TCHAR prevc[MAX_PATH];
    OPENFILENAME ofn;
    TCHAR szFileName[120];
    TCHAR szFileTitle[120];
    static TCHAR LoadedFile[MAX_PATH];

    GetCurrentDirectory(sizeof(prevc), prevc);

    memset(&szFileName, 0, sizeof(szFileName));
    memset(&szFileTitle, 0, sizeof(szFileTitle));

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hwnd;
    ofn.lpstrFilter         = 
        _T("GameCube Memcard Files (*.mci)\0*.mci\0")
        _T("All Files (*.*)\0*.*\0");
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFileName;
    ofn.nMaxFile            = 120;
    ofn.lpstrInitialDir     = lastDir;
    ofn.lpstrFileTitle      = szFileTitle;
    ofn.nMaxFileTitle       = 120;
    ofn.lpstrTitle          = _T("Create Memcard File\0");
    ofn.lpstrDefExt         = _T("");
    ofn.Flags               = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
         
    if(GetSaveFileName(&ofn))
    {
        int i;

        for(i=0; i<120; i++) 
        {
            LoadedFile[i] = szFileName[i];
        }
            
        LoadedFile[i] = '\0';       // terminate
        
        SetCurrentDirectory(prevc);
        return LoadedFile;
    }
    else
    {
        SetCurrentDirectory(prevc);
        return NULL;
    }
}

static TCHAR *ChooseMemcardFileProc(HWND hwnd, TCHAR * lastDir)
{
    TCHAR prevc[MAX_PATH];
    OPENFILENAME ofn;
    TCHAR szFileName[120];
    TCHAR szFileTitle[120];
    static TCHAR LoadedFile[MAX_PATH];

    GetCurrentDirectory(sizeof(prevc), prevc);

    memset(&szFileName, 0, sizeof(szFileName));
    memset(&szFileTitle, 0, sizeof(szFileTitle));

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hwnd;
    ofn.lpstrFilter         = 
        _T("GameCube Memcard Files (*.mci)\0*.mci\0")
        _T("All Files (*.*)\0*.*\0");
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFileName;
    ofn.nMaxFile            = 120;
    ofn.lpstrInitialDir     = lastDir;
    ofn.lpstrFileTitle      = szFileTitle;
    ofn.nMaxFileTitle       = 120;
    ofn.lpstrTitle          = _T("Open Memcard File\0");
    ofn.lpstrDefExt         = _T("");
    ofn.Flags               = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
         
    if(GetOpenFileName(&ofn))
    {
        int i;

        for(i=0; i<120; i++) 
        {
            LoadedFile[i] = szFileName[i];
        }
            
        LoadedFile[i] = '\0';       // terminate
          
        SetCurrentDirectory(prevc);
        return LoadedFile;
    }
    else
    {
        SetCurrentDirectory(prevc);
        return NULL;
    }
}

/*
 * Callback procedure for the choose size (of a new memcard) dialog
 */
static INT_PTR CALLBACK MemcardChooseSizeProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int index;
    TCHAR buf[256] = { 0 };

    switch(uMsg)
    {
        case WM_INITDIALOG:
            CenterChildWindow(wnd.hMainWindow, hwndDlg);
            SendMessage(hwndDlg, WM_SETICON,(WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DOLWIN_ICON)) );

            for (index = 0; index < Num_Memcard_ValidSizes; index ++) {
                int blocks, kb;
                blocks = Memcard_ValidSizes[index] / Memcard_BlockSize;
                kb = Memcard_ValidSizes[index] / 1024;
                _stprintf_s (buf, _countof(buf) - 1, _T("%d blocks  (%d Kb)"), blocks, kb);
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZES, CB_INSERTSTRING, (WPARAM)index, (LPARAM)buf);
            }
            SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZES, CB_SETCURSEL, (WPARAM)0,  (LPARAM)0);

            return TRUE;
        case WM_CLOSE:
            EndDialog(hwndDlg, -1);
            return TRUE;
        case WM_COMMAND:
            switch(wParam) {
            case IDCANCEL:
                EndDialog(hwndDlg, -1);
                return TRUE;
            case IDOK:
                index = (int)SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZES, CB_GETCURSEL, 0, 0);
                EndDialog(hwndDlg, index);

                return TRUE;
            }
            return FALSE;
        default:
            return FALSE;
    }
    return FALSE;
}

/*
 * Callback procedure for the memcard settings dialog
 */
static INT_PTR CALLBACK MemcardSettingsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR buf[MAX_PATH] = { 0 }, buf2[MAX_PATH] = { 0 }, * filename;
    size_t newsize;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            CenterChildWindow(wnd.hMainWindow, hwndDlg);
            SendMessage(hwndDlg, WM_SETICON,(WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DOLWIN_ICON)) );

            if (um_num == 0)
                SendMessage(hwndDlg, WM_SETTEXT, (WPARAM)0, lParam = (LPARAM)_T("Memcard A Settings"));
            else if (um_num == 1)
                SendMessage(hwndDlg, WM_SETTEXT, (WPARAM)0, lParam = (LPARAM)_T("Memcard B Settings"));

            if (SyncSave)
                CheckRadioButton(hwndDlg, IDC_MEMCARD_SYNCSAVE_FALSE,
                                IDC_MEMCARD_SYNCSAVE_TRUE, IDC_MEMCARD_SYNCSAVE_TRUE );
            else 
                CheckRadioButton(hwndDlg, IDC_MEMCARD_SYNCSAVE_FALSE,
                                IDC_MEMCARD_SYNCSAVE_TRUE, IDC_MEMCARD_SYNCSAVE_FALSE );

            if (Memcard_Connected[um_num])
                CheckDlgButton(hwndDlg, IDC_MEMCARD_CONNECTED, BST_CHECKED);

            _tcscpy_s(buf, sizeof(buf), Memcard_filename[um_num]);
            filename = _tcsrchr(buf, _T('\\'));
            if (filename == NULL) {
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
            }
            else {
                *filename = _T('\0');
                filename++;
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)filename);
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
            }

            if (Memcard_Connected[um_num]) {
                // TODO: Refactoring
                //_stprintf_s (buf, _countof(buf) - 1, _T("Size: %d usable blocks (%d Kb)"),
                //    (int)(memcard[um_num].size / Memcard_BlockSize - 5), (int)(memcard[um_num].size / 1024));
                //SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZEDESC, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
            }
            else {
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZEDESC, WM_SETTEXT,  (WPARAM)0, (LPARAM)_T("Not connected"));
            }

            um_filechanged = FALSE;
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;
        case WM_COMMAND:
            switch(wParam) {
            case IDC_MEMCARD_NEW:
                newsize = DialogBox(GetModuleHandle(NULL),
                        MAKEINTRESOURCE(IDD_MEMCARD_CHOOSESIZE),
                        hwndDlg,
                        MemcardChooseSizeProc);
                if (newsize == -1) return TRUE;
                newsize = Memcard_ValidSizes[newsize];

                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_GETTEXT,  (WPARAM)256, (LPARAM)(LPCTSTR)buf);
                filename = NewMemcardFileProc(hwndDlg, buf);
                if (filename == NULL) return TRUE;
                _tcscpy_s(buf, _countof(buf) - 1, filename );

                /* create the file */
                if (MCCreateMemcardFile(filename, (uint16_t)(newsize >> 17)) == FALSE) return TRUE ;

                filename = _tcsrchr(buf, _T('\\'));
                if (filename == NULL) {
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
                }
                else {
                    *filename = '\0';
                    filename++;
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)filename);
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
                }

                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZEDESC, WM_SETTEXT,  (WPARAM)0, (LPARAM)_T("Not connected"));

                um_filechanged = TRUE;
                return TRUE;

            case IDC_MEMCARD_CHOOSEFILE:
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_GETTEXT,  (WPARAM)256, (LPARAM)(LPCTSTR)buf);
                filename = ChooseMemcardFileProc(hwndDlg, buf);
                if (filename == NULL) return TRUE;
                _tcscpy_s (buf, _countof(buf) - 1, filename );

                filename = _tcsrchr(buf, _T('\\'));
                if (filename == NULL) {
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
                }
                else {
                    *filename = '\0';
                    filename++;
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)filename);
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_SETTEXT,  (WPARAM)0, (LPARAM)(LPCTSTR)buf);
                }
                SendDlgItemMessage(hwndDlg, IDC_MEMCARD_SIZEDESC, WM_SETTEXT,  (WPARAM)0, (LPARAM)_T("Not connected"));

                um_filechanged = TRUE;
                return TRUE;
            case IDCANCEL:
                EndDialog(hwndDlg, 0);
                return TRUE;
            case IDOK:
                if (um_filechanged == TRUE )
                {
                    size_t Fnsize, Pathsize;
                    Fnsize = SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_GETTEXTLENGTH,  (WPARAM)0, (LPARAM)0);
                    Pathsize = SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_GETTEXTLENGTH,  (WPARAM)0, (LPARAM)0);

                    if (Fnsize+1 + Pathsize+1 >= sizeof (Memcard_filename[um_num])) {
                        _stprintf_s (buf, _countof(buf) - 1, _T("File full path must be less than %zi characters."), sizeof (Memcard_filename[um_num]) );
                        MessageBox(hwndDlg, buf, _T("Invalid filename"), 0);
                        return TRUE;
                    }

                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_PATH, WM_GETTEXT,  (WPARAM)(Pathsize+1), (LPARAM)(LPCTSTR)buf);
                    SendDlgItemMessage(hwndDlg, IDC_MEMCARD_FILE, WM_GETTEXT,  (WPARAM)(Fnsize+1), (LPARAM)(LPCTSTR)buf2);

                    _tcscat_s(buf, _countof(buf) - 1, _T("\\"));
                    _tcscat_s(buf, _countof(buf) - 1, buf2);
                }

                if (IsDlgButtonChecked(hwndDlg, IDC_MEMCARD_SYNCSAVE_FALSE) == BST_CHECKED  )
                    SyncSave = false;
                else
                    SyncSave = true;

                if (IsDlgButtonChecked(hwndDlg, IDC_MEMCARD_CONNECTED) == BST_CHECKED ) {
                    /* memcard is supposed to be connected */

                    Memcard_Connected[um_num] = true;
                }
                else {
                    /* memcard is supposed to be disconnected */

                    Memcard_Connected[um_num] = false;
                }

                // Writeback settings

                if (um_num == 0)
                {
                    UI::Jdi.SetConfigBool(MemcardA_Connected_Key, Memcard_Connected[0], USER_MEMCARDS);
                    UI::Jdi.SetConfigString(MemcardA_Filename_Key, Util::TcharToString(Memcard_filename[0]), USER_MEMCARDS);
                }
                else
                {
                    UI::Jdi.SetConfigBool(MemcardB_Connected_Key, Memcard_Connected[1], USER_MEMCARDS);
                    UI::Jdi.SetConfigString(MemcardB_Filename_Key, Util::TcharToString(Memcard_filename[1]), USER_MEMCARDS);
                }
                UI::Jdi.SetConfigBool(Memcard_SyncSave_Key, SyncSave, USER_MEMCARDS);

                EndDialog(hwndDlg, 0);
                return TRUE;
            }
            return FALSE;
        default:
            return FALSE;
    }
    return FALSE;
}

/*
 * Calls the memcard settings dialog
 */ 
void MemcardConfigure(int num, HWND hParent) {
    bool openedBefore;
    if ((num != 0) && (num != 1)) return ;
    um_num = num;

    DialogBox(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_MEMCARD_SETTINGS),
        hParent,
        MemcardSettingsProc);
}
