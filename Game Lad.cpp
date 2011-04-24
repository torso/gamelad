#include	<windows.h>
#include	<shlobj.h>
#include	<afxres.h>
#include	"resource.h"

#define		GAME_LAD_CPP
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CGameBoys.h"
#include	"CCheats.h"
#include	"Emulation.h"
#include	"Debugger.h"
#include	"Error.h"
#include	"Input.h"



#define		GAME_LAD_RELEASENO				5
#define		VERSION_MAJOR					1
#define		VERSION_MINOR					31

#define		REQUIRED_DX_DLL_RELEASENO		0



#define		MENU_WINDOW		5



CList		*pJoySticks = NULL;
CList		*pKeySchemes = NULL;

struct JOYSTICKINFO
{
	GUID	Guid;

	/*char	AxisX[MAX_PATH];
	char	AxisY[MAX_PATH];
	char	AxisZ[MAX_PATH];
	char	AxisRX[MAX_PATH];
	char	AxisRY[MAX_PATH];
	char	AxisRZ[MAX_PATH];
	char	AxisSlider1[MAX_PATH];
	char	AxisSlider2[MAX_PATH];
	char	AxisPOV1[MAX_PATH];
	char	AxisPOV2[MAX_PATH];
	char	AxisPOV3[MAX_PATH];
	char	AxisPOV4[MAX_PATH];*/

	DWORD	dwButtons;
	char	Buttons[32][MAX_PATH];
};



struct KEYSCHEME
{
	char	szName[0x100];
	GUID	JoyGuid;
	KEYS	Keyboard[2];
	KEYS	Joystick[2];
	BOOL	JoyIsAnalog;

	BYTE	BuiltIn;
};



KEYS		Keys[2] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'X', 'Z', VK_RETURN, VK_SHIFT, VK_TAB,
							NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
KEYS		AutoFireKeys[2] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
							NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
KEYS		JoyButtons[2] = {0, 0, 0, 0, 1, 2, 3, 4, 0,
							0, 0, 0, 0, 1, 2, 3, 4, 0};
KEYS		AutoFireJoyButtons[2] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
							0, 0, 0, 0, 0, 0, 0, 0, 0};

GUID		JoyGuid[2];

char		szKeyScheme[2][0x100];



CGameBoy	*F4Pressed = NULL;

struct MENUHELPSTRINGS
{
	UINT	uiCommand, uiString;
} MenuHelpStrings[] = {
	ID_FILE_OPEN,					IDS_FILE_OPEN,
	ID_FILE_CLOSE,					IDS_FILE_CLOSE,
	ID_FILE_SAVESTATE,				IDS_FILE_SAVESTATE,
	ID_FILE_LOADSTATE,				IDS_FILE_LOADSTATE,
	ID_FILE_SAVESTATEAS,			IDS_FILE_SAVESTATEAS,
	ID_FILE_LOADSTATEAS,			IDS_FILE_LOADSTATEAS,
	ID_FILE_STATESLOT_0,			IDS_FILE_STATESLOT_0,
	ID_FILE_STATESLOT_1,			IDS_FILE_STATESLOT_1,
	ID_FILE_STATESLOT_2,			IDS_FILE_STATESLOT_2,
	ID_FILE_STATESLOT_3,			IDS_FILE_STATESLOT_3,
	ID_FILE_STATESLOT_4,			IDS_FILE_STATESLOT_4,
	ID_FILE_STATESLOT_5,			IDS_FILE_STATESLOT_5,
	ID_FILE_STATESLOT_6,			IDS_FILE_STATESLOT_6,
	ID_FILE_STATESLOT_7,			IDS_FILE_STATESLOT_7,
	ID_FILE_STATESLOT_8,			IDS_FILE_STATESLOT_8,
	ID_FILE_STATESLOT_9,			IDS_FILE_STATESLOT_9,
	ID_FILE_SAVEBATTERY,			IDS_FILE_SAVEBATTERY,
	ID_FILE_LOADBATTERY,			IDS_FILE_LOADBATTERY,
	ID_FILE_SAVEBATTERYAS,			IDS_FILE_SAVEBATTERYAS,
	ID_FILE_CLEARBATTERY,			IDS_FILE_CLEARBATTERY,
	ID_FILE_SAVESNAPSHOTAS,			IDS_FILE_SAVESNAPSHOTAS,
	ID_FILE_SAVEVIDEOAS,			IDS_FILE_SAVEVIDEOAS,
	ID_FILE_MERGECHEATS,			IDS_FILE_MERGECHEATS,
	ID_FILE_EXIT,					IDS_FILE_EXIT,
	ID_EDIT_GOTO,					IDS_EDIT_GOTO,
	ID_VIEW_DISASSEMBLY,			IDS_VIEW_DISASSEMBLY,
	ID_VIEW_MEMORY,					IDS_VIEW_MEMORY,
	ID_VIEW_REGISTERS,				IDS_VIEW_REGISTERS,
	ID_VIEW_HARDWARE,				IDS_VIEW_HARDWARE,
	ID_VIEW_PALETTES,				IDS_VIEW_PALETTES,
	ID_VIEW_TILEMAP,				IDS_VIEW_TILEMAP,
	ID_VIEW_TILES,					IDS_VIEW_TILES,
	ID_VIEW_STATUSBAR,				IDS_VIEW_STATUSBAR,
	ID_VIEW_ZOOM_100,				IDS_VIEW_ZOOM_100,
	ID_VIEW_ZOOM_200,				IDS_VIEW_ZOOM_200,
	ID_VIEW_ZOOM_300,				IDS_VIEW_ZOOM_300,
	ID_VIEW_ZOOM_400,				IDS_VIEW_ZOOM_400,
	ID_VIEW_FRAMESKIP_0,			IDS_VIEW_FRAMESKIP_0,
	ID_VIEW_FRAMESKIP_1,			IDS_VIEW_FRAMESKIP_1,
	ID_VIEW_FRAMESKIP_2,			IDS_VIEW_FRAMESKIP_2,
	ID_VIEW_FRAMESKIP_3,			IDS_VIEW_FRAMESKIP_3,
	ID_VIEW_FRAMESKIP_4,			IDS_VIEW_FRAMESKIP_4,
	ID_VIEW_FRAMESKIP_5,			IDS_VIEW_FRAMESKIP_5,
	ID_VIEW_FRAMESKIP_6,			IDS_VIEW_FRAMESKIP_6,
	ID_VIEW_FRAMESKIP_7,			IDS_VIEW_FRAMESKIP_7,
	ID_VIEW_FRAMESKIP_8,			IDS_VIEW_FRAMESKIP_8,
	ID_VIEW_FRAMESKIP_9,			IDS_VIEW_FRAMESKIP_9,
	ID_MEMORY_ROM,					IDS_MEMORY_ROM,
	ID_MEMORY_VBK_BANK0,			IDS_MEMORY_VBK_BANK0,
	ID_MEMORY_VBK_BANK1,			IDS_MEMORY_VBK_BANK1,
	ID_MEMORY_RAM_BANK0,			IDS_MEMORY_RAM_BANK0,
	ID_MEMORY_RAM_BANK1,			IDS_MEMORY_RAM_BANK1,
	ID_MEMORY_RAM_BANK2,			IDS_MEMORY_RAM_BANK2,
	ID_MEMORY_RAM_BANK3,			IDS_MEMORY_RAM_BANK3,
	ID_MEMORY_RAM_BANK4,			IDS_MEMORY_RAM_BANK4,
	ID_MEMORY_RAM_BANK5,			IDS_MEMORY_RAM_BANK5,
	ID_MEMORY_RAM_BANK6,			IDS_MEMORY_RAM_BANK6,
	ID_MEMORY_RAM_BANK7,			IDS_MEMORY_RAM_BANK7,
	ID_MEMORY_RAM_BANK8,			IDS_MEMORY_RAM_BANK8,
	ID_MEMORY_RAM_BANK9,			IDS_MEMORY_RAM_BANK9,
	ID_MEMORY_RAM_BANK10,			IDS_MEMORY_RAM_BANK10,
	ID_MEMORY_RAM_BANK11,			IDS_MEMORY_RAM_BANK11,
	ID_MEMORY_RAM_BANK12,			IDS_MEMORY_RAM_BANK12,
	ID_MEMORY_RAM_BANK13,			IDS_MEMORY_RAM_BANK13,
	ID_MEMORY_RAM_BANK14,			IDS_MEMORY_RAM_BANK14,
	ID_MEMORY_RAM_BANK15,			IDS_MEMORY_RAM_BANK15,
	ID_MEMORY_SVBK_BANK1,			IDS_MEMORY_SVBK_BANK1,
	ID_MEMORY_SVBK_BANK2,			IDS_MEMORY_SVBK_BANK2,
	ID_MEMORY_SVBK_BANK3,			IDS_MEMORY_SVBK_BANK3,
	ID_MEMORY_SVBK_BANK4,			IDS_MEMORY_SVBK_BANK4,
	ID_MEMORY_SVBK_BANK5,			IDS_MEMORY_SVBK_BANK5,
	ID_MEMORY_SVBK_BANK6,			IDS_MEMORY_SVBK_BANK6,
	ID_MEMORY_SVBK_BANK7,			IDS_MEMORY_SVBK_BANK7,
	ID_EMULATION_STARTDEBUG,		IDS_EMULATION_STARTDEBUG,
	ID_EMULATION_EXECUTE,			IDS_EMULATION_EXECUTE,
	ID_EMULATION_STOP,				IDS_EMULATION_STOP,
	ID_EMULATION_STEPINTO,			IDS_EMULATION_STEPINTO,
	ID_EMULATION_STEPOVER,			IDS_EMULATION_STEPOVER,
	ID_EMULATION_STEPOUT,			IDS_EMULATION_STEPOUT,
	ID_EMULATION_RUNTOCURSOR,		IDS_EMULATION_RUNTOCURSOR,
	ID_EMULATION_SETNEXTSTATEMENT,	IDS_EMULATION_SETNEXTSTATEMENT,
	ID_EMULATION_TOGGLEBREAKPOINT,	IDS_EMULATION_TOGGLEBREAKPOINT,
	ID_EMULATION_RESET,				IDS_EMULATION_RESET,
	ID_TOOLS_OPTIONS,				IDS_TOOLS_OPTIONS,
	ID_TOOLS_LINKCABLE,				IDS_TOOLS_LINKCABLE,
	ID_TOOLS_SOUNDENABLED,			IDS_TOOLS_SOUNDENABLED,
	ID_TOOLS_CHEAT,					IDS_TOOLS_CHEAT,
	ID_WINDOW_NEXT,					IDS_WINDOW_NEXT,
	ID_WINDOW_PREVIOUS,				IDS_WINDOW_PREVIOUS,
	ID_WINDOW_CASCADE,				IDS_WINDOW_CASCADE,
	ID_WINDOW_TILEHORIZONTALLY,		IDS_WINDOW_TILEHORIZONTALLY,
	ID_WINDOW_TILEVERTICALLY,		IDS_WINDOW_TILEVERTICALLY,
	ID_HELP_ABOUT,					IDS_HELP_ABOUT,
	0, 0};



/*
	Displays a message box with information about the last error encoutered.
*/
void DisplayErrorMessage()
{
	void	*lpMsgBuf;


	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hMsgParent, (LPCTSTR)lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
	LocalFree(lpMsgBuf);
}



void DisplayErrorMessage(DWORD dwErrCode)
{
	void	*lpMsgBuf;


	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hMsgParent, (LPCTSTR)lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
	LocalFree(lpMsgBuf);
}



void FatalError(DWORD ErrorNo, char *pszText)
{
	if (ErrorNo == FATAL_ERROR_OUTOFMEMORY)
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
	}
}



BOOL HexToNum(char *pc)
{
	if (*pc < '0')
	{
		return true;
	}
	if (*pc <= '9')
	{
		*pc -= '0';
	}
	else if (*pc < 'A')
	{
		return true;
	}
	else if (*pc <= 'F')
	{
		*pc = *pc - 'A' + 10;
	}
	else if (*pc < 'a')
	{
		return true;
	}
	else if (*pc <= 'f')
	{
		*pc = *pc - 'a' + 10;
	}
	else
	{
		return true;
	}

	return false;
}



BOOL ChangeExtension(char *pszFilename, char *pszExtension)
{
	if (strrchr(pszFilename, '.') > strrchr(pszFilename, '\\'))
	{
		*strrchr(pszFilename, '.') = 0;
	}
	if (strlen(pszFilename) + strlen(pszExtension) + 1 >= MAX_PATH)
	{
		return true;
	}
	strcat(pszFilename, pszExtension);

	return false;
}



HINSTANCE		hStringInstance = NULL;
char			szStringFilename[MAX_PATH] = "";

char *LoadString(UINT uID, char *pszBuffer, int nBufferMax)
{
	if (!hStringInstance)
	{
		LoadString(hInstance, uID, pszBuffer, nBufferMax);
	}
	else
	{
		if (!LoadString(hStringInstance, uID, pszBuffer, nBufferMax))
		{
			LoadString(hInstance, uID, pszBuffer, nBufferMax);
		}
	}

	return pszBuffer;
}



char *LoadString(UINT uID, char *pszBuffer, int nBufferMax, char *pszInsert)
{
	char		szBuffer2[0x100];


	LoadString(uID, pszBuffer, nBufferMax);
	if (!strchr(pszBuffer, '$'))
	{
		LoadString(hInstance, uID, pszBuffer, nBufferMax);
	}

	strcpy(szBuffer2, strchr(pszBuffer, '$') + 1);
	*strchr(pszBuffer, '$') = '\0';
	strcat(pszBuffer, pszInsert);
	strcat(pszBuffer, szBuffer2);

	return pszBuffer;
}



char *CopyString(char *pszDest, char *pszSrc, DWORD dwLength)
{
	if (strlen(pszSrc) >= dwLength)
	{
		pszDest[0] = '\0';
		strncpy(pszDest, pszSrc, dwLength);
	}
	else
	{
		strcpy(pszDest, pszSrc);
	}

	return pszDest;
}



#define		MENUITEM(uID, uString)				\
	mii.wID = uID;								\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hSubMenu, 0, true, &mii);

#define		MENUITEM2(uID, uString)				\
	mii.wID = uID;								\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hSubMenu2, 0, true, &mii);

#define		MENUITEM3(uID, uString)				\
	mii.wID = uID;								\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hSubMenu3, 0, true, &mii);

#define		SEPARATOR()							\
	mii.fMask = MIIM_TYPE;						\
	mii.fType = MFT_SEPARATOR;					\
	InsertMenuItem(hSubMenu, 0, true, &mii);	\
	mii.fMask = MIIM_ID | MIIM_TYPE;			\
	mii.fType = MFT_STRING;

#define		SUBMENU(uString)					\
	mii.fMask = MIIM_SUBMENU | MIIM_TYPE;		\
	mii.fType = MFT_STRING;						\
	mii.hSubMenu = hSubMenu;					\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hMenu, 0, true, &mii);		\
	mii.fMask = MIIM_ID | MIIM_TYPE;			\
	mii.fType = MFT_STRING;

#define		SUBMENU2(uString)					\
	mii.fMask = MIIM_SUBMENU | MIIM_TYPE;		\
	mii.fType = MFT_STRING;						\
	mii.hSubMenu = hSubMenu2;					\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hSubMenu, 0, true, &mii);	\
	mii.fMask = MIIM_ID | MIIM_TYPE;			\
	mii.fType = MFT_STRING;

#define		SUBMENU3(uString)					\
	mii.fMask = MIIM_SUBMENU | MIIM_TYPE;		\
	mii.fType = MFT_STRING;						\
	mii.hSubMenu = hSubMenu3;					\
	mii.dwTypeData = String(uString);			\
	InsertMenuItem(hSubMenu2, 0, true, &mii);	\
	mii.fMask = MIIM_ID | MIIM_TYPE;			\
	mii.fType = MFT_STRING;

