#include	<windows.h>

#define		GAME_BOY_CPP
#include	"CDebugInfo.h"
#include	"Game Lad.h"
#include	"CGameBoys.h"
#include	"Z80.h"
#include	"Debugger.h"
#include	"resource.h"



#define		GAME_LAD_SAVE_STATE_VERSION			0



void CloseSound_asm(CGameBoy *pGameBoy);



/*BYTE		NintendoGraphic[48] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};*/
WORD		GreyScales[4] =	{	0x7FFF,
								15855, //011110111101111
								11627, //010110101101011
								0
							};



BYTE RomSize(BYTE Byte148)
{
	if (Byte148 <= 7)
	{
		return (2 << Byte148) - 1;
	}

	switch (Byte148)
	{
	case 0x52:
		return 71;

	case 0x53:
		return 79;

	case 0x54:
		return 95;
	}

	return 0;
}



CGameBoy::CGameBoy(BYTE Flags)
{
	ZeroMemory(this, sizeof(*this));

	this->Flags = Flags & GB_COLOR;
	FrameSkip = Settings.FrameSkip;
}



CGameBoy::~CGameBoy()
{
	HANDLE		hTempThread;


	if (hTempThread = hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(hTempThread, INFINITE);
	}
	CloseSound();

	delete MEM_ROM;
	delete MemStatus_ROM;
	delete MemStatus_RAM;

	if (pDebugInfo)
	{
		delete pDebugInfo;
	}

	if (hGBWnd)
	{
		SetWindowLong(hGBWnd, GWL_USERDATA, NULL);
		SendMessage(hGBWnd, WM_CLOSE, 0, 0);
	}
	if (hGBBitmap)
	{
		SelectObject(hGBDC, hOldBitmap);
		DeleteObject(hGBBitmap);
	}
	if (hGBDC)
	{
		DeleteDC(hGBDC);
	}
}



BOOL CGameBoy::Init(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename)
{
	HANDLE			hFile;
	DWORD			FileSize, nBytes;
	BITMAPINFO		bmi;
	BOOL			Maximized;
	char			szStatusText[sizeof(Rom) + 7];


	if (!pszROMFilename || pszROMFilename[0] == '\0')
	{
		if (!pszStateFilename)
		{
			return true;
		}
		if (pszStateFilename[0] == '\0')
		{
			return true;
		}

		strcpy(Rom, pszStateFilename);
		if (strchr(Rom, '.') > Rom)
		{
			*strrchr(Rom, '.') = '\0';
		}
		if (SearchPath(NULL, Rom, ".gb", MAX_PATH, Rom, NULL) == 0)
		{
			if (SearchPath(NULL, Rom, ".gbc", MAX_PATH, Rom, NULL) == 0)
			{
				if (strlen(Rom) == 0)
				{
					MessageBox(hWnd, "ROM File not found.", NULL, MB_OK | MB_ICONERROR);
					return true;
				}
				if (Rom[strlen(Rom) - 1] >= '0' && Rom[strlen(Rom) - 1] <= '9')
				{
					Rom[strlen(Rom) - 1] = '\0';
					if (SearchPath(NULL, Rom, ".gb", MAX_PATH, Rom, NULL) == 0)
					{
						if (SearchPath(NULL, Rom, ".gbc", MAX_PATH, Rom, NULL) == 0)
						{
							MessageBox(hWnd, "ROM File not found.", NULL, MB_OK | MB_ICONERROR);
							return true;
						}
					}
				}
				else
				{
					MessageBox(hWnd, "ROM File not found.", NULL, MB_OK | MB_ICONERROR);
					return true;
				}
			}
		}
	}
	else
	{
		strcpy(Rom, pszROMFilename);
	}

	//Load rom
	if ((hFile = CreateFile(Rom, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		DisplayErrorMessage(hWnd);
		return true;
	}
	FileSize = GetFileSize(hFile, NULL);
	if (SetFilePointer(hFile, 0x148, NULL, FILE_BEGIN) == 0xFFFFFFFF)
	{
		CloseHandle(hFile);
		DisplayErrorMessage(hWnd);
		return true;
	}
	if (!ReadFile(hFile, &MaxRomBank, 1, &nBytes, NULL))
	{
		CloseHandle(hFile);
		DisplayErrorMessage(hWnd);
		return true;
	}
	MaxRomBank = RomSize(MaxRomBank);
	if (FileSize != (((DWORD)MaxRomBank + 1) * 16384))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, "File size doesn't match rom header.", NULL, MB_OK | MB_ICONERROR);
		return true;
	}
	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
	{
		CloseHandle(hFile);
		DisplayErrorMessage(hWnd);
		return true;
	}
	if (!(MEM_ROM = new BYTE[FileSize]))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_ICONERROR | MB_OK);
		return true;
	}
	if (!(MemStatus_ROM = new BYTE[FileSize]))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_ICONERROR | MB_OK);
		return true;
	}
	if (!ReadFile(hFile, MEM_ROM, FileSize, &nBytes, NULL))
	{
		CloseHandle(hFile);
		DisplayErrorMessage(hWnd);
		return true;
	}
	CloseHandle(hFile);

	switch (MEM_ROM[0x0149])
	{
	case 1:
		MaxRamBank = 0;
		SaveRamSize = 2 * 1024;
		break;

	case 2:
		MaxRamBank = 0;
		SaveRamSize = 8 * 1024;
		break;

	case 3:
		MaxRamBank = 3;
		SaveRamSize = 32 * 1024;
		break;

	case 4:
		MaxRamBank = 15;
		SaveRamSize = 128 * 1024;
		break;

	default:
		MaxRamBank = 0;
		SaveRamSize = 0;
	}

	ZeroMemory(MemStatus_ROM, 16384 * (RomSize(MEM_ROM[0x148]) + 1));
	ZeroMemory(MemStatus_VRAM, sizeof(MemStatus_VRAM));
	if (SaveRamSize)
	{
		//External Battery RAM
		if (!(MemStatus_RAM = new BYTE[(MaxRamBank + 1) * 0x2000]))
		{
			MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		ZeroMemory(MemStatus_RAM, (MaxRamBank + 1) * 0x2000);
	}
	ZeroMemory(MemStatus_CPU, sizeof(MemStatus_CPU));

	switch (MEM_ROM[0x0147])
	{
	case 0x03:
	case 0x06:
	case 0x09:
	case 0x0D:
	case 0x0F:
	case 0x10:
	case 0x13:
	case 0x1B:
	case 0x1E:
		break;

	default:
		SaveRamSize = 0;
	}


	if (!hGBDC)
	{
		if (!(hGBDC = CreateCompatibleDC(NULL)))
		{
			DisplayErrorMessage(hWnd);
			return true;
		}

		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = 160 + 14;
		bmi.bmiHeader.biHeight = -144;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		if (!(hGBBitmap = CreateDIBSection(hGBDC, &bmi, DIB_RGB_COLORS, (void **)&pGBBitmap, NULL, 0)))
		{
			DisplayErrorMessage(hWnd);
			return true;
		}

		if (!(hOldBitmap = (HBITMAP)SelectObject(hGBDC, hGBBitmap)))
		{
			DisplayErrorMessage(hWnd);
			return true;
		}
	}

	if (pszStateFilename)
	{
		Reset();
		strcpy(Battery, pszBatteryFilename);
		if (LoadState(pszStateFilename, false))
		{
			LoadBattery(pszBatteryFilename);
			Reset();
		}
	}
	else
	{
		LoadBattery(pszBatteryFilename);
		Reset();
	}

	if (pDebugInfo = new CDebugInfo())
	{
		if (pDebugInfo->LoadFile(pszROMFilename))
		{
			delete pDebugInfo;
			pDebugInfo = NULL;
		}
	}

	SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, (LPARAM)&Maximized);

	//Create window
	if (!(hGBWnd = CreateWindowEx(WS_EX_MDICHILD, "Game Boy", Rom, Maximized ? WS_VISIBLE | WS_MAXIMIZE : WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 160 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), hClientWnd, NULL, hInstance, NULL)))
	{
		DisplayErrorMessage(hWnd);
		return true;
	}
	SetWindowLong(hGBWnd, GWL_USERDATA, (long)this);

	strcpy(szStatusText, Rom);
	strcat(szStatusText, " loaded");
	SetStatus(szStatusText, SF_MESSAGE);

	return false;
}



