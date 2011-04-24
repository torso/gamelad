#ifndef		GAME_BOY_CPP

#define		GAME_BOY_CPP	extern

#endif



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

//#define		Offset_DrawLineMask		33

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

#define		Offset_SoundTicks		0x03A140 //1
#define		Offset_Sound1Enabled	0x03A141 //1
#define		Offset_Sound2Enabled	0x03A142 //1
#define		Offset_Sound3Enabled	0x03A143 //1
#define		Offset_Sound1Ticks		0x03A144 //4
#define		Offset_Sound1Frequency	0x03A148 //4
#define		Offset_Sound1Volume		0x03A14C //4
#define		Offset_Sound1Stage		0x03A150 //1
#define		Offset_Sound2Stage		0x03A151 //1
#define		Offset_Sound1L			0x03A154 //4
#define		Offset_Sound1R			0x03A158 //4

#define		FF00_C(Offset)			MEM_CPU[0x8F00 + Offset]
#define		pFF00_C(GB, Offset)		GB->MEM_CPU[0x8F00 + Offset]
#define		FF00_ASM				(Offset_MEM_CPU + 0x8F00)

//#define		ROM(n)					(*((BYTE *)(n + (DWORD)MEM_ROM)))
//#define		pROM(n)					((BYTE *)(n + (DWORD)MEM_ROM))

#define		pMemStatus_ROM(n)		((BYTE *)(n + (DWORD)MemStatus_ROM))


struct SOUNDBUFFER
{
	BYTE			Data[1024];
	WAVEHDR			wh;
};



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

	//BYTE			DrawLineMask;

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

	//DWORD			SIOClocks;


	BYTE			SoundTicks;
	BYTE			Sound1Enabled, Sound2Enabled, Sound3Enabled;
	DWORD			Sound1Ticks, Sound1Frequency, Sound1Volume;
	BYTE			Sound1Stage, Sound2Stage;
	int				Sound1L, Sound1R;

	DWORD			Sound1TimeOut, Sound1Sweep, Sound1Envelope;
	DWORD			Sound2Ticks, Sound2Frequency, Sound2Volume;
	DWORD			Sound2TimeOut, Sound2Envelope;
	int				Sound2L, Sound2R;

	DWORD			Sound3TimeOut;
	DWORD			Sound3Ticks, Sound3Frequency;
	int				Sound3L, Sound3R;
	BYTE			Sound3Stage;
	BYTE			Sound4Enabled, Sound4Bit;
	DWORD			Sound4Ticks, Sound4TimeOut, Sound4Frequency, Sound4Volume, Sound4Envelope;
	int				Sound4L, Sound4R;


	signed char		WindowTileY;
	BYTE			WindowY, WindowTileY2;

	BOOL			FastFwd;

	HWND			hGBWnd;
	HDC				hGBDC;
	HBITMAP			hGBBitmap, hOldBitmap;

	HWAVEOUT		hWaveOut;
	SOUNDBUFFER		*SoundBuffer;
	DWORD			SoundBufferPosition;

	HANDLE			hThread;
	DWORD			ThreadId;

	char			Rom[MAX_PATH];
	char			Battery[MAX_PATH];
	unsigned int	SaveRamSize;
	BOOL			BatteryAvailable;

	LARGE_INTEGER	LastTimerCount;
	LONGLONG		DelayTime;


public:
	CGameBoy(BYTE Flags);
	~CGameBoy();

	BOOL			Init(char *pszROMFilename, char *pszBatteryFilename);
	BOOL			LoadBattery();
	BOOL			LoadBattery(char *BatteryFilename);
	BOOL			SaveBattery(BOOL Prompt, BOOL SaveAs);
	void			GetBatteryFilename(char *Filename);
	LPARAM			GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void			Reset(DWORD Flags);
	void			Reset();
	void			PrepareEmulation(BOOL Debug);
	void			SetStartDelay();
	void			Delay();
	void			MainLoop();
	void			DebugMainLoop();
	void			RefreshScreen();
	BOOL			RestoreSound();
	void			CloseSound();
};



GAME_BOY_CPP	BYTE		ZeroStatus[0x1000];

extern BYTE					RomSize(BYTE Byte148);
extern void __fastcall		SoundUpdate(CGameBoy *GB);
extern void __fastcall		Sound1(CGameBoy *GB);
extern void __fastcall		Sound2(CGameBoy *GB);
extern void __fastcall		Sound3(CGameBoy *GB);
extern void __fastcall		Sound4(CGameBoy *GB);

