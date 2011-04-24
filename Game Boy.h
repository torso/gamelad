#include	"CList\\CList.h"
#include	"Sound.h"
#include	"Gfx.h"



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
	WORD		RunToBank;
};



struct CHEATDATA
{
	WORD		Offset;
	WORD		Bank;
	static union
	{
		BYTE	Value;
		BYTE	OldValue;
	};
	char		*pszCode;
};



enum
{
	TERMINATING_FALSE = 0,
	TERMINATING_STOP,
	TERMINATING_PAUSE
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
#define		GB_HASRUMBLEPACK		0x00200000
#define		GB_RUMBLE				0x00400000
#define		GB_SERIALBIT			0x00800000
#define		GB_LINKCABLE			0x01000000
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
#define		O1						28

#define		Offset_ActiveRomBank	(O1 + 0) //2
#define		Offset_MaxRomBank		(O1 + 2) //2
#define		Offset_ActiveRamBank	(O1 + 4) //1
#define		Offset_MaxRamBank		(O1 + 5) //1
#define		Offset_DirectionKeys	(O1 + 6) //1
#define		Offset_Buttons			(O1 + 7) //1
#define		O2						O1 + 8

#define		Offset_MEM				(O2 + 0) //0x40
#define		Offset_MEM_CPU			(O2 + 0x40) //0x9000
#define		Offset_MEM_ROM			(O2 + 0x9040) //4
#define		Offset_MEM_RAM			(O2 + 0x9044) //0x22000
#define		Offset_MEM_VRAM			(O2 + 0x2B044) //0x4000
#define		O3						O2 + 0x2F044
#define		Offset_MemStatus		(O3 + 0) //0x40
#define		Offset_MemStatus_CPU	(O3 + 0x40) //0x9000
#define		Offset_MemStatus_ROM	(O3 + 0x9040) //4
#define		Offset_MemStatus_RAM	(O3 + 0x9044) //4
#define		Offset_MemStatus_VRAM	(O3 + 0x9048) //0x4000
#define		O4						O3 + 0xD048

#define		Offset_BGP				(O4 + 0) //0x40
#define		Offset_OBP				(O4 + 0x40) //0x40
#define		O5						O4 + 0x80

#define		Offset_WindowY2			(O5 + 0) //1
#define		Offset_FastFwd			(O5 + 1) //1
#define		Offset_FramesToSkip		(O5 + 2) //1
#define		Offset_FrameSkip		(O5 + 3) //1
#define		Offset_LCD_Ticks		(O5 + 4) //1
#define		Offset_DIV_Ticks		(O5 + 5) //1
#define		Offset_TIMA_Ticks		(O5 + 8) //4
#define		Offset_Hz				(O5 + 0xC) //4
#define		Offset_nCycles			(O5 + 0x10) //4
#define		Offset_pGBBitmap		(O5 + 0x14) //4
#define		Offset_pPalette			(O5 + 0x18) //4
#define		Offset_pAVISoundBuffer	(O5 + 0x1C) //4
#define		Offset_dwAVISoundPos	(O5 + 0x20) //4
#define		O6						O5 + 0x24

//#define		Offset_DrawLineMask		0x03A142 //1

#define		Offset_SoundTicks		(O6 + 0) //1
#define		Offset_Sound1Enabled	(O6 + 1) //1
#define		Offset_Sound2Enabled	(O6 + 2) //1
#define		Offset_Sound3Enabled	(O6 + 3) //1
#define		Offset_Sound4Enabled	(O6 + 4) //1
#define		Offset_Sound1Stage		(O6 + 5) //1
#define		Offset_Sound2Stage		(O6 + 6) //1
#define		Offset_Sound3Stage		(O6 + 7) //1
#define		Offset_Sound4Bit		(O6 + 8) //1
#define		Offset_Sound1Volume		(O6 + 9) //1
#define		Offset_Sound2Volume		(O6 + 0xA) //1
#define		Offset_Sound4Volume		(O6 + 0xB) //1
#define		Offset_SoundL			(O6 + 0xC) //4
#define		Offset_SoundR			(O6 + 0x10) //4
#define		Offset_Sound1Ticks		(O6 + 0x14) //4
#define		Offset_Sound1TimeOut	(O6 + 0x18) //4
#define		Offset_Sound1Frequency	(O6 + 0x1C) //4
#define		Offset_Sound1Envelope	(O6 + 0x20) //4
#define		Offset_Sound1Sweep		(O6 + 0x24) //4
#define		Offset_Sound2Ticks		(O6 + 0x28) //4
#define		Offset_Sound2TimeOut	(O6 + 0x2C) //4
#define		Offset_Sound2Frequency	(O6 + 0x30) //4
#define		Offset_Sound2Envelope	(O6 + 0x34) //4
#define		Offset_Sound3Ticks		(O6 + 0x38) //4
#define		Offset_Sound3TimeOut	(O6 + 0x3C) //4
#define		Offset_Sound3Frequency	(O6 + 0x40) //4
#define		Offset_Sound4Ticks		(O6 + 0x44) //4
#define		Offset_Sound4TimeOut	(O6 + 0x48) //4
#define		Offset_Sound4Frequency	(O6 + 0x4C) //4
#define		Offset_Sound4Envelope	(O6 + 0x50) //4
#define		O7						O6 + 0x54

#define		Offset_RTC				(O7 + 0)
#define		Offset_SEC				(O7 + 0) //1
#define		Offset_MIN				(O7 + 1) //1
#define		Offset_HRS				(O7 + 2) //1
#define		Offset_DAY				Offset_DAYL
#define		Offset_DAYL				(O7 + 3) //1
#define		Offset_DAYH				(O7 + 4) //1
#define		Offset_Latch			(O7 + 5) //1
#define		Offset_RTC_Reg			(O7 + 6) //1
#define		Offset_RTC_Day0			(O7 + 8) //8
#define		O8						O7 + 0x10

#define		Offset_Terminating		(O8 + 0) //1

#define		Offset_SerialInput		(O8 + 1) //1
#define		Offset_SerialBit		(O8 + 2) //1
#define		Offset_SerialTicks		(O8 + 4) //4
#define		Offset_LinkGameBoy		(O8 + 8) //4
#define		Offset_dwNetworkLinkCableNo	(O8 + 0xC) //4

#define		Offset_SoundBuffer		(O8 + 0x10)


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

