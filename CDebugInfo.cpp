#include	<windows.h>

#include	"CDebugInfo.h"
#include	"Game Lad.h"



CDebugInfo::CDebugInfo()
{
	m_pLabelList = NULL;
	m_dwCurrentItem = 0;
}



CDebugInfo::~CDebugInfo()
{
	if (m_pLabelList)
	{
		delete m_pLabelList;
	}
}



BOOL ReadLine(HANDLE hFile, char *pBuffer)
{
	DWORD		nBytes;
	int			Pos;
	char		c;


	Pos = -1;
	do
	{
		if (!ReadFile(hFile, &c, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage(hWnd);
			return true;
		}
		if (pBuffer)
		{
			if (Pos < MAX_LABELLENGTH)
			{
				pBuffer[++Pos] = c;
			}
		}
		if (nBytes == 0)
		{
			if (pBuffer)
			{
				pBuffer[Pos] = 0;
			}
			return false;
		}
	}
	while (c != 10 && c != 13);

	if (pBuffer)
	{
		pBuffer[Pos] = 0;
		pBuffer[MAX_LABELLENGTH - 1] = 0;
	}
	return false;
}



BOOL RemoveWhiteSpaces(HANDLE hFile, char *pc)
{
	DWORD		nBytes;


	do
	{
		if (!ReadFile(hFile, pc, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage(hWnd);
			return true;
		}
		if (nBytes == 0)
		{
			CloseHandle(hFile);
			return true;
		}
		if (*pc == ';')
		{
			ReadLine(hFile, NULL);
		}
	}
	while (*pc <= 32 || *pc > 127 || *pc == ';');

	return false;
}



BOOL CDebugInfo::LoadFile(char *pszRomPath)
{
	char		szFilename[MAX_PATH + 4];
	HANDLE		hFile;
	DWORD		nBytes;
	char		Buffer[MAX_LABELLENGTH];
	WORD		Offset;
	BYTE		Bank;


	strcpy(szFilename, pszRomPath);
	if (strchr(szFilename, '.'))
	{
		*strrchr(szFilename, '.') = '\0';
	}
	strcat(szFilename, ".sym");

	if ((hFile = CreateFile(szFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		return true;
	}
	while (true)
	{
		if (RemoveWhiteSpaces(hFile, &Buffer[0]))
		{
			return false;
		}
		if (!ReadFile(hFile, &Buffer[1], 7, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage(hWnd);
			return false;
		}
		if (nBytes == 0 || Buffer[2] != ':' || Buffer[7] != ' ' || HexToNum(&Buffer[0]) || HexToNum(&Buffer[1])
			|| HexToNum(&Buffer[3]) || HexToNum(&Buffer[4]) || HexToNum(&Buffer[5]) || HexToNum(&Buffer[6]))
		{
			CloseHandle(hFile);
			return false;
		}
		Bank = (Buffer[0] << 4) | Buffer[1];
		Offset = (Buffer[3] << 12) | (Buffer[4] << 8) | (Buffer[5] << 4) | Buffer[6];
		if (ReadLine(hFile, Buffer))
		{
			return false;
		}

		if (AddLabel(Bank, Offset, Buffer))
		{
			CloseHandle(hFile);
			return false;
		}
	}
}



BOOL CDebugInfo::AddLabel(BYTE Bank, WORD Offset, char *pszName)
{
	LABELDATA		LabelData;


	if (!m_pLabelList)
	{
		if (!(m_pLabelList = new CList()))
		{
			return true;
		}
	}

	LabelData.Offset = Offset;
	LabelData.Bank = Bank;
	strcpy((char *)&LabelData.LabelName, pszName);
	if (!m_pLabelList->NewItem(sizeof(LabelData), &LabelData))
	{
		return true;
	}

	return false;
}



char *CDebugInfo::GetLabel(BYTE Bank, WORD Offset, DWORD dwStartItemNo)
{
	LABELDATA		*pLabelData;


	if (!m_pLabelList)
	{
		return NULL;
	}

	m_dwCurrentItem = dwStartItemNo + 1;

	while (pLabelData = (LABELDATA *)m_pLabelList->GetItem(m_dwCurrentItem))
	{
		if (pLabelData->Offset == Offset && pLabelData->Bank == Bank)
		{
			return (char *)&pLabelData->LabelName;
		}
		m_dwCurrentItem++;
	}

	return NULL;
}



char *CDebugInfo::GetLabel(BYTE Bank, WORD Offset)
{
	return GetLabel(Bank, Offset, 0);
}



char *CDebugInfo::GetNextLabel(BYTE Bank, WORD Offset)
{
	return GetLabel(Bank, Offset, m_dwCurrentItem);
}

