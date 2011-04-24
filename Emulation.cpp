#include	<windows.h>
#include	"resource.h"

#define		EMULATION_CPP
#include	"Game Lad.h"
#include	"Emulation.h"
#include	"Debugger.h"
#include	"Z80.h"



#define		DefaultFrameSkip	0

//#define		TIMEDEMO


DWORD WINAPI GameLoop(void *pGameBoy)
{
	MSG				msg;
	BYTE			FrameSkip = DefaultFrameSkip;
	int				SkippedFrameNo = 0;

#ifdef TIMEDEMO
	LARGE_INTEGER	StartTime, CurrentTime, TimerFrequency;
	DWORD			Count = 0;
#endif //TEMIDEMO


	((CGameBoy *)pGameBoy)->PrepareEmulation(false);
	MemoryFlags = 0;

#ifdef TIMEDEMO
	QueryPerformanceFrequency(&TimerFrequency);
	QueryPerformanceCounter(&StartTime);
#endif //TIMEDEMO


	while (true)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				((CGameBoy *)pGameBoy)->CloseSound();
				if (GameBoyList.GetActive() == pGameBoy)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((CGameBoy *)pGameBoy)->hThread = NULL;
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		((CGameBoy *)pGameBoy)->MainLoop();
		if (((CGameBoy *)pGameBoy)->Flags & GB_INVALIDOPCODE)
		{
			((CGameBoy *)pGameBoy)->CloseSound();
			MessageBox(hWnd, ultoa((BYTE)ReadMem((CGameBoy *)pGameBoy, ((CGameBoy *)pGameBoy)->Reg_PC), NumBuffer, 16), "Invalid OP code", MB_OK | MB_ICONERROR);
			if (GameBoyList.GetActive() == pGameBoy)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			((CGameBoy *)pGameBoy)->hThread = NULL;
			return 0;
		}

		/*if (((CGameBoy *)pGameBoy)->FastFwd)
		{
			FrameSkip = 9;
		}
		else
		{
			FrameSkip = DefaultFrameSkip;
		}*/

		((CGameBoy *)pGameBoy)->Delay();


		if (!SkippedFrameNo)
		{
			((CGameBoy *)pGameBoy)->RefreshScreen();
		}
		if (++SkippedFrameNo > FrameSkip)
		{
			SkippedFrameNo = 0;
		}

#ifdef TIMEDEMO
		QueryPerformanceCounter(&CurrentTime);
		if ((CurrentTime.QuadPart - StartTime.QuadPart) >= (TimerFrequency.QuadPart * 64))
		{
			((CGameBoy *)pGameBoy)->DirectionKeys = 0;
			((CGameBoy *)pGameBoy)->Buttons = 0;
			((CGameBoy *)pGameBoy)->CloseSound();
			((CGameBoy *)pGameBoy)->hThread = NULL;
			char	NumBuffer2[10];
			MessageBox(hWnd, ultoa(Count, NumBuffer, 10), ultoa((DWORD)(CurrentTime.QuadPart - StartTime.QuadPart), NumBuffer2, 10), MB_OK | MB_ICONINFORMATION);
			return 0;
		}
		Count++;
#endif //TIMEDEMO
	}
}



DWORD WINAPI DebugGameLoop(void *pGameBoy)
{
	MSG				msg;
	BYTE			FrameSkip = DefaultFrameSkip;
	int				SkippedFrameNo = 0;


	((CGameBoy *)pGameBoy)->PrepareEmulation(true);
	MemoryFlags = 0;

	while (true)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				((CGameBoy *)pGameBoy)->CloseSound();
				if (GameBoyList.GetActive() == pGameBoy)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((CGameBoy *)pGameBoy)->hThread = NULL;
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		((CGameBoy *)pGameBoy)->DebugMainLoop();
		if (((CGameBoy *)pGameBoy)->Flags & GB_INVALIDOPCODE)
		{
			((CGameBoy *)pGameBoy)->CloseSound();
			MessageBox(hWnd, ultoa((BYTE)ReadMem((CGameBoy *)pGameBoy, ((CGameBoy *)pGameBoy)->Reg_PC), NumBuffer, 16), "Invalid OP code", MB_OK | MB_ICONERROR);
			if (GameBoyList.GetActive() == pGameBoy)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			((CGameBoy *)pGameBoy)->hThread = NULL;
			return 0;
		}

		if (((CGameBoy *)pGameBoy)->Flags & GB_ERROR)
		{
			((CGameBoy *)pGameBoy)->CloseSound();
			if (GameBoyList.GetActive() == pGameBoy)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			((CGameBoy *)pGameBoy)->hThread = NULL;
			return 0;
		}

		/*if (((CGameBoy *)pGameBoy)->FastFwd)
		{
			FrameSkip = 9;
		}
		else
		{
			FrameSkip = DefaultFrameSkip;
		}*/

		((CGameBoy *)pGameBoy)->Delay();


		if (!SkippedFrameNo)
		{
			((CGameBoy *)pGameBoy)->RefreshScreen();
		}
		if (++SkippedFrameNo > FrameSkip)
		{
			SkippedFrameNo = 0;
		}
	}
}