BOOL CGameBoy::LoadBattery(char *BatteryFilename)
{
	HANDLE		hFile;
	DWORD		nBytes;
	char		szStatusText[MAX_PATH + 16];


	if (!SaveRamSize)
	{
		return false;
	}

	ZeroMemory(&MEM_RAM, sizeof(MEM_RAM));

	if (BatteryFilename)
	{
		strcpy(Battery, BatteryFilename);
	}
	else
	{
		Battery[0] = '\0';
		Reset();
		return false;
	}

	GetBatteryFilename(Battery);

	if ((hFile = CreateFile(Battery, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
	{
		if (!ReadFile(hFile, &MEM_RAM, SaveRamSize, &nBytes, NULL) || nBytes != SaveRamSize)
		{
			CloseHandle(hFile);

			strcpy(szStatusText, "Error loading battery RAM from ");
			strcat(szStatusText, Battery);
			SetStatus(szStatusText, SF_MESSAGE);

			ZeroMemory(&MEM_RAM, sizeof(MEM_RAM));
			BatteryAvailable = false;
			Battery[0] = 0;

			Reset();
			return true;
		}
		CloseHandle(hFile);
	}
	else
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			strcpy(szStatusText, Battery);
			strcat(szStatusText, " does not exist");
			SetStatus(szStatusText, SF_MESSAGE);
		}
		else
		{
			strcpy(szStatusText, "Couldn't open file ");
			strcat(szStatusText, Battery);
			SetStatus(szStatusText, SF_MESSAGE);
		}
		return true;
	}

	strcpy(szStatusText, "Battery RAM loaded from ");
	strcat(szStatusText, Battery);
	SetStatus(szStatusText, SF_MESSAGE);

	Reset();
	return false;
}



BOOL CGameBoy::SaveBattery(BOOL Prompt, BOOL SaveAs)
{
	HANDLE			hFile;
	DWORD			nBytes;
	BYTE			*Buffer;
	OPENFILENAME	of;
	char			szStatusText[MAX_PATH + 16];


	if (!SaveRamSize || (Prompt && !BatteryAvailable))
	{
		return false;
	}

	if (!Battery[0])
	{
		strcpy(Battery, Rom);
		if (strchr(Battery, '.'))
		{
			*strrchr(Battery, '.') = 0;
		}
		strcat(Battery, ".sav");
		if (Prompt)
		{
			switch (MessageBox(hWnd, "Save battery ram?", "Game Lad", MB_YESNOCANCEL | MB_ICONQUESTION))
			{
			case IDNO:
				return false;

			case IDCANCEL:
				return true;
			}
		}
		ZeroMemory(&of, sizeof(of));
		of.lStructSize = sizeof(of);
		of.hwndOwner = hWnd;
		of.lpstrFilter = "Game Boy Battery Ram (*.sav)\0*.SAV\0All files (*.*)\0*.*\0";
		of.lpstrFile = Battery;
		of.nMaxFile = sizeof(Battery);
		of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		of.lpstrDefExt = "sav";
		if (!GetSaveFileName(&of))
		{
			Battery[0] = 0;
			return true;
		}
		Prompt = false;
	}
	else
	{
		if (SaveAs)
		{
			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWnd;
			of.lpstrFilter = "Game Boy Battery Ram (*.sav)\0*.SAV\0All files (*.*)\0*.*\0";
			//of.nFilterIndex = 1;
			of.lpstrFile = Battery;
			of.nMaxFile = sizeof(Battery);
			of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
			of.lpstrDefExt = "sav";
			if (!GetSaveFileName(&of))
			{
				return true;
			}
		}
	}
	if (Prompt && !SaveAs)
	{
		//Compare file with RAM
		if ((hFile = CreateFile(Battery, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
		{
			if (Buffer = new BYTE[SaveRamSize])
			{
				if (ReadFile(hFile, Buffer, SaveRamSize, &nBytes, NULL))
				{
					if (!memcmp(Buffer, &MEM_RAM, SaveRamSize))
					{
						//File same as RAM
						delete Buffer;
						CloseHandle(hFile);
						return false;
					}
				}

				delete Buffer;
			}
			CloseHandle(hFile);
		}

		switch (MessageBox(hWnd, "Save battery ram?", "Game Lad", MB_YESNOCANCEL | MB_ICONQUESTION))
		{
		case IDNO:
			return false;

		case IDCANCEL:
			return true;
		}
	}

	while ((hFile = CreateFile(Battery, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (MessageBox(hWnd, "Couldn't create file.", NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			strcpy(szStatusText, "Couldn't create file ");
			strcat(szStatusText, Battery);
			SetStatus(szStatusText, SF_MESSAGE);
			return true;
		}
	}

	while (!WriteFile(hFile, &MEM_RAM, SaveRamSize, &nBytes, NULL) || nBytes != SaveRamSize)
	{
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		SetEndOfFile(hFile);
		if (MessageBox(hWnd, "Error while saving.", NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			CloseHandle(hFile);
			DeleteFile(Battery);
			strcpy(szStatusText, "Error saving battery RAM to ");
			strcat(szStatusText, Battery);
			SetStatus(szStatusText, SF_MESSAGE);
			return true;
		}
	}

	CloseHandle(hFile);

	strcpy(szStatusText, "Battery RAM saved to ");
	strcat(szStatusText, Battery);
	SetStatus(szStatusText, SF_MESSAGE);

	return false;
}



void CGameBoy::GetBatteryFilename(char *Filename)
{
	if (Battery[0])
	{
		if (Filename != Battery)
		{
			strcpy(Filename, Battery);
		}
	}
	else
	{
		strcpy(Filename, Rom);
		if (strchr(Filename, '.'))
		{
			*strrchr(Filename, '.') = 0;
		}
		strcat(Filename, ".sav");
	}
}



HDC				hPreviewDC;
HBITMAP			hPreviewBmp, hOldPreviewBmp;
WORD			*pPreviewBmp;
BOOL			PreviewFileSelected;
HWND			hPreviewBmpWnd;
WNDPROC			OldStaticWndProc;



LPARAM CALLBACK BitmapWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	HBRUSH			hBrush;
	RECT			rct;


	switch (uMsg)
	{
	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
			rct.left = 0;
			rct.top = 0;
			rct.right = 162;
			rct.bottom = 146;
			FrameRect(Paint.hdc, &rct, hBrush);
			if (hPreviewDC && PreviewFileSelected)
			{
				BitBlt(Paint.hdc, 1, 1, 160, 144, hPreviewDC, 0, 0, SRCCOPY);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;
	}

	return CallWindowProc(OldStaticWndProc, hWin, uMsg, wParam, lParam);
}



UINT CALLBACK StateOFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	BITMAPINFO		bmi;
	HANDLE			hFile;
	char			szFilename[MAX_PATH], c;
	DWORD			nBytes, y, Value;


	switch (uiMsg)
	{
	case WM_NOTIFY:
		switch (((OFNOTIFY *)lParam)->hdr.code)
		{
		/*case CDN_FILEOK:
			break;*/

		case CDN_SELCHANGE:
			PreviewFileSelected = false;
			InvalidateRect(hPreviewBmpWnd, NULL, true);
			if (!SendMessage(GetParent(hdlg), CDM_GETFILEPATH, sizeof(szFilename), (long)&szFilename))
			{
				return 0;
			}
			if ((hFile = CreateFile(szFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				return 0;
			}
			if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != sizeof(Value))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (Value != ('g' | ('l' << 8) | ('s' << 16)))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != sizeof(Value))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (Value != 0)
			{
				CloseHandle(hFile);
				return 0;
			}
			do
			{
				if (!ReadFile(hFile, &c, 1, &nBytes, NULL))
				{
					CloseHandle(hFile);
					return 0;
				}
				if (nBytes != 1)
				{
					CloseHandle(hFile);
					return 0;
				}
			}
			while (c != '\0');
			if (!ReadFile(hFile, &Value, 2, &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != 2)
			{
				CloseHandle(hFile);
				return 0;
			}
			for (y = 0; y < 144; y++)
			{
				if (!ReadFile(hFile, pPreviewBmp + y * 160, 160 * 2, &nBytes, NULL))
				{
					CloseHandle(hFile);
					return 0;
				}
				if (nBytes != 160 * 2)
				{
					CloseHandle(hFile);
					return 0;
				}
			}
			CloseHandle(hFile);
			PreviewFileSelected = true;
			break;
		}
		break;

	case WM_INITDIALOG:
		SetWindowLong(hdlg, GWL_STYLE, GetWindowLong(hdlg, GWL_STYLE) | WS_CLIPSIBLINGS);
		MoveWindow(hdlg, 0, 0, 167, 149, true);

		PreviewFileSelected = false;

		if (!(hPreviewDC = CreateCompatibleDC(NULL)))
		{
			return 0;
		}
		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = 160;
		bmi.bmiHeader.biHeight = -144;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		if (!(hPreviewBmp = CreateDIBSection(hPreviewDC, &bmi, DIB_RGB_COLORS, (void **)&pPreviewBmp, NULL, 0)))
		{
			ReleaseDC(NULL, hPreviewDC);
			hPreviewDC = NULL;
		}
		hOldPreviewBmp = (HBITMAP)SelectObject(hPreviewDC, hPreviewBmp);

		hPreviewBmpWnd = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BLACKFRAME, 5, 0, 162, 146, hdlg, NULL, hInstance, NULL);
		OldStaticWndProc = (WNDPROC)SetWindowLong(hPreviewBmpWnd, GWL_WNDPROC, (long)BitmapWndProc);
		break;
	}

	SetWindowLong(GetParent(hdlg), DWL_MSGRESULT, 0);
	return 0;
}



char *CGameBoy::GetStateFilename(char *szFilename)
{
	strcpy(szFilename, Rom);
	if (strchr(szFilename, '.'))
	{
		*strrchr(szFilename, '.') = '\0';
	}
	szFilename[strlen(szFilename) + 1] = '\0';
	szFilename[strlen(szFilename)] = StateSlot + '0';
	strcat(szFilename, ".gls");

	return szFilename;
}



BOOL CGameBoy::SaveState()
{
	char		szFilename[MAX_PATH + 5];

	return SaveState(GetStateFilename(szFilename), false);
}



BOOL CGameBoy::SaveStateAs()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH];


	Stop();

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	of.lpstrFilter = "Game Lad Save States (*.gls)\0*.GLS\0All files (*.*)\0*.*\0";
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	of.lpfnHook = StateOFNHookProc;
	szFilename[0] = 0;
	if (!GetSaveFileName(&of))
	{
		Resume();
		return false;
	}

	return SaveState(szFilename, true);
}



#define		WriteToFile(source)										\
	if (!WriteFile(hFile, &source, sizeof(source), &nBytes, NULL))	\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		strcpy(szBuffer, "Error saving state to ");					\
		strcat(szBuffer, pszFilename);								\
		SetStatus(szBuffer, SF_MESSAGE);							\
		Resume();													\
		return true;												\
	}																\
	if (nBytes != sizeof(source))									\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		strcpy(szBuffer, "Error saving state to ");					\
		strcat(szBuffer, pszFilename);								\
		SetStatus(szBuffer, SF_MESSAGE);							\
		Resume();													\
		return true;												\
	}

#define		WriteToFile2(source, size)								\
	if (!WriteFile(hFile, &source, (size), &nBytes, NULL))			\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		strcpy(szBuffer, "Error saving state to ");					\
		strcat(szBuffer, pszFilename);								\
		SetStatus(szBuffer, SF_MESSAGE);							\
		Resume();													\
		return true;												\
	}																\
	if (nBytes != (size))											\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		strcpy(szBuffer, "Error saving state to ");					\
		strcat(szBuffer, pszFilename);								\
		SetStatus(szBuffer, SF_MESSAGE);							\
		Resume();													\
		return true;												\
	}

BOOL CGameBoy::SaveState(char *pszFilename, BOOL AlreadyStopped)
{
	HANDLE		hFile;
	DWORD		nBytes, Value, y;
	char		szBuffer[MAX_PATH + 16];


	if (!AlreadyStopped)
	{
		Stop();
	}

	if ((hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		strcpy(szBuffer, "Couldn't create file ");
		strcat(szBuffer, pszFilename);
		Resume();
		return true;
	}

	Value = 'g' | ('l' << 8) | ('s' << 16);
	WriteToFile(Value);
	Value = GAME_LAD_SAVE_STATE_VERSION;
	WriteToFile(Value);
	WriteToFile2("1.2", 4);

	WriteToFile(MaxRomBank);
	WriteToFile(MaxRamBank);

	for (y = 0; y < 144; y++)
	{
		if (!WriteFile(hFile, pGBBitmap + 7 + y * (160 + 14), 2 * 160, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DeleteFile(pszFilename);
			strcpy(szBuffer, "Error saving state to ");
			strcat(szBuffer, pszFilename);
			SetStatus(szBuffer, SF_MESSAGE);
			Resume();
			return true;
		}
		if (nBytes != 2 * 160)
		{
			CloseHandle(hFile);
			DeleteFile(pszFilename);
			strcpy(szBuffer, "Error saving state to ");
			strcat(szBuffer, pszFilename);
			SetStatus(szBuffer, SF_MESSAGE);
			Resume();
			return true;
		}
	}

	Value = Flags & (GB_DEBUGRUNINFO | GB_COLOR | GB_ROM_COLOR | GB_RAMENABLE | GB_HALT | GB_ENABLEIE | GB_IE | GB_HDMA);
	WriteToFile(Value);

	WriteToFile(Reg_AF);
	WriteToFile(Reg_BC);
	WriteToFile(Reg_DE);
	WriteToFile(Reg_HL);
	WriteToFile(Reg_SP);
	WriteToFile(Reg_PC);
	WriteToFile(ActiveRomBank);
	WriteToFile(ActiveRamBank);
	WriteToFile(MEM_CPU);
	WriteToFile2(*MEM_RAM, SaveRamSize);
	WriteToFile(MEM_VRAM);
	WriteToFile(BGP);
	WriteToFile(OBP);

	if (Flags & GB_DEBUGRUNINFO)
	{
		WriteToFile2(*MemStatus_ROM, 16384 * (DWORD)RomSize(MEM_ROM[0x148]));
		WriteToFile(MemStatus_CPU);
		WriteToFile2(*MemStatus_RAM, SaveRamSize);
		WriteToFile(MemStatus_VRAM);
	}

	WriteToFile(LCD_Ticks);
	WriteToFile(DIV_Ticks);
	WriteToFile(TIMA_Ticks);
	WriteToFile(Hz);
	WriteToFile(WindowY);
	WriteToFile(WindowY2);
	WriteToFile(DrawLineMask);
	WriteToFile(SoundTicks);
	WriteToFile(Sound1Enabled);
	WriteToFile(Sound2Enabled);
	WriteToFile(Sound3Enabled);
	WriteToFile(Sound4Enabled);
	WriteToFile(Sound1Stage);
	WriteToFile(Sound2Stage);
	WriteToFile(Sound3Stage);
	WriteToFile(Sound4Bit);
	WriteToFile(Sound1Volume);
	WriteToFile(Sound2Volume);
	WriteToFile(Sound4Volume);
	WriteToFile(Sound1Ticks);
	WriteToFile(Sound1TimeOut);
	WriteToFile(Sound1Frequency);
	WriteToFile(Sound1Envelope);
	WriteToFile(Sound1Sweep);
	WriteToFile(Sound2Ticks);
	WriteToFile(Sound2TimeOut);
	WriteToFile(Sound2Frequency);
	WriteToFile(Sound2Envelope);
	WriteToFile(Sound3Ticks);
	WriteToFile(Sound3TimeOut);
	WriteToFile(Sound3Frequency);
	WriteToFile(Sound4Ticks);
	WriteToFile(Sound4TimeOut);
	WriteToFile(Sound4Frequency);
	WriteToFile(Sound4Envelope);

	CloseHandle(hFile);

	strcpy(szBuffer, "State saved to ");
	strcat(szBuffer, pszFilename);
	SetStatus(szBuffer, SF_MESSAGE);

	Resume();

	return false;
}



BOOL CGameBoy::LoadStateAs()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH];


	Stop();

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	of.lpstrFilter = "Game Lad Save States (*.gls)\0*.GLS\0All files (*.*)\0*.*\0";
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	of.lpfnHook = StateOFNHookProc;
	szFilename[0] = 0;
	if (!GetOpenFileName(&of))
	{
		Resume();
		return false;
	}

	return LoadState(szFilename, true);
}



BOOL CGameBoy::LoadState()
{
	char		szFilename[MAX_PATH + 5];

	return LoadState(GetStateFilename(szFilename), false);
}



#define		ReadFromFile(dest)									\
	if (!ReadFile(hFile, &dest, sizeof(dest), &nBytes, NULL))	\
	{															\
		CloseHandle(hFile);										\
		strcpy(szBuffer, "Error loading state from ");			\
		strcat(szBuffer, pszFilename);							\
		SetStatus(szBuffer, SF_MESSAGE);						\
		Resume();												\
		return true;											\
	}															\
	if (nBytes != sizeof(dest))									\
	{															\
		CloseHandle(hFile);										\
		strcpy(szBuffer, "Error loading state from ");			\
		strcat(szBuffer, pszFilename);							\
		SetStatus(szBuffer, SF_MESSAGE);						\
		Resume();												\
		return true;											\
	}

#define		ReadFromFile2(dest, size)							\
	if (!ReadFile(hFile, &dest, (size), &nBytes, NULL))			\
	{															\
		CloseHandle(hFile);										\
		Reset();												\
		strcpy(szBuffer, "Error loading state from ");			\
		strcat(szBuffer, pszFilename);							\
		SetStatus(szBuffer, SF_MESSAGE);						\
		return true;											\
	}															\
	if (nBytes != (size))										\
	{															\
		CloseHandle(hFile);										\
		Reset();												\
		strcpy(szBuffer, "Error loading state from ");			\
		strcat(szBuffer, pszFilename);							\
		SetStatus(szBuffer, SF_MESSAGE);						\
		return true;											\
	}

BOOL CGameBoy::LoadState(char *pszFilename, BOOL AlreadyStopped)
{
	HANDLE		hFile;
	DWORD		nBytes, Value, y, Pos;
	char		szBuffer[0x120], szPath[MAX_PATH + 2];
	int			Slot1, Slot2;
	FILETIME	FileTime1, FileTime2;


	if (!AlreadyStopped)
	{
		Stop();
	}


	//Load latest modified saved state
	if (pszFilename[0] == '\0')
	{
		strcpy(szPath, Rom);
		if (strchr(szPath, '.'))
		{
			*strrchr(szPath, '.') = '\0';
		}
		strcat(szPath, "0.gls");
		Slot1 = -1;
		for (Slot2 = 0; Slot2 <= 9; Slot2++)
		{
			szPath[strlen(szPath) - 5] = Slot2 + '0';
			if ((hFile = CreateFile(szPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
			{
				GetFileTime(hFile, NULL, NULL, &FileTime2);
				if (Slot1 >= 0)
				{
					if (FileTime2.dwHighDateTime > FileTime1.dwHighDateTime || (FileTime2.dwHighDateTime == FileTime1.dwHighDateTime && FileTime2.dwLowDateTime > FileTime1.dwLowDateTime))
					{
						Slot1 = Slot2;
						FileTime1.dwHighDateTime = FileTime2.dwHighDateTime;
						FileTime1.dwLowDateTime = FileTime2.dwLowDateTime;
					}
				}
				else
				{
					Slot1 = Slot2;
					FileTime1.dwHighDateTime = FileTime2.dwHighDateTime;
					FileTime1.dwLowDateTime = FileTime2.dwLowDateTime;
				}
				CloseHandle(hFile);
			}
		}
		if (Slot1 >= 0)
		{
			szPath[strlen(szPath) - 5] = Slot1 + '0';
			pszFilename = szPath;
			StateSlot = Slot1;
		}
		else
		{
			return true;
		}
	}


	if ((hFile = CreateFile(pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			strcpy(szBuffer, pszFilename);
			strcat(szBuffer, " does not exist.");
			SetStatus(szBuffer, SF_MESSAGE);
			Resume();
			return true;
		}
		strcpy(szBuffer, "Couldn't open file ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}

	if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	if (nBytes != sizeof(Value))
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	if (Value != ('g' | ('l' << 8) | ('s' << 16)))
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	if (nBytes != sizeof(Value))
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	strcpy(szBuffer, pszFilename);
	strcat(szBuffer, " requires Game Lad ");
	Pos = strlen(szBuffer);
	do
	{
		if (Pos < sizeof(szBuffer) - 12)
		{
			if (!ReadFile(hFile, &szBuffer[Pos++], sizeof(szBuffer[Pos]), &nBytes, NULL))
			{
				CloseHandle(hFile);
				strcpy(szBuffer, "Error loading state from ");
				strcat(szBuffer, pszFilename);
				SetStatus(szBuffer, SF_MESSAGE);
				Resume();
				return true;
			}
			if (nBytes != sizeof(szBuffer[Pos]))
			{
				CloseHandle(hFile);
				strcpy(szBuffer, "Error loading state from ");
				strcat(szBuffer, pszFilename);
				SetStatus(szBuffer, SF_MESSAGE);
				Resume();
				return true;
			}
			szBuffer[Pos] = '\0';
		}
		else
		{
			CloseHandle(hFile);
			strcpy(szBuffer, "Error loading state from ");
			strcat(szBuffer, pszFilename);
			SetStatus(szBuffer, SF_MESSAGE);
			Resume();
			return true;
		}
	}
	while (szBuffer[Pos - 1] != '\0');
	if (Value > GAME_LAD_SAVE_STATE_VERSION)
	{
		CloseHandle(hFile);
		strcat(szBuffer, " or higher.");
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}

	ReadFromFile2(Value, 1);
	if (MaxRomBank != (BYTE)Value)
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}
	ReadFromFile2(Value, 1);
	if (MaxRamBank != (BYTE)Value)
	{
		CloseHandle(hFile);
		strcpy(szBuffer, "Error loading state from ");
		strcat(szBuffer, pszFilename);
		SetStatus(szBuffer, SF_MESSAGE);
		Resume();
		return true;
	}

	for (y = 0; y < 144; y++)
	{
		if (!ReadFile(hFile, pGBBitmap + 7 + y * (160 + 14), 2 * 160, &nBytes, NULL))
		{
			CloseHandle(hFile);
			Reset();
			strcpy(szBuffer, "Error loading state from ");
			strcat(szBuffer, pszFilename);
			SetStatus(szBuffer, SF_MESSAGE);
			return true;
		}
		if (nBytes != 2 * 160)
		{
			CloseHandle(hFile);
			Reset();
			strcpy(szBuffer, "Error loading state from ");
			strcat(szBuffer, pszFilename);
			SetStatus(szBuffer, SF_MESSAGE);
			return true;
		}
	}

	ReadFromFile(Flags);

	ReadFromFile(Reg_AF);
	ReadFromFile(Reg_BC);
	ReadFromFile(Reg_DE);
	ReadFromFile(Reg_HL);
	ReadFromFile(Reg_SP);
	ReadFromFile(Reg_PC);
	ReadFromFile(ActiveRomBank);
	ReadFromFile(ActiveRamBank);
	ReadFromFile(MEM_CPU);
	ReadFromFile2(*MEM_RAM, SaveRamSize);
	ReadFromFile(MEM_VRAM);
	ReadFromFile(BGP);
	ReadFromFile(OBP);

	if (Flags & GB_DEBUGRUNINFO)
	{
		ReadFromFile2(*MemStatus_ROM, 16384 * (DWORD)RomSize(MEM_ROM[0x148]));
		ReadFromFile(MemStatus_CPU);
		ReadFromFile2(*MemStatus_RAM, SaveRamSize);
		ReadFromFile(MemStatus_VRAM);
	}

	ReadFromFile(LCD_Ticks);
	ReadFromFile(DIV_Ticks);
	ReadFromFile(TIMA_Ticks);
	ReadFromFile(Hz);
	ReadFromFile(WindowY);
	ReadFromFile(WindowY2);
	ReadFromFile(DrawLineMask);
	ReadFromFile(SoundTicks);
	ReadFromFile(Sound1Enabled);
	ReadFromFile(Sound2Enabled);
	ReadFromFile(Sound3Enabled);
	ReadFromFile(Sound4Enabled);
	ReadFromFile(Sound1Stage);
	ReadFromFile(Sound2Stage);
	ReadFromFile(Sound3Stage);
	ReadFromFile(Sound4Bit);
	ReadFromFile(Sound1Volume);
	ReadFromFile(Sound2Volume);
	ReadFromFile(Sound4Volume);
	ReadFromFile(Sound1Ticks);
	ReadFromFile(Sound1TimeOut);
	ReadFromFile(Sound1Frequency);
	ReadFromFile(Sound1Envelope);
	ReadFromFile(Sound1Sweep);
	ReadFromFile(Sound2Ticks);
	ReadFromFile(Sound2TimeOut);
	ReadFromFile(Sound2Frequency);
	ReadFromFile(Sound2Envelope);
	ReadFromFile(Sound3Ticks);
	ReadFromFile(Sound3TimeOut);
	ReadFromFile(Sound3Frequency);
	ReadFromFile(Sound4Ticks);
	ReadFromFile(Sound4TimeOut);
	ReadFromFile(Sound4Frequency);
	ReadFromFile(Sound4Envelope);

	CloseHandle(hFile);

	InvalidateRect(hGBWnd, NULL, false);

	strcpy(szBuffer, "State loaded from ");
	strcat(szBuffer, pszFilename);
	SetStatus(szBuffer, SF_MESSAGE);

	BatteryAvailable = true;

	Resume();

	return false;
}



void CGameBoy::SetStateSlot(int nSlot)
{
	char		szStatusText[0x100];


	if (nSlot >= 0)
	{
		if (nSlot <= 9)
		{
			StateSlot = nSlot;
		}
		else
		{
			StateSlot = 9;
		}
	}
	else
	{
		StateSlot = 0;
	}

	strcpy(szStatusText, "State slot   selected");
	szStatusText[11] = StateSlot + '0';
	SetStatus(szStatusText, SF_MESSAGE);
}



int CGameBoy::GetStateSlot()
{
	return StateSlot;
}



LPARAM CGameBoy::GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	RECT			rct;
	POINT			Pt;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_VIEW_ZOOM_100:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_200:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 2 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 2 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_300:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 3 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 3 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_400:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 4 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 4 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;
		}
		break;

	case WM_MDIACTIVATE:
		if (hGBWnd != (HWND)lParam)
		{
			DirectionKeys = 0;
			Buttons = 0;
			FastFwd = false;
		}
		return 0;

	case WM_ERASEBKGND:
		//Picture covers whole window, no need to erase background.
		return 1;

	case WM_PAINT:
		if (GetUpdateRect(hGBWnd, NULL, true))
		{
			BeginPaint(hGBWnd, &Paint);
			GetClientRect(hGBWnd, &rct);
			if (rct.right == 160 && rct.bottom == 144)
			{
				BitBlt(Paint.hdc, Paint.rcPaint.left, Paint.rcPaint.top, rct.right, rct.bottom, hGBDC, Paint.rcPaint.left + 7, Paint.rcPaint.top, SRCCOPY);
			}
			else
			{
				StretchBlt(Paint.hdc, 0, 0, rct.right, rct.bottom, hGBDC, 7, 0, 160, 144, SRCCOPY);
			}
			EndPaint(hGBWnd, &Paint);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam == Keys.Down)
		{
			DirectionKeys |= 0x08;
			return 0;
		}
		if (wParam == Keys.Up)
		{
			DirectionKeys |= 0x04;
			return 0;
		}
		if (wParam == Keys.Left)
		{
			DirectionKeys |= 0x02;
			return 0;
		}
		if (wParam == Keys.Right)
		{
			DirectionKeys |= 0x01;
			return 0;
		}
		if (wParam == Keys.Start)
		{
			Buttons |= 0x08;
			/*if (!(FF00_C(0) & 0x20))
			{
				FF00_C(0) |= 0x08;
			}*/
			return 0;
		}
		if (wParam == Keys.Select)
		{
			Buttons |= 0x04;
			return 0;
		}
		if (wParam == Keys.A)
		{
			Buttons |= 0x01;
			return 0;
		}
		if (wParam == Keys.B)
		{
			Buttons |= 0x02;
			return 0;
		}
		if (wParam == Keys.FastForward)
		{
			FastFwd = true;
			SoundL = SoundR = 0;
		}
		if (wParam == VK_ADD)
		{
			if (FrameSkip < 9)
			{
				SetFrameSkip(FrameSkip + 1);
			}
		}
		if (wParam == VK_SUBTRACT)
		{
			if (FrameSkip > 0)
			{
				SetFrameSkip(FrameSkip - 1);
			}
		}
		return 0;

	case WM_KEYUP:
		if (wParam == Keys.Down)
		{
			DirectionKeys &= ~0x08;
			return 0;
		}
		if (wParam == Keys.Up)
		{
			DirectionKeys &= ~0x04;
			return 0;
		}
		if (wParam == Keys.Left)
		{
			DirectionKeys &= ~0x02;
			return 0;
		}
		if (wParam == Keys.Right)
		{
			DirectionKeys &= ~0x01;
			return 0;
		}
		if (wParam == Keys.Start)
		{
			Buttons &= ~0x08;
			return 0;
		}
		if (wParam == Keys.Select)
		{
			Buttons &= ~0x04;
			return 0;
		}
		if (wParam == Keys.A)
		{
			Buttons &= ~0x01;
			return 0;
		}
		if (wParam == Keys.B)
		{
			Buttons &= ~0x02;
			return 0;
		}
		if (wParam == Keys.FastForward)
		{
			FastFwd = false;
			SoundL = SoundR = 0;
		}
		return 0;
	}

	return DefMDIChildProc(hGBWnd, uMsg, wParam, lParam);
}



void CGameBoy::Reset(DWORD Flags)
{
	this->Flags = (Flags & GB_COLOR) || (this->Flags & GB_DEBUG);
	Reset();
}



void CGameBoy::Reset()
{
	DWORD		RamSize, pByte;


	ZeroMemory(&MEM_CPU, sizeof(MEM_CPU));
	ZeroMemory(&MEM_VRAM, sizeof(MEM_VRAM));
	ZeroMemory(&BGP, sizeof(BGP));
	ZeroMemory(&OBP, sizeof(OBP));

	Flags &= GB_COLOR | GB_DEBUG;
	if (Flags & GB_COLOR && MEM_ROM[0x0143] & 0x80)
	{
		Reg_AF = 0x11B0;
		Flags |= GB_ROM_COLOR;
	}
	else
	{
		Reg_AF = 0x01B0;
	}
	Reg_BC = 0x0013;
	Reg_DE = 0x00D8;
	Reg_HL = 0x014D;
	Reg_SP = 0xFFFE;
	Reg_PC = 0x0100;

	ActiveRomBank = 1;
	ActiveRamBank = 0;

	//DrawLineMask = 0;

	DIV_Ticks = 0;
	Hz = 2048;
	//SIOClocks = 0;

	FF00_C(0x40) = 0x91;
	FF00_C(0x41) = 0x80;
	LCD_Ticks = 102;

	MEM[0x0] = &MEM_ROM[0x000000];
	MEM[0x1] = &MEM_ROM[0x001000];
	MEM[0x2] = &MEM_ROM[0x002000];
	MEM[0x3] = &MEM_ROM[0x003000];
	MEM[0x4] = &MEM_ROM[0x004000];
	MEM[0x5] = &MEM_ROM[0x005000];
	MEM[0x6] = &MEM_ROM[0x006000];
	MEM[0x7] = &MEM_ROM[0x007000];
	MEM[0x8] = &MEM_VRAM[0x0000];
	MEM[0x9] = &MEM_VRAM[0x1000];
	MEM[0xA] = &MEM_RAM[0x0000];
	MEM[0xB] = &MEM_RAM[0x1000];
	MEM[0xC] = &MEM_CPU[0x0000];
	MEM[0xD] = &MEM_CPU[0x1000];
	MEM[0xE] = &MEM_CPU[0x0000];
	MEM[0xF] = &MEM_CPU[0x8000];

	FillMemory(pGBBitmap, (160 + 14) * 144 * 2, 0xFF);


	MemStatus[0x0] = pMemStatus_ROM(0x000000);
	MemStatus[0x1] = pMemStatus_ROM(0x001000);
	MemStatus[0x2] = pMemStatus_ROM(0x002000);
	MemStatus[0x3] = pMemStatus_ROM(0x003000);
	MemStatus[0x4] = pMemStatus_ROM(0x004000);
	MemStatus[0x5] = pMemStatus_ROM(0x005000);
	MemStatus[0x6] = pMemStatus_ROM(0x006000);
	MemStatus[0x7] = pMemStatus_ROM(0x007000);
	MemStatus[0x8] = &MemStatus_VRAM[0x0000];
	MemStatus[0x9] = &MemStatus_VRAM[0x1000];
	/*if (MemStatus_RAM)
	{
		MemStatus[0xA] = MemStatus_RAM;
		if (MEM_ROM[0x0149] != 1)
		{
			MemStatus[0xB] = MemStatus_RAM + 0x1000;
		}
		else
		{
			MemStatus[0xB] = ZeroStatus;
		}
	}
	else*/
	{
		MemStatus[0xA] = ZeroStatus;
		MemStatus[0xB] = ZeroStatus;
	}
	MemStatus[0xC] = &MemStatus_CPU[0x0000];
	MemStatus[0xD] = &MemStatus_CPU[0x1000];
	MemStatus[0xE] = ZeroStatus;
	MemStatus[0xF] = &MemStatus_CPU[0x8000];

	//FillMemory(MemStatus_ROM, 16384 * RomSize(MEM_ROM[0x148]), MEM_WRITE | MEM_READ | MEM_EXECUTE);
	for (pByte = 0; pByte < (unsigned)16384 * (RomSize(MEM_ROM[0x148]) + 1); pByte++)
	{
		MemStatus_ROM[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
	}

	//VRAM
	//FillMemory(MemStatus_VRAM, sizeof(MemStatus_VRAM), MEM_WRITE | MEM_READ);
	for (pByte = 0; pByte < sizeof(MemStatus_VRAM); pByte++)
	{
		MemStatus_VRAM[pByte] |= MEM_WRITE | MEM_READ;
		MemStatus_VRAM[pByte] &= ~MEM_CHANGED;
	}

	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		ZeroMemory(&MemStatus_RAM[0x800], 0x1000 - 0x800);
		break;

	case 2:
		RamSize = 0x2000;
		break;

	case 3:
		RamSize = 0x8000;
		break;

	case 4:
		RamSize = 0x20000;
		break;

	default:
		//No external RAM
		RamSize = 0;
		break;
	}

	if (SaveRamSize)
	{
		//External Battery RAM
		//FillMemory(MemStatus_RAM, RamSize, MEM_WRITE | MEM_READ | MEM_EXECUTE);
		for (pByte = 0; pByte < RamSize; pByte++)
		{
			MemStatus_RAM[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
			MemStatus_RAM[pByte] &= ~MEM_CHANGED;
		}
	}
	else
	{
		//External RAM
		//FillMemory(MemStatus_RAM, RamSize, MEM_WRITE | MEM_EXECUTE);
		for (pByte = 0; pByte < RamSize; pByte++)
		{
			MemStatus_RAM[pByte] |= MEM_WRITE | MEM_EXECUTE;
			MemStatus_RAM[pByte] &= ~(MEM_CHANGED | MEM_READ);
		}
	}

	//Internal RAM
	//FillMemory(MemStatus_CPU, 0x8000, MEM_WRITE | MEM_EXECUTE);
	for (pByte = 0; pByte < 0x8000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~(MEM_CHANGED | MEM_READ);
	}
	//reserved
	ZeroMemory(&MemStatus_CPU[0x8000], 0x0E00);
	//Sprite RAM
	//FillMemory(&MemStatus_CPU[0x8E00], 0xA0, MEM_WRITE | MEM_READ);
	for (pByte = 0x8E00; pByte < 0x8EA0; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}
	//Reserved
	ZeroMemory(&MemStatus_CPU[0x8EA0], 0x60);
	//IO ports
	//FillMemory(&MemStatus_CPU[0x8F00], 0x80, MEM_WRITE | MEM_READ);
	for (pByte = 0x8F00; pByte < 0x8F80; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_READ;
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}
	//HiRAM
	//FillMemory(&MemStatus_CPU[0x8F80], 0x80, MEM_WRITE | MEM_EXECUTE);
	for (pByte = 0x8F80; pByte < 0x9000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~(MEM_CHANGED | MEM_READ);
	}
	//Interrupt
	MemStatus_CPU[0x8FFF] = MEM_WRITE | MEM_READ;

	Flags |= GB_DEBUGRUNINFO;

	RefreshScreen();

	SendMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
}



void CGameBoy::SetFrameSkip(int nFrameSkip)
{
	char		szStatus[24];


	if (nFrameSkip >= 0)
	{
		if (nFrameSkip <= 9)
		{
			FrameSkip = nFrameSkip;
		}
		else
		{
			FrameSkip = 9;
		}
	}
	else
	{
		FrameSkip = 0;
	}

	strcpy(szStatus, "Frame skip set to  ");
	szStatus[strlen(szStatus) - 1] = FrameSkip + '0';
	SetStatus(szStatus, SF_MESSAGE);
}



//#define		TIMEDEMO



void CGameBoy::ExecuteLoop()
{
	MSG				msg;

#ifdef TIMEDEMO
	LARGE_INTEGER	StartTime, CurrentTime, TimerFrequency;
	DWORD			Count = 0;
#endif //TEMIDEMO


	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}
	PrepareEmulation(false);
	MemoryFlags = DisAsmFlags = 0;

#ifdef TIMEDEMO
	QueryPerformanceFrequency(&TimerFrequency);
	QueryPerformanceCounter(&StartTime);
#endif //TIMEDEMO


	while (true)
	{
		MainLoop();
		if (Flags & GB_INVALIDOPCODE)
		{
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			CloseSound();
			RefreshScreen();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			MessageBox(hWnd, "Invalid operation code", "Game Lad", MB_OK | MB_ICONWARNING);
			return;
		}


		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_CLOSE || msg.message == WM_QUIT)
			{
				//Put back WM_QUIT to message que
				if (msg.message == WM_QUIT)
				{
					PostThreadMessage(ThreadId, WM_QUIT, msg.wParam, msg.lParam);
				}
				CloseSound();
				RefreshScreen();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			DispatchMessage(&msg);
		}


#ifndef TIMEDEMO
		Delay();
#endif

		RefreshScreen();


#ifdef TIMEDEMO
		QueryPerformanceCounter(&CurrentTime);
		if ((CurrentTime.QuadPart - StartTime.QuadPart) >= (TimerFrequency.QuadPart * 64))
		{
			CloseSound();
			RefreshScreen();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}

			char	NumBuffer2[10];
			MessageBox(hWnd, ultoa(Count, NumBuffer, 10), ultoa((DWORD)(CurrentTime.QuadPart - StartTime.QuadPart), NumBuffer2, 10), MB_OK | MB_ICONINFORMATION);
			return;
		}
		Count++;
#endif //TIMEDEMO
	}
}



void CGameBoy::DebugLoop()
{
	MSG				msg;


	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}
	PrepareEmulation(true);
	MemoryFlags = DisAsmFlags = 0;

	while (true)
	{
		DebugMainLoop();
		if (Flags & GB_INVALIDOPCODE)
		{
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			CloseSound();
			RefreshScreen();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			MessageBox(hWnd, "Invalid operation code", "Game Lad", MB_OK | MB_ICONWARNING);
			return;
		}

		if (Flags & GB_ERROR)
		{
			CloseSound();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			return;
		}


		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_CLOSE || msg.message == WM_QUIT)
			{
				//Put back WM_QUIT to message que
				if (msg.message == WM_QUIT)
				{
					PostThreadMessage(ThreadId, WM_QUIT, msg.wParam, msg.lParam);
				}
				CloseSound();
				RefreshScreen();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			DispatchMessage(&msg);
		}


		Delay();


		RefreshScreen();
	}
}



