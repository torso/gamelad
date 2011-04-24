#include	<windows.h>
#include	"resource.h"

#define		CGAMEBOYS_CPP
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CGameBoys.h"
#include	"Debugger.h"
#include	"Input.h"



CGameBoys::CGameBoys()
{
	m_pFirstGameBoy = NULL;
	m_pActiveGameBoy = NULL;
}



#ifdef _DEBUG

CGameBoys::~CGameBoys()
{
	if (m_pFirstGameBoy)
	{
		__asm int 3;
	}
}

#endif //_DEBUG



BOOL CGameBoys::GameBoyExists(CGameBoy *pGameBoy)
{
	GAMEBOYITEM		*pGameBoyItem;


	EnterCriticalSection(&csGameBoy);

	for (pGameBoyItem = m_pFirstGameBoy; pGameBoyItem; pGameBoyItem = pGameBoyItem->pNext)
	{
		if (pGameBoyItem->pGameBoy == pGameBoy)
		{
			LeaveCriticalSection(&csGameBoy);
			return true;
		}
	}

	LeaveCriticalSection(&csGameBoy);

	return false;
}



CGameBoy *CGameBoys::NewGameBoy(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart)
{
	GAMEBOYITEM		*pGameBoyItem;


	if (!(pGameBoyItem = new GAMEBOYITEM))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return NULL;
	}

	if (!(pGameBoyItem->pGameBoy = new CGameBoy(Flags)))
	{
		delete pGameBoyItem;
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return NULL;
	}
	if (pGameBoyItem->pGameBoy->Init(pszROMFilename, pszStateFilename, pszBatteryFilename))
	{
		delete pGameBoyItem->pGameBoy;
		delete pGameBoyItem;
		return NULL;
	}

	EnterCriticalSection(&csGameBoy);

	pGameBoyItem->pNext = m_pFirstGameBoy;
	m_pFirstGameBoy = pGameBoyItem;

	m_pActiveGameBoy = pGameBoyItem->pGameBoy;

	if (AutoStart == AUTOSTART_DEBUG)
	{
		SendMessage(hWnd, WM_COMMAND, ID_EMULATION_STARTDEBUG, 0);
	}
	if (AutoStart == AUTOSTART_EXECUTE)
	{
		SendMessage(hWnd, WM_COMMAND, ID_EMULATION_EXECUTE, 0);
	}

	LeaveCriticalSection(&csGameBoy);

	PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
	return pGameBoyItem->pGameBoy;
}



BOOL CGameBoys::DeleteGameBoy(CGameBoy *pGameBoy)
{
	GAMEBOYITEM	*pGameBoyItem, *pGameBoyItem2;
	char		szBuffer[0x100];


	pGameBoyItem = m_pFirstGameBoy;

	pGameBoy->Stop();

	EnterCriticalSection(&csGameBoy);

	if (!pGameBoy->CanUnload())
	{
		pGameBoy->Resume();
		LeaveCriticalSection(&csGameBoy);
		return true;
	}

	switch (Settings.SaveState)
	{
	case SAVESTATE_ALWAYS:
		pGameBoy->SaveState();
		break;
	case SAVESTATE_PROMPT:
		switch (MessageBox(hMsgParent, String(IDS_PROMPT_SAVESTATE), "Game Lad", MB_ICONQUESTION | MB_YESNOCANCEL))
		{
		case IDYES:
			pGameBoy->SaveState();
			break;
		case IDCANCEL:
			pGameBoy->Resume();
			LeaveCriticalSection(&csGameBoy);
			return true;
		}
	}
	switch (Settings.SaveBattery)
	{
	case SAVEBATTERY_ALWAYS:
		pGameBoy->SaveBattery(false, false);
		break;
	case SAVEBATTERY_PROMPT:
		if (pGameBoy->SaveBattery(true, false))
		{
			pGameBoy->Resume();
			LeaveCriticalSection(&csGameBoy);
			return true;
		}
	}

#ifdef _DEBUG
	if (pGameBoy->pLinkGameBoy)
	{
		__asm int 3;
		//pGameBoy->pLinkGameBoy->pLinkGameBoy = NULL;
	}
#endif

	//Find the Game Boy to delete in the chain
	for (pGameBoyItem = m_pFirstGameBoy, pGameBoyItem2 = NULL; pGameBoyItem->pGameBoy != pGameBoy; pGameBoyItem2 = pGameBoyItem, pGameBoyItem = pGameBoyItem->pNext);
	//pGameBoyItem2 == Game Boy before pGameBoyItem

	if (pGameBoyItem->pGameBoy == m_pActiveGameBoy)
	{
		m_pActiveGameBoy = NULL;
	}

	if (pGameBoyItem2)
	{
		pGameBoyItem2->pNext = pGameBoyItem->pNext;
	}
	else
	{
		m_pFirstGameBoy = m_pFirstGameBoy->pNext;
	}
	delete pGameBoyItem->pGameBoy;
	delete pGameBoyItem;

	LeaveCriticalSection(&csGameBoy);
	PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
	return false;
}