HMENU CreateMenus(HMENU hMainMenu, HMENU hPopupMenu)
{
	HMENU				hMenu, hSubMenu, hSubMenu2, hSubMenu3;
	MENUITEMINFO		mii;
	char				szBuffer[0x100];


	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_TYPE;
	mii.fType = MFT_STRING;


	hMenu = hMainMenu;

	//Help menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_HELP_ABOUT, IDS_HELP_ABOUT_MENU);
	SUBMENU(IDS_HELP_MENU);

	//Window menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_WINDOW_TILEVERTICALLY, IDS_WINDOW_TILEVERTICALLY_MENU);
	MENUITEM(ID_WINDOW_TILEHORIZONTALLY, IDS_WINDOW_TILEHORIZONTALLY_MENU);
	MENUITEM(ID_WINDOW_CASCADE, IDS_WINDOW_CASCADE_MENU);
	SEPARATOR();
	MENUITEM(ID_WINDOW_PREVIOUS, IDS_WINDOW_PREVIOUS_MENU);
	MENUITEM(ID_WINDOW_NEXT, IDS_WINDOW_NEXT_MENU);
	SUBMENU(IDS_WINDOW_MENU);

	//Tools menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_TOOLS_CHEAT, IDS_TOOLS_CHEAT_MENU);
	MENUITEM(ID_TOOLS_SOUNDENABLED, IDS_TOOLS_SOUNDENABLED_MENU);
	MENUITEM(ID_TOOLS_LINKCABLE, IDS_TOOLS_LINKCABLE_MENU);
	MENUITEM(ID_TOOLS_OPTIONS, IDS_TOOLS_OPTIONS_MENU);
	SUBMENU(IDS_TOOLS_MENU);

	//Emulation menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_EMULATION_RESET, IDS_EMULATION_RESET_MENU);
	SEPARATOR();
	MENUITEM(ID_EMULATION_TOGGLEBREAKPOINT, IDS_EMULATION_TOGGLEBREAKPOINT_MENU);
	MENUITEM(ID_EMULATION_SETNEXTSTATEMENT, IDS_EMULATION_SETNEXTSTATEMENT_MENU);
	MENUITEM(ID_EMULATION_RUNTOCURSOR, IDS_EMULATION_RUNTOCURSOR_MENU);
	MENUITEM(ID_EMULATION_STEPOUT, IDS_EMULATION_STEPOUT_MENU);
	MENUITEM(ID_EMULATION_STEPOVER, IDS_EMULATION_STEPOVER_MENU);
	MENUITEM(ID_EMULATION_STEPINTO, IDS_EMULATION_STEPINTO_MENU);
	SEPARATOR();
	MENUITEM(ID_EMULATION_STOP, IDS_EMULATION_STOP_MENU);
	MENUITEM(ID_EMULATION_EXECUTE, IDS_EMULATION_EXECUTE_MENU);
	MENUITEM(ID_EMULATION_STARTDEBUG, IDS_EMULATION_STARTDEBUG_MENU);
	SUBMENU(IDS_EMULATION_MENU);

	//View menu
	hSubMenu = CreateMenu();
	hSubMenu2 = CreateMenu();
	hSubMenu3 = CreateMenu();
	MENUITEM3(ID_MEMORY_SVBK_BANK7, IDS_VIEW_BANK_SVBK_7_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK6, IDS_VIEW_BANK_SVBK_6_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK5, IDS_VIEW_BANK_SVBK_5_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK4, IDS_VIEW_BANK_SVBK_4_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK3, IDS_VIEW_BANK_SVBK_3_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK2, IDS_VIEW_BANK_SVBK_2_MENU);
	MENUITEM3(ID_MEMORY_SVBK_BANK1, IDS_VIEW_BANK_SVBK_1_MENU);
	SUBMENU3(IDS_VIEW_BANK_SVBK_MENU);
	hSubMenu3 = CreateMenu();
	MENUITEM3(ID_MEMORY_RAM_BANK1, IDS_VIEW_BANK_RAM_1_MENU);
	MENUITEM3(ID_MEMORY_RAM_BANK0, IDS_VIEW_BANK_RAM_0_MENU);
	SUBMENU3(IDS_VIEW_BANK_RAM_MENU);
	hSubMenu3 = CreateMenu();
	MENUITEM3(ID_MEMORY_VBK_BANK1, IDS_VIEW_BANK_VBK_1_MENU);
	MENUITEM3(ID_MEMORY_VBK_BANK0, IDS_VIEW_BANK_VBK_0_MENU);
	SUBMENU3(IDS_VIEW_BANK_VBK_MENU);
	MENUITEM2(ID_MEMORY_ROM, IDS_VIEW_BANK_ROM_MENU);
	SUBMENU2(IDS_VIEW_BANK_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_VIEW_FRAMESKIP_9, IDS_VIEW_FRAMESKIP_9_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_8, IDS_VIEW_FRAMESKIP_8_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_7, IDS_VIEW_FRAMESKIP_7_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_6, IDS_VIEW_FRAMESKIP_6_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_5, IDS_VIEW_FRAMESKIP_5_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_4, IDS_VIEW_FRAMESKIP_4_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_3, IDS_VIEW_FRAMESKIP_3_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_2, IDS_VIEW_FRAMESKIP_2_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_1, IDS_VIEW_FRAMESKIP_1_MENU);
	MENUITEM2(ID_VIEW_FRAMESKIP_0, IDS_VIEW_FRAMESKIP_0_MENU);
	SUBMENU2(IDS_VIEW_FRAMESKIP_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_VIEW_ZOOM_400, IDS_VIEW_ZOOM_400_MENU);
	MENUITEM2(ID_VIEW_ZOOM_300, IDS_VIEW_ZOOM_300_MENU);
	MENUITEM2(ID_VIEW_ZOOM_200, IDS_VIEW_ZOOM_200_MENU);
	MENUITEM2(ID_VIEW_ZOOM_100, IDS_VIEW_ZOOM_100_MENU);
	SUBMENU2(IDS_VIEW_ZOOM_MENU);
	MENUITEM(ID_VIEW_STATUSBAR, IDS_VIEW_STATUSBAR_MENU);
	SEPARATOR();
	MENUITEM(ID_VIEW_TILES, IDS_VIEW_TILES_MENU);
	MENUITEM(ID_VIEW_TILEMAP, IDS_VIEW_TILEMAP_MENU);
	MENUITEM(ID_VIEW_PALETTES, IDS_VIEW_PALETTES_MENU);
	SEPARATOR();
	MENUITEM(ID_VIEW_HARDWARE, IDS_VIEW_HARDWARE_MENU);
	MENUITEM(ID_VIEW_REGISTERS, IDS_VIEW_REGISTERS_MENU);
	MENUITEM(ID_VIEW_MEMORY, IDS_VIEW_MEMORY_MENU);
	MENUITEM(ID_VIEW_DISASSEMBLY, IDS_VIEW_DISASSEMBLY_MENU);
	SUBMENU(IDS_VIEW_MENU);

	//Edit menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_EDIT_GOTO, IDS_EDIT_GOTO_MENU);
	SUBMENU(IDS_EDIT_MENU);

	//File menu
	hSubMenu = CreateMenu();
	MENUITEM(ID_FILE_EXIT, IDS_FILE_EXIT_MENU);
	SEPARATOR();
	MENUITEM(ID_FILE_MERGECHEATS, IDS_FILE_MERGECHEATS_MENU);
	SEPARATOR();
	MENUITEM(ID_FILE_SAVEVIDEOAS, IDS_FILE_SAVEVIDEOAS_MENU);
	MENUITEM(ID_FILE_SAVESNAPSHOTAS, IDS_FILE_SAVESNAPSHOTAS_MENU);
	SEPARATOR();
	MENUITEM(ID_FILE_CLEARBATTERY, IDS_FILE_CLEARBATTERY_MENU);
	MENUITEM(ID_FILE_SAVEBATTERYAS, IDS_FILE_SAVEBATTERYAS_MENU);
	MENUITEM(ID_FILE_LOADBATTERY, IDS_FILE_LOADBATTERY_MENU);
	MENUITEM(ID_FILE_SAVEBATTERY, IDS_FILE_SAVEBATTERY_MENU);
	SEPARATOR();
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_FILE_STATESLOT_9, IDS_FILE_STATESLOT_9_MENU);
	MENUITEM2(ID_FILE_STATESLOT_8, IDS_FILE_STATESLOT_8_MENU);
	MENUITEM2(ID_FILE_STATESLOT_7, IDS_FILE_STATESLOT_7_MENU);
	MENUITEM2(ID_FILE_STATESLOT_6, IDS_FILE_STATESLOT_6_MENU);
	MENUITEM2(ID_FILE_STATESLOT_5, IDS_FILE_STATESLOT_5_MENU);
	MENUITEM2(ID_FILE_STATESLOT_4, IDS_FILE_STATESLOT_4_MENU);
	MENUITEM2(ID_FILE_STATESLOT_3, IDS_FILE_STATESLOT_3_MENU);
	MENUITEM2(ID_FILE_STATESLOT_2, IDS_FILE_STATESLOT_2_MENU);
	MENUITEM2(ID_FILE_STATESLOT_1, IDS_FILE_STATESLOT_1_MENU);
	MENUITEM2(ID_FILE_STATESLOT_0, IDS_FILE_STATESLOT_0_MENU);
	SUBMENU2(IDS_FILE_STATESLOT_MENU);
	MENUITEM(ID_FILE_LOADSTATEAS, IDS_FILE_LOADSTATEAS_MENU);
	MENUITEM(ID_FILE_SAVESTATEAS, IDS_FILE_SAVESTATEAS_MENU);
	MENUITEM(ID_FILE_LOADSTATE, IDS_FILE_LOADSTATE_MENU);
	MENUITEM(ID_FILE_SAVESTATE, IDS_FILE_SAVESTATE_MENU);
	SEPARATOR();
	MENUITEM(ID_FILE_CLOSE, IDS_FILE_CLOSE_MENU);
	MENUITEM(ID_FILE_OPEN, IDS_FILE_OPEN_MENU);
	SUBMENU(IDS_FILE_MENU);


	hMenu = hPopupMenu;

	//Disassembly menu
	hSubMenu = CreateMenu();
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_SVBK_BANK7, IDS_VIEW_BANK_SVBK_7_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK6, IDS_VIEW_BANK_SVBK_6_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK5, IDS_VIEW_BANK_SVBK_5_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK4, IDS_VIEW_BANK_SVBK_4_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK3, IDS_VIEW_BANK_SVBK_3_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK2, IDS_VIEW_BANK_SVBK_2_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK1, IDS_VIEW_BANK_SVBK_1_MENU);
	SUBMENU2(IDS_VIEW_BANK_SVBK_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_RAM_BANK1, IDS_VIEW_BANK_RAM_1_MENU);
	MENUITEM2(ID_MEMORY_RAM_BANK0, IDS_VIEW_BANK_RAM_0_MENU);
	SUBMENU2(IDS_VIEW_BANK_RAM_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_VBK_BANK1, IDS_VIEW_BANK_VBK_1_MENU);
	MENUITEM2(ID_MEMORY_VBK_BANK0, IDS_VIEW_BANK_VBK_0_MENU);
	SUBMENU2(IDS_VIEW_BANK_VBK_MENU);
	MENUITEM(ID_MEMORY_ROM, IDS_VIEW_BANK_ROM_MENU);
	MENUITEM(ID_EDIT_GOTO, IDS_EDIT_GOTO_MENU);
	SEPARATOR();
	MENUITEM(ID_EMULATION_TOGGLEBREAKPOINT, IDS_EMULATION_TOGGLEBREAKPOINT_MENU);
	MENUITEM(ID_EMULATION_SETNEXTSTATEMENT, IDS_EMULATION_SETNEXTSTATEMENT_MENU);
	MENUITEM(ID_EMULATION_RUNTOCURSOR, IDS_EMULATION_RUNTOCURSOR_MENU);
	SUBMENU(IDS_VIEW_BANK_MENU);

	//Memory menu
	hSubMenu = CreateMenu();
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_SVBK_BANK7, IDS_VIEW_BANK_SVBK_7_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK6, IDS_VIEW_BANK_SVBK_6_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK5, IDS_VIEW_BANK_SVBK_5_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK4, IDS_VIEW_BANK_SVBK_4_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK3, IDS_VIEW_BANK_SVBK_3_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK2, IDS_VIEW_BANK_SVBK_2_MENU);
	MENUITEM2(ID_MEMORY_SVBK_BANK1, IDS_VIEW_BANK_SVBK_1_MENU);
	SUBMENU2(IDS_VIEW_BANK_SVBK_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_RAM_BANK1, IDS_VIEW_BANK_RAM_1_MENU);
	MENUITEM2(ID_MEMORY_RAM_BANK0, IDS_VIEW_BANK_RAM_0_MENU);
	SUBMENU2(IDS_VIEW_BANK_RAM_MENU);
	hSubMenu2 = CreateMenu();
	MENUITEM2(ID_MEMORY_VBK_BANK1, IDS_VIEW_BANK_VBK_1_MENU);
	MENUITEM2(ID_MEMORY_VBK_BANK0, IDS_VIEW_BANK_VBK_0_MENU);
	SUBMENU2(IDS_VIEW_BANK_VBK_MENU);
	MENUITEM(ID_MEMORY_ROM, IDS_VIEW_BANK_ROM_MENU);
	MENUITEM(ID_EDIT_GOTO, IDS_EDIT_GOTO_MENU);
	SUBMENU(IDS_VIEW_BANK_MENU);

	return hMenu;
}



#define		ChangeLanguageWnd(hWin)						\
	if (hWin)											\
	{													\
		SendMessage(hWin, WM_APP_CHANGELANGUAGE, 0, 0);	\
	}

void ChangeLanguage()
{
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(GetMenu(hWnd), 0, MF_BYPOSITION);
	DeleteMenu(hPopupMenu, 0, MF_BYPOSITION);
	DeleteMenu(hPopupMenu, 0, MF_BYPOSITION);
	CreateMenus(GetMenu(hWnd), hPopupMenu);
	DrawMenuBar(hWnd);
	ChangeLanguageWnd(hRegisters);
	ChangeLanguageWnd(hDisAsm);
	ChangeLanguageWnd(hMemory);
	ChangeLanguageWnd(hTiles);
	ChangeLanguageWnd(hTileMap);
	ChangeLanguageWnd(hPalettes);
	ChangeLanguageWnd(hHardware);
}



BOOL StatusReady = true;

void SetStatus(char *szStatusText, DWORD dwStatus)
{
	char		szBuffer[0x100];


	if (dwStatus != SF_F4PRESSED)
	{
		F4Pressed = NULL;
	}

	switch (dwStatus)
	{
	case SF_MESSAGE:
		SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)szStatusText);
		StatusReady = true;
		return;

	case SF_CLEAR:
		SendMessage(hStatusWnd, WM_SETTEXT, 0, NULL);
		StatusReady = true;
		return;

	case SF_READY:
		StatusReady = true;
		break;

	case SF_F4PRESSED:
		if (F4Pressed)
		{
			SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)String(IDS_STATUS_ENTERSLOTNUMBER));
			StatusReady = false;
		}
		else
		{
			StatusReady = true;
		}
		break;
	}

	if (StatusReady)
	{
		SendMessage(hStatusWnd, WM_SETTEXT, 0, (LPARAM)String(IDS_STATUS_READY));
	}
}



LPARAM CALLBACK GameBoyWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return GameBoys.WndProc(hWin, uMsg, wParam, lParam);
}



BOOL CreateKey(char *szKey, char *szData)
{
	HKEY	hKey;
	DWORD	dw, ValueType, ValueSize;
	char	szOldData[MAX_PATH + 4];


	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_EXECUTE | KEY_WRITE, NULL, &hKey, &dw))
	{
		return false;
	}
	ValueSize = sizeof(szOldData);
	if (!RegQueryValueEx(hKey, "", NULL, &ValueType, (BYTE *)&szOldData, &ValueSize))
	{
		if (ValueType == REG_SZ)
		{
			if (!strcmp(szData, szOldData))
			{
				return false;
			}
		}
	}
	if (RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)szData, strlen(szData) + 1))
	{
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);

	return true;
}



void RegisterGbFileType()
{
	char	szFilename[MAX_PATH + 4];
	BOOL	Change;


	GetModuleFileName(NULL, (char *)&szFilename[1], MAX_PATH);
	szFilename[0] = '\"';
	strcat(szFilename, "\"");

	Change = false;
	Change |= CreateKey(".gb", "GameBoyRom");
	Change |= CreateKey(".gbc", "GameBoyRom");
	Change |= CreateKey("GameBoyRom", "Game Boy Rom");
	Change |= CreateKey("GameBoyRom\\shell", "open");
	Change |= CreateKey("GameBoyRom\\shell\\open", "");
	Change |= CreateKey("GameBoyRom\\shell\\open\\command", szFilename);
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec", "open,\"%1\"");
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec\\Application", "Game Lad");
	Change |= CreateKey("GameBoyRom\\shell\\open\\ddeexec\\Topic", "system");
	Change |= CreateKey("GameBoyRom\\shellex\\PropertySheetHandlers\\Game Lad", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
	strcat(szFilename, ",0");
	Change |= CreateKey("GameBoyRom\\DefaultIcon", szFilename);
	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



void RegisterGlsFileType()
{
	char	szFilename[MAX_PATH + 4];
	BOOL	Change;


	GetModuleFileName(NULL, (char *)&szFilename[1], MAX_PATH);
	szFilename[0] = '\"';
	strcat(szFilename, "\"");

	Change = false;
	Change |= CreateKey(".gls", "GameLadSaveState");
	Change |= CreateKey("GameLadSaveState", "Game Lad Save State");
	Change |= CreateKey("GameLadSaveState\\shell", "open");
	Change |= CreateKey("GameLadSaveState\\shell\\open", "");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\command", szFilename);
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec", "state,\"%1\",open,\"\"");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec\\Application", "Game Lad");
	Change |= CreateKey("GameLadSaveState\\shell\\open\\ddeexec\\Topic", "system");
	Change |= CreateKey("GameLadSaveState\\shellex\\PropertySheetHandlers\\Game Lad", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
	strcat(szFilename, ",1");
	//Change |= CreateKey("GameLadSaveState\\DefaultIcon", szFilename);
	Change |= CreateKey("GameLadSaveState\\DefaultIcon", "%1");
	Change |= CreateKey("GameLadSaveState\\shellex\\IconHandler", "{acdece20-a9d8-11d4-ace1-e0ae57c10001}");
	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



BOOL DeleteKey(char *szKey)
{
	HKEY	hKey;


	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, &hKey))
	{
		return false;
	}
	RegCloseKey(hKey);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

	return true;
}



void UnRegisterGbFileType()
{
	BOOL	Change = false;


	Change |= DeleteKey(".gb");
	Change |= DeleteKey(".gbc");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec\\Application");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec\\Topic");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\ddeexec");
	Change |= DeleteKey("GameBoyRom\\shell\\open\\command");
	Change |= DeleteKey("GameBoyRom\\shell\\open");
	Change |= DeleteKey("GameBoyRom\\shell");
	Change |= DeleteKey("GameBoyRom\\shellex\\PropertySheetHandlers\\Game Lad");
	Change |= DeleteKey("GameBoyRom\\shellex\\PropertySheetHandlers");
	Change |= DeleteKey("GameBoyRom\\shellex");
	Change |= DeleteKey("GameBoyRom\\DefaultIcon");
	Change |= DeleteKey("GameBoyRom");

	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



void UnRegisterGlsFileType()
{
	BOOL	Change = false;


	Change |= DeleteKey(".gls");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec\\Application");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec\\Topic");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\ddeexec");
	Change |= DeleteKey("GameLadSaveState\\shell\\open\\command");
	Change |= DeleteKey("GameLadSaveState\\shell\\open");
	Change |= DeleteKey("GameLadSaveState\\shell");
	Change |= DeleteKey("GameLadSaveState\\shellex\\PropertySheetHandlers\\Game Lad");
	Change |= DeleteKey("GameLadSaveState\\shellex\\PropertySheetHandlers");
	Change |= DeleteKey("GameLadSaveState\\shellex\\IconHandler");
	Change |= DeleteKey("GameLadSaveState\\shellex");
	Change |= DeleteKey("GameLadSaveState\\DefaultIcon");
	Change |= DeleteKey("GameLadSaveState");

	if (Change)
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}



#define		DDEGB_DMG			0x00000001
#define		DDEGB_GBC			0x00000002
#define		DDEGB_LOAD			0x00000004
#define		DDEGB_DEBUG			0x00000008
#define		DDEGB_EXECUTE		0x00000010
#define		DDEGB_NOBATTERY		0x00000020
#define		DDEGB_BATTERY		0x00000040
#define		DDEGB_STATE			0x00000080
#define		DDEGB_NOSTATE		0x00000100

struct DDEGAMEBOY
{
	DWORD		Flags;
	char		szROM[MAX_PATH];
	char		szBattery[MAX_PATH];
	char		szState[MAX_PATH];
};



HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	char		*pBuffer;
	DDEGAMEBOY	*DDEGameBoy;
	DWORD		Size, FirstCharacter;


	switch (uType)
	{
	case XTYP_CONNECT:
		if (hsz1 == hDdeTopicString && hsz2 == hDdeServiceString)
		{
			return (HDDEDATA)true;
		}
		return false;

	case XTYP_EXECUTE:
		Size = DdeGetData(hdata, NULL, 0, 0);
		if (!(pBuffer = new char[Size]))
		{
			return DDE_FNOTPROCESSED;
		}
		if (!(DDEGameBoy = new DDEGAMEBOY))
		{
			delete pBuffer;
			return DDE_FNOTPROCESSED;
		}
		DdeGetData(hdata, (BYTE *)pBuffer, Size, 0);
		FirstCharacter = 0;
		DDEGameBoy->Flags = 0;
		DDEGameBoy->szROM[0] = 0;
		DDEGameBoy->szBattery[0] = 0;
		DDEGameBoy->szState[0] = 0;
		if (!strncmp(&pBuffer[FirstCharacter], "dmg,", 4))
		{
			DDEGameBoy->Flags |= DDEGB_DMG;
			FirstCharacter += 4;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "gbc,", 4))
		{
			DDEGameBoy->Flags |= DDEGB_GBC;
			FirstCharacter += 4;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "battery,", 8))
		{
			DDEGameBoy->Flags |= DDEGB_BATTERY;
			FirstCharacter += 8;
			if (pBuffer[FirstCharacter] == '\"')
			{
				Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
				strncpy(DDEGameBoy->szBattery, &pBuffer[FirstCharacter + 1], Size);
				DDEGameBoy->szBattery[Size] = 0;
				FirstCharacter += 2 + Size;
			}
			FirstCharacter++;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "nobattery,", 10))
		{
			DDEGameBoy->Flags |= DDEGB_NOBATTERY;
			FirstCharacter += 10;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "state,", 6))
		{
			DDEGameBoy->Flags |= DDEGB_STATE;
			FirstCharacter += 6;
			if (pBuffer[FirstCharacter] == '\"')
			{
				Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
				strncpy(DDEGameBoy->szState, &pBuffer[FirstCharacter + 1], Size);
				DDEGameBoy->szState[Size] = 0;
				FirstCharacter += 2 + Size;
			}
			FirstCharacter++;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "nostate,", 10))
		{
			DDEGameBoy->Flags |= DDEGB_NOSTATE;
			FirstCharacter += 8;
		}
		if (!strncmp(&pBuffer[FirstCharacter], "open,", 5))
		{
			FirstCharacter += 5;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "load,", 5))
		{
			DDEGameBoy->Flags |= DDEGB_LOAD;
			FirstCharacter += 5;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "debug,", 6))
		{
			DDEGameBoy->Flags |= DDEGB_DEBUG;
			FirstCharacter += 6;
		}
		else if (!strncmp(&pBuffer[FirstCharacter], "execute,", 8))
		{
			DDEGameBoy->Flags |= DDEGB_EXECUTE;
			FirstCharacter += 8;
		}
		Size = strchr(&pBuffer[FirstCharacter + 1], '\"') - &pBuffer[FirstCharacter + 1];
		strncpy(DDEGameBoy->szROM, &pBuffer[FirstCharacter + 1], Size);
		DDEGameBoy->szROM[Size] = 0;

		delete pBuffer;
		PostMessage(hWnd, WM_APP_DDEOPENFILE, 0, (LPARAM)DDEGameBoy);
		return (HDDEDATA)DDE_FACK;
	}

	return NULL;
}



WNDPROC		pStaticWndProc;

