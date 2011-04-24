enum FatalErrors
{
	FATAL_ERROR_OUTOFMEMORY,
	FATAL_ERROR_OPENFILE,
	FATAL_ERROR_READFILE,
	FATAL_ERROR_WRITEFILE,
};



extern	void	FatalError(DWORD ErrorNo, char *pszText);

