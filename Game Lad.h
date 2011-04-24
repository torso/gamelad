#ifndef		GAME_LAD_H
#define		GAME_LAD_H



//#define		DEBUGLOG



#define		GAME_LAD_RELEASENO				8
#define		VERSION_MAJOR					1
#define		VERSION_MINOR					6



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



#ifdef		DEBUGLOG

GAME_LAD_CPP	HANDLE		hDebugLogFile EQUALNULL;
GAME_LAD_CPP	DWORD		dwDebugLogBytesWritten;
GAME_LAD_CPP	char		szLog[0x100];

#define		BEGINLOG	\
	if ((hDebugLogFile = CreateFile("C:\\Game Lad.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)	\
	{	\
		hDebugLogFile = NULL;	\
		MessageBox(hMsgParent, "Couldn't create C:\\Game Lad.log", NULL, MB_OK | MB_ICONERROR);	\
	}

#define		ENDLOG	\
	if (hDebugLogFile)	\
	{	\
		CloseHandle(hDebugLogFile);	\
		hDebugLogFile = NULL;	\
	}

#define		LOG(sz)	\
	if (hDebugLogFile)	\
	{	\
		if (!WriteFile(hDebugLogFile, sz, strlen(sz), &dwDebugLogBytesWritten, NULL))	\
		{	\
			CloseHandle(hDebugLogFile);	\
			hDebugLogFile = NULL;	\
			MessageBox(hMsgParent, "Error writing to C:\\Game Lad.log", NULL, MB_OK | MB_ICONERROR);	\
		}	\
		if (hDebugLogFile)	\
		{	\
			if (strlen(sz) != dwDebugLogBytesWritten)	\
			{	\
				CloseHandle(hDebugLogFile);	\
				hDebugLogFile = NULL;	\
				MessageBox(hMsgParent, "Error writing to C:\\Game Lad.log", NULL, MB_OK | MB_ICONERROR);	\
			}	\
		}	\
	}

#define		LOGFUNCTION(sz)	\
	if (hDebugLogFile)	\
	{	\
		SetLastError(ERROR_SUCCESS);	\
		strcpy(szLog, sz);	\
		strcat(szLog, ": ");	\
		LOG(szLog);	\
	}

#define		LOGERROR	\
	if (hDebugLogFile)	\
	{	\
		itoa(GetLastError(), szLog, 16);	\
		strcat(szLog, "\n");	\
		LOG(szLog);	\
	}

#define		LOGSUCCESS	\
	if (hDebugLogFile)	\
	{	\
		LOG("Success\n");	\
	}

#else		//DEBUGLOG

#define		BEGINLOG
#define		ENDLOG
#define		LOG(sz)
#define		LOGFUNCTION(sz)
#define		LOGERROR
#define		LOGSUCCESS

#endif		//DEBUGLOG



#ifndef ListView_SetCheckState
	#define ListView_SetCheckState(hwndLV, i, fCheck) \
		ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck) + 1), LVIS_STATEIMAGEMASK)
#endif



//Messages for main window
enum WM_APP_Messages
{
	WM_APP_DDEOPENFILE = WM_APP,
	WM_APP_REFRESHDEBUG,
	WM_APP_CHANGELANGUAGE,
	WM_APP_INITGFX,
	WM_APP_RELEASEGFX,

	WM_APP_FIRSTFREEMESSAGE
};



enum
{
	AUTOSTART_STOPPED = 0,
	AUTOSTART_DEBUG,
	AUTOSTART_EXECUTE,
};
enum
{
	SAVEBATTERY_NO = 0,
	SAVEBATTERY_ALWAYS,
	SAVEBATTERY_PROMPT,
};
enum
{
	SAVESTATE_NO = 0,
	SAVESTATE_ALWAYS,
	SAVESTATE_PROMPT,
};
enum
{
	FULLSCREEN_FIT = 0,
	FULLSCREEN_ZOOM,
};



struct SETTINGS
{
	BYTE	AutoLoadBattery, AutoLoadState;
	BYTE	SaveBattery, SaveState;
	BYTE	AutoStart;
	BYTE	GBType;
	BYTE	SoundEnabled;
	BYTE	GbFile, GlsFile;
	BYTE	LinkCable/*, NetworkLinkCable*/;

	BOOL	Direct3D;
	GUID	Direct3DDeviceGuid;
	BYTE	FrameSkip, FrameSkipAuto, Zoom, Fullscreen, Fullscreen10_9, FullscreenZoom;
	DWORD	FullscreenX, FullscreenY, FullscreenBpp, FullscreenRefreshRate;

	BYTE	State_UseRomDir, Battery_UseRomDir;
	char	szRomDir[MAX_PATH], szStateDir[MAX_PATH], szBatteryDir[MAX_PATH], szCheatDir[MAX_PATH], szSendFileDir[MAX_PATH];
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
GAME_LAD_CPP	HWND				hWnd, hMsgParent, hClientWnd, hStatusWnd, hNetworkWnd;
GAME_LAD_CPP	HFONT				hFont, hFixedFont;
GAME_LAD_CPP	int					FixedFontWidth, FixedFontHeight;
GAME_LAD_CPP	HMENU				hPopupMenu;
GAME_LAD_CPP	char				NumBuffer[10];
GAME_LAD_CPP	DWORD				DdeInst EQUALNULL;
GAME_LAD_CPP	HSZ					hDdeServiceString, hDdeTopicString;
GAME_LAD_CPP	HANDLE				hStartStopEvent;
GAME_LAD_CPP	LARGE_INTEGER		TimerFrequency;
GAME_LAD_CPP	CRITICAL_SECTION	csSound, csGameBoy, csTerminate, csGraphic, csNetwork;



struct WINDOWSETTINGS
{
	DWORD		Appearance;
	DWORD		x, y;
	DWORD		Width, Height;
	DWORD		Zoom;
};

GAME_LAD_CPP	WINDOWSETTINGS		Main, Registers, DisAsm, Memory, Tiles, TileMap, Palettes, Hardware;
GAME_LAD_CPP	DWORD				StatusBarAppearance;



#define			OPTIONPAGE_KEYS		0
#define			OPTIONPAGE_FILE		1
#define			OPTIONPAGE_GENERAL	2
#define			OPTIONPAGE_GFX		3

GAME_LAD_CPP	DWORD				LastOptionPage EQUALNULL;



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
GAME_LAD_CPP	char				*DwordToHex(DWORD dw, char *psz);
GAME_LAD_CPP	char				NibbleToHex(BYTE b);

GAME_LAD_CPP	char				*LoadString(UINT uID, char *pszBuffer, int nBufferMax);
GAME_LAD_CPP	char				*LoadString(UINT uID, char *pszBuffer, int nBufferMax, char *pszInsert);
#define			String(uID)			LoadString(uID, szBuffer, sizeof(szBuffer))

GAME_LAD_CPP	BOOL				ChangeExtension(char *pszFilename, char *pszExtension);
GAME_LAD_CPP	char				*CopyString(char *pszDest, char *pszSrc, DWORD dwLength);


#undef		GAME_LAD_CPP
#undef		EQUALNULL


#endif	//GAME_LAD_H

