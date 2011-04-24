#include	"CList\\CList.h"



#ifndef		GAME_BOY_CPP

#define		GAME_BOY_CPP	extern

#endif		//GAME_BOY_CPP



#define			EMU_STEPINTO	1
#define			EMU_RUNTO		2
#define			EMU_STEPOUT		3



struct EMULATIONINFO
{
	DWORD		Flags;
	WORD		RunToOffset;
	BYTE		RunToBank;
};



struct CHEATDATA
{
	WORD		Offset;
	BYTE		Bank;
	static union
	{
		BYTE	Value;
		BYTE	OldValue;
	};
	char		*pszCode;
};



#define		GB_COLOR				0x00000001
//#define		GB_SUPER			0x00000002
#define		GB_ROM_COLOR			0x00000004
//#define		GB_ROM_SUPER		0x00000008
#define		GB_EXITLOOPDIRECTLY		0x00000010
#define		GB_EXITLOOP				0x00000020
#define		GB_INVALIDOPCODE		0x00000040
#define		GB_RAMENABLE			0x00000080
#define		GB_HALT					0x00010000
#define		GB_ENABLEIE				0x00020000
#define		GB_IE					0x00040000
#define		GB_HDMA					0x00100000
#define		GB_ERROR				0x10000000
#define		GB_DEBUGRUNINFO			0x20000000
#define		GB_DEBUGINFO			0x40000000
#define		GB_DEBUG				0x80000000



#define		Flag_C					0x10
#define		Flag_H					0x20
#define		Flag_N					0x40
#define		Flag_Z					0x80


#define		MEM_READ				0x01	//Only initialized data may be read (set when writing to address)
#define		MEM_WRITE				0x02	//Only reserved space and some hardware addresses may be written to
#define		MEM_EXECUTE				0x04	//Valid address for PC (must be combined with MEM_READ)
#define		MEM_STACK				0x08	//Valid address for SP
#define		MEM_BREAKPOINT			0x10
#define		MEM_CHANGED				0x20
#define		MEM_FIXED				0x40	//Data cannot be changed (due to cheats)


#define		Offset_Reg_F			0
#define		Offset_Reg_A			1
#define		Offset_Reg_AF			0
#define		Offset_Reg_C			4
#define		Offset_Reg_B			5
#define		Offset_Reg_BC			4
#define		Offset_Reg_E			8
#define		Offset_Reg_D			9
#define		Offset_Reg_DE			8
#define		Offset_Reg_L			12
#define		Offset_Reg_H			13
#define		Offset_Reg_HL			12
#define		Offset_Reg_SP			16
#define		Offset_Reg_PC			20
#define		Offset_Flags			24

#define		Offset_ActiveRomBank	28
#define		Offset_ActiveRamBank	29
#define		Offset_MaxRomBank		30
#define		Offset_MaxRamBank		31

#define		Offset_MEM				32
#define		Offset_MEM_CPU			0x60
#define		Offset_MEM_ROM			0x009060
#define		Offset_MEM_RAM			0x009064
#define		Offset_MEM_VRAM			0x029064

#define		Offset_MemStatus		0x02D064
#define		Offset_MemStatus_CPU	0x02D0A4
#define		Offset_MemStatus_ROM	0x0360A4
#define		Offset_MemStatus_RAM	0x0360A8
#define		Offset_MemStatus_VRAM	0x0360AC

#define		Offset_BGP				0x03A0AC //0x40
#define		Offset_OBP				0x03A0EC //0x40

#define		Offset_DirectionKeys	0x03A12C //1
#define		Offset_Buttons			0x03A12D //1
#define		Offset_LCD_Ticks		0x03A12E //1
#define		Offset_DIV_Ticks		0x03A12F //1
#define		Offset_TIMA_Ticks		0x03A130 //4
#define		Offset_Hz				0x03A134 //4

#define		Offset_nCycles			0x03A138 //4

#define		Offset_pGBBitmap		0x03A13C //4

#define		Offset_WindowY			0x03A140 //1
#define		Offset_WindowY2			0x03A141 //1

//#define		Offset_DrawLineMask		0x03A142 //1

