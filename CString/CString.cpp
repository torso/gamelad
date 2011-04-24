#include	<windows.h>

#include	"CString.h"
#include	"..\Error.h"
//#include	"CSolver.h"
//#include	"Glasm.h"



CString::CString()
{
	pFirstBuffer = NULL;
	pLastBuffer = NULL;
	Length = 0;
}



CString::~CString()
{
	*this = '\0';
}



BOOL CString::operator =(char c)
{
	BUFFER		*pCurrentBuffer;


	while (pFirstBuffer)
	{
		pCurrentBuffer = pFirstBuffer;
		pFirstBuffer = pFirstBuffer->pNext;
		delete pCurrentBuffer;
	}

	pLastBuffer = NULL;
	Length = 0;

	return *this += c;
}



BOOL CString::operator =(char *pszString)
{
	BUFFER		*pCurrentBuffer;


	while (pFirstBuffer)
	{
		pCurrentBuffer = pFirstBuffer;
		pFirstBuffer = pFirstBuffer->pNext;
		delete pCurrentBuffer;
	}

	pLastBuffer = NULL;
	Length = 0;

	return *this += pszString;
}



BOOL CString::operator =(CString *String)
{
	return *this = *String;
}



BOOL CString::operator =(CString &String)
{
	BUFFER		*pCurrentBuffer;


	while (pFirstBuffer)
	{
		pCurrentBuffer = pFirstBuffer;
		pFirstBuffer = pFirstBuffer->pNext;
		delete pCurrentBuffer;
	}

	pLastBuffer = NULL;
	Length = 0;

	return *this += String;
}



BOOL CString::operator +=(char c)
{
	if (Length % BUFFER_SIZE == 0)
	{
		if (pLastBuffer)
		{
			if (!(pLastBuffer->pNext = new BUFFER))
			{
				FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
				return true;
			}
			pLastBuffer->pNext->pPrevious = pLastBuffer;
			pLastBuffer = pLastBuffer->pNext;
		}
		else
		{
			if (!(pLastBuffer = new BUFFER))
			{
				FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
				return true;
			}
			pLastBuffer->pPrevious = NULL;
			pFirstBuffer = pLastBuffer;
		}
		pLastBuffer->pNext = NULL;
	}

	pLastBuffer->pBuffer[(Length++) % BUFFER_SIZE] = c;

	return false;
}



BOOL CString::operator +=(char *pszString)
{
	if (!pszString)
	{
		return false;
	}

	while (pszString[0])
	{
		if (Length % BUFFER_SIZE == 0)
		{
			if (pLastBuffer)
			{
				if (!(pLastBuffer->pNext = new BUFFER))
				{
					FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
					return true;
				}
				pLastBuffer->pNext->pPrevious = pLastBuffer;
				pLastBuffer = pLastBuffer->pNext;
			}
			else
			{
				if (!(pLastBuffer = new BUFFER))
				{
					FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
					return true;
				}
				pLastBuffer->pPrevious = NULL;
				pFirstBuffer = pLastBuffer;
			}
			pLastBuffer->pNext = NULL;
		}
		pLastBuffer->pBuffer[(Length++) % BUFFER_SIZE] = pszString[0];
		pszString++;
	}

	return false;
}



BOOL CString::operator +=(CString *String)
{
	return *this += *String;
}



BOOL CString::operator +=(CString &String)
{
	DWORD		dwPosition;


	if (!String.Length)
	{
		return false;
	}

	dwPosition = 0;
	while (String[dwPosition])
	{
		if (Length % BUFFER_SIZE == 0)
		{
			if (pLastBuffer)
			{
				if (!(pLastBuffer->pNext = new BUFFER))
				{
					FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
					return true;
				}
				pLastBuffer->pNext->pPrevious = pLastBuffer;
				pLastBuffer = pLastBuffer->pNext;
			}
			else
			{
				if (!(pLastBuffer = new BUFFER))
				{
					FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
					return true;
				}
				pLastBuffer->pPrevious = NULL;
				pFirstBuffer = pLastBuffer;
			}
			pLastBuffer->pNext = NULL;
		}
		pLastBuffer->pBuffer[(Length++) % BUFFER_SIZE] = String[dwPosition];
		dwPosition++;
	}

	return false;
}



BOOL CString::operator ==(char c)
{
	if (Length != 1)
	{
		return false;
	}

	if (pFirstBuffer->pBuffer[0] == c)
	{
		return true;
	}

	return false;
}



BOOL CString::operator ==(char *pszString)
{
	BUFFER		*pCurrentBuffer = pFirstBuffer;


	if (!pszString)
	{
		return Length == 0;
	}

	//Both strings must be of the same length to be equal
	if (Length != strlen(pszString))
	{
		return false;
	}

	//Both strings are equal if they do not contain any data
	if (Length == 0)
	{
		return true;
	}

	while (pCurrentBuffer->pNext)
	{
		if (memcmp(pCurrentBuffer->pBuffer, pszString, BUFFER_SIZE))
		{
			return false;
		}
		pCurrentBuffer = pCurrentBuffer->pNext;
		pszString += BUFFER_SIZE;
	}
	if (memcmp(pCurrentBuffer->pBuffer, pszString, Length % BUFFER_SIZE))
	{
		return false;
	}

	//The strings are equal
	return true;
}



