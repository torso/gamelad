#include	"Game Boy.h"



#ifndef		GAME_LAD_CPP

#define		GAME_LAD_CPP	extern
#define		EQUALNULL

#else //GAME_LAD_CPP

#define		EQUALNULL		= NULL

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

//
//#define		WM_UPDATEKEYS			WM_APP

//Messages for Disassembly window
//#define		WM_APP_STEPINTO			WM_APP
//#define		WM_APP_STEPOVER			(WM_APP + 1)
//#define		WM_APP_STEPOUT			(WM_APP + 2)
//#define		WM_APP_RUNTOCURSOR		(WM_APP + 3)
//#define		WM_APP_TOGGLEBREAKPOINT	(WM_APP + 4)
//#define		WM_APP_SETNEXTSTATEMENT	(WM_APP + 5)



struct GameBoy
{
	CGameBoy	*pGameBoy;
	GameBoy		*pNext;
};

class CGameBoyList
{
private:
	GameBoy		*FirstGameBoy;
	CGameBoy	*ActiveGameBoy;

public:
	CGameBoyList();

	CGameBoy	*NewGameBoy(char *pszROMFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart);
	BOOL		DeleteGameBoy(CGameBoy *pCGameBoy);
	BOOL		DeleteAll();
	LPARAM		WndProc(CGameBoy *pGameBoy, HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CGameBoy	*GetActive();
} GAME_LAD_CPP GameBoyList;



#define		AUTOSTART_DEBUG		1
#define		AUTOSTART_EXECUTE	2



struct _Settings
{
	//BOOL	AutoLoadDebug;
	BOOL	AutoLoadBattery;
	BYTE	AutoStart;
	BYTE	GBType;
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
GAME_LAD_CPP	HWND				hWnd, hClientWnd;
GAME_LAD_CPP	HFONT				hFont, hFixedFont;
GAME_LAD_CPP	int					FixedFontWidth, FixedFontHeight;
GAME_LAD_CPP	HMENU				hPopupMenu;
GAME_LAD_CPP	char				NumBuffer[10];
GAME_LAD_CPP	DWORD				DdeInst EQUALNULL;
GAME_LAD_CPP	HSZ					hDdeServiceString, hDdeTopicString;
GAME_LAD_CPP	CRITICAL_SECTION	cs;

GAME_LAD_CPP	LARGE_INTEGER		TimerFrequency;

struct WINDOWSETTINGS
{
	DWORD		Appearance;
	DWORD		x, y;
	DWORD		Width, Height;
	DWORD		Zoom;
};

GAME_LAD_CPP	WINDOWSETTINGS		Main, Registers, DisAsm, Memory, Tiles, TileMap, Palettes, Hardware;



GAME_LAD_CPP	void				DisplayErrorMessage(HWND hWin);
GAME_LAD_CPP	BOOL	HexToNum(char *pc);



#undef		GAME_LAD_CPP
#undef		EQUALNULL

