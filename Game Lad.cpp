#include	<windows.h>
#include	<afxres.h>
#include	"resource.h"

#define		GAME_LAD_CPP
#include	"Game Lad.h"
#include	"Emulation.h"
#include	"Debugger.h"



KEYS		Keys = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'Z', 'X', VK_RETURN, VK_SHIFT, VK_TAB};



CGameBoyList::CGameBoyList()
{
	FirstGameBoy = NULL;
	ActiveGameBoy = NULL;
}



CGameBoy *CGameBoyList::NewGameBoy(char *pszROMFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart)
{
	GameBoy		*LastGameBoy;


	if (!(LastGameBoy = new GameBoy))
	{
		MessageBox(NULL, "Insufficient memory.", NULL, MB_OK | MB_ICONERROR);
		return NULL;
	}
	LastGameBoy->pNext = FirstGameBoy;
	FirstGameBoy = LastGameBoy;

	if (!(LastGameBoy->pGameBoy = new CGameBoy(Flags)))
	{
		FirstGameBoy = LastGameBoy->pNext;
		delete LastGameBoy;
		MessageBox(NULL, "Insufficient memory.", NULL, MB_OK | MB_ICONERROR);
		return NULL;
	}
	if (LastGameBoy->pGameBoy->Init(pszROMFilename, pszBatteryFilename))
	{
		FirstGameBoy = LastGameBoy->pNext;
		delete LastGameBoy->pGameBoy;
		delete LastGameBoy;
		return NULL;
	}

	ActiveGameBoy = LastGameBoy->pGameBoy;
	if (AutoStart == AUTOSTART_DEBUG)
	{
		SendMessage(hWnd, WM_COMMAND, ID_EMULATION_STARTDEBUG, 0);
	}
	if (AutoStart == AUTOSTART_EXECUTE)
	{
		SendMessage(hWnd, WM_COMMAND, ID_EMULATION_EXECUTE, 0);
	}
	PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
	return LastGameBoy->pGameBoy;
}



BOOL CGameBoyList::DeleteGameBoy(CGameBoy *pCGameBoy)
{
	GameBoy		*pGameBoy = FirstGameBoy, *pGameBoy2;


	if (FirstGameBoy->pGameBoy == pCGameBoy)
	{
		if (pGameBoy->pGameBoy->SaveBattery(true, false))
		{
			return true;
		}
		if (pGameBoy->pGameBoy == ActiveGameBoy)
		{
			ActiveGameBoy = NULL;
		}
		FirstGameBoy = FirstGameBoy->pNext;
		delete pGameBoy->pGameBoy;
		delete pGameBoy;
		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		return false;
	}

	while (pGameBoy->pNext->pGameBoy != pCGameBoy)
	{
		pGameBoy = pGameBoy->pNext;
	}

	if (pGameBoy->pNext->pGameBoy->SaveBattery(true, false))
	{
		return true;
	}
	if (pGameBoy->pNext->pGameBoy == ActiveGameBoy)
	{
		ActiveGameBoy = NULL;
	}
	pGameBoy2 = pGameBoy->pNext;
	pGameBoy->pNext = pGameBoy2->pNext;
	delete pGameBoy2->pGameBoy;
	delete pGameBoy2;

	PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
	return false;
}



BOOL CGameBoyList::DeleteAll()
{
	GameBoy		*NextGameBoy;


	while (FirstGameBoy)
	{
		if (FirstGameBoy->pGameBoy)
		{
			if (FirstGameBoy->pGameBoy->hThread)
			{
				PostThreadMessage(FirstGameBoy->pGameBoy->ThreadId, WM_QUIT, 0, 0);
				WaitForSingleObject(FirstGameBoy->pGameBoy->hThread, INFINITE);
			}
			if (FirstGameBoy->pGameBoy->SaveBattery(true, false))
			{
				return true;
			}
			if (FirstGameBoy->pGameBoy == ActiveGameBoy)
			{
				ActiveGameBoy = NULL;
			}
			delete FirstGameBoy->pGameBoy;
		}
		NextGameBoy = FirstGameBoy->pNext;
		delete FirstGameBoy;
		FirstGameBoy = NextGameBoy;
	}

	return false;
}



CGameBoy *CGameBoyList::GetActive()
{
	return ActiveGameBoy;
}



