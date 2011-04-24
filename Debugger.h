#ifdef	DEBUGGER_CPP

#define		EQUALNULL		= NULL

#else

#define		DEBUGGER_CPP	extern
#define		EQUALNULL

#endif



#define			MEMORY_ROM		0x00000001
#define			MEMORY_RAM		0x00000002
#define			MEMORY_SVBK		0x00000004
#define			MEMORY_VBK		0x00000008

DEBUGGER_CPP	DWORD			MemoryFlags, DisAsmFlags;

DEBUGGER_CPP	HWND			hRegisters, hDisAsm, hMemory;
DEBUGGER_CPP	HWND			hTiles, hPalettes, hTileMap;
DEBUGGER_CPP	HWND			hHardware;



DEBUGGER_CPP	BOOL	CreateBankMenu(CGameBoy *pGameBoy, HMENU hMenu, DWORD dwFirstPos);
DEBUGGER_CPP	BOOL	CreateDebugWindows();

DEBUGGER_CPP	void	PaintRegisters(HDC hdc, RECT *pRect);



#undef	EQUALNULL