	WORD			ActiveRomBank;
	WORD			MaxRomBank;
	BYTE			ActiveRamBank;
	BYTE			MaxRamBank;

	BYTE			DirectionKeys;
	BYTE			Buttons;

	BYTE			*MEM[0x10];
	BYTE			MEM_CPU[0x9000];
	BYTE			*MEM_ROM;
	BYTE			MEM_RAM[0x022000];
	BYTE			MEM_VRAM[0x4000];

	BYTE			*MemStatus[0x10];
	BYTE			MemStatus_CPU[0x9000];
	BYTE			*MemStatus_ROM;
	BYTE			*MemStatus_RAM;
	BYTE			MemStatus_VRAM[0x4000];

	BYTE			BGP[0x40];
	BYTE			OBP[0x40];

	BYTE			WindowY2;
	BYTE			FastFwd;
	BYTE			FramesToSkip, FrameSkip;

	BYTE			LCD_Ticks;
	BYTE			DIV_Ticks;
	DWORD			TIMA_Ticks;
	DWORD			Hz;

	DWORD			nCycles;

	WORD			*pGBBitmap;
	WORD			*pPalette;

	void			*pAVISoundBuffer;
	DWORD			dwAVISoundPos;

	//BYTE			DrawLineMask;

	BYTE			SoundTicks;
	BYTE			Sound1Enabled, Sound2Enabled, Sound3Enabled, Sound4Enabled;
	BYTE			Sound1Stage, Sound2Stage, Sound3Stage, Sound4Bit;

	BYTE			Sound1Volume, Sound2Volume, Sound4Volume;

	int				SoundL, SoundR;

	DWORD			Sound1Ticks, Sound1TimeOut, Sound1Frequency, Sound1Envelope, Sound1Sweep;
	DWORD			Sound2Ticks, Sound2TimeOut, Sound2Frequency, Sound2Envelope;
	DWORD			Sound3Ticks, Sound3TimeOut, Sound3Frequency;
	DWORD			Sound4Ticks, Sound4TimeOut, Sound4Frequency, Sound4Envelope;

	BYTE			RTC_SEC, RTC_MIN, RTC_HRS;
	BYTE			RTC_DAYL, RTC_DAYH;
#define				RTC_DAY		(*(WORD *)&RTC_DAYL)
	BYTE			RTC_Latch, RTC_Reg;
	FILETIME		RTC_Day0;

