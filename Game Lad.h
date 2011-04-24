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



#ifndef ListView_SetCheckState
	#define ListView_SetCheckState(hwndLV, i, fCheck) \
		ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif



//Messages for main window
enum WM_APP_Messages
{
	WM_APP_DDEOPENFILE = WM_APP,
	WM_APP_REFRESHDEBUG,
	WM_APP_CHANGELANGUAGE,

	WM_APP_FIRSTFREEMESSAGE
};



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
	BYTE	LinkCable;
} GAME_LAD_CPP Settings;



struct KEYS
{
	DWORD	Up, Down;
	DWORD	Left, Right;
	DWORD	A, B;
	DWORD	Start, Select;
	DWORD	FastForward;
} extern Keys[2], AutoFireKeys[2], JoyButtons[2], AutoFireJoyButtons[2];



GAME_LAD_CPP	HINSTANCE			hInstance;
GAME_LAD_CPP	HWND				hWnd, hMsgParent, hClientWnd, hStatusWnd;
GAME_LAD_CPP	HFONT				hFont, hFixedFont;
GAME_LAD_CPP	int					FixedFontWidth, FixedFontHeight;
GAME_LAD_CPP	HMENU				hPopupMenu;
GAME_LAD_CPP	char				NumBuffer[10];
GAME_LAD_CPP	DWORD				DdeInst EQUALNULL;
GAME_LAD_CPP	HSZ					hDdeServiceString, hDdeTopicString;
GAME_LAD_CPP	HANDLE				hStartStopEvent;
GAME_LAD_CPP	LARGE_INTEGER		TimerFrequency;
GAME_LAD_CPP	CRITICAL_SECTION	csSound, csGameBoy;



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

GAME_LAD_CPP	void				DisplayErrorMessage();
GAME_LAD_CPP	void				DisplayErrorMessage(DWORD dwErrCode);
GAME_LAD_CPP	BOOL				HexToNum(char *pc);

GAME_LAD_CPP	char				*LoadString(UINT uID, char *pszBuffer, int nBufferMax);
GAME_LAD_CPP	char				*LoadString(UINT uID, char *pszBuffer, int nBufferMax, char *pszInsert);
#define			String(uID)			LoadString(uID, szBuffer, sizeof(szBuffer))

GAME_LAD_CPP	BOOL				ChangeExtension(char *pszFilename, char *pszExtension);
GAME_LAD_CPP	char				*CopyString(char *pszDest, char *pszSrc, DWORD dwLength);


#undef		GAME_LAD_CPP
#undef		EQUALNULL

