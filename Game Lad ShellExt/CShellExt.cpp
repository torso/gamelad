#include	<windows.h>
#include	<shlobj.h>

#include	"Game Lad ShellExt.h"
#include	"CShellExt.h"
#include	"resource.h"



CShellExt::CShellExt()
{
	m_cRef = 0;
	m_pDataObj = NULL;

	DllRefCount++;
}



CShellExt::~CShellExt()
{
	if (m_pDataObj)
	{
		m_pDataObj->Release();
	}

	DllRefCount--;
}



STDMETHODIMP CShellExt::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;


	if (IsEqualIID(iid, IID_IShellExtInit) || IsEqualIID(iid, IID_IUnknown))
	{
		*ppvObject = (LPSHELLEXTINIT)this;
	}
	else if (IsEqualIID(iid, IID_IPersistFile))
	{
		*ppvObject = (LPPERSISTFILE)this;
	}
	else if (IsEqualIID(iid, IID_IShellPropSheetExt))
	{
		*ppvObject = (LPSHELLPROPSHEETEXT)this;
	}
#ifdef CONTEXTMENUHANDLER
	else if (IsEqualIID(iid, IID_IContextMenu))
	{
		*ppvObject = (LPCONTEXTMENU)this;
	}
#endif //CONTEXTMENUHANDLER
#ifdef ICONHANDLER
	else if (IsEqualIID(iid, IID_IExtractIcon))
	{
		*ppvObject = (LPEXTRACTICON)this;
	}
#endif //ICONHANDLER
#ifdef COPYHOOKHANDLER
	else if (IsEqualIID(iid, IID_IShellCopyHook))
	{
		*ppvObject = (LPCOPYHOOK)this;
	}
#endif //COPYHOOKHANDLER

	if (*ppvObject)
	{
		AddRef();

		return NOERROR;
	}

	return E_NOINTERFACE;
}



STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
	return ++m_cRef;
}



STDMETHODIMP_(ULONG) CShellExt::Release()
{
	if (--m_cRef)
	{
		return m_cRef;
	}

	delete this;

	return 0;
}



STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hKeyProgID)
{
	//Initialize can be called more than once

	if (m_pDataObj)
	{
		m_pDataObj->Release();
	}

	//Duplicate the object pointer and registry handle

	if (lpdobj)
	{
		m_pDataObj = lpdobj;
		lpdobj->AddRef();
	}

	return NOERROR;
}



STDMETHODIMP CShellExt::GetClassID(CLSID *lpClassID)
{
	return E_FAIL;
}



STDMETHODIMP CShellExt::IsDirty()
{
	return S_FALSE;
}



STDMETHODIMP CShellExt::Load(LPCOLESTR lpszFileName, DWORD dwMode)
{
	WideCharToMultiByte(CP_ACP, 0, lpszFileName, -1, m_szFilename, sizeof(m_szFilename), NULL, NULL);

	return NOERROR;
}



STDMETHODIMP CShellExt::Save(LPCOLESTR lpszFileName, BOOL fRemember)
{
	return E_FAIL;
}



STDMETHODIMP CShellExt::SaveCompleted(LPCOLESTR lpszFileName)
{
	return E_FAIL;
}



STDMETHODIMP CShellExt::GetCurFile(LPOLESTR *ppszFileName)
{
	return E_FAIL;
}



UINT CALLBACK RomInfoCallback(HWND hWin, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch(uMsg)
	{
	case PSPCB_CREATE:
		break;

	case PSPCB_RELEASE:
		if (ppsp->lParam)
		{
			((CShellExt *)(ppsp->lParam))->Release();
		}
		break;
	}

	return TRUE;
}



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



