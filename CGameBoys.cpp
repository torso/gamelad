#include	<windows.h>
#include	"resource.h"

#define		CGAMEBOYS_CPP
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CGameBoys.h"
#include	"Debugger.h"



CGameBoys::CGameBoys()
{
	FirstGameBoy = NULL;
	ActiveGameBoy = NULL;
}



CGameBoy *CGameBoys::NewGameBoy(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart)
{
	GameBoy		*LastGameBoy;


	if (!(LastGameBoy = new GameBoy))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return NULL;
	}
	LastGameBoy->pNext = FirstGameBoy;
	FirstGameBoy = LastGameBoy;

	if (!(LastGameBoy->pGameBoy = new CGameBoy(Flags)))
	{
		FirstGameBoy = LastGameBoy->pNext;
		delete LastGameBoy;
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return NULL;
	}
	if (LastGameBoy->pGameBoy->Init(pszROMFilename, pszStateFilename, pszBatteryFilename))
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



BOOL CGameBoys::DeleteGameBoy(CGameBoy *pCGameBoy)
{
	GameBoy		*pGameBoy = FirstGameBoy, *pGameBoy2;
	char		szBuffer[0x100];


	pCGameBoy->Stop();

	switch (Settings.SaveState)
	{
	case SAVESTATE_ALWAYS:
		pCGameBoy->SaveState();
		break;
	case SAVESTATE_PROMPT:
		switch (MessageBox(hMsgParent, String(IDS_PROMPT_SAVESTATE), "Game Lad", MB_ICONQUESTION | MB_YESNOCANCEL))
		{
		case IDYES:
			pCGameBoy->SaveState();
			break;
		case IDCANCEL:
			pCGameBoy->Resume();
			return true;
		}
	}
	switch (Settings.SaveBattery)
	{
	case SAVEBATTERY_ALWAYS:
		pCGameBoy->SaveBattery(false, false);
		break;
	case SAVEBATTERY_PROMPT:
		if (pCGameBoy->SaveBattery(true, false))
		{
			pCGameBoy->Resume();
			return true;
		}
	}

	if (pCGameBoy == ActiveGameBoy)
	{
		ActiveGameBoy = NULL;
	}

	if (FirstGameBoy->pGameBoy == pCGameBoy)
	{
		FirstGameBoy = FirstGameBoy->pNext;
		delete pCGameBoy;
		delete pGameBoy;

		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		return false;
	}

	while (pGameBoy->pNext->pGameBoy != pCGameBoy)
	{
		pGameBoy = pGameBoy->pNext;
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



BOOL CGameBoys::DeleteAll()
{
	while (FirstGameBoy)
	{
		if (DeleteGameBoy(FirstGameBoy->pGameBoy))
		{
			return true;
		}
	}

	return false;
}



void CGameBoys::EnableSound()
{
	Settings.SoundEnabled = true;
}



void CGameBoys::CloseSound()
{
	GameBoy		*pGameBoy = FirstGameBoy;


	Settings.SoundEnabled = false;

	while (pGameBoy)
	{
		pGameBoy->pGameBoy->CloseSound();
		pGameBoy = pGameBoy->pNext;
	}
}



CGameBoy *CGameBoys::GetActive()
{
	return ActiveGameBoy;
}



LPARAM CGameBoys::WndProc(CGameBoy *pGameBoy, HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pGameBoy)
	{
		switch (uMsg)
		{
		case WM_MDIACTIVATE:
			if (hWin == (HWND)lParam)
			{
				if (ActiveGameBoy != pGameBoy)
				{
					ActiveGameBoy = pGameBoy;
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
					MemoryFlags = DisAsmFlags = 0;
				}
			}
			break;

		case WM_CLOSE:
			if (pGameBoy = GetActive())
			{
				DeleteGameBoy(pGameBoy);
			}
			return 0;
		}

		return pGameBoy->GameBoyWndProc(uMsg, wParam, lParam);
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}

