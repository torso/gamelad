#define		SND_DLL_RELEASENO		0
#define		SND_COMPATIBLERELEASE	4



#define		BufferSize				2048

#define		Offset_dwPosition		0x0000 //4
#define		Offset_IsPlaying		0x0004 //1
#define		Offset_Close			0x0005 //1
#define		Offset_lpdsb			0x0008 //4
#define		Offset_lpdsn			0x000C //4
#define		Offset_Buffer			0x0010 //BufferSize * 2
#define		Offset_csSound			(BufferSize * 2 + 0x0010)

struct SOUNDBUFFER
{
	DWORD					dwPosition;
	BYTE					IsPlaying;
	BYTE					Close;
#ifdef SOUND_CPP
	LPDIRECTSOUNDBUFFER		lpdsb;
	LPDIRECTSOUNDNOTIFY		lpdsn;
#else
	void					*lpdsb, *lpdsn;
#endif
	BYTE					Buffer[BufferSize * 2];
	CRITICAL_SECTION		csSound;
	HANDLE					hEvent[3];
};



//Values for dwAction

enum
{
	//First call must be SND_VERSION and may only be called once.
	//(Snd_Version *)pData
	SND_VERSION = 0,

	//Initializes sound
	//(HWND)dwData = valid window handle
	//					(must not be closed until SND_CLOSESOUND has been sent or dll is unloaded).
	SND_INITSOUND,

	//Closes sound
	SND_CLOSESOUND,

	//Creates a new buffer
	SND_CREATEBUFFER
};


//Return values

enum
{
	SND_OK = 0,

	//Call failed
	SND_FAIL,

	//Fatal error
	SND_ERROR,

	//Invalid dwAction value
	SND_NOTIMPLEMENTED,

	//pData is null or the data pointed to contains invalid information
	SND_INVALIDPARAMETER
};



struct SND_VERSIONSTRUCT
{
	//Program ID, must be 0x4C47 (GL)
	DWORD			dwId;

	//Release number of the calling program
	DWORD			dwReleaseNo;

	//Release number of the dll (returned)
	DWORD			*dwDllReleaseNo;

	//Earliest compatible version of the dll
	DWORD			dwMinimumReleaseNo;
};



typedef		DWORD (__cdecl *SOUNDMAIN)(DWORD, DWORD, VOID *);