DWORD WINAPI StepGameLoop(void *pEmulationInfo)
{
	MSG				msg;
	BYTE			FrameSkip = DefaultFrameSkip;
	int				SkippedFrameNo = 0;
	WORD			pByte;


	switch (((EMULATIONINFO *)pEmulationInfo)->Flags)
	{
	case EMU_STEPINTO:
		pByte = ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC;
		break;

	case EMU_STEPOUT:
		pByte = ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_SP;
		break;
	}


	((EMULATIONINFO *)pEmulationInfo)->GameBoy1->PrepareEmulation(true);
	MemoryFlags = 0;

	while (true)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
				if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
				delete pEmulationInfo;
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Flags |= GB_EXITLOOP;
		((EMULATIONINFO *)pEmulationInfo)->GameBoy1->DebugMainLoop();
		if (((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Flags & GB_INVALIDOPCODE)
		{
			((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
			MessageBox(hWnd, ultoa((BYTE)ReadMem(((EMULATIONINFO *)pEmulationInfo)->GameBoy1, ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC), NumBuffer, 16), "Invalid OP code", MB_OK | MB_ICONERROR);
			if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
			delete pEmulationInfo;
			return 0;
		}

		/*if (((CGameBoy *)pGameBoy)->FastFwd)
		{
			FrameSkip = 9;
		}
		else
		{
			FrameSkip = DefaultFrameSkip;
		}*/

		if (((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Flags & GB_ERROR)
		{
			((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
			if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
			delete pEmulationInfo;
			return 0;
		}

		switch (((EMULATIONINFO *)pEmulationInfo)->Flags)
		{
		case EMU_STEPINTO:
			if (pByte != ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC)
			{
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
				if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
				delete pEmulationInfo;
				return 0;
			}
			break;

		case EMU_RUNTO:
			if (((EMULATIONINFO *)pEmulationInfo)->RunToOffset == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC)
			{
				if (((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC >= 0x4000 && ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC < 0x8000)
				{
					if (((EMULATIONINFO *)pEmulationInfo)->RunToBank != ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->ActiveRomBank)
					{
						break;
					}
				}
				if (((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC >= 0xA000 && ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC < 0xC000)
				{
					if (((EMULATIONINFO *)pEmulationInfo)->RunToBank != ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->ActiveRamBank)
					{
						break;
					}
				}
				if (((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC >= 0xD000 && ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_PC < 0xE000 && ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Flags & GB_ROM_COLOR)
				{
					if (((EMULATIONINFO *)pEmulationInfo)->RunToBank != (pFF00_C(((EMULATIONINFO *)pEmulationInfo)->GameBoy1, 0x4F) & 7))
					{
						break;
					}
				}
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
				if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
				delete pEmulationInfo;
				return 0;
			}
			break;

		case EMU_STEPOUT:
			if (pByte < ((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Reg_SP)
			{
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->CloseSound();
				if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->hThread = NULL;
				delete pEmulationInfo;
				return 0;
			}
			break;
		}


		if (pFF00_C(((EMULATIONINFO *)pEmulationInfo)->GameBoy1, 0x44) == 0x90 && (pFF00_C(((EMULATIONINFO *)pEmulationInfo)->GameBoy1, 0x40) & 0x80))
		{
			((EMULATIONINFO *)pEmulationInfo)->GameBoy1->Delay();


			if (!SkippedFrameNo)
			{
				((EMULATIONINFO *)pEmulationInfo)->GameBoy1->RefreshScreen();
			}
			if (++SkippedFrameNo > FrameSkip)
			{
				SkippedFrameNo = 0;
			}
		}
	}
}



		/*pByte = DisAsmGameBoy->Reg_SP;
		do
		{
			DisAsmGameBoy->Flags |= GB_EXITLOOP;
			DisAsmGameBoy->DebugMainLoop();
		}
		while (pByte >= DisAsmGameBoy->Reg_SP && !(DisAsmGameBoy->Flags & GB_ERROR));
		DisAsmGameBoy->Flags &= ~(GB_EXITLOOP | GB_ERROR);
		DisAsmCaretByte = DisAsmGameBoy->Reg_PC;
		//SendMessage(hWin, WM_APP_SCROLLTOCURSOR, 0, 0);

		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);*/
		/*do
		{
			DisAsmGameBoy->Flags = GB_EXITLOOP;
			DisAsmGameBoy->DebugMainLoop();
		}
		while (DisAsmCaretByte != DisAsmGameBoy->Reg_PC && !(DisAsmGameBoy->Flags & GB_ERROR));
		DisAsmGameBoy->Flags &= ~(GB_EXITLOOP | GB_ERROR);

		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);*/
/*void GameLoop(CGameBoy *GameBoy1, CGameBoy *GameBoy2)
{
	MSG				msg;
	LARGE_INTEGER	TimerFrequency, TimerCounter, TimerCounter2;


	StopEmulation = false;

	if (QueryPerformanceFrequency(&TimerFrequency))
	{
		TimerFrequency.QuadPart /= 60;
		QueryPerformanceCounter(&TimerCounter2);
	}
	else
	{
		MessageBox(hWnd, "No high resolution timer available.", "Game Lad", MB_OK | MB_ICONERROR);
		return;
	}


/*	Connected = false;

	/*if (GameBoy1 && GameBoy2)
	{
		if (GameBoy1->Enabled && GameBoy2->Enabled)
		{
			rct_2.left += rct.right;
			rct_2.top += rct.bottom;

			if (SendMessage(hConnected, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				Connected = true;
			}
		}
	}*/

/*	//Create Window
	hWndGraphic = CreateWindow("Screen", "Screen", (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME) | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 2 * GetSystemMetrics(SM_CXFIXEDFRAME) + rct.right, 2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION) + rct.bottom + 50,
		hWnd, NULL, hInstance, NULL);
	//ShowWindow(hWndGraphic, SW_SHOW);
	if (InitSound(hWndGraphic))
	{
		DestroyWindow(hWndGraphic);
		MessageBox(hWnd, "Couldn't initialize DirectSound.", NULL, MB_OK | MB_ICONERROR);
		return;
	}

	if (GameBoy1)
	{
		if (GameBoy1->Enabled)
		{
			if (Restore(GameBoy1))
			{
				DestroyWindow(hWndGraphic);
				MessageBox(hWnd, "Couldn't resume emulation.", NULL, MB_OK | MB_ICONERROR);
				return;
			}
		}
	}

	/*if (GameBoy2)
	{
		if (GameBoy2->Enabled)
		{
			if (Restore(GameBoy2))
			{
				DestroyWindow(hWndGraphic);
				MessageBox(hWnd, "Couldn't resume emulation.", NULL, MB_OK | MB_ICONERROR);
				return;
			}
		}
	}*/


/*	while (true)
	{
		if (GameBoy1->InvalidOPCode)
		{
			MessageBox(hWnd, ultoa(GameBoy1->InvalidOPCode, NumBuffer, 16), "Invalid OP code", MB_OK | MB_ICONERROR);
			GameBoy1->InvalidOPCode = 0;
			return;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
			case WM_COMMAND:
				if (LOWORD(msg.wParam) == ID_EMULATION_STOP)
				{
					return;
				}
				break;

			case WM_QUIT:
				PostQuitMessage(msg.wParam);
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (StopEmulation)
		{
			return;
		}

		ZeroMemory(&GameBoy1->KeysDown, sizeof(GameBoy1->KeysDown));
		/*if (GameBoy2)
		{
			ZeroMemory(&GameBoy2->KeysDown, sizeof(GameBoy2->KeysDown));
		}*/

/*		GetKeyboardState((BYTE *)&KbdBuffer);
#ifdef _DEBUG
		if (KbdBuffer[VK_END] & 0x80)
		{
			PostQuitMessage(__LINE__);
			return;
		}
#endif //_DEBUG
		if (KbdBuffer[VK_ESCAPE] & 0x80)
		{
			return;
		}

		if (KbdBuffer[VK_UP] & 0x80)
		{
			GameBoy1->KeysDown.Up = true;
		}
		if (KbdBuffer[VK_DOWN] & 0x80)
		{
			GameBoy1->KeysDown.Down = true;
		}
		if (KbdBuffer[VK_RIGHT] & 0x80)
		{
			GameBoy1->KeysDown.Right = true;
		}
		if (KbdBuffer[VK_LEFT] & 0x80)
		{
			GameBoy1->KeysDown.Left = true;
		}
		if (KbdBuffer['X'] & 0x80)
		{
			GameBoy1->KeysDown.A = true;
		}
		if (KbdBuffer['Z'] & 0x80)
		{
			GameBoy1->KeysDown.B = true;
		}
		if (KbdBuffer[VK_RETURN] & 0x80)
		{
			GameBoy1->KeysDown.Start = true;
		}
		if (KbdBuffer[VK_RSHIFT] & 0x80)
		{
			GameBoy1->KeysDown.Select = true;
		}

		/*if (GameBoy2)
		{
			if (KbdBuffer[VK_L] & 0x80)
			{
				GameBoy2->KeysDown.Up = true;
			}
			if (KbdBuffer[VK_PERIOD] & 0x80)
			{
				GameBoy2->KeysDown.Down = true;
			}
			if (KbdBuffer[VK_SLASH] & 0x80)
			{
				GameBoy2->KeysDown.Right = true;
			}
			if (KbdBuffer[VK_COMMA] & 0x80)
			{
				GameBoy2->KeysDown.Left = true;
			}
			if (KbdBuffer[VK_S] & 0x80)
			{
				GameBoy2->KeysDown.A = true;
			}
			if (KbdBuffer[VK_A] & 0x80)
			{
				GameBoy2->KeysDown.B = true;
			}
			if (KbdBuffer[VK_N] & 0x80)
			{
				GameBoy2->KeysDown.Start = true;
			}
			if (KbdBuffer[VK_M] & 0x80)
			{
				GameBoy2->KeysDown.Select = true;
			}
		}*/
		/*if (lpdidKeyboard)
		{
			if (lpdidKeyboard->GetDeviceState(sizeof(KbdBuffer), &KbdBuffer))
			{
				CloseGfx(0);
				return;
			}

#ifdef _DEBUG
			if (KbdBuffer[DIK_END] & 0x80)
			{
				CloseGfx(0);
				PostQuitMessage(__LINE__);
				return;
			}
#endif //_DEBUG
			if (KbdBuffer[DIK_ESCAPE] & 0x80)
			{
				CloseGfx(0);
				return;
			}
			if (KbdBuffer[DIK_GRAVE] & 0x80)
			{
				FrameSkip = 9;
			}
			else
			{
				FrameSkip = DefaultFrameSkip;
			}
			if (GameBoy1)
			{
				if (KbdBuffer[DIK_UP] & 0x80)
				{
					GameBoy1->KeysDown.Up = true;
				}
				if (KbdBuffer[DIK_DOWN] & 0x80)
				{
					GameBoy1->KeysDown.Down = true;
				}
				if (KbdBuffer[DIK_RIGHT] & 0x80)
				{
					GameBoy1->KeysDown.Right = true;
				}
				if (KbdBuffer[DIK_LEFT] & 0x80)
				{
					GameBoy1->KeysDown.Left = true;
				}
				if (KbdBuffer[DIK_X] & 0x80)
				{
					GameBoy1->KeysDown.A = true;
				}
				if (KbdBuffer[DIK_Z] & 0x80)
				{
					GameBoy1->KeysDown.B = true;
				}
				if (KbdBuffer[DIK_RETURN] & 0x80 || KbdBuffer[DIK_NUMPADENTER] & 0x80)
				{
					GameBoy1->KeysDown.Start = true;
				}
				if (KbdBuffer[DIK_RSHIFT] & 0x80)
				{
					GameBoy1->KeysDown.Select = true;
				}
			}
			if (GameBoy2)
			{
				if (KbdBuffer[DIK_L] & 0x80)
				{
					GameBoy2->KeysDown.Up = true;
				}
				if (KbdBuffer[DIK_PERIOD] & 0x80)
				{
					GameBoy2->KeysDown.Down = true;
				}
				if (KbdBuffer[DIK_SLASH] & 0x80)
				{
					GameBoy2->KeysDown.Right = true;
				}
				if (KbdBuffer[DIK_COMMA] & 0x80)
				{
					GameBoy2->KeysDown.Left = true;
				}
				if (KbdBuffer[DIK_S] & 0x80)
				{
					GameBoy2->KeysDown.A = true;
				}
				if (KbdBuffer[DIK_A] & 0x80)
				{
					GameBoy2->KeysDown.B = true;
				}
				if (KbdBuffer[DIK_N] & 0x80)
				{
					GameBoy2->KeysDown.Start = true;
				}
				if (KbdBuffer[DIK_M] & 0x80)
				{
					GameBoy2->KeysDown.Select = true;
				}
			}
		} //lpdidKeyboard*/

/*		GameBoy1->MainLoop(true);
		/*if (GameBoy2)
		{
			if (GameBoy2->Enabled)
			{
				MainLoop(GameBoy2, true);
			}
		}*/

		/*while (true)
		{
			if (GameBoy1 && GameBoy2)
			{
				if (GameBoy1->Enabled && GameBoy2->Enabled && GameBoy1->TotalTickCount >= 35112 && GameBoy2->TotalTickCount >= 35112)
				{
					break;
				}
			}
			else
			{
				if (GameBoy1)
				{
					if (GameBoy1->TotalTickCount >= 35112)
					{
						break;
					}
				}
				else
				{
					if (GameBoy2->TotalTickCount >= 35112)
					{
						break;
					}
				}
			}

			//SIO
			if (Connected)
			{
				if (GameBoy1->SIO && GameBoy2->SIO)
				{
					GameBoy1->SIO = false;
					GameBoy2->SIO = false;
					GameBoy1->MEM_GB[0x2F02] &= ~0x80;
					GameBoy2->MEM_GB[0x2F02] &= ~0x80;
					Data = GameBoy1->MEM_GB[0x2F01];
					GameBoy1->MEM_GB[0x2F01] = GameBoy2->MEM_GB[0x2F01];
					GameBoy2->MEM_GB[0x2F01] = Data;
					GameBoy1->MEM_GB[0x2F0F] |= 0x08;
					GameBoy2->MEM_GB[0x2F0F] |= 0x08;
				}
			}

			if (GameBoy1)
			{
				if (GameBoy1->Enabled)
				{
					if (GameBoy1->TotalTickCount < 35112)
					{
						GameBoy1->MainLoop(false);
					}
				}
			}
			if (GameBoy2)
			{
				if (GameBoy2->Enabled)
				{
					if (GameBoy2->TotalTickCount < 35112)
					{
						GameBoy2->MainLoop(false);
					}
				}
			}
		}*/

/*		if (KbdBuffer[VK_TAB] & 0x80)
		{
			FrameSkip = 9;
		}
		else
		{
			FrameSkip = DefaultFrameSkip;

			do
			{
				QueryPerformanceCounter(&TimerCounter);
			}
			while (TimerCounter.QuadPart - TimerCounter2.QuadPart < TimerFrequency.QuadPart);

			TimerCounter2 = TimerCounter;
		}


		if (!SkippedFrameNo)// && lpddsPrimary)
		{
			GameBoy1->RefreshScreen();
		}
		if (++SkippedFrameNo > FrameSkip)
		{
			SkippedFrameNo = 0;
		}
	}
}



//#define LCD
#define		ExtendedScreen
#define		DoubleScreen

DWORD		TickCount, TickCount2;
BOOL		Connected;
HWND		hWndGraphic;



/*#ifdef LCD

__forceinline __declspec(naked) __fastcall EraseLine(WORD *Dest)
{
	__asm
	{
		push	ebx

		mov		eax, 0x50
FillMore:
		dec		eax
		mov		ebx, dword ptr [ecx + 4 * eax]
		add		ebx, (10570 << 16) | 10570
		jns		HiOk
		or		ebx, 0x7FFF0000
HiOk:
		test	ebx, 0x00008000
		jz		LowOk
		or		ebx, 0x00007FFF
LowOk:
		and		ebx, 0x7FFF7FFF
		mov		[ecx + 4 * eax], ebx
		test	eax, eax
		jnz		FillMore

		pop		ebx
		ret
	}
}

#else*/

/*__forceinline __declspec(naked) __fastcall EraseLine(WORD *Dest)
{
	__asm
	{
		mov		eax, 0x50
FillMore:
		dec		eax
		mov		dword ptr [ecx + 4 * eax], 0xFFFFFFFF
		jnz		FillMore

		ret
	}
}*/

//#endif