void CGameBoy::StepLoop(EMULATIONINFO *pEmulationInfo)
{
	MSG				msg;
	WORD			pByte;


	switch (pEmulationInfo->Flags)
	{
	case EMU_STEPINTO:
		pByte = Reg_PC;
		break;

	case EMU_STEPOUT:
		pByte = Reg_SP;
		break;
	}


	PrepareEmulation(true);
	MemoryFlags = DisAsmFlags = 0;

	Flags |= GB_EXITLOOP;
	DebugMainLoop();
	if (Flags & GB_INVALIDOPCODE)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
		CloseSound();
		RefreshScreen();
		if (GameBoys.GetActive() == this)
		{
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		}
		MessageBox(hWnd, "Invalid operation code", "Game Lad", MB_OK | MB_ICONWARNING);
		return;
	}

	if (Flags & GB_ERROR)
	{
		CloseSound();
		if (GameBoys.GetActive() == this)
		{
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		}
		return;
	}

	switch (pEmulationInfo->Flags)
	{
	case EMU_STEPINTO:
		if (pByte != Reg_PC)
		{
			CloseSound();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			return;
		}
		break;

	case EMU_RUNTO:
		if (pEmulationInfo->RunToOffset == Reg_PC)
		{
			if (Reg_PC >= 0x4000 && Reg_PC < 0x8000)
			{
				if (pEmulationInfo->RunToBank != ActiveRomBank)
				{
					break;
				}
			}
			if (Reg_PC >= 0xA000 && Reg_PC < 0xC000)
			{
				if (pEmulationInfo->RunToBank != ActiveRamBank)
				{
					break;
				}
			}
			if (Reg_PC >= 0xD000 && Reg_PC < 0xE000 && Flags & GB_ROM_COLOR)
			{
				if (FF00_C(0x70) & 7)
				{
					if (pEmulationInfo->RunToBank != (FF00_C(0x70) & 7))
					{
						break;
					}
				}
				else
				{
					if (pEmulationInfo->RunToBank != 1)
					{
						break;
					}
				}
			}
			CloseSound();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			return;
		}
		break;

	case EMU_STEPOUT:
		if (pByte < Reg_SP)
		{
			CloseSound();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			return;
		}
		break;
	}

	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}

	while (true)
	{
		Flags |= GB_EXITLOOP;
		DebugMainLoop();
		if (Flags & GB_INVALIDOPCODE)
		{
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			CloseSound();
			RefreshScreen();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			MessageBox(hWnd, "Invalid operation code", "Game Lad", MB_OK | MB_ICONWARNING);
			return;
		}

		if (Flags & GB_ERROR)
		{
			CloseSound();
			if (GameBoys.GetActive() == this)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			return;
		}

		switch (pEmulationInfo->Flags)
		{
		case EMU_STEPINTO:
			if (pByte != Reg_PC)
			{
				CloseSound();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			break;

		case EMU_RUNTO:
			if (pEmulationInfo->RunToOffset == Reg_PC)
			{
				if (Reg_PC >= 0x4000 && Reg_PC < 0x8000)
				{
					if (pEmulationInfo->RunToBank != ActiveRomBank)
					{
						break;
					}
				}
				if (Reg_PC >= 0xA000 && Reg_PC < 0xC000)
				{
					if (pEmulationInfo->RunToBank != ActiveRamBank)
					{
						break;
					}
				}
				if (Reg_PC >= 0xD000 && Reg_PC < 0xE000 && Flags & GB_ROM_COLOR)
				{
					if (FF00_C(0x70) & 7)
					{
						if (pEmulationInfo->RunToBank != (FF00_C(0x70) & 7))
						{
							break;
						}
					}
					else
					{
						if (pEmulationInfo->RunToBank != 1)
						{
							break;
						}
					}
				}
				CloseSound();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			break;

		case EMU_STEPOUT:
			if (pByte < Reg_SP)
			{
				CloseSound();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			break;
		}


		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_CLOSE || msg.message == WM_QUIT)
			{
				//Put back WM_QUIT to message que
				if (msg.message == WM_QUIT)
				{
					PostThreadMessage(ThreadId, WM_QUIT, msg.wParam, msg.lParam);
				}
				CloseSound();
				RefreshScreen();
				if (GameBoys.GetActive() == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				return;
			}
			DispatchMessage(&msg);
		}


		if (FF00_C(0x44) == 0x90 && (FF00_C(0x40) & 0x80))
		{
			Delay();

			RefreshScreen();
		}
	}
}



DWORD CGameBoy::ThreadProc()
{
	MSG				msg;
	EMULATIONINFO	EmulationInfo;
	BYTE			LastEmulationType = 0;


	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(hStartStopEvent);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch (msg.message)
		{
		case WM_COMMAND:
			switch (LOWORD(msg.wParam))
			{
			case ID_EMULATION_STARTDEBUG:
				Emulating = true;
				SetEvent(hStartStopEvent);
				LastEmulationType = 1;
				DebugLoop();
				Emulating = false;
				break;

			case ID_EMULATION_EXECUTE:
				Emulating = true;
				SetEvent(hStartStopEvent);
				LastEmulationType = 2;
				ExecuteLoop();
				Emulating = false;
				break;
			}
			break;

		case WM_APP_STEP:
			Emulating = true;
			SetEvent(hStartStopEvent);
			LastEmulationType = 3;
			if (msg.lParam)
			{
				EmulationInfo.Flags = ((EMULATIONINFO *)msg.lParam)->Flags;
				EmulationInfo.RunToBank = ((EMULATIONINFO *)msg.lParam)->RunToBank;
				EmulationInfo.RunToOffset = ((EMULATIONINFO *)msg.lParam)->RunToOffset;
			}
			StepLoop(&EmulationInfo);
			Emulating = false;
			break;

		case WM_APP_RESUME:
			switch (LastEmulationType)
			{
			case 1:
				PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_STARTDEBUG, 0);
				break;

			case 2:
				PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_EXECUTE, 0);
				break;

			case 3:
				PostThreadMessage(ThreadId, WM_APP_STEP, 0, 0);
				break;
			}
			break;

		default:
			DispatchMessage(&msg);
		}
	}

	hThread = NULL;

	return msg.wParam;
}



DWORD WINAPI GameBoyThreadProc(void *pGameBoy)
{
	return ((CGameBoy *)pGameBoy)->ThreadProc();
}



BOOL CGameBoy::StartThread()
{
	//Create a new thread if it doesn't exist
	if (!hThread)
	{
		ResetEvent(hStartStopEvent);

		if (!(hThread = CreateThread(NULL, 0, GameBoyThreadProc, this, 0, &ThreadId)))
		{
			return true;
		}

		//Wait for thread to initialize
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	return false;
}



BOOL CGameBoy::StartDebug()
{
	//Make sure a thread is running
	if (StartThread())
	{
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_STARTDEBUG, 0);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	return false;
}



BOOL CGameBoy::Execute()
{
	//Make sure a thread is running
	if (StartThread())
	{
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_EXECUTE, 0);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	return false;
}



BOOL CGameBoy::Step(EMULATIONINFO *pEmulationInfo)
{
	//Make sure a thread is running
	if (StartThread())
	{
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_APP_STEP, 0, (LPARAM)pEmulationInfo);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	return false;
}



void CGameBoy::Resume()
{
	if (hThread)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_APP_RESUME, 0, 0);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}
}



void CGameBoy::Stop()
{
	HANDLE		hTempThread;


	if (hTempThread = hThread)
	{
		if (Emulating)
		{
			PostThreadMessage(ThreadId, WM_CLOSE, 0, 0);
			if (GetCurrentThreadId() != ThreadId)
			{
				while (Emulating)
				{
					Sleep(0);
				}
			}
		}
		else
		{
			PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
			if (GetCurrentThreadId() != ThreadId)
			{
				WaitForSingleObject(hTempThread, INFINITE);
			}
		}
	}
}



BOOL CGameBoy::IsEmulating()
{
	return Emulating;
}



BOOL CGameBoy::HasBattery()
{
	return SaveRamSize ? true : false;
}



