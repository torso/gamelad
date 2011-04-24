#include	<windows.h>

#include	"CDebugInfo.h"
#include	"Game Lad.h"



CDebugInfo::CDebugInfo()
{
	m_pLabelList = NULL;
}



CDebugInfo::~CDebugInfo()
{
	if (m_pLabelList)
	{
		delete m_pLabelList;
	}
}



DWORD CDebugInfo::nLabels()
{
	if (!m_pLabelList)
	{
		return 0;
	}

	return m_pLabelList->GetMaxItemNo();
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
			DisplayErrorMessage();
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
			DisplayErrorMessage();
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
			DisplayErrorMessage();
			return false;
		}
		if (Buffer[2] == ':')
		{
			if (nBytes == 0 || Buffer[7] != ' ' || HexToNum(&Buffer[0]) || HexToNum(&Buffer[1])
				|| HexToNum(&Buffer[3]) || HexToNum(&Buffer[4]) || HexToNum(&Buffer[5]) || HexToNum(&Buffer[6]))
			{
				CloseHandle(hFile);
				return false;
			}
			Bank = (Buffer[0] << 4) | Buffer[1];
			Offset = (Buffer[3] << 12) | (Buffer[4] << 8) | (Buffer[5] << 4) | Buffer[6];
		}
		else if (Buffer[4] == ':')
		{
			if (!ReadFile(hFile, &Buffer[8], 2, &nBytes, NULL))
			{
				CloseHandle(hFile);
				DisplayErrorMessage();
				return false;
			}
			if (nBytes == 0 || Buffer[9] != ' ' || HexToNum(&Buffer[2]) || HexToNum(&Buffer[3])
				|| HexToNum(&Buffer[5]) || HexToNum(&Buffer[6]) || HexToNum(&Buffer[7]) || HexToNum(&Buffer[8]))
			{
				CloseHandle(hFile);
				return false;
			}
			Bank = (Buffer[2] << 4) | Buffer[3];
			Offset = (Buffer[5] << 12) | (Buffer[6] << 8) | (Buffer[7] << 4) | Buffer[8];
		}
		else
		{
			CloseHandle(hFile);
			return false;
		}
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



BOOL CDebugInfo::AddLabel(WORD Bank, WORD Offset, char *pszName)
{
	LABELDATA		LabelData;


	if (!m_pLabelList)
	{
		if (!(m_pLabelList = new CList()))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
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



void CDebugInfo::ResetSearch()
{
	m_pLabelList->ResetSearch();
}



char *CDebugInfo::GetNextLabel(WORD Bank, WORD Offset)
{
	LABELDATA		*pLabelData;


	if (!m_pLabelList)
	{
		return NULL;
	}

	while (pLabelData = (LABELDATA *)m_pLabelList->GetNextItem())
	{
		if (pLabelData->Offset == Offset && pLabelData->Bank == Bank)
		{
			return (char *)&pLabelData->LabelName;
		}
	}

	return NULL;
}