LRESULT CALLBACK LinkWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	HDC				hdc;
	HFONT			hFont;
	char			szText[0x100];
	SIZE			Size;
	int				x;
	RECT			rct;
	POINT			pt;
	LRESULT			lResult;


	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		SendMessage(hWin, WM_GETTEXT, sizeof(szText), (LPARAM)szText);
		if (strchr(szText, '['))
		{
			if (strchr(strchr(szText, '['), ';'))
			{
				*strchr(szText, ';') = '\0';
			}
			else
			{
				if (strchr(szText, ']'))
				{
					*strchr(szText, ']') = '\0';
				}
			}
			ShellExecute(hWin, NULL, strchr(szText, '[') + 1, NULL, NULL, SW_SHOW);
		}
		return 0;

	case WM_MOUSEMOVE:
		SendMessage(hWin, WM_GETTEXT, sizeof(szText), (LPARAM)szText);
		if (strchr(szText, '['))
		{
			hdc = GetDC(hWin);
			GetWindowRect(hWin, &rct);
			GetCursorPos(&pt);
			x = 0;
			hFont = CreateFont(8, 0, 0, 0, FW_DONTCARE, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "MS Sans Serif");
			hFont = (HFONT)SelectObject(hdc, hFont);
			if (szText[0] != '[')
			{
				GetTextExtentPoint32(hdc, szText, strchr(szText, '[') - szText, &Size);
				if (pt.x < rct.left + Size.cx)
				{
					hFont = (HFONT)SelectObject(hdc, hFont);
					DeleteObject(hFont);
					ReleaseDC(hWin, hdc);
					return 0;
				}
				x = Size.cx;
			}
			if (strchr(strchr(szText, '['), ';'))
			{
				GetTextExtentPoint32(hdc, strchr(strchr(szText, '['), ';') + 1, strchr(szText, ']') - strchr(szText, ';') - 1, &Size);
				if (pt.x > rct.left + x + Size.cx)
				{
					hFont = (HFONT)SelectObject(hdc, hFont);
					DeleteObject(hFont);
					ReleaseDC(hWin, hdc);
					return 0;
				}
			}
			else
			{
				GetTextExtentPoint32(hdc, strchr(szText, '[') + 1, strchr(szText, ']') - strchr(szText, '[') - 1, &Size);
				if (pt.x > rct.left + x + Size.cx)
				{
					hFont = (HFONT)SelectObject(hdc, hFont);
					DeleteObject(hFont);
					ReleaseDC(hWin, hdc);
					return 0;
				}
			}
			hFont = (HFONT)SelectObject(hdc, hFont);
			DeleteObject(hFont);
			ReleaseDC(hWin, hdc);
			SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649) /*IDC_HAND*/));
		}
		return 0;

	case WM_SETTEXT:
		ShowWindow(hWin, false);
		lResult = CallWindowProc(pStaticWndProc, hWin, uMsg, wParam, lParam);
		ShowWindow(hWin, true);
		return lResult;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			SetBkMode(Paint.hdc, TRANSPARENT);
			SendMessage(hWin, WM_GETTEXT, sizeof(szText), (LPARAM)szText);
			x = 0;
			if (strchr(szText, '['))
			{
				if (szText[0] != '[')
				{
					hFont = CreateFont(8, 0, 0, 0, FW_DONTCARE, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "MS Sans Serif");
					hFont = (HFONT)SelectObject(Paint.hdc, hFont);
					TextOut(Paint.hdc, 0, 0, szText, strchr(szText, '[') - szText);
					GetTextExtentPoint32(Paint.hdc, szText, strchr(szText, '[') - szText, &Size);
					x = Size.cx;
					hFont = (HFONT)SelectObject(Paint.hdc, hFont);
					DeleteObject(hFont);
				}
				hFont = CreateFont(8, 0, 0, 0, FW_DONTCARE, false, true, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "MS Sans Serif");
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				SetTextColor(Paint.hdc, RGB(0x00, 0x00, 0xFF));
				if (strchr(strchr(szText, '['), ';'))
				{
					TextOut(Paint.hdc, x, 0, strchr(strchr(szText, '['), ';') + 1, strchr(szText, ']') - strchr(szText, ';') - 1);
					GetTextExtentPoint32(Paint.hdc, strchr(strchr(szText, '['), ';') + 1, strchr(szText, ']') - strchr(szText, ';') - 1, &Size);
					x += Size.cx;
				}
				else
				{
					TextOut(Paint.hdc, x, 0, strchr(szText, '[') + 1, strchr(szText, ']') - strchr(szText, '[') - 1);
					GetTextExtentPoint32(Paint.hdc, strchr(szText, '[') + 1, strchr(szText, ']') - strchr(szText, '[') - 1, &Size);
					x += Size.cx;
				}
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				DeleteObject(hFont);

				hFont = CreateFont(8, 0, 0, 0, FW_DONTCARE, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "MS Sans Serif");
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				SetTextColor(Paint.hdc, RGB(0x00, 0x00, 0x00));
				TextOut(Paint.hdc, x, 0, strchr(szText, ']') + 1, szText + strlen(szText) - strchr(szText, ']') - 1);
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				DeleteObject(hFont);
			}
			else
			{
				hFont = CreateFont(8, 0, 0, 0, FW_DONTCARE, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "MS Sans Serif");
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				TextOut(Paint.hdc, 0, 0, szText, strlen(szText));
				hFont = (HFONT)SelectObject(Paint.hdc, hFont);
				DeleteObject(hFont);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;
	}

	return CallWindowProc(pStaticWndProc, hWin, uMsg, wParam, lParam);
}



BOOL CALLBACK AboutDialogProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT	rct;
	long	width, height;
	char	szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		if (wParam == IDOK)
		{
			EndDialog(hWin, 0);
			return true;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWin, 0);
		return true;

	case WM_INITDIALOG:
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)String(IDS_ABOUT_TITLE));

		pStaticWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDC_WRITTENBY), GWL_WNDPROC, (long)LinkWndProc);
		pStaticWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDC_GAMELADHOMEPAGE), GWL_WNDPROC, (long)LinkWndProc);

		//Center dialog
		GetWindowRect(hWin, &rct);
		width = rct.right - rct.left;
		height = rct.bottom - rct.top;
		GetWindowRect(hWnd, &rct);
		rct.right -= rct.left; //width
		rct.bottom -= rct.top; //height
		MoveWindow(hWin, rct.left + ((rct.right - width) >> 1), rct.top + ((rct.bottom - height) >> 1), width, height, true);

		SendDlgItemMessage(hWin, IDC_WRITTENBY, WM_SETTEXT, 0, (LPARAM)LoadString(IDS_ABOUT_WRITTENBY, szBuffer, sizeof(szBuffer), "[mailto:torbjrn@emuunlim.com;Torbjörn Söderstedt]"));
		return true;
	}

	return false;
}



HWND	hTitle, hAutoStart, hLoadState, hLoadBattery, hGBType;
BOOL	AutoStart, LoadState, LoadBattery;
BYTE	GBType;

UINT CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	char			Filename[MAX_PATH], Buffer[0x50];
	HANDLE			hFile;
	DWORD			nBytes, FileSize;
	char			szBuffer[0x100];


	switch (uiMsg)
	{
	/*case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		}
		break;*/

	case WM_NOTIFY:
		switch (((OFNOTIFY *)lParam)->hdr.code)
		{
		case CDN_FILEOK:
			switch (SendMessage(hAutoStart, CB_GETCURSEL, 0, 0))
			{
			case 0:
				AutoStart = 0;
				break;

			case 1:
				AutoStart = AUTOSTART_DEBUG;
				break;

			case 2:
				AutoStart = AUTOSTART_EXECUTE;
				break;
			}
			LoadState = false;
			if (SendMessage(hLoadState, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				LoadState = true;
			}
			LoadBattery = false;
			if (SendMessage(hLoadBattery, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				LoadBattery = true;
			}
			GBType = (SendMessage(hGBType, CB_GETCURSEL, 0, 0) == 1) ? GB_COLOR : 0;
			break;

		case CDN_SELCHANGE:
			SetWindowText(hTitle, LoadString(IDS_OPEN_TITLE, szBuffer, sizeof(szBuffer), ""));
			if (!SendMessage(GetParent(hdlg), CDM_GETFILEPATH, sizeof(Filename), (long)&Filename))
			{
				break;
			}
			if ((hFile = CreateFile(Filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				break;
			}
			if (SetFilePointer(hFile, 0x100, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				CloseHandle(hFile);
				break;
			}
			if (!ReadFile(hFile, Buffer, 0x50, &nBytes, NULL))
			{
				CloseHandle(hFile);
				break;
			}
			FileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
			if (nBytes != 0x50 || (RomSize(Buffer[0x48]) + 1) * 16384U != FileSize)
			{
				break;
			}
			/*if (memcmp(&Buffer[0x4], NintendoGraphic, 48))
			{
				break;
			}*/
			if (Buffer[0x43] & 0x80)
			{
				Buffer[0x43] = 0;
			}
			Buffer[0x44] = 0;
			SetWindowText(hTitle, LoadString(IDS_OPEN_TITLE, szBuffer, sizeof(szBuffer), &Buffer[0x34]));
			break;
		}
		break;

	//case WM_SIZE:
		//return 0;

	case WM_INITDIALOG:
		SetWindowLong(hdlg, GWL_STYLE, GetWindowLong(hdlg, GWL_STYLE) | WS_CLIPSIBLINGS);
		MoveWindow(hdlg, 0, 0, 400, 63, true);
		hTitle = CreateWindow("STATIC", LoadString(IDS_OPEN_TITLE, szBuffer, sizeof(szBuffer), ""), WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 7, 0, 180, 16, hdlg, NULL, hInstance, NULL);
		SendMessage(hTitle, WM_SETFONT, (WPARAM)hFont, true);

		hGBType = CreateWindow("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, 7, 40, 180, 200, hdlg, NULL, hInstance, NULL);
		SendMessage(hGBType, WM_SETFONT, (WPARAM)hFont, true);
		SendMessage(hGBType, CB_ADDSTRING, 0, (LPARAM)"Game Boy");
		SendMessage(hGBType, CB_ADDSTRING, 0, (LPARAM)"Game Boy Color");
		SendMessage(hGBType, CB_SETCURSEL, Settings.GBType == GB_COLOR ? 1 : 0, 0);


		hLoadState = CreateWindow("BUTTON", String(IDS_OPEN_LOADSTATE), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 7, 20, 180, 16, hdlg, NULL, hInstance, NULL);
		if (Settings.AutoLoadState)
		{
			SendMessage(hLoadState, BM_SETCHECK, BST_CHECKED, 0);
		}
		SendMessage(hLoadState, WM_SETFONT, (WPARAM)hFont, true);

		hLoadBattery = CreateWindow("BUTTON", String(IDS_OPEN_LOADBATTERY), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 205, 20, 180, 16, hdlg, NULL, hInstance, NULL);
		if (Settings.AutoLoadBattery)
		{
			SendMessage(hLoadBattery, BM_SETCHECK, BST_CHECKED, 0);
		}
		SendMessage(hLoadBattery, WM_SETFONT, (WPARAM)hFont, true);

		hAutoStart = CreateWindow("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, 205, 40, 180, 200, hdlg, NULL, hInstance, NULL);
		SendMessage(hAutoStart, WM_SETFONT, (WPARAM)hFont, true);
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)String(IDS_STOPPED));
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)String(IDS_STARTDEBUG));
		SendMessage(hAutoStart, CB_ADDSTRING, 0, (LPARAM)String(IDS_EXECUTE));
		switch (Settings.AutoStart)
		{
		case 0:
			SendMessage(hAutoStart, CB_SETCURSEL, 0, 0);
			break;

		case AUTOSTART_DEBUG:
			SendMessage(hAutoStart, CB_SETCURSEL, 1, 0);
			break;

		case AUTOSTART_EXECUTE:
			SendMessage(hAutoStart, CB_SETCURSEL, 2, 0);
			break;
		}

		/*hLoadDebug = CreateWindow("BUTTON", "Load debugging info if possible.", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 7, 18, 200, 16, hdlg, NULL, hInstance, NULL);
		if (Settings.AutoLoadDebug)
		{
			SendMessage(hLoadDebug, BM_SETCHECK, BST_CHECKED, 0);
		}
		SendMessage(hLoadDebug, WM_SETFONT, (long)hFont, true);*/
	}

	SetWindowLong(GetParent(hdlg), DWL_MSGRESULT, 0);
	return 0;
}



struct CHARACTERS
{
	BYTE	Vk, dummy;
	UINT	uID;
} const Characters[71] ={
							NULL, 0, IDS_OPTIONS_KEYS_NOKEY,
							'0', 0, 0,
							'1', 0, 0,
							'2', 0, 0,
							'3', 0, 0,
							'4', 0, 0,
							'5', 0, 0,
							'6', 0, 0,
							'7', 0, 0,
							'8', 0, 0,
							'9', 0, 0,
							'A', 0, 0,
							VK_APPS, 0, IDS_VK_APPS,
							'B', 0, 0,
							VK_BACK, 0, IDS_VK_BACK,
							'C', 0, 0,
							VK_CONTROL, 0, IDS_VK_CONTROL,
							'D', 0, 0,
							VK_DELETE, 0, IDS_VK_DELETE,
							VK_DOWN, 0, IDS_VK_DOWN,
							'E', 0, 0,
							VK_END, 0, IDS_VK_END,
							VK_RETURN, 0, IDS_VK_RETURN,
							'F', 0, 0,
							'G', 0, 0,
							'H', 0, 0,
							VK_HOME, 0, IDS_VK_HOME,
							'I', 0, 0,
							VK_INSERT, 0, IDS_VK_INSERT,
							'J', 0, 0,
							'K', 0, 0,
							'L', 0, 0,
							VK_LEFT, 0, IDS_VK_LEFT,
							'M', 0, 0,
							'N', 0, 0,
							VK_NUMLOCK, 0, IDS_VK_NUMLOCK,
							VK_NUMPAD0, 0, IDS_VK_NUMPAD0,
							VK_NUMPAD1, 0, IDS_VK_NUMPAD1,
							VK_NUMPAD2, 0, IDS_VK_NUMPAD2,
							VK_NUMPAD3, 0, IDS_VK_NUMPAD3,
							VK_NUMPAD4, 0, IDS_VK_NUMPAD4,
							VK_NUMPAD5, 0, IDS_VK_NUMPAD5,
							VK_NUMPAD6, 0, IDS_VK_NUMPAD6,
							VK_NUMPAD7, 0, IDS_VK_NUMPAD7,
							VK_NUMPAD8, 0, IDS_VK_NUMPAD8,
							VK_NUMPAD9, 0, IDS_VK_NUMPAD9,
							VK_ADD, 0, IDS_VK_ADD,
							VK_DECIMAL, 0, IDS_VK_DECIMAL,
							VK_DIVIDE, 0, IDS_VK_DIVIDE,
							VK_MULTIPLY, 0, IDS_VK_MULTIPLY,
							VK_SUBTRACT, 0, IDS_VK_SUBTRACT,
							'O', 0, 0,
							'P', 0, 0,
							VK_NEXT, 0, IDS_VK_NEXT,
							VK_PRIOR, 0, IDS_VK_PRIOR,
							VK_PAUSE, 0, IDS_VK_PAUSE,
							'Q', 0, 0,
							'R', 0, 0,
							VK_RIGHT, 0, IDS_VK_RIGHT,
							'S', 0, 0,
							VK_SHIFT, 0, IDS_VK_SHIFT,
							VK_SPACE, 0, IDS_VK_SPACE,
							'T', 0, 0,
							VK_TAB, 0, IDS_VK_TAB,
							'U', 0, 0,
							VK_UP, 0, IDS_VK_UP,
							'V', 0, 0,
							'W', 0, 0,
							'X', 0, 0,
							'Y', 0, 0,
							'Z', 0, 0,
						};



KEYS		TempKeys[4], TempJoyButtons[4];
GUID		TempGuid[2];
BOOL		TempJoyIsAnalog[2];
char		szTempKeyScheme[2][0x100];
DWORD		ButtonListPos;



void GetSelectedKey(HWND hWin, BOOL InvertPlayer, BOOL InvertButtonType)
{
	DWORD		dw, dwKey, dwJoyButton;


	if (SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0))
	{
		if (InvertPlayer)
		{
			dw = 0;
		}
		else
		{
			dw = 2;
		}
	}
	else
	{
		if (InvertPlayer)
		{
			dw = 2;
		}
		else
		{
			dw = 0;
		}
	}
	if (SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_GETCURSEL, 0, 0))
	{
		if (!InvertButtonType)
		{
			dw++;
		}
	}
	else
	{
		if (InvertButtonType)
		{
			dw++;
		}
	}
	dwKey = SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_GETCURSEL, 0, 0), 0);
	dwJoyButton = SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_GETCURSEL, 0, 0);
	switch (ButtonListPos)
	{
	case 0:
		TempKeys[dw].Up = dwKey;
		TempJoyButtons[dw].Up = dwJoyButton;
		break;
	case 1:
		TempKeys[dw].Down = dwKey;
		TempJoyButtons[dw].Down = dwJoyButton;
		break;
	case 2:
		TempKeys[dw].Left = dwKey;
		TempJoyButtons[dw].Left = dwJoyButton;
		break;
	case 3:
		TempKeys[dw].Right = dwKey;
		TempJoyButtons[dw].Right = dwJoyButton;
		break;
	case 4:
		TempKeys[dw].A = dwKey;
		TempJoyButtons[dw].A = dwJoyButton;
		break;
	case 5:
		TempKeys[dw].B = dwKey;
		TempJoyButtons[dw].B = dwJoyButton;
		break;
	case 6:
		TempKeys[dw].Start = dwKey;
		TempJoyButtons[dw].Start = dwJoyButton;
		break;
	case 7:
		TempKeys[dw].Select = dwKey;
		TempJoyButtons[dw].Select = dwJoyButton;
		break;
	case 8:
		TempKeys[dw & ~1].FastForward = dwKey;
		TempJoyButtons[dw & ~1].FastForward = dwJoyButton;
		break;
	}
}



BOOL RefreshKeys(HWND hWin, BOOL RefreshButtons)
{
	DWORD			VK, dwPos;
	char			szBuffer[0x100];
	DWORD			dw, dwJoyButton;
	JOYSTICKINFO	*pJoyStickInfo;


	dwPos = SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0);
	if (RefreshButtons)
	{
		if (dwPos == CB_ERR)
		{
			dwPos = 0;
		}
		SendDlgItemMessage(hWin, IDC_PLAYER, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWin, IDC_PLAYER, CB_ADDSTRING, 0, (LPARAM)LoadString(IDS_OPTIONS_KEYS_PLAYER, szBuffer, sizeof(szBuffer), "1"));
		SendDlgItemMessage(hWin, IDC_PLAYER, CB_ADDSTRING, 0, (LPARAM)LoadString(IDS_OPTIONS_KEYS_PLAYER, szBuffer, sizeof(szBuffer), "2"));
		SendDlgItemMessage(hWin, IDC_PLAYER, CB_SETCURSEL, dwPos, 0);

		dw = SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_GETCURSEL, 0, 0);
		if (dw == CB_ERR)
		{
			dw = 0;
		}
		SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_NORMAL));
		SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_AUTOFIRE));
		SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_SETCURSEL, dw, 0);
	}

	if (dwPos)
	{
		dw = 2;
	}
	else
	{
		dw = 0;
	}
	if (SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_GETCURSEL, 0, 0))
	{
		dw++;
	}
	switch (ButtonListPos = SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_GETCURSEL, 0, 0))
	{
	case 0:
		dwJoyButton = TempJoyButtons[dw].Up;
		dw = TempKeys[dw].Up;
		break;
	case 1:
		dwJoyButton = TempJoyButtons[dw].Down;
		dw = TempKeys[dw].Down;
		break;
	case 2:
		dwJoyButton = TempJoyButtons[dw].Left;
		dw = TempKeys[dw].Left;
		break;
	case 3:
		dwJoyButton = TempJoyButtons[dw].Right;
		dw = TempKeys[dw].Right;
		break;
	case 4:
		dwJoyButton = TempJoyButtons[dw].A;
		dw = TempKeys[dw].A;
		break;
	case 5:
		dwJoyButton = TempJoyButtons[dw].B;
		dw = TempKeys[dw].B;
		break;
	case 6:
		dwJoyButton = TempJoyButtons[dw].Start;
		dw = TempKeys[dw].Start;
		break;
	case 7:
		dwJoyButton = TempJoyButtons[dw].Select;
		dw = TempKeys[dw].Select;
		break;
	case 8:
		dwJoyButton = TempJoyButtons[dw & ~1].FastForward;
		dw = TempKeys[dw & ~1].FastForward;
		break;
	}

	if (RefreshButtons)
	{
		SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_RESETCONTENT, 0, 0);

		for (VK = 0; VK < 71; VK++)
		{
			if (Characters[VK].uID)
			{
				dwPos = SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_ADDSTRING, 0, (LPARAM)String(Characters[VK].uID));
			}
			else
			{
				dwPos = SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_ADDSTRING, 0, (LPARAM)&Characters[VK].Vk);
			}
			SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_SETITEMDATA, dwPos, Characters[VK].Vk);
			if (Characters[VK].Vk == dw)
			{
				SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_SETCURSEL, dwPos, 0);
			}
		}

		dwPos = (DWORD)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCOUNT, 0, 0);
		while (dwPos--)
		{
			if (SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, dwPos, 0))
			{
				if (SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0))
				{
					if (!memcmp(&TempGuid[1], &((JOYSTICKINFO *)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, dwPos, 0))->Guid, sizeof(GUID)))
					{
						SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_SETCURSEL, dwPos, 0);
						break;
					}
				}
				else
				{
					if (!memcmp(&TempGuid[0], &((JOYSTICKINFO *)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, dwPos, 0))->Guid, sizeof(GUID)))
					{
						SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_SETCURSEL, dwPos, 0);
						break;
					}
				}
			}
			else
			{
				SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_SETCURSEL, dwPos, 0);
			}
		}

		if (pJoyStickInfo = (JOYSTICKINFO *)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
		{
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_NOKEY));
			for (dwPos = 0; dwPos < pJoyStickInfo->dwButtons; dwPos++)
			{
				SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_ADDSTRING, 0, (LPARAM)&pJoyStickInfo->Buttons[dwPos]);
			}
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_SETCURSEL, dwJoyButton, 0);
		}
	}
	else
	{
		for (dwPos = 0; (VK = SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_GETITEMDATA, dwPos, 0)) != CB_ERR; dwPos++)
		{
			if (VK == dw)
			{
				SendDlgItemMessage(hWin, IDC_KEYBOARD, CB_SETCURSEL, dwPos, 0);
				break;
			}
		}
		if (SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
		{
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_SETCURSEL, dwJoyButton, 0);
		}
		else
		{
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_SETCURSEL, 0, 0);
		}
	}

	if (ButtonListPos == 8 && SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_GETCURSEL, 0, 0))
	{
		EnableWindow(GetDlgItem(hWin, IDC_KEYBOARD), false);
	}
	else
	{
		EnableWindow(GetDlgItem(hWin, IDC_KEYBOARD), true);
	}

	if ((ButtonListPos == 8 && SendDlgItemMessage(hWin, IDC_BUTTONTYPE, CB_GETCURSEL, 0, 0)) || !SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
	{
		EnableWindow(GetDlgItem(hWin, IDC_DEVICE_BUTTONS), false);
	}
	else
	{
		EnableWindow(GetDlgItem(hWin, IDC_DEVICE_BUTTONS), true);
	}

	SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, BM_SETCHECK, TempJoyIsAnalog[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0)] ? BST_CHECKED : BST_UNCHECKED, 0);
	if (SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
	{
		EnableWindow(GetDlgItem(hWin, IDC_JOYSTICK_ANALOG), true);
	}
	else
	{
		EnableWindow(GetDlgItem(hWin, IDC_JOYSTICK_ANALOG), false);
	}

	return false;
}



