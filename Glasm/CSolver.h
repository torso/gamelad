class CSolver
{
private:
	DWORD		dwFlags;
	DWORD		dwNumber;
	CString		*pExpression;

	BOOL		ReadNumber(CString &String, DWORD *Pos, CString &Number, DWORD *dwNewNumber, DWORD Flags);


public:
	CSolver();
	~CSolver();

	void		operator =(DWORD dwNewNumber);
	BOOL		operator =(CString &String);
	BOOL		operator =(CSolver *NewNumber);
	BOOL		operator +=(DWORD dwNewNumber);
};