	BYTE			Terminating;

	BYTE			SerialInput, SerialBit;
	DWORD			SerialTicks;
	CGameBoy		*pLinkGameBoy;
	DWORD			dwNetworkLinkCableNo;

	SOUNDBUFFER		SoundBuffer;

	int				JoyLeft, JoyRight, JoyUp, JoyDown;
	BYTE			AutoButtonDown;

	BYTE			StateSlot;


	IDirect3DDevice8	*m_pd3dd;
	HWND			hGfxWnd;

	HWND			hGBWnd;
	HDC				hGBDC;
	HBITMAP			hGBBitmap, hOldBitmap;


#ifdef	CDEBUGINFO_H
	CDebugInfo		*pDebugInfo;
#else	//CDEBUGINFO_H
	void			*pDebugInfo;
#endif	//CDEBUGINFO_H


	DWORD			ThreadId;


private:
	DWORD			RefCount;

	HANDLE			hThread;

	char			Rom[MAX_PATH];
	char			Battery[MAX_PATH];
	unsigned int	SaveRamSize;
	BYTE			BatteryAvailable;
	BYTE			Emulating;

	LARGE_INTEGER	LastTimerCount;
	BOOL			SkipNextFrame;
	char			FramesSkipped;

	CList			*m_pCheatList;

	HWND			m_hSearchCheatWnd;
	HWND			m_hSize, m_hNewSearch, m_hValue, m_hDec;
	HWND			m_hRelValue, m_hRelLast, m_hEqual, m_hNotEqual, m_hLessEqual, m_hMoreEqual, m_hLess, m_hMore;
	HWND			m_hNameStatic, m_hName, m_hAddCheat;
	BYTE			m_SearchSize;

public:
	HWND			m_hHex, m_hList;
	BYTE			m_SortBy;


private:
	BOOL			StartThread();
	void			LinkExecuteLoop();
	void			ExecuteLoop();
	void			LinkDebugLoop();
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

	BYTE			GetRealByte(WORD Offset, WORD Bank);
	BOOL			AddCheat(WORD Offset, BYTE Value, char *pszCode);
	BOOL			AddCheat(WORD Offset, BYTE Value, BYTE CompareValue, char *pszCode);
	BOOL			AddCheat(WORD Bank, WORD Offset, BYTE Value, char *pszCode);
	void			ReApplyCheats();

	void			PerformSearch(UINT uID);


public:
	CGameBoy(BYTE Flags);
	~CGameBoy();

	void			AddRef();
	void			Release();
	BOOL			CanUnload();

	BOOL			Init(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename);
	BOOL			LoadBattery(char *BatteryFilename);
	BOOL			SaveBattery(BOOL Prompt, BOOL SaveAs);
	void			GetBatteryFilename(char *Filename);
	char			*GetStateFilename(char *pszFilename, DWORD dwStateSlot);
	LRESULT			GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
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
	void			Pause();
	void			Stop();

	void			SetFrameSkip(int nFrameSkip);

	BOOL			SaveState();
	BOOL			SaveState(char *pszFilename);
	BOOL			SaveStateAs();
	BOOL			LoadState();
	BOOL			LoadState(char *pszFilename, BOOL QuickLoad);
	BOOL			LoadStateAs();
	void			SetStateSlot(int nSlot);
	int				GetStateSlot();

	BOOL			SaveSnapshot();
	BOOL			SaveVideo();

	BOOL			SwitchRomBank(WORD Bank);
	BOOL			SwitchRamBank(BYTE Bank);
	BOOL			SwitchVBK(BYTE Bank);
	BOOL			SwitchSVBK(BYTE Bank);

	void			RemoveCheats();
	int				VerifyCode(char *pszCode, BOOL CompareValue);
	BOOL			AddCheat(char *pszCode);
	BOOL			IsApplied(char *pszCode);
	void			SearchCheat();
	LRESULT			SearchCheatWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};



enum GameBoyMessages
{
	WM_APP_STEP = WM_APP_FIRSTFREEMESSAGE,
	//WM_APP_RESUME
};



GAME_BOY_CPP	BYTE		ZeroStatus[0x1000];

GAME_BOY_CPP	WORD					RomSize(BYTE Byte148);
GAME_BOY_CPP	void __fastcall		SoundUpdate(CGameBoy *GB);

