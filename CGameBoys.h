#ifndef		CGAMEBOYS_CPP
#define		CGAMEBOYS_CPP	extern
#endif		//CGAMEBOYS_CPP



struct GameBoy
{
	CGameBoy	*pGameBoy;
	GameBoy		*pNext;
};

class CGameBoys
{
private:
	GameBoy		*FirstGameBoy;
	CGameBoy	*ActiveGameBoy;

public:
	CGameBoys();

	CGameBoy	*NewGameBoy(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart);
	BOOL		DeleteGameBoy(CGameBoy *pCGameBoy);
	BOOL		DeleteAll();
	LPARAM		WndProc(CGameBoy *pGameBoy, HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CGameBoy	*GetActive();
	void		EnableSound();
	void		CloseSound();
} CGAMEBOYS_CPP GameBoys;

