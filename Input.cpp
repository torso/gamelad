#include	<windows.h>

#define		INPUT_CPP
#include	"Game Lad.h"
#include	"Input.h"





BOOL InitInput()
{
	if (!lpdi)
	{
		if (DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&lpdi, NULL))
		{
			return true;
		}

		ZeroMemory(lpdidJoysticks, sizeof(lpdidJoysticks));
	}

	return false;
}



void RestoreInput()
{
	if (lpdi)
	{
		if (lpdidJoysticks[0])
		{
			lpdidJoysticks[0]->Acquire();
		}
		if (lpdidJoysticks[1])
		{
			lpdidJoysticks[1]->Acquire();
		}
	}
}



void CloseInput()
{
	if (lpdi)
	{
		if (lpdidJoysticks[0] == lpdidJoysticks[1])
		{
			lpdidJoysticks[1] = NULL;
		}

		if (lpdidJoysticks[0])
		{
			if (lpdieRumble[0])
			{
				lpdieRumble[0]->Release();
				lpdieRumble[0] = NULL;
			}

			lpdidJoysticks[0]->Unacquire();
			lpdidJoysticks[0]->Release();
			lpdidJoysticks[0] = NULL;
		}
		if (lpdidJoysticks[1])
		{
			if (lpdieRumble[1])
			{
				lpdieRumble[1]->Release();
				lpdieRumble[1] = NULL;
			}

			lpdidJoysticks[1]->Unacquire();
			lpdidJoysticks[1]->Release();
			lpdidJoysticks[1] = NULL;
		}

		lpdi->Release();
		lpdi = NULL;
	}
}