BOOL CGameBoy::SwitchRomBank(BYTE Bank)
{
	if (Emulating || Bank > MaxRomBank)
	{
		return true;
	}

	ActiveRomBank = Bank;

	MEM[0x4] = &MEM_ROM[0x0000 + 0x4000 * ActiveRomBank];
	MEM[0x5] = &MEM_ROM[0x1000 + 0x4000 * ActiveRomBank];
	MEM[0x6] = &MEM_ROM[0x2000 + 0x4000 * ActiveRomBank];
	MEM[0x7] = &MEM_ROM[0x3000 + 0x4000 * ActiveRomBank];

	return false;
}



BOOL CGameBoy::SwitchRamBank(BYTE Bank)
{
	if (Emulating || Bank > MaxRamBank)
	{
		return true;
	}

	ActiveRamBank = Bank;

	MEM[0xA] = &MEM_RAM[0x0000 + 0x2000 * ActiveRamBank];
	MEM[0xB] = &MEM_RAM[0x1000 + 0x2000 * ActiveRamBank];

	return false;
}



BOOL CGameBoy::SwitchVBK(BYTE Bank)
{
	if (Emulating || Bank > 1)
	{
		return true;
	}

	FF00_C(0x4F) = (FF00_C(0x4F) & ~1) | (Bank & 1);

	MEM[0x8] = &MEM_VRAM[0x0000 + 0x2000 * (FF00_C(0x4F) & 1)];
	MEM[0x9] = &MEM_VRAM[0x1000 + 0x2000 * (FF00_C(0x4F) & 1)];

	return false;
}



BOOL CGameBoy::SwitchSVBK(BYTE Bank)
{
	if (Emulating || Bank > 7 || Bank < 1)
	{
		return true;
	}

	FF00_C(0x70) = (FF00_C(0x70) & ~7) | (Bank & 7);

	MEM[0xD] = &MEM_CPU[0x1000 + 0x1000 * (FF00_C(0x70) & 7)];

	return false;
}



void CGameBoy::PrepareEmulation(BOOL Debug)
{
	DWORD	pByte, RamSize;


	Flags &= ~(GB_ERROR | GB_INVALIDOPCODE);
	BatteryAvailable = true;
	DirectionKeys = 0;
	Buttons = 0;

	if (Debug)
	{
		Flags |= GB_DEBUG;

		MemStatus[0x4] = pMemStatus_ROM(ActiveRomBank * 0x4000);
		MemStatus[0x5] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x1000);
		MemStatus[0x6] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x2000);
		MemStatus[0x7] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x3000);

		MemStatus[0x8] = &MemStatus_VRAM[(FF00_C(0x4F) & 1) * 0x2000];
		MemStatus[0x9] = &MemStatus_VRAM[(FF00_C(0x4F) & 1) * 0x2000 + 0x1000];
		if (Flags & GB_RAMENABLE && MemStatus_RAM)
		{
			MemStatus[0xA] = &MemStatus_RAM[ActiveRamBank * 0x2000];
			if (MEM_ROM[0x0149] != 1)
			{
				MemStatus[0xB] = &MemStatus_RAM[ActiveRamBank * 0x2000 + 0x1000];
			}
			else
			{
				MemStatus[0xB] = ZeroStatus;
			}
		}
		else
		{
			MemStatus[0xA] = ZeroStatus;
			MemStatus[0xB] = ZeroStatus;
		}
		MemStatus[0xC] = &MemStatus_CPU[0x0000];
		if (FF00_C(0x70) != 0)
		{
			MemStatus[0xD] = &MemStatus_CPU[(FF00_C(0x70) & 7) * 0x1000];
		}
		else
		{
			MemStatus[0xD] = &MemStatus_CPU[0x1000];
		}
	}
	else
	{
		Flags &= ~GB_DEBUG;
		if (Flags & GB_DEBUGRUNINFO)
		{
			Flags &= ~GB_DEBUGRUNINFO;

			/*//VRAM
			for (pByte = 0; pByte < sizeof(MemStatus_VRAM), pByte++)
			{
				Memory[pByte] |= MEM_READ;
			}*/

			switch (MEM_ROM[0x0149])
			{
			case 1:
				RamSize = 0x800;
				break;

			case 2:
				RamSize = 0x2000;
				break;

			case 3:
				RamSize = 0x8000;
				break;

			case 4:
				RamSize = 0x20000;
				break;

			default:
				RamSize = 0;
				break;
			}

			//External Battery RAM
			for (pByte = 0; pByte < RamSize; pByte++)
			{
				MemStatus_RAM[pByte] |= MEM_READ;
			}

			//Internal RAM
			for (pByte = 0; pByte < 0x8000; pByte++)
			{
				MemStatus_CPU[pByte] |= MEM_READ;
			}
			//HiRAM
			for (pByte = 0x8F80; pByte < 0x9000; pByte++)
			{
				MemStatus_CPU[pByte] |= MEM_READ;
			}
		}
	}

	//VRAM
	for (pByte = 0; pByte < sizeof(MemStatus_VRAM); pByte++)
	{
		MemStatus_VRAM[pByte] &= ~MEM_CHANGED;
	}

	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		break;

	case 2:
		RamSize = 0x2000;
		break;

	case 3:
		RamSize = 0x8000;
		break;

	case 4:
		RamSize = 0x20000;
		break;

	default:
		RamSize = 0;
		break;
	}

	//External Battery RAM
	for (pByte = 0; pByte < RamSize; pByte++)
	{
		MemStatus_RAM[pByte] &= ~MEM_CHANGED;
	}

	for (pByte = 0; pByte < sizeof(MemStatus_CPU); pByte++)
	{
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}

	SetStartDelay();
}



void CGameBoy::SetStartDelay()
{
	nCycles = 0;
	DelayTime = 0;
	QueryPerformanceCounter(&LastTimerCount);
}



void CGameBoy::Delay()
{
	LONGLONG		StopTimerCount;
	LARGE_INTEGER	CurrentTimerCount;


	//return;
	if (FastFwd)
	{
		SetStartDelay();
		return;
	}

	StopTimerCount = nCycles * TimerFrequency.QuadPart;
	nCycles = 0;
	//CurrentTimerCount.QuadPart = StopTimerCount % 35340;
	StopTimerCount = StopTimerCount / 35340 /*+ DelayTime*/;
	//DelayTime = CurrentTimerCount.QuadPart;

	do
	{
		QueryPerformanceCounter(&CurrentTimerCount);
	}
	while (CurrentTimerCount.QuadPart - LastTimerCount.QuadPart < StopTimerCount);

	LastTimerCount = CurrentTimerCount;
}



const int	Volume[16] = {0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 127};

void __declspec(naked) CGameBoy::MainLoop()
{
	__asm
	{
		push	ebp
		push	esi
		push	edi
		push	edx
		push	ebx

		mov		esi, dword ptr [ecx + Offset_Flags]

ContinueLoop:
		//esi	= Flags
		test	esi, GB_HALT
		jz		NotHalt

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		mov		eax, 1
		//eax	= Ticks
		inc		ebx
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free
		jmp		HaltComplete

NotHalt:
		//esi	= Flags
		mov		dword ptr [ecx + Offset_Flags], esi

		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		//edx	= PC
		call	ReadMem
		//al	= mem[PC]
		and		eax, 0xFF
		call	dword ptr [OpCodes + 4 * eax]
		//eax	= Ticks

		mov		esi, dword ptr [ecx + Offset_Flags]

		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jnz		FastCPU
		shl		al, 1
FastCPU:

		test	esi, GB_EXITLOOPDIRECTLY
		jnz		ExitLoop

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		add		ebx, eax
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free

HaltComplete:
		//eax	= Ticks
		//esi	= Flags





		//LCD
		mov		bl, byte ptr [ecx + Offset_LCD_Ticks]
		//bl	= LCD_Ticks
		cmp		bl, al
		ja		LCD_NotEnoughTicks

		mov		dx, word ptr [ecx + FF00_ASM + 0x40]
		//dl	= FF40
		//dh	= FF41
		test	dl, 0x80
		jz		LCD_Off

		test	dh, 2
		jnz		LCD_Mode2or3

		test	dh, 1
		jnz		LCD_VBlank



//LCD_HBlank:
		mov		bh, byte ptr [ecx + FF00_ASM + 0x44]
		sub		bl, al

		cmp		bh, 143
		ja		LCD_ExitHBlank

		add		bl, 40
		inc		bh
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		//bh	free
		//ebx	free

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free

		jmp		LCD_Done

LCD_ExitHBlank:
		add		bl, 228
		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		//bh	= [FF0F]
		mov		byte ptr [ecx + FF00_ASM + 0x44], 145
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		and		dh, ~3
		or		dh, 1
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free

		or		bh, 1
		or		esi, GB_EXITLOOP

		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

		test	dh, 0x40
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bl, 144
		jne		LCD_Done

		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

		jmp		LCD_Done



LCD_VBlank:
		mov		bh, [ecx + FF00_ASM + 0x44]
		sub		bl, al

		//bh	= [FF44]
		test	bh, bh
		jz		LCD_ExitVBlank

		add		bl, 228
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		cmp		bh, 154
		ja		LCD_Line0

		inc		bh
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bh, bl
		//bl	free
		//bh	free
		//ebx	free
		jne		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		jmp		LCD_Done

LCD_Line0:
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0

		test	dh, 0x40
		//dh	free
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		test	bl, bl
		jnz		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		jmp		LCD_Done

LCD_ExitVBlank:
		add		bl, 40
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4A]
		mov		byte ptr [ecx + Offset_WindowY], bl
		mov		byte ptr [ecx + Offset_WindowY2], 0

		jmp		LCD_Done

LCD_Mode2or3:
		test	dh, 1
		jz		LCD_OAM



//LCD_ToLCD:
		add		bl, 104
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		//ebx	free

		and		dh, ~3
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		//edx	free

		test	esi, GB_HDMA
		jz		LCD_NoHDMA

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		//bl	= [FF4F]
		and		ebx, 1
		shl		ebx, 13

		mov		ebp, dword ptr [ecx + FF00_ASM + 0x51]
		//ebp	= [FF51-52]
		mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
		//edi	= [FF53-54]
		rol		bp, 8
		rol		di, 8
		and		ebp, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		mov		eax, ebp
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah

		mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
		//bl	= [FF55]

		mov		eax, ebp
		shr		ebp, 12
		and		eax, 0x0FFF
		mov		ebp, [ecx + Offset_MEM + ebp * 4]

		dec		bl	//[FF55]--

		mov		edx, [ebp + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		mov		edx, [ebp + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		mov		edx, [ebp + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		mov		edx, [ebp + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx

		mov		byte ptr [ecx + FF00_ASM + 0x55], bl
		test	bl, 0x80
		//bl	free
		jz		HDMA_NotFinished
		and		esi, ~GB_HDMA
HDMA_NotFinished:

LCD_NoHDMA:
		jmp		LCD_Done



LCD_OAM:
		//eax	= Ticks
		//esi	= Flags
		//dl	= [FF40]
		//dh	= [FF41]

		sub		bl, al
		add		bl, 84
		or		dh, 3
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		mov		bx, word ptr [ecx + FF00_ASM + 0x44]
		//bl	= [FF44]
		//bh	= [FF45]

		test	dh, 0x40
		jz		LCD_NoLYC

		cmp		bl, bh
		jne		LCD_NoLYC

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

LCD_NoLYC:
		//eax	= Ticks
		//esi	= Flags
		//bl	= [FF44]

		cmp		bl, 144
		ja		LCD_Done

		mov		bh, byte ptr [ecx + Offset_FramesToSkip]
		test	bh, bh
		jnz		LCD_Done


		//Set pointer to bitmap
		movzx	edi, bl
		//bl	free
		imul	edi, (160 + 14) * 2
		add		edi, dword ptr [ecx + Offset_pGBBitmap]

		push	eax
		push	ecx
		push	esi

		push	ebp
		mov		ebp, esp
		sub		esp, 28 + 2 * (160 + 14)

#define		LineX		(ebp - 4)
#define		LCD_X		(ebp - 5)
#define		WndX		(ebp - 6)
#define		Sprites		(ebp - 26)
#define		Line		(ebp - 28 - 2 * (160 + 14))

		push	edi

		mov		bl, byte ptr [ecx + FF00_ASM + 0x43]
		mov		byte ptr [LCD_X], bl

		lea		ebx, [Line]
		mov		dword ptr [LineX], ebx

		//WORD			Sprites[10];
				/*	BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (FF00_C(0x44) < 144)
					{
						BYTE	WndX;*/


		mov		dword ptr [Line], 0
		mov		dword ptr [Line + 4], 0
		mov		dword ptr [Line + 8], 0
		mov		dword ptr [Line + 12], 0


		test	esi, GB_ROM_COLOR
		jz		Gfx_NoColor


		/////////////
		//Background
		/////////////

							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		Bg_WndXOK
Bg_NoWnd:
							mov		bl, 160
Bg_WndXOK:
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
Bg_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Bg_9000
Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Bg_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Bg_NoVFlip
							mov		al, 14
							sub		al, ah
Bg_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Bg_HFlip

							shr		bl, 7
							or		bl, 2
Bg_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shl		dx, 1
							jnc		Bg_NoLow
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoLow:
							test	dh, 1
							jz		Bg_NoHigh
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_HFlip:
							shr		bl, 7
							or		bl, 2
Bg_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shr		edx, 1
							jnc		Bg_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoLow_HFlip:
							test	dl, 0x80
							jz		Bg_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel_HFlip

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_Complete:
							pop		edx
							pop		eax
Bg_NoDraw:


							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + Offset_WindowY]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Wnd_9000
Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Wnd_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Wnd_NoVFlip
							mov		al, 14
							sub		al, ah
Wnd_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Wnd_HFlip

							shr		bl, 7
							or		bl, 2
Wnd_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shl		dx, 1
							jnc		Wnd_NoLow
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoLow:
							test	dh, 1
							jz		Wnd_NoHigh
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_HFlip:
							shr		bl, 7
							or		bl, 2
Wnd_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shr		edx, 1
							jnc		Wnd_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoLow_HFlip:
							test	dl, 0x80
							jz		Wnd_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel_HFlip

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_Complete:
							pop		edx
							pop		eax
Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		Spr_On
							pop		edi
							jmp		Spr_NoDraw

Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		Spr_FindNextSprite1 //Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		Spr_FindNextSprite1	//Sprite outside right side of screen

							mov		byte ptr [Sprites + esi], dl
							inc		esi
							cmp		esi, 10
							je		Spr_Draw

Spr_FindNextSprite1:
							inc		edx
							xor		dh, dh
							cmp		dl, 0xA0 / 4
							jne		Spr_FindNextSprite


Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		Spr_NoDraw

Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + esi - 1]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							dec		al
							mov		dh, bh
							and		dh, 8
							shr		dh, 2
							shl		edx, 4
							test	bh, 0x40
							jnz		Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		Spr_Draw_VFlipOk
Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 2) * 2

							push	edi
							add		edi, eax
							mov		dword ptr [LineX], eax
							add		dword ptr [LineX], 2

							mov		esi, ebx
							and		esi, 0x0700
							shr		esi, 5
							add		esi, ecx

							test	bh, 0x20
							jnz		Spr_HFlip

Spr_NextPixel:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel
							test	bh, 0x80
							jnz		Spr_NoDrawPixel
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel
Spr_NoDrawPixel:
							shl		dl, 1
							shl		dh, 1
							jmp		Spr_NextPixel
Spr_DrawPixel:
							push	ebx
							xor		ebx, ebx

							shl		dh, 1
							jnc		Spr_NoLow
							mov		ebx, 4
Spr_NoLow:
							shl		dl, 1
							jnc		Spr_NoHigh
							add		ebx, 2
Spr_NoHigh:

							test	ebx, ebx
							jz		Spr_NextPixel1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel1:
							pop		ebx

							jmp		Spr_NextPixel

Spr_HFlip:
Spr_NextPixel_HFlip:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel_HFlip
							test	bh, 0x80
							jnz		Spr_NoDrawPixel_HFlip
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel_HFlip
Spr_NoDrawPixel_HFlip:
							shr		dl, 1
							shr		dh, 1
							jmp		Spr_NextPixel_HFlip
Spr_DrawPixel_HFlip:
							push	ebx
							xor		ebx, ebx

							shr		dl, 1
							jnc		Spr_NoLow_HFlip
							mov		ebx, 2
Spr_NoLow_HFlip:
							shr		dh, 1
							jnc		Spr_NoHigh_HFlip
							add		ebx, 4
Spr_NoHigh_HFlip:

							test	ebx, ebx
							jz		Spr_NextPixel_HFlip1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel_HFlip1:
							pop		ebx

							jmp		Spr_NextPixel_HFlip

Spr_Complete:
							pop		edi

Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		Spr_NextSprite
							jmp		Gfx_Done


							//------------------------------------------------------------------------
							//No color
Gfx_NoColor:
							/////////////
							//Background
							/////////////


							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		NC_Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		NC_Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		NC_Bg_WndXOK
NC_Bg_NoWnd:
							mov		bl, 160
NC_Bg_WndXOK:
							//Divide number of pixels to draw by tile width (8)
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
NC_Bg_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Bg_9000
NC_Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Bg_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Bg_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Bg_NoLow
							shr		al, 4
							mov		bl, 0x80
NC_Bg_NoLow:
							test	dh, 1
							jz		NC_Bg_NoHigh
							shr		al, 2
							mov		bl, 0x80
NC_Bg_NoHigh:

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax

							add		edi, 2
							dec		bh
							jnz		NC_Bg_NextPixel


							dec		byte ptr [WndX]
							jz		NC_Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		NC_Bg_NextTile

NC_Bg_Complete:
							pop		edx
							pop		eax
NC_Bg_NoDraw:

							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		NC_Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + Offset_WindowY]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
NC_Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Wnd_9000
NC_Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Wnd_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Wnd_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Wnd_NoLow
							shr		eax, 4
							mov		bl, 0x80
NC_Wnd_NoLow:
							test	dh, 1
							jz		NC_Wnd_NoHigh
							shr		eax, 2
							mov		bl, 0x80
NC_Wnd_NoHigh:

							add		dword ptr [LineX], 2

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax


							add		edi, 2
							dec		bh
							jnz		NC_Wnd_NextPixel


							cmp		dword ptr [LineX], 167 * 2
							jae		NC_Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		NC_Wnd_NextTile

NC_Wnd_Complete:
							pop		edx
							pop		eax
NC_Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		NC_Spr_On
							pop		edi
							jmp		NC_Spr_NoDraw

NC_Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
NC_Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		NC_Spr_FindNextSprite1	//Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		NC_Spr_FindNextSprite1	//Sprite outside right side of screen

							xor		edi, edi
NC_Spr_NextSpriteList:
							cmp		edi, esi
							jae		NC_Spr_MarkToDraw
							cmp		dh, byte ptr [Sprites + 2 * edi + 1]
							jae		NC_Spr_NextSpriteList1

							push	esi
NC_Spr_MoveSpriteList:
							mov		ebx, dword ptr [Sprites + 2 * esi - 2]
							mov		word ptr [Sprites + 2 * esi], bx
							dec		esi
							cmp		esi, edi
							jne		NC_Spr_MoveSpriteList
							pop		esi
							jmp		NC_Spr_MarkToDraw

NC_Spr_NextSpriteList1:
							inc		edi
							jmp		NC_Spr_NextSpriteList

NC_Spr_MarkToDraw:
							mov		word ptr [Sprites + 2 * edi], dx
							inc		esi
							cmp		esi, 10
							je		NC_Spr_Draw

NC_Spr_FindNextSprite1:
							xor		dh, dh
							inc		edx
							cmp		dl, 0xA0 / 4
							jne		NC_Spr_FindNextSprite


NC_Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		NC_Spr_NoDraw

NC_Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + 2 * esi - 2]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		NC_Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		NC_Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							movzx	esi, bh
							mov		bl, bh
							dec		al
							shr		esi, 4
							mov		dh, bh
							and		esi, 1
							shr		bl, 7
							and		dh, 8
							shr		dh, 1
							shl		edx, 4
							test	bh, 0x40
							jnz		NC_Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		NC_Spr_Draw_VFlipOk
