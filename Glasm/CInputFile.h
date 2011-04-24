#ifndef	CINPUTFILE_CPP

#define	CINPUTFILE_CPP	extern
#define	EQUALNULL

#else

#define	EQUALNULL		= NULL

#endif



class CInputFile
{
private:
	HANDLE		hFile;
	BYTE		Buffer[0x200];
	int			BufferStart, BufferLength;
	char		Filename[MAX_PATH];
	FILETIME	FileTime;
	DWORD		CurrentLine, LastLine;
	BOOL		Eof;


	BOOL		FillBuffer();
	BOOL		RemoveWhiteSpaces(BOOL RemoveLF);
	BOOL		ReadUntilEndComment();
	BOOL		WriteToObj(HANDLE hObjFile);

	void		DeleteAllItems();

public:
	CInputFile();
	~CInputFile();


	BOOL		Open(char *pFilename);
	BOOL		Read(CString &pString, BOOL BreakAtSpace);
	BOOL		ReadUntilLineBreak();
	char		*GetFilename();
	void		GetLastWriteFileTime(FILETIME *pFileTime);
	DWORD		GetCurrentLine();
	DWORD		GetLastLine();

	//BOOL		SaveObj(HANDLE hObjFile);
} CINPUTFILE_CPP	*InputFile EQUALNULL;

extern		CList	InputFiles;

