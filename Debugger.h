#ifdef	DEBUGGER_CPP

#define		DEBUGGER_OBJ
#define		EQUALNULL		= NULL

#else

#define		DEBUGGER_OBJ	extern
#define		EQUALNULL

#endif



#define			MEMORY_ROM		0x00000001
#define			MEMORY_RAM		0x00000002
#define			MEMORY_SVBK		0x00000004
#define			MEMORY_VBK		0x00000008

DEBUGGER_OBJ	DWORD			MemoryFlags;

DEBUGGER_OBJ	HWND			hRegisters, hDisAsm, hMemory;
DEBUGGER_OBJ	HWND			hTiles, hPalettes, hTileMap;
DEBUGGER_OBJ	HWND			hHardware;



DEBUGGER_OBJ	BOOL	CreateDebugWindows();



#undef	DEBUGGER_OBJ
#undef	EQUALNULL

