#include	<dsound.h>



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



extern	BOOL	InitSound();
extern	void	CloseSound();
extern	BOOL	NewSoundBuffer(SOUNDBUFFER *pSoundBuffer);

