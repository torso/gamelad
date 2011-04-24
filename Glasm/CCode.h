#ifndef	CCODE_CPP

#define	CCODE_CPP	extern

#endif



#define		SF_CODE			0x01
#define		SF_CONST		0x02
#define		SF_DATA			0x04
#define		SF_DATA_UNKNOWN	0x08
#define		SF_UNKNOWN		0x10



struct SECTION
{
	DWORD	File;
	DWORD	LineFrom;
	DWORD	Lines;

	WORD	Flags;
	BYTE	Size;
	static union
	{
		DWORD	Bytes;
		char	*pBytes;
	};
};



class CCode
{
private:
	POINTER		Offset;
	CList		*pSections;

public:
	CCode();
	~CCode();

	BOOL		SetOffset(DWORD SectionFlags, POINTER *pPointer);
	BOOL		SetOffset(POINTER *pPointer);
	BOOL		GetOffset(POINTER *pPointer);

	BOOL		InsertData(DWORD Bytes, WORD Flags);

	BOOL		SaveObj(HANDLE hObjFile);
} CCODE_CPP Code;

