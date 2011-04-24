#include	<windows.h>
#include	<shlobj.h>
#include	<afxres.h>
#include	"resource.h"

#define		GAME_LAD_CPP
#include	"Game Lad.h"
#include	"CGameBoys.h"
#include	"Emulation.h"
#include	"Debugger.h"
#include	"Error.h"



#define		GAME_LAD_RELEASENO				3
#define		VERSION_MAJOR					1
#define		VERSION_MINOR					20



#define		MENU_WINDOW		5



KEYS		Keys = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'X', 'Z', VK_RETURN, VK_SHIFT, VK_TAB};



CGameBoy	*F4Pressed = NULL;

struct MENUHELPSTRINGS
{
	UINT	uiCommand, uiString;
} MenuHelpStrings[] = {
	ID_FILE_OPEN,					IDS_FILE_OPEN,
	ID_FILE_CLOSE,					IDS_FILE_CLOSE,
	ID_FILE_SAVESTATE,				IDS_FILE_SAVESTATE,
	ID_FILE_LOADSTATE,				IDS_FILE_LOADSTATE,
	ID_FILE_SAVESTATEAS,			IDS_FILE_SAVESTATEAS,
	ID_FILE_LOADSTATEAS,			IDS_FILE_LOADSTATEAS,
	ID_FILE_STATESLOT_0,			IDS_FILE_STATESLOT_0,
	ID_FILE_STATESLOT_1,			IDS_FILE_STATESLOT_1,
	ID_FILE_STATESLOT_2,			IDS_FILE_STATESLOT_2,
	ID_FILE_STATESLOT_3,			IDS_FILE_STATESLOT_3,
	ID_FILE_STATESLOT_4,			IDS_FILE_STATESLOT_4,
	ID_FILE_STATESLOT_5,			IDS_FILE_STATESLOT_5,
	ID_FILE_STATESLOT_6,			IDS_FILE_STATESLOT_6,
	ID_FILE_STATESLOT_7,			IDS_FILE_STATESLOT_7,
	ID_FILE_STATESLOT_8,			IDS_FILE_STATESLOT_8,
	ID_FILE_STATESLOT_9,			IDS_FILE_STATESLOT_9,
	ID_FILE_SAVEBATTERY,			IDS_FILE_SAVEBATTERY,
	ID_FILE_LOADBATTERY,			IDS_FILE_LOADBATTERY,
	ID_FILE_SAVEBATTERYAS,			IDS_FILE_SAVEBATTERYAS,
	ID_FILE_CLEARBATTERY,			IDS_FILE_CLEARBATTERY,
	ID_FILE_EXIT,					IDS_FILE_EXIT,
	ID_EDIT_GOTO,					IDS_EDIT_GOTO,
	ID_VIEW_DISASSEMBLY,			IDS_VIEW_DISASSEMBLY,
	ID_VIEW_MEMORY,					IDS_VIEW_MEMORY,
	ID_VIEW_REGISTERS,				IDS_VIEW_REGISTERS,
	ID_VIEW_HARDWARE,				IDS_VIEW_HARDWARE,
	ID_VIEW_PALETTES,				IDS_VIEW_PALETTES,
	ID_VIEW_TILEMAP,				IDS_VIEW_TILEMAP,
	ID_VIEW_TILES,					IDS_VIEW_TILES,
	ID_VIEW_STATUSBAR,				IDS_VIEW_STATUSBAR,
	ID_VIEW_ZOOM_100,				IDS_VIEW_ZOOM_100,
	ID_VIEW_ZOOM_200,				IDS_VIEW_ZOOM_200,
	ID_VIEW_ZOOM_300,				IDS_VIEW_ZOOM_300,
	ID_VIEW_ZOOM_400,				IDS_VIEW_ZOOM_400,
	ID_VIEW_FRAMESKIP_0,			IDS_VIEW_FRAMESKIP_0,
	ID_VIEW_FRAMESKIP_1,			IDS_VIEW_FRAMESKIP_1,
	ID_VIEW_FRAMESKIP_2,			IDS_VIEW_FRAMESKIP_2,
	ID_VIEW_FRAMESKIP_3,			IDS_VIEW_FRAMESKIP_3,
	ID_VIEW_FRAMESKIP_4,			IDS_VIEW_FRAMESKIP_4,
	ID_VIEW_FRAMESKIP_5,			IDS_VIEW_FRAMESKIP_5,
	ID_VIEW_FRAMESKIP_6,			IDS_VIEW_FRAMESKIP_6,
	ID_VIEW_FRAMESKIP_7,			IDS_VIEW_FRAMESKIP_7,
	ID_VIEW_FRAMESKIP_8,			IDS_VIEW_FRAMESKIP_8,
	ID_VIEW_FRAMESKIP_9,			IDS_VIEW_FRAMESKIP_9,
	ID_MEMORY_ROM,					IDS_MEMORY_ROM,
	ID_MEMORY_VBK_BANK0,			IDS_MEMORY_VBK_BANK0,
	ID_MEMORY_VBK_BANK1,			IDS_MEMORY_VBK_BANK1,
	ID_MEMORY_RAM_BANK0,			IDS_MEMORY_RAM_BANK0,
	ID_MEMORY_RAM_BANK1,			IDS_MEMORY_RAM_BANK1,
	ID_MEMORY_RAM_BANK2,			IDS_MEMORY_RAM_BANK2,
	ID_MEMORY_RAM_BANK3,			IDS_MEMORY_RAM_BANK3,
	ID_MEMORY_RAM_BANK4,			IDS_MEMORY_RAM_BANK4,
	ID_MEMORY_RAM_BANK5,			IDS_MEMORY_RAM_BANK5,
	ID_MEMORY_RAM_BANK6,			IDS_MEMORY_RAM_BANK6,
	ID_MEMORY_RAM_BANK7,			IDS_MEMORY_RAM_BANK7,
	ID_MEMORY_RAM_BANK8,			IDS_MEMORY_RAM_BANK8,
	ID_MEMORY_RAM_BANK9,			IDS_MEMORY_RAM_BANK9,
	ID_MEMORY_RAM_BANK10,			IDS_MEMORY_RAM_BANK10,
	ID_MEMORY_RAM_BANK11,			IDS_MEMORY_RAM_BANK11,
	ID_MEMORY_RAM_BANK12,			IDS_MEMORY_RAM_BANK12,
	ID_MEMORY_RAM_BANK13,			IDS_MEMORY_RAM_BANK13,
	ID_MEMORY_RAM_BANK14,			IDS_MEMORY_RAM_BANK14,
	ID_MEMORY_RAM_BANK15,			IDS_MEMORY_RAM_BANK15,
	ID_MEMORY_SVBK_BANK1,			IDS_MEMORY_SVBK_BANK1,
	ID_MEMORY_SVBK_BANK2,			IDS_MEMORY_SVBK_BANK2,
	ID_MEMORY_SVBK_BANK3,			IDS_MEMORY_SVBK_BANK3,
	ID_MEMORY_SVBK_BANK4,			IDS_MEMORY_SVBK_BANK4,
	ID_MEMORY_SVBK_BANK5,			IDS_MEMORY_SVBK_BANK5,
	ID_MEMORY_SVBK_BANK6,			IDS_MEMORY_SVBK_BANK6,
	ID_MEMORY_SVBK_BANK7,			IDS_MEMORY_SVBK_BANK7,
	ID_EMULATION_STARTDEBUG,		IDS_EMULATION_STARTDEBUG,
	ID_EMULATION_EXECUTE,			IDS_EMULATION_EXECUTE,
	ID_EMULATION_STOP,				IDS_EMULATION_STOP,
	ID_EMULATION_STEPINTO,			IDS_EMULATION_STEPINTO,
	ID_EMULATION_STEPOVER,			IDS_EMULATION_STEPOVER,
	ID_EMULATION_STEPOUT,			IDS_EMULATION_STEPOUT,
	ID_EMULATION_RUNTOCURSOR,		IDS_EMULATION_RUNTOCURSOR,
	ID_EMULATION_SETNEXTSTATEMENT,	IDS_EMULATION_SETNEXTSTATEMENT,
	ID_EMULATION_TOGGLEBREAKPOINT,	IDS_EMULATION_TOGGLEBREAKPOINT,
	ID_EMULATION_RESET,				IDS_EMULATION_RESET,
	ID_TOOLS_OPTIONS,				IDS_TOOLS_OPTIONS,
	ID_TOOLS_SOUNDENABLED,			IDS_TOOLS_SOUNDENABLED,
	ID_WINDOW_NEXT,					IDS_WINDOW_NEXT,
	ID_WINDOW_PREVIOUS,				IDS_WINDOW_PREVIOUS,
	ID_WINDOW_CASCADE,				IDS_WINDOW_CASCADE,
	ID_WINDOW_TILEHORIZONTALLY,		IDS_WINDOW_TILEHORIZONTALLY,
	ID_WINDOW_TILEVERTICALLY,		IDS_WINDOW_TILEVERTICALLY,
	ID_HELP_ABOUT,					IDS_HELP_ABOUT,
	0, 0};



BOOL		StatusReady = true;

void SetStatus(char *szStatusText, DWORD dwStatus)
{
	if (dwStatus != SF_F4PRESSED)
	{
		F4Pressed = NULL;
	}

	switch (dwStatus)
	{
	case SF_MESSAGE:
		SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)szStatusText);
		StatusReady = true;
		return;

	case SF_CLEAR:
		SendMessage(hStatusWnd, WM_SETTEXT, 0, NULL);
		StatusReady = true;
		return;

	case SF_READY:
		StatusReady = true;
		break;

	case SF_F4PRESSED:
		if (F4Pressed)
		{
			SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)&"Enter number of a state slot");
			StatusReady = false;
		}
		else
		{
			StatusReady = true;
		}
		break;
	}

	if (StatusReady)
	{
		SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)&"Ready");
	}
}