NC_Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
NC_Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 1) * 2

							push	edi
							add		edi, eax

							test	bh, 0x20
							jnz		NC_Spr_HFlip

NC_Spr_NextPixel:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shl		dx, 1
							jnc		NC_Spr_NoLow
							or		bl, 2
							shr		al, 4
NC_Spr_NoLow:
							test	dh, 1
							jz		NC_Spr_NoHigh
							or		bl, 2
							shr		al, 2
							and		dh, 0xFE		//Clear bit
NC_Spr_NoHigh:

							test	bl, 2
							jz		NC_Spr_NextPixel	//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel
NC_Spr_Priority:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel

NC_Spr_HFlip:
NC_Spr_NextPixel_HFlip:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shr		edx, 1
							jnc		NC_Spr_NoLow_HFlip
							or		bl, 2
							shr		al, 2
NC_Spr_NoLow_HFlip:
							test	dl, 0x80
							jz		NC_Spr_NoHigh_HFlip
							or		bl, 2
							shr		al, 4
							and		dl, 0x7F	//Clear bit
NC_Spr_NoHigh_HFlip:

							test	bl, 2
							jz		NC_Spr_NextPixel_HFlip		//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority_HFlip
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel_HFlip
NC_Spr_Priority_HFlip:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel_HFlip

NC_Spr_Complete:
							pop		edi

NC_Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		NC_Spr_NextSprite
NC_Spr_NoDraw:
Spr_NoDraw:
Gfx_Done:

		add		esp, 28 + 2 * (160 + 14)
		pop		ebp
		pop		esi
		pop		ecx
		pop		eax

		jmp		LCD_Done



LCD_Off:
		//eax	= Ticks
		//esi	= Flags
		//dl	= FF40
		//dh	= FF41

		and		dh, ~3
		mov		byte ptr [ecx + Offset_LCD_Ticks], 0
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		sub		esp, 32 //sizeof(MSG)
		mov		edx, esp

		push	eax
		push	ecx
		push	esi
		push	PM_NOREMOVE
		push	NULL
		push	NULL
		push	NULL
		push	edx
		call	dword ptr [PeekMessage]
		mov		edx, eax
		pop		esi
		pop		ecx
		pop		eax
		add		esp, 32
		test	edx, edx
		jz		LCD_Done

		cmp		dword ptr [esp - 32 + 4], WM_CLOSE
		je		CloseMsg
		cmp		dword ptr [esp - 32 + 4], WM_QUIT
		jne		LCD_Done

CloseMsg:
		or		esi, GB_EXITLOOP

		jmp		LCD_Done

		/*if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE))
		{
			Flags |= GB_EXITLOOP;
		}*/



LCD_NotEnoughTicks:
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
LCD_Done:
		//eax	= Ticks
		//esi	= Flags







		//*******
		//Sound

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebp, ecx
		//ebp	= this
		//ecx	free

		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_Off

		mov		ebx, dword ptr [ebp + Offset_Sound1TimeOut]
		//bh	= Sound1TimeOut
		test	ebx, ebx
		jz		Sound1_NoTimeOut

		cmp		ebx, eax
		ja		Sound1_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound1_Off

Sound1_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound1TimeOut], ebx
		//ebx	free

Sound1_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound1Sweep]
		test	ebx, ebx
		jz		Sound1_NoSweep

		sub		ebx, eax
		ja		Sound1_NotSweepYet

		mov		cl, byte ptr [ebp + FF00_ASM + 0x10]
		//cl	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Sweep], 0
		mov		ch, cl
		//ch	= [FF10]
		test	cl, 8
		jz		Sound1_SweepIncrease

		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		cmp		edx, ebx
		jae		Sound1_Sweep_NotZero

		xor		ebx, ebx
Sound1_Sweep_NotZero:

		sub		edx, ebx
		jmp		Sound1_Sweeped

Sound1_SweepIncrease:
		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		add		edx, ebx
		cmp		edx, 2047
		jb		Sound1_Sweep_NotAbove2047
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		mov		edx, 2047
Sound1_Sweep_NotAbove2047:

Sound1_Sweeped:
		//edx	= Sound1Frequency
		//ch	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Frequency], edx
		not		edx
		and		dh, 7
		and		ch, 0x70
		xor		ebx, ebx
		mov		byte ptr [ebp + FF00_ASM + 0x13], dl
		mov		byte ptr [ebp + FF00_ASM + 0x14], dh
		mov		bl, ch
		shl		ebx, 9
Sound1_NotSweepYet:
		mov		dword ptr [ebp + Offset_Sound1Sweep], ebx
Sound1_NoSweep:

		mov		ebx, dword ptr [ebp + Offset_Sound1Envelope]
		test	ebx, ebx
		jz		Sound1_NoEnvelope

		sub		ebx, eax
		ja		Sound1_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x12]
		//dl	= [FF12]
		test	dl, 8
		jz		Sound1_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound1_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
		shl		ebx, 14
		jmp		Sound1_Envelope_Done

Sound1_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound1_Off

Sound1_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound1_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
Sound1_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound1_Envelope_Done

Sound1_Envelope_NotYet:
Sound1_Envelope_Done:
		//ebx	= Sound1Envelope
		mov		dword ptr [ebp + Offset_Sound1Envelope], ebx
		//ebx	free
Sound1_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound1_TicksSet
		mov		edi, eax
Sound1_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		jmp		Sound1_MixEbxEcx

Sound1_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		//jmp		Sound1_MixEbxEcx
Sound1_MixEbxEcx:
		add		ebx, ecx

Sound1_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound1_NotLeft:
		test	cl, 0x10
		jz		Sound1_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound1_NotRight:

Sound1_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_Off

		mov		ebx, dword ptr [ebp + Offset_Sound2TimeOut]
		//bh	= Sound2TimeOut
		test	ebx, ebx
		jz		Sound2_NoTimeOut

		cmp		ebx, eax
		ja		Sound2_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound2_Off

Sound2_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound2TimeOut], ebx
		//ebx	free

Sound2_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound2Envelope]
		test	ebx, ebx
		jz		Sound2_NoEnvelope

		sub		ebx, eax
		ja		Sound2_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x17]
		//dl	= [FF17]
		test	dl, 8
		jz		Sound2_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound2_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
		shl		ebx, 14
		jmp		Sound2_Envelope_Done

Sound2_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound2_Off

Sound2_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound2_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
Sound2_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound2_Envelope_Done

Sound2_Envelope_NotYet:
Sound2_Envelope_Done:
		//ebx	= Sound2Envelope
		mov		dword ptr [ebp + Offset_Sound2Envelope], ebx
		//ebx	free
Sound2_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound2_TicksSet
		mov		edi, eax
Sound2_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		jmp		Sound2_MixEbxEcx

Sound2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_MixEbxEcx
Sound2_MixEbxEcx:
		add		ebx, ecx

Sound2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound2_NotLeft:
		test	cl, 0x20
		jz		Sound2_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound2_NotRight:

Sound2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_Off

		mov		ebx, dword ptr [ebp + Offset_Sound3TimeOut]
		//bh	= Sound3TimeOut
		test	ebx, ebx
		jz		Sound3_NoTimeOut

		cmp		ebx, eax
		ja		Sound3_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		bh, byte ptr [ebp + FF00_ASM + 0x1A]
		mov		byte ptr [ebp + Offset_Sound3Enabled], 0
		and		bx, ~0x8004
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		mov		byte ptr [ebp + FF00_ASM + 0x1A], bh
		jmp		Sound3_Off

Sound3_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound3TimeOut], ebx
		//ebx	free

Sound3_NoTimeOut:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound3_TicksSet
		mov		edi, eax
Sound3_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume
		test	ch, 0x20
		jnz		Sound3_VolumeSet
		mov		bl, 8
		jmp		Sound3_VolumeSet
Sound3_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet
Sound3_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ToBuffer

Sound3_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume2
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		mov		bl, 8
		jmp		Sound3_VolumeSet2
Sound3_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet2
Sound3_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ChangeStage

Sound3_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume3
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		mov		dl, 8
		jmp		Sound3_VolumeSet3
Sound3_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_VolumeSet3
Sound3_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_ToBuffer

Sound3_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound3_NotLeft:
		test	cl, 0x40
		jz		Sound3_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_NotRight:

Sound3_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_Off

		mov		ebx, dword ptr [ebp + Offset_Sound4TimeOut]
		//bh	= Sound4TimeOut
		test	ebx, ebx
		jz		Sound4_NoTimeOut

		cmp		ebx, eax
		ja		Sound4_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound4_Off

Sound4_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound4TimeOut], ebx
		//ebx	free

Sound4_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound4Envelope]
		test	ebx, ebx
		jz		Sound4_NoEnvelope

		sub		ebx, eax
		ja		Sound4_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x21]
		//dl	= [FF21]
		test	dl, 8
		jz		Sound4_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound4_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
		shl		ebx, 14
		jmp		Sound4_Envelope_Done

Sound4_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound4_Off

Sound4_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound4_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
Sound4_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound4_Envelope_Done

Sound4_Envelope_NotYet:
Sound4_Envelope_Done:
		//ebx	= Sound4Envelope
		mov		dword ptr [ebp + Offset_Sound4Envelope], ebx
		//ebx	free
Sound4_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound4_TicksSet
		mov		edi, eax
Sound4_TicksSet:

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound4Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		////edx	= (2048 - Sound4Frequency) << 3
		cmp		ecx, edx
		jae		Sound4_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_ToBuffer
		neg		ebx
		jmp		Sound4_ToBuffer

Sound4_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_ChangeStage
		neg		ebx
		jmp		Sound4_ChangeStage

Sound4_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		//dl	= Sound4Bit
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jz		Sound4_MixEbxEcx
		neg		ecx
		//jmp		Sound4_MixEbxEcx

Sound4_MixEbxEcx:
		add		ebx, ecx

Sound4_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound4_NotLeft:
		test	cl, 0x80
		jz		Sound4_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound4_NotRight:

Sound4_Off:



		mov		dl, byte ptr [ebp + Offset_SoundTicks]
		sub		dl, al
		mov		byte ptr [ebp + Offset_SoundTicks], dl
		jae		Sound_NoUpdate

		mov		ecx, ebp

		add		edx, eax
		and		edx, 0xFF
		mov		edi, eax
		sub		edi, edx

		push	eax
		push	esi
		push	edi
		call	SoundUpdate
		pop		edi
		pop		esi
		pop		eax



		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		jmp		Sound1_2_MixEbxEcx

Sound1_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		//jmp		Sound1_2_MixEbxEcx
Sound1_2_MixEbxEcx:
		add		ebx, ecx

Sound1_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_2_NotLeft
		mov		dword ptr [ebp + Offset_SoundL], ebx
Sound1_2_NotLeft:
		test	cl, 0x10
		jz		Sound1_2_NotRight
		mov		dword ptr [ebp + Offset_SoundR], ebx
Sound1_2_NotRight:
		pop		edi
Sound1_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		jmp		Sound2_2_MixEbxEcx

Sound2_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_2_MixEbxEcx
Sound2_2_MixEbxEcx:
		add		ebx, ecx

Sound2_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_2_NotLeft
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
Sound2_2_NotLeft:
		test	cl, 0x20
		jz		Sound2_2_NotRight
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound2_2_NotRight:

		pop		edi

Sound2_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume
		test	ch, 0x20
		jnz		Sound3_2_VolumeSet
		mov		bl, 8
		jmp		Sound3_2_VolumeSet
Sound3_2_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet
Sound3_2_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ToBuffer

Sound3_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_2_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume2
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		mov		bl, 8
		jmp		Sound3_2_VolumeSet2
Sound3_2_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet2
Sound3_2_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ChangeStage

Sound3_2_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_2_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume3
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		mov		dl, 8
		jmp		Sound3_2_VolumeSet3
Sound3_2_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_2_VolumeSet3
Sound3_2_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_2_ToBuffer

Sound3_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_2_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound3_2_NotLeft:
		test	cl, 0x40
		jz		Sound3_2_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_2_NotRight:

		pop		edi

Sound3_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_2_Off

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		//edx	= Sound4Frequency
		cmp		ecx, edx
		jae		Sound4_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_2_ToBuffer
		neg		ebx
		jmp		Sound4_2_ToBuffer

Sound4_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_2_ChangeStage
		neg		ebx
		jmp		Sound4_2_ChangeStage

Sound4_2_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_2_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jnz		Sound4_2_MixEbxEcx
		neg		ecx
		//jmp		Sound4_2_MixEbxEcx

Sound4_2_MixEbxEcx:
		add		ebx, ecx

Sound4_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_2_NotLeft
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
Sound4_2_NotLeft:
		test	cl, 0x80
		jz		Sound4_2_NotRight
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound4_2_NotRight:

Sound4_2_Off:



Sound_NoUpdate:
		mov		ecx, ebp



		/*/******
		//Serial

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bh, byte ptr [ecx + FF00_ASM + 0x02]
		test	bh, 0x80
		//bh	= [FF02]
		jz		SIO_NotEnabled
		test	bh, 1
		jnz		SIO_InternalClock

		mov		bl, byte ptr [ecx + Offset_SerialInput]
		//bl	= SerialInput
		test	bl, 2
		jz		SIO_Wait
		mov		byte ptr [ecx + Offset_SerialInput], 0
		mov		edi, dword ptr [ecx + Offset_SerialOutput]
		//edi	= SerialOutput
		mov		dl, byte ptr [ecx + FF00_ASM + 0x01]
		//dl	= [FF01]
		shl		edx, 1
		and		bl, 1
		and		dh, 1
		or		dl, bl
		//bl	free
		mov		byte ptr [edx], dh
		//dh	free
		mov		byte ptr [ecx + FF00_ASM + 0x01], dl
		//dl	free
		//edx	free

		mov		bl, byte ptr [ecx + Offset_SerialByte]
		//bl	= SerialByte
		cmp		bl, 7
		jae		SIO_Interrupt
		inc		bl
		mov		byte ptr [ecx + Offset_SerialByte], bl
		//bl	free
		jmp		SIO_Complete

SIO_Interrupt:
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		//bl	= [FF0F]
		mov		bh, byte ptr [ecx + FF00_ASM + 0x02]
		//bh	= [FF02]
		or		bl, 8
		and		bh, ~8
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		mov		byte ptr [ecx + FF00_ASM + 0x02], bh
		jmp		SIO_Complete

SIO_InternalClock:
		mov		bl, byte ptr [ecx + Offset_SerialTicks]
		//bl	= SerialTicks
		sub		bl, al
		ja		SIO_NotEnoughTicks

		mov		byte ptr [ecx + Offset_SerialTicks], 128

		mov		edi, dword ptr [ecx + Offset_SerialOutput]
		//edi	= SerialOutput
		test	edi, edi
		jz		SIO_NotConnected
		mov		dl, byte ptr [ecx + FF00_ASM + 0x01]
		//dl	= [FF01]
		shl		edx, 1
		and		bl, 1
		and		dh, 1
		or		dl, bl
		//bl	free
		mov		byte ptr [edx], dh
		//dh	free
		mov		byte ptr [ecx + FF00_ASM + 0x01], dl
		//dl	free
		//edx	free

		mov		bl, byte ptr [ecx + Offset_SerialByte]
		//bl	= SerialByte
		cmp		bl, 7
		jae		SIO_Interrupt
		inc		bl
		mov		byte ptr [ecx + Offset_SerialByte], bl
		//bl	free
		jmp		SIO_Complete

SIO_NotEnoughTicks:
		mov		byte ptr [ecx + Offset_SerialTicks], bl

SIO_Wait:
SIO_Complete:
SIO_NotEnabled://*/



		//*******
		//Divider

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + Offset_DIV_Ticks]
		//bl	= DIV_Ticks
		sub		bl, al	//DIV_Ticks -= Ticks
		jnc		DIV_NotEnoughTicks
		mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
		//dl	= FF00(0x04)
		add		bl, 32	//DIV_Ticks += 32
		inc		dl
		mov		byte ptr [ecx + FF00_ASM + 0x04], dl
		//dl	free
		//edx	free
DIV_NotEnoughTicks:
		mov		byte ptr [ecx + Offset_DIV_Ticks], bl
		//bl	free
		//ebx	free

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags


		//*****
		//Timer

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + FF00_ASM + 0x07]
		test	bl, 0x04
		jz		TimerDisabled
		mov		ebx, dword ptr [ecx + Offset_TIMA_Ticks]
		//ebx	= TIMA_Ticks
		add		ebx, eax
		//eax	free
		mov		edx, dword ptr [ecx + Offset_Hz]
		//edx	= Hz
		//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
		cmp		ebx, edx
		jb		TIMA_NotEnoughTicks

		sub		ebx, edx
		//edx	free
		mov		dl, byte ptr [ecx + FF00_ASM + 0x05]
		//dl	= FF00(0x05)
		inc		dl
		jnz		TIMA_NotZero
		mov		dl, byte ptr [ecx + FF00_ASM + 0x06]
		//dl	= FF00(0x06) (new value of FF00(0x05))
		mov		dh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dh, 0x04
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dh
		//or		byte ptr [ecx + FF00_ASM + 0x0F], 0x04
TIMA_NotZero:
		mov		byte ptr [ecx + FF00_ASM + 0x05], dl
		//dl	free
		//edx	free

TIMA_NotEnoughTicks:
		mov		dword ptr [ecx + Offset_TIMA_Ticks], ebx
		//ebx	free

