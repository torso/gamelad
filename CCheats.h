#ifndef		CCHEATS_H

#ifndef		CCHEATS_CPP
#define		CCHEATS_CPP		extern
#endif		//CCHEATS_CPP



#include	"CList\CList.h"



enum VerifyCodeReturn
{
	CODE_INVALID,
	CODE_DUPLICATE,
	CODE_GENIE,
	CODE_SHARK,
	CODE_INEFFECTIVE,
};



#define		GIF_COLOR		0x01
#define		GIF_JAPAN		0x02

struct GAMEINFO
{
	char	szName[0x100], szTitle[17];
	BYTE	Flags;
	WORD	Checksum;
	CList	*pCheats;
};



struct CHEAT
{
	char	szComment[0x100];
	CList	*pCodes;
};



class CCheats
{
private:
	CList		*m_pGames;
	HWND		m_hTreeView;
	HTREEITEM	m_hGameItem;
	CGameBoy	*m_pGameBoy;

	BOOL		CompareCodes(char *pszCode1, char *pszCode2);

public:
	CCheats();
	~CCheats();

	void		SetGameBoy();
	void		ReleaseGameBoy();

	BOOL		MergeFile(char *pszFilename);
	BOOL		Save();
	BOOL		ShowCheatDialog();
	void		DisableDuplicateCodes(HTREEITEM hti);
	void		UpdateCheckMarks(HTREEITEM hti);
	BOOL		FillWindow(HWND hTreeView);
	void		Release();
	BOOL		UpdateCheats();
	BOOL		AddCheat(char *pszCheat);
	BOOL		AddCode(char *pszCode);
	BOOL		CanReplaceCheat(HTREEITEM hti, char *pszCheat);
	BOOL		CanReplaceCode(HTREEITEM hti, char *pszCode, char *pszNewCode);
	int			VerifyCode(char *pszCode);
	void		DeleteItem(HTREEITEM hti);
} CCHEATS_CPP Cheats;



#endif		//CCHEATS_H