LPARAM CGameBoyList::WndProc(CGameBoy *pGameBoy, HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pGameBoy)
	{
		switch (uMsg)
		{
		case WM_MDIACTIVATE:
			if (hWin == (HWND)lParam)
			{
				ActiveGameBoy = pGameBoy;
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				MemoryFlags = 0;
			}
			break;

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE && pGameBoy->hThread)
			{
				PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
				WaitForSingleObject(pGameBoy->hThread, INFINITE);
				return 0;
			}
			break;

		case WM_CLOSE:
			if (pGameBoy = GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					if (pGameBoy->SaveBattery(true, false))
					{
						if (pGameBoy->Flags & GB_DEBUG)
						{
							pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
						}
						else
						{
							pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
						}
						return 0;
					}
				}
				DeleteGameBoy(pGameBoy);
			}
			return 0;
		}

		return pGameBoy->GameBoyWndProc(uMsg, wParam, lParam);
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



LPARAM CALLBACK GameBoyWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return GameBoyList.WndProc((CGameBoy *)GetWindowLong(hWin, GWL_USERDATA), hWin, uMsg, wParam, lParam);
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



#define		DDEGB_DMG			0x01
#define		DDEGB_GBC			0x02
#define		DDEGB_LOAD			0x04
#define		DDEGB_DEBUG			0x08
#define		DDEGB_EXECUTE		0x10
#define		DDEGB_NOBATTERY		0x20
#define		DDEGB_BATTERY		0x40

struct DDEGAMEBOY
{
	DWORD		Flags;
	char		szROM[MAX_PATH];
	char		szBattery[MAX_PATH];
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
				DDEGameBoy->szBattery[FirstCharacter + 1 + Size] = 0;
				FirstCharacter += 2 + Size;
			}
			FirstCharacter++;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "nobattery,", 10))
		{
			DDEGameBoy->Flags |= DDEGB_NOBATTERY;
			FirstCharacter += 10;
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



BOOL CreateKey(HWND hWin, char *Key, char *Data)
{
	HKEY	hKey;
	DWORD	dw;


	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, Key, 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dw))
	{
		return true;
	}
	if (RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)Data, strlen(Data) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(hWin);
		return true;
	}
	RegCloseKey(hKey);

	return false;
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



BOOL CALLBACK ShellEnumChildProc(HWND hWin, LPARAM lParam)
{
	switch (GetWindowLong(hWin, GWL_ID))
	{
	case IDC_AUTOSTART:
		if (lParam)
		{
			SendMessage(hWin, CB_ADDSTRING, 0, (LPARAM)"Stopped");
			SendMessage(hWin, CB_ADDSTRING, 0, (LPARAM)"Start Debug");
			SendMessage(hWin, CB_ADDSTRING, 0, (LPARAM)"Execute");
			switch (Settings.AutoStart)
			{
			case 0:
				SendMessage(hWin, CB_SETCURSEL, 0, 0);
				break;

			case AUTOSTART_DEBUG:
				SendMessage(hWin, CB_SETCURSEL, 1, 0);
				break;

			case AUTOSTART_EXECUTE:
				SendMessage(hWin, CB_SETCURSEL, 2, 0);
				break;
			}
		}
		else
		{
			switch (SendMessage(hWin, CB_GETCURSEL, 0, 0))
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
		}
		break;

	case IDC_AUTOLOADBATTERY:
		if (lParam)
		{
			SendMessage(hWin, BM_SETCHECK, Settings.AutoLoadBattery ? BST_CHECKED : BST_UNCHECKED, 0);
		}
		else
		{
			Settings.AutoLoadBattery = SendMessage(hWin, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
		}
		break;

	case IDC_GBTYPE:
		if (lParam)
		{
			SendMessage(hWin, CB_ADDSTRING, 0, (long)"Game Boy");
			SendMessage(hWin, CB_ADDSTRING, 0, (long)"Game Boy Color");
			SendMessage(hWin, CB_SETCURSEL, Settings.GBType == GB_COLOR ? 1 : 0, 0);
		}
		else
		{
			Settings.GBType = SendMessage(hWin, CB_GETCURSEL, 0, 0) == 1 ? GB_COLOR : 0;
		}
		break;
	}

	return true;
}



BOOL CALLBACK ShellOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char	Filename[MAX_PATH + 4];
	BOOL	Error;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_REGISTER_FILE_TYPES:
			Error = false;
			Error |= CreateKey(hWin, ".gb", "GameBoyRom");
			Error |= CreateKey(hWin, ".gbc", "GameBoyRom");
			Error |= CreateKey(hWin, "GameBoyRom", "Game Boy Rom");
			GetModuleFileName(NULL, (char *)&Filename[1], MAX_PATH);
			Error |= CreateKey(hWin, "GameBoyRom\\DefaultIcon", (char *)&Filename[1]);
			Error |= CreateKey(hWin, "GameBoyRom\\shell", "open");
			Error |= CreateKey(hWin, "GameBoyRom\\shell\\open", "");
			Filename[0] = '\"';
			strcat(Filename, "\"");
			Error |= CreateKey(hWin, "GameBoyRom\\shell\\open\\command", Filename);
			Error |= CreateKey(hWin, "GameBoyRom\\shell\\open\\ddeexec", "open,\"%1\"");
			Error |= CreateKey(hWin, "GameBoyRom\\shell\\open\\ddeexec\\Application", "Game Lad");
			Error |= CreateKey(hWin, "GameBoyRom\\shell\\open\\ddeexec\\Topic", "system");
			Error |= CreateKey(hWin, "GameBoyRom\\shellex\\PropertySheetHandlers\\Game Lad", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
			if (Error)
			{
				MessageBox(hWin, "An error occurred while updating the registry.", NULL, MB_OK | MB_ICONERROR);
			}
			else
			{
				MessageBox(hWin, "The registry was successfully updated.", "Game Lad", MB_OK | MB_ICONINFORMATION);
			}
			return true;

		case IDC_AUTOSTART:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;

		case IDC_AUTOLOADBATTERY:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;

		case IDC_GBTYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		break;

	/*case WM_HELP:
		if (((HELPINFO *)lParam)->iCtrlId == ID_REGISTER_FILE_TYPES)
		{
		}
		break;*/

	case WM_INITDIALOG:
		EnumChildWindows(hWin, ShellEnumChildProc, true);
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->code == PSN_APPLY)
		{
			EnumChildWindows(hWin, ShellEnumChildProc, false);
		}
		break;
	}

	return false;
}