TimerDisabled:
		//ecx	= this
		//esi	= Flags


		//**********
		//Interrupts

		//ecx	= this
		//esi	= Flags

		test	esi, GB_IE
		jz		NoInterrupt
		mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
		//ah	= FF00(0x0F)
		mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
		and		al, ah
		//al	= FF00(0xFF) & FF00(0x0F)
		test	al, 0x0F
		jz		InterruptServiced

		and		esi, ~(GB_IE | GB_ENABLEIE | GB_HALT)
		mov		dword ptr [ecx + Offset_Flags], esi


		push	ecx
		xor		ecx, ecx
		shr		al, 1
		jc		InterruptFound
TestNextInterrupt:
		inc		cl
		shr		al, 1
		jnc		TestNextInterrupt

InterruptFound:
		//al	free
		mov		bl, 1
		shl		bl, cl
		not		bl
		and		ah, bl
		//ah	= new value of FF0F
		lea		ebx, [ecx * 8 + 0x40]
		//ebx	= interrupt address
		pop		ecx
		//ecx	= this

		mov		edx, dword ptr [ecx + Offset_Reg_SP]
		//edx	= Reg_SP
		sub		dx, 2

		mov		byte ptr [ecx + FF00_ASM + 0x0F], ah
		//ah	free
		//eax	free

		mov		eax, dword ptr [ecx + Offset_Reg_PC]
		//eax	= Reg_PC

		mov		word ptr [ecx + Offset_Reg_PC], bx
		//ebx	free

		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		//eax	free
		//edx	free

		jmp		InterruptServiced

NoInterrupt:
		//ecx	= this
		//esi	= Flags

		test	esi, GB_ENABLEIE
		jz		InterruptServiced
		or		esi, GB_IE

InterruptServiced:
		//ecx	= this
		//esi	= Flags


		//*********
		//Exit loop

		//ecx	= this
		//esi	= Flags

		test	esi, GB_EXITLOOP
		jz		ContinueLoop

ExitLoop:
		and		esi, ~(GB_EXITLOOP | GB_EXITLOOPDIRECTLY)
		mov		dword ptr [ecx + Offset_Flags], esi
		//esi	free

		pop		ebx
		pop		edx
		pop		edi
		pop		esi
		pop		ebp
		ret
	}
}

#undef		Sprites
#undef		LineX
#undef		Line
#undef		LCD_X
#undef		WndX



char	ReadAccessDeniedMsg[] = "Read access needed.";
char	ExecuteAccessDeniedMsg[] = "Execute access needed.";
char	InterruptWriteAccessDeniedMsg[] = "Write access needed for stack pointer, interrupt cannot be serviced.";



void AccessDenied(CGameBoy *pGameBoy, char *pMsg)
{
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, pMsg, "Game Lad", MB_ICONWARNING | MB_OK);
	pGameBoy->Stop();
	pGameBoy->Flags |= GB_ERROR;
}



void __declspec(naked) CGameBoy::DebugMainLoop()
{
	__asm
	{
		push	ebp
		push	esi
		push	edi
		push	edx
		push	ebx

		mov		esi, dword ptr [ecx + Offset_Flags]

ContinueLoop:
		//esi	= Flags
		test	esi, GB_HALT
		jz		NotHalt

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		mov		eax, 1
		//eax	= Ticks
		inc		ebx
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free
		jmp		HaltComplete

NotHalt:
		//esi	= Flags
		mov		dword ptr [ecx + Offset_Flags], esi

		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		//edx	= Reg_PC

		call	RetrieveAccess
		test	al, MEM_EXECUTE
		jz		ExecuteAccessDenied

		test	al, MEM_READ
		jz		ReadAccessDenied

		call	ReadMem
		and		eax, 0xFF
		call	dword ptr [DebugOpCodes + 4 * eax]

		mov		esi, dword ptr [ecx + Offset_Flags]

		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jnz		FastCPU
		shl		al, 1
FastCPU:

		test	esi, GB_EXITLOOPDIRECTLY
		jnz		ExitLoop

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		add		ebx, eax
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free

		push	eax
		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		call	RetrieveAccess
		test	al, MEM_BREAKPOINT
		pop		eax
		jz		NotBreakPoint
		or		esi, GB_EXITLOOP | GB_ERROR
NotBreakPoint:

HaltComplete:
		//eax	= Ticks
		//esi	= Flags





		//LCD
		mov		bl, byte ptr [ecx + Offset_LCD_Ticks]
		//bl	= LCD_Ticks
		cmp		bl, al
		ja		LCD_NotEnoughTicks

		mov		dx, word ptr [ecx + FF00_ASM + 0x40]
		//dl	= FF40
		//dh	= FF41
		test	dl, 0x80
		jz		LCD_Off

		test	dh, 2
		jnz		LCD_Mode2or3

		test	dh, 1
		jnz		LCD_VBlank



//LCD_HBlank:
		mov		bh, byte ptr [ecx + FF00_ASM + 0x44]
		sub		bl, al

		cmp		bh, 143
		ja		LCD_ExitHBlank

		add		bl, 40
		inc		bh
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		//bh	free
		//ebx	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		jmp		LCD_Done

LCD_ExitHBlank:
		add		bl, 228
		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		//bh	= [FF0F]
		mov		byte ptr [ecx + FF00_ASM + 0x44], 145
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		and		dh, ~3
		or		dh, 1
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		or		bh, 1
		or		esi, GB_EXITLOOP

		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		test	dh, 0x40
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bl, 144
		jne		LCD_Done

		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		//or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		jmp		LCD_Done



LCD_VBlank:
		mov		bh, [ecx + FF00_ASM + 0x44]
		sub		bl, al

		//bh	= [FF44]
		test	bh, bh
		jz		LCD_ExitVBlank

		add		bl, 228
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		cmp		bh, 154
		ja		LCD_Line0

		inc		bh
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bh, bl
		//bl	free
		//bh	free
		//ebx	free
		jne		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
		jmp		LCD_Done

LCD_Line0:
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		test	dh, 0x40
		//dh	free
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		test	bl, bl
		jnz		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
		jmp		LCD_Done

LCD_ExitVBlank:
		add		bl, 40
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4A]
		mov		byte ptr [ecx + Offset_WindowY], bl
		mov		byte ptr [ecx + Offset_WindowY2], 0

		jmp		LCD_Done

LCD_Mode2or3:
		test	dh, 1
		jz		LCD_OAM



//LCD_ToLCD:
		add		bl, 104
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		//ebx	free

		and		dh, ~3
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		//edx	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		test	esi, GB_HDMA
		jz		LCD_NoHDMA

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		//bl	= [FF4F]
		and		ebx, 1
		shl		ebx, 13

		mov		ebp, dword ptr [ecx + FF00_ASM + 0x51]
		//ebp	= [FF51-52]
		mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
		//edi	= [FF53-54]
		rol		bp, 8
		rol		di, 8
		and		ebp, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		mov		eax, ebp
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F52], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F51], MEM_CHANGED
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F54], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F53], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
		//bl	= [FF55]

		mov		eax, ebp
		shr		ebp, 12
		and		eax, 0x0FFF
		mov		ebp, [ecx + Offset_MEM + ebp * 4]

		dec		bl	//[FF55]--

		mov		edx, [ebp + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x04], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x08], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x0C], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED

		mov		byte ptr [ecx + FF00_ASM + 0x55], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F55], MEM_CHANGED
		test	bl, 0x80
		//bl	free
		jz		HDMA_NotFinished
		and		esi, ~GB_HDMA
HDMA_NotFinished:

LCD_NoHDMA:
		jmp		LCD_Done



LCD_OAM:
		//eax	= Ticks
		//esi	= Flags
		//dl	= [FF40]
		//dh	= [FF41]

		sub		bl, al
		add		bl, 84
		or		dh, 3
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		mov		bx, word ptr [ecx + FF00_ASM + 0x44]
		//bl	= [FF44]
		//bh	= [FF45]

		test	dh, 0x40
		jz		LCD_NoLYC

		cmp		bl, bh
		jne		LCD_NoLYC

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

LCD_NoLYC:
		//eax	= Ticks
		//esi	= Flags
		//bl	= [FF44]

		cmp		bl, 144
		ja		LCD_Done

		mov		bh, byte ptr [ecx + Offset_FramesToSkip]
		test	bh, bh
		jnz		LCD_Done


		//Set pointer to bitmap
		movzx	edi, bl
		//bl	free
		imul	edi, (160 + 14) * 2
		add		edi, dword ptr [ecx + Offset_pGBBitmap]

		push	eax
		push	ecx
		push	esi

		push	ebp
		mov		ebp, esp
		sub		esp, 28 + 2 * (160 + 14)

#define		LineX		(ebp - 4)
#define		LCD_X		(ebp - 5)
#define		WndX		(ebp - 6)
#define		Sprites		(ebp - 26)
#define		Line		(ebp - 28 - 2 * (160 + 14))

		push	edi

		mov		bl, byte ptr [ecx + FF00_ASM + 0x43]
		mov		byte ptr [LCD_X], bl

		lea		ebx, [Line]
		mov		dword ptr [LineX], ebx

		//WORD			Sprites[10];
				/*	BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (FF00_C(0x44) < 144)
					{
						BYTE	WndX;*/


		mov		dword ptr [Line], 0
		mov		dword ptr [Line + 4], 0
		mov		dword ptr [Line + 8], 0
		mov		dword ptr [Line + 12], 0


		test	esi, GB_ROM_COLOR
		jz		Gfx_NoColor


		/////////////
		//Background
		/////////////

							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		Bg_WndXOK
Bg_NoWnd:
							mov		bl, 160
Bg_WndXOK:
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
Bg_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Bg_9000
Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Bg_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Bg_NoVFlip
							mov		al, 14
							sub		al, ah
Bg_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Bg_HFlip

							shr		bl, 7
							or		bl, 2
Bg_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shl		dx, 1
							jnc		Bg_NoLow
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoLow:
							test	dh, 1
							jz		Bg_NoHigh
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_HFlip:
							shr		bl, 7
							or		bl, 2
Bg_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shr		edx, 1
							jnc		Bg_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoLow_HFlip:
							test	dl, 0x80
							jz		Bg_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel_HFlip

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_Complete:
							pop		edx
							pop		eax
Bg_NoDraw:


							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + Offset_WindowY]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Wnd_9000
Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Wnd_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Wnd_NoVFlip
							mov		al, 14
							sub		al, ah
Wnd_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Wnd_HFlip

							shr		bl, 7
							or		bl, 2
Wnd_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shl		dx, 1
							jnc		Wnd_NoLow
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoLow:
							test	dh, 1
							jz		Wnd_NoHigh
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_HFlip:
							shr		bl, 7
							or		bl, 2
Wnd_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shr		edx, 1
							jnc		Wnd_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoLow_HFlip:
							test	dl, 0x80
							jz		Wnd_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel_HFlip

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_Complete:
							pop		edx
							pop		eax
Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		Spr_On
							pop		edi
							jmp		Spr_NoDraw

Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		Spr_FindNextSprite1 //Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		Spr_FindNextSprite1	//Sprite outside right side of screen

							mov		byte ptr [Sprites + esi], dl
							inc		esi
							cmp		esi, 10
							je		Spr_Draw

Spr_FindNextSprite1:
							inc		edx
							xor		dh, dh
							cmp		dl, 0xA0 / 4
							jne		Spr_FindNextSprite


Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		Spr_NoDraw

Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + esi - 1]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							dec		al
							mov		dh, bh
							and		dh, 8
							shr		dh, 2
							shl		edx, 4
							test	bh, 0x40
							jnz		Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		Spr_Draw_VFlipOk
Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 2) * 2

							push	edi
							add		edi, eax
							mov		dword ptr [LineX], eax
							add		dword ptr [LineX], 2

							mov		esi, ebx
							and		esi, 0x0700
							shr		esi, 5
							add		esi, ecx

							test	bh, 0x20
							jnz		Spr_HFlip

Spr_NextPixel:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel
							test	bh, 0x80
							jnz		Spr_NoDrawPixel
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel
Spr_NoDrawPixel:
							shl		dl, 1
							shl		dh, 1
							jmp		Spr_NextPixel
Spr_DrawPixel:
							push	ebx
							xor		ebx, ebx

							shl		dh, 1
							jnc		Spr_NoLow
							mov		ebx, 4
Spr_NoLow:
							shl		dl, 1
							jnc		Spr_NoHigh
							add		ebx, 2
Spr_NoHigh:

							test	ebx, ebx
							jz		Spr_NextPixel1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel1:
							pop		ebx

							jmp		Spr_NextPixel

Spr_HFlip:
Spr_NextPixel_HFlip:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel_HFlip
							test	bh, 0x80
							jnz		Spr_NoDrawPixel_HFlip
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel_HFlip
Spr_NoDrawPixel_HFlip:
							shr		dl, 1
							shr		dh, 1
							jmp		Spr_NextPixel_HFlip
Spr_DrawPixel_HFlip:
							push	ebx
							xor		ebx, ebx

							shr		dl, 1
							jnc		Spr_NoLow_HFlip
							mov		ebx, 2
Spr_NoLow_HFlip:
							shr		dh, 1
							jnc		Spr_NoHigh_HFlip
							add		ebx, 4
Spr_NoHigh_HFlip:

							test	ebx, ebx
							jz		Spr_NextPixel_HFlip1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel_HFlip1:
							pop		ebx

							jmp		Spr_NextPixel_HFlip

Spr_Complete:
							pop		edi

Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		Spr_NextSprite
							jmp		Gfx_Done


							//------------------------------------------------------------------------
							//No color
Gfx_NoColor:
							/////////////
							//Background
							/////////////


							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		NC_Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		NC_Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		NC_Bg_WndXOK
NC_Bg_NoWnd:
							mov		bl, 160
NC_Bg_WndXOK:
							//Divide number of pixels to draw by tile width (8)
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
NC_Bg_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Bg_9000
NC_Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Bg_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Bg_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Bg_NoLow
							shr		al, 4
							mov		bl, 0x80
NC_Bg_NoLow:
							test	dh, 1
							jz		NC_Bg_NoHigh
							shr		al, 2
							mov		bl, 0x80
NC_Bg_NoHigh:

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax

							add		edi, 2
							dec		bh
							jnz		NC_Bg_NextPixel


							dec		byte ptr [WndX]
							jz		NC_Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		NC_Bg_NextTile

NC_Bg_Complete:
							pop		edx
							pop		eax
NC_Bg_NoDraw:

							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		NC_Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + Offset_WindowY]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
NC_Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Wnd_9000
NC_Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Wnd_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Wnd_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Wnd_NoLow
							shr		eax, 4
							mov		bl, 0x80
NC_Wnd_NoLow:
							test	dh, 1
							jz		NC_Wnd_NoHigh
							shr		eax, 2
							mov		bl, 0x80
NC_Wnd_NoHigh:

							add		dword ptr [LineX], 2

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax


							add		edi, 2
							dec		bh
							jnz		NC_Wnd_NextPixel


							cmp		dword ptr [LineX], 167 * 2
							jae		NC_Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		NC_Wnd_NextTile

NC_Wnd_Complete:
							pop		edx
							pop		eax
NC_Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		NC_Spr_On
							pop		edi
							jmp		NC_Spr_NoDraw

NC_Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
NC_Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		NC_Spr_FindNextSprite1	//Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		NC_Spr_FindNextSprite1	//Sprite outside right side of screen

							xor		edi, edi
NC_Spr_NextSpriteList:
							cmp		edi, esi
							jae		NC_Spr_MarkToDraw
							cmp		dh, byte ptr [Sprites + 2 * edi + 1]
							jae		NC_Spr_NextSpriteList1

							push	esi
NC_Spr_MoveSpriteList:
							mov		ebx, dword ptr [Sprites + 2 * esi - 2]
							mov		word ptr [Sprites + 2 * esi], bx
							dec		esi
							cmp		esi, edi
							jne		NC_Spr_MoveSpriteList
							pop		esi
							jmp		NC_Spr_MarkToDraw

NC_Spr_NextSpriteList1:
							inc		edi
							jmp		NC_Spr_NextSpriteList

NC_Spr_MarkToDraw:
							mov		word ptr [Sprites + 2 * edi], dx
							inc		esi
							cmp		esi, 10
							je		NC_Spr_Draw

NC_Spr_FindNextSprite1:
							xor		dh, dh
							inc		edx
							cmp		dl, 0xA0 / 4
							jne		NC_Spr_FindNextSprite


NC_Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		NC_Spr_NoDraw

NC_Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + 2 * esi - 2]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		NC_Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		NC_Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							movzx	esi, bh
							mov		bl, bh
							dec		al
							shr		esi, 4
							mov		dh, bh
							and		esi, 1
							shr		bl, 7
							and		dh, 8
							shr		dh, 1
							shl		edx, 4
							test	bh, 0x40
							jnz		NC_Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		NC_Spr_Draw_VFlipOk
NC_Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
NC_Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 1) * 2

							push	edi
							add		edi, eax

							test	bh, 0x20
							jnz		NC_Spr_HFlip

NC_Spr_NextPixel:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shl		dx, 1
							jnc		NC_Spr_NoLow
							or		bl, 2
							shr		al, 4
NC_Spr_NoLow:
							test	dh, 1
							jz		NC_Spr_NoHigh
							or		bl, 2
							shr		al, 2
							and		dh, 0xFE		//Clear bit
NC_Spr_NoHigh:

							test	bl, 2
							jz		NC_Spr_NextPixel	//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel
NC_Spr_Priority:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel

NC_Spr_HFlip:
NC_Spr_NextPixel_HFlip:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shr		edx, 1
							jnc		NC_Spr_NoLow_HFlip
							or		bl, 2
							shr		al, 2
NC_Spr_NoLow_HFlip:
							test	dl, 0x80
							jz		NC_Spr_NoHigh_HFlip
							or		bl, 2
							shr		al, 4
							and		dl, 0x7F	//Clear bit
NC_Spr_NoHigh_HFlip:

							test	bl, 2
							jz		NC_Spr_NextPixel_HFlip		//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority_HFlip
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel_HFlip
NC_Spr_Priority_HFlip:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel_HFlip

NC_Spr_Complete:
							pop		edi

NC_Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		NC_Spr_NextSprite
NC_Spr_NoDraw:
Spr_NoDraw:
Gfx_Done:

		add		esp, 28 + 2 * (160 + 14)
		pop		ebp
		pop		esi
		pop		ecx
		pop		eax

		jmp		LCD_Done



LCD_Off:
		//eax	= Ticks
		//esi	= Flags
		//dl	= FF40
		//dh	= FF41

		and		dh, ~3
		mov		byte ptr [ecx + Offset_LCD_Ticks], 0
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		sub		esp, 32 //sizeof(MSG)
		mov		edx, esp

		push	eax
		push	ecx
		push	esi
		push	PM_NOREMOVE
		push	NULL
		push	NULL
		push	NULL
		push	edx
		call	dword ptr [PeekMessage]
		mov		edx, eax
		pop		esi
		pop		ecx
		pop		eax
		add		esp, 32
		test	edx, edx
		jz		LCD_Done

		cmp		dword ptr [esp - 32 + 4], WM_CLOSE
		je		CloseMsg
		cmp		dword ptr [esp - 32 + 4], WM_QUIT
		jne		LCD_Done

