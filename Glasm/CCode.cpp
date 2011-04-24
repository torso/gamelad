#include	<windows.h>

#define		CCODE_CPP

#include	"..\CString\CString.h"
#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"CSolver.h"
#include	"Glasm.h"
#include	"CInputFile.h"
#include	"CCode.h"



void CALLBACK DeleteSection(void *p)
{
	if (((SECTION *)p)->Size > 4)
	{
		delete ((SECTION *)p)->pBytes;
	}

	delete p;
}



CCode::CCode()
{
	Offset.pOffset = NULL;
	Offset.BankNumber = 0;
	pSections = NULL;
}



CCode::~CCode()
{
	if (pSections)
	{
		delete pSections;
	}
}



BOOL CCode::SetOffset(DWORD SectionFlags, POINTER *pPointer)
{
	//Check if pPointer is a valid pointer

	if (!Offset.pOffset)
	{
		if (!(Offset.pOffset = new CSolver()))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}

	if (*Offset.pOffset = pPointer->pOffset)
	{
		return true;
	}
	Offset.BankNumber = pPointer->BankNumber;

	return false;
}



BOOL CCode::SetOffset(POINTER *pPointer)
{
	if (!Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		return true;
	}

	if (*Offset.pOffset = pPointer->pOffset)
	{
		return true;
	}
	Offset.BankNumber = pPointer->BankNumber;

	return false;
}



BOOL CCode::GetOffset(POINTER *pPointer)
{
	if (*pPointer->pOffset = Offset.pOffset)
	{
		return true;
	}
	pPointer->BankNumber = Offset.BankNumber;

	return false;
}



BOOL CCode::InsertData(DWORD Bytes, WORD Flags)
{
	SECTION		*pSection;


	if (!Offset.pOffset)
	{
		CompileError(COMPILE_ERROR_CODEOUTSIDESECTION, NULL);
		return true;
	}

	if (!pSections)
	{
		if (!(pSections = new CList(NULL, DeleteSection)))
		{
			return true;
		}
	}

	if (!(pSection = (SECTION *)pSections->NewItem(sizeof(SECTION))))
	{
		return true;
	}

	pSection->File = InputFiles.GetCurrentItemNo();
	pSection->LineFrom = ((CInputFile *)InputFiles.GetCurrentItem())->GetLastLine() + 1;
	pSection->Lines = ((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine() - pSection->LineFrom + 1;

	pSection->Size = 1;
	if (Bytes & 0x0000FF00)
	{
		pSection->Size = 2;
	}
	if (Bytes & 0x00FF0000)
	{
		pSection->Size = 3;
	}
	if (Bytes & 0xFF000000)
	{
		pSection->Size = 4;
	}
	pSection->Flags = Flags;
	pSection->Bytes = Bytes;

	*Offset.pOffset += pSection->Size;

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


	if (!pSections)
	{
		MaxItemNo = 0;
		WriteToFile(hObjFile, &MaxItemNo, sizeof(MaxItemNo));
		return false;
	}

	MaxItemNo = pSections->GetMaxItemNo();
	ItemNo = 1;
	while (ItemNo <= MaxItemNo)
	{
		pSection = (SECTION *)pSections->GetItem(ItemNo);
		if (pSection->File != File || pSection->LineFrom != Line)
		{
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
		cout << "Lines: " << pSection->Lines << " (" << Line << ")" << endl;
		if (pSection->Flags & SF_CODE)
		{
			Data = 1;
			WriteToFile(hObjFile, &Data, sizeof(WORD));
			Data = 2;
			WriteToFile(hObjFile, &Data, sizeof(BYTE));
			WriteToFile(hObjFile, ((BYTE *)&pSection->Bytes) + pSection->Size - 1, 1);
			if (pSection->Size != 1)
			{
				Data = pSection->Size - 1;
				WriteToFile(hObjFile, &Data, sizeof(WORD));
				Data = 1;
				WriteToFile(hObjFile, &Data, sizeof(BYTE));
				WriteToFile(hObjFile, &pSection->Bytes, (unsigned)pSection->Size - 1);
			}
		}
		else
		{
		}
		Data = 0;
		WriteToFile(hObjFile, &Data, sizeof(WORD));
		/*WriteToFile(hObjFile, &pSection->Size, sizeof(pSection->Size));
		cout << "Size: " << (int)pSection->Size << endl;
		if (!(p = new BYTE[pSection->Size + (((pSection->Size == 1 && pSection->Flags & SF_CODE) || pSection->Flags & SF_UNKNOWN) ? 2 : 4)]))
		{
			return true;
		}
		if (pSection->Size > 4)
		{
			p[0] = 1;
			p[1] = pSection->Size;
			CopyMemory(&p[2], pSection->pBytes, pSection->Size);
			WriteToFile(hObjFile, p, (unsigned)pSection->Size + 2);
		}
		else
		{
			if (pSection->File & SF_CODE)
			{
				p[0] = 2;
				p[1] = 1;
				p[2] = (BYTE)(pSection->Bytes >> (pSection->Size * 8 - 8));
				if (pSection->Size > 1)
				{
					p[3] = 1;
					p[4] = (BYTE)pSection->Size - 1;
					CopyMemory(&p[5], &pSection->Bytes, pSection->Size - 1);
					WriteToFile(hObjFile, p, (unsigned)pSection->Size + 4);
				}
				else
				{
					WriteToFile(hObjFile, p, (unsigned)pSection->Size + 2);
				}
			}
			else
			{
				p[0] = 1;
				p[1] = pSection->Size;
				CopyMemory(&p[2], &pSection->Bytes, pSection->Size);
				WriteToFile(hObjFile, p, (unsigned)pSection->Size + 2);
			}
		}*/
		cout << endl;
		ItemNo++;
	}

	return false;
}