BOOL CALLBACK PropSheetEnumChildProc(HWND hWin, LPARAM lParam)
{
	BYTE		Byte143, Byte144;
	char		*Text, NumBuffer[13];
	DWORD		Size;


	switch (GetWindowLong(hWin, GWL_ID))
	{
	case IDC_TITLE:
		Byte143 = ((CShellExt *)lParam)->m_RomHeader[0x43];
		if (Byte143 & 0x80)
		{
			((CShellExt *)lParam)->m_RomHeader[0x43] = '\0';
		}
		Byte144 = ((CShellExt *)lParam)->m_RomHeader[0x44];
		((CShellExt *)lParam)->m_RomHeader[0x44] = '\0';
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&((CShellExt *)lParam)->m_RomHeader[0x34]);
		((CShellExt *)lParam)->m_RomHeader[0x43] = Byte143;
		((CShellExt *)lParam)->m_RomHeader[0x44] = Byte144;
		break;

	case IDC_GBC:
		if (((CShellExt *)lParam)->m_RomHeader[0x43] == 0xC0)
		{
			SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&"Yes, GBC only");
		}
		else if (((CShellExt *)lParam)->m_RomHeader[0x43] & 0x80)
		{
			SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&"Yes");
		}
		else
		{
			SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&"No");
		}
		break;

	case IDC_SGB:
		if (((CShellExt *)lParam)->m_RomHeader[0x46] == 3 && ((CShellExt *)lParam)->m_RomHeader[0x4B] == 0x33)
		{
			SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&"Yes");
		}
		else
		{
			SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&"No");
		}
		break;

	case IDC_CARTRIDGETYPE:
		switch (((CShellExt *)lParam)->m_RomHeader[0x47])
		{
		case 0x00: Text = "ROM"; break;
		case 0x01: Text = "ROM+MBC1"; break;
		case 0x02: Text = "ROM+MBC1+RAM"; break;
		case 0x03: Text = "ROM+MBC1+RAM+BATT"; break;
		case 0x05: Text = "ROM+MBC2"; break;
		case 0x06: Text = "ROM+MBC2+BATT"; break;
		case 0x08: Text = "ROM+RAM"; break;
		case 0x09: Text = "ROM+RAM+BATT"; break;
		case 0x0B: Text = "ROM+MMM01"; break;
		case 0x0C: Text = "ROM+MMM01+RAM"; break;
		case 0x0D: Text = "ROM+MMM01+RAM+BATT"; break;
		case 0x0F: Text = "ROM+MBC3+TIMER+BATT"; break;
		case 0x10: Text = "ROM+MBC3+TIMER+RAM+BATT"; break;
		case 0x11: Text = "ROM+MBC3"; break;
		case 0x12: Text = "ROM+MBC3+RAM"; break;
		case 0x13: Text = "ROM+MBC3+RAM+BATT"; break;
		case 0x19: Text = "ROM+MBC5"; break;
		case 0x1A: Text = "ROM+MBC5+RAM"; break;
		case 0x1B: Text = "ROM+MBC5+RAM+BATT"; break;
		case 0x1C: Text = "ROM+MBC5+RUMBLE"; break;
		case 0x1D: Text = "ROM+MBC5+RUMBLE+RAM"; break;
		case 0x1E: Text = "ROM+MBC5+RUMBLE+RAM+BATT"; break;
		case 0x1F: Text = "Pocket Camera"; break;
		case 0xFD: Text = "Bandai TAMA5"; break;
		case 0xFE: Text = "Hudson HuC-3"; break;
		case 0xFF: Text = "Hudson HuC-1"; break;
		default: Text = "Unknown"; break;
		}
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)Text);
		break;

	case IDC_ROMSIZE:
		ultoa((RomSize(((CShellExt *)lParam)->m_RomHeader[0x48]) + 1) * 16, NumBuffer, 10);
		strcat(NumBuffer, " kB");
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)NumBuffer);
		break;

	case IDC_RAMSIZE:
		switch (((CShellExt *)lParam)->m_RomHeader[0x49])
		{
		case 1:
			Size = 2;
			break;
		case 2:
			Size = 8;
			break;
		case 3:
			Size = 32;
			break;
		case 4:
			Size = 128;
			break;
		default:
			Size = 0;
			break;
		}
		ultoa(Size, NumBuffer, 10);
		strcat(NumBuffer, " kB");
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)NumBuffer);
		break;

	case IDC_BADNINTENDOCHARACTERAREA:
	case ID_FIXNINTENDOCHARACTERAREA:
		if (((CShellExt *)lParam)->m_RomHeaderFlags & RHF_BADNINTENDOCHARACTERAREA)
		{
			ShowWindow(hWin, SW_SHOW);
		}
		else
		{
			ShowWindow(hWin, SW_HIDE);
		}
		break;

	case IDC_BADCOMPLEMENTCHECK:
	case ID_FIXCOMPLEMENTCHECK:
		if (((CShellExt *)lParam)->m_RomHeaderFlags & RHF_BADCOMPLEMENTCHECK)
		{
			ShowWindow(hWin, SW_SHOW);
		}
		else
		{
			ShowWindow(hWin, SW_HIDE);
		}
		break;

	case IDC_BADCHECKSUM:
	case ID_FIXCHECKSUM:
		if (((CShellExt *)lParam)->m_RomHeaderFlags & RHF_BADCHECKSUM)
		{
			ShowWindow(hWin, SW_SHOW);
		}
		else
		{
			ShowWindow(hWin, SW_HIDE);
		}
		break;
	}

	return true;
}