CloseMsg:
		or		esi, GB_EXITLOOP

		jmp		LCD_Done

		/*if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE))
		{
			Flags |= GB_EXITLOOP;
		}*/



LCD_NotEnoughTicks:
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
LCD_Done:
		//eax	= Ticks
		//esi	= Flags







		//*******
		//Sound

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebp, ecx
		//ebp	= this
		//ecx	free

		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_Off

		mov		ebx, dword ptr [ebp + Offset_Sound1TimeOut]
		//bh	= Sound1TimeOut
		test	ebx, ebx
		jz		Sound1_NoTimeOut

		cmp		ebx, eax
		ja		Sound1_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound1_Off

Sound1_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound1TimeOut], ebx
		//ebx	free

Sound1_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound1Sweep]
		test	ebx, ebx
		jz		Sound1_NoSweep

		sub		ebx, eax
		ja		Sound1_NotSweepYet

		mov		cl, byte ptr [ebp + FF00_ASM + 0x10]
		//cl	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Sweep], 0
		mov		ch, cl
		//ch	= [FF10]
		test	cl, 8
		jz		Sound1_SweepIncrease

		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		cmp		edx, ebx
		jae		Sound1_Sweep_NotZero

		xor		ebx, ebx
Sound1_Sweep_NotZero:

		sub		edx, ebx
		jmp		Sound1_Sweeped

Sound1_SweepIncrease:
		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		add		edx, ebx
		cmp		edx, 2047
		jb		Sound1_Sweep_NotAbove2047
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		mov		edx, 2047
Sound1_Sweep_NotAbove2047:

Sound1_Sweeped:
		//edx	= Sound1Frequency
		//ch	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Frequency], edx
		not		edx
		and		dh, 7
		and		ch, 0x70
		xor		ebx, ebx
		mov		byte ptr [ebp + FF00_ASM + 0x13], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F13], MEM_CHANGED
		mov		byte ptr [ebp + FF00_ASM + 0x14], dh
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F14], MEM_CHANGED
		mov		bl, ch
		shl		ebx, 9
Sound1_NotSweepYet:
		mov		dword ptr [ebp + Offset_Sound1Sweep], ebx
Sound1_NoSweep:

		mov		ebx, dword ptr [ebp + Offset_Sound1Envelope]
		test	ebx, ebx
		jz		Sound1_NoEnvelope

		sub		ebx, eax
		ja		Sound1_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x12]
		//dl	= [FF12]
		test	dl, 8
		jz		Sound1_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound1_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F12], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
		shl		ebx, 14
		jmp		Sound1_Envelope_Done

Sound1_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound1_Off

Sound1_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound1_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F12], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
Sound1_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound1_Envelope_Done

Sound1_Envelope_NotYet:
Sound1_Envelope_Done:
		//ebx	= Sound1Envelope
		mov		dword ptr [ebp + Offset_Sound1Envelope], ebx
		//ebx	free
Sound1_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound1_TicksSet
		mov		edi, eax
Sound1_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		jmp		Sound1_MixEbxEcx

Sound1_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		//jmp		Sound1_MixEbxEcx
Sound1_MixEbxEcx:
		add		ebx, ecx

Sound1_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound1_NotLeft:
		test	cl, 0x10
		jz		Sound1_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound1_NotRight:

Sound1_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_Off

		mov		ebx, dword ptr [ebp + Offset_Sound2TimeOut]
		//bh	= Sound2TimeOut
		test	ebx, ebx
		jz		Sound2_NoTimeOut

		cmp		ebx, eax
		ja		Sound2_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound2_Off

Sound2_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound2TimeOut], ebx
		//ebx	free

Sound2_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound2Envelope]
		test	ebx, ebx
		jz		Sound2_NoEnvelope

		sub		ebx, eax
		ja		Sound2_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x17]
		//dl	= [FF17]
		test	dl, 8
		jz		Sound2_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound2_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F17], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
		shl		ebx, 14
		jmp		Sound2_Envelope_Done

Sound2_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound2_Off

Sound2_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound2_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F17], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
Sound2_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound2_Envelope_Done

Sound2_Envelope_NotYet:
Sound2_Envelope_Done:
		//ebx	= Sound2Envelope
		mov		dword ptr [ebp + Offset_Sound2Envelope], ebx
		//ebx	free
Sound2_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound2_TicksSet
		mov		edi, eax
Sound2_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		jmp		Sound2_MixEbxEcx

Sound2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_MixEbxEcx
Sound2_MixEbxEcx:
		add		ebx, ecx

Sound2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound2_NotLeft:
		test	cl, 0x20
		jz		Sound2_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound2_NotRight:

Sound2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_Off

		mov		ebx, dword ptr [ebp + Offset_Sound3TimeOut]
		//bh	= Sound3TimeOut
		test	ebx, ebx
		jz		Sound3_NoTimeOut

		cmp		ebx, eax
		ja		Sound3_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		bh, byte ptr [ebp + FF00_ASM + 0x1A]
		mov		byte ptr [ebp + Offset_Sound3Enabled], 0
		and		bx, ~0x8004
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		mov		byte ptr [ebp + FF00_ASM + 0x1A], bh
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F1A], MEM_CHANGED
		jmp		Sound3_Off

Sound3_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound3TimeOut], ebx
		//ebx	free

Sound3_NoTimeOut:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound3_TicksSet
		mov		edi, eax
Sound3_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume
		test	ch, 0x20
		jnz		Sound3_VolumeSet
		mov		bl, 8
		jmp		Sound3_VolumeSet
Sound3_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet
Sound3_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ToBuffer

Sound3_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume2
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		mov		bl, 8
		jmp		Sound3_VolumeSet2
Sound3_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet2
Sound3_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ChangeStage

Sound3_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume3
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		mov		dl, 8
		jmp		Sound3_VolumeSet3
Sound3_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_VolumeSet3
Sound3_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_ToBuffer

Sound3_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound3_NotLeft:
		test	cl, 0x40
		jz		Sound3_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_NotRight:

Sound3_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_Off

		mov		ebx, dword ptr [ebp + Offset_Sound4TimeOut]
		//bh	= Sound4TimeOut
		test	ebx, ebx
		jz		Sound4_NoTimeOut

		cmp		ebx, eax
		ja		Sound4_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound4_Off

Sound4_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound4TimeOut], ebx
		//ebx	free

Sound4_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound4Envelope]
		test	ebx, ebx
		jz		Sound4_NoEnvelope

		sub		ebx, eax
		ja		Sound4_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x21]
		//dl	= [FF21]
		test	dl, 8
		jz		Sound4_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound4_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F21], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
		shl		ebx, 14
		jmp		Sound4_Envelope_Done

Sound4_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound4_Off

Sound4_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound4_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F21], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
Sound4_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound4_Envelope_Done

Sound4_Envelope_NotYet:
Sound4_Envelope_Done:
		//ebx	= Sound4Envelope
		mov		dword ptr [ebp + Offset_Sound4Envelope], ebx
		//ebx	free
Sound4_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound4_TicksSet
		mov		edi, eax
Sound4_TicksSet:

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound4Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		////edx	= (2048 - Sound4Frequency) << 3
		cmp		ecx, edx
		jae		Sound4_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_ToBuffer
		neg		ebx
		jmp		Sound4_ToBuffer

Sound4_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_ChangeStage
		neg		ebx
		jmp		Sound4_ChangeStage

Sound4_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		//dl	= Sound4Bit
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jz		Sound4_MixEbxEcx
		neg		ecx
		//jmp		Sound4_MixEbxEcx

Sound4_MixEbxEcx:
		add		ebx, ecx

Sound4_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound4_NotLeft:
		test	cl, 0x80
		jz		Sound4_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound4_NotRight:

Sound4_Off:



		mov		dl, byte ptr [ebp + Offset_SoundTicks]
		sub		dl, al
		mov		byte ptr [ebp + Offset_SoundTicks], dl
		jae		Sound_NoUpdate

		mov		ecx, ebp

		add		edx, eax
		and		edx, 0xFF
		mov		edi, eax
		sub		edi, edx

		push	eax
		push	esi
		push	edi
		call	SoundUpdate
		pop		edi
		pop		esi
		pop		eax



		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		jmp		Sound1_2_MixEbxEcx

Sound1_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		//jmp		Sound1_2_MixEbxEcx
Sound1_2_MixEbxEcx:
		add		ebx, ecx

Sound1_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_2_NotLeft
		mov		dword ptr [ebp + Offset_SoundL], ebx
Sound1_2_NotLeft:
		test	cl, 0x10
		jz		Sound1_2_NotRight
		mov		dword ptr [ebp + Offset_SoundR], ebx
Sound1_2_NotRight:
		pop		edi
Sound1_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		jmp		Sound2_2_MixEbxEcx

Sound2_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_2_MixEbxEcx
Sound2_2_MixEbxEcx:
		add		ebx, ecx

Sound2_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_2_NotLeft
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
Sound2_2_NotLeft:
		test	cl, 0x20
		jz		Sound2_2_NotRight
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound2_2_NotRight:

		pop		edi

Sound2_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume
		test	ch, 0x20
		jnz		Sound3_2_VolumeSet
		mov		bl, 8
		jmp		Sound3_2_VolumeSet
Sound3_2_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet
Sound3_2_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ToBuffer

Sound3_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_2_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume2
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		mov		bl, 8
		jmp		Sound3_2_VolumeSet2
Sound3_2_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet2
Sound3_2_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ChangeStage

Sound3_2_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_2_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume3
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		mov		dl, 8
		jmp		Sound3_2_VolumeSet3
Sound3_2_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_2_VolumeSet3
Sound3_2_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_2_ToBuffer

Sound3_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_2_NotLeft
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
Sound3_2_NotLeft:
		test	cl, 0x40
		jz		Sound3_2_NotRight
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_2_NotRight:

		pop		edi

Sound3_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_2_Off

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		//edx	= Sound4Frequency
		cmp		ecx, edx
		jae		Sound4_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_2_ToBuffer
		neg		ebx
		jmp		Sound4_2_ToBuffer

Sound4_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_2_ChangeStage
		neg		ebx
		jmp		Sound4_2_ChangeStage

Sound4_2_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_2_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jnz		Sound4_2_MixEbxEcx
		neg		ecx
		//jmp		Sound4_2_MixEbxEcx

Sound4_2_MixEbxEcx:
		add		ebx, ecx

Sound4_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_2_NotLeft
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
Sound4_2_NotLeft:
		test	cl, 0x80
		jz		Sound4_2_NotRight
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound4_2_NotRight:

Sound4_2_Off:



Sound_NoUpdate:
		mov		ecx, ebp



		//*******
		//Divider

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + Offset_DIV_Ticks]
		//bl	= DIV_Ticks
		sub		bl, al	//DIV_Ticks -= Ticks
		jnc		DIV_NotEnoughTicks
		mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F04], MEM_CHANGED
		//dl	= FF00(0x04)
		add		bl, 32	//DIV_Ticks += 32
		inc		dl
		mov		byte ptr [ecx + FF00_ASM + 0x04], dl
		//dl	free
		//edx	free
DIV_NotEnoughTicks:
		mov		byte ptr [ecx + Offset_DIV_Ticks], bl
		//bl	free
		//ebx	free

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags


		//*****
		//Timer

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + FF00_ASM + 0x07]
		test	bl, 0x04
		jz		TimerDisabled
		mov		ebx, dword ptr [ecx + Offset_TIMA_Ticks]
		//ebx	= TIMA_Ticks
		add		ebx, eax
		//eax	free
		mov		edx, dword ptr [ecx + Offset_Hz]
		//edx	= Hz
		//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
		cmp		ebx, edx
		jb		TIMA_NotEnoughTicks

		sub		ebx, edx
		//edx	free
		mov		dl, byte ptr [ecx + FF00_ASM + 0x05]
		//dl	= FF00(0x05)
		inc		dl
		jnz		TIMA_NotZero
		mov		dl, byte ptr [ecx + FF00_ASM + 0x06]
		//dl	= FF00(0x06) (new value of FF00(0x05))
		mov		dh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dh, 0x04
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dh
		//or		byte ptr [ecx + FF00_ASM + 0x0F], 0x04
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
TIMA_NotZero:
		mov		byte ptr [ecx + FF00_ASM + 0x05], dl
		//dl	free
		//edx	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F05], MEM_CHANGED

TIMA_NotEnoughTicks:
		mov		dword ptr [ecx + Offset_TIMA_Ticks], ebx
		//ebx	free

TimerDisabled:
		//ecx	= this
		//esi	= Flags


		//**********
		//Interrupts

		//ecx	= this
		//esi	= Flags

		test	esi, GB_IE
		jz		NoInterrupt
		mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
		//ah	= FF00(0x0F)
		mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
		and		al, ah
		//al	= FF00(0xFF) & FF00(0x0F)
		test	al, 0x0F
		jz		InterruptServiced

		and		esi, ~(GB_IE | GB_ENABLEIE | GB_HALT)
		mov		dword ptr [ecx + Offset_Flags], esi


		push	ecx
		xor		ecx, ecx
		shr		al, 1
		jc		InterruptFound
TestNextInterrupt:
		inc		cl
		shr		al, 1
		jnc		TestNextInterrupt

InterruptFound:
		//al	free
		mov		bl, 1
		shl		bl, cl
		not		bl
		and		ah, bl
		//ah	= new value of FF0F
		lea		ebx, [ecx * 8 + 0x40]
		//ebx	= interrupt address
		pop		ecx
		//ecx	= this

		mov		edx, dword ptr [ecx + Offset_Reg_SP]
		//edx	= Reg_SP
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		Interrupt_WriteAccessDenied

		mov		byte ptr [ecx + FF00_ASM + 0x0F], ah
		//ah	free
		//eax	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		mov		eax, dword ptr [ecx + Offset_Reg_PC]
		//eax	= Reg_PC

		mov		word ptr [ecx + Offset_Reg_PC], bx
		//ebx	free

		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		edx, ebx
		//edx	= Reg_PC (new)
		//ebx	free

		call	RetrieveAccess
		//al	= access
		//edx	free
		test	al, MEM_BREAKPOINT
		//al	free
		jz		Int_NotBreakPoint
		or		esi, GB_EXITLOOP | GB_ERROR
Int_NotBreakPoint:

		jmp		InterruptServiced

NoInterrupt:
		//ecx	= this
		//esi	= Flags

		test	esi, GB_ENABLEIE
		jz		InterruptServiced
		or		esi, GB_IE

InterruptServiced:
		//ecx	= this
		//esi	= Flags


		//*********
		//Exit loop

		//ecx	= this
		//esi	= Flags

		test	esi, GB_EXITLOOP
		jz		ContinueLoop

ExitLoop:
		and		esi, ~GB_EXITLOOP
		mov		dword ptr [ecx + Offset_Flags], esi
		//esi	free

		pop		ebx
		pop		edx
		pop		edi
		pop		esi
		pop		ebp
		ret

ExecuteAccessDenied:
		push	offset ExecuteAccessDeniedMsg
		push	ecx
		call	AccessDenied
		add		esp, 8
		jmp		ExitLoop

ReadAccessDenied:
		push	offset ReadAccessDeniedMsg
		push	ecx
		call	AccessDenied
		add		esp, 8
		jmp		ExitLoop

Interrupt_WriteAccessDenied:
		push	offset InterruptWriteAccessDeniedMsg
		push	ecx
		call	AccessDenied
		add		esp, 8
		jmp		ExitLoop
	}

		//Serial transfer
		/*if (SIOClocks)
		{
			if (SIOClocks <= Ticks)
			{
				SIOClocks = 0;

				if (Connected)
				{
					SIO = SIO_SEND;
					return;
				}

				//if (pFF00_C(0x02) & 0x01)
				//{
					/*GB->MEM_GB[0x2F01] = 0xFF;
					GB->MEM_GB[0x2F02] &= ~0x80;
					GB->MEM_GB[0x2F0F] |= 0x08;*/
				//}
/*			}
			else
			{
				SIOClocks -= Ticks;
			}
		}*/
}



void CGameBoy::RefreshScreen()
{
	if (FramesToSkip)
	{
		FramesToSkip--;
	}
	else
	{
		InvalidateRect(hGBWnd, NULL, false);
		PostMessage(hGBWnd, WM_PAINT, 0, 0);
		if (FastFwd)
		{
			FramesToSkip = 9;
		}
		else
		{
			FramesToSkip = FrameSkip;
		}
	}
}



void __declspec(naked) __fastcall SoundUpdate(CGameBoy *GB)
{
	//
	__asm
	{
		mov		esi, dword ptr [ecx + Offset_hWaveOut]
		mov		al, byte ptr [ecx + Offset_FastFwd]
		mov		edi, dword ptr [ecx + Offset_SoundBufferPosition]
		test	esi, esi
		jz		NoSound
		test	al, al
		jz		SoundEnabled
		test	edi, edi
		jnz		NoSound

SoundEnabled:
		mov		esi, dword ptr [ecx + Offset_pSoundBuffer]


		mov		ebx, dword ptr [ecx + Offset_SoundR]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		R_AboveMin
		xor		al, al
		jmp		R_WithinLimits
R_AboveMin:
		cmp		eax, 127
		jle		R_BelowMax
		mov		al, 127
R_BelowMax:
		add		al, 0x80

R_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundR], 0

		mov		byte ptr [esi + edi], al
		inc		edi


		mov		ebx, dword ptr [ecx + Offset_SoundL]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		L_AboveMin
		xor		al, al
		jmp		L_WithinLimits
L_AboveMin:
		cmp		eax, 127
		jle		L_BelowMax
		mov		al, 127
L_BelowMax:
		add		al, 0x80

L_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundL], 0

		mov		byte ptr [esi + edi], al
		inc		edi


		mov		al, byte ptr [ecx + Offset_SoundTicks]
		add		al, 96
		mov		byte ptr [ecx + Offset_SoundTicks], al

		cmp		edi, 2048
		jae		BufferFilled


		mov		dword ptr [ecx + Offset_SoundBufferPosition], edi
NoSound:
		ret


BufferFilled:
		mov		dword ptr [esi + 2048 +  0], esi
		mov		dword ptr [esi + 2048 +  4], 2048
		mov		dword ptr [esi + 2048 + 16], 0

		mov		dword ptr [ecx + Offset_SoundBufferPosition], 0

		//waveOutWrite
		push	32 //sizeof(WAVEHDR)
		add		esi, 2048
		push	esi
		mov		eax, dword ptr [ecx + Offset_hWaveOut]
		push	eax

		//waveOutPrepareHeader
		push	32
		push	esi
		push	eax


		mov		esi, ecx

