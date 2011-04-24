#ifdef	GLASM_CPP

#define	EQUALNULL	= NULL

#else

#define	GLASM_CPP	extern
#define	EQUALNULL

#endif



typedef		ULONGLONG		QWORD;



#define		WriteToFile(hFile, lpBuffer, nBytes)					\
	if (!WriteFile(hFile, lpBuffer, nBytes, &BytesWritten, NULL))	\
	{																\
		return true;												\
	}																\
	if (BytesWritten != (nBytes))									\
	{																\
		return true;												\
	}



enum CompileErrors
{
	COMPILE_ERROR_SYNTAXERROR = 1,
	COMPILE_ERROR_IMPROPEROPERANDTYPE,
	COMPILE_ERROR_INVALIDNUMBEROFOPERANDS,
	COMPILE_ERROR_BADNUMBER,
	COMPILE_ERROR_UNMATCHEDBRACKET,
	COMPILE_ERROR_IDENTIFIERREDEFINED,
	COMPILE_ERROR_CODEOUTSIDESECTION,
	COMPILE_ERROR_DIFFERENTLINKAGE,
};



GLASM_CPP	void	CompileError(DWORD ErrorNo, char *pszText);
GLASM_CPP	void	Warning(DWORD WarningNo, char *pszText);



#define		PF_OFFSET				0x01
#define		PF_OFFSETEXPRESSION		0x02

struct		POINTER
{
	CSolver	*pOffset;
	DWORD	BankNumber;
};



struct		MNEMONIC
{
	char	Name[6];
	BYTE	OpCode;
	BYTE	Flags;
};



struct		OPCODE
{
	BYTE	OpCode;
	WORD	Code;
	DWORD	Flags;
};



#define		IF_DEFINED				0x00000001
#define		IF_EXPORT				0x00000002
#define		IF_IMPORT				0x00000004
#define		IF_GLOBAL				0x00000008
#define		IF_LINKAGE				(IF_EXPORT | IF_IMPORT | IF_GLOBAL)

#define		IF_LABEL				0x00000010
#define		IF_VARIABLE				0x00000020
#define		IF_STRUCTDEFINITION		0x00000030
#define		IF_TYPE					(IF_LABEL | IF_VARIABLE | IF_STRUCTDEFINITION)

#define		IF_MNEMONIC				0x00000100

struct IDENTIFIER
{
	DWORD	Flags;
	DWORD	File;
	DWORD	Line;

	char	*pName;

	static union
	{
		POINTER		Pointer;
		static struct
		{
			MNEMONIC	*Mnemonic;
		};
	};
};



#undef	EQUALNULL

