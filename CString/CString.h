#define		BUFFER_SIZE		0x100



struct BUFFER
{
	char	pBuffer[BUFFER_SIZE];
	BUFFER	*pNext;
	BUFFER	*pPrevious;
};



class CString
{
private:
	BUFFER	*pFirstBuffer, *pLastBuffer;
	DWORD	Length;

public:
	DWORD	UserData;


	CString();
	~CString();

	BOOL	operator =(char c);
	BOOL	operator =(char *pszString);
	BOOL	operator =(CString *String);
	BOOL	operator =(CString &String);
	BOOL	operator +=(char c);
	BOOL	operator +=(char *pszString);
	BOOL	operator +=(CString *String);
	BOOL	operator +=(CString &String);
	BOOL	operator ==(char c);
	BOOL	operator ==(char *pszString);
	BOOL	operator ==(CString *String);
	BOOL	operator ==(CString &String);
	BOOL	operator !=(char c);
	BOOL	operator !=(char *pszString);
	BOOL	operator !=(CString *String);
	BOOL	operator !=(CString &String);
	char	operator [](DWORD dwPosition);

	BOOL	GetString(char **pBuffer);
	void	GetString(char *pBuffer, DWORD Size);
	DWORD	GetLength();
	CString	&ToLower();
	void	ToUpper();
	void	DeleteLastCharacter();
};

