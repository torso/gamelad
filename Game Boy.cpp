#include	<windows.h>

#define		GAME_BOY_CPP
#include	"Game Lad.h"
#include	"Z80.h"
#include	"Debugger.h"
#include	"resource.h"



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
}



CGameBoy::~CGameBoy()
{
	CloseSound();

	delete MEM_ROM;
	delete MemStatus_ROM;
	delete MemStatus_RAM;

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



BOOL CGameBoy::Init(char *pszROMFilename, char *pszBatteryFilename)
{
	HANDLE			hFile;
	DWORD			FileSize, nBytes;
	BITMAPINFO		bmi;
	BOOL			Maximized;


	strcpy(Rom, pszROMFilename);

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
		MessageBox(hWnd, "Insufficient memory.", NULL, MB_ICONERROR | MB_OK);
		return true;
	}
	if (!(MemStatus_ROM = new BYTE[FileSize]))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, "Insufficient memory.", NULL, MB_ICONERROR | MB_OK);
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
		MemStatus_RAM = new BYTE[(MaxRamBank + 1) * 0x2000];
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

	LoadBattery(pszBatteryFilename);
	Reset();

	SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, (LPARAM)&Maximized);

	//Create window
	if (!(hGBWnd = CreateWindowEx(WS_EX_MDICHILD, "Game Boy", Rom, Maximized ? WS_VISIBLE | WS_MAXIMIZE : WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 160 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), hClientWnd, NULL, hInstance, NULL)))
	{
		DisplayErrorMessage(hWnd);
		return true;
	}
	SetWindowLong(hGBWnd, GWL_USERDATA, (long)this);

	return false;
}



/*BOOL CGameBoy::LoadBattery()
{
	if (!Battery[0])
	{
		GetBatteryFilename(Battery);
	}
	return LoadBattery(Battery);
}*/



BOOL CGameBoy::LoadBattery(char *BatteryFilename)
{
	HANDLE		hFile;
	DWORD		nBytes;


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

			ZeroMemory(&MEM_RAM, sizeof(MEM_RAM));
			BatteryAvailable = false;
			Battery[0] = 0;

			MessageBox(hWnd, "Error loading battery ram.", NULL, MB_OK | MB_ICONWARNING);

			Reset();
			return true;
		}
		else
		{
			CloseHandle(hFile);
		}
	}

	Reset();
	return false;
}



