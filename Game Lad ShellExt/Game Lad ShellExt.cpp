#include	<windows.h>
#include	<shlobj.h>
#include	<initguid.h>

#define		GAME_LAD_SHELLEXT_CPP

#include	"Game Lad ShellExt.h"
#include	"CShellExt.h"
#include	"CShellExtClassFactory.h"



void DisplayErrorMessage(HWND hWin)
{
	void	*lpMsgBuf;


	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hWin, (LPCTSTR)lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
	LocalFree(lpMsgBuf);
}



BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hInstance = hinstDLL;
	}

	return true;
}



STDAPI DllCanUnloadNow()
{
	return DllRefCount == 0 ? S_OK : S_FALSE;
}



STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
	*ppvOut = NULL;

	if (IsEqualIID(rclsid, CLSID_ShellExtension))
	{
		CShellExtClassFactory *pcf = new CShellExtClassFactory;

		return pcf->QueryInterface(riid, ppvOut);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