BOOL CALLBACK EnumDeviceObjectsProc(const DIDEVICEOBJECTINSTANCE *lpddoi, void *pJoyStickInfo)
{
	if (lpddoi->guidType != GUID_Button)
	{
		return DIENUM_CONTINUE;
	}

	if (((JOYSTICKINFO *)pJoyStickInfo)->dwButtons >= 32)
	{
		return DIENUM_STOP;
	}

	strcpy(((JOYSTICKINFO *)pJoyStickInfo)->Buttons[((JOYSTICKINFO *)pJoyStickInfo)->dwButtons++], lpddoi->tszName);

	return DIENUM_CONTINUE;
}



BOOL CALLBACK EnumDeviceProc(LPCDIDEVICEINSTANCE lpddi, void *hWin)
{
	LPDIRECTINPUTDEVICE8	lpdidDevice;
	DIDEVCAPS				didc;
	JOYSTICKINFO			*pJoyStickInfo;


	if (lpdi->CreateDevice(lpddi->guidInstance, &lpdidDevice, NULL))
	{
		Error("lpdi->CreateDevice(lpddi->guidInstance, &lpdidDevice, NULL)");
		return DIENUM_CONTINUE;
	}
	if (lpdidDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))
	{
		Error("lpdidDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)");
	}
	lpdidDevice->SetDataFormat(&c_dfDIJoystick);
	didc.dwSize = sizeof(didc);
	if (lpdidDevice->GetCapabilities(&didc))
	{
		lpdidDevice->Release();
		Error("lpdidDevice->GetCapabilities(&didc)");
		return DIENUM_CONTINUE;
	}

	if (!pJoySticks)
	{
		if (!(pJoySticks = new CList()))
		{
			lpdidDevice->Release();
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return DIENUM_STOP;
		}
	}

	if (!(pJoyStickInfo = (JOYSTICKINFO *)pJoySticks->NewItem(sizeof(JOYSTICKINFO))))
	{
		lpdidDevice->Release();
		return DIENUM_STOP;
	}
	pJoyStickInfo->Guid = lpddi->guidInstance;
	pJoyStickInfo->dwButtons = 0;

	SendDlgItemMessage((HWND)hWin, IDC_INPUTDEVICES, CB_SETITEMDATA, SendDlgItemMessage((HWND)hWin, IDC_INPUTDEVICES, CB_ADDSTRING, 0, (LPARAM)&lpddi->tszProductName), (LPARAM)pJoyStickInfo);

	lpdidDevice->EnumObjects(EnumDeviceObjectsProc, pJoyStickInfo, DIDFT_ALL);

	lpdidDevice->Release();


	return DIENUM_CONTINUE;
}



BOOL CALLBACK KeySchemeDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (!GetWindowText(GetDlgItem(hWin, IDC_NUMBER), (char *)GetWindowLong(hWin, GWL_USERDATA), 0x100))
			{
				MessageBox(hWin, String(IDS_OPTIONS_KEYS_SAVESCHEME_NONAME), "Game Lad", MB_OK);
				return true;
			}
			EndDialog(hWin, false);
			return true;

		case IDCANCEL:
			EndDialog(hWin, true);
			return true;
		}
		break;

	case WM_INITDIALOG:
		SetWindowText(hWin, String(IDS_OPTIONS_KEYS_SAVESCHEMEAS_TITLE));
		SetWindowLong(hWin, GWL_USERDATA, lParam);
		SetWindowText(GetDlgItem(hWin, IDC_NUMBER), (char *)lParam);
		return true;
	}

	return false;
}



BOOL CALLBACK KeyOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char			szBuffer[0x100], szBuffer2[0x100];
	DWORD			dwPos, dwPlayer;
	KEYSCHEME		*pKeyScheme;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PLAYER:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				GetSelectedKey(hWin, true, false);
				SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETLBTEXT, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), (LPARAM)szTempKeyScheme[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0) == 1 ? 0 : 1]);
				dwPos = SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCOUNT, 0, 0);
				while (dwPos--)
				{
					pKeyScheme = (KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, dwPos, 0);
					if (!stricmp(szTempKeyScheme[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0)], pKeyScheme->szName))
					{
						SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, dwPos, 0);
						dwPos = 1;
						break;
					}
				}
				if (dwPos == 0)
				{
					SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, 0, 0);
				}
				RefreshKeys(hWin, true);
				return true;
			}
			break;

		case IDC_BUTTONTYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				GetSelectedKey(hWin, false, true);
				RefreshKeys(hWin, false);
				return true;
			}
			break;

		case IDC_KEYBOARD:
		case IDC_DEVICE_BUTTONS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
				return true;
			}
			break;

		case IDC_BUTTONLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				GetSelectedKey(hWin, false, false);
				RefreshKeys(hWin, false);
				return true;
			}
			break;

		case IDC_INPUTDEVICES:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
				switch (SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0))
				{
				case 0:
					if (SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
					{
						CopyMemory(&TempGuid[0], &((JOYSTICKINFO *)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))->Guid, sizeof(GUID));
					}
					else
					{
						ZeroMemory(&TempGuid[0], sizeof(TempGuid[0]));
					}
					break;
				case 1:
					if (SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))
					{
						CopyMemory(&TempGuid[1], &((JOYSTICKINFO *)SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_GETCURSEL, 0, 0), 0))->Guid, sizeof(GUID));
					}
					else
					{
						ZeroMemory(&TempGuid[1], sizeof(TempGuid[1]));
					}
					break;
				}
				RefreshKeys(hWin, true);
				return true;
			}
			break;

		case IDC_JOYSTICK_ANALOG:
			SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			TempJoyIsAnalog[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0)] = (SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
			/*if (SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, CB_GETCURSEL, 0, 0), 0))
			{
				TempJoyIsAnalog[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0) == 1 ? 0 : 1] = (SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
			}*/
			return true;

		case IDC_SCHEME:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
				dwPlayer = SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0);
				pKeyScheme = (KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0);
				memcpy(&TempGuid[dwPlayer], &pKeyScheme->JoyGuid, sizeof(GUID));
				memcpy(&TempKeys[dwPlayer * 2], pKeyScheme->Keyboard, sizeof(KEYS[2]));
				memcpy(&TempJoyButtons[dwPlayer * 2], pKeyScheme->Joystick, sizeof(KEYS[2]));
				TempJoyIsAnalog[dwPlayer] = pKeyScheme->JoyIsAnalog;
				RefreshKeys(hWin, true);

				if (((KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0))->BuiltIn)
				{
					EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), false);
				}
				else
				{
					EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), true);
				}

				return true;
			}
			break;

		case IDC_SAVESCHEMEAS:
			GetSelectedKey(hWin, false, false);
			SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETLBTEXT, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), (LPARAM)szBuffer);
			if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ENTERNUMBER), hWin, KeySchemeDlgProc, (LPARAM)szBuffer))
			{
				pKeyScheme = NULL;
				dwPos = SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCOUNT, 0, 0);
				if (dwPos != 0)
				{
					while (dwPos--)
					{
						SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETLBTEXT, dwPos, (LPARAM)szBuffer2);
						if (!stricmp(szBuffer, szBuffer2))
						{
							pKeyScheme = (KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, dwPos, 0);
							SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, dwPos, 0);
							if (((KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0))->BuiltIn)
							{
								EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), false);
							}
							else
							{
								EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), true);
							}
							break;
						}
					}
				}
				if (!pKeyScheme)
				{
					dwPos = CB_ERR;
					if (!(pKeyScheme = (KEYSCHEME *)pKeySchemes->NewItem(sizeof(*pKeyScheme))))
					{
						return true;
					}
				}
				else
				{
					if (pKeyScheme->BuiltIn)
					{
						MessageBox(hWin, String(IDS_OPTIONS_KEYS_SAVESCHEMEAS_CANNOTREPLACE), "Game Lad", MB_OK | MB_ICONWARNING);
						return true;
					}
				}
				dwPlayer = SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0);
				pKeyScheme->BuiltIn = false;
				strcpy(pKeyScheme->szName, szBuffer);
				memcpy(&pKeyScheme->JoyGuid, &TempGuid[dwPlayer], sizeof(GUID));
				memcpy(pKeyScheme->Keyboard, &TempKeys[dwPlayer * 2], sizeof(KEYS[2]));
				memcpy(pKeyScheme->Joystick, &TempJoyButtons[dwPlayer * 2], sizeof(KEYS[2]));
				pKeyScheme->JoyIsAnalog = TempJoyIsAnalog[dwPlayer];
				if (dwPos == CB_ERR)
				{
					if ((dwPos = SendDlgItemMessage(hWin, IDC_SCHEME, CB_ADDSTRING, 0, (LPARAM)pKeyScheme->szName)) != CB_ERR)
					{
						SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETITEMDATA, dwPos, (LPARAM)pKeyScheme);
						SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, dwPos, 0);
						if (((KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0))->BuiltIn)
						{
							EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), false);
						}
						else
						{
							EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), true);
						}
					}
				}
			}
			return true;

		case IDC_DELETESCHEME:
			pKeyScheme = (KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0);
			if (pKeyScheme->BuiltIn)
			{
				return true;
			}
			pKeySchemes->DeleteItem(pKeyScheme);
			SendDlgItemMessage(hWin, IDC_SCHEME, CB_DELETESTRING, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0);
			SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, 0, 0);
			if (((KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0))->BuiltIn)
			{
				EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), false);
			}
			else
			{
				EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), true);
			}
			return true;
		}
		break;

	case WM_INITDIALOG:
		CopyMemory((void *)&TempKeys[0], &Keys[0], sizeof(KEYS));
		CopyMemory((void *)&TempKeys[1], &AutoFireKeys[0], sizeof(KEYS));
		CopyMemory((void *)&TempKeys[2], &Keys[1], sizeof(KEYS));
		CopyMemory((void *)&TempKeys[3], &AutoFireKeys[1], sizeof(KEYS));
		CopyMemory((void *)&TempJoyButtons[0], &JoyButtons[0], sizeof(KEYS));
		CopyMemory((void *)&TempJoyButtons[1], &AutoFireJoyButtons[0], sizeof(KEYS));
		CopyMemory((void *)&TempJoyButtons[2], &JoyButtons[1], sizeof(KEYS));
		CopyMemory((void *)&TempJoyButtons[3], &AutoFireJoyButtons[1], sizeof(KEYS));
		CopyMemory(TempGuid, JoyGuid, sizeof(GUID[2]));
		strcpy(szTempKeyScheme[0], szKeyScheme[0]);
		strcpy(szTempKeyScheme[1], szKeyScheme[1]);
		TempJoyIsAnalog[0] = JoyIsAnalog[0];
		TempJoyIsAnalog[1] = JoyIsAnalog[1];

		hMsgParent = GetParent(hWin);
		return true;

	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code)
		{
		case PSN_APPLY:
			SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETLBTEXT, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), (LPARAM)szTempKeyScheme[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0)]);
			GetSelectedKey(hWin, false, false);
			CopyMemory(&Keys[0], &TempKeys[0], sizeof(KEYS));
			CopyMemory(&AutoFireKeys[0], &TempKeys[1], sizeof(KEYS));
			CopyMemory(&Keys[1], &TempKeys[2], sizeof(KEYS));
			CopyMemory(&AutoFireKeys[1], &TempKeys[3], sizeof(KEYS));
			CopyMemory(&JoyButtons[0], &TempJoyButtons[0], sizeof(KEYS));
			CopyMemory(&AutoFireJoyButtons[0], &TempJoyButtons[1], sizeof(KEYS));
			CopyMemory(&JoyButtons[1], &TempJoyButtons[2], sizeof(KEYS));
			CopyMemory(&AutoFireJoyButtons[1], &TempJoyButtons[3], sizeof(KEYS));
			CopyMemory(JoyGuid, TempGuid, sizeof(GUID[2]));
			strcpy(szKeyScheme[0], szTempKeyScheme[0]);
			strcpy(szKeyScheme[1], szTempKeyScheme[1]);
			TempJoyIsAnalog[SendDlgItemMessage(hWin, IDC_PLAYER, CB_GETCURSEL, 0, 0)] = SendDlgItemMessage(hWin, IDC_JOYSTICK_ANALOG, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			JoyIsAnalog[0] = TempJoyIsAnalog[0];
			JoyIsAnalog[1] = TempJoyIsAnalog[1];
			return true;

		case PSN_SETACTIVE:
			if ((dwPos = SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_GETCURSEL, 0, 0)) == LB_ERR)
			{
				dwPos = 0;
			}
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_UP));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_DOWN));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_LEFT));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_RIGHT));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_A));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_B));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_START));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_SELECT));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_FASTFORWARD));
			SendDlgItemMessage(hWin, IDC_BUTTONLIST, LB_SETCURSEL, dwPos, 0);

			SetWindowText(GetDlgItem(hWin, IDC_SAVESCHEMEAS), String(IDS_OPTIONS_KEYS_SAVESCHEMEAS));
			SetWindowText(GetDlgItem(hWin, IDC_DELETESCHEME), String(IDS_OPTIONS_KEYS_DELETESCHEME));
			SetWindowText(GetDlgItem(hWin, IDC_JOYSTICK_ANALOG), String(IDS_OPTIONS_KEYS_JOYSTICK_ANALOG));
			SetWindowText(GetDlgItem(hWin, IDC_KEYBOARD_STATIC), String(IDS_OPTIONS_KEYS_KEYBOARD));
			SetWindowText(GetDlgItem(hWin, IDC_JOYSTICK_STATIC), String(IDS_OPTIONS_KEYS_JOYSTICK));

			SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_SETITEMDATA, SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_JOYSTICK_NONE)), NULL);
			SendDlgItemMessage(hWin, IDC_INPUTDEVICES, CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hWin, IDC_DEVICE_BUTTONS), false);

			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_ADDSTRING, 0, (LPARAM)String(IDS_OPTIONS_KEYS_NOKEY));
			SendDlgItemMessage(hWin, IDC_DEVICE_BUTTONS, CB_SETCURSEL, 0, 0);

			if (lpdi)
			{
				lpdi->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDeviceProc, hWin, DIEDFL_ATTACHEDONLY);
			}

			SendDlgItemMessage(hWin, IDC_SCHEME, CB_RESETCONTENT, 0, 0);
			dwPos = 0;
			pKeySchemes->ResetSearch();
			while (pKeyScheme = (KEYSCHEME *)pKeySchemes->GetNextItem())
			{
				if (dwPos)
				{
					SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_ADDSTRING, 0, (LPARAM)pKeyScheme->szName), (LPARAM)pKeyScheme);
				}
				else
				{
					SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETITEMDATA, dwPos = SendDlgItemMessage(hWin, IDC_SCHEME, CB_ADDSTRING, 0, (LPARAM)pKeyScheme->szName), (LPARAM)pKeyScheme);
					if (!stricmp(pKeyScheme->szName, szTempKeyScheme[0]))
					{
						SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, dwPos, 0);
						dwPos = 1;
					}
					else
					{
						dwPos = 0;
					}
				}
			}
			if (dwPos == 0)
			{
				SendDlgItemMessage(hWin, IDC_SCHEME, CB_SETCURSEL, 0, 0);
			}
			if (((KEYSCHEME *)SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_SCHEME, CB_GETCURSEL, 0, 0), 0))->BuiltIn)
			{
				EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), false);
			}
			else
			{
				EnableWindow(GetDlgItem(hWin, IDC_DELETESCHEME), true);
			}

			RefreshKeys(hWin, true);

			return true;
		}
	}

	return false;
}



BOOL CALLBACK FileOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD		dwSelection;
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_REGISTERGBROM:
		case IDC_REGISTERSAVESTATE:
		case IDC_AUTOLOADBATTERY:
		case IDC_AUTOLOADSTATE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;

		case IDC_SAVEBATTERY:
		case IDC_SAVESTATE:
		case IDC_GBTYPE:
		case IDC_AUTOSTART:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		break;

	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code)
		{
		case PSN_APPLY:
			if (Settings.GbFile = SendDlgItemMessage(hWin, IDC_REGISTERGBROM, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false)
			{
				RegisterGbFileType();
			}
			else
			{
				UnRegisterGbFileType();
			}
			if (Settings.GlsFile = SendDlgItemMessage(hWin, IDC_REGISTERSAVESTATE, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false)
			{
				RegisterGlsFileType();
			}
			else
			{
				UnRegisterGlsFileType();
			}

			Settings.AutoLoadBattery = SendDlgItemMessage(hWin, IDC_AUTOLOADBATTERY, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			Settings.AutoLoadState = SendDlgItemMessage(hWin, IDC_AUTOLOADSTATE, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

			Settings.SaveBattery = (BYTE)SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_GETCURSEL, 0, 0);
			Settings.SaveState = (BYTE)SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_GETCURSEL, 0, 0);

			Settings.GBType = SendDlgItemMessage(hWin, IDC_GBTYPE, CB_GETCURSEL, 0, 0) == 1 ? GB_COLOR : 0;

			switch (SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_GETCURSEL, 0, 0))
			{
			case 0:
				Settings.AutoStart = 0;
				break;
			case 1:
				Settings.AutoStart = AUTOSTART_DEBUG;
				break;
			case 2:
				Settings.AutoStart = AUTOSTART_EXECUTE;
				break;
			}

			return true;

		case PSN_SETACTIVE:
			SetWindowText(GetDlgItem(hWin, IDC_FILEASSOC_FRAME), String(IDS_FILEASSOC_FRAME));
			SetWindowText(GetDlgItem(hWin, IDC_REGISTERGBROM), String(IDS_FILEASSOC_GB));
			SetWindowText(GetDlgItem(hWin, IDC_REGISTERSAVESTATE), String(IDS_FILEASSOC_GLS));
			SetWindowText(GetDlgItem(hWin, IDC_SAVE_FRAME), String(IDS_SAVE_FRAME));
			SetWindowText(GetDlgItem(hWin, IDC_AUTOLOADBATTERY), String(IDS_SAVE_LOADBATTERY));
			SetWindowText(GetDlgItem(hWin, IDC_AUTOLOADSTATE), String(IDS_SAVE_LOADSTATE));
			SetWindowText(GetDlgItem(hWin, IDC_SAVEBATTERY_STATIC), String(IDS_SAVE_SAVEBATTERY));
			SetWindowText(GetDlgItem(hWin, IDC_SAVESTATE_STATIC), String(IDS_SAVE_SAVESTATE));
			SetWindowText(GetDlgItem(hWin, IDC_MISC_FRAME), String(IDS_MISC_FRAME));
			SetWindowText(GetDlgItem(hWin, IDC_GBTYPE_STATIC), String(IDS_MISC_HARDWARE));
			SetWindowText(GetDlgItem(hWin, IDC_AUTOSTART_STATIC), String(IDS_MISC_AUTOSTART));

			dwSelection = SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_NO));
			SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_YES));
			SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_PROMPT));
			SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_SETCURSEL, dwSelection, 0);
			dwSelection = SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_NO));
			SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_YES));
			SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_PROMPT));
			SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_SETCURSEL, dwSelection, 0);
			dwSelection = SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_STOPPED));
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_STARTDEBUG));
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_EXECUTE));
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, dwSelection, 0);
			return true;
		}
		break;

	/*case WM_HELP:
		if (((HELPINFO *)lParam)->iCtrlId == IDC_REGISTERGBROM)
		{
		}
		break;*/

	case WM_INITDIALOG:
		SendDlgItemMessage(hWin, IDC_REGISTERGBROM, BM_SETCHECK, Settings.GbFile ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hWin, IDC_REGISTERSAVESTATE, BM_SETCHECK, Settings.GlsFile ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hWin, IDC_AUTOLOADBATTERY, BM_SETCHECK, Settings.AutoLoadBattery ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hWin, IDC_AUTOLOADSTATE, BM_SETCHECK, Settings.AutoLoadState ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_NO));
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_YES));
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_ADDSTRING, 0, (LPARAM)String(IDS_PROMPT));
		SendDlgItemMessage(hWin, IDC_SAVEBATTERY, CB_SETCURSEL, Settings.SaveBattery, 0);
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_NO));
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_YES));
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_ADDSTRING, 0, (LPARAM)String(IDS_PROMPT));
		SendDlgItemMessage(hWin, IDC_SAVESTATE, CB_SETCURSEL, Settings.SaveState, 0);

		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_ADDSTRING, 0, (long)"Game Boy");
		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_ADDSTRING, 0, (long)"Game Boy Color");
		SendDlgItemMessage(hWin, IDC_GBTYPE, CB_SETCURSEL, Settings.GBType == GB_COLOR ? 1 : 0, 0);

		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_STOPPED));
		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_STARTDEBUG));
		SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_ADDSTRING, 0, (LPARAM)String(IDS_EXECUTE));
		switch (Settings.AutoStart)
		{
		case 0:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 0, 0);
			break;
		case AUTOSTART_DEBUG:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 1, 0);
			break;
		case AUTOSTART_EXECUTE:
			SendDlgItemMessage(hWin, IDC_AUTOSTART, CB_SETCURSEL, 2, 0);
			break;
		}

		hMsgParent = GetParent(hWin);
		break;
	}

	return false;
}