BOOL CGameBoy::SaveBattery(BOOL Prompt, BOOL SaveAs)
{
	HANDLE			hFile;
	DWORD			nBytes;
	BYTE			*Buffer;
	OPENFILENAME	of;


	if (!SaveRamSize || !BatteryAvailable)
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
		//Compare file with ram
		if ((hFile = CreateFile(Battery, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
		{
			if (Buffer = new BYTE[SaveRamSize])
			{
				if (ReadFile(hFile, Buffer, SaveRamSize, &nBytes, NULL))
				{
					if (!memcmp(Buffer, &MEM_RAM, SaveRamSize))
					{
						//File same as ram
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
			return true;
		}
	}

	while (!WriteFile(hFile, &MEM_RAM, SaveRamSize, &nBytes, NULL) && nBytes != SaveRamSize)
	{
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		SetEndOfFile(hFile);
		if (MessageBox(hWnd, "Error while saving.", NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			CloseHandle(hFile);
			DeleteFile(Battery);
			return true;
		}
	}

	CloseHandle(hFile);

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



LPARAM CGameBoy::GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_VIEW_ZOOM_100:
			GetWindowRect(hWnd, &Rect2);
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			MoveWindow(hGBWnd, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 160 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_200:
			GetWindowRect(hWnd, &Rect2);
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			MoveWindow(hGBWnd, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 160 * 2 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 2 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_300:
			GetWindowRect(hWnd, &Rect2);
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			MoveWindow(hGBWnd, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 160 * 3 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 3 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_400:
			GetWindowRect(hWnd, &Rect2);
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			MoveWindow(hGBWnd, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 160 * 4 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 4 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
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
			Buttons |= 0x02;
			return 0;
		}
		if (wParam == Keys.B)
		{
			Buttons |= 0x01;
			return 0;
		}
		if (wParam == Keys.FastForward)
		{
			FastFwd = true;
			SoundBufferPosition = 0;
			Sound1L = Sound1R = 0;
			Sound2L = Sound2R = 0;
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
			Buttons &= ~0x02;
			return 0;
		}
		if (wParam == Keys.B)
		{
			Buttons &= ~0x01;
			return 0;
		}
		if (wParam == Keys.FastForward)
		{
			FastFwd = false;
			Sound1L = Sound1R = 0;
			Sound2L = Sound2R = 0;
		}
		return 0;
	}

	return DefMDIChildProc(hGBWnd, uMsg, wParam, lParam);
}



void CGameBoy::Reset(DWORD Flags)
{
	this->Flags = Flags & GB_COLOR;
	Reset();
}



void CGameBoy::Reset()
{
	DWORD		RamSize, pByte;


	ZeroMemory(&MEM_CPU, sizeof(MEM_CPU));
	ZeroMemory(&MEM_VRAM, sizeof(MEM_VRAM));
	ZeroMemory(&BGP, sizeof(BGP));
	ZeroMemory(&OBP, sizeof(OBP));

	Flags &= GB_COLOR;
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



void CGameBoy::PrepareEmulation(BOOL Debug)
{
	DWORD	pByte, RamSize;


	Flags &= ~(GB_ERROR | GB_INVALIDOPCODE);
	BatteryAvailable = true;
	DirectionKeys = 0;
	Buttons = 0;
	RestoreSound();

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
		MemStatus[0xC] = &MemStatus_CPU[(FF00_C(0x70) & 7) * 0x2000];
		MemStatus[0xD] = &MemStatus_CPU[(FF00_C(0x70) & 7) * 0x2000 + 0x1000];
	}
	else
	{
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

void CGameBoy::MainLoop()
{
	BYTE			Ticks, Ticks2, Ticks3;
	WORD			Sprites[10];
	MSG				msg;


	do
	{
//ContinueLoop:
		if (Flags & GB_HALT)
		{
			Ticks = 1;
			nCycles++;
		}
		else
		{
		/*__asm
		{
			mov		ecx, this
			test	dword ptr [ecx + Offset_Flags], GB_HALT
			jz		NotHalt

			mov		eax, dword ptr [ecx + Offset_nCycles]
			mov		byte ptr [Ticks], 1
			inc		eax
			mov		dword ptr [ecx + Offset_nCycles], eax
			//inc		dword ptr [ecx + Offset_nCycles]
			jmp		HaltComplete*/

			//Because OpCodes functions can change eax, ebx and edx, all these registers should be
			//used within this __asm block. If not, an optimized compilation will result in strange errors.
			__asm
			{
//NotHalt:
				mov		ecx, this
				mov		edx, dword ptr [ecx + Offset_Reg_PC]
				call	ReadMem
				and		eax, 0xFF
				call	dword ptr [OpCodes + 4 * eax]

				test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
				jnz		FastCPU
				shl		al, 1
FastCPU:
				mov		ebx, dword ptr [ecx + Offset_nCycles]
				mov		byte ptr [Ticks], al
				add		ebx, eax
				mov		dword ptr [ecx + Offset_nCycles], ebx

				//test	byte ptr [ecx + Offset_Flags], 0//GB_EXITLOOPDIRECTLY
				//jnz		ExitLoop
				//int		3
			}
//HaltComplete:

			if (Flags & GB_EXITLOOPDIRECTLY)
			{
				Flags &= ~(GB_EXITLOOPDIRECTLY | GB_EXITLOOP);
				return;
			}
		}


		//LCD
		if (LCD_Ticks <= Ticks)
		{
			if (FF00_C(0x40) & 0x80) //LCD on
			{
				switch (FF00_C(0x41) & 3)
				{
				case 3: //Transfer to LCD
					LCD_Ticks += 104;

					FF00_C(0x41) &= ~0x03;		//H-Blank

					//HDMA
					if (Flags & GB_HDMA)
					{
						__asm
						{
							mov		ecx, this

							mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
							and		ebx, 1
							shl		ebx, 13

							mov		esi, dword ptr [ecx + FF00_ASM + 0x51]
							mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
							dec		byte ptr [ecx + FF00_ASM + 0x55]
							rol		si, 8
							rol		di, 8
							and		esi, 0xFFF0
							and		edi, 0x1FF0

							add		edi, ebx

							mov		eax, esi
							add		eax, 0x0010
							mov		byte ptr [ecx + FF00_ASM + 0x52], al
							mov		byte ptr [ecx + FF00_ASM + 0x51], ah
							mov		eax, edi
							add		eax, 0x0010
							mov		byte ptr [ecx + FF00_ASM + 0x54], al
							mov		byte ptr [ecx + FF00_ASM + 0x53], ah

							mov		eax, esi
							shr		esi, 12
							and		eax, 0x0FFF
							mov		esi, [ecx + Offset_MEM + esi * 4]

							mov		edx, [esi + eax]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
							mov		edx, [esi + eax + 0x04]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
							mov		edx, [esi + eax + 0x08]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
							mov		edx, [esi + eax + 0x0C]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx

							mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
							test	bl, 0x80
							jz		HDMA_NotFinished
							and		dword ptr [ecx + Offset_Flags], ~GB_HDMA
HDMA_NotFinished:
						}
					}
					break;

				case 1: //V-Blank
					if (!FF00_C(0x44))
					{
						LCD_Ticks += 40;
						FF00_C(0x41) &= ~0x03;
						FF00_C(0x41) |= 0x02;	//OAM-RAM search

						//++DrawLineMask &= 1;

						WindowY = FF00_C(0x4A);
						WindowTileY = 0;
						WindowTileY2 = 0;

						break;
					}

					FF00_C(0x44)++;
					if (FF00_C(0x44) >= 154)
					{
						FF00_C(0x44) = 0;
					}

					LCD_Ticks += 228;

					if (FF00_C(0x41) & 0x40 && FF00_C(0x44) == FF00_C(0x45))
					{
						//LYC coincidence interrupt
						FF00_C(0x0F) |= 0x02;
					}
					break;

				case 0: //H-Blank
					FF00_C(0x44)++;

					if (FF00_C(0x44) >= 144)
					{
						LCD_Ticks += 228;

						FF00_C(0x41) &= ~0x03;
						FF00_C(0x41) |= 0x01;		//V-Blank

						//V-Blank interrupt
						FF00_C(0x0F) |= 1;

						Flags |= GB_EXITLOOP;
					}
					else
					{
						LCD_Ticks += 40;

						FF00_C(0x41) &= ~0x03;
						FF00_C(0x41) |= 0x02;		//OAM search
					}
					break;

				case 2: //OAM search
					LCD_Ticks += 84;
					FF00_C(0x41) |= 0x03;			//Transfer to LCD

					if (FF00_C(0x41) & 0x40 && FF00_C(0x44) == FF00_C(0x45))
					{
						//LYC coincidence interrupt
						FF00_C(0x0F) |= 0x02;
					}

					BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (/*DrawLineMask == (FF00_C(0x44) & 1) &&*/ FF00_C(0x44) < 144)
					{
						BYTE	WndX;
						__asm
						{
							mov		ecx, this


							mov		dword ptr [Line], 0
							mov		dword ptr [Line + 4], 0
							mov		dword ptr [Line + 8], 0
							mov		dword ptr [Line + 12], 0


							//Set pointer to bitmap
							movzx	edi, byte ptr [ecx + FF00_ASM + 0x44]
							imul	edi, (160 + 14) * 2
							add		edi, dword ptr [ecx + Offset_pGBBitmap]
							push	edi


							test	byte ptr [ecx + Offset_Flags], GB_ROM_COLOR
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
							cmp		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		Wnd_NoDraw		//LY too low

							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
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
							shr		dh, 1
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
							cmp		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		NC_Wnd_NoDraw		//LY too low

							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							cmp		bl, 143
							ja		NC_Wnd_NoDraw		//WY too high
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
						}
					}
				}
				LCD_Ticks -= Ticks;
			}
			else
			{
				LCD_Ticks = 0;
				FF00_C(0x44) = 0;
				FF00_C(0x41) &= ~0x03;

				if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE))
				{
					Flags |= GB_EXITLOOP;
				}
			}
		}
		else
		{
			LCD_Ticks -= Ticks;
		}



		if (Sound1Enabled)
		{
			if (Sound1TimeOut)
			{
				if (Sound1TimeOut <= Ticks)
				{
					Sound1Enabled = false;
					FF00_C(0x26) &= ~0x01;
				}
				else
				{
					Sound1TimeOut -= Ticks;
				}
			}
			if (Sound1Sweep)
			{
				if (Sound1Sweep <= Ticks)
				{
					Sound1Sweep = 0;

					if (FF00_C(0x10) & 0x08)
					{
						if (Sound1Frequency >= (Sound1Frequency >> (FF00_C(0x10) & 0x07)))
						{
							Sound1Frequency -= (Sound1Frequency >> (FF00_C(0x10) & 0x07));
						}
						else
						{
							Sound1Frequency = 0;
						}
					}
					else
					{
						Sound1Frequency += (Sound1Frequency >> (FF00_C(0x10) & 0x07));
						if (Sound1Frequency > 2047)
						{
							Sound1Enabled = false;
							FF00_C(0x26) &= ~0x01;
							Sound1Frequency = 2047;
						}
					}
					FF00_C(0x13) = (BYTE)((~Sound1Frequency) & 0xFF);
					FF00_C(0x14) = (BYTE)(((~Sound1Frequency) & 0x0700) >> 8);

					Sound1Sweep = 8192 * ((FF00_C(0x10) & 0x70) >> 4);
				}
				else
				{
					Sound1Sweep -= Ticks;
				}
			}
			if (Sound1Envelope)
			{
				if (Sound1Envelope <= Ticks)
				{
					if (FF00_C(0x12) & 0x08)
					{
						if ((FF00_C(0x12) & 0xF0) == 0xF0)
						{
							Sound1Enabled = false;
							FF00_C(0x26) &= ~0x01;
						}
						else
						{
							FF00_C(0x12) += 0x10;
							Sound1Volume = FF00_C(0x12) >> 4;
							Sound1Envelope = ((DWORD)FF00_C(0x12) & 0x07) * 16384;
						}
					}
					else
					{
						if (FF00_C(0x12) & 0xF0)
						{
							FF00_C(0x12) -= 0x10;
							Sound1Volume = FF00_C(0x12) >> 4;
						}
						Sound1Envelope = ((DWORD)FF00_C(0x12) & 0x07) * 16384;
					}
				}
				else
				{
					Sound1Envelope -= Ticks;
				}
			}


			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound1Ticks++;
				if (Sound1Ticks >= ((2048 - Sound1Frequency) << 3))
				{
					Sound1Ticks = 0;
					Sound1Stage = ++Sound1Stage & 7;
				}

				switch (FF00_C(0x11) >> 6)
				{
				case 0:
					if (Sound1Stage == 0)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 1:
					if (Sound1Stage <= 1)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 2:
					if (Sound1Stage <= 3)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 3:
					if (Sound1Stage <= 5)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				}
			}
		}

		if (Sound2Enabled)
		{
			if (Sound2TimeOut)
			{
				if (Sound2TimeOut <= Ticks)
				{
					Sound2Enabled = false;
					FF00_C(0x26) &= ~0x02;
				}
				else
				{
					Sound2TimeOut -= Ticks;
				}
			}
			if (Sound2Envelope)
			{
				if (Sound2Envelope <= Ticks)
				{
					if (FF00_C(0x17) & 0x08)
					{
						if ((FF00_C(0x17) & 0xF0) == 0xF0)
						{
							Sound2Enabled = false;
							FF00_C(0x26) &= ~0x02;
						}
						else
						{
							FF00_C(0x17) += 0x10;
							Sound2Volume = FF00_C(0x17) >> 4;
							Sound2Envelope = ((DWORD)FF00_C(0x17) & 0x07) * 16384;
						}
					}
					else
					{
						if (FF00_C(0x17) & 0xF0)
						{
							FF00_C(0x17) -= 0x10;
							Sound2Volume = FF00_C(0x17) >> 4;
						}
						Sound2Envelope = ((DWORD)FF00_C(0x17) & 0x07) * 16384;
					}
				}
				else
				{
					Sound2Envelope -= Ticks;
				}
			}


			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound2Ticks++;
				if (Sound2Ticks >= ((2048 - Sound2Frequency) << 3))
				{
					Sound2Ticks = 0;
					Sound2Stage = ++Sound2Stage & 7;
				}

				switch (FF00_C(0x16) >> 6)
				{
				case 0:
					if (Sound2Stage == 0)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 1:
					if (Sound2Stage <= 1)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 2:
					if (Sound2Stage <= 3)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 3:
					if (Sound2Stage <= 5)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				}
			}
		}

		if (Sound3Enabled)
		{
			if (Sound3TimeOut)
			{
				if (Sound3TimeOut <= Ticks)
				{
					Sound3Enabled = false;
					FF00_C(0x1A) &= ~0x80;
					FF00_C(0x26) &= ~0x04;
				}
			}

			Sound3TimeOut -= Ticks;

			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound3Ticks++;
				if (Sound3Ticks >= 2048 - Sound3Frequency)
				{
					Sound3Ticks = 0;
					Sound3Stage = ++Sound3Stage & 31;
				}

				BYTE	SoundBit;

				if (Sound3Stage & 1)
				{
					SoundBit = FF00_C(0x30 + (Sound3Stage >> 1)) & 0x0F;
				}
				else
				{
					SoundBit = FF00_C(0x30 + (Sound3Stage >> 1)) >> 4;
				}
				switch (FF00_C(0x1C) & 0x60)
				{
				case 0x00:
					SoundBit = 8;
					break;

				case 0x20:
					break;

				case 0x40:
					SoundBit = 8 | (SoundBit >> 1);
					break;

				case 0x60:
					SoundBit = 0xC | (SoundBit >> 2);
					break;
				}

				if (FF00_C(0x25) & 0x04)
				{
					Sound3L += (SoundBit << 4) - 0x80;
				}
				if (FF00_C(0x25) & 0x40)
				{
					Sound3R += (SoundBit << 4) - 0x80;
				}
			}
		}

		if (Sound4Enabled)
		{
			if (Sound4TimeOut)
			{
				if (Sound4TimeOut <= Ticks)
				{
					Sound4Enabled = false;
					FF00_C(0x26) &= ~0x08;
				}
				else
				{
					Sound4TimeOut -= Ticks;
				}
			}
			if (Sound4Envelope)
			{
				if (Sound4Envelope <= Ticks)
				{
					if (FF00_C(0x21) & 0x08)
					{
						if ((FF00_C(0x21) & 0xF0) == 0xF0)
						{
							Sound4Enabled = false;
							FF00_C(0x26) &= ~0x08;
						}
						else
						{
							FF00_C(0x21) += 0x10;
							Sound4Volume = FF00_C(0x21) >> 4;
							Sound4Envelope = ((DWORD)FF00_C(0x21) & 0x07) * 16384;
						}
					}
					else
					{
						if (FF00_C(0x21) & 0xF0)
						{
							FF00_C(0x21) -= 0x10;
							Sound4Volume = FF00_C(0x21) >> 4;
						}
						Sound4Envelope = ((DWORD)FF00_C(0x21) & 0x07) * 16384;
					}
				}
				else
				{
					Sound4Envelope -= Ticks;
				}
			}

			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound4Ticks++;
				if (Sound4Ticks >= Sound4Frequency)
				{
					Sound4Ticks = 0;
					Sound4Bit = rand() & 1;
				}

				if (Sound4Bit)
				{
					if (FF00_C(0x25) & 0x08)
					{
						Sound4L += Volume[Sound4Volume];
					}
					if (FF00_C(0x25) & 0x80)
					{
						Sound4R += Volume[Sound4Volume];
					}
				}
				else
				{
					if (FF00_C(0x25) & 0x08)
					{
						Sound4L -= Volume[Sound4Volume];
					}
					if (FF00_C(0x25) & 0x80)
					{
						Sound4R -= Volume[Sound4Volume];
					}
				}
			}
		}

		//__asm int 3
		if (SoundTicks < Ticks)
		{
			Ticks2 = Ticks - (BYTE)SoundTicks;
			SoundUpdate(this);

			if (Sound1Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound1Ticks++;
					if (Sound1Ticks >= ((2048 - Sound1Frequency) << 3))
					{
						Sound1Ticks = 0;
						Sound1Stage = ++Sound1Stage & 7;
					}


					switch (FF00_C(0x11) >> 6)
					{
					case 0:
						if (Sound1Stage == 0)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 1:
						if (Sound1Stage <= 1)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 2:
						if (Sound1Stage <= 3)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 3:
						if (Sound1Stage <= 5)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					}
				}
			}
			if (Sound2Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound2Ticks++;
					if (Sound2Ticks >= ((2048 - Sound2Frequency) << 3))
					{
						Sound2Ticks = 0;
						Sound2Stage = ++Sound2Stage & 7;
					}

					switch (FF00_C(0x16) >> 6)
					{
					case 0:
						if (Sound2Stage == 0)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 1:
						if (Sound2Stage <= 1)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 2:
						if (Sound2Stage <= 3)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 3:
						if (Sound2Stage <= 5)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					}
				}
			}

			if (Sound3Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound3Ticks++;
					if (Sound3Ticks >= 2048 - Sound3Frequency)
					{
						Sound3Ticks = 0;
						Sound3Stage = ++Sound3Stage & 31;
					}

					BYTE	Sound3Bit;

					if (Sound3Stage & 1)
					{
						Sound3Bit = FF00_C(0x30 + (Sound3Stage >> 1)) & 0x0F;
					}
					else
					{
						Sound3Bit = FF00_C(0x30 + (Sound3Stage >> 1)) >> 4;
					}
					switch (FF00_C(0x1C) & 0x60)
					{
					case 0x00:
						Sound3Bit = 8;
						break;

					case 0x20:
						break;

					case 0x40:
						Sound3Bit = 8 | (Sound3Bit >> 1);
						break;

					case 0x60:
						Sound3Bit = 0xC | (Sound3Bit >> 2);
						break;
					}

					if (FF00_C(0x25) & 0x04)
					{
						Sound3L += (Sound3Bit << 4) - 0x80;
					}
					if (FF00_C(0x25) & 0x40)
					{
						Sound3R += (Sound3Bit << 4) - 0x80;
					}
				}
			}

			if (Sound4Enabled)
			{
				while (Ticks2 != 0)
				{
					Ticks2--;
					Sound4Ticks++;
					if (Sound4Ticks >= Sound4Frequency)
					{
						Sound4Ticks = 0;
						Sound4Bit = rand() & 1;
					}

					if (Sound4Bit)
					{
						if (FF00_C(0x25) & 0x08)
						{
							Sound4L += Volume[Sound4Volume];
						}
						if (FF00_C(0x25) & 0x80)
						{
							Sound4R += Volume[Sound4Volume];
						}
					}
					else
					{
						if (FF00_C(0x25) & 0x08)
						{
							Sound4L -= Volume[Sound4Volume];
						}
						if (FF00_C(0x25) & 0x80)
						{
							Sound4R -= Volume[Sound4Volume];
						}
					}
				}
			}
		}
		SoundTicks -= Ticks;//*/
		//__asm int 3
/*
004069CA 8B 7D FC             mov         edi,dword ptr [ebp-4]			//Ticks
004069CD 8B 86 60 39 04 00    mov         eax,dword ptr [esi+43960h]	//SoundTicks
004069D3 81 E7 FF 00 00 00    and         edi,0FFh
004069D9 3B C7                cmp         eax,edi
004069DB 89 7D EC             mov         dword ptr [ebp-14h],edi		//Ticks2
004069DE 0F 83 D2 02 00 00    jae         00406CB6
004069E4 8A C8                mov         cl,al
004069E6 2A D9                sub         bl,cl
004069E8 8B CE                mov         ecx,esi
004069EA 88 5D F8             mov         byte ptr [ebp-8],bl
004069ED E8 DE 04 00 00       call        00406ED0
004069F2 8B 86 68 39 04 00    mov         eax,dword ptr [esi+43968h]
004069F8 8B 4D F8             mov         ecx,dword ptr [ebp-8]
004069FB 85 C0                test        eax,eax
004069FD 0F 84 57 01 00 00    je          00406B5A
00406A03 84 DB                test        bl,bl
00406A05 0F 84 4F 01 00 00    je          00406B5A
00406A0B 8B 96 78 39 04 00    mov         edx,dword ptr [esi+43978h]
00406A11 33 C0                xor         eax,eax
00406A13 8A 86 71 8F 00 00    mov         al,byte ptr [esi+8F71h]
00406A19 BF 00 40 00 00       mov         edi,4000h
00406A1E C1 E2 03             shl         edx,3
00406A21 2B FA                sub         edi,edx
00406A23 8B D1                mov         edx,ecx
00406A25 C1 E8 06             shr         eax,6
00406A28 89 45 E8             mov         dword ptr [ebp-18h],eax
00406A2B 81 E2 FF 00 00 00    and         edx,0FFh
00406A31 EB 03                jmp         00406A36
00406A33 8B 45 E8             mov         eax,dword ptr [ebp-18h]
00406A36 8B 9E 70 39 04 00    mov         ebx,dword ptr [esi+43970h]
00406A3C 43                   inc         ebx
00406A3D 3B DF                cmp         ebx,edi
00406A3F 89 9E 70 39 04 00    mov         dword ptr [esi+43970h],ebx
00406A45 72 1A                jb          00406A61
00406A47 8B 9E 74 39 04 00    mov         ebx,dword ptr [esi+43974h]
00406A4D C7 86 70 39 04 00 00 mov         dword ptr [esi+43970h],0
00406A57 43                   inc         ebx
00406A58 83 E3 07             and         ebx,7
00406A5B 89 9E 74 39 04 00    mov         dword ptr [esi+43974h],ebx
00406A61 83 F8 03             cmp         eax,3
00406A64 0F 87 E3 00 00 00    ja          00406B4D
00406A6A FF 24 85 00 6E 40 00 jmp         dword ptr [eax*4+406E00h]
00406A71 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406A77 85 C0                test        eax,eax
00406A79 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406A7F 75 06                jne         00406A87
00406A81 A8 01                test        al,1
00406A83 74 77                je          00406AFC
00406A85 EB 62                jmp         00406AE9
00406A87 A8 01                test        al,1
00406A89 74 43                je          00406ACE
00406A8B EB 2E                jmp         00406ABB
00406A8D 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406A93 83 F8 01             cmp         eax,1
00406A96 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406A9C 77 E9                ja          00406A87
00406A9E EB E1                jmp         00406A81
00406AA0 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406AA6 83 F8 03             cmp         eax,3
00406AA9 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406AAF 77 06                ja          00406AB7
00406AB1 A8 01                test        al,1
00406AB3 74 47                je          00406AFC
00406AB5 EB 32                jmp         00406AE9
00406AB7 A8 01                test        al,1
00406AB9 74 13                je          00406ACE
00406ABB 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406AC1 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406AC8 29 9E 8C 39 04 00    sub         dword ptr [esi+4398Ch],ebx
00406ACE A8 10                test        al,10h
00406AD0 74 7B                je          00406B4D
00406AD2 EB 5E                jmp         00406B32
00406AD4 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406ADA 83 F8 05             cmp         eax,5
00406ADD 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406AE3 77 32                ja          00406B17
00406AE5 A8 01                test        al,1
00406AE7 74 13                je          00406AFC
00406AE9 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406AEF 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406AF6 01 9E 8C 39 04 00    add         dword ptr [esi+4398Ch],ebx
00406AFC A8 10                test        al,10h
00406AFE 74 4D                je          00406B4D
00406B00 8B 86 7C 39 04 00    mov         eax,dword ptr [esi+4397Ch]
00406B06 8B 9E 90 39 04 00    mov         ebx,dword ptr [esi+43990h]
00406B0C 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406B13 03 D8                add         ebx,eax
00406B15 EB 30                jmp         00406B47
00406B17 A8 01                test        al,1
00406B19 74 13                je          00406B2E
00406B1B 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406B21 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406B28 29 9E 8C 39 04 00    sub         dword ptr [esi+4398Ch],ebx
00406B2E A8 10                test        al,10h
00406B30 74 1B                je          00406B4D
00406B32 8B 86 7C 39 04 00    mov         eax,dword ptr [esi+4397Ch]
00406B38 8B 9E 90 39 04 00    mov         ebx,dword ptr [esi+43990h]
00406B3E 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406B45 2B D8                sub         ebx,eax
00406B47 89 9E 90 39 04 00    mov         dword ptr [esi+43990h],ebx
00406B4D 4A                   dec         edx
00406B4E 0F 85 DF FE FF FF    jne         00406A33
00406B54 8B 7D EC             mov         edi,dword ptr [ebp-14h]
00406B57 8A 5D F8             mov         bl,byte ptr [ebp-8]
00406B5A 8B 86 6C 39 04 00    mov         eax,dword ptr [esi+4396Ch]
00406B60 85 C0                test        eax,eax
00406B62 0F 84 4E 01 00 00    je          00406CB6
00406B68 84 DB                test        bl,bl
00406B6A 0F 84 46 01 00 00    je          00406CB6
00406B70 8B 86 9C 39 04 00    mov         eax,dword ptr [esi+4399Ch]
00406B76 BA 00 40 00 00       mov         edx,4000h
00406B7B C1 E0 03             shl         eax,3
00406B7E 2B D0                sub         edx,eax
00406B80 33 C0                xor         eax,eax
00406B82 8A 86 76 8F 00 00    mov         al,byte ptr [esi+8F76h]
00406B88 8B F8                mov         edi,eax
00406B8A C1 EF 06             shr         edi,6
00406B8D 81 E1 FF 00 00 00    and         ecx,0FFh
00406B93 8B 9E 94 39 04 00    mov         ebx,dword ptr [esi+43994h]
00406B99 43                   inc         ebx
00406B9A 8B C3                mov         eax,ebx
00406B9C 89 9E 94 39 04 00    mov         dword ptr [esi+43994h],ebx
00406BA2 3B C2                cmp         eax,edx
00406BA4 72 1A                jb          00406BC0
00406BA6 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BAC C7 86 94 39 04 00 00 mov         dword ptr [esi+43994h],0
00406BB6 40                   inc         eax
00406BB7 83 E0 07             and         eax,7
00406BBA 89 86 98 39 04 00    mov         dword ptr [esi+43998h],eax
00406BC0 83 FF 03             cmp         edi,3
00406BC3 0F 87 E3 00 00 00    ja          00406CAC
00406BC9 FF 24 BD 10 6E 40 00 jmp         dword ptr [edi*4+406E10h]
00406BD0 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BD6 85 C0                test        eax,eax
00406BD8 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406BDE 75 06                jne         00406BE6
00406BE0 A8 02                test        al,2
00406BE2 74 77                je          00406C5B
00406BE4 EB 62                jmp         00406C48
00406BE6 A8 02                test        al,2
00406BE8 74 43                je          00406C2D
00406BEA EB 2E                jmp         00406C1A
00406BEC 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BF2 83 F8 01             cmp         eax,1
00406BF5 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406BFB 77 E9                ja          00406BE6
00406BFD EB E1                jmp         00406BE0
00406BFF 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406C05 83 F8 03             cmp         eax,3
00406C08 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406C0E 77 06                ja          00406C16
00406C10 A8 02                test        al,2
00406C12 74 47                je          00406C5B
00406C14 EB 32                jmp         00406C48
00406C16 A8 02                test        al,2
00406C18 74 13                je          00406C2D
00406C1A 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C20 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C27 29 9E AC 39 04 00    sub         dword ptr [esi+439ACh],ebx
00406C2D A8 20                test        al,20h
00406C2F 74 7B                je          00406CAC
00406C31 EB 5E                jmp         00406C91
00406C33 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406C39 83 F8 05             cmp         eax,5
00406C3C 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406C42 77 32                ja          00406C76
00406C44 A8 02                test        al,2
00406C46 74 13                je          00406C5B
00406C48 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C4E 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C55 01 9E AC 39 04 00    add         dword ptr [esi+439ACh],ebx
00406C5B A8 20                test        al,20h
00406C5D 74 4D                je          00406CAC
00406C5F 8B 86 A0 39 04 00    mov         eax,dword ptr [esi+439A0h]
00406C65 8B 9E B0 39 04 00    mov         ebx,dword ptr [esi+439B0h]
00406C6B 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406C72 03 D8                add         ebx,eax
00406C74 EB 30                jmp         00406CA6
00406C76 A8 02                test        al,2
00406C78 74 13                je          00406C8D
00406C7A 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C80 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C87 29 9E AC 39 04 00    sub         dword ptr [esi+439ACh],ebx
00406C8D A8 20                test        al,20h
00406C8F 74 1B                je          00406CAC
00406C91 8B 86 A0 39 04 00    mov         eax,dword ptr [esi+439A0h]
00406C97 8B 9E B0 39 04 00    mov         ebx,dword ptr [esi+439B0h]
00406C9D 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406CA4 2B D8                sub         ebx,eax
00406CA6 89 9E B0 39 04 00    mov         dword ptr [esi+439B0h],ebx
00406CAC 49                   dec         ecx
00406CAD 0F 85 E0 FE FF FF    jne         00406B93
00406CB3 8B 7D EC             mov         edi,dword ptr [ebp-14h]
00406CB6 29 BE 60 39 04 00    sub         dword ptr [esi+43960h],edi
*/


		__asm
		{
			xor		ebx, ebx
			mov		ecx, this
			//ecx	= this
			mov		bl, byte ptr [Ticks]
			//ebx	= Ticks


			/*/*****
			//Sound

			//ecx	= this
			//ebx	= Ticks

			push	ebx
			mov		al, byte ptr [ecx + Offset_SoundTicks]
			//al	= SoundTicks
			sub		bl, al
			//ebx	= Ticks2 (new value)
			//al	free
			jb		Sound_NotEnoughTicks
//#ifdef _DEBUG
			push	ebx
			push	ecx
//#endif //_DEBUG
			call	SoundUpdate
//#ifdef _DEBUG
			pop		ecx
			pop		ebx
//#endif //_DEBUG
			test	bl, bl
			jz		SoundDone
			mov		al, byte ptr [ecx + Offset_Sound1Enabled]
			//al	= Sound1Enabled
			test	al, al
			//al	free
			jz		Sound1Done
			mov		eax, dword ptr [ecx + Offset_Sound1Ticks]
			//eax	= Sound1Ticks
			mov		edx, dword ptr [ecx + Offset_Sound1Frequency]
			//edx	= Sound1Frequency
			shl		edx, 3
			//edx	= Sound1Frequency * 8
			neg		edx
			//edx	= -Sound1Frequency * 8
			add		edx, 2048 * 8
			//edx	= (2048 - Sound1Frequency) * 8
			cmp		edx, eax
			ja		Sound1NoStageChange

			mov		dword ptr [ecx + Offset_Sound1Ticks], eax

			mov		dh, byte ptr [ecx + Offset_Sound1Stage]
			inc		dh
			and		dh, 7
			mov		byte ptr [ecx + Offset_Sound1Stage], dh
			/*xor		edx, edx
			//edx	= 0
			mov		dl, byte ptr [ecx + Offset_Sound1Volume]
			//edx	= Sound1Volume
			mov		esi, dword ptr [Volume + edx * 4]*/
			//esi	= Volume
			//edx	free

			//l, r (edx)

			/*sub		eax, edi
			//edi	free
			jc		Sound1Done
			mov		dword ptr [ecx + Offset_Sound1Ticks], 0
			Sound1Stage = ++Sound1Stage & 7*/
/*			xor		esi, esi
			jmp		Sound1StateChanged

Sound1NoStageChange:
			//edx	free
			xor		edx, edx
			//edx	= 0
			mov		dl, byte ptr [ecx + Offset_Sound1Volume]
			//edx	= Sound1Volume
			mov		esi, dword ptr [Volume + edx * 4]
			//esi	= Volume
			//edx	free
			mov		dword ptr [ecx + Offset_Sound1Ticks], eax
			mov		dh, byte ptr [ecx + Offset_Sound1Stage]
			//dh	= Sound1Stage
			test	dh, dh
			jz		Sound1Up
			shr		dh, 1

			mov		dl, byte ptr [ecx + FF00_ASM + 0x11]
			//dl	= FF00(0x11)
			shr		dl, 6
			cmp		dh, dl
			//edx	free
			jb		Sound1Up

//Sound1Down:
			neg		esi

Sound1Up:
			//edx	free

			imul	esi, ebx
			//eax	= L, R
Sound1StateChanged:
			//mov		edx, dword ptr [ecx + Offset_Sound1L]
			//add		edx, esi
			mov		dword ptr [ecx + Offset_Sound1L], esi
Sound1Done:
			//bl	= Ticks2

			//Sound2
Sound_NotEnoughTicks:
SoundDone:
			pop		ebx
			//ebx	= Ticks
			mov		al, byte ptr [ecx + Offset_SoundTicks]
			sub		al, bl
			mov		byte ptr [ecx + Offset_SoundTicks], al
			//*/


			//*******
			//Divider

			//ecx	= this
			//ebx	= Ticks

			mov		al, byte ptr [ecx + Offset_DIV_Ticks]
			//al	= DIV_Ticks
			sub		al, bl	//DIV_Ticks -= Ticks
			//jc		DIV_Increase
//DIV_Increased:
			jnc		DIV_NotEnoughTicks
			mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
			//dl	= FF00(0x04)
			add		al, 32	//DIV_Ticks += 32
			inc		dl
			mov		byte ptr [ecx + FF00_ASM + 0x04], dl
			//dl	free
			//edx	free
DIV_NotEnoughTicks:
			mov		byte ptr [ecx + Offset_DIV_Ticks], al
			//al	free
			//eax	free

			//ecx	= this
			//ebx	= Ticks


			//*****
			//Timer

			//ecx	= this
			//ebx	= Ticks
			mov		al, byte ptr [ecx + FF00_ASM + 0x07]
			test	al, 0x04
			jz		TimerDisabled
			mov		eax, dword ptr [ecx + Offset_TIMA_Ticks]
			//eax	= TIMA_Ticks
			add		eax, ebx
			//ebx	free
			mov		edx, dword ptr [ecx + Offset_Hz]
			//edx	= Hz
			//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
			cmp		eax, edx
			jb		TIMA_NotEnoughTicks

			sub		eax, edx
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
			mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
			//eax	free

TimerDisabled:
			//ecx	= this


			//**********
			//Interrupts

			//ecx	= this

			mov		edx, dword ptr [ecx + Offset_Flags]
			//edx	= Flags
			test	edx, GB_IE
			jz		NoInterrupt
			mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
			//ah	= FF00(0x0F)
			mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
			and		al, ah
			//al	= FF00(0xFF) & FF00(0x0F)
			test	al, 0x0F
			jz		InterruptServiced

			and		edx, ~(GB_IE | GB_ENABLEIE | GB_HALT)
			mov		dword ptr [ecx + Offset_Flags], edx
			//edx	free


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

			//eax	free
			//edx	free

			jmp		InterruptServiced

NoInterrupt:
			//ecx	= this
			//edx	= Flags

			and		edx, GB_ENABLEIE
			jz		InterruptServiced
			shl		edx, 1
			mov		eax, dword ptr [ecx + Offset_Flags]
			//eax	= Flags
			or		edx, eax
			//eax	free
			mov		dword ptr [ecx + Offset_Flags], edx
			//edx	free

InterruptServiced:
			//ecx	= this


			/*/*********
			//Exit loop

			//ecx	= this

			mov		dl, byte ptr [ecx + Offset_Flags]
			//edx	= Flags
			test	dl, GB_EXITLOOP
			jz		ContinueLoop
			and		dl, ~GB_EXITLOOP
			mov		byte ptr [ecx + Offset_Flags], dl
			//ecx	free
			//edx	free//*/
		}
	}
	while (!(Flags & GB_EXITLOOP));

	Flags &= ~(GB_EXITLOOPDIRECTLY | GB_EXITLOOP);
	return;

/*DIV_Increase:
	__asm
	{
		mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
		//dl	= FF00(0x04)
		add		al, 32	//DIV_Ticks += 32
		inc		dl
		mov		byte ptr [ecx + FF00_ASM + 0x04], dl
		//dl	free
		//edx	free
		jmp		DIV_Increased
	}*/

/*ExitLoop:
	__asm
	{
		int		3
		mov		ecx, this
		mov		eax, dword ptr [ecx + Offset_Flags]
		and		eax, ~(GB_EXITLOOPDIRECTLY | GB_EXITLOOP)
		mov		dword ptr [ecx + Offset_Flags], eax
	}
	return;*/
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



void CGameBoy::DebugMainLoop()
{
	BYTE			Ticks, Ticks2, Ticks3;
	WORD			Sprites[10];
	MSG				msg;


	do
	{
		if (Flags & GB_HALT)
		{
			Ticks = 1;
			nCycles++;
		}
		else
		{
			//Because OpCodes functions can change eax, ebx and edx, all these registers should be
			//used within this __asm block. If not, an optimized compilation will result in strange errors.
			__asm
			{
				mov		ecx, this
				mov		edx, dword ptr [ecx + Offset_Reg_PC]

				call	RetrieveAccess
				test	al, MEM_EXECUTE
				jz		ExecuteAccessDenied

				test	al, MEM_READ
				jz		ReadAccessDenied

				call	ReadMem
				and		eax, 0xFF
				call	dword ptr [DebugOpCodes + 4 * eax]

				test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
				jnz		FastCPU
				shl		al, 1
FastCPU:
				mov		ebx, dword ptr [ecx + Offset_nCycles]
				mov		byte ptr [Ticks], al
				add		ebx, eax
				mov		dword ptr [ecx + Offset_nCycles], ebx

				mov		edx, dword ptr [ecx + Offset_Reg_PC]
				call	RetrieveAccess
				test	al, MEM_BREAKPOINT
				jz		NotBreakPoint
				or		dword ptr [ecx + Offset_Flags], GB_EXITLOOP | GB_ERROR
NotBreakPoint:
			}

			if (Flags & GB_EXITLOOPDIRECTLY)
			{
				Flags &= ~(GB_EXITLOOPDIRECTLY | GB_EXITLOOP);
				return;
			}
		}


		//LCD
		if (LCD_Ticks <= Ticks)
		{
			if (FF00_C(0x40) & 0x80) //LCD on
			{
				switch (FF00_C(0x41) & 3)
				{
				case 3: //Transfer to LCD
					LCD_Ticks += 104;

					FF00_C(0x41) &= ~0x03;		//H-Blank
					MemStatus_CPU[0x8F41] |= MEM_CHANGED;

					//HDMA
					if (Flags & GB_HDMA)
					{
						__asm
						{
							mov		ecx, this

							mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
							and		ebx, 1
							shl		ebx, 13

							mov		esi, dword ptr [ecx + FF00_ASM + 0x51]
							mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
							dec		byte ptr [ecx + FF00_ASM + 0x55]
							or		dword ptr [ecx + Offset_MemStatus_CPU + 0x8F51], MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
							or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F55], MEM_CHANGED
							rol		si, 8
							rol		di, 8
							and		esi, 0xFFF0
							and		edi, 0x1FF0

							add		edi, ebx

							mov		eax, esi
							add		eax, 0x0010
							mov		byte ptr [ecx + FF00_ASM + 0x52], al
							mov		byte ptr [ecx + FF00_ASM + 0x51], ah
							mov		eax, edi
							add		eax, 0x0010
							mov		byte ptr [ecx + FF00_ASM + 0x54], al
							mov		byte ptr [ecx + FF00_ASM + 0x53], ah

							mov		eax, esi
							shr		esi, 12
							and		eax, 0x0FFF
							mov		esi, [ecx + Offset_MEM + esi * 4]

							mov		edx, dword ptr [esi + eax]
							mov		ebx, dword ptr [ecx + Offset_MemStatus_VRAM + edi]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
							or		ebx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
							mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi], ebx
							mov		edx, dword ptr [esi + eax + 0x04]
							mov		ebx, dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x04]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
							or		ebx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
							mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x04], ebx
							mov		edx, dword ptr [esi + eax + 0x08]
							mov		ebx, dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x08]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
							or		ebx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
							mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x08], ebx
							mov		edx, dword ptr [esi + eax + 0x0C]
							mov		ebx, dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x0C]
							mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx
							or		ebx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
							mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x0C], ebx

							mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
							test	bl, 0x80
							jz		HDMA_NotFinished
							and		dword ptr [ecx + Offset_Flags], ~GB_HDMA
HDMA_NotFinished:
						}
					}
					break;

				case 1: //V-Blank
					if (!FF00_C(0x44))
					{
						LCD_Ticks += 40;
						FF00_C(0x41) = (FF00_C(0x41) & ~0x03) | 0x02;	//OAM-RAM search
						MemStatus_CPU[0x8F41] |= MEM_CHANGED;

						//++DrawLineMask &= 1;

						WindowY = FF00_C(0x4A);
						WindowTileY = 0;
						WindowTileY2 = 0;

						break;
					}

					FF00_C(0x44)++;
					MemStatus_CPU[0x8F44] |= MEM_CHANGED;
					if (FF00_C(0x44) >= 154)
					{
						FF00_C(0x44) = 0;
					}

					LCD_Ticks += 228;

					if (FF00_C(0x41) & 0x40 && FF00_C(0x44) == FF00_C(0x45))
					{
						//LYC coincidence interrupt
						FF00_C(0x0F) |= 0x02;
						MemStatus_CPU[0x8F0F] |= MEM_CHANGED;
					}
					break;

				case 0: //H-Blank
					FF00_C(0x44)++;
					MemStatus_CPU[0x8F44] |= MEM_CHANGED;

					if (FF00_C(0x44) >= 144)
					{
						LCD_Ticks += 228;

						FF00_C(0x41) = (FF00_C(0x41) & ~0x03) | 0x01;		//V-Blank
						MemStatus_CPU[0x8F41] |= MEM_CHANGED;

						//V-Blank interrupt
						FF00_C(0x0F) |= 1;
						MemStatus_CPU[0x8F0F] |= MEM_CHANGED;

						Flags |= GB_EXITLOOP;
					}
					else
					{
						LCD_Ticks += 40;

						FF00_C(0x41) = (FF00_C(0x41) & ~0x03) | 0x02;		//OAM search
						MemStatus_CPU[0x8F41] |= MEM_CHANGED;
					}
					break;

				case 2: //OAM search
					LCD_Ticks += 84;
					FF00_C(0x41) |= 0x03;			//Transfer to LCD
					MemStatus_CPU[0x8F41] |= MEM_CHANGED;

					if (FF00_C(0x41) & 0x40 && FF00_C(0x44) == FF00_C(0x45))
					{
						//LYC coincidence interrupt
						FF00_C(0x0F) |= 0x02;
						MemStatus_CPU[0x8F0F] |= MEM_CHANGED;
					}

					BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (/*DrawLineMask == (FF00_C(0x44) & 1) &&*/ FF00_C(0x44) < 144)
					{
						BYTE	WndX;
						__asm
						{
							mov		ecx, this


							mov		dword ptr [Line], 0
							mov		dword ptr [Line + 4], 0
							mov		dword ptr [Line + 8], 0
							mov		dword ptr [Line + 12], 0


							//Set pointer to bitmap
							movzx	edi, byte ptr [ecx + FF00_ASM + 0x44]
							imul	edi, (160 + 14) * 2
							add		edi, dword ptr [ecx + Offset_pGBBitmap]
							push	edi


							test	byte ptr [ecx + Offset_Flags], GB_ROM_COLOR
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
							cmp		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		Wnd_NoDraw		//LY too low

							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
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
							shr		dh, 1
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
							cmp		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		NC_Wnd_NoDraw		//LY too low

							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							cmp		bl, 143
							ja		NC_Wnd_NoDraw		//WY too high
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
						}
					}
				}
				LCD_Ticks -= Ticks;
			}
			else
			{
				LCD_Ticks = 0;
				if (FF00_C(0x44) != 0)
				{
					FF00_C(0x44) = 0;
					MemStatus_CPU[0x8F44] |= MEM_CHANGED;
				}
				if (FF00_C(0x41) & 3)
				{
					FF00_C(0x41) &= ~0x03;
					MemStatus_CPU[0x8F41] |= MEM_CHANGED;
				}

				if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE))
				{
					Flags |= GB_EXITLOOP;
				}
			}
		}
		else
		{
			LCD_Ticks -= Ticks;
		}



		if (Sound1Enabled)
		{
			if (Sound1TimeOut)
			{
				if (Sound1TimeOut <= Ticks)
				{
					Sound1Enabled = false;
					FF00_C(0x26) &= ~0x01;
					MemStatus_CPU[0x8F26] |= MEM_CHANGED;
				}
				else
				{
					Sound1TimeOut -= Ticks;
				}
			}
			if (Sound1Sweep)
			{
				if (Sound1Sweep <= Ticks)
				{
					Sound1Sweep = 0;

					if (FF00_C(0x10) & 0x08)
					{
						if (Sound1Frequency >= (Sound1Frequency >> (FF00_C(0x10) & 0x07)))
						{
							Sound1Frequency -= (Sound1Frequency >> (FF00_C(0x10) & 0x07));
						}
						else
						{
							Sound1Frequency = 0;
						}
					}
					else
					{
						Sound1Frequency += (Sound1Frequency >> (FF00_C(0x10) & 0x07));
						if (Sound1Frequency > 2047)
						{
							Sound1Enabled = false;
							FF00_C(0x26) &= ~0x01;
							MemStatus_CPU[0x8F26] |= MEM_CHANGED;
							Sound1Frequency = 2047;
						}
					}
					FF00_C(0x13) = (BYTE)((~Sound1Frequency) & 0xFF);
					MemStatus_CPU[0x8F13] |= MEM_CHANGED;
					FF00_C(0x14) = (BYTE)(((~Sound1Frequency) & 0x0700) >> 8);
					MemStatus_CPU[0x8F14] |= MEM_CHANGED;

					Sound1Sweep = 8192 * ((FF00_C(0x10) & 0x70) >> 4);
				}
				else
				{
					Sound1Sweep -= Ticks;
				}
			}
			if (Sound1Envelope)
			{
				if (Sound1Envelope <= Ticks)
				{
					if (FF00_C(0x12) & 0x08)
					{
						if ((FF00_C(0x12) & 0xF0) == 0xF0)
						{
							Sound1Enabled = false;
							FF00_C(0x26) &= ~0x01;
							MemStatus_CPU[0x8F26] |= MEM_CHANGED;
						}
						else
						{
							FF00_C(0x12) += 0x10;
							Sound1Volume = FF00_C(0x12) >> 4;
							Sound1Envelope = ((DWORD)FF00_C(0x12) & 0x07) * 16384;
							MemStatus_CPU[0x8F12] |= MEM_CHANGED;
						}
					}
					else
					{
						if (FF00_C(0x12) & 0xF0)
						{
							FF00_C(0x12) -= 0x10;
							Sound1Volume = FF00_C(0x12) >> 4;
							MemStatus_CPU[0x8F12] |= MEM_CHANGED;
						}
						Sound1Envelope = ((DWORD)FF00_C(0x12) & 0x07) * 16384;
					}
				}
				else
				{
					Sound1Envelope -= Ticks;
				}
			}


			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound1Ticks++;
				if (Sound1Ticks >= ((2048 - Sound1Frequency) << 3))
				{
					Sound1Ticks = 0;
					Sound1Stage = ++Sound1Stage & 7;
				}

				switch (FF00_C(0x11) >> 6)
				{
				case 0:
					if (Sound1Stage == 0)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 1:
					if (Sound1Stage <= 1)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 2:
					if (Sound1Stage <= 3)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				case 3:
					if (Sound1Stage <= 5)
					{
						if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
						if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
					}
					break;
				}
			}
		}

		if (Sound2Enabled)
		{
			if (Sound2TimeOut)
			{
				if (Sound2TimeOut <= Ticks)
				{
					Sound2Enabled = false;
					FF00_C(0x26) &= ~0x02;
					MemStatus_CPU[0x8F26] |= MEM_CHANGED;
				}
				else
				{
					Sound2TimeOut -= Ticks;
				}
			}
			if (Sound2Envelope)
			{
				if (Sound2Envelope <= Ticks)
				{
					if (FF00_C(0x17) & 0x08)
					{
						if ((FF00_C(0x17) & 0xF0) == 0xF0)
						{
							Sound2Enabled = false;
							FF00_C(0x26) &= ~0x02;
							MemStatus_CPU[0x8F26] |= MEM_CHANGED;
						}
						else
						{
							FF00_C(0x17) += 0x10;
							Sound2Volume = FF00_C(0x17) >> 4;
							Sound2Envelope = ((DWORD)FF00_C(0x17) & 0x07) * 16384;
							MemStatus_CPU[0x8F17] |= MEM_CHANGED;
						}
					}
					else
					{
						if (FF00_C(0x17) & 0xF0)
						{
							FF00_C(0x17) -= 0x10;
							Sound2Volume = FF00_C(0x17) >> 4;
							MemStatus_CPU[0x8F17] |= MEM_CHANGED;
						}
						Sound2Envelope = ((DWORD)FF00_C(0x17) & 0x07) * 16384;
					}
				}
				else
				{
					Sound2Envelope -= Ticks;
				}
			}


			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound2Ticks++;
				if (Sound2Ticks >= ((2048 - Sound2Frequency) << 3))
				{
					Sound2Ticks = 0;
					Sound2Stage = ++Sound2Stage & 7;
				}

				switch (FF00_C(0x16) >> 6)
				{
				case 0:
					if (Sound2Stage == 0)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 1:
					if (Sound2Stage <= 1)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 2:
					if (Sound2Stage <= 3)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				case 3:
					if (Sound2Stage <= 5)
					{
						if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
					}
					else
					{
						if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
						if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
					}
					break;
				}
			}
		}

		if (Sound3Enabled)
		{
			if (Sound3TimeOut)
			{
				if (Sound3TimeOut <= Ticks)
				{
					Sound3Enabled = false;
					FF00_C(0x1A) &= ~0x80;
					MemStatus_CPU[0x8F1A] |= MEM_CHANGED;
					FF00_C(0x26) &= ~0x04;
					MemStatus_CPU[0x8F26] |= MEM_CHANGED;
				}
			}

			Sound3TimeOut -= Ticks;

			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound3Ticks++;
				if (Sound3Ticks >= 2048 - Sound3Frequency)
				{
					Sound3Ticks = 0;
					Sound3Stage = ++Sound3Stage & 31;
				}

				BYTE	SoundBit;

				if (Sound3Stage & 1)
				{
					SoundBit = FF00_C(0x30 + (Sound3Stage >> 1)) & 0x0F;
				}
				else
				{
					SoundBit = FF00_C(0x30 + (Sound3Stage >> 1)) >> 4;
				}
				switch (FF00_C(0x1C) & 0x60)
				{
				case 0x00:
					SoundBit = 8;
					break;

				case 0x20:
					break;

				case 0x40:
					SoundBit = 8 | (SoundBit >> 1);
					break;

				case 0x60:
					SoundBit = 0xC | (SoundBit >> 2);
					break;
				}

				if (FF00_C(0x25) & 0x04)
				{
					Sound3L += (SoundBit << 4) - 0x80;
				}
				if (FF00_C(0x25) & 0x40)
				{
					Sound3R += (SoundBit << 4) - 0x80;
				}
			}
		}

		if (Sound4Enabled)
		{
			if (Sound4TimeOut)
			{
				if (Sound4TimeOut <= Ticks)
				{
					Sound4Enabled = false;
					FF00_C(0x26) &= ~0x08;
					MemStatus_CPU[0x8F26] |= MEM_CHANGED;
				}
				else
				{
					Sound4TimeOut -= Ticks;
				}
			}
			if (Sound4Envelope)
			{
				if (Sound4Envelope <= Ticks)
				{
					if (FF00_C(0x21) & 0x08)
					{
						if ((FF00_C(0x21) & 0xF0) == 0xF0)
						{
							Sound4Enabled = false;
							FF00_C(0x26) &= ~0x08;
							MemStatus_CPU[0x8F26] |= MEM_CHANGED;
						}
						else
						{
							FF00_C(0x21) += 0x10;
							Sound4Volume = FF00_C(0x21) >> 4;
							Sound4Envelope = ((DWORD)FF00_C(0x21) & 0x07) * 16384;
						}
					}
					else
					{
						if (FF00_C(0x21) & 0xF0)
						{
							FF00_C(0x21) -= 0x10;
							MemStatus_CPU[0x8F21] |= MEM_CHANGED;
							Sound4Volume = FF00_C(0x21) >> 4;
						}
						Sound4Envelope = ((DWORD)FF00_C(0x21) & 0x07) * 16384;
					}
				}
				else
				{
					Sound4Envelope -= Ticks;
				}
			}

			if (SoundTicks >= Ticks)
			{
				Ticks2 = Ticks;
			}
			else
			{
				Ticks2 = (BYTE)SoundTicks;
			}
			while (Ticks2 != 0)
			{
				Ticks2--;
				Sound4Ticks++;
				if (Sound4Ticks >= Sound4Frequency)
				{
					Sound4Ticks = 0;
					Sound4Bit = rand() & 1;
				}

				if (Sound4Bit)
				{
					if (FF00_C(0x25) & 0x08)
					{
						Sound4L += Volume[Sound4Volume];
					}
					if (FF00_C(0x25) & 0x80)
					{
						Sound4R += Volume[Sound4Volume];
					}
				}
				else
				{
					if (FF00_C(0x25) & 0x08)
					{
						Sound4L -= Volume[Sound4Volume];
					}
					if (FF00_C(0x25) & 0x80)
					{
						Sound4R -= Volume[Sound4Volume];
					}
				}
			}
		}

		//__asm int 3
		if (SoundTicks < Ticks)
		{
			Ticks2 = Ticks - (BYTE)SoundTicks;
			SoundUpdate(this);

			if (Sound1Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound1Ticks++;
					if (Sound1Ticks >= ((2048 - Sound1Frequency) << 3))
					{
						Sound1Ticks = 0;
						Sound1Stage = ++Sound1Stage & 7;
					}


					switch (FF00_C(0x11) >> 6)
					{
					case 0:
						if (Sound1Stage == 0)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 1:
						if (Sound1Stage <= 1)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 2:
						if (Sound1Stage <= 3)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					case 3:
						if (Sound1Stage <= 5)
						{
							if (FF00_C(0x25) & 0x01) Sound1L += Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R += Volume[Sound1Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x01) Sound1L -= Volume[Sound1Volume];
							if (FF00_C(0x25) & 0x10) Sound1R -= Volume[Sound1Volume];
						}
						break;
					}
				}
			}
			if (Sound2Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound2Ticks++;
					if (Sound2Ticks >= ((2048 - Sound2Frequency) << 3))
					{
						Sound2Ticks = 0;
						Sound2Stage = ++Sound2Stage & 7;
					}

					switch (FF00_C(0x16) >> 6)
					{
					case 0:
						if (Sound2Stage == 0)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 1:
						if (Sound2Stage <= 1)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 2:
						if (Sound2Stage <= 3)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					case 3:
						if (Sound2Stage <= 5)
						{
							if (FF00_C(0x25) & 0x02) Sound2L += Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R += Volume[Sound2Volume];
						}
						else
						{
							if (FF00_C(0x25) & 0x02) Sound2L -= Volume[Sound2Volume];
							if (FF00_C(0x25) & 0x20) Sound2R -= Volume[Sound2Volume];
						}
						break;
					}
				}
			}

			if (Sound3Enabled)
			{
				Ticks3 = Ticks2;
				while (Ticks3 != 0)
				{
					Ticks3--;
					Sound3Ticks++;
					if (Sound3Ticks >= 2048 - Sound3Frequency)
					{
						Sound3Ticks = 0;
						Sound3Stage = ++Sound3Stage & 31;
					}

					BYTE	Sound3Bit;

					if (Sound3Stage & 1)
					{
						Sound3Bit = FF00_C(0x30 + (Sound3Stage >> 1)) & 0x0F;
					}
					else
					{
						Sound3Bit = FF00_C(0x30 + (Sound3Stage >> 1)) >> 4;
					}
					switch (FF00_C(0x1C) & 0x60)
					{
					case 0x00:
						Sound3Bit = 8;
						break;

					case 0x20:
						break;

					case 0x40:
						Sound3Bit = 8 | (Sound3Bit >> 1);
						break;

					case 0x60:
						Sound3Bit = 0xC | (Sound3Bit >> 2);
						break;
					}

					if (FF00_C(0x25) & 0x04)
					{
						Sound3L += (Sound3Bit << 4) - 0x80;
					}
					if (FF00_C(0x25) & 0x40)
					{
						Sound3R += (Sound3Bit << 4) - 0x80;
					}
				}
			}

			if (Sound4Enabled)
			{
				while (Ticks2 != 0)
				{
					Ticks2--;
					Sound4Ticks++;
					if (Sound4Ticks >= Sound4Frequency)
					{
						Sound4Ticks = 0;
						Sound4Bit = rand() & 1;
					}

					if (Sound4Bit)
					{
						if (FF00_C(0x25) & 0x08)
						{
							Sound4L += Volume[Sound4Volume];
						}
						if (FF00_C(0x25) & 0x80)
						{
							Sound4R += Volume[Sound4Volume];
						}
					}
					else
					{
						if (FF00_C(0x25) & 0x08)
						{
							Sound4L -= Volume[Sound4Volume];
						}
						if (FF00_C(0x25) & 0x80)
						{
							Sound4R -= Volume[Sound4Volume];
						}
					}
				}
			}
		}
		SoundTicks -= Ticks;//*/
		//__asm int 3