#define		Offset_SoundTicks		0x03A143 //1
#define		Offset_Sound1Enabled	0x03A144 //1
#define		Offset_Sound2Enabled	0x03A145 //1
#define		Offset_Sound3Enabled	0x03A146 //1
#define		Offset_Sound4Enabled	0x03A147 //1
#define		Offset_Sound1Stage		0x03A148 //1
#define		Offset_Sound2Stage		0x03A149 //1
#define		Offset_Sound3Stage		0x03A14A //1
#define		Offset_Sound4Bit		0x03A14B //1
#define		Offset_Sound1Volume		0x03A14C //1
#define		Offset_Sound2Volume		0x03A14D //1
#define		Offset_Sound4Volume		0x03A14E //1
#define		Offset_SoundL			0x03A150 //4
#define		Offset_SoundR			0x03A154 //4
#define		Offset_Sound1Ticks		0x03A158 //4
#define		Offset_Sound1TimeOut	0x03A15C //4
#define		Offset_Sound1Frequency	0x03A160 //4
#define		Offset_Sound1Envelope	0x03A164 //4
#define		Offset_Sound1Sweep		0x03A168 //4
#define		Offset_Sound2Ticks		0x03A16C //4
#define		Offset_Sound2TimeOut	0x03A170 //4
#define		Offset_Sound2Frequency	0x03A174 //4
#define		Offset_Sound2Envelope	0x03A178 //4
#define		Offset_Sound3Ticks		0x03A17C //4
#define		Offset_Sound3TimeOut	0x03A180 //4
#define		Offset_Sound3Frequency	0x03A184 //4
#define		Offset_Sound4Ticks		0x03A188 //4
#define		Offset_Sound4TimeOut	0x03A18C //4
#define		Offset_Sound4Frequency	0x03A190 //4
#define		Offset_Sound4Envelope	0x03A194 //4
#define		Offset_FastFwd			0x03A198 //1
#define		Offset_FramesToSkip		0x03A199 //1
#define		Offset_FrameSkip		0x03A19A //1
#define		Offset_pAVISoundBuffer	0x03A19C //4
#define		Offset_dwAVISoundPos	0x03A1A0 //4
#define		Offset_pPalette			0x03A1A4 //4
#define		Offset_SoundBuffer		0x03A1A8
/*#define		Offset_SerialOutput		0x03A198 //4
#define		Offset_SerialInput		0x03A199 //1
#define		Offset_SerialByte		0x03A19A //1
#define		Offset_SerialTicks		0x03A19B //1*/


#define		FF00_C(Offset)			MEM_CPU[0x8F00 + Offset]
#define		pFF00_C(GB, Offset)		GB->MEM_CPU[0x8F00 + Offset]
#define		FF00_ASM				(Offset_MEM_CPU + 0x8F00)

//#define		ROM(n)					(*((BYTE *)(n + (DWORD)MEM_ROM)))
//#define		pROM(n)					((BYTE *)(n + (DWORD)MEM_ROM))

#define		pMemStatus_ROM(n)		((BYTE *)(n + (DWORD)MemStatus_ROM))



class CGameBoy
{
public:
	static union
	{
		static struct
		{
			BYTE	Reg_F;
			BYTE	Reg_A;
			WORD	dummy;
		};
		WORD		Reg_AF;
	};
	static union
	{
		static struct
		{
			BYTE	Reg_C;
			BYTE	Reg_B;
			WORD	dummy;
		};
		WORD		Reg_BC;
	};
	static union
	{
		static struct
		{
			BYTE	Reg_E;
			BYTE	Reg_D;
			WORD	dummy;
		};
		WORD		Reg_DE;
	};
	static union
	{
		static struct
		{
			BYTE	Reg_L;
			BYTE	Reg_H;
			WORD	dummy;
		};
		WORD		Reg_HL;
	};
	static struct
	{
		WORD		Reg_SP;
		WORD		dummy;
	};
	static struct
	{
		WORD		Reg_PC;
		WORD		dummy;
	};
	DWORD			Flags;

	BYTE			ActiveRomBank;
	BYTE			ActiveRamBank;
	BYTE			MaxRomBank;
	BYTE			MaxRamBank;

	BYTE			*MEM[0x10];
	BYTE			MEM_CPU[0x9000];
	BYTE			*MEM_ROM;
	BYTE			MEM_RAM[0x020000];
	BYTE			MEM_VRAM[0x4000];

	BYTE			*MemStatus[0x10];
	BYTE			MemStatus_CPU[0x9000];
	BYTE			*MemStatus_ROM;
	BYTE			*MemStatus_RAM;
	BYTE			MemStatus_VRAM[0x4000];

	BYTE			BGP[0x40];
	BYTE			OBP[0x40];

	BYTE			DirectionKeys;
	BYTE			Buttons;

	BYTE			LCD_Ticks;
	BYTE			DIV_Ticks;
	DWORD			TIMA_Ticks;
	DWORD			Hz;

	DWORD			nCycles;

	WORD			*pGBBitmap;

	BYTE			WindowY, WindowY2;

	BYTE			DrawLineMask; //not used


	BYTE			SoundTicks;
	BYTE			Sound1Enabled, Sound2Enabled, Sound3Enabled, Sound4Enabled;
	BYTE			Sound1Stage, Sound2Stage, Sound3Stage, Sound4Bit;

	BYTE			Sound1Volume, Sound2Volume, Sound4Volume;

	int				SoundL, SoundR;