BOOL CALLBACK GeneralOptionsDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char				*pszFilename, szFilename[MAX_PATH], **pszLanguage, szBuffer[0x100], szBuffer2[0x100];
	WIN32_FIND_DATA		FindFileData;
	HANDLE				hFindFile;
	HINSTANCE			hTempInstance;
	TCITEM				tci;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LANGUAGELIST:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);

				pszFilename = (char *)SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETCURSEL, 0, 0), 0);
				hTempInstance = NULL;
				if (pszFilename)
				{
					hTempInstance = LoadLibrary(pszFilename);
				}
				LoadString(hTempInstance ? hTempInstance : hInstance, IDS_TRANSLATOR_NAME, szBuffer, sizeof(szBuffer));
				if (hTempInstance)
				{
					FreeLibrary(hTempInstance);
				}
				SetWindowText(GetDlgItem(hWin, IDC_TRANSLATOR), LoadString(IDS_OPTIONS_LANGUAGE_TRANSLATEDBY, szBuffer2, sizeof(szBuffer2), szBuffer));
			}
			return true;

		case IDC_FRAMESKIP:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		break;

	case WM_NOTIFY:
		if (wParam == IDC_FRAMESKIPSPIN)
		{
			if (((NMUPDOWN *)lParam)->hdr.code == UDN_DELTAPOS)
			{
				SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_GETTEXT, sizeof(NumBuffer), (LPARAM)&NumBuffer);
				if (atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta < 0)
				{
					SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)&"0");
				}
				else
				{
					if (atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta > 9)
					{
						SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)&"9");
					}
					else
					{
						SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)itoa(atoi(NumBuffer) - ((NMUPDOWN *)lParam)->iDelta, NumBuffer, 10));
					}
				}
				SetWindowLong(hWin, DWL_MSGRESULT, false);
				SendMessage(GetParent(hWin), PSM_CHANGED, (WPARAM)hWin, 0);
			}
			return true;
		}
		switch (((NMHDR *)lParam)->code)
		{
		case PSN_APPLY:
			pszFilename = (char *)SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETCURSEL, 0, 0), 0);
			if (hStringInstance)
			{
				FreeLibrary(hStringInstance);
				hStringInstance = NULL;
			}
			if (pszFilename)
			{
				if (hStringInstance = LoadLibrary(pszFilename))
				{
					strcpy(szStringFilename, pszFilename);
				}
				else
				{
					szStringFilename[0] = '\0';
				}
			}
			else
			{
				szStringFilename[0] = '\0';
			}
			ChangeLanguage();
			PropSheet_SetTitle(GetParent(hWin), 0, String(IDS_OPTIONS));
			tci.mask = TCIF_TEXT;
			tci.pszText = String(IDS_OPTIONS_KEYS);
			TabCtrl_SetItem(PropSheet_GetTabControl(GetParent(hWin)), 0, &tci);
			tci.pszText = String(IDS_OPTIONS_FILE);
			TabCtrl_SetItem(PropSheet_GetTabControl(GetParent(hWin)), 1, &tci);
			tci.pszText = String(IDS_OPTIONS_GENERAL);
			TabCtrl_SetItem(PropSheet_GetTabControl(GetParent(hWin)), 2, &tci);

			SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_GETTEXT, sizeof(NumBuffer), (LPARAM)&NumBuffer);
			Settings.FrameSkip = atoi(NumBuffer) < 0 ? 0 : atoi(NumBuffer) > 9 ? 9 : atoi(NumBuffer);

			//return true;

		case PSN_SETACTIVE:
			SetWindowText(GetDlgItem(hWin, IDC_LANGUAGE), String(IDS_OPTIONS_LANGUAGE));
			SetWindowText(GetDlgItem(hWin, IDC_MISC_FRAME), String(IDS_MISC_FRAME));
			SetWindowText(GetDlgItem(hWin, IDC_FRAMESKIP_STATIC), String(IDS_MISC_FRAMESKIP));

			pszFilename = (char *)SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETITEMDATA, SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETCURSEL, 0, 0), 0);
			hTempInstance = NULL;
			if (pszFilename)
			{
				hTempInstance = LoadLibrary(pszFilename);
			}
			LoadString(hTempInstance ? hTempInstance : hInstance, IDS_TRANSLATOR_NAME, szBuffer, sizeof(szBuffer));
			if (hTempInstance)
			{
				FreeLibrary(hTempInstance);
			}
			SetWindowText(GetDlgItem(hWin, IDC_TRANSLATOR), LoadString(IDS_OPTIONS_LANGUAGE_TRANSLATEDBY, szBuffer2, sizeof(szBuffer2), szBuffer));
			return true;
		}
		break;

	case WM_INITDIALOG:
		SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_ADDSTRING, 0, (LPARAM)"English (Default)");
		SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_SETCURSEL, 0, 0);
		GetModuleFileName(hInstance, szFilename, sizeof(szFilename));
		if (strchr(szFilename, '\\'))
		{
			*strrchr(szFilename, '\\') = '\0';
		}
		strcat(szFilename, "\\*.dll");
		if ((hFindFile = FindFirstFile(szFilename, &FindFileData)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (hTempInstance = LoadLibrary(FindFileData.cFileName))
				{
					if (pszLanguage = (char **)GetProcAddress(hTempInstance, "GameLadLanguage"))
					{
						if (pszFilename = new char[MAX_PATH])
						{
							strcpy(pszFilename, FindFileData.cFileName);
							SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_ADDSTRING, 0, (LPARAM)*pszLanguage);
							SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_SETITEMDATA, SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_FINDSTRING, -1, (LPARAM)*pszLanguage), (LPARAM)pszFilename);
							if (!strcmp(FindFileData.cFileName, szStringFilename))
							{
								SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_SETCURSEL, SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_FINDSTRING, -1, (LPARAM)*pszLanguage), 0);
							}
						}
					}
					FreeLibrary(hTempInstance);
				}
			}
			while (FindNextFile(hFindFile, &FindFileData));
			FindClose(hFindFile);
		}

		pStaticWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDC_TRANSLATOR), GWL_WNDPROC, (long)LinkWndProc);

		SendDlgItemMessage(hWin, IDC_FRAMESKIP, WM_SETTEXT, 0, (LPARAM)itoa(Settings.FrameSkip, NumBuffer, 10));

		hMsgParent = GetParent(hWin);
		break;

	case WM_DESTROY:
		while (SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETCOUNT, 0, 0))
		{
			delete (void *)SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_GETITEMDATA, 0, 0);
			SendDlgItemMessage(hWin, IDC_LANGUAGELIST, CB_DELETESTRING, 0, 0);
		}
		break;
	}

	return false;
}



