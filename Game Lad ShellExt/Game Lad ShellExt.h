#ifdef	GAME_LAD_SHELLEXT_CPP

#define	EQUALNULL	= NULL

#else	//GAME_LAD_SHELLEXT_CPP

#define	GAME_LAD_SHELLEXT_CPP	extern
#define	EQUALNULL

#endif	//GAME_LAD_SHELLEXT_CPP



//#define	CONTEXTMENUHANDLER
//#define	ICONHANDLER
//#define	COPYHOOKHANDLER



DEFINE_GUID(CLSID_ShellExtension, 0xacdece20, 0xa9d8, 0x11d4, 0xAC, 0xE1, 0xe0, 0xae, 0x57, 0xc1, 0x00, 0x01);

GAME_LAD_SHELLEXT_CPP	HINSTANCE		hInstance;
GAME_LAD_SHELLEXT_CPP	UINT			DllRefCount EQUALNULL;



extern					void			DisplayErrorMessage(HWND hWin);



#undef	EQUALNULL

