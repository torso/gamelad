#ifndef		CDEBUGINFO_H
#define		CDEBUGINFO_H

#include	"CList\CList.h"



#define		MAX_LABELLENGTH		0x80



struct LABELDATA
{
	WORD	Offset;
	BYTE	Bank;
	char	LabelName[MAX_LABELLENGTH];
};



class CDebugInfo
{
private:
	CList		*m_pLabelList;
	DWORD		m_dwCurrentItem;

	char		*GetLabel(BYTE Bank, WORD Offset, DWORD dwStartItemNo);

public:
	CDebugInfo();
	~CDebugInfo();

	BOOL		LoadFile(char *pszRomPath);
	BOOL		AddLabel(BYTE Bank, WORD Offset, char *pszName);
	char		*GetLabel(BYTE Bank, WORD Offset);
	char		*GetNextLabel(BYTE Bank, WORD Offset);
};



#endif		CDEBUGINFO_H