char *GetFileTime(char *pszBuffer)
{
	HANDLE				hFile;
	FILETIME			FileTime, LocalFileTime;
	SYSTEMTIME			SystemTime;


	if ((hFile = CreateFile(pszBuffer, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		return LoadString(IDS_FILE_STATESLOT_EMPTY, pszBuffer, 0x100);
	}

	GetFileTime(hFile, NULL, NULL, &FileTime);
	CloseHandle(hFile);
	FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
	FileTimeToSystemTime(&LocalFileTime, &SystemTime);
	ultoa(SystemTime.wYear, pszBuffer, 10);
	if (SystemTime.wMonth <= 9)
	{
		strcat(pszBuffer, "-0");
	}
	else
	{
		strcat(pszBuffer, "-");
	}
	ultoa(SystemTime.wMonth, pszBuffer + strlen(pszBuffer), 10);
	if (SystemTime.wDay <= 9)
	{
		strcat(pszBuffer, "-0");
	}
	else
	{
		strcat(pszBuffer, "-");
	}
	ultoa(SystemTime.wDay, pszBuffer + strlen(pszBuffer), 10);
	if (SystemTime.wHour <= 9)
	{
		strcat(pszBuffer, " 0");
	}
	else
	{
		strcat(pszBuffer, " ");
	}
	ultoa(SystemTime.wHour, pszBuffer + strlen(pszBuffer), 10);
	if (SystemTime.wMinute <= 9)
	{
		strcat(pszBuffer, ":0");
	}
	else
	{
		strcat(pszBuffer, ":");
	}
	ultoa(SystemTime.wMinute, pszBuffer + strlen(pszBuffer), 10);
	if (SystemTime.wSecond <= 9)
	{
		strcat(pszBuffer, ":0");
	}
	else
	{
		strcat(pszBuffer, ":");
	}
	ultoa(SystemTime.wSecond, pszBuffer + strlen(pszBuffer), 10);

	return pszBuffer;
}



LRESULT CALLBACK WndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CLIENTCREATESTRUCT	ccs;
	OPENFILENAME		of;
	char				Filename[MAX_PATH], szBuffer[0x100], szBuffer2[0x100], szBuffer3[0x100], szBuffer4[0x100];
	DWORD				ItemNo, dw, dw2;
	CGameBoy			*pGameBoy;
	MENUITEMINFO		mii;
	HWND				hTempWnd;
	PROPSHEETPAGE		psp[3];
	PROPSHEETHEADER		psh;
	RECT				rct;


	switch (uMsg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
		{
			RestoreInput();
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_OPEN:
			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWin;
			LoadString(IDS_OPEN_GBROM, szBuffer2, sizeof(szBuffer2));
			dw = strlen(szBuffer2) + 1;
			strcpy(szBuffer2 + dw, "*.GB;*.GBC");
			dw += strlen(szBuffer2 + dw) + 1;
			LoadString(IDS_OPEN_ALLFILES, szBuffer2 + dw, sizeof(szBuffer2) - dw);
			dw += strlen(szBuffer2 + dw) + 1;
			strcpy(szBuffer2 + dw, "*.*");
			dw += strlen(szBuffer2 + dw) + 1;
			*(szBuffer2 + dw) = '\0';
			of.lpstrFilter = szBuffer2;
			of.nFilterIndex = 1;
			of.lpstrFile = Filename;
			of.nMaxFile = sizeof(Filename);
			of.lpstrTitle = String(IDS_OPEN);
			of.Flags = OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
			of.lpfnHook = OFNHookProc;
			Filename[0] = 0;
			if (!GetOpenFileName(&of))
			{
				return 0;
			}
			if (!*(Filename + strlen(Filename) + 1))
			{
				//One file selected

				GameBoys.NewGameBoy(Filename, LoadState ? "" : NULL, LoadBattery ? "" : NULL, GBType, AutoStart);
			}
			else
			{
				//Multiple files selected

				*(Filename + strlen(Filename)) = '\\';
				dw = strlen(Filename) + 1;
				GameBoys.NewGameBoy(Filename, LoadState ? "" : NULL, LoadBattery ? "" : NULL, GBType, AutoStart);
				while (*(Filename + dw))
				{
					dw2 = dw;
					dw += strlen(Filename + dw) + 1;
					MoveMemory(strrchr(Filename, '\\') + 1, Filename + dw2, strlen(Filename + dw2) + 1);
					GameBoys.NewGameBoy(Filename, LoadState ? "" : NULL, LoadBattery ? "" : NULL, GBType, AutoStart);
				}
			}
			return 0;

		case ID_FILE_CLOSE:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
			{
				SendMessage(hTempWnd, WM_CLOSE, 0, 0);
			}
			return 0;

		case ID_FILE_SAVESTATE:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SaveState();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_LOADSTATE:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->LoadState();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_SAVESTATEAS:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SaveStateAs();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_LOADSTATEAS:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->LoadStateAs();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_STATESLOT_0:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(0);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_1:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(1);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_2:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(2);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_3:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(3);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_4:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(4);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_5:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(5);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_6:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(6);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_7:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(7);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_8:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(8);
				pGameBoy->Release();
			}
			return 0;
		case ID_FILE_STATESLOT_9:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetStateSlot(9);
				pGameBoy->Release();
			}
			return 0;

		case ID_SAVESTATESLOT:
			F4Pressed = GameBoys.GetActive(false);
			SetStatus(NULL, SF_F4PRESSED);
			return 0;

		case ID_FILE_LOADBATTERY:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				if (!pGameBoy->SaveBattery(true, false))
				{
					ZeroMemory(&of, sizeof(of));
					of.lStructSize = sizeof(of);
					of.hwndOwner = hWin;
					String(IDS_OPEN_BATTERY);
					dw = strlen(szBuffer) + 1;
					strcpy(szBuffer + dw, "*.SAV");
					dw += strlen(szBuffer + dw) + 1;
					LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
					dw += strlen(szBuffer + dw) + 1;
					strcpy(szBuffer + dw, "*.*");
					dw += strlen(szBuffer + dw) + 1;
					*(szBuffer + dw) = '\0';
					of.lpstrFilter = szBuffer;
					of.nFilterIndex = 1;
					of.lpstrFile = Filename;
					of.nMaxFile = sizeof(Filename);
					of.lpstrTitle = LoadString(IDS_OPEN, szBuffer2, sizeof(szBuffer2));
					of.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
					of.lpfnHook = OFNHookProc;
					pGameBoy->GetBatteryFilename(Filename);
					if (GetOpenFileName(&of))
					{
						pGameBoy->LoadBattery(Filename);
					}
				}
				pGameBoy->Resume();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_SAVEBATTERY:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				pGameBoy->SaveBattery(false, false);
				pGameBoy->Resume();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_SAVEBATTERYAS:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				pGameBoy->SaveBattery(false, true);
				pGameBoy->Resume();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_CLEARBATTERY:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				if (!pGameBoy->SaveBattery(true, false))
				{
					pGameBoy->LoadBattery(NULL);
				}
				pGameBoy->Resume();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_SAVESNAPSHOTAS:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SaveSnapshot();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_SAVEVIDEOAS:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SaveVideo();
				pGameBoy->Release();
			}
			return 0;

		case ID_FILE_MERGECHEATS:
			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWin;
			String(IDS_OPEN_CHEATXML);
			dw = strlen(szBuffer) + 1;
			strcpy(szBuffer + dw, "*.XML");
			dw += strlen(szBuffer + dw) + 1;
			LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
			dw += strlen(szBuffer + dw) + 1;
			strcpy(szBuffer + dw, "*.*");
			dw += strlen(szBuffer + dw) + 1;
			*(szBuffer + dw) = '\0';
			of.lpstrFilter = szBuffer;
			of.nFilterIndex = 1;
			of.lpstrFile = Filename;
			of.nMaxFile = sizeof(Filename);
			of.lpstrTitle = LoadString(IDS_OPEN, szBuffer2, sizeof(szBuffer2));
			of.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
			Filename[0] = '\0';
			if (GetOpenFileName(&of))
			{
				Cheats.MergeFile(of.lpstrFile);
			}
			return 0;

		case ID_FILE_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;

		case ID_EDIT_GOTO:
			SendMessage((HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0), uMsg, wParam, lParam);
			return 0;

		case ID_VIEW_REGISTERS:
			if (!hRegisters)
			{
				if (!(hRegisters = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Registers", String(IDS_WND_REGISTERS), WS_VISIBLE | WS_CAPTION | WS_BORDER, Registers.x, Registers.y, Registers.Width, Registers.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hRegisters, 0);
			}
			return 0;

		case ID_VIEW_DISASSEMBLY:
			if (!hDisAsm)
			{
				if (!(hDisAsm = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "DisAsm", String(IDS_WND_DISASSEMBLY), WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL /*| WS_HSCROLL*/, DisAsm.x, DisAsm.y, DisAsm.Width, DisAsm.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
			}
			return 0;

		case ID_VIEW_MEMORY:
			if (!hMemory)
			{
				if (!(hMemory = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Memory", String(IDS_WND_MEMORY), WS_VISIBLE | WS_CAPTION | WS_BORDER, Memory.x, Memory.y, Memory.Width, Memory.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hMemory, 0);
			}
			return 0;

		case ID_VIEW_TILES:
			if (!hTiles)
			{
				if (!(hTiles = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Tiles", String(IDS_WND_TILES), WS_VISIBLE | WS_CAPTION | WS_BORDER, Tiles.x, Tiles.y, Tiles.Width, Tiles.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hTiles, 0);
			}
			return 0;

		case ID_VIEW_TILEMAP:
			if (!hTileMap)
			{
				if (!(hTileMap = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, "TileMap", String(IDS_WND_TILEMAP), WS_VISIBLE | WS_CAPTION | WS_BORDER, TileMap.x, TileMap.y, TileMap.Width, TileMap.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hTileMap, 0);
			}
			return 0;

		case ID_VIEW_PALETTES:
			if (!hPalettes)
			{
				if (!(hPalettes = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Palettes", String(IDS_WND_PALETTES), WS_VISIBLE | WS_CAPTION | WS_BORDER /*| WS_VSCROLL | WS_HSCROLL*/, Palettes.x, Palettes.y, Palettes.Width, Palettes.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hPalettes, 0);
			}
			return 0;

		case ID_VIEW_HARDWARE:
			if (!hHardware)
			{
				if (!(hHardware = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE, "Hardware", String(IDS_WND_HARDWARE), WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_VSCROLL | WS_HSCROLL, Hardware.x, Hardware.y, Hardware.Width, Hardware.Height, hClientWnd, NULL, hInstance, NULL)))
				{
					DisplayErrorMessage();
				}
			}
			else
			{
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hHardware, 0);
			}
			return 0;

		case ID_VIEW_STATUSBAR:
			if (GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE)
			{
				ShowWindow(hStatusWnd, false);
				StatusBarAppearance = false;
			}
			else
			{
				ShowWindow(hStatusWnd, true);
				StatusBarAppearance = true;
			}
			GetClientRect(hWin, &rct);
			SendMessage(hWin, WM_SIZE, 0, (rct.bottom << 16) | (rct.right & 0xFFFF));
			return 0;

		case ID_VIEW_ZOOM_100:
		case ID_VIEW_ZOOM_200:
		case ID_VIEW_ZOOM_300:
		case ID_VIEW_ZOOM_400:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
			{
				SendMessage(hTempWnd, WM_COMMAND, wParam, lParam);
			}
			return 0;

		case ID_VIEW_FRAMESKIP_0:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(0);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_1:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(1);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_2:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(2);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_3:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(3);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_4:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(4);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_5:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(5);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_6:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(6);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_7:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(7);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_8:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(8);
				pGameBoy->Release();
			}
			return 0;
		case ID_VIEW_FRAMESKIP_9:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->SetFrameSkip(9);
				pGameBoy->Release();
			}
			return 0;

		case ID_EMULATION_STARTDEBUG:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->StartDebug();
				pGameBoy->Release();
			}
			return 0;

		case ID_EMULATION_EXECUTE:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Execute();
				pGameBoy->Release();
			}
			return 0;

		case ID_EMULATION_STOP:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				pGameBoy->Release();
			}
			return 0;

		case ID_EMULATION_STEPINTO:
		case ID_EMULATION_STEPOVER:
		case ID_EMULATION_STEPOUT:
		case ID_EMULATION_RUNTOCURSOR:
		case ID_EMULATION_TOGGLEBREAKPOINT:
		case ID_EMULATION_SETNEXTSTATEMENT:
			if (hDisAsm)
			{
				SendMessage(hDisAsm, WM_COMMAND, wParam, lParam);
			}
			return 0;

		case ID_MEMORY_ROM:
		case ID_MEMORY_VBK_BANK0:
		case ID_MEMORY_VBK_BANK1:
		case ID_MEMORY_RAM_BANK0:
		case ID_MEMORY_RAM_BANK1:
		case ID_MEMORY_RAM_BANK2:
		case ID_MEMORY_RAM_BANK3:
		case ID_MEMORY_RAM_BANK4:
		case ID_MEMORY_RAM_BANK5:
		case ID_MEMORY_RAM_BANK6:
		case ID_MEMORY_RAM_BANK7:
		case ID_MEMORY_RAM_BANK8:
		case ID_MEMORY_RAM_BANK9:
		case ID_MEMORY_RAM_BANK10:
		case ID_MEMORY_RAM_BANK11:
		case ID_MEMORY_RAM_BANK12:
		case ID_MEMORY_RAM_BANK13:
		case ID_MEMORY_RAM_BANK14:
		case ID_MEMORY_RAM_BANK15:
		case ID_MEMORY_SVBK_BANK1:
		case ID_MEMORY_SVBK_BANK2:
		case ID_MEMORY_SVBK_BANK3:
		case ID_MEMORY_SVBK_BANK4:
		case ID_MEMORY_SVBK_BANK5:
		case ID_MEMORY_SVBK_BANK6:
		case ID_MEMORY_SVBK_BANK7:
			if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0))
			{
				return SendMessage(hTempWnd, uMsg, wParam, lParam);
			}
			break;

		case ID_EMULATION_RESET:
			if (pGameBoy = GameBoys.GetActive(true))
			{
				pGameBoy->Stop();
				pGameBoy->Reset();
				pGameBoy->Resume();
				pGameBoy->Release();
			}
			return 0;

		case ID_TOOLS_OPTIONS:
			CloseInput();
			InitInput();
			psp[0].dwSize = sizeof(psp[0]);
			psp[0].dwFlags = PSP_USETITLE;
			psp[0].hInstance = hInstance;
			psp[0].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_KEYS);
			psp[0].pszTitle = LoadString(IDS_OPTIONS_KEYS, szBuffer2, sizeof(szBuffer2));
			psp[0].pfnDlgProc = KeyOptionsDlgProc;
			psp[1].dwSize = sizeof(psp[1]);
			psp[1].dwFlags = PSP_USETITLE;
			psp[1].hInstance = hInstance;
			psp[1].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_FILE);
			psp[1].pszTitle = LoadString(IDS_OPTIONS_FILE, szBuffer3, sizeof(szBuffer3));
			psp[1].pfnDlgProc = FileOptionsDlgProc;
			psp[2].dwSize = sizeof(psp[2]);
			psp[2].dwFlags = PSP_USETITLE;
			psp[2].hInstance = hInstance;
			psp[2].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_GENERAL);
			psp[2].pszTitle = LoadString(IDS_OPTIONS_GENERAL, szBuffer4, sizeof(szBuffer4));
			psp[2].pfnDlgProc = GeneralOptionsDlgProc;
			psh.dwSize = sizeof(psh);
			psh.dwFlags = PSH_PROPSHEETPAGE;
			psh.hwndParent = hWin;
			psh.hInstance = hInstance;
			psh.pszCaption = String(IDS_OPTIONS);
			psh.nPages = 3;
			psh.nStartPage = 0;
			psh.ppsp = psp;
			PropertySheet(&psh);
			hMsgParent = hWnd;
			if (pJoySticks)
			{
				delete pJoySticks;
				pJoySticks = NULL;
			}
			InitInputDevice(JoyGuid[0], NULL, 0);
			InitInputDevice(JoyGuid[1], &JoyGuid[0], 1);
			return 0;

		case ID_TOOLS_LINKCABLE:
			if (Settings.LinkCable)
			{
				Settings.LinkCable = false;
			}
			else
			{
				Settings.LinkCable = true;
			}
			return 0;

		case ID_TOOLS_SOUNDENABLED:
			if (!Settings.SoundEnabled)
			{
				InitSound();
			}
			else
			{
				GameBoys.CloseSound();
			}
			return 0;

		case ID_TOOLS_CHEAT:
			Cheats.ShowCheatDialog();
			return 0;

		case ID_WINDOW_NEXT:
			return SendMessage(hClientWnd, WM_MDINEXT, NULL, 0);
		case ID_WINDOW_PREVIOUS:
			return SendMessage(hClientWnd, WM_MDINEXT, NULL, 1);

		case ID_WINDOW_CASCADE:
			return SendMessage(hClientWnd, WM_MDICASCADE, 0, 0);
		case ID_WINDOW_TILEHORIZONTALLY:
			return SendMessage(hClientWnd, WM_MDITILE, MDITILE_HORIZONTAL, 0);
		case ID_WINDOW_TILEVERTICALLY:
			return SendMessage(hClientWnd, WM_MDITILE, MDITILE_VERTICAL, 0);

		case ID_HELP_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWin, AboutDialogProc);
			return 0;
		}
		break;

	case WM_DROPFILES:
		ItemNo = 0;
		while (ItemNo < DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0))
		{
			DragQueryFile((HDROP)wParam, ItemNo, Filename, sizeof(Filename));
			GameBoys.NewGameBoy(Filename, Settings.AutoLoadState ? "" : NULL, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
			ItemNo++;
		}
		return 0;

	case WM_APP_DDEOPENFILE:
		GameBoys.NewGameBoy(((DDEGAMEBOY *)lParam)->szROM,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_STATE ? ((DDEGAMEBOY *)lParam)->szState : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_NOSTATE ? NULL : Settings.AutoLoadState ? "" : NULL,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_BATTERY ? ((DDEGAMEBOY *)lParam)->szBattery : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_NOBATTERY ? NULL : Settings.AutoLoadBattery ? "" : NULL,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_DMG ? 0 : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_GBC ? GB_COLOR : Settings.GBType,
			((DDEGAMEBOY *)lParam)->Flags & DDEGB_LOAD ? 0 : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_DEBUG ? AUTOSTART_DEBUG : ((DDEGAMEBOY *)lParam)->Flags & DDEGB_EXECUTE ? AUTOSTART_EXECUTE : Settings.AutoStart);
		delete (DDEGAMEBOY *)lParam;
		return 0;

	case WM_APP_REFRESHDEBUG:
		MemoryFlags = 0;
		if (hMemory)
		{
			InvalidateRect(hMemory, NULL, true);
		}
		if (hRegisters)
		{
			InvalidateRect(hRegisters, NULL, true);
		}
		if (hDisAsm)
		{
			InvalidateRect(hDisAsm, NULL, true);
		}
		if (hTiles)
		{
			InvalidateRect(hTiles, NULL, true);
		}
		if (hTileMap)
		{
			InvalidateRect(hTileMap, NULL, true);
		}
		if (hPalettes)
		{
			InvalidateRect(hPalettes, NULL, true);
		}
		if (hHardware)
		{
			InvalidateRect(hHardware, NULL, true);
		}
		return 0;

	case WM_MENUSELECT:
		SetStatus(NULL, SF_CLEAR);
		if (!(HIWORD(wParam) & MF_POPUP))
		{
			ItemNo = 0;
			while (MenuHelpStrings[ItemNo].uiCommand)
			{
				if (MenuHelpStrings[ItemNo].uiCommand == LOWORD(wParam))
				{
					SetStatus(String(MenuHelpStrings[ItemNo].uiString), SF_MESSAGE);
					break;
				}
				ItemNo++;
			}
		}
		return 0;

	case WM_EXITMENULOOP:
		SetStatus(NULL, SF_READY);
		return 0;

	case WM_INITMENU:
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STATE;
		if (pGameBoy = GameBoys.GetActive(true))
		{
			mii.fState = pGameBoy->HasBattery() ? MFS_ENABLED : MFS_GRAYED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);

			if (pGameBoy->IsEmulating())
			{
				mii.fState = MFS_ENABLED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
				mii.fState = MFS_GRAYED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
			}
			else
			{
				mii.fState = MFS_ENABLED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
				mii.fState = MFS_GRAYED;
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
				if (hDisAsm == (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, 0))
				{
					mii.fState = MFS_ENABLED;
				}
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
				SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
			}

			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->GetStateSlot() == 0 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_0_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 0)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_0, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_1_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 1)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_1, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_2_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 2)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_2, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_3_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 3)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_3, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_4_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 4)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_4, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 5 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_5_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 5)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_5, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 6 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_6_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 6)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_6, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 7 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_7_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 7)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_7, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 8 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_8_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 8)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_8, false, &mii);
			mii.fState = pGameBoy->GetStateSlot() == 9 ? MFS_CHECKED : MFS_UNCHECKED;
			mii.dwTypeData = LoadString(IDS_FILE_STATESLOT_9_MENU, szBuffer, sizeof(szBuffer), GetFileTime(pGameBoy->GetStateFilename(szBuffer2, 9)));
			SetMenuItemInfo((HMENU)wParam, ID_FILE_STATESLOT_9, false, &mii);
			mii.fMask = MIIM_STATE;

			mii.fState = MFS_ENABLED;
		}
		else
		{
			mii.fState = MFS_GRAYED;
			SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEBATTERYAS, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_FILE_CLEARBATTERY, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STARTDEBUG, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_EXECUTE, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STOP, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPINTO, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOVER, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_STEPOUT, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RUNTOCURSOR, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_SETNEXTSTATEMENT, false, &mii);
			SetMenuItemInfo((HMENU)wParam, ID_EMULATION_TOGGLEBREAKPOINT, false, &mii);
		}
		SetMenuItemInfo((HMENU)wParam, ID_EMULATION_RESET, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVESTATE, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADSTATE, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVESTATEAS, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_LOADSTATEAS, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVESNAPSHOTAS, false, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_FILE_SAVEVIDEOAS, false, &mii);
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 0), 7, true, &mii);
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 11, true, &mii);
		if (mii.fState == MFS_ENABLED)
		{
			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.dwTypeData = szBuffer;
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_0, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 0 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_0, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_1, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_1, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_2, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_2, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_3, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_3, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_4, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_4, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_5, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 5 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_5, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_6, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 6 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_6, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_7, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 7 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_7, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_8, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 8 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_8, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_9, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = pGameBoy->FrameSkip == 9 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_FRAMESKIP_9, false, &mii);
			mii.fMask = MIIM_STATE;
		}
		if (hTempWnd = (HWND)SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, NULL))
		{
			if (hTempWnd == hDisAsm || hTempWnd == hMemory || hTempWnd == hRegisters || hTempWnd == hPalettes || hTempWnd == hHardware
				|| hTempWnd == hTileMap || hTempWnd == hTiles)
			{
				mii.fState = MFS_GRAYED;
			}
			else
			{
				mii.fState = MFS_ENABLED;
			}
		}
		else
		{
			mii.fState = MFS_GRAYED;
		}
		SetMenuItemInfo((HMENU)wParam, ID_FILE_CLOSE, false, &mii);
		if (hTempWnd && (hTempWnd == hTileMap || hTempWnd == hTiles))
		{
			mii.fState = MFS_ENABLED;
		}
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 10, true, &mii);
		if (mii.fState == MFS_ENABLED)
		{
			if (hTempWnd == hTiles)
			{
				dw = Tiles.Zoom;
			}
			else
			{
				if (hTempWnd == hTileMap)
				{
					dw = TileMap.Zoom;
				}
				else
				{
					GetClientRect(hTempWnd, &rct);
					if (rct.right / 160 == rct.bottom / 144 && rct.right % 160 == 0 && rct.bottom % 144 == 0)
					{
						dw = rct.right / 160;
					}
					else
					{
						dw = 0;
					}
				}
			}
			mii.fMask = MIIM_TYPE | MIIM_STATE;
			mii.dwTypeData = szBuffer;
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_100, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 1 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_100, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_200, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 2 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_200, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_300, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 3 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_300, false, &mii);
			mii.cch = sizeof(szBuffer);
			GetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_400, false, &mii);
			mii.fType = MFT_RADIOCHECK;
			mii.fState = dw == 4 ? MFS_CHECKED : MFS_UNCHECKED;
			SetMenuItemInfo((HMENU)wParam, ID_VIEW_ZOOM_400, false, &mii);
			mii.fMask = MIIM_STATE;
		}
		if (hTempWnd && (hTempWnd == hMemory || hTempWnd == hDisAsm) && pGameBoy)
		{
			mii.fState = MFS_ENABLED;
			CreateBankMenu(pGameBoy, GetSubMenu(GetSubMenu((HMENU)wParam, 2), 12), 0);
		}
		else
		{
			mii.fState = MFS_GRAYED;
		}
		SetMenuItemInfo(GetSubMenu((HMENU)wParam, 2), 12, true, &mii);
		SetMenuItemInfo((HMENU)wParam, ID_EDIT_GOTO, false, &mii);
		mii.fState = Settings.SoundEnabled ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo((HMENU)wParam, ID_TOOLS_SOUNDENABLED, false, &mii);
		mii.fState = GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo((HMENU)wParam, ID_VIEW_STATUSBAR, false, &mii);

		mii.fState = MFS_ENABLED | (Settings.LinkCable ? MFS_CHECKED : MFS_UNCHECKED);
		SetMenuItemInfo((HMENU)wParam, ID_TOOLS_LINKCABLE, false, &mii);

		pGameBoy->Release();
		return 0;

	/*case WM_NCCALCSIZE:
		if (wParam)
		{
			DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
			((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].left += Registers.Width;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].top += ;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].right -= ;
			//((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].bottom -= ;
			return WVR_HREDRAW | WVR_VREDRAW;
		}
		else
		{
			DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
			((RECT *)lParam)->left += Registers.Width;
			return 0;
		}

	case WM_NCPAINT:
		DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
		HDC		hdc;
		SetLastError(NOERROR);
		if (!(hdc = GetWindowDC(hWin)))
		{
			DisplayErrorMessage(hWin);
			return 0;
		}
		//rct.right = rct.left + Registers.Width;
		//rct.bottom = rct.top + Registers.Height;
		RECT	Rect2, Rect3;
		POINT	pt;
		GetWindowRect(hWin, &rct);
		GetClientRect(hWin, &Rect2);
		pt.x = 0;
		pt.y = 0;
		ClientToScreen(hWin, &pt);
		Rect2.left = pt.x - rct.left;
		Rect2.top = pt.y - rct.top;
		rct.right -= rct.left;
		rct.bottom -= rct.top;
		rct.left = 0;
		rct.top = 0;

		//rct
		//	right	= width of window
		Rect3.left = GetSystemMetrics(SM_CXSIZEFRAME);
		Rect3.top = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);
		Rect3.right = Rect2.left;
		Rect3.bottom = rct.bottom - GetSystemMetrics(SM_CYSIZEFRAME);
		FillRect(hdc, &Rect3, (HBRUSH)(COLOR_BTNFACE + 1));
		rct.left = GetSystemMetrics(SM_CXSIZEFRAME);
		rct.top = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);
		rct.right = rct.left + Registers.Width - 1;
		rct.bottom = rct.top + Registers.Height - 1;
		FillRect(hdc, &rct, (HBRUSH)(COLOR_WINDOW + 1));
		PaintRegisters(hdc, &rct);

		//Draw Border around client area
		Rect2.bottom += Rect2.top + GetSystemMetrics(SM_CYEDGE);
		Rect2.right += Rect2.left + GetSystemMetrics(SM_CXEDGE);
		Rect2.left -= GetSystemMetrics(SM_CXEDGE);
		Rect2.top -= GetSystemMetrics(SM_CYEDGE);
		DrawEdge(hdc, &Rect2, EDGE_SUNKEN, BF_ADJUST | BF_RECT);
		ReleaseDC(hWin, hdc);
		return 0;*/

	case WM_SIZE:
		if (!(GetWindowLong(hStatusWnd, GWL_STYLE) & WS_VISIBLE))
		{
			break;
		}
		GetWindowRect(hStatusWnd, &rct);
		MoveWindow(hClientWnd, 0, 0, LOWORD(lParam), HIWORD(lParam) - (rct.bottom - rct.top), true);
		MoveWindow(hStatusWnd, 0, HIWORD(lParam) - (rct.bottom - rct.top), LOWORD(lParam), HIWORD(lParam), true);
		return 0;

	case WM_CREATE:
		if (!(hStatusWnd = CreateWindow(STATUSCLASSNAME, String(IDS_STATUS_READY), (StatusBarAppearance ? WS_VISIBLE : 0) | WS_CHILD | SBARS_SIZEGRIP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL)))
		{
			DisplayErrorMessage();
			return -1;
		}
		ccs.hWindowMenu = GetSubMenu(GetMenu(hWin), MENU_WINDOW);
		ccs.idFirstChild = 0;
		if (!(hClientWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "MDICLIENT", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, 0, 0, 0, 0, hWin, NULL, hInstance, &ccs)))
		{
			DisplayErrorMessage();
			return -1;
		}
		return 0;

	case WM_CLOSE:
		if (GameBoys.DeleteAll())
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		Main.Appearance = GetWindowLong(hWin, GWL_STYLE) & WS_MAXIMIZE ? 2 : 0;
		if (!(Main.Appearance & 2) && !(GetWindowLong(hWin, GWL_STYLE) & WS_MINIMIZE))
		{
			GetWindowRect(hWin, &rct);
			Main.x = rct.left;
			Main.y = rct.top;
			Main.Width = rct.right - rct.left;
			Main.Height = rct.bottom - rct.top;
		}
		Registers.Appearance = hRegisters ? 1 : 0;
		DisAsm.Appearance = hDisAsm ? 1 : 0;
		Memory.Appearance = hMemory ? 1 : 0;
		Tiles.Appearance = hTiles ? 1 : 0;
		TileMap.Appearance = hTileMap ? 1 : 0;
		Palettes.Appearance = hPalettes ? 1 : 0;
		Hardware.Appearance = hHardware ? 1 : 0;
		PostQuitMessage(0);
	}

	return DefFrameProc(hWin, hClientWnd, uMsg, wParam, lParam);
}



void UpdateRegistry()
{
	HKEY		hKey;
	DWORD		Value, ReleaseNo, ValueSize, ValueType, dw, dwErrCode;
	char		szPath[MAX_PATH];


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_EXECUTE | KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}

	GetModuleFileName(NULL, szPath, sizeof(szPath));
	if (dwErrCode = RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE *)&szPath, strlen(szPath) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	Value = VERSION_MAJOR;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMajor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	Value = VERSION_MINOR;
	if (dwErrCode = RegSetValueEx(hKey, "VersionMinor", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}

	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, "Release", NULL, &ValueType, (BYTE *)&ReleaseNo, &ValueSize))
	{
		if (ValueType != REG_DWORD)
		{
			ReleaseNo = 0;
		}
	}
	else
	{
		ReleaseNo = 0;
	}

	if (ReleaseNo < GAME_LAD_RELEASENO)
	{
		ReleaseNo = GAME_LAD_RELEASENO;
		if (dwErrCode = RegSetValueEx(hKey, "Release", 0, REG_DWORD, (BYTE *)&ReleaseNo, sizeof(DWORD)))
		{
			RegCloseKey(hKey);
			DisplayErrorMessage(dwErrCode);
			return;
		}
	}
	RegCloseKey(hKey);

	if (strchr(szPath, '.'))
	{
		*strrchr(szPath, '.') = 0;
	}
	strcat(szPath, ".dll");
	if (dwErrCode = RegCreateKeyEx(HKEY_CLASSES_ROOT, "CLSID\\{acdece20-a9d8-11d4-ace1-e0ae57c10001}", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "", 0, REG_SZ, (BYTE *)"Game Lad", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);
	if (dwErrCode = RegCreateKeyEx(HKEY_CLASSES_ROOT, "CLSID\\{acdece20-a9d8-11d4-ace1-e0ae57c10001}\\InProcServer32", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "", 0, REG_SZ, (BYTE *)&szPath, strlen(szPath) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "ThreadingModel", 0, REG_SZ, (BYTE *)"Apartment", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);

	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, "REG_SZ", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, "{acdece20-a9d8-11d4-ace1-e0ae57c10001}", 0, REG_SZ, (BYTE *)"Game Lad", 10))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);


	if (Settings.GbFile)
	{
		RegisterGbFileType();
	}
	if (Settings.GlsFile)
	{
		RegisterGlsFileType();
	}
}



