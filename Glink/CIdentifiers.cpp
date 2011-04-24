#include	<windows.h>
#include	<iostream.h>
#include	<crtdbg.h>
#include	"glink.h"
#include	"cidentifiers.h"



CIdentifiers::CIdentifiers()
{
	LabelList = NULL;
	VariableList = NULL;
	//StructDefList = NULL;

	nIdentifiers = 0;
}



CIdentifiers::~CIdentifiers()
{
	LABELLIST		*NextLabelList;
	VARIABLELIST	*NextVariableList;
	//STRUCTDEFLIST	*NextStructDefList;


	while (LabelList)
	{
		NextLabelList = LabelList->pNext;
		delete LabelList;
		LabelList = NextLabelList;
	}

	while (VariableList)
	{
		NextVariableList = VariableList->pNext;
		delete VariableList;
		VariableList = NextVariableList;
	}
}



LABEL *CIdentifiers::CreateLabel(char *Name)
{
	LABELLIST	*CurrentLabelList = LabelList;
	int			CurrentLabel;


	//If no list is available, create one.
	if (!CurrentLabelList)
	{
		if (!(CurrentLabelList = LabelList = new LABELLIST))
		{
			return NULL;
		}
		LabelList->nLabels = 0;
		LabelList->pNext = NULL;
	}


	//Find last list
	while (true)
	{
		for (CurrentLabel = 0; CurrentLabel < CurrentLabelList->nLabels; CurrentLabel++)
		{
			if (!strcmp(CurrentLabelList->Label[CurrentLabel].Name, Name))
			{
				return &CurrentLabelList->Label[CurrentLabel];
			}
		}
		if (!CurrentLabelList->pNext)
		{
			//If the last list is full, create a new one
			if (CurrentLabelList->nLabels == 100)
			{
				if (!(CurrentLabelList->pNext = new LABELLIST))
				{
					return NULL;
				}
				CurrentLabelList = CurrentLabelList->pNext;

				CurrentLabelList->nLabels = 0;
				CurrentLabelList->pNext = NULL;
			}

			//Initialize label
			strcpy(CurrentLabelList->Label[CurrentLabelList->nLabels].Name, Name);
			CurrentLabelList->Label[CurrentLabelList->nLabels].HasValue = false;
			CurrentLabelList->Label[CurrentLabelList->nLabels].Type = CID_TYPE_UNKNOWN;

			//Increment number of identifiers
			nIdentifiers++;

			return &CurrentLabelList->Label[CurrentLabelList->nLabels++];
		}
		CurrentLabelList = CurrentLabelList->pNext;
	}
}



int CIdentifiers::AddLabel(char *Name, WORD Offset, BYTE Bank, WORD File, DWORD Line)
{
	LABEL	*NewLabel;


	if (!(NewLabel = CreateLabel(Name)))
	{
		Error("Insufficient memory.");
		FatalError = true;
		return CID_ERROR;
	}

	if (NewLabel->HasValue)
	{
		ErrorHeader();
		cout << "Label \'" << NewLabel->Name << "\' redefined" << endl;
		return CID_ALREADYDEFINED;
	}

	switch (NewLabel->Type)
	{
	case CID_TYPE_IMPORT:
		//Imported symbols should not be given a value
		ErrorHeader();
		cout << "Imported symbol \'" << NewLabel << "\' defined." << endl;
		return CID_NOVALUE;

	case CID_TYPE_UNKNOWN:
		//Default to local
		NewLabel->Type = CID_TYPE_LOCAL;
	}

	NewLabel->HasValue = true;
	NewLabel->Offset = Offset;
	NewLabel->Bank = Bank;
	NewLabel->File = File;
	NewLabel->Line = Line;

	return CID_SUCCESS;
}



int CIdentifiers::GetLabel(char *Name)
{
	LABEL	*NewLabel;


	if (!(NewLabel = CreateLabel(Name)))
	{
		Error("Insufficient memory.");
		FatalError = true;
		return CID_ERROR;
	}

	if (NewLabel->Type == CID_TYPE_UNKNOWN)
	{
		//Default to local
		NewLabel->Type = CID_TYPE_LOCAL;
	}

	if (!NewLabel->HasValue)
	{
		return CID_NOVALUE;
	}

	return (NewLabel->Bank << 16) | NewLabel->Offset;
}



int CIdentifiers::GetLabel(char *Name, BYTE Type)
{
	LABEL	*NewLabel;


	if (!(NewLabel = CreateLabel(Name)))
	{
		Error("Insufficient memory.");
		FatalError = true;
		return CID_ERROR;
	}

	if (NewLabel->Type == CID_TYPE_UNKNOWN)
	{
		NewLabel->Type = Type;
	}
	else
	{
		if (NewLabel->Type != Type)
		{
			ErrorHeader();
			cout << "Linkage specification contradicts earlier specification for \'" << NewLabel->Name << "\'." << endl;
		}
		else
		{
			WarningHeader();
			cout << "Linkage specification for \'" << NewLabel->Name << "\' already defined." << endl;
		}
		return CID_ALREADYDEFINED;
	}

	return CID_SUCCESS;
}



