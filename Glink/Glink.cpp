#include	<windows.h>
#include	<iostream.h>
#include	<conio.h>

#define		GLINK_CPP

#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"..\CString\CString.h"
#include	"..\CSolver\CSolver.h"
#include	"Glink.h"



#ifdef _DEBUG

#define		WAIT

#endif //_DEBUG



#define		ReadFromFile(hFile, pBuffer, BytesToRead)				\
	if (!ReadFile(hFile, pBuffer, BytesToRead, &BytesRead, NULL))	\
	{																\
		CloseHandle(hFile);											\
		if (pFileNumbers)											\
		{															\
			delete pFileNumbers;									\
		}															\
		FatalError(FATAL_ERROR_READFILE, pszFilename);				\
		return true;												\
	}																\
	if (BytesRead != BytesToRead)									\
	{																\
		CloseHandle(hFile);											\
		if (pFileNumbers)											\
		{															\
			delete pFileNumbers;									\
		}															\
		FatalError(FATAL_ERROR_READFILE, pszFilename);				\
		return true;												\
	}



CList			Files(NULL, NULL);
CList			Sections(NULL, NULL);

char			NumBuffer[10];
BOOL			bFatalError = false;
DWORD			Errors = 0;



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



void LinkError(DWORD ErrorNo, char *pszFilename, char *pszText)
{
	Errors++;

	cerr << pszFilename << " : error LNK" << itoa(ErrorNo, NumBuffer, 16) << ": ";

	switch (ErrorNo)
	{
	case LINK_ERROR_UNSUPPORTEDFORMAT:
		cerr << "file format unsupported" << endl;
		break;

	case LINK_ERROR_UNSUPPORTEDVERSION:
		cerr << "file format version unsupported" << endl;
		break;
	}
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



BOOL LoadObjectFile(char *pszFilename, BOOL DebugInfo)
{
	HANDLE		hFile;
	DWORD		BytesRead;
	DWORD		dw, Data, Pos, dwItemNo;
	void		*pv;
	BOOL		AlreadyExists;
	WORD		Size;
	char		Buffer[MAX_PATH + sizeof(FILETIME)];
	DWORD		*pFileNumbers = NULL;
	SECTION		*pSection;
	DWORD		File, Line;


	if ((hFile = CreateFile(pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		FatalError(FATAL_ERROR_OPENFILE, pszFilename);
		return true;
	}

	ReadFromFile(hFile, &Data, 4);
	if (Data != 0x42474C47)
	{
		CloseHandle(hFile);
		hFile = NULL;
		LinkError(LINK_ERROR_UNSUPPORTEDFORMAT, pszFilename, NULL);
		return true;
	}

	ReadFromFile(hFile, &Data, 4);
#ifdef _DEBUG
	Data &= ~0x80000000;
#endif //_DEBUG
	if (Data != 0)
	{
		CloseHandle(hFile);
		hFile = NULL;
		LinkError(LINK_ERROR_UNSUPPORTEDVERSION, pszFilename, NULL);
		return true;
	}

	ReadFromFile(hFile, &File, 4);
	pFileNumbers = new DWORD[File + 1];
	pFileNumbers[0] = 0;
	for (dw = 1; dw <= File; dw++)
	{
		Pos = 0;
		do
		{
			ReadFromFile(hFile, &Buffer[Pos], 1);
		}
		while (Buffer[Pos++] != 0);

		ReadFromFile(hFile, &Buffer[strlen(Buffer) + 1], sizeof(FILETIME));

		AlreadyExists = false;
		dwItemNo = Files.GetMaxItemNo();
		while (pv = Files.GetItem(dwItemNo))
		{
			if (!stricmp(Buffer, (char *)pv))
			{
				AlreadyExists = true;
				pFileNumbers[dw] = dwItemNo;
				break;
			}
			--dwItemNo;
		}

		if (!AlreadyExists)
		{
			if (!Files.NewItem(Pos + sizeof(FILETIME), Buffer))
			{
				CloseHandle(hFile);
				if (pFileNumbers)
				{
					delete pFileNumbers;
				}
				return true;
			}
			pFileNumbers[dw] = Files.GetMaxItemNo();
		}
	}

	ReadFromFile(hFile, &Data, 4);
	if (Data)
	{
		CloseHandle(hFile);
		cerr << "Symbols not supported." << endl;
		bFatalError = true;
		if (pFileNumbers)
		{
			delete pFileNumbers;
		}
		return true;
	}

	while (hFile)
	{
		if (!ReadFile(hFile, &File, 4, &BytesRead, NULL))
		{
			break;
		}
		if (BytesRead == 0)
		{
			break;
		}
		if (BytesRead < 4)
		{
			FatalError(FATAL_ERROR_READFILE, pszFilename);
			break;
		}
		File = pFileNumbers[File];

		cout << "File: " << File << endl;
		ReadFromFile(hFile, &Line, 4);
		cout << "Line: " << Line << endl;

		//Address

		while (hFile)
		{
			if (!ReadFile(hFile, &Data, 4, &BytesRead, NULL))
			{
				CloseHandle(hFile);
				hFile = NULL;
				break;
			}
			if (BytesRead == 0)
			{
				CloseHandle(hFile);
				hFile = NULL;
				break;
			}
			if (BytesRead < 4)
			{
				FatalError(FATAL_ERROR_READFILE, pszFilename);
				CloseHandle(hFile);
				hFile = NULL;
				break;
			}
			if (Data == 0)
			{
				break;
			}

			while (hFile)
			{
				if (!ReadFile(hFile, &Size, 2, &BytesRead, NULL))
				{
					CloseHandle(hFile);
					hFile = NULL;
					break;
				}
				if (BytesRead == 0)
				{
					CloseHandle(hFile);
					hFile = NULL;
					break;
				}

				if (!(pSection = (SECTION *)Sections.NewItem(sizeof(SECTION))))
				{
					CloseHandle(hFile);
					if (pFileNumbers)
					{
						delete pFileNumbers;
					}
					return true;
				}
				cout << "  Size: " << Size << endl;
				if (Size == 0)
				{
					break;
				}
				pSection->File = File;
				pSection->LineFrom = Line;
				Line += Data;
				pSection->Lines = Data;
				cout << " Lines: " << Data << endl;
				pSection->Size = Size;
				ReadFromFile(hFile, &Data, 1);
				pSection->Flags = Data;
				if (Data == 0)
				{
					cout << "  Type: unknown" << endl;
				}
				else
				{
					if ((Data & ~0x80) == 2)
					{
						cout << "  Type: code";
					}
					else
					{
						cout << "  Type: const";
					}
					if (Data & 0x80)
					{
						cout << ", expression" << endl;
					}
					else
					{
						cout << endl;
						if (Size <= 4)
						{
							ReadFromFile(hFile, &pSection->Bytes, Size);
							if (Size == 1)
							{
								cout << "  Code: " << ultoa(pSection->Bytes & 0xFF, NumBuffer, 16) << endl;
							}
						}
						else
						{
							if (!(pSection->pBytes = new BYTE[Size]))
							{
								FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
								CloseHandle(hFile);
								hFile = NULL;
								break;
							}
						}
					}
				}
			}
		}
	}

	if (pFileNumbers)
	{
		delete pFileNumbers;
	}
	if (hFile)
	{
		CloseHandle(hFile);
	}

	return false;
}



struct DBGHEADER
{
	DWORD		ID;
	DWORD		Version;
	DWORD		nFiles;
} DbgHeader = {0x42474C47, 0
#ifdef _DEBUG
	| 0x80000000 //Only debug versions of other programs should be used while in development.
#endif //_DEBUG
};



BOOL SaveOutput(char *pszOutFilename, BOOL WriteDebug, char *pszDbgFilename)
{
	HANDLE		hOutFile, hDbgFile = NULL;
	char		szDbgFilename[MAX_PATH];
	DWORD		BytesWritten, ItemNo;


	if (strchr(pszOutFilename, '\\'))
	{
		*strrchr(pszOutFilename, '\\') = '\0';
		if (!CreateDirectory(pszOutFilename, NULL))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				cerr << "Couldn't create directory." << endl;
				return true;
			}
		}
		*(pszOutFilename + strlen(pszOutFilename)) = '\\';
	}
	if ((hOutFile = CreateFile(pszOutFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		cerr << "Couldn't create \'" << pszOutFilename << "\'" << endl;
		return true;
	}

	if (WriteDebug)
	{
		if (pszDbgFilename)
		{
			strcpy(szDbgFilename, pszDbgFilename);
		}
		else
		{
			strcpy(szDbgFilename, pszOutFilename);
			ChangeExtension(szDbgFilename, ".dbg");
		}
		if ((hDbgFile = CreateFile(szDbgFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hOutFile);
			cerr << "Couldn't create \'" << szDbgFilename << "\'" << endl;
			return true;
		}

		DbgHeader.nFiles = Files.GetMaxItemNo();

		if (!WriteFile(hDbgFile, &DbgHeader, sizeof(DbgHeader), &BytesWritten, NULL))
		{
			CloseHandle(hOutFile);
			CloseHandle(hDbgFile);
			FatalError(FATAL_ERROR_WRITEFILE, szDbgFilename);
			return true;
		}
		if (BytesWritten != sizeof(DbgHeader))
		{
			CloseHandle(hOutFile);
			CloseHandle(hDbgFile);
			FatalError(FATAL_ERROR_WRITEFILE, szDbgFilename);
			return true;
		}

		ItemNo = 1;
		while (ItemNo <= Files.GetMaxItemNo())
		{
			if (!WriteFile(hDbgFile, Files.GetItem(ItemNo), strlen((char *)Files.GetItem(ItemNo)) + 1 + sizeof(FILETIME), &BytesWritten, NULL))
			{
				CloseHandle(hOutFile);
				CloseHandle(hDbgFile);
				FatalError(FATAL_ERROR_WRITEFILE, szDbgFilename);
				return true;
			}
			if (BytesWritten != strlen((char *)Files.GetItem(ItemNo)) + 1 + sizeof(FILETIME))
			{
				CloseHandle(hOutFile);
				CloseHandle(hDbgFile);
				FatalError(FATAL_ERROR_WRITEFILE, szDbgFilename);
				return true;
			}
			ItemNo++;
		}
	}

	CloseHandle(hOutFile);
	if (hDbgFile)
	{
		CloseHandle(hDbgFile);
	}

	return false;
}



int main(int argc, char *argv[])
{
	int			CurrentArg;
	int			OutputArgNo = 0, DebugArgNo = 0;


	cout << "Glink v1.0 Preview" << endl << endl;

	///out:"C:\Torbjrn\CPP\Game Lad\Glasm\Sample\Sample.gb" /debug "C:\Torbjrn\CPP\Game Lad\Glasm\Sample\Sample.obj"
	if (argc < 3)
	{
		cout << "Usage: GLINK [options] files" << endl;
		cout << "Options:" << endl;
		cout << "	/DEBUG[:filename]	Write debugging info" << endl;
		//cout << "	/MAP[:filename]		Write mapfile" << endl;
		cout << "	/OUT:filename		Output" << endl;
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}


	CurrentArg = 1;
	while (CurrentArg < argc)
	{
		if (argv[CurrentArg][0] == '-' || argv[CurrentArg][0] == '/')
		{
			switch (tolower(argv[CurrentArg][1]))
			{
			case 'd':
				if (!strnicmp(&argv[CurrentArg][1], "debug", 5) && !DebugArgNo)
				{
					if (argv[CurrentArg][6] == '\0' || argv[CurrentArg][6] == ':')
					{
						DebugArgNo = CurrentArg;
						break;
					}
				}
			case 'o':
				if (!strnicmp(&argv[CurrentArg][1], "out:", 4) && !OutputArgNo)
				{
					OutputArgNo = CurrentArg;
					break;
				}
			default:
				cerr << "Invalid argument \'" << argv[CurrentArg] << "\'\n";
#ifdef WAIT
				getch();
#endif //WAIT
				return 1;
			}
		}
		CurrentArg++;
	}

	CurrentArg = 1;
	while (CurrentArg < argc)
	{
		if (argv[CurrentArg][0] != '-' && argv[CurrentArg][0] != '/')
		{
			if (LoadObjectFile(argv[CurrentArg], DebugArgNo ? true : false))
			{
				break;
			}
			if (bFatalError)
			{
				break;
			}
		}
		CurrentArg++;
	}


	if (bFatalError)
	{
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}


	if (OutputArgNo)
	{
		if (DebugArgNo)
		{
			SaveOutput(&argv[OutputArgNo][5], true, strlen(argv[DebugArgNo]) > 6 ? &argv[DebugArgNo][7] : NULL);
		}
		else
		{
			SaveOutput(&argv[OutputArgNo][5], false, NULL);
		}
	}

	cout << endl;
	cout << itoa(Errors, NumBuffer, 10) << " error(s)" << endl;

#ifdef WAIT
	getch();
#endif //WAIT
	return 0;
}

