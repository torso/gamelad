#ifdef	DEBUGGER_CPP

#define		DEBUGGER_OBJ
#define		EQUALNULL		= NULL

#else

#define		DEBUGGER_OBJ	extern
#define		EQUALNULL

#endif



DEBUGGER_OBJ	HWND	hRegisters, hDisAsm, hMemory;
DEBUGGER_OBJ	HWND	hTiles, hPalettes, hTileMap;
DEBUGGER_OBJ	HWND	hHardware;



DEBUGGER_OBJ	BOOL	CreateDebugWindows();



#undef	DEBUGGER_OBJ
#undef	EQUALNULL