/*
004069CA 8B 7D FC             mov         edi,dword ptr [ebp-4]			//Ticks
004069CD 8B 86 60 39 04 00    mov         eax,dword ptr [esi+43960h]	//SoundTicks
004069D3 81 E7 FF 00 00 00    and         edi,0FFh
004069D9 3B C7                cmp         eax,edi
004069DB 89 7D EC             mov         dword ptr [ebp-14h],edi		//Ticks2
004069DE 0F 83 D2 02 00 00    jae         00406CB6
004069E4 8A C8                mov         cl,al
004069E6 2A D9                sub         bl,cl
004069E8 8B CE                mov         ecx,esi
004069EA 88 5D F8             mov         byte ptr [ebp-8],bl
004069ED E8 DE 04 00 00       call        00406ED0
004069F2 8B 86 68 39 04 00    mov         eax,dword ptr [esi+43968h]
004069F8 8B 4D F8             mov         ecx,dword ptr [ebp-8]
004069FB 85 C0                test        eax,eax
004069FD 0F 84 57 01 00 00    je          00406B5A
00406A03 84 DB                test        bl,bl
00406A05 0F 84 4F 01 00 00    je          00406B5A
00406A0B 8B 96 78 39 04 00    mov         edx,dword ptr [esi+43978h]
00406A11 33 C0                xor         eax,eax
00406A13 8A 86 71 8F 00 00    mov         al,byte ptr [esi+8F71h]
00406A19 BF 00 40 00 00       mov         edi,4000h
00406A1E C1 E2 03             shl         edx,3
00406A21 2B FA                sub         edi,edx
00406A23 8B D1                mov         edx,ecx
00406A25 C1 E8 06             shr         eax,6
00406A28 89 45 E8             mov         dword ptr [ebp-18h],eax
00406A2B 81 E2 FF 00 00 00    and         edx,0FFh
00406A31 EB 03                jmp         00406A36
00406A33 8B 45 E8             mov         eax,dword ptr [ebp-18h]
00406A36 8B 9E 70 39 04 00    mov         ebx,dword ptr [esi+43970h]
00406A3C 43                   inc         ebx
00406A3D 3B DF                cmp         ebx,edi
00406A3F 89 9E 70 39 04 00    mov         dword ptr [esi+43970h],ebx
00406A45 72 1A                jb          00406A61
00406A47 8B 9E 74 39 04 00    mov         ebx,dword ptr [esi+43974h]
00406A4D C7 86 70 39 04 00 00 mov         dword ptr [esi+43970h],0
00406A57 43                   inc         ebx
00406A58 83 E3 07             and         ebx,7
00406A5B 89 9E 74 39 04 00    mov         dword ptr [esi+43974h],ebx
00406A61 83 F8 03             cmp         eax,3
00406A64 0F 87 E3 00 00 00    ja          00406B4D
00406A6A FF 24 85 00 6E 40 00 jmp         dword ptr [eax*4+406E00h]
00406A71 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406A77 85 C0                test        eax,eax
00406A79 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406A7F 75 06                jne         00406A87
00406A81 A8 01                test        al,1
00406A83 74 77                je          00406AFC
00406A85 EB 62                jmp         00406AE9
00406A87 A8 01                test        al,1
00406A89 74 43                je          00406ACE
00406A8B EB 2E                jmp         00406ABB
00406A8D 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406A93 83 F8 01             cmp         eax,1
00406A96 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406A9C 77 E9                ja          00406A87
00406A9E EB E1                jmp         00406A81
00406AA0 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406AA6 83 F8 03             cmp         eax,3
00406AA9 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406AAF 77 06                ja          00406AB7
00406AB1 A8 01                test        al,1
00406AB3 74 47                je          00406AFC
00406AB5 EB 32                jmp         00406AE9
00406AB7 A8 01                test        al,1
00406AB9 74 13                je          00406ACE
00406ABB 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406AC1 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406AC8 29 9E 8C 39 04 00    sub         dword ptr [esi+4398Ch],ebx
00406ACE A8 10                test        al,10h
00406AD0 74 7B                je          00406B4D
00406AD2 EB 5E                jmp         00406B32
00406AD4 8B 86 74 39 04 00    mov         eax,dword ptr [esi+43974h]
00406ADA 83 F8 05             cmp         eax,5
00406ADD 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406AE3 77 32                ja          00406B17
00406AE5 A8 01                test        al,1
00406AE7 74 13                je          00406AFC
00406AE9 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406AEF 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406AF6 01 9E 8C 39 04 00    add         dword ptr [esi+4398Ch],ebx
00406AFC A8 10                test        al,10h
00406AFE 74 4D                je          00406B4D
00406B00 8B 86 7C 39 04 00    mov         eax,dword ptr [esi+4397Ch]
00406B06 8B 9E 90 39 04 00    mov         ebx,dword ptr [esi+43990h]
00406B0C 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406B13 03 D8                add         ebx,eax
00406B15 EB 30                jmp         00406B47
00406B17 A8 01                test        al,1
00406B19 74 13                je          00406B2E
00406B1B 8B 9E 7C 39 04 00    mov         ebx,dword ptr [esi+4397Ch]
00406B21 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406B28 29 9E 8C 39 04 00    sub         dword ptr [esi+4398Ch],ebx
00406B2E A8 10                test        al,10h
00406B30 74 1B                je          00406B4D
00406B32 8B 86 7C 39 04 00    mov         eax,dword ptr [esi+4397Ch]
00406B38 8B 9E 90 39 04 00    mov         ebx,dword ptr [esi+43990h]
00406B3E 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406B45 2B D8                sub         ebx,eax
00406B47 89 9E 90 39 04 00    mov         dword ptr [esi+43990h],ebx
00406B4D 4A                   dec         edx
00406B4E 0F 85 DF FE FF FF    jne         00406A33
00406B54 8B 7D EC             mov         edi,dword ptr [ebp-14h]
00406B57 8A 5D F8             mov         bl,byte ptr [ebp-8]
00406B5A 8B 86 6C 39 04 00    mov         eax,dword ptr [esi+4396Ch]
00406B60 85 C0                test        eax,eax
00406B62 0F 84 4E 01 00 00    je          00406CB6
00406B68 84 DB                test        bl,bl
00406B6A 0F 84 46 01 00 00    je          00406CB6
00406B70 8B 86 9C 39 04 00    mov         eax,dword ptr [esi+4399Ch]
00406B76 BA 00 40 00 00       mov         edx,4000h
00406B7B C1 E0 03             shl         eax,3
00406B7E 2B D0                sub         edx,eax
00406B80 33 C0                xor         eax,eax
00406B82 8A 86 76 8F 00 00    mov         al,byte ptr [esi+8F76h]
00406B88 8B F8                mov         edi,eax
00406B8A C1 EF 06             shr         edi,6
00406B8D 81 E1 FF 00 00 00    and         ecx,0FFh
00406B93 8B 9E 94 39 04 00    mov         ebx,dword ptr [esi+43994h]
00406B99 43                   inc         ebx
00406B9A 8B C3                mov         eax,ebx
00406B9C 89 9E 94 39 04 00    mov         dword ptr [esi+43994h],ebx
00406BA2 3B C2                cmp         eax,edx
00406BA4 72 1A                jb          00406BC0
00406BA6 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BAC C7 86 94 39 04 00 00 mov         dword ptr [esi+43994h],0
00406BB6 40                   inc         eax
00406BB7 83 E0 07             and         eax,7
00406BBA 89 86 98 39 04 00    mov         dword ptr [esi+43998h],eax
00406BC0 83 FF 03             cmp         edi,3
00406BC3 0F 87 E3 00 00 00    ja          00406CAC
00406BC9 FF 24 BD 10 6E 40 00 jmp         dword ptr [edi*4+406E10h]
00406BD0 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BD6 85 C0                test        eax,eax
00406BD8 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406BDE 75 06                jne         00406BE6
00406BE0 A8 02                test        al,2
00406BE2 74 77                je          00406C5B
00406BE4 EB 62                jmp         00406C48
00406BE6 A8 02                test        al,2
00406BE8 74 43                je          00406C2D
00406BEA EB 2E                jmp         00406C1A
00406BEC 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406BF2 83 F8 01             cmp         eax,1
00406BF5 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406BFB 77 E9                ja          00406BE6
00406BFD EB E1                jmp         00406BE0
00406BFF 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406C05 83 F8 03             cmp         eax,3
00406C08 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406C0E 77 06                ja          00406C16
00406C10 A8 02                test        al,2
00406C12 74 47                je          00406C5B
00406C14 EB 32                jmp         00406C48
00406C16 A8 02                test        al,2
00406C18 74 13                je          00406C2D
00406C1A 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C20 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C27 29 9E AC 39 04 00    sub         dword ptr [esi+439ACh],ebx
00406C2D A8 20                test        al,20h
00406C2F 74 7B                je          00406CAC
00406C31 EB 5E                jmp         00406C91
00406C33 8B 86 98 39 04 00    mov         eax,dword ptr [esi+43998h]
00406C39 83 F8 05             cmp         eax,5
00406C3C 8A 86 85 8F 00 00    mov         al,byte ptr [esi+8F85h]
00406C42 77 32                ja          00406C76
00406C44 A8 02                test        al,2
00406C46 74 13                je          00406C5B
00406C48 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C4E 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C55 01 9E AC 39 04 00    add         dword ptr [esi+439ACh],ebx
00406C5B A8 20                test        al,20h
00406C5D 74 4D                je          00406CAC
00406C5F 8B 86 A0 39 04 00    mov         eax,dword ptr [esi+439A0h]
00406C65 8B 9E B0 39 04 00    mov         ebx,dword ptr [esi+439B0h]
00406C6B 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406C72 03 D8                add         ebx,eax
00406C74 EB 30                jmp         00406CA6
00406C76 A8 02                test        al,2
00406C78 74 13                je          00406C8D
00406C7A 8B 9E A0 39 04 00    mov         ebx,dword ptr [esi+439A0h]
00406C80 8B 1C 9D 60 22 41 00 mov         ebx,dword ptr [ebx*4+412260h]
00406C87 29 9E AC 39 04 00    sub         dword ptr [esi+439ACh],ebx
00406C8D A8 20                test        al,20h
00406C8F 74 1B                je          00406CAC
00406C91 8B 86 A0 39 04 00    mov         eax,dword ptr [esi+439A0h]
00406C97 8B 9E B0 39 04 00    mov         ebx,dword ptr [esi+439B0h]
00406C9D 8B 04 85 60 22 41 00 mov         eax,dword ptr [eax*4+412260h]
00406CA4 2B D8                sub         ebx,eax
00406CA6 89 9E B0 39 04 00    mov         dword ptr [esi+439B0h],ebx
00406CAC 49                   dec         ecx
00406CAD 0F 85 E0 FE FF FF    jne         00406B93
00406CB3 8B 7D EC             mov         edi,dword ptr [ebp-14h]
00406CB6 29 BE 60 39 04 00    sub         dword ptr [esi+43960h],edi
*/


		__asm
		{
			xor		ebx, ebx
			mov		ecx, this
			//ecx	= this
			mov		bl, byte ptr [Ticks]
			//ebx	= Ticks


			/*/*****
			//Sound

			//ecx	= this
			//ebx	= Ticks

			push	ebx
			mov		al, byte ptr [ecx + Offset_SoundTicks]
			//al	= SoundTicks
			sub		bl, al
			//ebx	= Ticks2 (new value)
			//al	free
			jb		Sound_NotEnoughTicks
//#ifdef _DEBUG
			push	ebx
			push	ecx
//#endif //_DEBUG
			call	SoundUpdate
//#ifdef _DEBUG
			pop		ecx
			pop		ebx
//#endif //_DEBUG
			test	bl, bl
			jz		SoundDone
			mov		al, byte ptr [ecx + Offset_Sound1Enabled]
			//al	= Sound1Enabled
			test	al, al
			//al	free
			jz		Sound1Done
			mov		eax, dword ptr [ecx + Offset_Sound1Ticks]
			//eax	= Sound1Ticks
			mov		edx, dword ptr [ecx + Offset_Sound1Frequency]
			//edx	= Sound1Frequency
			shl		edx, 3
			//edx	= Sound1Frequency * 8
			neg		edx
			//edx	= -Sound1Frequency * 8
			add		edx, 2048 * 8
			//edx	= (2048 - Sound1Frequency) * 8
			cmp		edx, eax
			ja		Sound1NoStageChange

			mov		dword ptr [ecx + Offset_Sound1Ticks], eax

			mov		dh, byte ptr [ecx + Offset_Sound1Stage]
			inc		dh
			and		dh, 7
			mov		byte ptr [ecx + Offset_Sound1Stage], dh
			/*xor		edx, edx
			//edx	= 0
			mov		dl, byte ptr [ecx + Offset_Sound1Volume]
			//edx	= Sound1Volume
			mov		esi, dword ptr [Volume + edx * 4]*/
			//esi	= Volume
			//edx	free

			//l, r (edx)

			/*sub		eax, edi
			//edi	free
			jc		Sound1Done
			mov		dword ptr [ecx + Offset_Sound1Ticks], 0
			Sound1Stage = ++Sound1Stage & 7*/
/*			xor		esi, esi
			jmp		Sound1StateChanged

Sound1NoStageChange:
			//edx	free
			xor		edx, edx
			//edx	= 0
			mov		dl, byte ptr [ecx + Offset_Sound1Volume]
			//edx	= Sound1Volume
			mov		esi, dword ptr [Volume + edx * 4]
			//esi	= Volume
			//edx	free
			mov		dword ptr [ecx + Offset_Sound1Ticks], eax
			mov		dh, byte ptr [ecx + Offset_Sound1Stage]
			//dh	= Sound1Stage
			test	dh, dh
			jz		Sound1Up
			shr		dh, 1

			mov		dl, byte ptr [ecx + FF00_ASM + 0x11]
			//dl	= FF00(0x11)
			shr		dl, 6
			cmp		dh, dl
			//edx	free
			jb		Sound1Up

//Sound1Down:
			neg		esi

Sound1Up:
			//edx	free

			imul	esi, ebx
			//eax	= L, R
Sound1StateChanged:
			//mov		edx, dword ptr [ecx + Offset_Sound1L]
			//add		edx, esi
			mov		dword ptr [ecx + Offset_Sound1L], esi
Sound1Done:
			//bl	= Ticks2

			//Sound2
Sound_NotEnoughTicks:
SoundDone:
			pop		ebx
			//ebx	= Ticks
			mov		al, byte ptr [ecx + Offset_SoundTicks]
			sub		al, bl
			mov		byte ptr [ecx + Offset_SoundTicks], al
			//*/


			//*******
			//Divider

			//ecx	= this
			//ebx	= Ticks

			mov		al, byte ptr [ecx + Offset_DIV_Ticks]
			//al	= DIV_Ticks
			sub		al, bl	//DIV_Ticks -= Ticks
			jnc		DIV_NotEnoughTicks
			mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
			//dl	= FF00(0x04)
			add		al, 32	//DIV_Ticks += 32
			inc		dl
			mov		byte ptr [ecx + FF00_ASM + 0x04], dl
			//dl	free
			//edx	free
			or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F04], MEM_CHANGED
