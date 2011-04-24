#include	<windows.h>

#define		CIDENTIFIERS_CPP

#include	"..\CString\CString.h"
#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"CSolver.h"
#include	"Glasm.h"
#include	"OpCodes.h"
#include	"CIdentifiers.h"
#include	"CInputFile.h"



void CALLBACK DeleteLabelList(void *pList)
{
	delete ((IDENTIFIER *)pList)->pName;

	delete pList;
}



CIdentifiers::CIdentifiers()
{
	pIdentifierList = NULL;
}



CIdentifiers::~CIdentifiers()
{
	if (pIdentifierList)
	{
		delete pIdentifierList;
	}
}



BOOL CIdentifiers::AddIdentifier(char *pName, BOOL CopyName, DWORD Type, POINTER *pPointer)
{
	DWORD			ItemNo;
	IDENTIFIER		*pIdentifier;


	if (pIdentifierList)
	{
		ItemNo = 1;
		while (pIdentifier = (IDENTIFIER *)pIdentifierList->GetItem(ItemNo++))
		{
			if (!strcmp(pIdentifier->pName, pName))
			{
				if (pIdentifier->Flags & IF_DEFINED || (pIdentifier->Flags & IF_TYPE) != Type)
				{
					CompileError(COMPILE_ERROR_IDENTIFIERREDEFINED, pName);
					if (!CopyName)
					{
						delete pName;
					}
					return false;
				}
				pIdentifier->Flags |= IF_DEFINED | Type;
				pIdentifier->File = InputFiles.GetCurrentItemNo();
				pIdentifier->Line = ((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine();
				if (!CopyName)
				{
					delete pName;
				}
				CopyMemory(&pIdentifier->Pointer, pPointer, sizeof(POINTER));
				return false;
			}
		}
	}
	else
	{
		if (!(pIdentifierList = new CList(NULL, DeleteLabelList)))
		{
			if (!CopyName)
			{
				delete pName;
			}
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
	}

	if (!(pIdentifier = (IDENTIFIER *)pIdentifierList->NewItem(sizeof(IDENTIFIER))))
	{
		if (!CopyName)
		{
			delete pName;
		}
		return true;
	}
	pIdentifier->Flags = IF_DEFINED | Type;
	pIdentifier->File = InputFiles.GetCurrentItemNo();
	pIdentifier->Line = ((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine();
	if (CopyName)
	{
		if (!(pIdentifier->pName = new char[strlen(pName) + 1]))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return true;
		}
		strcpy(pIdentifier->pName, pName);
	}
	else
	{
		pIdentifier->pName = pName;
	}
	CopyMemory(&pIdentifier->Pointer, pPointer, sizeof(POINTER));

	return false;
}



BOOL CIdentifiers::AddIdentifier(CString &String, DWORD Type, POINTER *pPointer)
{
	char		*pName;


	String.GetString(&pName);
	return AddIdentifier(pName, false, Type, pPointer);
}



void CIdentifiers::GetIdentifier(char *pName, IDENTIFIER *pIdentifier)
{
	DWORD		ItemNo;
	IDENTIFIER	*pCurrentIdentifier;


	pIdentifier->Flags = 0;

	ItemNo = MAX_MNEMONIC;
	while (ItemNo--)
	{
		if (!stricmp(pName, MnemonicList[ItemNo].Name))
		{
			pIdentifier->Flags |= IF_MNEMONIC;
			pIdentifier->pName = (char *)&MnemonicList[ItemNo].Name;
			pIdentifier->Mnemonic = (MNEMONIC *)&MnemonicList[ItemNo];
			break;
		}
	}
	/*ItemNo = MAXMNEMONIC0;
	while (ItemNo--)
	{
		if (!stricmp(pName, MnemonicList0[ItemNo].Name))
		{
			pIdentifier->Flags |= IF_MNEMONIC0;
			pIdentifier->pName = (char *)&MnemonicList0[ItemNo].Name;
			pIdentifier->Mnemonic0 = (MNEMONIC0 *)&MnemonicList0[ItemNo];
			break;
		}
	}

	ItemNo = MAXMNEMONIC1;
	while (ItemNo--)
	{
		if (!stricmp(pName, MnemonicList1[ItemNo].Name))
		{
			pIdentifier->Flags |= IF_MNEMONIC1;
			pIdentifier->pName = (char *)&MnemonicList1[ItemNo].Name;
			pIdentifier->Mnemonic1 = (MNEMONIC1 *)&MnemonicList1[ItemNo];
			break;
		}
	}

	ItemNo = MAXMNEMONIC2;
	while (ItemNo--)
	{
		if (!stricmp(pName, MnemonicList2[ItemNo].Name))
		{
			pIdentifier->Flags |= IF_MNEMONIC2;
			pIdentifier->pName = (char *)&MnemonicList2[ItemNo].Name;
			pIdentifier->Mnemonic2 = (MNEMONIC2 *)&MnemonicList2[ItemNo];
			break;
		}
	}*/

	if (pIdentifier->Flags & IF_MNEMONIC)
	{
		return;
	}

	if (pIdentifierList)
	{
		ItemNo = 1;
		while (pCurrentIdentifier = (IDENTIFIER *)pIdentifierList->GetItem(ItemNo++))
		{
			if (!strcmp(pCurrentIdentifier->pName, pName))
			{
				CopyMemory(pIdentifier, pCurrentIdentifier, sizeof(IDENTIFIER));
				return;
			}
		}
	}

	return;
}



BOOL CIdentifiers::GetIdentifier(CString &String, IDENTIFIER *pIdentifier)
{
	char		*pName;


	if (String.GetString(&pName))
	{
		return true;
	}

	if (!pName)
	{
		pIdentifier->Flags = 0;
		return false;
	}

	GetIdentifier(pName, pIdentifier);
	delete pName;

	return false;
}



BOOL CIdentifiers::SaveObj(HANDLE hObjFile)
{
	DWORD		BytesWritten, MaxItemNo;


	if (!pIdentifierList)
	{
		MaxItemNo = 0;
		WriteToFile(hObjFile, &MaxItemNo, sizeof(MaxItemNo));
		return false;
	}

	MaxItemNo = pIdentifierList->GetMaxItemNo();
	WriteToFile(hObjFile, &MaxItemNo, sizeof(MaxItemNo));

	return false;
}

