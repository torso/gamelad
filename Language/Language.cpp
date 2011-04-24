#include	<windows.h>



char	*GameLadLanguage = 
#ifdef	CZECH_EXPORTS
	"Czech"; //change
#endif
#ifdef	FRENCH_EXPORTS
	"Français";
#endif
#ifdef	ITALIAN_EXPORTS
	"Italian"; //change
#endif
#ifdef	SPANISH_EXPORTS
	"Español";
#endif
#ifdef	PORTUGUESE_EXPORTS
	"Portuguese"; //change
#endif
#ifdef	SWEDISH_EXPORTS
	"Svenska";
#endif



BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	return true;
}