BOOL CGameBoys::DeleteAll()
{
	EnterCriticalSection(&csGameBoy);

	while (m_pFirstGameBoy)
	{
		if (DeleteGameBoy(m_pFirstGameBoy->pGameBoy))
		{
			LeaveCriticalSection(&csGameBoy);
			return true;
		}
	}

	LeaveCriticalSection(&csGameBoy);
	return false;
}



void CGameBoys::EnableSound()
{
	Settings.SoundEnabled = true;
}



void CGameBoys::CloseSound()
{
	GAMEBOYITEM		*pGameBoyItem;


	Settings.SoundEnabled = false;

	EnterCriticalSection(&csGameBoy);
	for (pGameBoyItem = m_pFirstGameBoy; pGameBoyItem; pGameBoyItem = pGameBoyItem->pNext)
	{
		pGameBoyItem->pGameBoy->CloseSound();
	}
	LeaveCriticalSection(&csGameBoy);
}



CGameBoy *CGameBoys::GetActive(BOOL AddRef)
{
	CGameBoy		*pGameBoy;


	EnterCriticalSection(&csGameBoy);

	if (pGameBoy = m_pActiveGameBoy)
	{
		if (AddRef)
		{
			pGameBoy->AddRef();
		}
	}

	LeaveCriticalSection(&csGameBoy);

	return pGameBoy;
}



CGameBoy *CGameBoys::GetPlayer2(BOOL AddRef)
{
	CGameBoy		*pGameBoy;
	HWND			hWin;


	EnterCriticalSection(&csGameBoy);

	if (pGameBoy = m_pActiveGameBoy)
	{
		hWin = pGameBoy->hGBWnd;
		while (hWin = GetNextWindow(hWin, GW_HWNDNEXT))
		{
			pGameBoy = (CGameBoy *)GetWindowLong(hWin, GWL_USERDATA);
			if (GameBoyExists(pGameBoy))
			{
				if (AddRef)
				{
					pGameBoy->AddRef();
				}
				break;
			}
			pGameBoy = NULL;
		}
	}

	LeaveCriticalSection(&csGameBoy);

	return pGameBoy;
}



LRESULT CGameBoys::WndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;


	if (pGameBoy = (CGameBoy *)GetWindowLong(hWin, GWL_USERDATA))
	{
		switch (uMsg)
		{
		case WM_MDIACTIVATE:
			if (hWin == (HWND)lParam)
			{
				if (m_pActiveGameBoy != pGameBoy)
				{
					m_pActiveGameBoy = pGameBoy;
					MemoryFlags = DisAsmFlags = 0;
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);

					SetRumble(m_pActiveGameBoy);
					SetRumble(GetPlayer2(false));
				}
			}
			break;

		case WM_CLOSE:
			DeleteGameBoy(pGameBoy);
			return 0;
		}

		return pGameBoy->GameBoyWndProc(uMsg, wParam, lParam);
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