LPARAM CALLBACK GameBoyWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return GameBoys.WndProc((CGameBoy *)GetWindowLong(hWin, GWL_USERDATA), hWin, uMsg, wParam, lParam);
}



/*
	Displays a message box with information about the last error encoutered.
*/
void DisplayErrorMessage(HWND hWin)
{
	void	*lpMsgBuf;


	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hWin, (LPCTSTR)lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
	LocalFree(lpMsgBuf);
}



void DisplayErrorMessage(HWND hWin, DWORD dwErrCode)
{
	void	*lpMsgBuf;


	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hWin, (LPCTSTR)lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
	LocalFree(lpMsgBuf);
}



void FatalError(DWORD ErrorNo, char *pszText)
{
	if (ErrorNo == FATAL_ERROR_OUTOFMEMORY)
	{
		MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_OK | MB_ICONERROR);
	}
}



BOOL HexToNum(char *pc)
{
	if (*pc < '0')
	{
		return true;
	}
	if (*pc <= '9')
	{
		*pc = *pc - '0';
	}
	else if (*pc < 'A')
	{
		return true;
	}
	else if (*pc <= 'F')
	{
		*pc = *pc - 'A' + 10;
	}
	else if (*pc < 'a')
	{
		return true;
	}
	else if (*pc <= 'f')
	{
		*pc = *pc - 'a' + 10;
	}
	else
	{
		return true;
	}

	return false;
}



BOOL CreateKey(char *szKey, char *szData)
{
	HKEY	hKey;
	DWORD	dw, ValueType, ValueSize;
	char	szOldData[MAX_PATH + 4];


	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_EXECUTE | KEY_WRITE, NULL, &hKey, &dw))
	{
		return false;
	}
	ValueSize = sizeof(szOldData);
	if (!RegQueryValueEx(hKey, "", NULL, &ValueType, (BYTE *)&szOldData, &ValueSize))
	{
		if (ValueType == REG_SZ)
		{
			if (!strcmp(szData, szOldData))
			{
				return false;
			}
		}
	}
	if (RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)szData, strlen(szData) + 1))
	{
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);

	return true;
}



void RegisterGbFileType()
{
	char	szFilename[MAX_PATH + 4];
	BOOL	Change;


	GetModuleFileName(NULL, (char *)&szFilename[1], MAX_PATH);
	szFilename[0] = '\"';
	strcat(szFilename, "\"");

	Change = false;
	Change |= CreateKey(".gb", "GameBoyRom");
	Change |= CreateKey(".gbc", "GameBoyRom");
	Change |= CreateKey("GameBoyRom", "Game Boy Rom");
	Change |= CreateKey("GameBoyRom\\shell", "open");
	Change |= CreateKey("GameBoyRom\\shell\\open", "");
	Change |= CreateKey("GameBoyRom\\shell\\open\\command", szFilename);
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec", "open,\"%1\"");
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec\\Application", "Game Lad");
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec\\Topic", "system");
	Change |= CreateKey("GameBoyRom\\shellex\\PropertySheetHandlers\\Game Lad", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
	strcat(szFilename, ",0");
	Change |= CreateKey("GameBoyRom\\DefaultIcon", szFilename);
	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



void RegisterGlsFileType()
{
	char	szFilename[MAX_PATH + 4];
	BOOL	Change;


	GetModuleFileName(NULL, (char *)&szFilename[1], MAX_PATH);
	szFilename[0] = '\"';
	strcat(szFilename, "\"");

	Change = false;
	Change |= CreateKey(".gls", "GameLadSaveState");
	Change |= CreateKey("GameLadSaveState", "Game Lad Save State");
	Change |= CreateKey("GameLadSaveState\\shell", "open");
	Change |= CreateKey("GameLadSaveState\\shell\\open", "");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\command", szFilename);
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec", "state,\"%1\",open,\"\"");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec\\Application", "Game Lad");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec\\Topic", "system");
	Change |= CreateKey("GameLadSaveState\\shellex\\PropertySheetHandlers\\Game Lad", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
	strcat(szFilename, ",1");
	Change |= CreateKey("GameLadSaveState\\DefaultIcon", szFilename);
	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}

	//Change |= CreateKey(hWin, "GameLadSaveState\\DefaultIcon", "%1");
	//Change |= CreateKey(hWin, "GameLadSaveState\\shellex\\IconHandler", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
}



BOOL DeleteKey(char *szKey)
{
	HKEY	hKey;


	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, &hKey))
	{
		return false;
	}
	RegCloseKey(hKey);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

	return true;
}



void UnRegisterGbFileType()
{
	BOOL	Change = false;


	Change |= DeleteKey(".gb");
	Change |= DeleteKey(".gbc");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec\\Application");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec\\Topic");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\command");
	Change |= DeleteKey("GameBoyRom\\shell\\open");
	Change |= DeleteKey("GameBoyRom\\shell");
	Change |= DeleteKey("GameBoyRom\\shellex\\PropertySheetHandlers\\Game Lad");
	Change |= DeleteKey("GameBoyRom\\shellex\\PropertySheetHandlers");
	Change |= DeleteKey("GameBoyRom\\shellex");
	Change |= DeleteKey("GameBoyRom\\DefaultIcon");
	Change |= DeleteKey("GameBoyRom");

	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



void UnRegisterGlsFileType()
{
	BOOL	Change = false;


	Change |= DeleteKey(".gls");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec\\Application");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec\\Topic");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\command");
	Change |= DeleteKey("GameLadSaveState\\shell\\open");
	Change |= DeleteKey("GameLadSaveState\\shell");
	Change |= DeleteKey("GameLadSaveState\\shellex\\PropertySheetHandlers\\Game Lad");
	Change |= DeleteKey("GameLadSaveState\\shellex\\PropertySheetHandlers");
	Change |= DeleteKey("GameLadSaveState\\shellex");
	Change |= DeleteKey("GameLadSaveState\\DefaultIcon");
	Change |= DeleteKey("GameLadSaveState");

	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



#define		DDEGB_DMG			0x00000001
#define		DDEGB_GBC			0x00000002
#define		DDEGB_LOAD			0x00000004
#define		DDEGB_DEBUG			0x00000008
#define		DDEGB_EXECUTE		0x00000010
#define		DDEGB_NOBATTERY		0x00000020
#define		DDEGB_BATTERY		0x00000040
#define		DDEGB_STATE			0x00000080
#define		DDEGB_NOSTATE		0x00000100

struct DDEGAMEBOY
{
	DWORD		Flags;
	char		szROM[MAX_PATH];
	char		szBattery[MAX_PATH];
	char		szState[MAX_PATH];
};



HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	char		*pBuffer;
	DDEGAMEBOY	*DDEGameBoy;
	DWORD		Size, FirstCharacter;


	switch (uType)
	{
	case XTYP_CONNECT:
		if (hsz1 == hDdeTopicString && hsz2 == hDdeServiceString)
		{
			return (HDDEDATA)true;
		}
		return false;

	case XTYP_EXECUTE:
		Size = DdeGetData(hdata, NULL, 0, 0);
		if (!(pBuffer = new char[Size]))
		{
			return DDE_FNOTPROCESSED;
		}
		if (!(DDEGameBoy = new DDEGAMEBOY))
		{
			delete pBuffer;
			return DDE_FNOTPROCESSED;
		}
		DdeGetData(hdata, (BYTE *)pBuffer, Size, 0);
		FirstCharacter = 0;
		DDEGameBoy->Flags = 0;
		DDEGameBoy->szROM[0] = 0;
		DDEGameBoy->szBattery[0] = 0;
		DDEGameBoy->szState[0] = 0;
		if (!strncmp(&pBuffer[FirstCharacter], "dmg,", 4))
		{
			DDEGameBoy->Flags |= DDEGB_DMG;
			FirstCharacter += 4;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "gbc,", 4))
		{
			DDEGameBoy->Flags |= DDEGB_GBC;
			FirstCharacter += 4;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "battery,", 8))
		{
			DDEGameBoy->Flags |= DDEGB_BATTERY;
			FirstCharacter += 8;
			if (pBuffer[FirstCharacter] == '\"')
			{
				Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
				strncpy(DDEGameBoy->szBattery, &pBuffer[FirstCharacter + 1], Size);
				DDEGameBoy->szBattery[Size] = 0;
				FirstCharacter += 2 + Size;
			}
			FirstCharacter++;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "nobattery,", 10))
		{
			DDEGameBoy->Flags |= DDEGB_NOBATTERY;
			FirstCharacter += 10;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "state,", 6))
		{
			DDEGameBoy->Flags |= DDEGB_STATE;
			FirstCharacter += 6;
			if (pBuffer[FirstCharacter] == '\"')
			{
				Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
				strncpy(DDEGameBoy->szState, &pBuffer[FirstCharacter + 1], Size);
				DDEGameBoy->szState[Size] = 0;
				FirstCharacter += 2 + Size;
			}
			FirstCharacter++;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "nostate,", 10))
		{
			DDEGameBoy->Flags |= DDEGB_NOSTATE;
			FirstCharacter += 8;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "open,", 5))
		{
			FirstCharacter += 5;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "load,", 5))
		{
			DDEGameBoy->Flags |= DDEGB_LOAD;
			FirstCharacter += 5;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "debug,", 6))
		{
			DDEGameBoy->Flags |= DDEGB_DEBUG;
			FirstCharacter += 6;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "execute,", 8))
		{
			DDEGameBoy->Flags |= DDEGB_EXECUTE;
			FirstCharacter += 8;
		}
		Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
		strncpy(DDEGameBoy->szROM, &pBuffer[FirstCharacter + 1], Size);
		DDEGameBoy->szROM[Size] = 0;

		delete pBuffer;
		PostMessage(hWnd, WM_APP_DDEOPENFILE, 0, (LPARAM)DDEGameBoy);
		return (HDDEDATA)DDE_FACK;
	}

	return NULL;
}