Delay:
		cmp		byte ptr [esi + Offset_nSoundBuffers], 3
		jbe		NoDelay

		push	0
		call	dword ptr [Sleep]
		jmp		Delay
NoDelay:

		//EnterCriticalSection
		push	offset csSound
		call	dword ptr [EnterCriticalSection]

		call	dword ptr [waveOutPrepareHeader]
		call	dword ptr [waveOutWrite]

		inc		byte ptr [esi + Offset_nSoundBuffers]

		push	offset csSound
		call	dword ptr [LeaveCriticalSection]

		push	2048 + 32 //sizeof(SOUNDBUFFER)
		call	new
		add		esp, 4
		mov		dword ptr [esi + Offset_pSoundBuffer], eax
		test	eax, eax
		jz		OutOfMemory

		ret

OutOfMemory:
		//MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_OK | MB_ICONERROR);
		//GB->CloseSound();

		dec		byte ptr [esi + Offset_nSoundBuffers]

		push	offset csSound
		call	dword ptr [LeaveCriticalSection]

		push	esi
		call	CloseSound_asm
		add		esp, 4

		push	MB_OK | MB_ICONERROR
		push	NULL
		push	offset OutOfMemoryMsg
		push	dword ptr [hWnd]
		call	dword ptr [MessageBox]
		ret


/*BufferFilled:
		lea		esi, [ecx + Offset_SoundBuffer + Offset_csSound]
		push	esi
		call	dword ptr [EnterCriticalSection]

		mov		dword ptr [ecx + Offset_SoundBuffer + Offset_dwPosition], 0

Delay:
		cmp		byte ptr [esi + Offset_SoundBuffer + Offset_dwPosition], BufferSize
		jb		NoDelay

		push	0
		call	dword ptr [Sleep]
		jmp		Delay
NoDelay:

		//EnterCriticalSection
		add		esi, Offset_SoundBuffer + Offset_csSound
		push	esi
		call	dword ptr [EnterCriticalSection]

		call	dword ptr [waveOutPrepareHeader]
		call	dword ptr [waveOutWrite]

		push	esi
		call	dword ptr [LeaveCriticalSection]

		ret*/
	}

	/*/
	int			l, r;
	__asm int 3;


	if (!GB->hWaveOut || (GB->FastFwd && GB->SoundBufferPosition == 0))
	{
		return;
	}


	l = ((signed)GB->SoundL) / ((signed)96);
	r = ((signed)GB->SoundR) / ((signed)96);
	GB->SoundL = 0;
	GB->SoundR = 0;


	l >>= 2;
	if (l < -128)
	{
		l = -128;
	}
	if (l > 127)
	{
		l = 127;
	}
	l += 0x80;
	r >>= 2;
	if (r < -128)
	{
		r = -128;
	}
	if (r > 127)
	{
		r = 127;
	}
	r += 0x80;


	GB->SoundBuffer->Data[GB->SoundBufferPosition++] = (BYTE)r;
	GB->SoundBuffer->Data[GB->SoundBufferPosition++] = (BYTE)l;
	if (GB->SoundBufferPosition >= sizeof(GB->SoundBuffer->Data))
	{
		ZeroMemory(&GB->SoundBuffer->wh, sizeof(GB->SoundBuffer->wh));
		GB->SoundBuffer->wh.lpData = (char *)&GB->SoundBuffer->Data;
		GB->SoundBuffer->wh.dwBufferLength = sizeof(GB->SoundBuffer->Data);

		EnterCriticalSection(&csSound);
		waveOutPrepareHeader(GB->hWaveOut, &GB->SoundBuffer->wh, sizeof(GB->SoundBuffer->wh));
		waveOutWrite(GB->hWaveOut, &GB->SoundBuffer->wh, sizeof(GB->SoundBuffer->wh));
		if (!(GB->SoundBuffer = new SOUNDBUFFER))
		{
			MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_OK | MB_ICONERROR);
			GB->CloseSound();
		}
		LeaveCriticalSection(&csSound);
		GB->SoundBufferPosition = 0;
	}

	GB->SoundTicks += 96;//*/
	/*
00410FA4 8B 86 98 A1 03 00    mov         eax,dword ptr [esi+3A198h]
00410FAA 85 C0                test        eax,eax
00410FAC 0F 84 A3 01 00 00    je          00411155
00410FB2 8A 86 A4 A1 03 00    mov         al,byte ptr [esi+3A1A4h]
00410FB8 84 C0                test        al,al
00410FBA 74 0E                je          00410FCA
00410FBC 8B 86 A0 A1 03 00    mov         eax,dword ptr [esi+3A1A0h]
00410FC2 85 C0                test        eax,eax
00410FC4 0F 84 8B 01 00 00    je          00411155
00410FCA 8B 8E 50 A1 03 00    mov         ecx,dword ptr [esi+3A150h]
00410FD0 B8 AB AA AA 2A       mov         eax,2AAAAAABh
00410FD5 F7 E9                imul        ecx
00410FD7 C1 FA 04             sar         edx,4
00410FDA 8B C2                mov         eax,edx
00410FDC C7 86 50 A1 03 00 00 mov         dword ptr [esi+3A150h],0
00410FE6 C1 E8 1F             shr         eax,1Fh
00410FE9 03 D0                add         edx,eax
00410FEB B8 AB AA AA 2A       mov         eax,2AAAAAABh
00410FF0 8B CA                mov         ecx,edx
00410FF2 8B 96 54 A1 03 00    mov         edx,dword ptr [esi+3A154h]
00410FF8 F7 EA                imul        edx
00410FFA C1 FA 04             sar         edx,4
00410FFD 8B C2                mov         eax,edx
00410FFF C7 86 54 A1 03 00 00 mov         dword ptr [esi+3A154h],0
00411009 C1 E8 1F             shr         eax,1Fh
0041100C C1 F9 02             sar         ecx,2
0041100F 03 D0                add         edx,eax
00411011 83 F9 80             cmp         ecx,80h
00411014 7D 07                jge         0041101D
00411016 B9 80 FF FF FF       mov         ecx,0FFFFFF80h
0041101B EB 0A                jmp         00411027
0041101D 83 F9 7F             cmp         ecx,7Fh
00411020 7E 05                jle         00411027
00411022 B9 7F 00 00 00       mov         ecx,7Fh
00411027 C1 FA 02             sar         edx,2
0041102A 81 C1 80 00 00 00    add         ecx,80h
00411030 83 FA 80             cmp         edx,80h
00411033 7D 07                jge         0041103C
00411035 BA 80 FF FF FF       mov         edx,0FFFFFF80h
0041103A EB 0A                jmp         00411046
0041103C 83 FA 7F             cmp         edx,7Fh
0041103F 7E 05                jle         00411046
00411041 BA 7F 00 00 00       mov         edx,7Fh
00411046 8B 86 A0 A1 03 00    mov         eax,dword ptr [esi+3A1A0h]
0041104C 57                   push        edi
0041104D 8B BE 9C A1 03 00    mov         edi,dword ptr [esi+3A19Ch]
00411053 80 C2 80             add         dl,80h
00411056 88 14 38             mov         byte ptr [eax+edi],dl
00411059 8B BE A0 A1 03 00    mov         edi,dword ptr [esi+3A1A0h]
0041105F 8B 96 9C A1 03 00    mov         edx,dword ptr [esi+3A19Ch]
00411065 47                   inc         edi
00411066 8B C7                mov         eax,edi
00411068 89 BE A0 A1 03 00    mov         dword ptr [esi+3A1A0h],edi
0041106E 88 0C 10             mov         byte ptr [eax+edx],cl
00411071 8B 96 A0 A1 03 00    mov         edx,dword ptr [esi+3A1A0h]
00411077 42                   inc         edx
00411078 8B C2                mov         eax,edx
0041107A 89 96 A0 A1 03 00    mov         dword ptr [esi+3A1A0h],edx
00411080 3D 00 04 00 00       cmp         eax,400h
00411085 0F 82 BB 00 00 00    jb          00411146
0041108B 8B BE 9C A1 03 00    mov         edi,dword ptr [esi+3A19Ch]
00411091 B9 08 00 00 00       mov         ecx,8
00411096 33 C0                xor         eax,eax
00411098 81 C7 00 04 00 00    add         edi,400h
0041109E F3 AB                rep stos    dword ptr [edi]
004110A0 8B 86 9C A1 03 00    mov         eax,dword ptr [esi+3A19Ch]
004110A6 68 A0 7E 42 00       push        427EA0h
004110AB 89 80 00 04 00 00    mov         dword ptr [eax+400h],eax
004110B1 8B 86 9C A1 03 00    mov         eax,dword ptr [esi+3A19Ch]
004110B7 C7 80 04 04 00 00 00 mov         dword ptr [eax+404h],400h
004110C1 FF 15 CC 10 42 00    call        dword ptr ds:[4210CCh]
004110C7 8B 8E 9C A1 03 00    mov         ecx,dword ptr [esi+3A19Ch]
004110CD 8B 96 98 A1 03 00    mov         edx,dword ptr [esi+3A198h]
004110D3 81 C1 00 04 00 00    add         ecx,400h
004110D9 6A 20                push        20h
004110DB 51                   push        ecx
004110DC 52                   push        edx
004110DD FF 15 94 12 42 00    call        dword ptr ds:[421294h]
004110E3 8B 86 9C A1 03 00    mov         eax,dword ptr [esi+3A19Ch]
004110E9 8B 8E 98 A1 03 00    mov         ecx,dword ptr [esi+3A198h]
004110EF 05 00 04 00 00       add         eax,400h
004110F4 6A 20                push        20h
004110F6 50                   push        eax
004110F7 51                   push        ecx
004110F8 FF 15 98 12 42 00    call        dword ptr ds:[421298h]
004110FE 68 20 04 00 00       push        420h
00411103 E8 D4 B9 00 00       call        0041CADC
00411108 83 C4 04             add         esp,4
0041110B 89 86 9C A1 03 00    mov         dword ptr [esi+3A19Ch],eax
00411111 85 C0                test        eax,eax
00411113 75 1C                jne         00411131
00411115 8B 15 78 7F 42 00    mov         edx,dword ptr ds:[427F78h]
0041111B 6A 10                push        10h
0041111D 50                   push        eax
0041111E 68 14 52 42 00       push        425214h
00411123 52                   push        edx
00411124 FF 15 3C 12 42 00    call        dword ptr ds:[42123Ch]
0041112A 8B CE                mov         ecx,esi
0041112C E8 3F 05 00 00       call        00411670
00411131 68 A0 7E 42 00       push        427EA0h
00411136 FF 15 74 10 42 00    call        dword ptr ds:[421074h]
0041113C C7 86 A0 A1 03 00 00 mov         dword ptr [esi+3A1A0h],0
00411146 8A 86 43 A1 03 00    mov         al,byte ptr [esi+3A143h]
0041114C 5F                   pop         edi
0041114D 04 60                add         al,60h
0041114F 88 86 43 A1 03 00    mov         byte ptr [esi+3A143h],al
00411155 5E                   pop         esi
00411156 C3                   ret
	*/
}



void __fastcall Sound1(CGameBoy *GB)
{
	/*__asm
	{
		mov		al, byte ptr [ecx + FF00_ASM + 0x26]
		test	al, 0x80
		jz		Sound1_Off
		test	al, 0x01
		jz		Sound1_Off

		xor		eax, eax

		mov		al, byte ptr [ecx + FF00_ASM + 0x14]
		test	al, 0x80
		jz		Sound1_NotUpdated
		test	al, 0x40
		jz		Sound1_NoTimeOut

		mov		al, 
	}*/
	if ((pFF00_C(GB, 0x26) & 0x81) == 0x81)
	{
		if (pFF00_C(GB, 0x14) & 0x80)
		{
			if (pFF00_C(GB, 0x14) & 0x40)
			{
				GB->Sound1TimeOut = (64 - (pFF00_C(GB, 0x11) & 0x3F)) << 13;
			}
			else
			{
				GB->Sound1TimeOut = 0;
			}

			pFF00_C(GB, 0x14) &= ~0x80;
		}

		GB->Sound1Enabled = true;

		GB->Sound1Frequency = (((pFF00_C(GB, 0x14) & 0x07) << 8) | pFF00_C(GB, 0x13));
		GB->Sound1Volume = pFF00_C(GB, 0x12) >> 4;
		GB->Sound1Envelope = ((DWORD)pFF00_C(GB, 0x12) & 0x07) * 16384;

		if (pFF00_C(GB, 0x10) & 0x07 && pFF00_C(GB, 0x10) & 0x70)
		{
			GB->Sound1Sweep = 8192 * ((pFF00_C(GB, 0x10) & 0x70) >> 4);
		}
		else
		{
			GB->Sound1Sweep = 0;
		}
	}
	else
	{
		GB->Sound1Enabled = false;
		pFF00_C(GB, 0x26) &= ~0x01;
	}
}



void __fastcall Sound2(CGameBoy *GB)
{
	if ((pFF00_C(GB, 0x26) & 0x82) == 0x82)
	{
		if (pFF00_C(GB, 0x19) & 0x80)
		{
			if (pFF00_C(GB, 0x19) & 0x40)
			{
				GB->Sound2TimeOut = (64 - (pFF00_C(GB, 0x16) & 0x3F)) << 13;
			}
			else
			{
				GB->Sound2TimeOut = 0;
			}

			pFF00_C(GB, 0x19) &= ~0x80;
		}

		GB->Sound2Enabled = true;

		GB->Sound2Frequency = (((pFF00_C(GB, 0x19) & 0x07) << 8) | pFF00_C(GB, 0x18));
		GB->Sound2Volume = pFF00_C(GB, 0x17) >> 4;
		GB->Sound2Envelope = ((DWORD)pFF00_C(GB, 0x17) & 0x07) * 16384;
	}
	else
	{
		GB->Sound2Enabled = false;
		pFF00_C(GB, 0x26) &= ~0x02;
	}
}



void __fastcall Sound3(CGameBoy *pGameBoy)
{
	if ((pFF00_C(pGameBoy, 0x26) & 0x84) == 0x84)
	{
		if (pFF00_C(pGameBoy, 0x1E) & 0x80)
		{
			if (pFF00_C(pGameBoy, 0x1E) & 0x40)
			{
				pGameBoy->Sound3TimeOut = (256 - pFF00_C(pGameBoy, 0x1B)) << 13;
			}
			else
			{
				pGameBoy->Sound3TimeOut = 0;
			}

			pFF00_C(pGameBoy, 0x1E) &= ~0x80;
		}

		pGameBoy->Sound3Enabled = true;

		pGameBoy->Sound3Frequency = ((pFF00_C(pGameBoy, 0x1E) & 0x07) << 8) | pFF00_C(pGameBoy, 0x1D);
	}
	else
	{
		pGameBoy->Sound3Enabled = false;
		pFF00_C(pGameBoy, 0x26) &= ~0x04;
	}
}



void __fastcall Sound4(CGameBoy *pGameBoy)
{
	if ((pFF00_C(pGameBoy, 0x26) & 0x88) == 0x88)
	{
		if (pFF00_C(pGameBoy, 0x23) & 0x80)
		{
			if (pFF00_C(pGameBoy, 0x23) & 0x40)
			{
				pGameBoy->Sound4TimeOut = (64 - (pFF00_C(pGameBoy, 0x20) & 0x3F)) << 13;
			}
			else
			{
				pGameBoy->Sound4TimeOut = 0;
			}

			pFF00_C(pGameBoy, 0x23) &= ~0x80;
		}

		pGameBoy->Sound4Enabled = true;

		pGameBoy->Sound4Volume = pFF00_C(pGameBoy, 0x21) >> 4;
		pGameBoy->Sound4Envelope = ((DWORD)pFF00_C(pGameBoy, 0x21) & 0x07) * 16384;
		switch(pFF00_C(pGameBoy, 0x22) & 7)
		{
			case 0: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) * 2) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 1: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) * 1) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 2: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 2) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 3: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 3) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 4: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 4) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 5: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 5) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 6: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 6) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 7: pGameBoy->Sound4Frequency = 2097152 / (((512 * 1024) / 7) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
		}
	}
	else
	{
		pGameBoy->Sound4Enabled = false;
		pFF00_C(pGameBoy, 0x26) &= ~0x08;
	}
}



void CALLBACK WaveCallback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if(uMsg == WOM_DONE)
	{
		EnterCriticalSection(&csSound);
		((CGameBoy *)dwInstance)->nSoundBuffers--;
		waveOutUnprepareHeader((HWAVEOUT)hWave, (WAVEHDR *)dwParam1, sizeof(WAVEHDR));
		delete ((WAVEHDR *)dwParam1)->lpData;
		LeaveCriticalSection(&csSound);
	}
}



BOOL CGameBoy::RestoreSound()
{
	WAVEFORMATEX	wfx;


	EnterCriticalSection(&csSound);
	if (!hWaveOut)
	{
		if (waveOutGetNumDevs() == 0)
		{
			LeaveCriticalSection(&csSound);
			MessageBox(NULL, "No audio device present.", NULL, MB_OK | MB_ICONERROR);
			return true;
		}

		ZeroMemory(&wfx, sizeof(wfx));
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 2;
		wfx.nSamplesPerSec = 22050;
		wfx.wBitsPerSample = 8;
		wfx.nBlockAlign = 2;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD)WaveCallback /*hWnd*/, (DWORD)this, CALLBACK_FUNCTION /*CALLBACK_WINDOW*/))
		{
			LeaveCriticalSection(&csSound);
			MessageBox(NULL, "Could not open audio device.", NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		waveOutReset(hWaveOut);
	}

	if (!SoundBuffer)
	{
		if (!(SoundBuffer = new SOUNDBUFFER))
		{
			LeaveCriticalSection(&csSound);
			CloseSound();
			MessageBox(hWnd, OutOfMemoryMsg, NULL, MB_OK | MB_ICONERROR);
			return true;
		}
	}
	SoundBufferPosition = 0;
	SoundL = SoundR = 0;
	SoundL = SoundR = 0;
	nSoundBuffers = 0;
	LeaveCriticalSection(&csSound);

	return false;
}



void CGameBoy::CloseSound()
{
	HWAVEOUT		hWave;


	if (hWaveOut)
	{
		hWave = hWaveOut;
		hWaveOut = NULL;
		while (nSoundBuffers)
		{
			Sleep(0);
		}
		EnterCriticalSection(&csSound);
		waveOutClose(hWave);
		LeaveCriticalSection(&csSound);
	}
	if (SoundBuffer)
	{
		delete SoundBuffer;
		SoundBuffer = NULL;
	}
}



void CloseSound_asm(CGameBoy *pGameBoy)
{
	pGameBoy->CloseSound();
}

