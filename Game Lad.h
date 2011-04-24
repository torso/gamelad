#include	"Game Boy.h"



#ifndef		GAME_LAD_CPP

#define		GAME_LAD_CPP	extern
#define		EQUALNULL

extern		char			OutOfMemoryMsg[];

#else //GAME_LAD_CPP

#define		EQUALNULL		= NULL

			char			OutOfMemoryMsg[] = "Out of memory.";

#endif //GAME_LAD_CPP



#ifdef _DEBUG

	#define Error(Text)											\
			OutputDebugString(__FILE__ "(");					\
			OutputDebugString(ultoa(__LINE__, NumBuffer, 10));	\
			OutputDebugString(") : ");							\
			OutputDebugString(Text);							\
			OutputDebugString("\n");

#else //_DEBUG

	#define Error(Text)

#endif //_DEBUG



//Messages for main window
#define		WM_APP_DDEOPENFILE		WM_APP
#define		WM_APP_REFRESHDEBUG		(WM_APP + 1)



#define		AUTOSTART_DEBUG		1
#define		AUTOSTART_EXECUTE	2

#define		SAVEBATTERY_NO		0
#define		SAVEBATTERY_ALWAYS	1
#define		SAVEBATTERY_PROMPT	2

#define		SAVESTATE_NO		0
#define		SAVESTATE_ALWAYS	1
#define		SAVESTATE_PROMPT	2



struct SETTINGS
{
	BYTE	AutoLoadBattery, AutoLoadState;
	BYTE	SaveBattery, SaveState;
	BYTE	AutoStart;
	BYTE	GBType;
	BYTE	FrameSkip;
	BYTE	SoundEnabled;
	BYTE	GbFile, GlsFile;
} GAME_LAD_CPP Settings;



struct KEYS
{
	DWORD	Up, Down;
	DWORD	Left, Right;
	DWORD	A, B;
	DWORD	Start, Select;
	DWORD	FastForward;
} extern Keys;



GAME_LAD_CPP	HINSTANCE			hInstance;
#define			hStringInstance		hInstance
GAME_LAD_CPP	HWND				hWnd, hClientWnd, hStatusWnd;
GAME_LAD_CPP	HFONT				hFont, hFixedFont;
GAME_LAD_CPP	int					FixedFontWidth, FixedFontHeight;
GAME_LAD_CPP	HMENU				hPopupMenu;
GAME_LAD_CPP	char				NumBuffer[10];
GAME_LAD_CPP	DWORD				DdeInst EQUALNULL;
GAME_LAD_CPP	HSZ					hDdeServiceString, hDdeTopicString;
GAME_LAD_CPP	HANDLE				hStartStopEvent;
GAME_LAD_CPP	LARGE_INTEGER		TimerFrequency;
GAME_LAD_CPP	CRITICAL_SECTION	csSound;



struct WINDOWSETTINGS
{
	DWORD		Appearance;
	DWORD		x, y;
	DWORD		Width, Height;
	DWORD		Zoom;
};

GAME_LAD_CPP	WINDOWSETTINGS		Main, Registers, DisAsm, Memory, Tiles, TileMap, Palettes, Hardware;
GAME_LAD_CPP	DWORD				StatusBarAppearance;



enum
{
	SF_MESSAGE,
	SF_READY,
	SF_CLEAR,
	SF_F4PRESSED
};

GAME_LAD_CPP	void				SetStatus(char *szStatusText, DWORD dwStatus);

GAME_LAD_CPP	void				DisplayErrorMessage(HWND hWin);
GAME_LAD_CPP	BOOL				HexToNum(char *pc);


#undef		GAME_LAD_CPP
#undef		EQUALNULL

