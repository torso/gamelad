#ifndef		CGAMEBOYS_CPP
#define		CGAMEBOYS_CPP	extern
#endif		//CGAMEBOYS_CPP



struct GAMEBOYITEM
{
	CGameBoy	*pGameBoy;
	GAMEBOYITEM	*pNext;
};

class CGameBoys
{
private:
	GAMEBOYITEM	*m_pFirstGameBoy;
	CGameBoy	*m_pActiveGameBoy;


public:
	CGameBoys();
#ifdef _DEBUG
	~CGameBoys();
#endif

	BOOL		GameBoyExists(CGameBoy *pGameBoy);
	CGameBoy	*NewGameBoy(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename, BYTE Flags, BYTE AutoStart);
	BOOL		DeleteGameBoy(CGameBoy *pGameBoy);
	BOOL		DeleteAll();
	LRESULT		WndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CGameBoy	*GetActive(BOOL AddRef);
	CGameBoy	*GetPlayer2(BOOL AddRef);
	void		EnableSound();
	void		CloseSound();
	void		UpdateKeys(CGameBoy *pGameBoy);
	void		SetRumble(CGameBoy *pGameBoy);
} CGAMEBOYS_CPP GameBoys;