BOOL CALLBACK DialogProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT	rct;
	long	width, height;


	switch (uMsg)
	{
	case WM_COMMAND:
		if (wParam == IDOK)
		{
			EndDialog(hWin, 0);
			return true;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWin, 0);
		return true;

	case WM_INITDIALOG:
		//Center dialog
		GetWindowRect(hWin, &rct);
		width = rct.right - rct.left;
		height = rct.bottom - rct.top;
		GetWindowRect(hWnd, &rct);
		rct.right -= rct.left; //width
		rct.bottom -= rct.top; //height
		MoveWindow(hWin, rct.left + ((rct.right - width) >> 1), rct.top + ((rct.bottom - height) >> 1), width, height, true);
		return true;
	}

	return false;
}



HWND	hTitleCaption, hTitle, hAutoStart, /*hLoadDebug,*/ hLoadBattery, hGBType;
BOOL	AutoStart, /*LoadDebug,*/ LoadBattery;
BYTE	GBType;

UINT CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	char			Filename[MAX_PATH], Buffer[0x50];
	HANDLE			hFile;
	DWORD			nBytes, FileSize;


	switch (uiMsg)
	{
	/*case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		}
		break;*/

	case WM_NOTIFY:
		switch (((OFNOTIFY *)lParam)->hdr.code)
		{
		case CDN_FILEOK:
			switch (SendMessage(hAutoStart, CB_GETCURSEL, 0, 0))
			{
			case 0:
				AutoStart = 0;
				break;

			case 1:
				AutoStart = AUTOSTART_DEBUG;
				break;

			case 2:
				AutoStart = AUTOSTART_EXECUTE;
				break;
			}
			/*LoadDebug = false;
			if (SendMessage(hLoadDebug, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				LoadDebug = true;
			}*/
			LoadBattery = false;
			if (SendMessage(hLoadBattery, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				LoadBattery = true;
			}
			GBType = (SendMessage(hGBType, CB_GETCURSEL, 0, 0) == 1) ? GB_COLOR : 0;
			break;

		case CDN_SELCHANGE:
			SendMessage(hTitle, WM_SETTEXT, 0, NULL);
			if (!SendMessage(GetParent(hdlg), CDM_GETFILEPATH, sizeof(Filename), (long)&Filename))
			{
				break;
			}
			if ((hFile = CreateFile(Filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				break;
			}
			if (SetFilePointer(hFile, 0x100, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				CloseHandle(hFile);
				break;
			}
			if (!ReadFile(hFile, Buffer, 0x50, &nBytes, NULL))
			{
				CloseHandle(hFile);
				break;
			}
			FileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
			if (nBytes != 0x50 || (RomSize(Buffer[0x48]) + 1) * 16384U != FileSize)
			{
				break;
			}
			/*if (memcmp(&Buffer[0x4], NintendoGraphic, 48))
			{
				break;
			}*/
			if (Buffer[0x43] & 0x80)
			{
				Buffer[0x43] = 0;
			}
			Buffer[0x44] = 0;
			SendMessage(hTitle, WM_SETTEXT, 0, (long)&Buffer[0x34]);
			break;
		}
		break;

	//case WM_SIZE:
		//return 0;

	case WM_INITDIALOG:
		SetWindowLong(hdlg, GWL_STYLE, GetWindowLong(hdlg, GWL_STYLE) | WS_CLIPSIBLINGS);
		MoveWindow(hdlg, 0, 0, 400, 43, true);
		hTitleCaption = CreateWindow("STATIC", "Title:", WS_CHILD | WS_VISIBLE, 7, 0, 50, 16, hdlg, NULL, hInstance, NULL);
		SendMessage(hTitleCaption, WM_SETFONT, (WPARAM)hFont, true);
		hTitle = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 50, 0, 130, 16, hdlg, NULL, hInstance, NULL);
		SendMessage(hTitle, WM_SETFONT, (WPARAM)hFont, true);

		hGBType = CreateWindow("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, 7, 20, 180, 200, hdlg, NULL, hInstance, NULL);
		SendMessage(hGBType, WM_SETFONT, (WPARAM)hFont, true);
		SendMessage(hGBType, CB_ADDSTRING, 0, (LPARAM)"Game Boy");
		SendMessage(hGBType, CB_ADDSTRING, 0, (LPARAM)"Game Boy Color");
		SendMessage(hGBType, CB_SETCURSEL, Settings.GBType == GB_COLOR ? 1 : 0, 0);


		hLoadBattery = CreateWindow("BUTTON", "Load battery if possible.", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 205, 0, 180, 16, hdlg, NULL, hInstance, NULL);
		if (Settings.AutoLoadBattery)
		{
			SendMessage(hLoadBattery, BM_SETCHECK, BST_CHECKED, 0);
		}
		SendMessage(hLoadBattery, WM_SETFONT, (WPARAM)hFont, true);

		hAutoStart = CreateWindow("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, 205, 20, 180, 200, hdlg, NULL, hInstance, NULL);
		SendMessage(hAutoStart, WM_SETFONT, (WPARAM)hFont, true);
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)"Stopped");
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)"Start Debug");
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)"Execute");
		switch (Settings.AutoStart)
		{
		case 0:
			SendMessage(hAutoStart, CB_SETCURSEL, 0, 0);
			break;

		case AUTOSTART_DEBUG:
			SendMessage(hAutoStart, CB_SETCURSEL, 1, 0);
			break;

		case AUTOSTART_EXECUTE:
			SendMessage(hAutoStart, CB_SETCURSEL, 2, 0);
			break;
		}

		/*hLoadDebug = CreateWindow("BUTTON", "Load debugging info if possible.", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 7, 18, 200, 16, hdlg, NULL, hInstance, NULL);
		if (Settings.AutoLoadDebug)
		{
			SendMessage(hLoadDebug, BM_SETCHECK, BST_CHECKED, 0);
		}
		SendMessage(hLoadDebug, WM_SETFONT, (long)hFont, true);*/
	}

	SetWindowLong(GetParent(hdlg), DWL_MSGRESULT, 0);
	return 0;
}



struct CHARACTERS
{
	BYTE	Vk;
	char	Name[16];
} const Characters[70] ={
							'0', "0",
							'1', "1",
							'2', "2",
							'3', "3",
							'4', "4",
							'5', "5",
							'6', "6",
							'7', "7",
							'8', "8",
							'9', "9",
							'A', "A",
							VK_APPS, "Application",
							'B', "B",
							VK_BACK, "Backspace",
							'C', "C",
							VK_CONTROL, "Control",
							'D', "D",
							VK_DELETE, "Delete",
							VK_DOWN, "Down Arrow",
							'E', "E",
							VK_END, "End",
							VK_RETURN, "Enter",
							'F', "F",
							'G', "G",
							'H', "H",
							VK_HOME, "Home",
							'I', "I",
							VK_INSERT, "Insert",
							'J', "J",
							'K', "K",
							'L', "L",
							VK_LEFT, "Left Arrow",
							'M', "M",
							'N', "N",
							VK_NUMLOCK, "Num Lock",
							VK_NUMPAD0, "Numpad 0",
							VK_NUMPAD1, "Numpad 1",
							VK_NUMPAD2, "Numpad 2",
							VK_NUMPAD3, "Numpad 3",
							VK_NUMPAD4, "Numpad 4",
							VK_NUMPAD5, "Numpad 5",
							VK_NUMPAD6, "Numpad 6",
							VK_NUMPAD7, "Numpad 7",
							VK_NUMPAD8, "Numpad 8",
							VK_NUMPAD9, "Numpad 9",
							VK_ADD, "Numpad Add",
							VK_DECIMAL, "Numpad Decimal",
							VK_DIVIDE, "Numpad Divide",
							VK_MULTIPLY, "Numpad Multiply",
							VK_SUBTRACT, "Numpad Subtract",
							'O', "O",
							'P', "P",
							VK_NEXT, "Page Down",
							VK_PRIOR, "Page Up",
							VK_PAUSE, "Pause",
							'Q', "Q",
							'R', "R",
							VK_RIGHT, "Right Arrow",
							'S', "S",
							VK_SHIFT, "Shift",
							VK_SPACE, "Space",
							'T', "T",
							VK_TAB, "Tab",
							'U', "U",
							VK_UP, "Up Arrow",
							'V', "V",
							'W', "W",
							'X', "X",
							'Y', "Y",
							'Z', "Z",
						};



