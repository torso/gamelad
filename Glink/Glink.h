#ifndef	GLINK_CPP

#define	GLINK_CPP	extern

#endif



enum LinkErrors
{
	LINK_ERROR_UNSUPPORTEDFORMAT = 1,
	LINK_ERROR_UNSUPPORTEDVERSION,
};



GLINK_CPP	void	LinkError(DWORD ErrorNo, char *pszFilename, char *pszText);



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

