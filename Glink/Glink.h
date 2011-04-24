#ifndef	GLINK_CPP

#define	GLINK_CPP	extern

#endif



enum LinkErrors
{
	LINK_ERROR_UNSUPPORTEDFORMAT = 1,
	LINK_ERROR_UNSUPPORTEDVERSION,
};



GLINK_CPP	void	LinkError(DWORD ErrorNo, char *pszFilename, char *pszText);



struct SECTION
{
};



struct SECTIONDATA
{
};

