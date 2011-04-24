#include	<windows.h>
#include	<iostream.h>
#include	<conio.h>

#define		GLASM_CPP

#include	"..\CString\CString.h"
#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"..\CSolver\CSolver.h"
#include	"Glasm.h"
#include	"OpCodes.h"
#include	"CCode.h"
#include	"CIdentifiers.h"
#include	"CInputFile.h"



#ifdef _DEBUG

//This should NOT be defined when building roms, it will freeze Visual C++ (until Ctrl+Break is pressed)
#define	WAIT

#endif //_DEBUG



char			NumBuffer[10];
BOOL			bFatalError = false;
BOOL			bTerminate = false;
BOOL			GenerateCode = true;
DWORD			Errors = 0;
DWORD			Warnings = 0;



BOOL ChangeExtension(char *pszFilename, char *pszExtension)
{
	if (strrchr(pszFilename, '.') > strrchr(pszFilename, '\\'))
	{
		*strrchr(pszFilename, '.') = 0;
	}
	if (strlen(pszFilename) + strlen(pszExtension) + 1 >= MAX_PATH)
	{
		return true;
	}
	strcat(pszFilename, pszExtension);

	return false;
}



void CompileError(DWORD ErrorNo, char *pszText)
{
	Errors++;

	cerr << ((CInputFile *)InputFiles.GetCurrentItem())->GetFilename() << "(" << itoa(((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine(), NumBuffer, 10);
	cerr << ") : error C" << itoa(ErrorNo, NumBuffer, 16) << ": ";
	if (pszText)
	{
		cerr << "\'" << pszText << "\' : ";
	}

	switch (ErrorNo)
	{
	case COMPILE_ERROR_SYNTAXERROR:
		cerr << "syntax error";
		break;

	case COMPILE_ERROR_IMPROPEROPERANDTYPE:
		cerr << "improper operand type";
		break;

	case COMPILE_ERROR_INVALIDNUMBEROFOPERANDS:
		cerr << "illegal number of operands";
		break;
		//unexpected end of file found
		//unexpected directive

	case COMPILE_ERROR_BADNUMBER:
		cerr << "bad number";
		break;

	case COMPILE_ERROR_UNMATCHEDBRACKET:
		cerr << "unmatched bracket";
		break;

	case COMPILE_ERROR_IDENTIFIERREDEFINED:
		cerr << "identifier redefined";
		break;

	case COMPILE_ERROR_CODEOUTSIDESECTION:
		cerr << "code outside section";
		GenerateCode = false;
		break;

	case COMPILE_ERROR_DIFFERENTLINKAGE:
		cerr << "redefinition; different linkage";
		break;
	}

	cerr << endl;
}



void FatalError(DWORD ErrorNo, char *pszText)
{
	bFatalError = true;

	switch (ErrorNo)
	{
	case FATAL_ERROR_OUTOFMEMORY:
		cerr << "out of memory";
		break;

	case FATAL_ERROR_OPENFILE:
		cerr << "error opening \'" << pszText << "\'" << endl;
		break;

	case FATAL_ERROR_READFILE:
		cerr << "error reading \'" << pszText << "\'" << endl;
		break;

	case FATAL_ERROR_WRITEFILE:
		cerr << "error writing to \'" << pszText << "\'" << endl;
		break;
	}
}



void Terminate(DWORD ErrorNo)
{
	cerr << "unable to recover from previous error(s); stopping compilation";
	bTerminate = true;
}



BOOL ProcessOperand(DWORD Flags, CString &Operand, DWORD *Data, CSolver &Expression)
{
	CString		Temp;


	if (Temp = Operand)
	{
		return true;
	}
	Temp.ToLower();
	*Data = 0;

	if ((Flags & ARG_REG) == ARG_REG_A)
	{
		if (Temp == 'a')
		{
			return false;
		}
	}
	if (Flags & ARG_ADDRESS)
	{
		if ((Flags & ARG_REG) == ARG_REG_C)
		{
			if (Temp == "[c]")
			{
				return false;
			}
		}
		if (Flags & ARG_BITS16)
		{
			if ((Flags & ARG_REG) == ARG_REG_BC)
			{
				if (Temp == "[bc]")
				{
					return false;
				}
			}
			if ((Flags & ARG_REG) == ARG_REG_DE)
			{
				if (Temp == "[de]")
				{
					return false;
				}
			}
			if ((Flags & ARG_REG) == ARG_REG_HL)
			{
				if (Temp == "[hl]")
				{
					return false;
				}
			}
			if ((Flags & ARG_REG) == ARG_REG_SP)
			{
				if (Temp == "[sp]")
				{
					return false;
				}
			}
		}
		if (Operand[0] != '[')
		{
			*Data = -1;
			return false;
		}
		if (Flags & ARG_DATA)
		{
			if (Expression = Operand)
			{
				return true;
			}
			return false;
		}
		*Data = -1;
		return false;
	}
	if ((Flags & ARG_REG) == ARG_REG8)
	{
		if (Temp.GetLength() == 1)
		{
			switch (Temp[0])
			{
			case 'a':
				*Data = 7;
				return false;
			case 'b':
				*Data = 0;
				return false;
			case 'c':
				*Data = 1;
				return false;
			case 'd':
				*Data = 2;
				return false;
			case 'e':
				*Data = 3;
				return false;
			case 'h':
				*Data = 4;
				return false;
			case 'l':
				*Data = 5;
				return false;
			}
		}
		else
		{
			if (Temp == "[hl]")
			{
				*Data = 6;
				return false;
			}
		}
		*Data = -1;
		return false;
	}
	if (Flags & ARG_BITS16)
	{
		if ((Flags & ARG_REG) == ARG_REG_BC)
		{
			if (Temp == "bc")
			{
				return false;
			}
		}
		if ((Flags & ARG_REG) == ARG_REG_DE)
		{
			if (Temp == "de")
			{
				return false;
			}
		}
		if ((Flags & ARG_REG) == ARG_REG_HL)
		{
			if (Temp == "hl")
			{
				return false;
			}
		}
		if ((Flags & ARG_REG) == ARG_REG_SP)
		{
			if (Temp == "sp")
			{
				return false;
			}
		}
	}

	if (Flags & ARG_COND)
	{
		if (Temp == "nz")
		{
			*Data = 0;
			return false;
		}
		if (Temp == 'z')
		{
			*Data = 8;
			return false;
		}
		if (Temp == "nc")
		{
			*Data = 16;
			return false;
		}
		if (Temp == 'c')
		{
			*Data = 24;
			return false;
		}
	}

	if (Flags & ARG_DATA)
	{
		if (Expression = Operand)
		{
			return true;
		}
		return false;
	}

	*Data = -1;
	return false;
}



void Assemble(CString &String)
{
	IDENTIFIER	Identifier;
	DWORD		ItemNo;
	CString		Arg1, Arg2;
	DWORD		Data1, Data2;
	BYTE		nOperands;
	DWORD		MachineCode;
	BOOL		MnemonicFound;
	POINTER		Pointer;
	CSolver		Expression1, Expression2;


	Identifiers.GetIdentifier(String, &Identifier);
	if (Identifier.Flags & IF_MNEMONIC)
	{
		nOperands = MNEMONIC0;
		if (String.UserData != '\n')
		{
			if (((CInputFile *)InputFiles.GetCurrentItem())->Read(Arg1, false))
			{
				return;
			}
			if (Arg1[0] == '[' && Arg1[Arg1.GetLength() - 1] != ']')
			{
				CompileError(COMPILE_ERROR_UNMATCHEDBRACKET, NULL);
				((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
				return;
			}
			switch (Arg1.UserData)
			{
			case ':':
				CompileError(COMPILE_ERROR_SYNTAXERROR, ":");
				((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
				return;

			case '\n':
				if (Arg1.GetLength())
				{
					nOperands = MNEMONIC1;
				}
				break;

			case ',':
				nOperands = MNEMONIC2;
				if (((CInputFile *)InputFiles.GetCurrentItem())->Read(Arg2, false))
				{
					return;
				}
				if (Arg2[0] == '[' && Arg2[Arg2.GetLength() - 1] != ']')
				{
					CompileError(COMPILE_ERROR_UNMATCHEDBRACKET, NULL);
					((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
					return;
				}
				switch (Arg2.UserData)
				{
				case ':':
					CompileError(COMPILE_ERROR_SYNTAXERROR, ":");
					((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
					return;

				case ',':
					CompileError(COMPILE_ERROR_INVALIDNUMBEROFOPERANDS, NULL);
					((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
					return;

				case ' ':
					CompileError(COMPILE_ERROR_SYNTAXERROR, NULL);
					((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
					return;

				case 0:
					return;
				}
				break;

			case ' ':
				CompileError(COMPILE_ERROR_SYNTAXERROR, NULL);
				((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
				return;

			case 0:
				return;
			}
		}

		MnemonicFound = false;
		ItemNo = MAX_OPCODES;
		while (ItemNo--)
		{
			if ((OpCodeList[ItemNo].OpCode == Identifier.Mnemonic->OpCode) && ((OpCodeList[ItemNo].Flags & Identifier.Mnemonic->Flags & MNEMONICARGS) == nOperands))
			{
				switch (nOperands)
				{
				case MNEMONIC0:
					MnemonicFound = true;
					if (GenerateCode)
					{
						if (OpCodeList[ItemNo].Code & 0xFF00)
						{
							Code.InsertData(OpCodeList[ItemNo].Code >> 8, SF_CODE);
							Code.InsertData((BYTE)OpCodeList[ItemNo].Code, SF_CONST);
						}
						else
						{
							Code.InsertData((BYTE)OpCodeList[ItemNo].Code, SF_CODE);
						}
					}
					cout << "Cmd: \'" << Identifier.Mnemonic->Name << "\' Code: 0x" << itoa(OpCodeList[ItemNo].Code, NumBuffer, 16);
					cout << " Line: "<< itoa(((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine(), NumBuffer, 10) << endl;
					return;

				case MNEMONIC1:
					MnemonicFound = true;
					if (ProcessOperand(OpCodeList[ItemNo].Flags & 0xFFFF & ~MNEMONICARGS, Arg1, &Data1, Expression1))
					{
						return;
					}
					if (Data1 == -1)
					{
						break;
					}
					MachineCode = OpCodeList[ItemNo].Code;
					if ((OpCodeList[ItemNo].Flags & ARG_REG) == ARG_REG8)
					{
						MachineCode += Data1;
					}
					if (OpCodeList[ItemNo].Flags & ARG_RST)
					{
						if (Data1 & 0x7 || Data1 > 0x38)
						{
							break;
						}
						MachineCode += Data1;
					}
					if (OpCodeList[ItemNo].Flags & ARG_COND)
					{
						MachineCode += Data1;
					}
					cout << "Cmd: \'" << Identifier.Mnemonic->Name << "\' Code: 0x" << itoa(MachineCode, NumBuffer, 16);
					cout << " Line: "<< itoa(((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine(), NumBuffer, 10) << endl;
					return;

				case MNEMONIC2:
					MnemonicFound = true;
					if (ProcessOperand(OpCodeList[ItemNo].Flags & 0xFFFF & ~MNEMONICARGS, Arg1, &Data1, Expression1))
					{
						return;
					}
					if (Data1 == -1)
					{
						break;
					}
					if (ProcessOperand((OpCodeList[ItemNo].Flags >> 16) & 0xFFFF & ~MNEMONICARGS, Arg2, &Data2, Expression2))
					{
						return;
					}
					if (Data2 == -1)
					{
						break;
					}
					MachineCode = OpCodeList[ItemNo].Code;
					if ((OpCodeList[ItemNo].Flags & ARG1(ARG_REG)) == ARG1(ARG_REG8))
					{
						MachineCode += Data2;
					}
					if ((OpCodeList[ItemNo].Flags & ARG_REG) == ARG_REG8)
					{
						MachineCode += 8 * Data1;
					}

					//ld [hl], [hl] (halt)
					if (MachineCode == 0x76)
					{
						break;
					}

					if (OpCodeList[ItemNo].Flags & ARG_BIT)
					{
						if (Data1 > 7)
						{
							break;
						}
						MachineCode += 8 * Data1;
					}
					if (OpCodeList[ItemNo].Flags & ARG_COND)
					{
						MachineCode += Data1;
					}

					if (GenerateCode)
					{
						if (MachineCode & 0xFF00)
						{
							Code.InsertData((BYTE)(MachineCode >> 8), SF_CODE);
							Code.InsertData((BYTE)MachineCode, SF_CONST);
						}
						else
						{
							Code.InsertData((BYTE)MachineCode, SF_CODE);
							if (OpCodeList[ItemNo].Flags & ARG1(ARG_DATA))
							{
								Code.InsertData(Expression2, SF_CONST, OpCodeList[ItemNo].Flags & ARG1(ARG_BITS16) ? 2 : 1);
							}
						}
					}
					cout << "Cmd: \'" << Identifier.Mnemonic->Name << "\' Code: 0x" << itoa(MachineCode, NumBuffer, 16);
					cout << " Line: "<< itoa(((CInputFile *)InputFiles.GetCurrentItem())->GetCurrentLine(), NumBuffer, 10) << endl;
					return;
				}
			}
		}
		if (MnemonicFound)
		{
			CompileError(COMPILE_ERROR_IMPROPEROPERANDTYPE, NULL);
		}
		else
		{
			CompileError(COMPILE_ERROR_INVALIDNUMBEROFOPERANDS, NULL);
		}
		return;
	}

	String.ToLower();
	if (String == "code")
	{
		if (!(Pointer.pOffset = new CSolver()))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return;
		}
		*Pointer.pOffset = (DWORD)0;
		Pointer.BankNumber = 0;
		if (String.UserData != '\n')
		{
			((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
		}
		Code.SetOffset(SF_CODE, &Pointer);
		delete Pointer.pOffset;

		return;
	}

	if (String.UserData != '\n')
	{
		((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
	}
	CompileError(COMPILE_ERROR_SYNTAXERROR, NULL);
	((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
}



struct OBJHEADER
{
	DWORD		ID;
	DWORD		Version;
	DWORD		nFiles;
} ObjHeader = {0x42474C47, 0
#ifdef _DEBUG
	| 0x80000000 //Only debug versions of other programs should be used while in development.
#endif //_DEBUG
};



BOOL SaveObj(char *pszFilename)
{
	HANDLE		hFile;
	DWORD		BytesWritten;
	CInputFile	*pInputFile;
	DWORD		ItemNo;
	FILETIME	FileTime;


	if (strchr(pszFilename, '\\'))
	{
		*strrchr(pszFilename, '\\') = '\0';
		if (!CreateDirectory(pszFilename, NULL))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				cerr << "Couldn't create directory." << endl;
				return true;
			}
		}
		*(pszFilename + strlen(pszFilename)) = '\\';
	}
	if ((hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		cerr << "Couldn't create \'" << pszFilename << "\'" << endl;
		return true;
	}

	ObjHeader.nFiles = InputFiles.GetMaxItemNo();

	if (!WriteFile(hFile, &ObjHeader, sizeof(ObjHeader), &BytesWritten, NULL))
	{
		CloseHandle(hFile);
		FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
		return true;
	}
	if (BytesWritten != sizeof(ObjHeader))
	{
		CloseHandle(hFile);
		FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
		return true;
	}


	ItemNo = 1;
	while (pInputFile = (CInputFile *)InputFiles.GetItem(ItemNo))
	{
		if (!WriteFile(hFile, pInputFile->GetFilename(), strlen(pInputFile->GetFilename()) + 1, &BytesWritten, NULL))
		{
			CloseHandle(hFile);
			FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
			return true;
		}
		if (BytesWritten != strlen(pInputFile->GetFilename()) + 1)
		{
			CloseHandle(hFile);
			FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
			return true;
		}
		pInputFile->GetLastWriteFileTime(&FileTime);
		if (!WriteFile(hFile, &FileTime, sizeof(FileTime), &BytesWritten, NULL))
		{
			CloseHandle(hFile);
			FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
			return true;
		}
		if (BytesWritten != sizeof(FileTime))
		{
			CloseHandle(hFile);
			FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
			return true;
		}
		ItemNo++;
	}
	if (Identifiers.SaveObj(hFile))
	{
		CloseHandle(hFile);
		FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
		return true;
	}
	if (Code.SaveObj(hFile))
	{
		CloseHandle(hFile);
		FatalError(FATAL_ERROR_WRITEFILE, pszFilename);
		return true;
	}


	CloseHandle(hFile);

	return false;
}



int main(int argc, char *argv[], char *envp[])
{
	CInputFile	*InputFile;
	IDENTIFIER	Identifier;
	char		ObjFilename[MAX_PATH];
	CString		String;


	cout << "Glasm v1.0 Preview 1" << endl << endl;

	if (argc < 2)
	{
		cout << "Syntax:" << endl;
		cout << "Glasm input [output]" << endl;
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}


	if (!(InputFile = (CInputFile *)InputFiles.NewItem(sizeof(CInputFile))))
	{
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}
	if (InputFile->Open(argv[1]))
	{
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}

	if (argc >= 3)
	{
		if (strlen(argv[2]) >= MAX_PATH)
		{
			cout << "Path too long." << endl;
#ifdef WAIT
			getch();
#endif //WAIT
			return 1;
		}
		strcpy(ObjFilename, argv[2]);
	}
	else
	{
		strcpy(ObjFilename, argv[1]);
		if (ChangeExtension(ObjFilename, ".obj"))
		{
			cout << "Path too long." << endl;
#ifdef WAIT
			getch();
#endif //WAIT
			return 1;
		}
	}


	ZeroMemory(&Identifier, sizeof(Identifier));

	while (!bTerminate && !bFatalError && Errors < 100)
	{
		if (((CInputFile *)InputFiles.GetCurrentItem())->Read(String, true))
		{
			break;
		}
		switch (String.UserData)
		{
		case ':':
			if (Code.GetOffset(&Identifier.Pointer))
			{
#ifdef WAIT
				getch();
#endif //WAIT
				return 1;
			}
			if (Identifier.Pointer.pOffset)
			{
				if (Identifiers.AddIdentifier(String, IF_LABEL, &Identifier.Pointer))
				{
#ifdef WAIT
					getch();
#endif //WAIT
					return 1;
				}
			}
			break;

		case ' ':
		case '\n':
			if (String.GetLength())
			{
				Assemble(String);
			}
			break;

		case ',':
			CompileError(COMPILE_ERROR_SYNTAXERROR, ",");
			((CInputFile *)InputFiles.GetCurrentItem())->ReadUntilLineBreak();
			break;
		}
	}


	if (bFatalError)
	{
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}

	if (Errors == 0)
	{
		if (SaveObj(ObjFilename))
		{
#ifdef WAIT
			getch();
#endif //WAIT
			return 1;
		}
	}

	cout << endl;
	cout << itoa(Warnings, NumBuffer, 10) << " warning(s)" << endl;
	cout << itoa(Errors, NumBuffer, 10) << " error(s)" << endl;


#ifdef WAIT
	getch();
#endif //WAIT
	return 0;
}

