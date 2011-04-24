#ifndef	CIDENTIFIERS_CPP

#define	CIDENTIFIERS_CPP	extern

#endif




class CIdentifiers
{
private:
	CList			*pIdentifierList;

public:
	CIdentifiers();
	~CIdentifiers();

	BOOL			AddIdentifier(char *pName, BOOL CopyName, DWORD Type, POINTER *pPointer);
	BOOL			AddIdentifier(CString &String, DWORD Type, POINTER *pPointer);
	void			GetIdentifier(char *pName, IDENTIFIER *pIdentifier);
	BOOL			GetIdentifier(CString &String, IDENTIFIER *pIdentifier);

	BOOL			SaveObj(HANDLE hObjFile);
} CIDENTIFIERS_CPP Identifiers;