/*WNDPROC			DefaultClientWndProc;

LRESULT CALLBACK ClientWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	}

	return CallWindowProc(DefaultClientWndProc, hWin, uMsg, wParam, lParam);
}*/



LRESULT CALLBACK WndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CLIENTCREATESTRUCT	ccs;
	OPENFILENAME		of;
	char				Filename[MAX_PATH];
	DWORD				ItemNo;
	CGameBoy			*pGameBoy;
	BOOL				Restart;
	MENUITEMINFO		mii;
	HWND				hTempWnd;
	PROPSHEETPAGE		psp[2];
	PROPSHEETHEADER		psh;


	switch (uMsg)
	{
	case MM_WOM_DONE:
		EnterCriticalSection(&cs);
		waveOutUnprepareHeader((HWAVEOUT)wParam, (WAVEHDR *)lParam, sizeof(WAVEHDR));
		delete ((WAVEHDR *)lParam)->lpData;
		LeaveCriticalSection(&cs);
		return 0;

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
			GameBoyList.NewGameBoy(Filename, LoadBattery ? "" : NULL, GBType, AutoStart);
			return 0;

		case ID_FILE_CLOSE:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
			{
				SendMessage(hTempWnd, WM_CLOSE, 0, 0);
			}
			return 0;

		case ID_FILE_LOADBATTERY:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					Restart = true;
				}
				else
				{
					Restart = false;
				}
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
				if (Restart)
				{
					if (pGameBoy->Flags & GB_DEBUG)
					{
						pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
					else
					{
						pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
				}
			}
			return 0;

		case ID_FILE_SAVEBATTERY:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					pGameBoy->SaveBattery(false, false);
					if (pGameBoy->Flags & GB_DEBUG)
					{
						pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
					else
					{
						pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
				}
				else
				{
					pGameBoy->SaveBattery(false, false);
				}
			}
			return 0;

		case ID_FILE_SAVEBATTERYAS:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					pGameBoy->SaveBattery(false, true);
					if (pGameBoy->Flags & GB_DEBUG)
					{
						pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
					else
					{
						pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
				}
				else
				{
					pGameBoy->SaveBattery(false, true);
				}
			}
			return 0;

		case ID_FILE_CLEARBATTERY:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					Restart = true;
				}
				else
				{
					Restart = false;
				}
				if (!pGameBoy->SaveBattery(true, false))
				{
					pGameBoy->LoadBattery(NULL);
				}
				if (Restart)
				{
					if (pGameBoy->Flags & GB_DEBUG)
					{
						pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
					else
					{
						pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
				}
			}
			return 0;

		case ID_FILE_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;

		case ID_EDIT_GOTO:
			SendMessage((HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0), uMsg, wParam, lParam);
			return 0;

		case ID_VIEW_MEMORY:
			if (!hMemory)
			{
				if (!(hMemory = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Memory", "Memory", WS_VISIBLE | WS_CAPTION | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 42 * FixedFontWidth + 1 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL), 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE), hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_REGISTERS:
			if (!hRegisters)
			{
				if (!(hRegisters = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Registers", "Registers", WS_VISIBLE | WS_CAPTION | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 14 * FixedFontWidth + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 14 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE), hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_DISASSEMBLY:
			if (!hDisAsm)
			{
				if (!(hDisAsm = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "DisAsm", "Disassembly", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL /*| WS_HSCROLL*/, CW_USEDEFAULT, CW_USEDEFAULT, 32 * FixedFontWidth + 15 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL), 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE), hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_TILES:
			if (!hTiles)
			{
				if (!(hTiles = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Tiles", "Tiles", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL | WS_HSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_TILEMAP:
			if (!hTileMap)
			{
				if (!(hTileMap = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "TileMap", "Tile Map", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL | WS_HSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_PALETTES:
			if (!hPalettes)
			{
				if (!(hPalettes = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Palettes", "Palettes", WS_VISIBLE | WS_CAPTION | WS_BORDER /*| WS_VSCROLL | WS_HSCROLL*/, CW_USEDEFAULT, CW_USEDEFAULT, 362 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 345 + FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) +  2 * GetSystemMetrics(SM_CYEDGE), hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
			return 0;

		case ID_VIEW_HARDWARE:
			if (!hHardware)
			{
				if (!(hHardware = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Hardware", "Hardware", WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL | WS_HSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 19 * FixedFontWidth + 4 * GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXVSCROLL), 3 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 4 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYHSCROLL), hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage(NULL);
				}
			}
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

		case ID_EMULATION_STARTDEBUG:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (!pGameBoy->hThread)
				{
					pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
				}
			}
			return 0;

		case ID_EMULATION_EXECUTE:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (!pGameBoy->hThread)
				{
					pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
				}
			}
			return 0;

		case ID_EMULATION_STOP:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
				}
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

		case ID_EMULATION_RESET:
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
					WaitForSingleObject(pGameBoy->hThread, INFINITE);
					pGameBoy->Reset();
					if (pGameBoy->Flags & GB_DEBUG)
					{
						pGameBoy->hThread = CreateThread(NULL, 0, DebugGameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
					else
					{
						pGameBoy->hThread = CreateThread(NULL, 0, GameLoop, pGameBoy, 0, &pGameBoy->ThreadId);
					}
				}
				else
				{
					pGameBoy->Reset();
				}
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
			psp[1].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_SHELL);
			psp[1].pfnDlgProc = ShellOptionsDlgProc;
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
			GameBoyList.NewGameBoy(Filename, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
			ItemNo++;
		}
		return 0;

	case WM_APP_DDEOPENFILE:
		GameBoyList.NewGameBoy(((DDEGAMEBOY *)lParam)->szROM,
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

	case WM_INITMENU:
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STATE;
		if (pGameBoy = GameBoyList.GetActive())
		{
			if (!pGameBoy->SaveRamSize)
			{
				mii.fState = MFS_GRAYED;
				SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			}
			else
			{
				mii.fState = MFS_ENABLED;
				SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			}
			if (pGameBoy->hThread)
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
		if (hTempWnd && (hTempWnd == hMemory || hTempWnd == hDisAsm) && GameBoyList.GetActive())
		{
			mii.fState = MFS_ENABLED;
		}
		else
		{
			mii.fState = MFS_GRAYED;
		}
		SetMenuItemInfo((HMENU)wParam, ID_EDIT_GOTO, false, &mii);
		if (hTempWnd && (hTempWnd == hTileMap || hTempWnd == hTiles))
		{
			mii.fState = MFS_ENABLED;
		}
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_100, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_200, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_300, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_400, false, &mii);
		return 0;

	case WM_CREATE:
		ccs.hWindowMenu = GetSubMenu(GetMenu(hWin), 5);
		ccs.idFirstChild = 0;
		if (!(hClientWnd = CreateWindow("MDICLIENT", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, 0, 0, 0, 0, hWin, NULL, hInstance, &ccs)))
		{
			DisplayErrorMessage(NULL);
			return 1;
		}
		//DefaultClientWndProc = (WNDPROC)SetWindowLong(hClientWnd, GWL_WNDPROC, (long)ClientWndProc);
		return 0;

	case WM_CLOSE:
		if (GameBoyList.DeleteAll())
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
	}

	return DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
}



void UpdateRegister()
{
	HKEY		hKey;
	DWORD		Value, dw, dwErrCode;
	char		szPath[MAX_PATH];


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
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
	Value = 1;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMajor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = 0;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMinor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
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
}



void LoadCustomSettings()
{
	HKEY		hKey;
	DWORD		Value, ValueSize, ValueType;


	Settings.AutoStart = 0;
	//Settings.AutoLoadDebug = true;
	Settings.AutoLoadBattery = true;
	Settings.GBType = GB_COLOR;

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "AutoStart", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Settings.AutoStart = (BYTE)Value & (AUTOSTART_DEBUG | AUTOSTART_EXECUTE);
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "AutoLoadBattery", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Settings.AutoLoadBattery = Value ? true : false;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "GameBoyType", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Settings.GBType = (BYTE)Value & GB_COLOR;
			}
		}
		RegCloseKey(hKey);
	}
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Up", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Up = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Down", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Down = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Left", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Left = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Right", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Right = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "A", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.A = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "B", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.B = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Start", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Start = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Select", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.Select = Value;
			}
		}
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "FastForward", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				Keys.FastForward = Value;
			}
		}
		RegCloseKey(hKey);
	}
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

	Value = Settings.AutoStart;
	if (dwErrCode = RegSetValueEx(hKey, "AutoStart", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Settings.AutoLoadBattery;
	if (dwErrCode = RegSetValueEx(hKey, "AutoLoadBattery", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Settings.GBType;
	if (dwErrCode = RegSetValueEx(hKey, "GameBoyType", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	RegCloseKey(hKey);

	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}

	Value = Keys.Up;
	if (dwErrCode = RegSetValueEx(hKey, "Up", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.Down;
	if (dwErrCode = RegSetValueEx(hKey, "Down", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.Left;
	if (dwErrCode = RegSetValueEx(hKey, "Left", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.Right;
	if (dwErrCode = RegSetValueEx(hKey, "Right", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.A;
	if (dwErrCode = RegSetValueEx(hKey, "A", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.B;
	if (dwErrCode = RegSetValueEx(hKey, "B", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.Start;
	if (dwErrCode = RegSetValueEx(hKey, "Start", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.Select;
	if (dwErrCode = RegSetValueEx(hKey, "Select", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	Value = Keys.FastForward;
	if (dwErrCode = RegSetValueEx(hKey, "FastForward", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(NULL, dwErrCode);
		return;
	}
	RegCloseKey(hKey);
}



//INITCOMMONCONTROLSEX	CommonControls = {sizeof(INITCOMMONCONTROLSEX), ICC_USEREX_CLASSES | ICC_TAB_CLASSES /*| ICC_COOL_CLASSES | ICC_BAR_CLASSES*/};

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


	//Set Game Lad specific keys in the system registry
	UpdateRegister();


	LoadCustomSettings();


	/*if (!InitCommonControlsEx(&CommonControls))
	{
		DisplayErrorMessage(NULL);
		return 1;
	}*/


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
	if (!(hWnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, "Game Lad", "Game Lad",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU)), hInstance, NULL)))
	{
		DestroyMenu(hPopupMenu);
		DisplayErrorMessage(NULL);
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


	if (CreateDebugWindows())
	{
		DestroyMenu(hPopupMenu);
		DestroyWindow(hWnd);
		return 1;
	}


	hAccelerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));


	//Initialize Dynamic Date Exchange
	DdeInitialize(&DdeInst, DdeCallback, APPCLASS_STANDARD | CBF_FAIL_ADVISES | CBF_FAIL_POKES | CBF_FAIL_REQUESTS | CBF_FAIL_SELFCONNECTIONS | CBF_SKIP_ALLNOTIFICATIONS, 0);
	hDdeServiceString = DdeCreateStringHandle(DdeInst, "Game Lad", CP_WINANSI);
	hDdeTopicString = DdeCreateStringHandle(DdeInst, SZDDESYS_TOPIC, CP_WINANSI);
	DdeNameService(DdeInst, hDdeServiceString, 0, DNS_REGISTER);


	//Syncronization for sound
	InitializeCriticalSection(&cs);


	ZeroMemory(ZeroStatus, sizeof(ZeroStatus));


	//See if a path to a file has been sent on the command line
	if (lpCmdLine[0])
	{
		GameBoyList.NewGameBoy(lpCmdLine, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
	}


	//Show window
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	//Main message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccelerator, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	//Time to clean up


	DestroyMenu(hPopupMenu);

	DeleteCriticalSection(&cs);

	DdeNameService(DdeInst, 0, 0, DNS_UNREGISTER);
	DdeFreeStringHandle(DdeInst, hDdeServiceString);
	DdeFreeStringHandle(DdeInst, hDdeTopicString);
	DdeUninitialize(DdeInst);


	SaveCustomSettings();


	return msg.wParam;
}

