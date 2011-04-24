#include	<windows.h>
#include	<iostream.h>
#include	<conio.h>

#define		GLINK_CPP

#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"Glink.h"



#ifdef _DEBUG

#define		WAIT

#endif //_DEBUG



#define		ReadFromFile(hFile, pBuffer, BytesToRead)				\
	if (!ReadFile(hFile, pBuffer, BytesToRead, &BytesRead, NULL))	\
	{																\
		CloseHandle(hFile);											\
		hFile = NULL;												\
		FatalError(FATAL_ERROR_READFILE, argv[CurrentArg]);			\
		break;														\
	}																\
	if (BytesRead != BytesToRead)									\
	{																\
		CloseHandle(hFile);											\
		hFile = NULL;												\
		FatalError(FATAL_ERROR_READFILE, argv[CurrentArg]);			\
		break;														\
	}



CList			Files(NULL, NULL);

char			NumBuffer[10];
BOOL			bFatalError = false;
DWORD			Errors = 0;
DWORD			Warnings = 0;



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



int main(int argc, char *argv[])
{
	int			CurrentArg;
	HANDLE		hFile = NULL;
	DWORD		BytesRead;
	DWORD		Data, Pos, dwItemNo;
	void		*pv;
	BOOL		AlreadyExists;
	WORD		Size;
	char		Buffer[MAX_PATH];


	cout << "Glink v1.0" << endl << endl;

	if (argc < 2)
	{
		cout << "Syntax:" << endl;
		cout << "Glink input" << endl;
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}


	CurrentArg = 1;

	while (!bFatalError && CurrentArg < argc)
	{
		if ((hFile = CreateFile(argv[CurrentArg], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
		{
			FatalError(FATAL_ERROR_OPENFILE, argv[CurrentArg]);
			break;
		}

		ReadFromFile(hFile, &Data, 4);
		if (Data != 0x42474C47)
		{
			CloseHandle(hFile);
			hFile = NULL;
			LinkError(LINK_ERROR_UNSUPPORTEDFORMAT, argv[CurrentArg], NULL);
			break;
		}

		ReadFromFile(hFile, &Data, 4);
#ifdef _DEBUG
		Data &= ~0x80000000;
#endif //_DEBUG
		if (Data != 0)
		{
			CloseHandle(hFile);
			hFile = NULL;
			LinkError(LINK_ERROR_UNSUPPORTEDVERSION, argv[CurrentArg], NULL);
			break;
		}

		ReadFromFile(hFile, &Data, 4);
		while (Data--)
		{
			Pos = 0;
			do
			{
				ReadFromFile(hFile, &Buffer[Pos], 1);
			}
			while (Buffer[Pos++] != 0);

			if (!hFile)
			{
				break;
			}

			AlreadyExists = false;
			dwItemNo = Files.GetMaxItemNo();
			while (pv = Files.GetItem(dwItemNo))
			{
				if (!stricmp(Buffer, (char *)pv))
				{
					AlreadyExists = true;
					break;
				}
				--dwItemNo;
			}

			if (!AlreadyExists)
			{
				if (!Files.NewItem(Pos, Buffer))
				{
					CloseHandle(hFile);
					hFile = NULL;
					break;
				}
			}

			//Skip filetime
			ReadFromFile(hFile, &Buffer, 8);
		}

		if (!hFile)
		{
			break;
		}

		ReadFromFile(hFile, &Data, 4);
		if (Data)
		{
			cerr << "Symbols not supported." << endl;
			bFatalError = true;
			break;
		}

		while (hFile)
		{
			if (!ReadFile(hFile, &Data, 4, &BytesRead, NULL))
			{
				break;
			}
			if (BytesRead == 0)
			{
				break;
			}
			if (BytesRead < 4)
			{
				FatalError(FATAL_ERROR_READFILE, argv[CurrentArg]);
				break;
			}

			while (hFile)
			{
				cout << "File: " << Data << endl;
				ReadFromFile(hFile, &Data, 4);
				cout << "Line: " << Data << endl;

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
						FatalError(FATAL_ERROR_READFILE, argv[CurrentArg]);
						CloseHandle(hFile);
						hFile = NULL;
						break;
					}
					if (Data == 0)
					{
						break;
					}
					cout << Data << " lines" << endl;

					while (hFile)
					{
						ReadFromFile(hFile, &Size, 2);
						if (Size == 0)
						{
							break;
						}
						Data = 0;
						ReadFromFile(hFile, &Data, 1);
						if (Data == 0)
						{
							cout << "unknown" << endl;
						}
						else
						{
							if ((Data & ~0x80) == 2)
							{
								cout << "code";
							}
							else
							{
								cout << "const";
							}
							if (Data & 0x80)
							{
								cout << ", expression" << endl;
							}
							else
							{
								cout << endl;
								ReadFromFile(hFile, &Buffer, Size);
								if (Size == 1)
								{
									cout << ultoa(((DWORD)Buffer[0]) & 0xFF, NumBuffer, 16) << endl;
								}
							}
						}
					}
				}
			}
		}

		if (hFile)
		{
			CloseHandle(hFile);
			hFile = NULL;
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

	/*if ((hFile = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		//FatalError(FATAL_ERROR_OPENFILE, pszFilename);
		cerr << "Error opening \'" << argv[1] << "\'" << endl;
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}



	ReadFromFile(hFile, &Data, 4);
	cout << Data << " file(s)" << endl;
	while (Data--)
	{
		Pos = 0;
		do
		{
			ReadFromFile(hFile, &Buffer[Pos], 1);
		}
		while (Buffer[Pos++] != 0);

		cout << "File " << Data + 1 << ": " << Buffer << endl;
		ReadFromFile(hFile, &FileTime, 8);

		if ((hSourceFile = CreateFile(Buffer, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			cout << "Source file has been removed." << endl;
		}
		else
		{
			GetFileTime(hSourceFile, NULL, NULL, &FileTime2);
			CloseHandle(hSourceFile);
			if (CompareFileTime(&FileTime, &FileTime2) == 0)
			{
				cout << "File has not been modified." << endl;
			}
			else
			{
				cout << "File has been modified." << endl;
			}
		}
	}

	ReadFromFile(hFile, &Data, 4);
	if (Data == 0)
	{
		cout << "No symbols" << endl;
	}
	else
	{
		cout << "Number of symbols: " << Data << endl;
		cout << "Symbols not supported." << endl;
		CloseHandle(hFile);
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}

	/*ReadFromFile(hFile, &Data, 4);
	if (Data == 0)
	{
		cout << "No local symbols" << endl;
	}
	else
	{
		cout << "Number of local symbols: " << Data << endl;
		cout << "Local symbols not supported." << endl;
		CloseHandle(hFile);
#ifdef WAIT
		getch();
#endif //WAIT
		return 1;
	}*/

	//CloseHandle(hFile);



	cout << endl;
	cout << itoa(Warnings, NumBuffer, 10) << " warning(s)" << endl;
	cout << itoa(Errors, NumBuffer, 10) << " error(s)" << endl;

#ifdef WAIT
	getch();
#endif //WAIT
	return 0;
}