void LoadWindowSettings(HKEY hKey, char *pszWindowName, WINDOWSETTINGS *pWindowSettings)
{
	char		szKey[50];
	DWORD		dwLength;
	DWORD		Value, ValueSize, ValueType;


	strcpy(szKey, pszWindowName);
	dwLength = strlen(szKey);

	strcat(szKey, "Appearance");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Appearance = Value & 3;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "X");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->x = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Y");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->y = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Width");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Width = Value;
		}
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Height");
	ValueSize = sizeof(Value);
	if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
	{
		if (ValueType == REG_DWORD)
		{
			pWindowSettings->Height = Value;
		}
	}
	if (pWindowSettings->Zoom != 0)
	{
		szKey[dwLength] = '\0';
		strcat(szKey, "Zoom");
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, szKey, NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType == REG_DWORD)
			{
				if (Value > 4)
				{
					Value = 4;
				}
				if (Value < 1)
				{
					Value = 1;
				}
				pWindowSettings->Zoom = Value;
			}
		}
	}
}



BOOL SaveWindowSettings(HKEY hKey, char *pszWindowName, WINDOWSETTINGS *pWindowSettings)
{
	char		szKey[50];
	DWORD		dwLength;
	DWORD		Value, dwErrCode;


	strcpy(szKey, pszWindowName);
	dwLength = strlen(szKey);

	strcat(szKey, "Appearance");
	Value = pWindowSettings->Appearance;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "X");
	Value = pWindowSettings->x;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Y");
	Value = pWindowSettings->y;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Width");
	Value = pWindowSettings->Width;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return true;
	}
	szKey[dwLength] = '\0';
	strcat(szKey, "Height");
	Value = pWindowSettings->Height;
	if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return true;
	}
	if (pWindowSettings->Zoom != 0)
	{
		szKey[dwLength] = '\0';
		strcat(szKey, "Zoom");
		Value = pWindowSettings->Zoom;
		if (dwErrCode = RegSetValueEx(hKey, szKey, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))
		{
			RegCloseKey(hKey);
			DisplayErrorMessage(dwErrCode);
			return true;
		}
	}

	return false;
}



#define		LoadSetting(szSetting, cmd)													\
	ValueSize = sizeof(Value);															\
	if (!RegQueryValueEx(hKey, szSetting, NULL, &ValueType, (BYTE *)&Value, &ValueSize))\
	{																					\
		if (ValueType == REG_DWORD)														\
		{																				\
			cmd;																		\
		}																				\
	}

BOOL LoadCustomSettings()
{
	HKEY		hKey, hKey2;
	DWORD		Value, ValueSize, ValueType, dwPos;
	char		szFilename[MAX_PATH], szBuffer[MAX_PATH];
	KEYSCHEME	*pKeyScheme;


	Settings.GbFile = true;
	Settings.GlsFile = true;
	Settings.AutoLoadBattery = true;
	Settings.AutoLoadState = false;
	Settings.SaveBattery = SAVEBATTERY_PROMPT;
	Settings.SaveState = SAVESTATE_NO;
	Settings.FrameSkip = 0;
	Settings.GBType = GB_COLOR;
	Settings.AutoStart = 0;
	Settings.SoundEnabled = true;
	Settings.LinkCable = false;

	JoyIsAnalog[0] = false;
	JoyIsAnalog[1] = false;

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings", 0, KEY_EXECUTE, &hKey))
	{
		LoadSetting("GbFile", {Settings.GbFile = Value ? true : false;});
		LoadSetting("GlsFile", {Settings.GlsFile = Value ? true : false;});
		LoadSetting("AutoLoadBattery", {Settings.AutoLoadBattery = Value ? true : false;});
		LoadSetting("AutoLoadState", {Settings.AutoLoadState = Value ? true : false;});
		LoadSetting("SaveBattery", {Settings.SaveBattery = (BYTE)Value & (SAVEBATTERY_NO | SAVEBATTERY_ALWAYS | SAVEBATTERY_PROMPT);});
		LoadSetting("SaveState", {Settings.SaveState = (BYTE)Value & (SAVESTATE_NO | SAVESTATE_ALWAYS | SAVESTATE_PROMPT);});
		LoadSetting("FrameSkip", {Settings.FrameSkip = Value >= 9 ? 9 : (BYTE)Value;});
		LoadSetting("GameBoyType", {Settings.GBType = (BYTE)Value & GB_COLOR;});
		LoadSetting("AutoStart", {Settings.AutoStart = (BYTE)Value & (AUTOSTART_DEBUG | AUTOSTART_EXECUTE);});
		LoadSetting("SoundEnabled", {Settings.SoundEnabled = Value ? true : false;});
		LoadSetting("LinkCable", {Settings.LinkCable = Value ? true : false;});
		ValueSize = sizeof(szStringFilename);
		if (!RegQueryValueEx(hKey, "Language", NULL, &ValueType, (BYTE *)&szStringFilename, &ValueSize))
		{
			if (ValueType == REG_SZ)
			{
				if (szStringFilename[0])
				{
					if (!(hStringInstance = LoadLibrary(szStringFilename)))
					{
						szStringFilename[0] = '\0';
					}
				}
			}
			else
			{
				szStringFilename[0] = '\0';
			}
		}
		RegCloseKey(hKey);
	}

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(szKeyScheme[0]);
		if (!RegQueryValueEx(hKey, NULL, NULL, &ValueType, (BYTE *)&szKeyScheme[0], &ValueSize))
		{
			if (ValueType != REG_SZ)
			{
				szKeyScheme[0][0] = '\0';
			}
		}
		else
		{
			szKeyScheme[0][0] = '\0';
		}
		LoadSetting("Up", {Keys[0].Up = Value;});
		LoadSetting("Down", {Keys[0].Down = Value;});
		LoadSetting("Left", {Keys[0].Left = Value;});
		LoadSetting("Right", {Keys[0].Right = Value;});
		LoadSetting("A", {Keys[0].A = Value;});
		LoadSetting("B", {Keys[0].B = Value;});
		LoadSetting("Start", {Keys[0].Start = Value;});
		LoadSetting("Select", {Keys[0].Select = Value;});
		LoadSetting("FastForward", {Keys[0].FastForward = Value;});
		LoadSetting("AutoUp", {AutoFireKeys[0].Up = Value;});
		LoadSetting("AutoDown", {AutoFireKeys[0].Down = Value;});
		LoadSetting("AutoLeft", {AutoFireKeys[0].Left = Value;});
		LoadSetting("AutoRight", {AutoFireKeys[0].Right = Value;});
		LoadSetting("AutoA", {AutoFireKeys[0].A = Value;});
		LoadSetting("AutoB", {AutoFireKeys[0].B = Value;});
		LoadSetting("AutoStart", {AutoFireKeys[0].Start = Value;});
		LoadSetting("AutoSelect", {AutoFireKeys[0].Select = Value;});
		LoadSetting("JoyUp", {JoyButtons[0].Up = Value;});
		LoadSetting("JoyDown", {JoyButtons[0].Down = Value;});
		LoadSetting("JoyLeft", {JoyButtons[0].Left = Value;});
		LoadSetting("JoyRight", {JoyButtons[0].Right = Value;});
		LoadSetting("JoyA", {JoyButtons[0].A = Value;});
		LoadSetting("JoyB", {JoyButtons[0].B = Value;});
		LoadSetting("JoyStart", {JoyButtons[0].Start = Value;});
		LoadSetting("JoySelect", {JoyButtons[0].Select = Value;});
		LoadSetting("JoyFastForward", {JoyButtons[0].FastForward = Value;});
		LoadSetting("JoyAutoUp", {AutoFireJoyButtons[0].Up = Value;});
		LoadSetting("JoyAutoDown", {AutoFireJoyButtons[0].Down = Value;});
		LoadSetting("JoyAutoLeft", {AutoFireJoyButtons[0].Left = Value;});
		LoadSetting("JoyAutoRight", {AutoFireJoyButtons[0].Right = Value;});
		LoadSetting("JoyAutoA", {AutoFireJoyButtons[0].A = Value;});
		LoadSetting("JoyAutoB", {AutoFireJoyButtons[0].B = Value;});
		LoadSetting("JoyAutoStart", {AutoFireJoyButtons[0].Start = Value;});
		LoadSetting("JoyAutoSelect", {AutoFireJoyButtons[0].Select = Value;});
		LoadSetting("JoyIsAnalog", {JoyIsAnalog[0] = Value;});
		ValueSize = sizeof(JoyGuid[0]);
		if (!RegQueryValueEx(hKey, "JoyGuid", NULL, &ValueType, (BYTE *)&JoyGuid[0], &ValueSize))
		{
			if (ValueType != REG_BINARY || ValueSize != sizeof(JoyGuid[0]))
			{
				ZeroMemory(&JoyGuid[0], sizeof(JoyGuid[0]));
			}
		}
		else
		{
			ZeroMemory(&JoyGuid[0], sizeof(JoyGuid[0]));
		}
		RegCloseKey(hKey);
	}
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player2", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(szKeyScheme[1]);
		if (!RegQueryValueEx(hKey, NULL, NULL, &ValueType, (BYTE *)&szKeyScheme[1], &ValueSize))
		{
			if (ValueType != REG_SZ)
			{
				szKeyScheme[1][0] = '\0';
			}
		}
		else
		{
			szKeyScheme[1][0] = '\0';
		}
		LoadSetting("Up", {Keys[1].Up = Value;});
		LoadSetting("Down", {Keys[1].Down = Value;});
		LoadSetting("Left", {Keys[1].Left = Value;});
		LoadSetting("Right", {Keys[1].Right = Value;});
		LoadSetting("A", {Keys[1].A = Value;});
		LoadSetting("B", {Keys[1].B = Value;});
		LoadSetting("Start", {Keys[1].Start = Value;});
		LoadSetting("Select", {Keys[1].Select = Value;});
		LoadSetting("FastForward", {Keys[1].FastForward = Value;});
		LoadSetting("AutoUp", {AutoFireKeys[1].Up = Value;});
		LoadSetting("AutoDown", {AutoFireKeys[1].Down = Value;});
		LoadSetting("AutoLeft", {AutoFireKeys[1].Left = Value;});
		LoadSetting("AutoRight", {AutoFireKeys[1].Right = Value;});
		LoadSetting("AutoA", {AutoFireKeys[1].A = Value;});
		LoadSetting("AutoB", {AutoFireKeys[1].B = Value;});
		LoadSetting("AutoStart", {AutoFireKeys[1].Start = Value;});
		LoadSetting("AutoSelect", {AutoFireKeys[1].Select = Value;});
		LoadSetting("JoyUp", {JoyButtons[1].Up = Value;});
		LoadSetting("JoyDown", {JoyButtons[1].Down = Value;});
		LoadSetting("JoyLeft", {JoyButtons[1].Left = Value;});
		LoadSetting("JoyRight", {JoyButtons[1].Right = Value;});
		LoadSetting("JoyA", {JoyButtons[1].A = Value;});
		LoadSetting("JoyB", {JoyButtons[1].B = Value;});
		LoadSetting("JoyStart", {JoyButtons[1].Start = Value;});
		LoadSetting("JoySelect", {JoyButtons[1].Select = Value;});
		LoadSetting("JoyFastForward", {JoyButtons[1].FastForward = Value;});
		LoadSetting("JoyAutoUp", {AutoFireJoyButtons[1].Up = Value;});
		LoadSetting("JoyAutoDown", {AutoFireJoyButtons[1].Down = Value;});
		LoadSetting("JoyAutoLeft", {AutoFireJoyButtons[1].Left = Value;});
		LoadSetting("JoyAutoRight", {AutoFireJoyButtons[1].Right = Value;});
		LoadSetting("JoyAutoA", {AutoFireJoyButtons[1].A = Value;});
		LoadSetting("JoyAutoB", {AutoFireJoyButtons[1].B = Value;});
		LoadSetting("JoyAutoStart", {AutoFireJoyButtons[1].Start = Value;});
		LoadSetting("JoyAutoSelect", {AutoFireJoyButtons[1].Select = Value;});
		LoadSetting("JoyIsAnalog", {JoyIsAnalog[1] = Value;});
		ValueSize = sizeof(JoyGuid[1]);
		if (!RegQueryValueEx(hKey, "JoyGuid", NULL, &ValueType, (BYTE *)&JoyGuid[1], &ValueSize))
		{
			if (ValueType != REG_BINARY || ValueSize != sizeof(JoyGuid[1]))
			{
				ZeroMemory(&JoyGuid[1], sizeof(JoyGuid[1]));
			}
		}
		else
		{
			ZeroMemory(&JoyGuid[1], sizeof(JoyGuid[1]));
		}
		RegCloseKey(hKey);
	}

	//In Game Lad 1.1 (release 2) and below, the A and B buttons were swapped.
	//This will swap the buttons if Game Lad 1.2 (release 3) or higher has never
	//been run on this computer.
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad", 0, KEY_EXECUTE, &hKey))
	{
		ValueSize = sizeof(Value);
		if (!RegQueryValueEx(hKey, "Release", NULL, &ValueType, (BYTE *)&Value, &ValueSize))
		{
			if (ValueType != REG_DWORD)
			{
				Value = 0;
			}
		}
		else
		{
			Value = 0;
		}
		RegCloseKey(hKey);
		if (Value < 3)
		{
			Value = Keys[0].A;
			Keys[0].A = Keys[0].B;
			Keys[0].B = Value;
		}
		if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, KEY_SET_VALUE, &hKey))
		{
			Value = Keys[0].A;
			RegSetValueEx(hKey, "A", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD));
			Value = Keys[0].B;
			RegSetValueEx(hKey, "B", 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD));
			RegCloseKey(hKey);
		}
	}


	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\KeySchemes", 0, KEY_EXECUTE | KEY_ENUMERATE_SUB_KEYS, &hKey2))
	{
		dwPos = 0;
		while (true)
		{
			ValueSize = sizeof(szBuffer);
			if (RegEnumKeyEx(hKey2, dwPos++, szBuffer, &ValueSize, NULL, NULL, NULL, NULL))
			{
				break;
			}
			if (!(RegOpenKeyEx(hKey2, szBuffer, 0, KEY_EXECUTE, &hKey)))
			{
				if (!(pKeyScheme = (KEYSCHEME *)pKeySchemes->NewItem(sizeof(*pKeyScheme))))
				{
					RegCloseKey(hKey);
					RegCloseKey(hKey2);
					DisplayErrorMessage(ERROR_OUTOFMEMORY);
					return true;
				}
				strcpy(pKeyScheme->szName, szBuffer);
				LoadSetting("Up", {pKeyScheme->Keyboard[0].Up = Value;});
				LoadSetting("Down", {pKeyScheme->Keyboard[0].Down = Value;});
				LoadSetting("Left", {pKeyScheme->Keyboard[0].Left = Value;});
				LoadSetting("Right", {pKeyScheme->Keyboard[0].Right = Value;});
				LoadSetting("A", {pKeyScheme->Keyboard[0].A = Value;});
				LoadSetting("B", {pKeyScheme->Keyboard[0].B = Value;});
				LoadSetting("Start", {pKeyScheme->Keyboard[0].Start = Value;});
				LoadSetting("Select", {pKeyScheme->Keyboard[0].Select = Value;});
				LoadSetting("FastForward", {pKeyScheme->Keyboard[0].FastForward = Value;});
				LoadSetting("AutoUp", {pKeyScheme->Keyboard[1].Up = Value;});
				LoadSetting("AutoDown", {pKeyScheme->Keyboard[1].Down = Value;});
				LoadSetting("AutoLeft", {pKeyScheme->Keyboard[1].Left = Value;});
				LoadSetting("AutoRight", {pKeyScheme->Keyboard[1].Right = Value;});
				LoadSetting("AutoA", {pKeyScheme->Keyboard[1].A = Value;});
				LoadSetting("AutoB", {pKeyScheme->Keyboard[1].B = Value;});
				LoadSetting("AutoStart", {pKeyScheme->Keyboard[1].Start = Value;});
				LoadSetting("AutoSelect", {pKeyScheme->Keyboard[1].Select = Value;});
				LoadSetting("JoyUp", {pKeyScheme->Joystick[0].Up = Value;});
				LoadSetting("JoyDown", {pKeyScheme->Joystick[0].Down = Value;});
				LoadSetting("JoyLeft", {pKeyScheme->Joystick[0].Left = Value;});
				LoadSetting("JoyRight", {pKeyScheme->Joystick[0].Right = Value;});
				LoadSetting("JoyA", {pKeyScheme->Joystick[0].A = Value;});
				LoadSetting("JoyB", {pKeyScheme->Joystick[0].B = Value;});
				LoadSetting("JoyStart", {pKeyScheme->Joystick[0].Start = Value;});
				LoadSetting("JoySelect", {pKeyScheme->Joystick[0].Select = Value;});
				LoadSetting("JoyFastForward", {pKeyScheme->Joystick[0].FastForward = Value;});
				LoadSetting("JoyAutoUp", {pKeyScheme->Joystick[1].Up = Value;});
				LoadSetting("JoyAutoDown", {pKeyScheme->Joystick[1].Down = Value;});
				LoadSetting("JoyAutoLeft", {pKeyScheme->Joystick[1].Left = Value;});
				LoadSetting("JoyAutoRight", {pKeyScheme->Joystick[1].Right = Value;});
				LoadSetting("JoyAutoA", {pKeyScheme->Joystick[1].A = Value;});
				LoadSetting("JoyAutoB", {pKeyScheme->Joystick[1].B = Value;});
				LoadSetting("JoyAutoStart", {pKeyScheme->Joystick[1].Start = Value;});
				LoadSetting("JoyAutoSelect", {pKeyScheme->Joystick[1].Select = Value;});
				LoadSetting("JoyIsAnalog", {pKeyScheme->JoyIsAnalog = Value;});
				ValueSize = sizeof(pKeyScheme->JoyGuid);
				if (!RegQueryValueEx(hKey, "JoyGuid", NULL, &ValueType, (BYTE *)&pKeyScheme->JoyGuid, &ValueSize))
				{
					if (ValueType != REG_BINARY || ValueSize != sizeof(pKeyScheme->JoyGuid))
					{
						ZeroMemory(&pKeyScheme->JoyGuid, sizeof(pKeyScheme->JoyGuid));
					}
				}
				else
				{
					ZeroMemory(&pKeyScheme->JoyGuid, sizeof(pKeyScheme->JoyGuid));
				}
				RegCloseKey(hKey);
			}
		}
		RegCloseKey(hKey2);
	}


	Main.Appearance = 0;
	Main.x = CW_USEDEFAULT;
	Main.y = CW_USEDEFAULT;
	Main.Width = CW_USEDEFAULT;
	Main.Height = CW_USEDEFAULT;
	Main.Zoom = 0;
	Registers.Appearance = 0;
	Registers.x = CW_USEDEFAULT;
	Registers.y = CW_USEDEFAULT;
	Registers.Width = 12 * FixedFontWidth + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	Registers.Height = 14 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Registers.Zoom = 0;
	DisAsm.Appearance = 0;
	DisAsm.x = CW_USEDEFAULT;
	DisAsm.y = CW_USEDEFAULT;
	DisAsm.Width = 32 * FixedFontWidth + 15 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL);
	DisAsm.Height = 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	DisAsm.Zoom = 0;
	Memory.Appearance = 0;
	Memory.x = CW_USEDEFAULT;
	Memory.y = CW_USEDEFAULT;
	Memory.Width = 42 * FixedFontWidth + 1 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXVSCROLL);
	Memory.Height = 32 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Memory.Zoom = 0;
	Tiles.Appearance = 0;
	Tiles.x = CW_USEDEFAULT;
	Tiles.y = CW_USEDEFAULT;
	Tiles.Width = 134 + 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE);
	Tiles.Height = 45 + 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	Tiles.Zoom = 1;
	TileMap.Appearance = 0;
	TileMap.x = CW_USEDEFAULT;
	TileMap.y = CW_USEDEFAULT;
	TileMap.Width = 171 + 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE);
	TileMap.Height = 55 + 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	TileMap.Zoom = 1;
	Palettes.Appearance = 0;
	Palettes.x = CW_USEDEFAULT;
	Palettes.y = CW_USEDEFAULT;
	Palettes.Width = 362 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	Palettes.Height = 345 + FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE);
	Palettes.Zoom = 0;
	Hardware.Appearance = 0;
	Hardware.x = CW_USEDEFAULT;
	Hardware.y = CW_USEDEFAULT;
	Hardware.Width = 19 * FixedFontWidth + 4 * GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXVSCROLL);
	Hardware.Height = 3 * FixedFontHeight + GetSystemMetrics(SM_CYSMCAPTION) + 4 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYHSCROLL);
	Hardware.Zoom = 0;
	StatusBarAppearance = true;

	if (!RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Windows", 0, KEY_EXECUTE, &hKey))
	{
		LoadWindowSettings(hKey, "", &Main);
		LoadWindowSettings(hKey, "Registers", &Registers);
		LoadWindowSettings(hKey, "Disassembly", &DisAsm);
		LoadWindowSettings(hKey, "Memory", &Memory);
		LoadWindowSettings(hKey, "Tiles", &Tiles);
		LoadWindowSettings(hKey, "TileMap", &TileMap);
		LoadWindowSettings(hKey, "Palettes", &Palettes);
		LoadWindowSettings(hKey, "Hardware", &Hardware);
		LoadSetting("StatusBarAppearance", {StatusBarAppearance = Value ? true : false;});
		RegCloseKey(hKey);
	}

	if (Main.x >= (unsigned)GetSystemMetrics(SM_CXFULLSCREEN) - 30)
	{
		Main.x = CW_USEDEFAULT;
	}
	if (Main.y >= (unsigned)GetSystemMetrics(SM_CYFULLSCREEN) - 20)
	{
		Main.y = CW_USEDEFAULT;
	}
	if (Tiles.Width == 0)
	{
		Tiles.Width = 46 + Tiles.Zoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	}
	if (Tiles.Height == 0)
	{
		Tiles.Height = 34 + Tiles.Zoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	}
	if (TileMap.Width == 0)
	{
		TileMap.Width = 83 + TileMap.Zoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE);
	}
	if (TileMap.Height == 0)
	{
		TileMap.Height = 40 + TileMap.Zoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION);
	}


	GetModuleFileName(hInstance, szFilename, sizeof(szFilename));
	if (strchr(szFilename, '\\'))
	{
		*strrchr(szFilename, '\\') = '\0';
		strcat(szFilename, "\\Cheats.xml");
	}
	Cheats.MergeFile(szFilename);

	return false;
}