DIV_NotEnoughTicks:
			mov		byte ptr [ecx + Offset_DIV_Ticks], al
			//al	free
			//eax	free

			//ecx	= this
			//ebx	= Ticks


			//*****
			//Timer

			//ecx	= this
			//ebx	= Ticks
			mov		al, byte ptr [ecx + FF00_ASM + 0x07]
			test	al, 0x04
			jz		TimerDisabled
			mov		eax, dword ptr [ecx + Offset_TIMA_Ticks]
			//eax	= TIMA_Ticks
			add		eax, ebx
			//ebx	free
			mov		edx, dword ptr [ecx + Offset_Hz]
			//edx	= Hz
			//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
			cmp		eax, edx
			jb		TIMA_NotEnoughTicks

			sub		eax, edx
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
			or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
TIMA_NotZero:
			mov		byte ptr [ecx + FF00_ASM + 0x05], dl
			or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F05], MEM_CHANGED
			//dl	free
			//edx	free

TIMA_NotEnoughTicks:
			mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
			//eax	free

TimerDisabled:
			//ecx	= this


			//**********
			//Interrupts

			//ecx	= this

			mov		edx, dword ptr [ecx + Offset_Flags]
			//edx	= Flags
			test	edx, GB_IE
			jz		NoInterrupt
			mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
			//ah	= FF00(0x0F)
			mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
			and		al, ah
			//al	= FF00(0xFF) & FF00(0x0F)
			test	al, 0x0F
			jz		InterruptServiced

			and		edx, ~(GB_IE | GB_ENABLEIE | GB_HALT)
			mov		dword ptr [ecx + Offset_Flags], edx
			//edx	free


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

			//eax	free
			//edx	free

			jmp		InterruptServiced

