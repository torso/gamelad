#include	<windows.h>



#ifdef	CZECH_EXPORTS
char	*GameLadLanguage = "Czech"; //change
#endif
#ifdef	FRENCH_EXPORTS
char	*GameLadLanguage = "Français";
#endif
#ifdef	SPANISH_EXPORTS
char	*GameLadLanguage = "Español";
#endif
#ifdef	PORTUGUESE_EXPORTS
char	*GameLadLanguage = "Portuguese"; //change
#endif
#ifdef	SWEDISH_EXPORTS
char	*GameLadLanguage = "Svenska";
#endif



BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	return true;
}

