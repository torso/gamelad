#define		CID_SUCCESS			0
#define		CID_NOVALUE			(0x80000000 | 1)
#define		CID_ALREADYDEFINED	(0x80000000 | 2)
#define		CID_ERROR			(0x80000000 | 3)

#define		CID_TYPE_LOCAL		0
#define		CID_TYPE_IMPORT		1
#define		CID_TYPE_EXPORT		2
#define		CID_TYPE_GLOBAL		3
#define		CID_TYPE_UNKNOWN	4	//This value is only used internally

#define		CID_LABEL			0
#define		CID_UINT			1
#define		CID_INT				2
#define		CID_CHAR			3
#define		CID_STRUCT			4
#define		CID_STRUCTDEF		5
#define		CID_EQU				0x10
#define		CID_TEXTEQU			0x20

#define		IDENTIFIER_LENGTH	32



struct LABEL
{
	char			Name[IDENTIFIER_LENGTH];
	BOOL			HasValue;
	BYTE			Bank;
	WORD			Offset;
	DWORD			Line;
	WORD			File;
	BYTE			Type;
};



struct LABELLIST
{
	LABEL			Label[100];
	BYTE			nLabels;
	LABELLIST		*pNext;
};



struct VARIABLE
{
	char			Name[IDENTIFIER_LENGTH];
	BYTE			Type;
	BYTE			StorageClass;
	BYTE			Pointer;
	WORD			Dimension;
	char			*Struct;
	WORD			Size;
	ULONGLONG		Value;
	BOOL			HasValue;
	BYTE			Bank;
	WORD			File;
	DWORD			Line;
};



struct VARIABLELIST
{
	VARIABLE		Variable[100];
	BYTE			nIdentifiers;
	VARIABLELIST	*pNext;
};



/*struct STRUCTDEF
{
	char			Name[IDENTIFIER_LENGTH];
	WORD			File;
	DWORD			Line;
};



struct STRUCTDEFLIST
{
	STRUCTDEF		StructDef[10];
	BYTE			nStructDef;
	STRUCTDEFLIST	*pNext;
};*/



class CIdentifiers
{
private:
	LABELLIST		*LabelList;
	VARIABLELIST	*VariableList;
	//STRUCTDEFLIST	*StructDefList;

	DWORD			nIdentifiers;


	LABEL			*CreateLabel(char *Name);

public:
	CIdentifiers();
	~CIdentifiers();

	int		AddLabel(char *Name, WORD Offset, BYTE Bank, WORD File, DWORD Line);
	int		GetLabel(char *Name);
	int		GetLabel(char *Name, BYTE Type);

	/*int		GetValue(char *Name, int *Value, BYTE *Bank);
	void	SetValue(char *Name, int Value, BYTE Bank, WORD File, DWORD Line);
	BOOL	UseIdentifier(char *Name, int Address, BYTE Bank, BYTE Flags);*/

	BOOL	SaveObj(HANDLE hObjFile);
};