/*int CIdentifiers::GetValue(char *Name, int *Value, BYTE *Bank)
{
	_IdentifierList		*CurrentIdentifierList = IdentifierList;
	int					nIdentifier;


	if (!CurrentIdentifierList)
	{
		return CID_NOIDENTIFIER;
	}


	do
	{
		for (nIdentifier = 0; nIdentifier < 100; nIdentifier++)
		{
			if (!strcmp(CurrentIdentifierList->Identifier[nIdentifier].Name, Name))
			{
				if (CurrentIdentifierList->Identifier[nIdentifier].HasValue)
				{
					if (Value)
					{
						*Value = CurrentIdentifierList->Identifier[nIdentifier].Value;
					}
					if (Bank)
					{
						*Bank = CurrentIdentifierList->Identifier[nIdentifier].Bank;
					}
					return 0;
				}
				return CID_NOVALUE;
			}
		}
		CurrentIdentifierList = CurrentIdentifierList->pNext;
	}
	while (CurrentIdentifierList);

	return CID_NOIDENTIFIER;
}



void CIdentifiers::SetValue(char *Name, int Value, BYTE Bank, WORD File, DWORD Line)
{
	_IdentifierList		*CurrentIdentifierList = IdentifierList;
	int					nIdentifier;


	_ASSERT(CurrentIdentifierList);


	do
	{
		for (nIdentifier = 0; nIdentifier < 100; nIdentifier++)
		{
			if (!strcmp(CurrentIdentifierList->Identifier[nIdentifier].Name, Name))
			{
				CurrentIdentifierList->Identifier[nIdentifier].Value = Value;
				CurrentIdentifierList->Identifier[nIdentifier].HasValue = true;
				CurrentIdentifierList->Identifier[nIdentifier].Bank = Bank;
				CurrentIdentifierList->Identifier[nIdentifier].File = File;
				CurrentIdentifierList->Identifier[nIdentifier].Line = Line;
				return;
			}
		}
		CurrentIdentifierList = CurrentIdentifierList->pNext;
	}
	while (CurrentIdentifierList);

	_ASSERT(0);
}*/



BOOL CIdentifiers::SaveObj(HANDLE hObjFile)
{
	DWORD			BytesWritten;
	LABELLIST		*CurrentLabelList = LabelList;
	VARIABLELIST	*CurrentVariableList = VariableList;
	int				nItem;


	WriteToFile(hObjFile, &nIdentifiers, sizeof(nIdentifiers));

	while (CurrentLabelList)
	{
		for (nItem = 0; nItem < CurrentLabelList->nLabels; nItem++)
		{
			if (!CurrentLabelList->Label[nItem].HasValue && (CurrentLabelList->Label[nItem].Type == CID_TYPE_LOCAL || CurrentLabelList->Label[nItem].Type == CID_TYPE_EXPORT))
			{
				ErrorHeader();
				cout << "Label \'" << CurrentLabelList->Label[nItem].Name << "\' was undefined.";
			}
			else
			{
				WriteToFile(hObjFile, CurrentLabelList->Label[nItem].Name, strlen(CurrentLabelList->Label[nItem].Name) + 1);
				WriteToFile(hObjFile, &CurrentLabelList->Label[nItem].Type, sizeof(CurrentLabelList->Label[nItem].Type));
				WriteToFile(hObjFile, "", 1); //Label
				if (CurrentLabelList->Label[nItem].Type != CID_TYPE_IMPORT && (CurrentLabelList->Label[nItem].Type != CID_TYPE_GLOBAL || CurrentLabelList->Label[nItem].HasValue))
				{
					WriteToFile(hObjFile, &CurrentLabelList->Label[nItem].Bank, sizeof(CurrentLabelList->Label[nItem].Bank));
					WriteToFile(hObjFile, &CurrentLabelList->Label[nItem].Offset, sizeof(CurrentLabelList->Label[nItem].Offset));

					WriteToFile(hObjFile, &CurrentLabelList->Label[nItem].File, sizeof(CurrentLabelList->Label[nItem].File));
					WriteToFile(hObjFile, &CurrentLabelList->Label[nItem].Line, sizeof(CurrentLabelList->Label[nItem].Line));
				}
			}
		}
		CurrentLabelList = CurrentLabelList->pNext;
	}

					/*WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].StorageClass, sizeof(IdentifierList->Identifier[nIdentifier].StorageClass));
					if (IdentifierList->Identifier[nIdentifier].StorageClass != CID_LABEL)
					{
						WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Pointer, sizeof(IdentifierList->Identifier[nIdentifier].Pointer));
						WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Dimension, sizeof(IdentifierList->Identifier[nIdentifier].Dimension));
						if (IdentifierList->Identifier[nIdentifier].StorageClass != CID_STRUCT)
						{
							WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Struct, sizeof(IdentifierList->Identifier[nIdentifier].Struct));
						}
						else
						{
							WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Size, sizeof(IdentifierList->Identifier[nIdentifier].Size));
						}
					}
					if (IdentifierList->Identifier[nIdentifier].StorageClass == CID_EQU)
					{
						WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Value, sizeof(IdentifierList->Identifier[nIdentifier].Value));
					}
					else
					{
						if (IdentifierList->Identifier[nIdentifier].StorageClass == CID_TEXTEQU)
						{
							WriteToFile(hObjFile, (char *)IdentifierList->Identifier[nIdentifier].Value, strlen((char *)IdentifierList->Identifier[nIdentifier].Value) + 1);
						}
						else
						{
							WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Bank, sizeof(IdentifierList->Identifier[nIdentifier].Bank));
							WriteToFile(hObjFile, &IdentifierList->Identifier[nIdentifier].Value, sizeof(WORD));
						}
					}*/

	return CID_SUCCESS;
}