BOOL CALLBACK KeysEnumChildProc(HWND hWin, LPARAM lParam)
{
	DWORD		VK, *pKey;
	char		SelectedKey[16];


	switch (GetWindowLong(hWin, GWL_ID))
	{
	case IDC_UP:
		pKey = &Keys.Up;
		break;
	case IDC_DOWN:
		pKey = &Keys.Down;
		break;
	case IDC_LEFT:
		pKey = &Keys.Left;
		break;
	case IDC_RIGHT:
		pKey = &Keys.Right;
		break;
	case IDC_A:
		pKey = &Keys.A;
		break;
	case IDC_B:
		pKey = &Keys.B;
		break;
	case IDC_START:
		pKey = &Keys.Start;
		break;
	case IDC_SELECT:
		pKey = &Keys.Select;
		break;
	case IDC_FASTFORWARD:
		pKey = &Keys.FastForward;
		break;
	}

	if (!lParam)
	{
		SendMessage(hWin, CB_GETLBTEXT, SendMessage(hWin, CB_GETCURSEL, 0, 0), (LPARAM)SelectedKey);
	}

	switch (GetWindowLong(hWin, GWL_ID))
	{
	case IDC_UP:
	case IDC_DOWN:
	case IDC_LEFT:
	case IDC_RIGHT:
	case IDC_A:
	case IDC_B:
	case IDC_START:
	case IDC_SELECT:
	case IDC_FASTFORWARD:
		VK = 0;
		while (VK < 70)
		{
			if (lParam)
			{
				SendMessage(hWin, CB_ADDSTRING, 0, (LPARAM)Characters[VK].Name);
				if (Characters[VK].Vk == *pKey)
				{
					SendMessage(hWin, CB_SETCURSEL, SendMessage(hWin, CB_GETCOUNT, 0, 0) - 1, 0);
				}
			}
			else
			{
				if (!strcmp(Characters[VK].Name, SelectedKey))
				{
					*pKey = Characters[VK].Vk;
					return true;
				}
			}
			VK++;
		}
	}

	return true;
}



BOOL CALLBACK KeyOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
		}
		break;

	case WM_INITDIALOG:
		EnumChildWindows(hWin, KeysEnumChildProc, true);
		return true;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->code == PSN_APPLY)
		{
			EnumChildWindows(hWin, KeysEnumChildProc, false);
		}
	}

	return false;
}



BOOL CALLBACK GeneralOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_REGISTERGBROM:
		case IDC_REGISTERSAVESTATE:
		case IDC_AUTOLOADBATTERY:
		case IDC_AUTOLOADSTATE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;

		case IDC_SAVEBATTERY:
		case IDC_SAVESTATE:
		case IDC_GBTYPE:
		case IDC_AUTOSTART:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;

		case IDC_FRAMESKIP:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		break;

	case WM_NOTIFY:
		if (wParam == IDC_FRAMESKIPSPIN)
		{
			if (((NMUPDOWN *)lParam)->hdr.code == UDN_DELTAPOS)
			{
				SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_GETTEXT, sizeof(NumBuffer), (LPARAM)&NumBuffer);
				if (atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta < 0)
				{
					SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)&"0");
				}
				else
				{
					if (atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta > 9)
					{
						SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)&"9");
					}
					else
					{
						SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)itoa(atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta, NumBuffer, 10));
					}
				}
				SetWindowLong(hWin, DWL_MSGRESULT, false);
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		if (((NMHDR *)lParam)->code == PSN_APPLY)
		{
			if (Settings.GbFile = SendDlgItemMessage(hWin, IDC_REGISTERGBROM, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false)
			{
				RegisterGbFileType();
			}
			else
			{
				UnRegisterGbFileType();
			}
			if (Settings.GlsFile = SendDlgItemMessage(hWin, IDC_REGISTERSAVESTATE, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false)
			{
				RegisterGlsFileType();
			}
			else
			{
				UnRegisterGlsFileType();
			}

			Settings.AutoLoadBattery = SendDlgItemMessage(hWin, IDC_AUTOLOADBATTERY, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			Settings.AutoLoadState = SendDlgItemMessage(hWin, IDC_AUTOLOADSTATE, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

			Settings.SaveBattery = (BYTE)SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_GETCURSEL, 0, 0);
			Settings.SaveState = (BYTE)SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_GETCURSEL, 0, 0);

			SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_GETTEXT, sizeof(NumBuffer), (LPARAM)&NumBuffer);
			Settings.FrameSkip = atoi(NumBuffer) < 0 ? 0 : atoi(NumBuffer) > 9 ? 9 : atoi(NumBuffer);

			Settings.GBType = SendDlgItemMessage(hWin, IDC_GBTYPE, CB_GETCURSEL, 0, 0) == 1 ? GB_COLOR : 0;

			switch (SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_GETCURSEL, 0, 0))
			{
			case 0:
				Settings.AutoStart = 0;
				break;
			case 1:
				Settings.AutoStart = AUTOSTART_DEBUG;
				break;
			case 2:
				Settings.AutoStart = AUTOSTART_EXECUTE;
				break;
			}

			return true;
		}
		break;

	/*case WM_HELP:
		if (((HELPINFO *)lParam)->iCtrlId == IDC_REGISTERGBROM)
		{
		}
		break;*/

	case WM_INITDIALOG:
		SendDlgItemMessage(hWin, IDC_REGISTERGBROM, BM_SETCHECK, Settings.GbFile ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hWin, IDC_REGISTERSAVESTATE, BM_SETCHECK, Settings.GlsFile ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hWin, IDC_AUTOLOADBATTERY, BM_SETCHECK, Settings.AutoLoadBattery ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hWin, IDC_AUTOLOADSTATE, BM_SETCHECK, Settings.AutoLoadState ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)"No");
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)"Yes");
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)"Prompt");
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_SETCURSEL, Settings.SaveBattery, 0);
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)"No");
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)"Yes");
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)"Prompt");
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_SETCURSEL, Settings.SaveState, 0);

		SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)itoa(Settings.FrameSkip, NumBuffer, 10));

		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_ADDSTRING, 0, (long)"Game Boy");
		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_ADDSTRING, 0, (long)"Game Boy Color");
		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_SETCURSEL, Settings.GBType == GB_COLOR ? 1 : 0, 0);

		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)"Stopped");
		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)"Debug");
		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)"Execute");
		switch (Settings.AutoStart)
		{
		case 0:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 0, 0);
			break;
		case AUTOSTART_DEBUG:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 1, 0);
			break;
		case AUTOSTART_EXECUTE:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 2, 0);
			break;
		}
		break;
	}

	return false;
}



