#include	<dinput.h>

#ifdef		INPUT_CPP
#define		EQUALNULL	= NULL
#else
#define		INPUT_CPP	extern
#define		EQUALNULL
#endif



INPUT_CPP	LPDIRECTINPUT8			lpdi EQUALNULL;
INPUT_CPP	LPDIRECTINPUTDEVICE8	lpdidJoysticks[2];
INPUT_CPP	LPDIRECTINPUTEFFECT		lpdieRumble[2];
INPUT_CPP	long					JoyLeftX[2], JoyRightX[2], JoyDownY[2], JoyUpY[2];
INPUT_CPP	long					JoyMinX[2], JoyMaxX[2], JoyMinY[2], JoyMaxY[2];
INPUT_CPP	BOOL					JoyIsAnalog[2];



INPUT_CPP	BOOL	InitInput();
INPUT_CPP	void	RestoreInput();
INPUT_CPP	void	CloseInput();

INPUT_CPP	BOOL	InitInputDevice(GUID Guid, GUID *Guid2, DWORD dwJoystick);

