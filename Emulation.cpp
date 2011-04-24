#include	<windows.h>
#include	"resource.h"

#define		EMULATION_CPP
#include	"Game Lad.h"
#include	"Emulation.h"
#include	"Debugger.h"
#include	"Z80.h"



DWORD DebugGameLoop(void *pGameBoy)
{
	MSG				msg;
	BYTE			FrameSkip = DefaultFrameSkip;
	int				SkippedFrameNo = 0;


	((CGameBoy *)pGameBoy)->PrepareEmulation(true);
	MemoryFlags = DisAsmFlags = 0;

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
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			if (GameBoyList.GetActive() == pGameBoy)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			MessageBox(hWnd, "Invalid OP code", "Game Lad", MB_OK | MB_ICONWARNING);
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



DWORD StepGameLoop(void *pEmulationInfo)
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
	MemoryFlags = DisAsmFlags = 0;

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
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			if (GameBoyList.GetActive() == ((EMULATIONINFO *)pEmulationInfo)->GameBoy1)
			{
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			}
			MessageBox(hWnd, "Invalid OP code", "Game Lad", MB_OK | MB_ICONWARNING);
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



DWORD WINAPI GameBoyThreadProc(void *pGameBoy)
{
	MSG				msg;


	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(hStartStopEvent);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch (msg.message)
		{
		case WM_COMMAND:
			switch (LOWORD(msg.wParam))
			{
			case ID_EMULATION_EXECUTE:
				((CGameBoy *)pGameBoy)->Emulating = true;
				SetEvent(hStartStopEvent);
				GameLoop((CGameBoy *)pGameBoy);
				((CGameBoy *)pGameBoy)->Emulating = false;
				break;
			}
			break;

		case WM_APP_STEP:
			break;

		case WM_APP_RESUME:
			((CGameBoy *)pGameBoy)->Emulating = true;
			SetEvent(hStartStopEvent);
			GameLoop((CGameBoy *)pGameBoy);
			((CGameBoy *)pGameBoy)->Emulating = false;
			break;

		default:
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}



/*void GameLoop(CGameBoy *GameBoy1, CGameBoy *GameBoy2)
{
	MSG				msg;
	LARGE_INTEGER	TimerFrequency, TimerCounter, TimerCounter2;


	StopEmulation = false;

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