BOOL InitInputDevice(GUID Guid, GUID *Guid2, DWORD dwJoystick)
{
	DIDEVCAPS				didc;
	DIEFFECT				die;
	DIPERIODIC				dip;
	DIPROPDWORD				dipDword;
	DIPROPRANGE				dipRange;
	DWORD					dwAxes[2] = {DIJOFS_X, DIJOFS_Y};
	LONG					lDirection[2] = {1, 0};
	GUID					TempGuid;


	ZeroMemory(&TempGuid, sizeof(TempGuid));
	if (!memcmp(&Guid, &TempGuid, sizeof(GUID)))
	{
		return true;
	}

	if (Guid2)
	{
		if (!memcmp(&Guid, Guid2, sizeof(GUID)))
		{
			if (dwJoystick == 1)
			{
				lpdidJoysticks[1] = lpdidJoysticks[0];
				lpdieRumble[1] = lpdieRumble[0];
				JoyLeftX[1] = JoyLeftX[0];
				JoyRightX[1] = JoyRightX[0];
				JoyMinX[1] = JoyMinX[0];
				JoyMaxX[1] = JoyMaxX[0];
				JoyDownY[1] = JoyDownY[0];
				JoyUpY[1] = JoyUpY[0];
				JoyMinY[1] = JoyMinY[0];
				JoyMaxY[1] = JoyMaxY[0];
			}
			else
			{
				lpdidJoysticks[0] = lpdidJoysticks[1];
				lpdieRumble[0] = lpdieRumble[1];
				JoyLeftX[0] = JoyLeftX[1];
				JoyRightX[0] = JoyRightX[1];
				JoyMinX[0] = JoyMinX[1];
				JoyMaxX[0] = JoyMaxX[1];
				JoyDownY[0] = JoyDownY[1];
				JoyUpY[0] = JoyUpY[1];
				JoyMinY[0] = JoyMinY[1];
				JoyMaxY[0] = JoyMaxY[1];
			}
			return false;
		}
	}

	if (lpdi->CreateDevice(Guid, &lpdidJoysticks[dwJoystick], NULL))
	{
		return true;
	}
	lpdidJoysticks[dwJoystick]->SetDataFormat(&c_dfDIJoystick);
	lpdidJoysticks[dwJoystick]->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);

	dipDword.diph.dwSize = sizeof(dipDword);
	dipDword.diph.dwHeaderSize = sizeof(dipDword.diph);
	dipDword.diph.dwObj = 0;
	dipDword.diph.dwHow = DIPH_DEVICE;
	dipDword.dwData = 0;
	lpdidJoysticks[dwJoystick]->SetProperty(DIPROP_DEADZONE, &dipDword.diph);
	dipRange.diph.dwSize = sizeof(dipRange);
	dipRange.diph.dwHeaderSize = sizeof(dipRange.diph);
	dipRange.diph.dwObj = DIJOFS_X;
	dipRange.diph.dwHow = DIPH_BYOFFSET;
	lpdidJoysticks[dwJoystick]->GetProperty(DIPROP_RANGE, &dipRange.diph);
	if (JoyIsAnalog[dwJoystick])
	{
		JoyLeftX[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 - (dipRange.lMax - dipRange.lMin) / 20;
		JoyMinX[dwJoystick] = dipRange.lMin + (JoyLeftX[dwJoystick] - dipRange.lMin) / 3;
		JoyRightX[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 + (dipRange.lMax - dipRange.lMin) / 20;
		JoyMaxX[dwJoystick] = dipRange.lMax - (dipRange.lMax - JoyRightX[dwJoystick]) / 3;
	}
	else
	{
		JoyMinX[dwJoystick] = dipRange.lMin;
		JoyMaxX[dwJoystick] = dipRange.lMax;
		JoyLeftX[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 - (dipRange.lMax - dipRange.lMin) / 10;
		JoyRightX[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 + (dipRange.lMax - dipRange.lMin) / 10;
	}
	dipRange.diph.dwObj = DIJOFS_Y;
	lpdidJoysticks[dwJoystick]->GetProperty(DIPROP_RANGE, &dipRange.diph);
	if (JoyIsAnalog[dwJoystick])
	{
		JoyDownY[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 - (dipRange.lMax - dipRange.lMin) / 20;
		JoyMinY[dwJoystick] = dipRange.lMin + (JoyDownY[dwJoystick] - dipRange.lMin) / 3;
		JoyUpY[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 + (dipRange.lMax - dipRange.lMin) / 20;
		JoyMaxY[dwJoystick] = dipRange.lMax - (dipRange.lMax - JoyUpY[dwJoystick]) / 3;
	}
	else
	{
		JoyMinY[dwJoystick] = dipRange.lMin;
		JoyMaxY[dwJoystick] = dipRange.lMax;
		JoyDownY[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 - (dipRange.lMax - dipRange.lMin) / 10;
		JoyUpY[dwJoystick] = (dipRange.lMax - dipRange.lMin) / 2 + (dipRange.lMax - dipRange.lMin) / 10;
	}

	didc.dwSize = sizeof(didc);
	lpdidJoysticks[dwJoystick]->GetCapabilities(&didc);

	if (didc.dwFlags & DIDC_FORCEFEEDBACK)
	{
		ZeroMemory(&die, sizeof(die));
		die.dwSize = sizeof(die);
		die.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		die.dwDuration = INFINITE;
		die.dwSamplePeriod = 0;
		die.dwGain = DI_FFNOMINALMAX;
		die.dwTriggerButton = DIEB_NOTRIGGER;
		die.dwTriggerRepeatInterval = 0;
		die.cAxes = 2;
		die.rgdwAxes = dwAxes;
		die.rglDirection = lDirection;
		die.lpEnvelope = NULL;
		die.cbTypeSpecificParams = sizeof(dip);
		die.lpvTypeSpecificParams = &dip;
		dip.dwMagnitude = 4000;
		dip.lOffset = 0;
		dip.dwPhase = 0;
		dip.dwPeriod = 70000;

		/*dipDword.diph.dwSize = sizeof(dipDword);
		dipDword.diph.dwHeaderSize = sizeof(dipDword.diph);
		dipDword.diph.dwObj = 0;
		dipDword.diph.dwHow = DIPH_DEVICE;
		dipDword.dwData = false;
		lpdidJoysticks[dwJoystick]->SetProperty(DIPROP_AUTOCENTER, &dipDword.diph);*/

		lpdidJoysticks[dwJoystick]->Acquire();

		lpdidJoysticks[dwJoystick]->CreateEffect(GUID_Sine, &die, &lpdieRumble[dwJoystick], NULL);
	}
	else
	{
		lpdidJoysticks[dwJoystick]->Acquire();
	}

	/*if (lpdieRumble[dwJoystick])
	{
		lpdieRumble[dwJoystick]->Start(1, 0);
	}*/

	return false;
}

