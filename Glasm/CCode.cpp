#include	<windows.h>

#define		CCODE_CPP

#include	"..\CString\CString.h"
#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"..\CSolver\CSolver.h"
#include	"Glasm.h"
#include	"CInputFile.h"
#include	"CCode.h"



void CALLBACK DeleteSection(void *p, DWORD dw)
{
	if (((SECTION *)p)->Size > 4)
	{
		delete ((SECTION *)p)->pBytes;
	}

	delete p;
}



CCode::CCode()
{
	m_Offset.pOffset = NULL;
	m_Offset.BankNumber = 0;
	m_pSections = NULL;
}



CCode::~CCode()
{
	if (m_pSections)
	{
		delete m_pSections;
	}
}



BOOL CCode::SetOffset(DWORD SectionFlags, POINTER *pPointer)
{
	if (!m_Offset.pOffset)
	{
		if (!(m_Offset.pOffset = new CSolver()))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}

	if (*m_Offset.pOffset = pPointer->pOffset)
	{
		return true;
	}
	m_Offset.BankNumber = pPointer->BankNumber;

	return false;
}



BOOL CCode::SetOffset(POINTER *pPointer)
{
	if (!m_Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		return true;
	}

	if (*m_Offset.pOffset = pPointer->pOffset)
	{
		return true;
	}
	m_Offset.BankNumber = pPointer->BankNumber;

	return false;
}



BOOL CCode::GetOffset(POINTER *pPointer)
{
	if (!m_Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		if (pPointer->pOffset)
		{
			delete pPointer->pOffset;
			pPointer->pOffset = NULL;
		}
		return false;
	}

	if (!pPointer->pOffset)
	{
		if (!(pPointer->pOffset = new CSolver()))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}
	if (*pPointer->pOffset = m_Offset.pOffset)
	{
		return true;
	}
	pPointer->BankNumber = m_Offset.BankNumber;

	return false;
}