BYTE		NintendoGraphic[48] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

BOOL CALLBACK RomInfoDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HANDLE		hFile;
	DWORD		nBytes;
	CShellExt	*cse;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FIXNINTENDOCHARACTERAREA:
			cse = (CShellExt *)GetWindowLong(hDlg, GWL_USERDATA);
			if ((hFile = CreateFile(cse->m_szFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (SetFilePointer(hFile, 0x104, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (!WriteFile(hFile, NintendoGraphic, sizeof(NintendoGraphic), &nBytes, NULL))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			CloseHandle(hFile);

			cse->m_RomHeaderFlags &= ~RHF_BADNINTENDOCHARACTERAREA;

			//Hide text and button
			EnumChildWindows(hDlg, PropSheetEnumChildProc, (LPARAM)cse);
			return true;

		case ID_FIXCOMPLEMENTCHECK:
			cse = (CShellExt *)GetWindowLong(hDlg, GWL_USERDATA);

			if ((hFile = CreateFile(cse->m_szFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (SetFilePointer(hFile, 0x14D, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (!WriteFile(hFile, &cse->m_ComplementCheck, 1, &nBytes, NULL))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			CloseHandle(hFile);

			cse->m_RomHeaderFlags &= ~RHF_BADCOMPLEMENTCHECK;

			//Hide text and button
			EnumChildWindows(hDlg, PropSheetEnumChildProc, (LPARAM)cse);
			return true;

		case ID_FIXCHECKSUM:
			cse = (CShellExt *)GetWindowLong(hDlg, GWL_USERDATA);

			if ((hFile = CreateFile(cse->m_szFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (SetFilePointer(hFile, 0x14E, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			if (!WriteFile(hFile, &cse->m_CheckSum, 2, &nBytes, NULL))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(hDlg);
				return true;
			}
			CloseHandle(hFile);

			cse->m_RomHeaderFlags &= ~RHF_BADCHECKSUM;

			//Hide text and button
			EnumChildWindows(hDlg, PropSheetEnumChildProc, (LPARAM)cse);
			return true;
		}
		break;

	case WM_INITDIALOG:
		SetWindowLong(hDlg, GWL_USERDATA, (LPARAM)((PROPSHEETPAGE *)lParam)->lParam);

		EnumChildWindows(hDlg, PropSheetEnumChildProc, (LPARAM)(CShellExt *)((PROPSHEETPAGE *)lParam)->lParam);
		return true;

	/*case WM_DESTROY:
		break;*/

	/*case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code)
		{
		case PSN_APPLY:
			break;
		}
		break;*/
	}

	return false;
}



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
			BitBlt(Paint.hdc, 1, 1, 160, 144, (HDC)GetWindowLong(hWin, GWL_USERDATA), 0, 0, SRCCOPY);

			EndPaint(hWin, &Paint);
		}
		return 0;
	}

	return CallWindowProc(((CShellExt *)GetWindowLong(GetParent(hWin), GWL_USERDATA))->m_OldStaticWndProc, hWin, uMsg, wParam, lParam);
}



BOOL CALLBACK SaveStateDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hDlg, GWL_USERDATA, (LPARAM)((PROPSHEETPAGE *)lParam)->lParam);

		SendDlgItemMessage(hDlg, IDC_VERSION, WM_SETTEXT, 0, (LPARAM)((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_Version);

		if (((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_SaveStateFlags & SSF_SCREEN)
		{
			((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_hBitmapWnd = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BLACKFRAME, 10, 30, 162, 146, hDlg, NULL, hInstance, NULL);
			((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_OldStaticWndProc = (WNDPROC)SetWindowLong(((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_hBitmapWnd, GWL_WNDPROC, (long)BitmapWndProc);
			SetWindowLong(((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_hBitmapWnd, GWL_USERDATA, (long)((CShellExt *)((PROPSHEETPAGE *)lParam)->lParam)->m_hBitmapDC);
		}
		return false;
	}

	return false;
}



#define		ReadFromFile(dest)									\
	if (!ReadFile(hFile, &dest, sizeof(dest), &nBytes, NULL))	\
	{															\
		CloseHandle(hFile);										\
		return NOERROR;											\
	}															\
	if (nBytes != sizeof(dest))									\
	{															\
		CloseHandle(hFile);										\
		return NOERROR;											\
	}

#define		ReadFromFile2(dest, size)							\
	if (!ReadFile(hFile, &dest, (size), &nBytes, NULL))			\
	{															\
		CloseHandle(hFile);										\
		return NOERROR;											\
	}															\
	if (nBytes != (size))										\
	{															\
		CloseHandle(hFile);										\
		return NOERROR;											\
	}

STDMETHODIMP CShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
	PROPSHEETPAGE	psp;
	HPROPSHEETPAGE	hpsp;
	FORMATETC		fmte;
	STGMEDIUM		medium;
	HANDLE			hFile;
	DWORD			nBytes, FileSize, Value, Pos, y;
	BYTE			Buffer[0x100];
	BITMAPINFO		bmi;


	if (!m_pDataObj)
	{
		return NOERROR;
	}

	fmte.cfFormat = CF_HDROP;
	fmte.ptd = NULL;
	fmte.dwAspect = DVASPECT_CONTENT;
	fmte.lindex = -1;
	fmte.tymed = TYMED_HGLOBAL;

	if (m_pDataObj->GetData(&fmte, &medium))
	{
		return NOERROR;
	}
	if (!medium.hGlobal)
	{
		return NOERROR;
	}

	//Find out how many files the user has selected...
	if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, 0, 0) < 2)
	{
		DragQueryFile((HDROP)medium.hGlobal, 0, m_szFilename, sizeof(m_szFilename));
		if (!strchr(m_szFilename, '.'))
		{
			return NOERROR;
		}

		if ((hFile = CreateFile(m_szFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			return NOERROR;
		}
		psp.dwSize = sizeof(psp);
		psp.dwFlags = PSP_USEREFPARENT | PSP_USECALLBACK;
		psp.hInstance = hInstance;
		psp.pfnCallback = RomInfoCallback;
		psp.pcRefParent = &DllRefCount;
		psp.lParam = (LPARAM)this;
		if (tolower(*(strrchr(m_szFilename, '.') + 1)) == 'g' && tolower(*(strrchr(m_szFilename, '.') + 2)) == 'l')
		{
			m_SaveStateFlags = 0;

			ReadFromFile(Value);
			if (Value != ('g' | ('l' << 8) | ('s' << 16)))
			{
				CloseHandle(hFile);
				return NOERROR;
			}
			ReadFromFile(Value);
			Pos = 0;
			do
			{
				ReadFromFile(m_Version[Pos]);
				Pos++;
			}
			while (m_Version[Pos - 1] != '\0');
			if (Value <= 2)
			{
				if (Value >= 2)
				{
					ReadFromFile2(Value, 3);
				}
				else
				{
					ReadFromFile2(Value, 2);
				}

				m_SaveStateFlags |= SSF_SCREEN;

				if (!(m_hBitmapDC = CreateCompatibleDC(NULL)))
				{
					CloseHandle(hFile);
					return NOERROR;
				}
				ZeroMemory(&bmi, sizeof(bmi));
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = 160;
				bmi.bmiHeader.biHeight = -144;
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 16;
				bmi.bmiHeader.biCompression = BI_RGB;
				if (!(m_hBitmap = CreateDIBSection(m_hBitmapDC, &bmi, DIB_RGB_COLORS, (void **)&m_pBitmap, NULL, 0)))
				{
					ReleaseDC(NULL, m_hBitmapDC);
					m_hBitmapDC = NULL;
				}
				m_hOldBitmap = (HBITMAP)SelectObject(m_hBitmapDC, m_hBitmap);

				for (y = 0; y < 144; y++)
				{
					if (!ReadFile(hFile, m_pBitmap + y * 160, 160 * 2, &nBytes, NULL))
					{
						CloseHandle(hFile);
						return NOERROR;
					}
					if (nBytes != 160 * 2)
					{
						CloseHandle(hFile);
						return NOERROR;
					}
				}
			}
			psp.pszTemplate = MAKEINTRESOURCE(IDD_SAVESTATE);
			psp.pfnDlgProc = SaveStateDlgProc;
			CloseHandle(hFile);
		}
		else
		{
			if (!ReadFile(hFile, Buffer, 0x100, &nBytes, NULL))
			{
				CloseHandle(hFile);
				return NOERROR;
			}
			if (!ReadFile(hFile, m_RomHeader, 0x50, &nBytes, NULL))
			{
				CloseHandle(hFile);
				return NOERROR;
			}
			FileSize = GetFileSize(hFile, NULL);
			if (nBytes != 0x50 || (RomSize(m_RomHeader[0x48]) + 1) * 16384U != FileSize)
			{
				CloseHandle(hFile);
				return NOERROR;
			}

			m_RomHeaderFlags = 0;

			//Complement check
			m_ComplementCheck = 0xE7;
			for (nBytes = 25; nBytes != 0; nBytes--)
			{
				m_ComplementCheck -= m_RomHeader[0x33 + nBytes];
			}
			if (m_ComplementCheck != m_RomHeader[0x4D])
			{
				m_RomHeaderFlags |= RHF_BADCOMPLEMENTCHECK;
			}

			//Checksum
			m_CheckSum = 0x1546 + m_ComplementCheck + m_RomHeader[0] + m_RomHeader[1] + m_RomHeader[2] + m_RomHeader[3];
			for (nBytes = 0; nBytes < 0x100; nBytes++)
			{
				m_CheckSum += Buffer[nBytes];
			}
			for (nBytes = 0x34; nBytes < 0x4D; nBytes++)
			{
				m_CheckSum += m_RomHeader[nBytes];
			}
			if (!ReadFile(hFile, Buffer, 0xB0, &nBytes, NULL))
			{
				CloseHandle(hFile);
				return NOERROR;
			}
			for (nBytes = 0; nBytes < 0xB0; nBytes++)
			{
				m_CheckSum += Buffer[nBytes];
			}
			while (true)
			{
				if (!ReadFile(hFile, Buffer, 0x100, &nBytes, NULL))
				{
					CloseHandle(hFile);
					return NOERROR;
				}
				if (nBytes < 0x100)
				{
					if (nBytes != 0)
					{
						char NumBuffer[10];
						MessageBox(NULL, itoa(nBytes, NumBuffer, 16), NULL, MB_OK);
					}
					break;
				}

				for (nBytes = 0; nBytes < 0x100; nBytes++)
				{
					m_CheckSum += Buffer[nBytes];
				}
			}
			m_CheckSum = (m_CheckSum << 8) | (m_CheckSum >> 8);
			if (m_CheckSum != *(WORD *)&m_RomHeader[0x4E])
			{
				m_RomHeaderFlags |= RHF_BADCHECKSUM;
			}

			CloseHandle(hFile);

			//Nintendo character area
			if (memcmp(&m_RomHeader[4], NintendoGraphic, sizeof(NintendoGraphic)))
			{
				m_RomHeaderFlags |= RHF_BADNINTENDOCHARACTERAREA;
			}

			psp.pszTemplate = MAKEINTRESOURCE(IDD_ROMINFO);
			psp.pfnDlgProc = RomInfoDlgProc;
		}

		AddRef();
		if (hpsp = CreatePropertySheetPage(&psp))
		{
			if (!lpfnAddPage(hpsp, lParam)) 
			{
				DestroyPropertySheetPage(hpsp);
				Release();
				return NOERROR;
			}
		}
	}

	return NOERROR;
}



STDMETHODIMP CShellExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam)
{
	return E_FAIL;
}



#ifdef CONTEXTMENUHANDLER
STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	UINT	idCmd = idCmdFirst;
	char	szMenuText[64];
	char	szMenuText2[64];
	char	szMenuText3[64];
	char	szMenuText4[64];
	BOOL	bAppendItems = true;


	if ((uFlags & 0x000F) == CMF_NORMAL)
	{
		lstrcpy(szMenuText, "&New .GAK menu 1, Normal File");
		lstrcpy(szMenuText2, "&New .GAK menu 2, Normal File");
		lstrcpy(szMenuText3, "&New .GAK menu 3, Normal File");
		lstrcpy(szMenuText4, "&New .GAK menu 4, Normal File");
	}
	else
		if (uFlags & CMF_VERBSONLY)
		{
			lstrcpy(szMenuText, "&New .GAK menu 1, Shortcut File");
			lstrcpy(szMenuText2, "N&ew .GAK menu 2, Shortcut File");
			lstrcpy(szMenuText3, "&New .GAK menu 3, Shortcut File");
			lstrcpy(szMenuText4, "&New .GAK menu 4, Shortcut File");
		}
	else
		if (uFlags & CMF_EXPLORE)
		{
			lstrcpy(szMenuText, "&New .GAK menu 1, Normal File right click in Explorer");
			lstrcpy(szMenuText2, "N&ew .GAK menu 2, Normal File right click in Explorer");
			lstrcpy(szMenuText3, "&New .GAK menu 3, Normal File right click in Explorer");
			lstrcpy(szMenuText4, "&New .GAK menu 4, Normal File right click in Explorer");
		}
	else
		if (uFlags & CMF_DEFAULTONLY)
		{
			bAppendItems = FALSE;
		}
	else
		{
			char szTemp[32];

			wsprintf(szTemp, "uFlags==>%d\r\n", uFlags);
			bAppendItems = FALSE;
		}

	if (bAppendItems)
	{
		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

		InsertMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION, idCmd++, szMenuText);

		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

		InsertMenu(hMenu, indexMenu++, MF_STRING | MF_BYPOSITION, idCmd++, szMenuText2);

		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

		InsertMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION, idCmd++, szMenuText3);

		InsertMenu(hMenu, indexMenu++, MF_STRING | MF_BYPOSITION, idCmd++, szMenuText4);

		return ResultFromShort(idCmd-idCmdFirst);
	}

	return NOERROR;
}



STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
	HRESULT hr = E_INVALIDARG;


	if (!HIWORD(lpici->lpVerb))
	{
		UINT idCmd = LOWORD(lpici->lpVerb);

		/*switch (idCmd)
		{
		case 0:
			hr = DoGAKMenu1(lpici->hwnd, lpici->lpDirectory, lpici->lpVerb, lpici->lpParameters, lpici->nShow);
			break;

		case 1:
			hr = DoGAKMenu2(lpici->hwnd, lpici->lpDirectory, lpici->lpVerb, lpici->lpParameters, lpici->nShow);
			break;

		case 2:
			hr = DoGAKMenu3(lpici->hwnd, lpici->lpDirectory, lpici->lpVerb, lpici->lpParameters, lpici->nShow);
			break;

		case 3:
			hr = DoGAKMenu4(lpici->hwnd, lpici->lpDirectory, lpici->lpVerb, lpici->lpParameters, lpici->nShow);
			break;
		}*/
	}
	return hr;
}



STDMETHODIMP CShellExt::GetCommandString(UINT idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
	switch (idCmd)
	{
	case 0:
		lstrcpy(pszName, "New menu item number 1");
		break;

	case 1:
		lstrcpy(pszName, "New menu item number 2");
		break;

	case 2:
		lstrcpy(pszName, "New menu item number 3");
		break;

	case 3:
		lstrcpy(pszName, "New menu item number 4");
		break;
	}

	return NOERROR;
}
#endif //CONTEXTMENUHANDLER



#ifdef ICONHANDLER
STDMETHODIMP CShellExt::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, LPINT piIndex, UINT *pwFlags)
{
	strcpy(szIconFile, m_szFilename);

	*pwFlags = GIL_DONTCACHE | GIL_PERINSTANCE | GIL_NOTFILENAME;
	return NOERROR;
}



STDMETHODIMP CShellExt::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	HANDLE			hFile;
	DWORD			Value, nBytes, Pos, y;
	BYTE			*ANDbits, *XORbits;
	HDC				hDisplayDC, hdcSrc, hdcDst;
	BITMAPINFO		bmi;
	HBITMAP			hbmpSrc, hbmpDst, hbmpSrcOld, hbmpDstOld;
	BITMAP			Bitmap;


	if ((hFile = CreateFile(pszFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		return S_FALSE;
	}
	ReadFromFile(Value);
	if (Value != ('g' | ('l' << 8) | ('s' << 16)))
	{
		CloseHandle(hFile);
		return S_FALSE;
	}
	ReadFromFile(Value);
	if (Value > 2)
	{
		CloseHandle(hFile);
		return S_FALSE;
	}
	Pos = 0;
	do
	{
		ReadFromFile(m_Version[Pos]);
		Pos++;
	}
	while (m_Version[Pos - 1] != '\0');

	if (Value >= 2)
	{
		ReadFromFile(m_Version[Pos]);
	}

	if (!(hDisplayDC = CreateDC("DISPLAY", NULL, NULL, NULL)))
	{
		CloseHandle(hFile);
		return S_FALSE;
	}
	if (!(hdcSrc = CreateCompatibleDC(hDisplayDC)))
	{
		CloseHandle(hFile);
		DeleteDC(hDisplayDC);
		return S_FALSE;
	}
	if (!(hdcDst = CreateCompatibleDC(hDisplayDC)))
	{
		CloseHandle(hFile);
		DeleteDC(hDisplayDC);
		DeleteDC(hdcSrc);
		return S_FALSE;
	}
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 160;
	bmi.bmiHeader.biHeight = -144;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 16;
	bmi.bmiHeader.biCompression = BI_RGB;
	if (!(hbmpSrc = CreateDIBSection(m_hBitmapDC, &bmi, DIB_RGB_COLORS, (void **)&m_pBitmap, NULL, 0)))
	{
		CloseHandle(hFile);
		DeleteDC(hDisplayDC);
		DeleteDC(hdcSrc);
		DeleteDC(hdcDst);
		return S_FALSE;
	}
	hbmpSrcOld = (HBITMAP)SelectObject(hdcSrc, hbmpSrc);

	for (y = 0; y < 144; y++)
	{
		if (!ReadFile(hFile, m_pBitmap + y * 160, 160 * 2, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DeleteDC(hDisplayDC);
			SelectObject(hdcSrc, hbmpSrcOld);
			DeleteDC(hdcSrc);
			DeleteDC(hdcDst);
			DeleteObject(hbmpSrc);
			return S_FALSE;
		}
		if (nBytes != 160 * 2)
		{
			CloseHandle(hFile);
			DeleteDC(hDisplayDC);
			SelectObject(hdcSrc, hbmpSrcOld);
			DeleteDC(hdcSrc);
			DeleteDC(hdcDst);
			DeleteObject(hbmpSrc);
			return S_FALSE;
		}
	}
	CloseHandle(hFile);

	if (!(ANDbits = new BYTE[LOWORD(nIconSize) * LOWORD(nIconSize) / 8]))
	{
		DeleteDC(hDisplayDC);
		SelectObject(hdcSrc, hbmpSrcOld);
		DeleteDC(hdcSrc);
		DeleteDC(hdcDst);
		DeleteObject(hbmpSrc);
		return S_FALSE;
	}
	ZeroMemory(ANDbits, LOWORD(nIconSize) * LOWORD(nIconSize) / 8);

	if (!(hbmpDst = CreateCompatibleBitmap(hDisplayDC, LOWORD(nIconSize), LOWORD(nIconSize))))
	{
		DeleteDC(hDisplayDC);
		SelectObject(hdcSrc, hbmpSrcOld);
		DeleteDC(hdcSrc);
		DeleteDC(hdcDst);
		DeleteObject(hbmpSrc);
		delete ANDbits;
		return S_FALSE;
	}
	GetObject(hbmpDst, sizeof(BITMAP), &Bitmap);
	hbmpDstOld = (HBITMAP)SelectObject(hdcDst, hbmpDst);
	StretchBlt(hdcDst, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, hdcSrc, 7 /*+ 48*/, 0 /*+ 40*/, 160 /*- 48 * 2*/, 144 /*- 40 * 2*/, SRCCOPY);
	if (!(XORbits = new BYTE[Bitmap.bmWidthBytes * Bitmap.bmHeight * Bitmap.bmPlanes]))
	{
		DeleteDC(hDisplayDC);
		SelectObject(hdcSrc, hbmpSrcOld);
		DeleteDC(hdcSrc);
		SelectObject(hdcDst, hbmpDstOld);
		DeleteDC(hdcDst);
		DeleteObject(hbmpSrc);
		DeleteObject(hbmpDst);
		delete ANDbits;
		return S_FALSE;
	}
	GetBitmapBits(hbmpDst, Bitmap.bmWidthBytes * Bitmap.bmHeight * Bitmap.bmPlanes, XORbits);
	SelectObject(hdcDst, hbmpDstOld);
	DeleteDC(hdcDst);
	DeleteObject(hbmpDst);
	*phiconSmall = *phiconLarge = CreateIcon(hInstance, Bitmap.bmWidth, Bitmap.bmHeight, (BYTE)Bitmap.bmPlanes, (BYTE)Bitmap.bmBitsPixel, ANDbits, XORbits);
	SelectObject(hdcSrc, hbmpSrcOld);
	DeleteDC(hdcSrc);
	DeleteObject(hbmpSrc);
	delete ANDbits;
	delete XORbits;
	DeleteDC(hDisplayDC);

	return NOERROR;
}
#endif //ICONHANDLER



#ifdef COPYHOOKHANDLER
STDMETHODIMP_(UINT) CShellExt::CopyCallback(HWND hwnd, UINT wFunc, UINT wFlags, LPCSTR pszSrcFile, DWORD dwSrcAttribs, LPCSTR pszDestFile, DWORD dwDestAttribs)
{
	MessageBox(NULL, "CopyCallback!", "Game Lad", MB_OK);

	return IDYES;
}
#endif //COPYHOOKHANDLER

