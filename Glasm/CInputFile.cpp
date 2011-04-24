#include	<windows.h>

#define		CINPUTFILE_CPP

#include	"..\CString\CString.h"
#include	"..\CList\CList.h"
#include	"..\Error.h"
#include	"..\CSolver\CSolver.h"
#include	"Glasm.h"
#include	"CInputFile.h"



BOOL CALLBACK CreateCInputFile(void **p, DWORD dw)
{
	if (!(*p = new CInputFile()))
	{
		FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
		return true;
	}

	return false;
}



void CALLBACK DeleteCInputFile(void *p, DWORD dw)
{
	delete (CInputFile *)p;
}



CList		InputFiles(CreateCInputFile, DeleteCInputFile);



CInputFile::CInputFile()
{
	hFile = NULL;
	BufferStart = 0;
	BufferLength = 0;
	Eof = false;
	CurrentLine = 0;
	LastLine = 0;
}



CInputFile::~CInputFile()
{
	if (hFile)
	{
		CloseHandle(hFile);
	}
}



BOOL CInputFile::Open(char *pszFilename)
{
	if ((hFile = CreateFile(pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
		FatalError(FATAL_ERROR_OPENFILE, pszFilename);
		return true;
	}

	GetFileTime(hFile, NULL, NULL, &FileTime);

	strcpy(Filename, pszFilename);
	CurrentLine = 1;
	LastLine = 0;

	return false;
}



BOOL CInputFile::FillBuffer()
{
	DWORD	nBytes;


	if (Eof)
	{
		return true;
	}


	MoveMemory(Buffer, &Buffer[BufferStart], BufferLength);
	BufferStart = 0;
	if (!ReadFile(hFile, &Buffer[BufferLength], sizeof(Buffer) - BufferLength, &nBytes, NULL))
	{
		FatalError(FATAL_ERROR_READFILE, Filename);
		return true;
	}
	if (nBytes == 0)
	{
		Eof = true;
		CloseHandle(hFile);
		hFile = NULL;
		return BufferLength == 0;
	}
	BufferLength += nBytes;

	return false;
}



BOOL CInputFile::RemoveWhiteSpaces(BOOL RemoveLF)
{
	BOOL	Slash = false;


	if (BufferLength == 0)
	{
		if (FillBuffer())
		{
			return true;
		}
	}

	while (Buffer[BufferStart] == 9 || Buffer[BufferStart] == 10
		|| Buffer[BufferStart] == ' ' || Buffer[BufferStart] == 13
		|| Buffer[BufferStart] == '/')
	{
		if (Buffer[BufferStart] == '/')
		{
			if (BufferLength <= 1)
			{
				if (FillBuffer())
				{
					return true;
				}
			}
			if (BufferLength <= 1)
			{
				return false;
			}
			switch (Buffer[BufferStart + 1])
			{
			case '/':
				BufferStart += 2;
				BufferLength -= 2;
				if (ReadUntilLineBreak())
				{
					return true;
				}
				break;

			case '*':
				BufferStart += 2;
				BufferLength -= 2;
				if (ReadUntilEndComment())
				{
					return true;
				}

			default:
				return false;
			}
		}

		if (Buffer[BufferStart] == '\n')
		{
			if (RemoveLF)
			{
				CurrentLine++;
			}
			else
			{
				return false;
			}
		}

		BufferStart++;
		if (--BufferLength == 0)
		{
			if (FillBuffer())
			{
				return true;
			}
		}
	}

	return false;
}



BOOL CInputFile::ReadUntilLineBreak()
{
	BOOL	Slash = false;


	if (BufferLength == 0)
	{
		if (FillBuffer())
		{
			return true;
		}
	}

	while (Buffer[BufferStart] != '\n')
	{
		if (Buffer[BufferStart] == '*')
		{
			if (Slash)
			{
				if (ReadUntilEndComment())
				{
					return true;
				}
				if (Buffer[BufferStart] == '\n')
				{
					return false;
				}
			}
		}
		if (Buffer[BufferStart] == '/')
		{
			Slash = true;
		}
		else
		{
			Slash = false;
		}
		BufferStart++;
		if (--BufferLength == 0)
		{
			if (FillBuffer())
			{
				return true;
			}
		}
	}

	return false;
}



BOOL CInputFile::ReadUntilEndComment()
{
	BOOL	Asterisk = false;


	if (BufferLength == 0)
	{
		if (FillBuffer())
		{
			return true;
		}
	}

	while (true)
	{
		if (Buffer[BufferStart] == '/')
		{
			if (Asterisk)
			{
				BufferStart++;
				BufferLength--;
				return false;
			}
		}
		Asterisk = false;
		if (Buffer[BufferStart] == '*')
		{
			Asterisk = true;
		}
		if (Buffer[BufferStart] == '\n')
		{
			CurrentLine++;
		}

		BufferStart++;
		if (--BufferLength == 0)
		{
			if (FillBuffer())
			{
				return true;
			}
		}
	}
}



BOOL CInputFile::Read(CString &String, BOOL BreakAtSpace)
{
	BOOL	SkipCharacter = false, DiscardCharacter = false, Slash = false, StartBracket = false, EndBracket = false;
	char	c;


	String = (char *)NULL;
	String.UserData = 0;

	if (RemoveWhiteSpaces(true))
	{
		return true;
	}

	if (BufferLength == 0)
	{
		return false;
	}

	while (true)
	{
		if (BufferLength == 0)
		{
			if (FillBuffer())
			{
				if (Eof && String.GetLength())
				{
					String.UserData = '\n';
					return false;
				}

				return true;
			}
		}

		switch (Buffer[BufferStart])
		{
		case ';':
			if (ReadUntilLineBreak())
			{
				if (!(Eof && String.GetLength()))
				{
					return true;
				}
			}
			String.UserData = '\n';
			return false;

		case '/':
			if (Slash)
			{
				if (ReadUntilLineBreak())
				{
					if (!(Eof && String.GetLength()))
					{
						return true;
					}
				}
				String.DeleteLastCharacter();
				String.UserData = '\n';
				return false;
			}
			break;

		case '*':
			if (Slash)
			{
				if (ReadUntilEndComment())
				{
					if (!(Eof && String.GetLength()))
					{
						return true;
					}
				}
				String.DeleteLastCharacter();
				if (String.GetLength())
				{
					String.UserData = '\n';
					return false;
				}
			}
			break;

		case '\n':
		case 13:
			String.UserData = '\n';
			return false;

		case ',':
		case ':':
			String.UserData = Buffer[BufferStart++];
			BufferLength--;
			return false;

		case ' ':
		case 9:
			if (!BreakAtSpace)
			{
				break;
			}

			if (RemoveWhiteSpaces(false))
			{
				if (Eof && String.GetLength())
				{
					String.UserData = '\n';
					return false;
				}

				return true;
			}

			switch (Buffer[BufferStart])
			{
			case '\n':
			case 13:
				String.UserData = '\n';
				return false;

			case ',':
				String.UserData = Buffer[BufferStart++];
				return false;

			default:
				String.UserData = ' ';
				return false;
			}

		case '[':
			if (String.GetLength() != 0)
			{
				CompileError(COMPILE_ERROR_SYNTAXERROR, "[");
				String = (char *)NULL;
				return ReadUntilLineBreak();
			}
			else
			{
				StartBracket = true;
				if (String += Buffer[BufferStart])
				{
					return true;
				}
				BufferStart++;
				BufferLength--;
				if (RemoveWhiteSpaces(false))
				{
					if (Eof && String.GetLength())
					{
						return false;
					}

					return true;
				}
			}
			SkipCharacter = true;
			break;
		}

		if (EndBracket)
		{
			if (Buffer[BufferStart] == ' ' || Buffer[BufferStart] == 9)
			{
				DiscardCharacter = true;
			}
			else
			{
				CompileError(COMPILE_ERROR_SYNTAXERROR, NULL);
				return ReadUntilLineBreak();
			}
		}
		if (Buffer[BufferStart] == ']')
		{
			if (!StartBracket)
			{
				CompileError(COMPILE_ERROR_SYNTAXERROR, "]");
				return ReadUntilLineBreak();
			}

			while (String.GetLength())
			{
				c = String[String.GetLength() - 1];
				if (c != ' ' && c != 9)
				{
					break;
				}
				String.DeleteLastCharacter();
			}

			if (String += ']')
			{
				return true;
			}

			BufferStart++;
			BufferLength--;
			if (RemoveWhiteSpaces(false))
			{
				if (Eof && String.GetLength())
				{
					String.UserData = '\n';
					return false;
				}

				return true;
			}

			SkipCharacter = true;
			EndBracket = true;
		}

		if (SkipCharacter)
		{
			SkipCharacter = false;
		}
		else
		{
			if (DiscardCharacter)
			{
				DiscardCharacter = false;
			}
			else
			{
				if (Buffer[BufferStart] == '/')
				{
					Slash = true;
				}
				else
				{
					Slash = false;
				}

				if (String += Buffer[BufferStart])
				{
					return true;
				}
			}
			BufferStart++;
			BufferLength--;
		}
	}
}



char *CInputFile::GetFilename()
{
	return Filename;
}



void CInputFile::GetLastWriteFileTime(FILETIME *pFileTime)
{
	CopyMemory(pFileTime, &FileTime, sizeof(FileTime));
}



DWORD CInputFile::GetCurrentLine()
{
	return CurrentLine;
}



DWORD CInputFile::GetLastLine()
{
	DWORD		Line;


	Line = LastLine;
	LastLine = CurrentLine;
	return Line;
}

