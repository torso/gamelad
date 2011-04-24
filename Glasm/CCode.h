#ifndef	CCODE_CPP

#define	CCODE_CPP	extern

#endif



#define		SF_CONST		0x00000001
#define		SF_CODE			0x00000002
#define		SF_UNKNOWN		0x00000004
#define		SF_EXPRESSION	0x00000080



struct SECTION
{
	DWORD	File;
	DWORD	LineFrom;
	DWORD	Lines;

	DWORD	Flags;
	DWORD	Size;
	static union
	{
		DWORD	Bytes;
		BYTE	*pBytes;
		CSolver	*pExpression;
	};
};



class CCode
{
private:
	POINTER		m_Offset;
	CList		*m_pSections;

public:
	CCode();
	~CCode();

	BOOL		SetOffset(DWORD SectionFlags, POINTER *pPointer);
	BOOL		SetOffset(POINTER *pPointer);
	BOOL		GetOffset(POINTER *pPointer);

	BOOL		InsertData(BYTE Byte, DWORD Flags);
	BOOL		InsertData(CSolver &Expression, BYTE Size, DWORD Flags);

	BOOL		SaveObj(HANDLE hObjFile);
} CCODE_CPP Code;