NoInterrupt:
			//ecx	= this
			//edx	= Flags

			and		edx, GB_ENABLEIE
			jz		InterruptServiced
			shl		edx, 1
			mov		eax, dword ptr [ecx + Offset_Flags]
			//eax	= Flags
			or		edx, eax
			//eax	free
			mov		dword ptr [ecx + Offset_Flags], edx
			//edx	free

InterruptServiced:
			//ecx	= this


			/*/*********
			//Exit loop

			//ecx	= this

			mov		dl, byte ptr [ecx + Offset_Flags]
			//edx	= Flags
			test	dl, GB_EXITLOOP
			jz		ContinueLoop
			and		dl, ~GB_EXITLOOP
			mov		byte ptr [ecx + Offset_Flags], dl
			//ecx	free
			//edx	free//*/
		}
	}
	while (!(Flags & GB_EXITLOOP));

	Flags &= ~GB_EXITLOOP;
	return;

/*BreakPoint:
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	if (hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
	}
	Flags |= GB_ERROR;
	return;*/

ExecuteAccessDenied:
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Execute access needed.", "Game Lad", MB_ICONWARNING | MB_OK);
	if (hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
	}
	Flags |= GB_ERROR;
	return;

ReadAccessDenied:
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Read access needed.", "Game Lad", MB_ICONWARNING | MB_OK);
	if (hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
	}
	Flags |= GB_ERROR;
	return;

