#ifndef		EMULATION_CPP

#define		EMULATION_CPP	extern

#endif



#define			EMU_STEPINTO	1
#define			EMU_RUNTO		2
#define			EMU_STEPOUT		3



struct EMULATIONINFO
{
	CGameBoy	*GameBoy1;
	//CGameBoy	*GameBoy2;
	DWORD		Flags;
	WORD		RunToOffset;
	BYTE		RunToBank;
};



EMULATION_CPP	DWORD WINAPI	GameLoop(void *pGameBoy);
EMULATION_CPP	DWORD WINAPI	DebugGameLoop(void *pGameBoy);
EMULATION_CPP	DWORD WINAPI	StepGameLoop(void *pEmulationInfo);