BOOL CCode::InsertData(BYTE Byte, DWORD Flags)
{
	SECTION		*pSection;


	if (!m_Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		return false;
	}

	if (!m_pSections)
	{
		if (!(m_pSections = new CList(NULL, DeleteSection)))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}

	if (!(pSection = (SECTION *)m_pSections->NewItem(sizeof(SECTION))))
	{
		return true;
	}

	pSection->File = InputFiles.GetCurrentItemNo();
	pSection->LineFrom = ((CInputFile *)InputFiles.GetCurrentItem())->GetLastLine() + 1;
	pSection->Lines = ((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine() - pSection->LineFrom + 1;

	pSection->Size = 1;
	pSection->Flags = Flags;
	pSection->Bytes = Byte;

	*m_Offset.pOffset += pSection->Size;

	return false;
}



BOOL CCode::InsertData(CSolver &Expression, BYTE Size, DWORD Flags)
{
	SECTION		*pSection;


	if (!m_Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		return false;
	}

	if (!m_pSections)
	{
		if (!(m_pSections = new CList(NULL, DeleteSection)))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}

	if (!(pSection = (SECTION *)m_pSections->NewItem(sizeof(SECTION))))
	{
		return true;
	}

	pSection->File = InputFiles.GetCurrentItemNo();
	pSection->LineFrom = ((CInputFile *)InputFiles.GetCurrentItem())->GetLastLine() + 1;
	pSection->Lines = ((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine() - pSection->LineFrom + 1;

	pSection->Size = Size;
	pSection->Flags = Flags | SF_EXPRESSION;
	if (!(pSection->pExpression = new CSolver()))
	{
		FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
		return true;
	}
	*pSection->pExpression = Expression;

	*m_Offset.pOffset += pSection->Size;

	return false;
}



/*BOOL CCode::SetOffset(BYTE NewBank, WORD NewOffset)
{
	if (NewOffset < 0x4000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0x4000 && NewOffset < 0x8000)
	{
		if (NewBank >= MAX_BANK)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0x8000 && NewOffset < 0xA000)
	{
		if (NewBank > 1)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0xA000 && NewOffset < 0xC000)
	{
		if (NewBank >= 16)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0xC000 && NewOffset < 0xD000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0xD000 && NewOffset < 0xE000)
	{
		if (NewBank < 1 || NewBank > 7)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset > 0xE000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}


	if (!Banks[NewBank])
	{
		if (!(Banks[NewBank] = new BANK))
		{
			cout << "Insufficient memory." << endl;
			FatalError = true;
			return true;
		}
		ZeroMemory(Banks[NewBank], sizeof(*Banks[NewBank]));
	}

	Offset = NewOffset;
	BankNo = NewBank;

	return false;
}*/



#include	<iostream.h>
BOOL CCode::SaveObj(HANDLE hObjFile)
{
	DWORD		BytesWritten, MaxItemNo, ItemNo, /*MaxItemNo2, ItemNo2,*/ File = 0, Line;
	SECTION		*pSection;
	DWORD		Data;
	BOOL		Size0 = false;
	DWORD		dwValue;
	char		*pszExpression;


	if (!m_pSections)
	{
		MaxItemNo = 0;
		WriteToFile(hObjFile, &MaxItemNo, sizeof(MaxItemNo));
		return false;
	}

	cout << endl << "Writing code" << endl;

	MaxItemNo = m_pSections->GetMaxItemNo();
	ItemNo = 1;

	pSection = (SECTION *)m_pSections->GetItem(ItemNo);
	File = pSection->File;
	WriteToFile(hObjFile, &File, sizeof(File));
	cout << "File: " << File << endl;
	Line = pSection->LineFrom;
	WriteToFile(hObjFile, &Line, sizeof(Line));
	cout << "Line: " << Line << endl;
	//Address

	while (ItemNo <= MaxItemNo)
	{
		pSection = (SECTION *)m_pSections->GetItem(ItemNo);
		if (pSection->Lines != 0)
		{
			if (Size0)
			{
				Size0 = false;

				//Size
				Data = 0;
				WriteToFile(hObjFile, &Data, sizeof(WORD));
				cout << "  Size: 0" << endl << endl;
			}
			if (pSection->File != File || pSection->LineFrom != Line)
			{
				//Lines
				Data = 0;
				WriteToFile(hObjFile, &Data, sizeof(Data));
				cout << " Lines: " << 0 << endl;

				File = pSection->File;
				WriteToFile(hObjFile, &File, sizeof(File));
				cout << "File: " << File << endl;
				Line = pSection->LineFrom;
				WriteToFile(hObjFile, &Line, sizeof(Line));
				cout << "Line: " << Line << endl;
				//Address
			}
			Line += pSection->Lines;
			WriteToFile(hObjFile, &pSection->Lines, sizeof(pSection->Lines));
			cout << " Lines: " << pSection->Lines << " (" << Line << ")" << endl;
		}
		if (pSection->Flags & (SF_CODE | SF_CONST))
		{
			Size0 = true;

			Data = pSection->Size;
			WriteToFile(hObjFile, &Data, sizeof(WORD));
			cout << "  Size: " << Data << endl;
			pszExpression = NULL;
			if (pSection->Flags & SF_EXPRESSION)
			{
				if (pSection->pExpression->GetNumber(&dwValue, &pszExpression))
				{
					return true;
				}
			}
			Data = pSection->Flags & (SF_CODE | SF_CONST | (pszExpression ? SF_EXPRESSION : 0));
			WriteToFile(hObjFile, &Data, sizeof(BYTE));
			if (Data & 1)
			{
				cout << "  Type: const";
			}
			else if (Data & 2)
			{
				cout << "  Type: code";
			}
			else
			{
				cout << "  Type: unknown";
			}
			if (Data & SF_EXPRESSION)
			{
				cout << ", expression";
			}
			cout << endl;
			if (pSection->Flags & SF_EXPRESSION)
			{
				if (pszExpression)
				{
					WriteToFile(hObjFile, &pszExpression, strlen(pszExpression) + 1);
					delete pszExpression;
				}
				else
				{
					WriteToFile(hObjFile, &dwValue, (unsigned)pSection->Size);
				}
			}
			else
			{
				if (pSection->Size <= 4)
				{
					WriteToFile(hObjFile, &pSection->Bytes, (unsigned)pSection->Size);
				}
				else
				{
					WriteToFile(hObjFile, pSection->pBytes, (unsigned)pSection->Size);
				}
			}
		}
		else
		{
		}

		ItemNo++;
	}

	return false;
}

