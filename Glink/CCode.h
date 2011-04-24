#define		MAX_BANK			128

#define		MACHINE_UNKNOWN		0
#define		MACHINE_CGB			1



struct BANK
{
	BYTE	Bank[0x4000];
	BYTE	Used[0x4000 / 8];
	WORD	File[0x4000];
	DWORD	LineFrom[0x4000];
	DWORD	Lines[0x4000];
	char	*Expression[0x4000];
};



class CCode
{
private:
	WORD	Offset;
	BYTE	BankNo;
	BYTE	Machine;
	BANK	*Banks[MAX_BANK];

	BOOL	SpaceUsed(WORD Size, WORD File, DWORD LineFrom, DWORD Lines);

public:
	CCode();
	~CCode();

	BOOL	InsertCode(DWORD Code, WORD Size, WORD File, DWORD LineFrom, DWORD LineTo);
	BOOL	InsertCode(BYTE *Code, WORD Size, WORD File, DWORD LineFrom, DWORD LineTo);
	BOOL	InsertUnknownCode(WORD Size, WORD File, DWORD LineFrom, DWORD LineTo);
	BOOL	InsertExpression(char *Expression, BYTE Size, WORD File, DWORD LineFrom, DWORD LineTo);

	BOOL	SetOffset(BYTE NewBank, WORD NewOffset);
	WORD	GetOffset();
	BYTE	GetBank();
	BYTE	GetMachine();

	BOOL	SaveObj(HANDLE hFile);
};