void DeleteAllSubKeys(HKEY hKey, char *pszKey)
{
	char		szBuffer[MAX_PATH];
	DWORD		sizeof_szBuffer;
	HKEY		hKey2;


	if (RegOpenKeyEx(hKey, pszKey, 0, KEY_ENUMERATE_SUB_KEYS, &hKey2))
	{
		return;
	}

	while (true)
	{
		sizeof_szBuffer = sizeof(szBuffer);
		if (RegEnumKeyEx(hKey2, 0, szBuffer, &sizeof_szBuffer, NULL, NULL, NULL, NULL))
		{
			break;
		}
		DeleteAllSubKeys(hKey2, szBuffer);
	}

	RegCloseKey(hKey2);

	RegDeleteKey(hKey, pszKey);
}



#define		SaveSetting(dwSetting, szSetting)													\
	Value = dwSetting;																			\
	if (dwErrCode = RegSetValueEx(hKey, szSetting, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))\
	{																							\
		RegCloseKey(hKey);																		\
		DisplayErrorMessage(dwErrCode);															\
		return;																					\
	}

#define		SaveSetting2(dwSetting, szSetting)													\
	Value = dwSetting;																			\
	if (dwErrCode = RegSetValueEx(hKey, szSetting, 0, REG_DWORD, (BYTE *)&Value, sizeof(DWORD)))\
	{																							\
		RegCloseKey(hKey);																		\
		RegCloseKey(hKey2);																		\
		DisplayErrorMessage(dwErrCode);															\
		return;																					\
	}

void SaveCustomSettings()
{
	HKEY		hKey, hKey2;
	DWORD		Value, dw, dwErrCode;
	KEYSCHEME	*pKeyScheme;


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	SaveSetting(Settings.GbFile, "GbFile");
	SaveSetting(Settings.GlsFile, "GlsFile");
	SaveSetting(Settings.AutoLoadBattery, "AutoLoadBattery");
	SaveSetting(Settings.AutoLoadState, "AutoLoadState");
	SaveSetting(Settings.SaveBattery, "SaveBattery");
	SaveSetting(Settings.SaveState, "SaveState");
	SaveSetting(Settings.FrameSkip, "FrameSkip");
	SaveSetting(Settings.GBType, "GameBoyType");
	SaveSetting(Settings.AutoStart, "AutoStart");
	SaveSetting(Settings.SoundEnabled, "SoundEnabled");
	SaveSetting(Settings.LinkCable, "LinkCable");
	if (dwErrCode = RegSetValueEx(hKey, "Language", 0, REG_SZ, (BYTE *)&szStringFilename, strlen(szStringFilename) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);

	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player1", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)&szKeyScheme[0], strlen(szKeyScheme[0]) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	SaveSetting(Keys[0].Up, "Up");
	SaveSetting(Keys[0].Down, "Down");
	SaveSetting(Keys[0].Left, "Left");
	SaveSetting(Keys[0].Right, "Right");
	SaveSetting(Keys[0].A, "A");
	SaveSetting(Keys[0].B, "B");
	SaveSetting(Keys[0].Start, "Start");
	SaveSetting(Keys[0].Select, "Select");
	SaveSetting(Keys[0].FastForward, "FastForward");
	SaveSetting(AutoFireKeys[0].Up, "AutoUp");
	SaveSetting(AutoFireKeys[0].Down, "AutoDown");
	SaveSetting(AutoFireKeys[0].Left, "AutoLeft");
	SaveSetting(AutoFireKeys[0].Right, "AutoRight");
	SaveSetting(AutoFireKeys[0].A, "AutoA");
	SaveSetting(AutoFireKeys[0].B, "AutoB");
	SaveSetting(AutoFireKeys[0].Start, "AutoStart");
	SaveSetting(AutoFireKeys[0].Select, "AutoSelect");
	SaveSetting(JoyButtons[0].Up, "JoyUp");
	SaveSetting(JoyButtons[0].Down, "JoyDown");
	SaveSetting(JoyButtons[0].Left, "JoyLeft");
	SaveSetting(JoyButtons[0].Right, "JoyRight");
	SaveSetting(JoyButtons[0].A, "JoyA");
	SaveSetting(JoyButtons[0].B, "JoyB");
	SaveSetting(JoyButtons[0].Start, "JoyStart");
	SaveSetting(JoyButtons[0].Select, "JoySelect");
	SaveSetting(JoyButtons[0].FastForward, "JoyFastForward");
	SaveSetting(AutoFireJoyButtons[0].Up, "JoyAutoUp");
	SaveSetting(AutoFireJoyButtons[0].Down, "JoyAutoDown");
	SaveSetting(AutoFireJoyButtons[0].Left, "JoyAutoLeft");
	SaveSetting(AutoFireJoyButtons[0].Right, "JoyAutoRight");
	SaveSetting(AutoFireJoyButtons[0].A, "JoyAutoA");
	SaveSetting(AutoFireJoyButtons[0].B, "JoyAutoB");
	SaveSetting(AutoFireJoyButtons[0].Start, "JoyAutoStart");
	SaveSetting(AutoFireJoyButtons[0].Select, "JoyAutoSelect");
	SaveSetting(JoyIsAnalog[0], "JoyIsAnalog");
	if (dwErrCode = RegSetValueEx(hKey, "JoyGuid", 0, REG_BINARY, (BYTE *)&JoyGuid[0], sizeof(JoyGuid[0])))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);
	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Player2", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	if (dwErrCode = RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)&szKeyScheme[1], strlen(szKeyScheme[1]) + 1))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	SaveSetting(Keys[1].Up, "Up");
	SaveSetting(Keys[1].Down, "Down");
	SaveSetting(Keys[1].Left, "Left");
	SaveSetting(Keys[1].Right, "Right");
	SaveSetting(Keys[1].A, "A");
	SaveSetting(Keys[1].B, "B");
	SaveSetting(Keys[1].Start, "Start");
	SaveSetting(Keys[1].Select, "Select");
	SaveSetting(Keys[1].FastForward, "FastForward");
	SaveSetting(AutoFireKeys[1].Up, "AutoUp");
	SaveSetting(AutoFireKeys[1].Down, "AutoDown");
	SaveSetting(AutoFireKeys[1].Left, "AutoLeft");
	SaveSetting(AutoFireKeys[1].Right, "AutoRight");
	SaveSetting(AutoFireKeys[1].A, "AutoA");
	SaveSetting(AutoFireKeys[1].B, "AutoB");
	SaveSetting(AutoFireKeys[1].Start, "AutoStart");
	SaveSetting(AutoFireKeys[1].Select, "AutoSelect");
	SaveSetting(JoyButtons[1].Up, "JoyUp");
	SaveSetting(JoyButtons[1].Down, "JoyDown");
	SaveSetting(JoyButtons[1].Left, "JoyLeft");
	SaveSetting(JoyButtons[1].Right, "JoyRight");
	SaveSetting(JoyButtons[1].A, "JoyA");
	SaveSetting(JoyButtons[1].B, "JoyB");
	SaveSetting(JoyButtons[1].Start, "JoyStart");
	SaveSetting(JoyButtons[1].Select, "JoySelect");
	SaveSetting(JoyButtons[1].FastForward, "JoyFastForward");
	SaveSetting(AutoFireJoyButtons[1].Up, "JoyAutoUp");
	SaveSetting(AutoFireJoyButtons[1].Down, "JoyAutoDown");
	SaveSetting(AutoFireJoyButtons[1].Left, "JoyAutoLeft");
	SaveSetting(AutoFireJoyButtons[1].Right, "JoyAutoRight");
	SaveSetting(AutoFireJoyButtons[1].A, "JoyAutoA");
	SaveSetting(AutoFireJoyButtons[1].B, "JoyAutoB");
	SaveSetting(AutoFireJoyButtons[1].Start, "JoyAutoStart");
	SaveSetting(AutoFireJoyButtons[1].Select, "JoyAutoSelect");
	SaveSetting(JoyIsAnalog[1], "JoyIsAnalog");
	if (dwErrCode = RegSetValueEx(hKey, "JoyGuid", 0, REG_BINARY, (BYTE *)&JoyGuid[1], sizeof(JoyGuid[1])))
	{
		RegCloseKey(hKey);
		DisplayErrorMessage(dwErrCode);
		return;
	}
	RegCloseKey(hKey);


	DeleteAllSubKeys(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\KeySchemes");
	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\KeySchemes", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey2, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}
	pKeySchemes->ResetSearch();
	while (pKeyScheme = (KEYSCHEME *)pKeySchemes->GetNextItem())
	{
		if (!pKeyScheme->BuiltIn)
		{
			if (dwErrCode = RegCreateKeyEx(hKey2, pKeyScheme->szName, 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
			{
				RegCloseKey(hKey2);
				DisplayErrorMessage(dwErrCode);
				return;
			}
			SaveSetting2(pKeyScheme->Keyboard[0].Up, "Up");
			SaveSetting2(pKeyScheme->Keyboard[0].Down, "Down");
			SaveSetting2(pKeyScheme->Keyboard[0].Left, "Left");
			SaveSetting2(pKeyScheme->Keyboard[0].Right, "Right");
			SaveSetting2(pKeyScheme->Keyboard[0].A, "A");
			SaveSetting2(pKeyScheme->Keyboard[0].B, "B");
			SaveSetting2(pKeyScheme->Keyboard[0].Start, "Start");
			SaveSetting2(pKeyScheme->Keyboard[0].Select, "Select");
			SaveSetting2(pKeyScheme->Keyboard[0].FastForward, "FastForward");
			SaveSetting2(pKeyScheme->Keyboard[1].Up, "AutoUp");
			SaveSetting2(pKeyScheme->Keyboard[1].Down, "AutoDown");
			SaveSetting2(pKeyScheme->Keyboard[1].Left, "AutoLeft");
			SaveSetting2(pKeyScheme->Keyboard[1].Right, "AutoRight");
			SaveSetting2(pKeyScheme->Keyboard[1].A, "AutoA");
			SaveSetting2(pKeyScheme->Keyboard[1].B, "AutoB");
			SaveSetting2(pKeyScheme->Keyboard[1].Start, "AutoStart");
			SaveSetting2(pKeyScheme->Keyboard[1].Select, "AutoSelect");
			SaveSetting2(pKeyScheme->Joystick[0].Up, "JoyUp");
			SaveSetting2(pKeyScheme->Joystick[0].Down, "JoyDown");
			SaveSetting2(pKeyScheme->Joystick[0].Left, "JoyLeft");
			SaveSetting2(pKeyScheme->Joystick[0].Right, "JoyRight");
			SaveSetting2(pKeyScheme->Joystick[0].A, "JoyA");
			SaveSetting2(pKeyScheme->Joystick[0].B, "JoyB");
			SaveSetting2(pKeyScheme->Joystick[0].Start, "JoyStart");
			SaveSetting2(pKeyScheme->Joystick[0].Select, "JoySelect");
			SaveSetting2(pKeyScheme->Joystick[0].FastForward, "JoyFastForward");
			SaveSetting2(pKeyScheme->Joystick[1].Up, "JoyAutoUp");
			SaveSetting2(pKeyScheme->Joystick[1].Down, "JoyAutoDown");
			SaveSetting2(pKeyScheme->Joystick[1].Left, "JoyAutoLeft");
			SaveSetting2(pKeyScheme->Joystick[1].Right, "JoyAutoRight");
			SaveSetting2(pKeyScheme->Joystick[1].A, "JoyAutoA");
			SaveSetting2(pKeyScheme->Joystick[1].B, "JoyAutoB");
			SaveSetting2(pKeyScheme->Joystick[1].Start, "JoyAutoStart");
			SaveSetting2(pKeyScheme->Joystick[1].Select, "JoyAutoSelect");
			SaveSetting2(pKeyScheme->JoyIsAnalog, "JoyIsAnalog");
			if (dwErrCode = RegSetValueEx(hKey, "JoyGuid", 0, REG_BINARY, (BYTE *)&pKeyScheme->JoyGuid, sizeof(pKeyScheme->JoyGuid)))
			{
				RegCloseKey(hKey);
				RegCloseKey(hKey2);
				DisplayErrorMessage(dwErrCode);
				return;
			}
			RegCloseKey(hKey);
		}
	}
	RegCloseKey(hKey2);


	if (dwErrCode = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Game Lad\\Settings\\Windows", 0, "REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dw))
	{
		DisplayErrorMessage(dwErrCode);
		return;
	}

	if (SaveWindowSettings(hKey, "", &Main))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Registers", &Registers))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Disassembly", &DisAsm))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Memory", &Memory))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Tiles", &Tiles))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "TileMap", &TileMap))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Palettes", &Palettes))
	{
		return;
	}
	if (SaveWindowSettings(hKey, "Hardware", &Hardware))
	{
		return;
	}
	SaveSetting(StatusBarAppearance, "StatusBarAppearance");

	RegCloseKey(hKey);


	Cheats.Save();
}



INITCOMMONCONTROLSEX	CommonControls = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG					msg;
	WNDCLASS			wc;
	HDC					hdc;
	TEXTMETRIC			tm;
	HACCEL				hAccelerator;
	HMENU				hMenu;
	KEYSCHEME			*pKeyScheme;


	//Make hInst global
	hInstance = hInst;

	hMsgParent = NULL;


	//Check if a high resolution timer is available, and get its frequency
	if (QueryPerformanceFrequency(&TimerFrequency))
	{
		TimerFrequency.QuadPart /= 60;
	}
	else
	{
		MessageBox(hMsgParent, "No high resolution timer available.", NULL, MB_OK | MB_ICONERROR);
		return 1;
	}


	//Load fonts used
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);

	//Get size of the fonts
	hdc = GetDC(hWnd);
	SelectObject(hdc, hFixedFont);
	GetTextMetrics(hdc, &tm);
	ReleaseDC(hWnd, hdc);
	FixedFontWidth = tm.tmMaxCharWidth;
	FixedFontHeight = tm.tmHeight;


	if (!(pKeySchemes = new CList()))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return 1;
	}
	if (!(pKeyScheme = (KEYSCHEME *)pKeySchemes->NewItem(sizeof(*pKeyScheme))))
	{
		delete pKeySchemes;
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return 1;
	}
	pKeyScheme->BuiltIn = true;
	strcpy(pKeyScheme->szName, "(Default)");
	ZeroMemory(&pKeyScheme->JoyGuid, sizeof(GUID));
	CopyMemory(&pKeyScheme->Keyboard[0], &Keys[0], sizeof(KEYS));
	CopyMemory(&pKeyScheme->Keyboard[1], &AutoFireKeys[0], sizeof(KEYS));
	CopyMemory(&pKeyScheme->Joystick[0], &JoyButtons[0], sizeof(KEYS));
	CopyMemory(&pKeyScheme->Joystick[1], &AutoFireJoyButtons[0], sizeof(KEYS));

	//Loads user's settings
	if (LoadCustomSettings())
	{
		return 1;
	}


	//Set Game Lad specific keys in the system registry
	UpdateRegistry();


	if (!InitCommonControlsEx(&CommonControls))
	{
		DisplayErrorMessage();
		delete pKeySchemes;
		return 1;
	}


	//Register main window's class
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Game Lad";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		delete pKeySchemes;
		return 1;
	}
	//and Game Boy windows' class
	wc.lpfnWndProc = GameBoyWndProc;
	wc.hbrBackground = NULL;
	wc.lpszClassName = "Game Boy";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		delete pKeySchemes;
		return 1;
	}

	hMenu = CreateMenu();
	hPopupMenu = CreateMenu();
	CreateMenus(hMenu, hPopupMenu);

	//Create main window
	if (!(hWnd = hMsgParent = CreateWindowEx(WS_EX_ACCEPTFILES, "Game Lad", "Game Lad",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		Main.x, Main.y, Main.Width, Main.Height,
		NULL, hMenu, hInstance, NULL)))
	{
		DisplayErrorMessage();
		DestroyMenu(hPopupMenu);
		delete pKeySchemes;
		return 1;
	}


	if (CreateDebugWindows())
	{
		DestroyMenu(hPopupMenu);
		DestroyWindow(hWnd);
		delete pKeySchemes;
		return 1;
	}


	if (!(hStartStopEvent = CreateEvent(NULL, true, false, NULL)))
	{
		DisplayErrorMessage();
		DestroyMenu(hPopupMenu);
		DestroyWindow(hWnd);
		delete pKeySchemes;
		return 1;
	}


	hAccelerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));


	//Initialize Dynamic Data Exchange
	DdeInitialize(&DdeInst, DdeCallback, APPCLASS_STANDARD | CBF_FAIL_ADVISES | CBF_FAIL_POKES | CBF_FAIL_REQUESTS | CBF_FAIL_SELFCONNECTIONS | CBF_SKIP_ALLNOTIFICATIONS, 0);
	hDdeServiceString = DdeCreateStringHandle(DdeInst, "Game Lad", CP_WINANSI);
	hDdeTopicString = DdeCreateStringHandle(DdeInst, SZDDESYS_TOPIC, CP_WINANSI);
	DdeNameService(DdeInst, hDdeServiceString, 0, DNS_REGISTER);


	ZeroMemory(ZeroStatus, sizeof(ZeroStatus));


	InitInput();
	if (Settings.SoundEnabled)
	{
		InitSound();
	}


	//See if a path to a file has been sent on the command line
	if (lpCmdLine[0])
	{
		GameBoys.NewGameBoy(lpCmdLine, Settings.AutoLoadState ? "" : NULL, Settings.AutoLoadBattery ? "" : NULL, Settings.GBType, Settings.AutoStart);
	}


	if (Registers.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_REGISTERS, 0);
	}
	if (DisAsm.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	if (Memory.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_MEMORY, 0);
	}
	if (Tiles.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_TILES, 0);
	}
	if (TileMap.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_TILEMAP, 0);
	}
	if (Palettes.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_PALETTES, 0);
	}
	if (Hardware.Appearance & 1)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_HARDWARE, 0);
	}


	InitializeCriticalSection(&csSound);
	InitializeCriticalSection(&csGameBoy);


	//Show window
	ShowWindow(hWnd, Main.Appearance & 2 ? SW_SHOWMAXIMIZED : nCmdShow);
	UpdateWindow(hWnd);

	InitInputDevice(JoyGuid[0], NULL, 0);
	InitInputDevice(JoyGuid[1], &JoyGuid[0], 1);


	//Main message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccelerator, &msg))
		{
			if (msg.message == WM_CHAR && F4Pressed)
			{
				if (msg.wParam >= '0' && msg.wParam <= '9')
				{
					if (F4Pressed == GameBoys.GetActive(false))
					{
						F4Pressed->AddRef();
						F4Pressed->SetStateSlot(msg.wParam - '0');
						F4Pressed->Release();
					}
				}
				else
				{
					SetStatus(NULL, SF_READY);
					DispatchMessage(&msg);
				}
				F4Pressed = NULL;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}


	//Time to clean up


	CloseSound();
	CloseInput();

	if (hStringInstance)
	{
		FreeLibrary(hStringInstance);
	}


	DeleteCriticalSection(&csGameBoy);
	DeleteCriticalSection(&csSound);


	DestroyMenu(hPopupMenu);

	CloseHandle(hStartStopEvent);

	DdeNameService(DdeInst, 0, 0, DNS_UNREGISTER);
	DdeFreeStringHandle(DdeInst, hDdeServiceString);
	DdeFreeStringHandle(DdeInst, hDdeTopicString);
	DdeUninitialize(DdeInst);


	SaveCustomSettings();


	delete pKeySchemes;


	return msg.wParam;
}