void CGameBoys::UpdateKeys(CGameBoy *pGameBoy)
{
	DIJOYSTATE		dijs;
	float			f;
	DWORD			dwPlayerNo, dw;


	pGameBoy->DirectionKeys = 0;
	pGameBoy->Buttons = 0;
	pGameBoy->FastFwd = 0;

	EnterCriticalSection(&csGameBoy);
	if (pGameBoy == m_pActiveGameBoy)
	{
		pGameBoy->AddRef();
		dwPlayerNo = 0;
	}
	else if (pGameBoy == GetPlayer2(true))
	{
		dwPlayerNo = 1;
	}
	else
	{
		LeaveCriticalSection(&csGameBoy);
		pGameBoy->AutoButtonDown = 0;
		pGameBoy->SoundL = pGameBoy->SoundR = 0;
		return;
	}
	LeaveCriticalSection(&csGameBoy);

	if (GetForegroundWindow() == hWnd)
	{
		//Read keyboard
		if (Keys[dwPlayerNo].Up)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Up) & 0x8000)
			{
				pGameBoy->DirectionKeys |= 4;
			}
		}
		if (Keys[dwPlayerNo].Down)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Down) & 0x8000)
			{
				pGameBoy->DirectionKeys |= 8;
			}
		}
		if (Keys[dwPlayerNo].Left)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Left) & 0x8000)
			{
				pGameBoy->DirectionKeys |= 2;
			}
		}
		if (Keys[dwPlayerNo].Right)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Right) & 0x8000)
			{
				pGameBoy->DirectionKeys |= 1;
			}
		}
		if (Keys[dwPlayerNo].A)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].A) & 0x8000)
			{
				pGameBoy->Buttons |= 1;
			}
		}
		if (Keys[dwPlayerNo].B)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].B) & 0x8000)
			{
				pGameBoy->Buttons |= 2;
			}
		}
		if (Keys[dwPlayerNo].Start)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Start) & 0x8000)
			{
				pGameBoy->Buttons |= 8;
			}
		}
		if (Keys[dwPlayerNo].Select)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].Select) & 0x8000)
			{
				pGameBoy->Buttons |= 4;
			}
		}
		if (pGameBoy->AutoButtonDown & 0x80)
		{
			if (AutoFireKeys[dwPlayerNo].Up)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Up) & 0x8000)
				{
					pGameBoy->DirectionKeys |= 4;
				}
			}
			if (AutoFireKeys[dwPlayerNo].Down)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Down) & 0x8000)
				{
					pGameBoy->DirectionKeys |= 8;
				}
			}
			if (AutoFireKeys[dwPlayerNo].Left)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Left) & 0x8000)
				{
					pGameBoy->DirectionKeys |= 2;
				}
			}
			if (AutoFireKeys[dwPlayerNo].Right)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Right) & 0x8000)
				{
					pGameBoy->DirectionKeys |= 1;
				}
			}
			if (AutoFireKeys[dwPlayerNo].A)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].A) & 0x8000)
				{
					pGameBoy->Buttons |= 1;
				}
			}
			if (AutoFireKeys[dwPlayerNo].B)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].B) & 0x8000)
				{
					pGameBoy->Buttons |= 2;
				}
			}
			if (AutoFireKeys[dwPlayerNo].Start)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Start) & 0x8000)
				{
					pGameBoy->Buttons |= 8;
				}
			}
			if (AutoFireKeys[dwPlayerNo].Select)
			{
				if (GetAsyncKeyState(AutoFireKeys[dwPlayerNo].Select) & 0x8000)
				{
					pGameBoy->Buttons |= 4;
				}
			}
		}
		if (Keys[dwPlayerNo].FastForward)
		{
			if (GetAsyncKeyState(Keys[dwPlayerNo].FastForward) & 0x8000)
			{
				pGameBoy->FastFwd = true;
			}
		}

		//Read joystick
		if (lpdidJoysticks[dwPlayerNo])
		{
			if (lpdidJoysticks[dwPlayerNo]->Poll() == DIERR_INPUTLOST)
			{
				lpdidJoysticks[dwPlayerNo]->Acquire();
				lpdidJoysticks[dwPlayerNo]->Poll();
			}
			if (!lpdidJoysticks[dwPlayerNo]->GetDeviceState(sizeof(dijs), &dijs))
			{
				for (dw = 1; dw < 5; dw++)
				{
					if (DirectionEnabled[dwPlayerNo][dw])
					{
						if ((dijs.rgdwPOV[0] & 0xFFFF) != 0xFFFF)
						{
							if (dijs.rgdwPOV[0] >= 4500 && dijs.rgdwPOV[0] <= 13500)
							{
								pGameBoy->DirectionKeys |= 0x01;
							}
							if (dijs.rgdwPOV[0] >= 13500 && dijs.rgdwPOV[0] <= 22500)
							{
								pGameBoy->DirectionKeys |= 0x08;
							}
							if (dijs.rgdwPOV[0] >= 22500 && dijs.rgdwPOV[0] <= 31500)
							{
								pGameBoy->DirectionKeys |= 0x02;
							}
							if (dijs.rgdwPOV[0] >= 31500 || dijs.rgdwPOV[0] <= 4500)
							{
								pGameBoy->DirectionKeys |= 0x04;
							}
						}
					}
				}
				if (DirectionEnabled[dwPlayerNo][0])
				{
					if (dijs.lX < JoyLeftX[dwPlayerNo])
					{
						if (JoyIsAnalog[dwPlayerNo])
						{
							f = (10 * (float)(JoyLeftX[dwPlayerNo] - JoyMinX[dwPlayerNo] - dijs.lX)) / (float)(JoyLeftX[dwPlayerNo] - JoyMinX[dwPlayerNo]);
							if (++pGameBoy->JoyLeft > 10)
							{
								pGameBoy->JoyLeft = 0;
							}
							if (pGameBoy->JoyLeft <= f)
							{
								pGameBoy->DirectionKeys |= 0x02;
							}
						}
						else
						{
							pGameBoy->DirectionKeys |= 0x02;
						}
					}
					if (dijs.lX > JoyRightX[dwPlayerNo])
					{
						if (JoyIsAnalog[dwPlayerNo])
						{
							f = (10 * (float)(dijs.lX - JoyRightX[dwPlayerNo])) / (float)(JoyMaxX[dwPlayerNo] - JoyRightX[dwPlayerNo]);
							if (++pGameBoy->JoyRight > 10)
							{
								pGameBoy->JoyRight = 0;
							}
							if (pGameBoy->JoyRight <= f)
							{
								pGameBoy->DirectionKeys |= 0x01;
							}
						}
						else
						{
							pGameBoy->DirectionKeys |= 0x01;
						}
					}
					if (dijs.lY > JoyUpY[dwPlayerNo])
					{
						if (JoyIsAnalog[dwPlayerNo])
						{
							f = (10 * (float)(dijs.lY - JoyUpY[dwPlayerNo])) / (float)(JoyMaxY[dwPlayerNo] - JoyUpY[dwPlayerNo]);
							if (++pGameBoy->JoyUp > 10)
							{
								pGameBoy->JoyUp = 0;
							}
							if (pGameBoy->JoyUp <= f)
							{
								pGameBoy->DirectionKeys |= 0x08;
							}
						}
						else
						{
							pGameBoy->DirectionKeys |= 0x08;
						}
					}
					if (dijs.lY < JoyDownY[dwPlayerNo])
					{
						if (JoyIsAnalog[dwPlayerNo])
						{
							f = (10 * (float)(JoyDownY[dwPlayerNo] - JoyMinY[dwPlayerNo] - dijs.lY)) / (float)(JoyDownY[dwPlayerNo] - JoyMinY[dwPlayerNo]);
							if (++pGameBoy->JoyDown > 10)
							{
								pGameBoy->JoyDown = 0;
							}
							if (pGameBoy->JoyDown <= f)
							{
								pGameBoy->DirectionKeys |= 0x04;
							}
						}
						else
						{
							pGameBoy->DirectionKeys |= 0x04;
						}
					}
				}
				if (JoyButtons[dwPlayerNo].Up)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Up - 1] & 0x80)
					{
						pGameBoy->DirectionKeys |= 0x04;
					}
				}
				if (JoyButtons[dwPlayerNo].Down)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Down - 1] & 0x80)
					{
						pGameBoy->DirectionKeys |= 0x08;
					}
				}
				if (JoyButtons[dwPlayerNo].Left)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Left - 1] & 0x80)
					{
						pGameBoy->DirectionKeys |= 0x02;
					}
				}
				if (JoyButtons[dwPlayerNo].Right)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Right - 1] & 0x80)
					{
						pGameBoy->DirectionKeys |= 0x01;
					}
				}
				if (JoyButtons[dwPlayerNo].A)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].A - 1] & 0x80)
					{
						pGameBoy->Buttons |= 0x01;
					}
				}
				if (JoyButtons[dwPlayerNo].B)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].B - 1] & 0x80)
					{
						pGameBoy->Buttons |= 0x02;
					}
				}
				if (JoyButtons[dwPlayerNo].Start)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Start - 1] & 0x80)
					{
						pGameBoy->Buttons |= 0x08;
					}
				}
				if (JoyButtons[dwPlayerNo].Select)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].Select - 1] & 0x80)
					{
						pGameBoy->Buttons |= 0x04;
					}
				}
				if (pGameBoy->AutoButtonDown & 0x80)
				{
					if (AutoFireJoyButtons[dwPlayerNo].Up)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Up - 1] & 0x80)
						{
							pGameBoy->DirectionKeys |= 0x04;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].Down)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Down - 1] & 0x80)
						{
							pGameBoy->DirectionKeys |= 0x08;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].Left)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Left - 1] & 0x80)
						{
							pGameBoy->DirectionKeys |= 0x02;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].Right)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Right - 1] & 0x80)
						{
							pGameBoy->DirectionKeys |= 0x01;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].A)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].A - 1] & 0x80)
						{
							pGameBoy->Buttons |= 0x01;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].B)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].B - 1] & 0x80)
						{
							pGameBoy->Buttons |= 0x02;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].Start)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Start - 1] & 0x80)
						{
							pGameBoy->Buttons |= 0x08;
						}
					}
					if (AutoFireJoyButtons[dwPlayerNo].Select)
					{
						if (dijs.rgbButtons[AutoFireJoyButtons[dwPlayerNo].Select - 1] & 0x80)
						{
							pGameBoy->Buttons |= 0x04;
						}
					}
				}
				if (JoyButtons[dwPlayerNo].FastForward)
				{
					if (dijs.rgbButtons[JoyButtons[dwPlayerNo].FastForward - 1] & 0x80)
					{
						pGameBoy->FastFwd = true;
					}
				}
			}
		}
	}

	if (pGameBoy->AutoButtonDown & 0x7F)
	{
		pGameBoy->AutoButtonDown--;
	}
	else
	{
		if (pGameBoy->AutoButtonDown & 0x80)
		{
			pGameBoy->AutoButtonDown = 1;
		}
		else
		{
			pGameBoy->AutoButtonDown = 0x81;
		}
	}
	if (!pGameBoy->FastFwd)
	{
		pGameBoy->SoundL = pGameBoy->SoundR = 0;
	}

	pGameBoy->Release();
}



void CGameBoys::SetRumble(CGameBoy *pGameBoy)
{
	DWORD		dwPlayerNo;


	if (!pGameBoy)
	{
		return;
	}

	EnterCriticalSection(&csGameBoy);
	if (pGameBoy == m_pActiveGameBoy)
	{
		dwPlayerNo = 0;
	}
	else if (pGameBoy == GetPlayer2(false))
	{
		dwPlayerNo = 1;
	}
	else
	{
		LeaveCriticalSection(&csGameBoy);
		return;
	}

	if (lpdieRumble[dwPlayerNo])
	{
		if (pGameBoy->Flags & GB_RUMBLE)
		{
			lpdieRumble[dwPlayerNo]->Start(1, 0);
		}
		else
		{
			lpdieRumble[dwPlayerNo]->Stop();
		}
	}
	LeaveCriticalSection(&csGameBoy);
}

