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

public:
	CDebugInfo();
	~CDebugInfo();

	DWORD		nLabels();
	BOOL		LoadFile(char *pszRomPath);
	BOOL		AddLabel(BYTE Bank, WORD Offset, char *pszName);
	void		ResetSearch();
	char		*GetNextLabel(BYTE Bank, WORD Offset);
};



#endif		CDEBUGINFO_H