Interrupt_WriteAccessDenied:
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Write access needed for stack pointer, interrupt cannot be serviced.", "Game Lad", MB_ICONWARNING | MB_OK);
	if (hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
	}
	Flags |= GB_ERROR;
	return;
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
	InvalidateRect(hGBWnd, NULL, false);
	PostMessage(hGBWnd, WM_PAINT, 0, 0);
}



/*void CALLBACK WaveCallback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if(uMsg == WOM_DONE)
	{
		EnterCriticalSection(&((CGameBoy *)dwInstance)->cs);
		if (waveOutUnprepareHeader((HWAVEOUT)hWave, (WAVEHDR *)dwParam1, sizeof(WAVEHDR)))
		{
			MessageBox(hWnd, "Error unpreparing header.", "Game Lad", MB_OK | MB_ICONERROR);
		}

		delete (SOUNDBUFFER *)((WAVEHDR *)dwParam1)->dwUser;
		((CGameBoy *)dwInstance)->nBuffers--;
		LeaveCriticalSection(&((CGameBoy *)dwInstance)->cs);
	}
}*/



void __fastcall SoundUpdate(CGameBoy *GB)
{
	int			l, r;


	if (!GB->hWaveOut || (GB->FastFwd && GB->SoundBufferPosition == 0))
	{
		return;
	}


	l = ((signed)GB->Sound1L) / ((signed)96);
	r = ((signed)GB->Sound1R) / ((signed)96);
	GB->Sound1L = 0;
	GB->Sound1R = 0;
	l += ((signed)GB->Sound2L) / ((signed)96);
	r += ((signed)GB->Sound2R) / ((signed)96);
	GB->Sound2L = 0;
	GB->Sound2R = 0;
	l += ((signed)GB->Sound3L) / ((signed)96);
	r += ((signed)GB->Sound3R) / ((signed)96);
	GB->Sound3L = 0;
	GB->Sound3R = 0;
	l += ((signed)GB->Sound4L) / ((signed)96);
	r += ((signed)GB->Sound4R) / ((signed)96);
	GB->Sound4L = 0;
	GB->Sound4R = 0;


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

		EnterCriticalSection(&cs);
		waveOutPrepareHeader(GB->hWaveOut, &GB->SoundBuffer->wh, sizeof(GB->SoundBuffer->wh));
		waveOutWrite(GB->hWaveOut, &GB->SoundBuffer->wh, sizeof(GB->SoundBuffer->wh));
		if (!(GB->SoundBuffer = new SOUNDBUFFER))
		{
			MessageBox(hWnd, "Insufficient memory for sound.", NULL, MB_OK | MB_ICONERROR);
			GB->CloseSound();
		}
		LeaveCriticalSection(&cs);
		GB->SoundBufferPosition = 0;
	}

	GB->SoundTicks += 96;
}