LRESULT CALLBACK WndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CLIENTCREATESTRUCT	ccs;
	OPENFILENAME		of;
	char				Filename[MAX_PATH], szBuffer[0x100];
	DWORD				ItemNo, dw;
	CGameBoy			*pGameBoy;
	MENUITEMINFO		mii;
	HWND				hTempWnd;
	PROPSHEETPAGE		psp[2];
	PROPSHEETHEADER		psh;
	RECT				rct;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_OPEN:
			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWin;
			of.lpstrFilter = "Game Boy Roms (*.gb; *.gbc)\0*.GB;*.GBC\0All files (*.*)\0*.*\0";
			of.nFilterIndex = 1;
			of.lpstrFile = Filename;
			of.nMaxFile = sizeof(Filename);
			of.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
			of.lpfnHook = OFNHookProc;
			Filename[0] = 0;
			if (!GetOpenFileName(&of))
			{
				return 0;
			}
			GameBoys.NewGameBoy(Filename, NULL, LoadBattery ? "" : NULL, GBType, AutoStart);
			return 0;

		case ID_FILE_CLOSE:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
			{
				SendMessage(hTempWnd, WM_CLOSE, 0, 0);
			}
			return 0;

		case ID_FILE_SAVESTATE:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->SaveState();
			}
			return 0;

		case ID_FILE_LOADSTATE:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->LoadState();
			}
			return 0;

		case ID_FILE_SAVESTATEAS:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->SaveStateAs();
			}
			return 0;

		case ID_FILE_LOADSTATEAS:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->LoadStateAs();
			}
			return 0;

		case ID_FILE_STATESLOT_0:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(0);
			}
			return 0;
		case ID_FILE_STATESLOT_1:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(1);
			}
			return 0;
		case ID_FILE_STATESLOT_2:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(2);
			}
			return 0;
		case ID_FILE_STATESLOT_3:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(3);
			}
			return 0;
		case ID_FILE_STATESLOT_4:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(4);
			}
			return 0;
		case ID_FILE_STATESLOT_5:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(5);
			}
			return 0;
		case ID_FILE_STATESLOT_6:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(6);
			}
			return 0;
		case ID_FILE_STATESLOT_7:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(7);
			}
			return 0;
		case ID_FILE_STATESLOT_8:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(8);
			}
			return 0;
		case ID_FILE_STATESLOT_9:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetStateSlot(9);
			}
			return 0;

		case ID_SAVESTATESLOT:
			F4Pressed = GameBoys.GetActive();
			SetStatus(NULL, SF_F4PRESSED);
			return 0;

		case ID_FILE_LOADBATTERY:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
				if (!pGameBoy->SaveBattery(true, false))
				{
					ZeroMemory(&of, sizeof(of));
					of.lStructSize = sizeof(of);
					of.hwndOwner = hWin;
					of.lpstrFilter = "Game Boy Battery Ram (*.sav)\0*.SAV\0All files (*.*)\0*.*\0";
					of.nFilterIndex = 1;
					of.lpstrFile = Filename;
					of.nMaxFile = sizeof(Filename);
					of.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
					of.lpfnHook = OFNHookProc;
					pGameBoy->GetBatteryFilename(Filename);
					if (GetOpenFileName(&of))
					{
						pGameBoy->LoadBattery(Filename);
					}
				}
				pGameBoy->Resume();
			}
			return 0;

		case ID_FILE_SAVEBATTERY:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
				pGameBoy->SaveBattery(false, false);
				pGameBoy->Resume();
			}
			return 0;

		case ID_FILE_SAVEBATTERYAS:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
				pGameBoy->SaveBattery(false, true);
				pGameBoy->Resume();
			}
			return 0;

		case ID_FILE_CLEARBATTERY:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
				if (!pGameBoy->SaveBattery(true, false))
				{
					pGameBoy->LoadBattery(NULL);
				}
				pGameBoy->Resume();
			}
			return 0;

		case ID_FILE_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;

		case ID_EDIT_GOTO:
			SendMessage((HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0), uMsg, wParam, lParam);
			return 0;

		case ID_VIEW_REGISTERS:
			if (!hRegisters)
			{
				if (!(hRegisters = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Registers", "Registers", WS_VISIBLE | WS_CAPTION | WS_BORDER, Registers.x, Registers.y, Registers.Width, Registers.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hRegisters, 0);
			}
			return 0;

		case ID_VIEW_DISASSEMBLY:
			if (!hDisAsm)
			{
				if (!(hDisAsm = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "DisAsm", "Disassembly", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL /*| WS_HSCROLL*/, DisAsm.x, DisAsm.y, DisAsm.Width, DisAsm.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
			}
			return 0;

		case ID_VIEW_MEMORY:
			if (!hMemory)
			{
				if (!(hMemory = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Memory", "Memory", WS_VISIBLE | WS_CAPTION | WS_BORDER, Memory.x, Memory.y, Memory.Width, Memory.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hMemory, 0);
			}
			return 0;

		case ID_VIEW_TILES:
			if (!hTiles)
			{
				if (!(hTiles = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Tiles", "Tiles", WS_VISIBLE | WS_CAPTION | WS_BORDER, Tiles.x, Tiles.y, Tiles.Width, Tiles.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hTiles, 0);
			}
			return 0;

		case ID_VIEW_TILEMAP:
			if (!hTileMap)
			{
				if (!(hTileMap = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, "TileMap", "Tile Map", WS_VISIBLE | WS_CAPTION | WS_BORDER, TileMap.x, TileMap.y, TileMap.Width, TileMap.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hTileMap, 0);
			}
			return 0;

		case ID_VIEW_PALETTES:
			if (!hPalettes)
			{
				if (!(hPalettes = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Palettes", "Palettes", WS_VISIBLE | WS_CAPTION | WS_BORDER /*| WS_VSCROLL | WS_HSCROLL*/, Palettes.x, Palettes.y, Palettes.Width, Palettes.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hPalettes, 0);
			}
			return 0;

		case ID_VIEW_HARDWARE:
			if (!hHardware)
			{
				if (!(hHardware = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Hardware", "Hardware", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL | WS_HSCROLL, Hardware.x, Hardware.y, Hardware.Width, Hardware.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hHardware, 0);
			}
			return 0;

		case ID_VIEW_STATUSBAR:
			if (GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE)
			{
				ShowWindow(hStatusWnd, false);
				StatusBarAppearance = false;
			}
			else
			{
				ShowWindow(hStatusWnd, true);
				StatusBarAppearance = true;
			}
			GetClientRect(hWin, &rct);
			SendMessage(hWin, WM_SIZE, 0, (rct.bottom << 16) | (rct.right & 0xFFFF));
			return 0;

		case ID_VIEW_ZOOM_100:
		case ID_VIEW_ZOOM_200:
		case ID_VIEW_ZOOM_300:
		case ID_VIEW_ZOOM_400:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
			{
				SendMessage(hTempWnd, WM_COMMAND, wParam, lParam);
			}
			return 0;

		case ID_VIEW_FRAMESKIP_0:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(0);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_1:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(1);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_2:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(2);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_3:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(3);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_4:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(4);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_5:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(5);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_6:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(6);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_7:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(7);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_8:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(8);
			}
			return 0;
		case ID_VIEW_FRAMESKIP_9:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->SetFrameSkip(9);
			}
			return 0;

		case ID_EMULATION_STARTDEBUG:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->StartDebug();
			}
			return 0;

		case ID_EMULATION_EXECUTE:
			if (pGameBoy = GameBoys.GetActive())
			{
				return pGameBoy->Execute();
			}
			return 0;

		case ID_EMULATION_STOP:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
			}
			return 0;

		case ID_EMULATION_STEPINTO:
		case ID_EMULATION_STEPOVER:
		case ID_EMULATION_STEPOUT:
		case ID_EMULATION_RUNTOCURSOR:
		case ID_EMULATION_TOGGLEBREAKPOINT:
		case ID_EMULATION_SETNEXTSTATEMENT:
			if (hDisAsm)
			{
				SendMessage(hDisAsm, WM_COMMAND, wParam, lParam);
			}
			return 0;

		case ID_MEMORY_ROM:
		case ID_MEMORY_VBK_BANK0:
		case ID_MEMORY_VBK_BANK1:
		case ID_MEMORY_RAM_BANK0:
		case ID_MEMORY_RAM_BANK1:
		case ID_MEMORY_RAM_BANK2:
		case ID_MEMORY_RAM_BANK3:
		case ID_MEMORY_RAM_BANK4:
		case ID_MEMORY_RAM_BANK5:
		case ID_MEMORY_RAM_BANK6:
		case ID_MEMORY_RAM_BANK7:
		case ID_MEMORY_RAM_BANK8:
		case ID_MEMORY_RAM_BANK9:
		case ID_MEMORY_RAM_BANK10:
		case ID_MEMORY_RAM_BANK11:
		case ID_MEMORY_RAM_BANK12:
		case ID_MEMORY_RAM_BANK13:
		case ID_MEMORY_RAM_BANK14:
		case ID_MEMORY_RAM_BANK15:
		case ID_MEMORY_SVBK_BANK1:
		case ID_MEMORY_SVBK_BANK2:
		case ID_MEMORY_SVBK_BANK3:
		case ID_MEMORY_SVBK_BANK4:
		case ID_MEMORY_SVBK_BANK5:
		case ID_MEMORY_SVBK_BANK6:
		case ID_MEMORY_SVBK_BANK7:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0))
			{
				return SendMessage(hTempWnd, uMsg, wParam, lParam);
			}
			break;

		case ID_EMULATION_RESET:
			if (pGameBoy = GameBoys.GetActive())
			{
				pGameBoy->Stop();
				pGameBoy->Reset();
				pGameBoy->Resume();
			}
			return 0;

		case ID_TOOLS_OPTIONS:
			psp[0].dwSize = sizeof(psp[0]);
			psp[0].dwFlags = 0;
			psp[0].hInstance = hInstance;
			psp[0].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_KEYS);
			psp[0].pfnDlgProc = KeyOptionsDlgProc;
			psp[1].dwSize = sizeof(psp[1]);
			psp[1].dwFlags = 0;
			psp[1].hInstance = hInstance;
			psp[1].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_GENERAL);
			psp[1].pfnDlgProc = GeneralOptionsDlgProc;
			psh.dwSize = sizeof(psh);
			psh.dwFlags = PSH_PROPSHEETPAGE;
			psh.hwndParent = hWin;
			psh.hInstance = hInstance;
			psh.pszCaption = "Options";
			psh.nPages = 2;
			psh.nStartPage = 0;
			psh.ppsp = psp;
			PropertySheet(&psh);
			return 0;

		case ID_TOOLS_SOUNDENABLED:
			if (!Settings.SoundEnabled)
			{
				GameBoys.EnableSound();
			}
			else
			{
				GameBoys.CloseSound();
			}
			return 0;

		case ID_WINDOW_NEXT:
			return SendMessage(hClientWnd, WM_MDINEXT, NULL, 0);
		case ID_WINDOW_PREVIOUS:
			return SendMessage(hClientWnd, WM_MDINEXT, NULL, 1);

		case ID_WINDOW_CASCADE:
			return SendMessage(hClientWnd, WM_MDICASCADE, 0, 0);
		case ID_WINDOW_TILEHORIZONTALLY:
			return SendMessage(hClientWnd, WM_MDITILE, MDITILE_HORIZONTAL, 0);
		case ID_WINDOW_TILEVERTICALLY:
			return SendMessage(hClientWnd, WM_MDITILE, MDITILE_VERTICAL, 0);

		case ID_HELP_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWin, DialogProc);
			return 0;
		}
		break;

	case WM_DROPFILES:
		ItemNo = 0;
		while (ItemNo < DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0))
		{
			DragQueryFile((HDROP)wParam, ItemNo, Filename, sizeof(Filename));
			GameBoys.NewGameBoy(Filename, Settings.AutoLoadState ? "" : NULL, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
			ItemNo++;
		}
		return 0;

	case WM_APP_DDEOPENFILE:
		GameBoys.NewGameBoy(((DDEGAMEBOY *)lParam)->szROM,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_STATE ? ((DDEGAMEBOY *)lParam)->szState : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_NOSTATE ? NULL : Settings.AutoLoadState ? "" : NULL,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_BATTERY ? ((DDEGAMEBOY *)lParam)->szBattery : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_NOBATTERY ? NULL : Settings.AutoLoadBattery ? "" : NULL,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_DMG ? 0 : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_GBC ? GB_COLOR : Settings.GBType,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_LOAD ? 0 : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_DEBUG ? AUTOSTART_DEBUG : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_EXECUTE ? AUTOSTART_EXECUTE : Settings.AutoStart);
		delete (DDEGAMEBOY *)lParam;
		return 0;

	case WM_APP_REFRESHDEBUG:
		MemoryFlags = 0;
		if (hMemory)
		{
			InvalidateRect(hMemory, NULL, true);
		}
		if (hRegisters)
		{
			InvalidateRect(hRegisters, NULL, true);
		}
		if (hDisAsm)
		{
			InvalidateRect(hDisAsm, NULL, true);
		}
		if (hTiles)
		{
			InvalidateRect(hTiles, NULL, true);
		}
		if (hTileMap)
		{
			InvalidateRect(hTileMap, NULL, true);
		}
		if (hPalettes)
		{
			InvalidateRect(hPalettes, NULL, true);
		}
		if (hHardware)
		{
			InvalidateRect(hHardware, NULL, true);
		}
		return 0;

	case WM_MENUSELECT:
		SetStatus(NULL, SF_CLEAR);
		if (!(HIWORD(wParam) & MF_POPUP))
		{
			ItemNo = 0;
			while (MenuHelpStrings[ItemNo].uiCommand)
			{
				if (MenuHelpStrings[ItemNo].uiCommand == LOWORD(wParam))
				{
					if (LoadString(hStringInstance, MenuHelpStrings[ItemNo].uiString, szBuffer, sizeof(szBuffer)))
					{
						SetStatus(szBuffer, SF_MESSAGE);
					}
					break;
				}
				ItemNo++;
			}
		}
		return 0;

	case WM_EXITMENULOOP:
		SetStatus(NULL, SF_READY);
		return 0;

	case WM_INITMENU:
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STATE;
		if (pGameBoy = GameBoys.GetActive())
		{
			mii.fState = pGameBoy->HasBattery() ? MFS_ENABLED : MFS_GRAYED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);

			if (pGameBoy->IsEmulating())
			{
				mii.fState = MFS_ENABLED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
				mii.fState = MFS_GRAYED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
			}
			else
			{
				mii.fState = MFS_ENABLED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
				mii.fState = MFS_GRAYED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
				if (hDisAsm == (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0))
				{
					mii.fState = MFS_ENABLED;
				}
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
			}

			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.dwTypeData = szBuffer;
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_0, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 0 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_0, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_1, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_1, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_2, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_2, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_3, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_3, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_4, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_4, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_5, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 5 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_5, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_6, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 6 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_6, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_7, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 7 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_7, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_8, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 8 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_8, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_9, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 9 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_9, false, &mii);
			mii.fMask = MIIM_STATE;

			mii.fState = MFS_ENABLED;
		}
		else
		{
			mii.fState = MFS_GRAYED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
		}
		SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RESET, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVESTATE, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADSTATE, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVESTATEAS, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADSTATEAS, false, &mii);
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 0), 7, true, &mii);
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 11, true, &mii);
		if (mii.fState == MFS_ENABLED)
		{
			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.dwTypeData = szBuffer;
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_0, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 0 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_0, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_1, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_1, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_2, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_2, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_3, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_3, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_4, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_4, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_5, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 5 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_5, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_6, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 6 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_6, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_7, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 7 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_7, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_8, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 8 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_8, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_9, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 9 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_9, false, &mii);
			mii.fMask = MIIM_STATE;
		}
		if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
		{
			if (hTempWnd == hDisAsm || hTempWnd == hMemory || hTempWnd == hRegisters || hTempWnd == hPalettes || hTempWnd == hHardware
				|| hTempWnd == hTileMap || hTempWnd == hTiles)
			{
				mii.fState = MFS_GRAYED;
			}
			else
			{
				mii.fState = MFS_ENABLED;
			}
		}
		else
		{
			mii.fState = MFS_GRAYED;
		}
		SetMenuItemInfo((HMENU)wParam, ID_FILE_CLOSE, false, &mii);
		if (hTempWnd && (hTempWnd == hTileMap || hTempWnd == hTiles))
		{
			mii.fState = MFS_ENABLED;
		}
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 10, true, &mii);
		if (mii.fState == MFS_ENABLED)
		{
			if (hTempWnd == hTiles)
			{
				dw = Tiles.Zoom;
			}
			else
			{
				if (hTempWnd == hTileMap)
				{
					dw = TileMap.Zoom;
				}
				else
				{
					GetClientRect(hTempWnd, &rct);
					if (rct.right / 160 == rct.bottom / 144 && rct.right % 160 == 0 && rct.bottom % 144 == 0)
					{
						dw = rct.right / 160;
					}
					else
					{
						dw = 0;
					}
				}
			}
			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.dwTypeData = szBuffer;
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_100, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_100, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_200, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_200, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_300, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_300, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_400, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_400, false, &mii);
			mii.fMask = MIIM_STATE;
		}
		if (hTempWnd && (hTempWnd == hMemory || hTempWnd == hDisAsm) && pGameBoy)
		{
			mii.fState = MFS_ENABLED;
			CreateBankMenu(pGameBoy, GetSubMenu(GetSubMenu((HMENU)wParam, 2), 12), 0);
		}
		else
		{
			mii.fState = MFS_GRAYED;
		}
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 12, true, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_EDIT_GOTO, false, &mii);
		mii.fState = Settings.SoundEnabled ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo((HMENU)wParam, ID_TOOLS_SOUNDENABLED, false, &mii);
		mii.fState = GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_STATUSBAR, false, &mii);
		return 0;

	/*case WM_NCCALCSIZE:
		if (wParam)
		{
			DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
			((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].left += Registers.Width;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].top += ;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].right -= ;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].bottom -= ;
			return WVR_HREDRAW | WVR_VREDRAW;
		}
		else
		{
			DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
			((RECT *)lParam)->left += Registers.Width;
			return 0;
		}

	case WM_NCPAINT:
		DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
		HDC		hdc;
		SetLastError(NOERROR);
		if (!(hdc = GetWindowDC(hWin)))
		{
			DisplayErrorMessage(hWin);
			return 0;
		}
		//rct.right = rct.left + Registers.Width;
		//rct.bottom = rct.top + Registers.Height;
		RECT	Rect2, Rect3;
		POINT	pt;
		GetWindowRect(hWin, &rct);
		GetClientRect(hWin, &Rect2);
		pt.x = 0;
		pt.y = 0;
		ClientToScreen(hWin, &pt);
		Rect2.left = pt.x - rct.left;
		Rect2.top = pt.y - rct.top;
		rct.right -= rct.left;
		rct.bottom -= rct.top;
		rct.left = 0;
		rct.top = 0;

		//rct
		//	right	= width of window
		Rect3.left = GetSystemMetrics(SM_CXSIZEFRAME);
		Rect3.top = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);
		Rect3.right = Rect2.left;
		Rect3.bottom = rct.bottom - GetSystemMetrics(SM_CYSIZEFRAME);
		FillRect(hdc, &Rect3, (HBRUSH)(COLOR_BTNFACE + 1));
		rct.left = GetSystemMetrics(SM_CXSIZEFRAME);
		rct.top = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);
		rct.right = rct.left + Registers.Width - 1;
		rct.bottom = rct.top + Registers.Height - 1;
		FillRect(hdc, &rct, (HBRUSH)(COLOR_WINDOW + 1));
		PaintRegisters(hdc, &rct);

		//Draw Border around client area
		Rect2.bottom += Rect2.top + GetSystemMetrics(SM_CYEDGE);
		Rect2.right += Rect2.left + GetSystemMetrics(SM_CXEDGE);
		Rect2.left -= GetSystemMetrics(SM_CXEDGE);
		Rect2.top -= GetSystemMetrics(SM_CYEDGE);
		DrawEdge(hdc, &Rect2, EDGE_SUNKEN, BF_ADJUST | BF_RECT);
		ReleaseDC(hWin, hdc);
		return 0;*/

	case WM_SIZE:
		if (!(GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE))
		{
			break;
		}
		GetWindowRect(hStatusWnd, &rct);
		MoveWindow(hClientWnd, 0, 0, LOWORD(lParam), HIWORD(lParam) - (rct.bottom - rct.top), true);
		MoveWindow(hStatusWnd, 0, HIWORD(lParam) - (rct.bottom - rct.top), LOWORD(lParam), HIWORD(lParam), true);
		return 0;

	case WM_CREATE:
		if (!(hStatusWnd = CreateWindow(STATUSCLASSNAME, "Ready", (StatusBarAppearance ? WS_VISIBLE : 0) | WS_CHILD | SBARS_SIZEGRIP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL)))
		{
			DisplayErrorMessage(NULL);
			return -1;
		}
		ccs.hWindowMenu = GetSubMenu(GetMenu(hWin), MENU_WINDOW);
		ccs.idFirstChild = 0;
		if (!(hClientWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "MDICLIENT", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, 0, 0, 0, 0, hWin, NULL, hInstance, &ccs)))
		{
			DisplayErrorMessage(NULL);
			return -1;
		}
		return 0;

	case WM_CLOSE:
		if (GameBoys.DeleteAll())
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		Main.Appearance = GetWindowLong(hWin, GWL_STYLE) & WS_MAXIMIZE ? 2 : 0;
		if (!(Main.Appearance & 2) && !(GetWindowLong(hWin, GWL_STYLE) & WS_MINIMIZE))
		{
			GetWindowRect(hWin, &rct);
			Main.x = rct.left;
			Main.y = rct.top;
			Main.Width = rct.right - rct.left;
			Main.Height = rct.bottom - rct.top;
		}
		Registers.Appearance = hRegisters ? 1 : 0;
		DisAsm.Appearance = hDisAsm ? 1 : 0;
		Memory.Appearance = hMemory ? 1 : 0;
		Tiles.Appearance = hTiles ? 1 : 0;
		TileMap.Appearance = hTileMap ? 1 : 0;
		Palettes.Appearance = hPalettes ? 1 : 0;
		Hardware.Appearance = hHardware ? 1 : 0;
		PostQuitMessage(0);
	}

	return DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
}