BOOL CString::operator ==(CString *String)
{
	return *this == *String;
}



BOOL CString::operator ==(CString &String)
{
	BUFFER		*pCurrentBuffer = pFirstBuffer, *pCurrentBuffer2 = String.pFirstBuffer;


	if (Length != String.Length)
	{
		return false;
	}
	if (Length == 0)
	{
		return true;
	}

	while (pCurrentBuffer->pNext)
	{
		if (memcmp(pCurrentBuffer->pBuffer, pCurrentBuffer2->pBuffer, BUFFER_SIZE))
		{
			return false;
		}
		pCurrentBuffer = pCurrentBuffer->pNext;
		pCurrentBuffer2 = pCurrentBuffer2->pNext;
	}
	if (memcmp(pCurrentBuffer->pBuffer, pCurrentBuffer2->pBuffer, Length % BUFFER_SIZE))
	{
		return false;
	}

	return true;
}



BOOL CString::operator !=(char c)
{
	return !(*this == c);
}



BOOL CString::operator !=(char *pszString)
{
	return !(*this == pszString);
}



BOOL CString::operator !=(CString *String)
{
	return !(*this == *String);
}



BOOL CString::operator !=(CString &String)
{
	return !(*this == String);
}



char CString::operator [](DWORD dwPosition)
{
	BUFFER		*pCurrentBuffer = pFirstBuffer;


	if (dwPosition >= Length)
	{
		return '\0';
	}

	while (dwPosition >= BUFFER_SIZE)
	{
		pCurrentBuffer = pCurrentBuffer->pNext;
		dwPosition -= BUFFER_SIZE;
	}

	return pCurrentBuffer->pBuffer[dwPosition];
}



BOOL CString::GetString(char **pBuffer)
{
	BUFFER		*pCurrentBuffer;
	DWORD		dwPosition;


	if (Length == 0)
	{
		*pBuffer = NULL;
		return false;
	}

	if (!(*pBuffer = new char[Length + 1]))
	{
		FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
		return true;
	}
	(*pBuffer)[Length] = 0;

	dwPosition = 0;
	for (pCurrentBuffer = pFirstBuffer; pCurrentBuffer->pNext; pCurrentBuffer = pCurrentBuffer->pNext)
	{
		CopyMemory(&(*pBuffer)[dwPosition], pCurrentBuffer->pBuffer, BUFFER_SIZE);
		dwPosition += BUFFER_SIZE;
	}
	CopyMemory(&(*pBuffer)[dwPosition], pCurrentBuffer->pBuffer, Length % BUFFER_SIZE);

	return false;
}



void CString::GetString(char *pBuffer, DWORD Size)
{
	BUFFER		*pCurrentBuffer = pFirstBuffer;


	if (Size > Length)
	{
		pBuffer[Length] = 0;
		Size = Length;
	}

	while (pCurrentBuffer && Size != 0)
	{
		CopyMemory(pBuffer, pCurrentBuffer->pBuffer, (Size < BUFFER_SIZE) ? Size : BUFFER_SIZE);
		if (Size <= BUFFER_SIZE)
		{
			break;
		}
		Size -= BUFFER_SIZE;

		pCurrentBuffer = pCurrentBuffer->pNext;
		pBuffer += BUFFER_SIZE;
	}
}



DWORD CString::GetLength()
{
	return Length;
}



CString &CString::ToLower()
{
	BUFFER		*pCurrentBuffer = pFirstBuffer;
	DWORD		dwPosition;


	while (pCurrentBuffer)
	{
		dwPosition = 0;
		while (dwPosition < BUFFER_SIZE)
		{
			if (pCurrentBuffer->pBuffer[dwPosition] == '\0')
			{
				return *this;
			}
			pCurrentBuffer->pBuffer[dwPosition] = tolower(pCurrentBuffer->pBuffer[dwPosition]);

			dwPosition++;
		}
		pCurrentBuffer = pCurrentBuffer->pNext;
	}

	return *this;
}



void CString::ToUpper()
{
	BUFFER		*pCurrentBuffer = pFirstBuffer;
	DWORD		dwPosition;


	while (pCurrentBuffer)
	{
		dwPosition = 0;
		while (dwPosition < BUFFER_SIZE)
		{
			if (pCurrentBuffer->pBuffer[dwPosition] == '\0')
			{
				return;
			}
			pCurrentBuffer->pBuffer[dwPosition] = toupper(pCurrentBuffer->pBuffer[dwPosition]);

			dwPosition++;
		}
		pCurrentBuffer = pCurrentBuffer->pNext;
	}
}



void CString::DeleteLastCharacter()
{
	if (!Length)
	{
		return;
	}

	if ((--Length) % BUFFER_SIZE == 0)
	{
		if (pLastBuffer->pPrevious)
		{
			pLastBuffer = pLastBuffer->pPrevious;
			delete pLastBuffer->pNext;
			pLastBuffer->pNext = NULL;
		}
		else
		{
			delete pLastBuffer;
			pFirstBuffer = NULL;
			pLastBuffer = NULL;
		}
	}
}