void __fastcall Sound1(CGameBoy *GB)
{
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
			case 0: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) * 2) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 1: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) * 1) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 2: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 2) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 3: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 3) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 4: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 4) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 5: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 5) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 6: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 6) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
			case 7: pGameBoy->Sound4Frequency = 32768 / (((512 * 1024) / 7) >> ((pFF00_C(pGameBoy, 0x22) >> 4) + 1)); break;
		}
	}
	else
	{
		pGameBoy->Sound4Enabled = false;
		pFF00_C(pGameBoy, 0x26) &= ~0x08;
	}
}



BOOL CGameBoy::RestoreSound()
{
	WAVEFORMATEX	wfx;


	EnterCriticalSection(&cs);
	if (!hWaveOut)
	{
		if (waveOutGetNumDevs() == 0)
		{
			LeaveCriticalSection(&cs);
			MessageBox(NULL, "No audio devices present.", NULL, MB_OK | MB_ICONERROR);
			return true;
		}

		ZeroMemory(&wfx, sizeof(wfx));
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 2;
		wfx.nSamplesPerSec = 22050;
		wfx.wBitsPerSample = 8;
		wfx.nBlockAlign = 2;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD)hWnd, (DWORD)this, CALLBACK_WINDOW))
		{
			LeaveCriticalSection(&cs);
			MessageBox(NULL, "Could not open audio device.", NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		waveOutReset(hWaveOut);
	}

	if (!SoundBuffer)
	{
		if (!(SoundBuffer = new SOUNDBUFFER))
		{
			LeaveCriticalSection(&cs);
			CloseSound();
			MessageBox(hWnd, "Not enough memory for sound.", NULL, MB_OK | MB_ICONWARNING);
			return true;
		}
	}
	SoundBufferPosition = 0;
	Sound1L = Sound1R = 0;
	Sound2L = Sound2R = 0;
	LeaveCriticalSection(&cs);

	return false;
}



void CGameBoy::CloseSound()
{
	EnterCriticalSection(&cs);
	if (hWaveOut)
	{
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
		hWaveOut = NULL;
	}
	if (SoundBuffer)
	{
		delete SoundBuffer;
		SoundBuffer = NULL;
	}
	LeaveCriticalSection(&cs);
}