void UpdateRegistry()
{
	HKEY		hKey;
	DWORD		Value, ReleaseNo, ValueSize, ValueType, dw, dwErrCode;
	char		szPath[MAX_PATH];


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_EXECUTE | KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}

	GetModuleFileName(NULL, szPath, sizeof(szPath));
	if (dwErrCode = RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE *)&szPath, strlen(szPath) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = VERSION_MAJOR;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMajor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = VERSION_MINOR;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMinor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}

	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, "Release", NULL, &ValueType, (BYTE *)&ReleaseNo, &ValueSize))
	{
		if (ValueType != REG_DWORD)
		{
			ReleaseNo = 0;
		}
	}
	else
	{
		ReleaseNo = 0;
	}

	if (ReleaseNo < GAME_LAD_RELEASENO)
	{
		ReleaseNo = GAME_LAD_RELEASENO;
		if (dwErrCode = RegSetValueEx(hKey, "Release", 0, REG_DWORD, (BYTE *)&ReleaseNo, sizeof(DWORD)))
		{
			RegCloseKey(hKey);
			DisplayErrorMessage(NULL, dwErrCode);
			return;
		}
	}
	RegCloseKey(hKey);

	if (strchr(szPath, '.'))
	{
		*strrchr(szPath, '.') = 0;
	}
	strcat(szPath, ".dll");
	if (dwErrCode = RegCreateKeyEx(HKEY_CLASSES_ROOT, "CLSID\\{acdece20-a9d8-11d4-ace1-e0ae57c10001}", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "", 0, REG_SZ, (BYTE *)"Game Lad", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	RegCloseKey(hKey);
	if (dwErrCode = RegCreateKeyEx(HKEY_CLASSES_ROOT, "CLSID\\{acdece20-a9d8-11d4-ace1-e0ae57c10001}\\InProcServer32", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "", 0, REG_SZ, (BYTE *)&szPath, strlen(szPath) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "ThreadingModel", 0, REG_SZ, (BYTE *)"Apartment", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	RegCloseKey(hKey);

	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "{acdece20-a9d8-11d4-ace1-e0ae57c10001}", 0, REG_SZ, (BYTE *)"Game Lad", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	RegCloseKey(hKey);


	if (Settings.GbFile)
	{
		RegisterGbFileType();
	}
	if (Settings.GlsFile)
	{
		RegisterGlsFileType();
	}
}



void LoadWindowSettings(HKEY hKey, char *pszWindowName, WINDOWSETTINGS *pWindowSettings)
{
	char		szKey[50];
	DWORD		dwLength;
	DWORD		Value, ValueSize, ValueType;


	strcpy(szKey, pszWindowName);
	dwLength = strlen(szKey);

	strcat(szKey, "Appearance");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Appearance = Value & 3;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "X");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->x = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Y");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->y = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Width");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Width = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Height");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Height = Value;
		}
	}
	if (pWindowSettings->Zoom != 0)
	{
		szKey[dwLength] = '\0';
		strcat(szKey, "Zoom");
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				if (Value > 4)
				{
					Value = 4;
				}
				if (Value < 1)
				{
					Value = 1;
				}
				pWindowSettings->Zoom = Value;
			}
		}
	}
}