	DWORD			Sound1Ticks, Sound1TimeOut, Sound1Frequency, Sound1Envelope, Sound1Sweep;
	DWORD			Sound2Ticks, Sound2TimeOut, Sound2Frequency, Sound2Envelope;
	DWORD			Sound3Ticks, Sound3TimeOut, Sound3Frequency;
	DWORD			Sound4Ticks, Sound4TimeOut, Sound4Frequency, Sound4Envelope;

	BYTE			FastFwd;
	BYTE			FramesToSkip, FrameSkip;

	void			*pAVISoundBuffer;
	DWORD			dwAVISoundPos;

	WORD			*pPalette;

	SOUNDBUFFER		SoundBuffer;

	BYTE			StateSlot;

	//BYTE			*SerialOutput, SerialInput, SerialByte, SerialTicks;


	HWND			hGBWnd;
	HDC				hGBDC;
	HBITMAP			hGBBitmap, hOldBitmap;

#ifdef	CDEBUGINFO_H
	CDebugInfo		*pDebugInfo;
#else	//CDEBUGINFO_H
	void			*pDebugInfo;
#endif	//CDEBUGINFO_H


private:
	HANDLE			hThread;
	DWORD			ThreadId;

	BOOL			Terminating;

	char			Rom[MAX_PATH];
	char			Battery[MAX_PATH];
	unsigned int	SaveRamSize;
	BYTE			BatteryAvailable;
	BYTE			Emulating;

	LARGE_INTEGER	LastTimerCount;
	LONGLONG		DelayTime;

	CList			*m_pCheatList;


private:
	BOOL			StartThread();
	void			ExecuteLoop();
	void			DebugLoop();
	void			StepLoop(EMULATIONINFO *pEmulationInfo);
	void			SetStartDelay();
	void			Delay();

	void			ClearDebugRunInfo();
	void			PrepareEmulation(BOOL Debug);
	void			MainLoop();
	void			DebugMainLoop();
	void			RefreshScreen();
	BOOL			RestoreSound();

	void			CloseAVI();
	BOOL			WriteAVI();

	char			*GetStateFilename(char *pszFilename);

	BYTE			GetRealByte(WORD Offset, BYTE Bank);
	BOOL			AddCheat(WORD Offset, BYTE Value, char *pszCode);
	BOOL			AddCheat(WORD Offset, BYTE Value, BYTE CompareValue, char *pszCode);
	BOOL			AddCheat(BYTE Bank, WORD Offset, BYTE Value, char *pszCode);
	void			ReApplyCheats();


public:
	CGameBoy(BYTE Flags);
	~CGameBoy();

	BOOL			Init(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename);
	BOOL			LoadBattery(char *BatteryFilename);
	BOOL			SaveBattery(BOOL Prompt, BOOL SaveAs);
	void			GetBatteryFilename(char *Filename);
	char			*GetStateFilename(char *pszFilename, DWORD dwStateSlot);
	LPARAM			GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void			Reset(DWORD Flags);
	void			Reset();
	void			CloseSound();

	DWORD			ThreadProc();

	BOOL			IsEmulating();
	BOOL			HasBattery();

	BOOL			Execute();
	BOOL			StartDebug();
	BOOL			Step(EMULATIONINFO *pEmulationInfo);
	void			Resume();
	void			Stop();

	void			SetFrameSkip(int nFrameSkip);

	BOOL			SaveState();
	BOOL			SaveState(char *pszFilename, BOOL AlreadyStopped);
	BOOL			SaveStateAs();
	BOOL			LoadState();
	BOOL			LoadState(char *pszFilename, BOOL AlreadyStopped, BOOL QuickLoad);
	BOOL			LoadStateAs();
	void			SetStateSlot(int nSlot);
	int				GetStateSlot();

	BOOL			SaveSnapshot();
	BOOL			SaveVideo();

	BOOL			SwitchRomBank(BYTE Bank);
	BOOL			SwitchRamBank(BYTE Bank);
	BOOL			SwitchVBK(BYTE Bank);
	BOOL			SwitchSVBK(BYTE Bank);

	void			RemoveCheats();
	int				VerifyCode(char *pszCode, BOOL CompareValue);
	BOOL			AddCheat(char *pszCode);
	BOOL			IsApplied(char *pszCode);
};



enum GameBoyMessages
{
	WM_APP_STEP = WM_APP_FIRSTFREEMESSAGE,
	WM_APP_RESUME
};



GAME_BOY_CPP	BYTE		ZeroStatus[0x1000];

extern BYTE					RomSize(BYTE Byte148);
extern void __fastcall		SoundUpdate(CGameBoy *GB);
extern void __fastcall		Sound1(CGameBoy *GB);
extern void __fastcall		Sound2(CGameBoy *GB);
extern void __fastcall		Sound3(CGameBoy *GB);
extern void __fastcall		Sound4(CGameBoy *GB);