BOOL SaveWindowSettings(HKEY hKey, char *pszWindowName, WINDOWSETTINGS *pWindowSettings)
{
	char		szKey[50];
	DWORD		dwLength;
	DWORD		Value, dwErrCode;


	strcpy(szKey, pszWindowName);
	dwLength = strlen(szKey);

	strcat(szKey, "Appearance");
	Value = pWindowSettings->Appearance;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "X");
	Value = pWindowSettings->x;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Y");
	Value = pWindowSettings->y;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Width");
	Value = pWindowSettings->Width;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Height");
	Value = pWindowSettings->Height;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return true;
	}
	if (pWindowSettings->Zoom != 0)
	{
		szKey[dwLength] = '\0';
		strcat(szKey, "Zoom");
		Value = pWindowSettings->Zoom;
		if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
		{
			RegCloseKey(hKey);
			DisplayErrorMessage(NULL, dwErrCode);
			return true;
		}
	}

	return false;
}



#define		LoadSetting(szSetting, cmd)													\
	ValueSize = sizeof(Value);															\
	if (!RegQueryValueEx(hKey, szSetting, NULL, &ValueType, (BYTE *)&Value, &ValueSize))\
	{																					\
		if (ValueType == REG_DWORD)														\
		{																				\
			cmd;																		\
		}																				\
	}

void LoadCustomSettings()
{
	HKEY		hKey;
	DWORD		Value, ValueSize, ValueType;


	Settings.GbFile = true;
	Settings.GlsFile = true;
	Settings.AutoLoadBattery = true;
	Settings.AutoLoadState = false;
	Settings.SaveBattery = SAVEBATTERY_PROMPT;
	Settings.SaveState = SAVESTATE_NO;
	Settings.FrameSkip = 0;
	Settings.GBType = GB_COLOR;
	Settings.AutoStart = 0;
	Settings.SoundEnabled = true;

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings", 0, KEY_EXECUTE, &hKey))
	{
		LoadSetting("GbFile", {Settings.GbFile = Value ? true : false;});
		LoadSetting("GlsFile", {Settings.GlsFile = Value ? true : false;});
		LoadSetting("AutoLoadBattery", {Settings.AutoLoadBattery = Value ? true : false;});
		LoadSetting("AutoLoadState", {Settings.AutoLoadState = Value ? true : false;});
		LoadSetting("SaveBattery", {Settings.SaveBattery = (BYTE)Value & (SAVEBATTERY_NO | SAVEBATTERY_ALWAYS | SAVEBATTERY_PROMPT);});
		LoadSetting("SaveState", {Settings.SaveState = (BYTE)Value & (SAVESTATE_NO | SAVESTATE_ALWAYS | SAVESTATE_PROMPT);});
		LoadSetting("FrameSkip", {Settings.FrameSkip = Value >= 9 ? 9 : (BYTE)Value;});
		LoadSetting("GameBoyType", {Settings.GBType = (BYTE)Value & GB_COLOR;});
		LoadSetting("AutoStart", {Settings.AutoStart = (BYTE)Value & (AUTOSTART_DEBUG | AUTOSTART_EXECUTE);});
		LoadSetting("SoundEnabled", {Settings.SoundEnabled = Value ? true : false;});
		RegCloseKey(hKey);
	}

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, KEY_EXECUTE, &hKey))
	{
		LoadSetting("Up", {Keys.Up = Value;});
		LoadSetting("Down", {Keys.Down = Value;});
		LoadSetting("Left", {Keys.Left = Value;});
		LoadSetting("Right", {Keys.Right = Value;});
		LoadSetting("A", {Keys.A = Value;});
		LoadSetting("B", {Keys.B = Value;});
		LoadSetting("Start", {Keys.Start = Value;});
		LoadSetting("Select", {Keys.Select = Value;});
		LoadSetting("FastForward", {Keys.FastForward = Value;});
		RegCloseKey(hKey);
	}

	//In Game Lad 1.1 (release 2) and below, the A and B buttons were swapped.
	//This will swap the buttons if Game Lad 1.2 (release 3) or higher has never
	//been run on this computer.
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Release", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType != REG_DWORD)
			{
				Value = 0;
			}
		}
		else
		{
			Value = 0;
		}
		RegCloseKey(hKey);
		if (Value < 3)
		{
			Value = Keys.A;
			Keys.A = Keys.B;
			Keys.B = Value;
		}
		if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, KEY_SET_VALUE, &hKey))
		{
			Value = Keys.A;
			RegSetValueEx(hKey, "A", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD));
			Value = Keys.B;
			RegSetValueEx(hKey, "B", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD));
			RegCloseKey(hKey);
		}
	}


	Main.Appearance = 0;
	Main.x = CW_USEDEFAULT;
	Main.y = CW_USEDEFAULT;
	Main.Width = CW_USEDEFAULT;
	Main.Height = CW_USEDEFAULT;
	Main.Zoom = 0;
	Registers.Appearance = 0;
	Registers.x = CW_USEDEFAULT;
	Registers.y = CW_USEDEFAULT;
	Registers.Width = 12 * FixedFontWidth + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	Registers.Height = 14 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Registers.Zoom = 0;
	DisAsm.Appearance = 0;
	DisAsm.x = CW_USEDEFAULT;
	DisAsm.y = CW_USEDEFAULT;
	DisAsm.Width = 32 * FixedFontWidth + 15 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL);
	DisAsm.Height = 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	DisAsm.Zoom = 0;
	Memory.Appearance = 0;
	Memory.x = CW_USEDEFAULT;
	Memory.y = CW_USEDEFAULT;
	Memory.Width = 42 * FixedFontWidth + 1 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL);
	Memory.Height = 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Memory.Zoom = 0;
	Tiles.Appearance = 0;
	Tiles.x = CW_USEDEFAULT;
	Tiles.y = CW_USEDEFAULT;
	Tiles.Width = 134 + 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE);
	Tiles.Height = 45 + 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	Tiles.Zoom = 1;
	TileMap.Appearance = 0;
	TileMap.x = CW_USEDEFAULT;
	TileMap.y = CW_USEDEFAULT;
	TileMap.Width = 171 + 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE);
	TileMap.Height = 55 + 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	TileMap.Zoom = 1;
	Palettes.Appearance = 0;
	Palettes.x = CW_USEDEFAULT;
	Palettes.y = CW_USEDEFAULT;
	Palettes.Width = 362 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	Palettes.Height = 345 + FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Palettes.Zoom = 0;
	Hardware.Appearance = 0;
	Hardware.x = CW_USEDEFAULT;
	Hardware.y = CW_USEDEFAULT;
	Hardware.Width = 19 * FixedFontWidth + 4 * GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXVSCROLL);
	Hardware.Height = 3 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 4 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYHSCROLL);
	Hardware.Zoom = 0;
	StatusBarAppearance = true;

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Windows", 0, KEY_EXECUTE, &hKey))
	{
		LoadWindowSettings(hKey, "", &Main);
		LoadWindowSettings(hKey, "Registers", &Registers);
		LoadWindowSettings(hKey, "Disassembly", &DisAsm);
		LoadWindowSettings(hKey, "Memory", &Memory);
		LoadWindowSettings(hKey, "Tiles", &Tiles);
		LoadWindowSettings(hKey, "TileMap", &TileMap);
		LoadWindowSettings(hKey, "Palettes", &Palettes);
		LoadWindowSettings(hKey, "Hardware", &Hardware);
		LoadSetting("StatusBarAppearance", {StatusBarAppearance = Value ? true : false;});
		RegCloseKey(hKey);
	}

	if (Main.x >= (unsigned)GetSystemMetrics(SM_CXFULLSCREEN) - 30)
	{
		Main.x = CW_USEDEFAULT;
	}
	if (Main.y >= (unsigned)GetSystemMetrics(SM_CYFULLSCREEN) - 20)
	{
		Main.y = CW_USEDEFAULT;
	}
	if (Tiles.Width == 0)
	{
		Tiles.Width = 46 + Tiles.Zoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	}
	if (Tiles.Height == 0)
	{
		Tiles.Height = 34 + Tiles.Zoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	}
	if (TileMap.Width == 0)
	{
		TileMap.Width = 83 + TileMap.Zoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	}
	if (TileMap.Height == 0)
	{
		TileMap.Height = 40 + TileMap.Zoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	}
}



#define		SaveSetting(dwSetting, szSetting)													\
	Value = dwSetting;																			\
	if (dwErrCode = RegSetValueEx(hKey, szSetting, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))\
	{																							\
		RegCloseKey(hKey);																		\
		DisplayErrorMessage(NULL, dwErrCode);													\
		return;																					\
	}

void SaveCustomSettings()
{
	HKEY		hKey;
	DWORD		Value, dw, dwErrCode;


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	SaveSetting(Settings.GbFile, "GbFile");
	SaveSetting(Settings.GlsFile, "GlsFile");
	SaveSetting(Settings.AutoLoadBattery, "AutoLoadBattery");
	SaveSetting(Settings.AutoLoadState, "AutoLoadState");
	SaveSetting(Settings.SaveBattery, "SaveBattery");
	SaveSetting(Settings.SaveState, "SaveState");
	SaveSetting(Settings.FrameSkip, "FrameSkip");
	SaveSetting(Settings.GBType, "GameBoyType");
	SaveSetting(Settings.AutoStart, "AutoStart");
	SaveSetting(Settings.SoundEnabled, "SoundEnabled");
	RegCloseKey(hKey);

	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	SaveSetting(Keys.Up, "Up");
	SaveSetting(Keys.Down, "Down");
	SaveSetting(Keys.Left, "Left");
	SaveSetting(Keys.Right, "Right");
	SaveSetting(Keys.A, "A");
	SaveSetting(Keys.B, "B");
	SaveSetting(Keys.Start, "Start");
	SaveSetting(Keys.Select, "Select");
	SaveSetting(Keys.FastForward, "FastForward");
	RegCloseKey(hKey);


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Windows", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}

	if (SaveWindowSettings(hKey, "", &Main))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Registers", &Registers))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Disassembly", &DisAsm))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Memory", &Memory))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Tiles", &Tiles))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "TileMap", &TileMap))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Palettes", &Palettes))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Hardware", &Hardware))
	{
		return;
	}
	SaveSetting(StatusBarAppearance, "StatusBarAppearance");

	RegCloseKey(hKey);
}



INITCOMMONCONTROLSEX	CommonControls = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG					msg;
	WNDCLASS			wc;
	HDC					hdc;
	TEXTMETRIC			tm;
	HACCEL				hAccelerator;


	//Make hInst global
	hInstance = hInst;


	//Check if a high resolution timer is available, and get its frequency
	if (QueryPerformanceFrequency(&TimerFrequency))
	{
		TimerFrequency.QuadPart /= 60;
	}
	else
	{
		MessageBox(hWnd, "No high resolution timer available.", NULL, MB_OK | MB_ICONERROR);
		return 1;
	}


	//Load fonts used
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);

	//Get size of the fonts
	hdc = GetDC(hWnd);
	SelectObject(hdc, hFixedFont);
	GetTextMetrics(hdc, &tm);
	ReleaseDC(hWnd, hdc);
	FixedFontWidth = tm.tmMaxCharWidth;
	FixedFontHeight = tm.tmHeight;


	//Loads user's settings
	LoadCustomSettings();


	//Set Game Lad specific keys in the system registry
	UpdateRegistry();


	if (!InitCommonControlsEx(&CommonControls))
	{
		DisplayErrorMessage(NULL);
		return 1;
	}


	//Register main window's class
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Game Lad";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return 1;
	}
	//and Game Boy windows' class
	wc.lpfnWndProc = GameBoyWndProc;
	wc.hbrBackground = NULL;
	wc.lpszClassName = "Game Boy";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return 1;
	}

	//Load popup menus
	if (!(hPopupMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUPMENU))))
	{
		DisplayErrorMessage(NULL);
		return 1;
	}

	//Create main window
	if (!(hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, "Game Lad", "Game Lad",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		Main.x, Main.y, Main.Width, Main.Height,
		NULL, LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU)), hInstance, NULL)))
	{
		DisplayErrorMessage(NULL);
		DestroyMenu(hPopupMenu);
		return 1;
	}


	if (CreateDebugWindows())
	{
		DestroyMenu(hPopupMenu);
		DestroyWindow(hWnd);
		return 1;
	}


	if (!(hStartStopEvent = CreateEvent(NULL, true, false, NULL)))
	{
		DisplayErrorMessage(NULL);
		DestroyMenu(hPopupMenu);
		DestroyWindow(hWnd);
		return 1;
	}


	hAccelerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));


	//Initialize Dynamic Data Exchange
	DdeInitialize(&DdeInst, DdeCallback, APPCLASS_STANDARD | CBF_FAIL_ADVISES | CBF_FAIL_POKES | CBF_FAIL_REQUESTS | CBF_FAIL_SELFCONNECTIONS | CBF_SKIP_ALLNOTIFICATIONS, 0);
	hDdeServiceString = DdeCreateStringHandle(DdeInst, "Game Lad", CP_WINANSI);
	hDdeTopicString = DdeCreateStringHandle(DdeInst, SZDDESYS_TOPIC, CP_WINANSI);
	DdeNameService(DdeInst, hDdeServiceString, 0, DNS_REGISTER);


	ZeroMemory(ZeroStatus, sizeof(ZeroStatus));


	//See if a path to a file has been sent on the command line
	if (lpCmdLine[0])
	{
		GameBoys.NewGameBoy(lpCmdLine, Settings.AutoLoadState ? "" : NULL, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
	}


	if (Registers.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_REGISTERS, 0);
	}
	if (DisAsm.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	if (Memory.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_MEMORY, 0);
	}
	if (Tiles.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_TILES, 0);
	}
	if (TileMap.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_TILEMAP, 0);
	}
	if (Palettes.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_PALETTES, 0);
	}
	if (Hardware.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_HARDWARE, 0);
	}


	InitializeCriticalSection(&csSound);


	/*if (!(hStringInstance = LoadLibrary("Language.dll")))
	{
		hStringInstance = hInstance;
	}*/


	//Show window
	ShowWindow(hWnd, Main.Appearance & 2 ? SW_SHOWMAXIMIZED : nCmdShow);
	UpdateWindow(hWnd);


	//Main message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccelerator, &msg))
		{
			if (msg.message == WM_CHAR && F4Pressed)
			{
				if (msg.wParam >= '0' && msg.wParam <= '9')
				{
					if (F4Pressed == GameBoys.GetActive())
					{
						F4Pressed->SetStateSlot(msg.wParam - '0');
					}
				}
				else
				{
					SetStatus(NULL, SF_READY);
					DispatchMessage(&msg);
				}
				F4Pressed = NULL;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}


	//Time to clean up


	/*if (hStringInstance != hInstance)
	{
		FreeLibrary(hStringInstance);
	}*/


	DeleteCriticalSection(&csSound);


	DestroyMenu(hPopupMenu);

	CloseHandle(hStartStopEvent);

	DdeNameService(DdeInst, 0, 0, DNS_UNREGISTER);
	DdeFreeStringHandle(DdeInst, hDdeServiceString);
	DdeFreeStringHandle(DdeInst, hDdeTopicString);
	DdeUninitialize(DdeInst);


	SaveCustomSettings();


	return msg.wParam;
}

