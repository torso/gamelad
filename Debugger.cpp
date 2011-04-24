#define		_WIN32_WINNT	0x0500

#include	<windows.h>
#include	"resource.h"

#define		DEBUGGER_CPP
#include	"CDebugInfo.h"
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CGameBoys.h"
#include	"Debugger.h"
#include	"Emulation.h"
#include	"Z80.h"



enum DebuggerMessages
{
	WM_APP_SELECTTILE = WM_APP_FIRSTFREEMESSAGE,
	WM_APP_HOVERTILE,
	WM_APP_SETTILE
};



//Color1    (BYTE)  76543210
//      ->  (DWORD) --0---1- --3---2- --5---4- --7---6-
//Color2    (BYTE)  76543210
//      ->  (DWORD) ---0---1 ---3---2 ---5---4 ---7---6
#define		TileToBmp(Color1, Color2)											\
							(((Color1 & 0x80) >> 2) | ((Color2 & 0x80) >> 3)  |	\
							((Color1 & 0x40) >> 5)  | ((Color2 & 0x40) >> 6)  |	\
							((Color1 & 0x20) << 8)  | ((Color2 & 0x20) << 7)  |	\
							((Color1 & 0x10) << 5)  | ((Color2 & 0x10) << 4)  |	\
							((Color1 & 0x08) << 18) | ((Color2 & 0x08) << 17) |	\
							((Color1 & 0x04) << 15) | ((Color2 & 0x04) << 14) |	\
							((Color1 & 0x02) << 28) | ((Color2 & 0x02) << 27) |	\
							((Color1 & 0x01) << 25) | ((Color2 & 0x01) << 24))

//Color1    (BYTE)  76543210
//      ->  (DWORD) --6---7- --4---5- --2---3- --0---1-
//Color2    (BYTE)  76543210
//      ->  (DWORD) ---6---7 ---4---5 ---2---3 ---0---1
#define		TileToBmpFlipX(Color1, Color2)										\
							(((Color1 & 0x80) << 18)| ((Color2 & 0x80) << 17) |	\
							((Color1 & 0x40) << 23) | ((Color2 & 0x40) << 22) |	\
							((Color1 & 0x20) << 12) | ((Color2 & 0x20) << 11) |	\
							((Color1 & 0x10) << 17) | ((Color2 & 0x10) << 16) |	\
							((Color1 & 0x08) << 6)  | ((Color2 & 0x08) << 5)  |	\
							((Color1 & 0x04) << 11) | ((Color2 & 0x04) << 10) |	\
							 (Color1 & 0x02)        | ((Color2 & 0x02) >> 1)  |	\
							((Color1 & 0x01) << 5)  | ((Color2 & 0x01) << 4))



const DWORD	GreyScales[4] =	{	0xFFFFFF,
								0xAAAAAA,
								0x555555,
								0
							};



struct _OpCodeNames
{
	char	Text1[19], Text2[6];
	DWORD	Flags;
};



#define		OCF_DATA8		0x00000001
#define		OCF_DATA16		0x00000002
#define		OCF_DISP		0x00000004
#define		OCF_CALL		0x00000008
#define		OCF_JUMP		0x40000000
#define		OCF_ADDRESS_BC	0x00000010
#define		OCF_ADDRESS_DE	0x00000020
#define		OCF_ADDRESS_HL	0x00000040
#define		OCF_ADDRESS_SP	0x20000000
#define		OCF_ADDRESS_C	0x80000000
#define		OCF_ADDRESS		0x00000080
#define		OCF_ADDRESS_16	0x10000000
#define		OCF_REG_HL		0x01000000
#define		OCF_COND_NZ		0x00000100
#define		OCF_COND_Z		0x00000200
#define		OCF_COND_NC		0x00000400
#define		OCF_COND_C		0x00000800
#define		OCF_RST00		0x00001000
#define		OCF_RST08		0x00002000
#define		OCF_RST10		0x00004000
#define		OCF_RST18		0x00008000
#define		OCF_RST20		0x00010000
#define		OCF_RST28		0x00020000
#define		OCF_RST30		0x00040000
#define		OCF_RST38		0x00080000

#define		OCF_BYTES		(OCF_DATA8 | OCF_DATA16)

const _OpCodeNames	OpCodeNames[256] = {
							"nop", "", 0,			//0x00
							"ld   bc, ", "", OCF_DATA16,
							"ld   [bc], a", "", OCF_ADDRESS_BC,
							"inc  bc", "", 0,
							"inc  b", "", 0,
							"dec  b", "", 0,
							"ld   b, ", "", OCF_DATA8,
							"rlca", "", 0,
							"ld   [", "], sp", OCF_DATA16 | OCF_ADDRESS | OCF_ADDRESS_16,	//0x08
							"add  hl, bc", "", 0,
							"ld   a, [bc]", "", OCF_ADDRESS_BC,
							"dec  bc", "", 0,
							"inc  c", "", 0,
							"dec  c", "", 0,
							"ld   c, ", "", OCF_DATA8,
							"rrca", "", 0,
							"stop", "", OCF_DATA8,			//0x10
							"ld   de, ", "", OCF_DATA16,
							"ld   [de], a", "", OCF_ADDRESS_DE,
							"inc  de", "", 0,
							"inc  d", "", 0,
							"dec  d", "", 0,
							"ld   d, ", "", OCF_DATA8,
							"rla", "", 0,
							"jr   ", "", OCF_DATA8 | OCF_JUMP | OCF_DISP,			//0x18
							"add  hl, de", "", 0,
							"ld   a, [de]", "", OCF_ADDRESS_DE,
							"dec  de", "", 0,
							"inc  e", "", 0,
							"dec  e", "", 0,
							"ld   e, ", "", OCF_DATA8,
							"rra", "", 0,
							"jr   nz, ", "", OCF_DATA8 | OCF_JUMP | OCF_DISP | OCF_COND_NZ,	//0x20
							"ld   hl, ", "", OCF_DATA16,
							"ldi  [hl], a", "", OCF_ADDRESS_HL,
							"inc  hl", "", 0,
							"inc  h", "", 0,
							"dec  h", "", 0,
							"ld   h, ", "", OCF_DATA8,
							"daa", "", 0,
							"jr   z, ", "", OCF_DATA8 | OCF_JUMP | OCF_DISP | OCF_COND_Z,	//0x28
							"add  hl, hl", "", 0,
							"ldi  a, [hl]", "", OCF_ADDRESS_HL,
							"dec  hl", "", 0,
							"inc  l", "", 0,
							"dec  l", "", 0,
							"ld   l, ", "", OCF_DATA8,
							"cpl", "", 0,
							"jr   nc, ", "", OCF_DATA8 | OCF_JUMP | OCF_DISP | OCF_COND_NC,	//0x30
							"ld   sp, ", "", OCF_DATA16,
							"ldd  [hl], a", "", OCF_ADDRESS_HL,
							"inc  sp", "", 0,
							"inc  [hl]", "", OCF_ADDRESS_HL,
							"dec  [hl]", "", OCF_ADDRESS_HL,
							"ld   [hl], ", "", OCF_DATA8 | OCF_ADDRESS_HL,
							"scf", "", 0,
							"jr   c, ", "", OCF_DATA8 | OCF_JUMP | OCF_DISP | OCF_COND_C,	//0x38
							"add  hl, sp", "", 0,
							"ldd  a, [hl]", "", OCF_ADDRESS_HL,
							"dec  sp", "", 0,
							"inc  a", "", 0,
							"dec  a", "", 0,
							"ld   a, ", "", OCF_DATA8,
							"ccf", "", 0,
							"ld   b, b", "", 0,		//0x40
							"ld   b, c", "", 0,
							"ld   b, d", "", 0,
							"ld   b, e", "", 0,
							"ld   b, h", "", 0,
							"ld   b, l", "", 0,
							"ld   b, [hl]", "", OCF_ADDRESS_HL,
							"ld   b, a", "", 0,
							"ld   c, b", "", 0,		//0x48
							"ld   c, c", "", 0,
							"ld   c, d", "", 0,
							"ld   c, e", "", 0,
							"ld   c, h", "", 0,
							"ld   c, l", "", 0,
							"ld   c, [hl]", "", OCF_ADDRESS_HL,
							"ld   c, a", "", 0,
							"ld   d, b", "", 0,		//0x50
							"ld   d, c", "", 0,
							"ld   d, d", "", 0,
							"ld   d, e", "", 0,
							"ld   d, h", "", 0,
							"ld   d, l", "", 0,
							"ld   d, [hl]", "", OCF_ADDRESS_HL,
							"ld   d, a", "", 0,
							"ld   e, b", "", 0,		//0x58
							"ld   e, c", "", 0,
							"ld   e, d", "", 0,
							"ld   e, e", "", 0,
							"ld   e, h", "", 0,
							"ld   e, l", "", 0,
							"ld   e, [hl]", "", OCF_ADDRESS_HL,
							"ld   e, a", "", 0,
							"ld   h, b", "", 0,		//0x60
							"ld   h, c", "", 0,
							"ld   h, d", "", 0,
							"ld   h, e", "", 0,
							"ld   h, h", "", 0,
							"ld   h, l", "", 0,
							"ld   h, [hl]", "", OCF_ADDRESS_HL,
							"ld   h, a", "", 0,
							"ld   l, b", "", 0,		//0x68
							"ld   l, c", "", 0,
							"ld   l, d", "", 0,
							"ld   l, e", "", 0,
							"ld   l, h", "", 0,
							"ld   l, l", "", 0,
							"ld   l, [hl]", "", OCF_ADDRESS_HL,
							"ld   l, a", "", 0,
							"ld   [hl], b", "", OCF_ADDRESS_HL,	//0x70
							"ld   [hl], c", "", OCF_ADDRESS_HL,
							"ld   [hl], d", "", OCF_ADDRESS_HL,
							"ld   [hl], e", "", OCF_ADDRESS_HL,
							"ld   [hl], h", "", OCF_ADDRESS_HL,
							"ld   [hl], l", "", OCF_ADDRESS_HL,
							"halt", "", 0,
							"ld   [hl], a", "", OCF_ADDRESS_HL,
							"ld   a, b", "", 0,		//0x78
							"ld   a, c", "", 0,
							"ld   a, d", "", 0,
							"ld   a, e", "", 0,
							"ld   a, h", "", 0,
							"ld   a, l", "", 0,
							"ld   a, [hl]", "", OCF_ADDRESS_HL,
							"ld   a, a", "", 0,
							"add  a, b", "", 0,		//0x80
							"add  a, c", "", 0,
							"add  a, d", "", 0,
							"add  a, e", "", 0,
							"add  a, h", "", 0,
							"add  a, l", "", 0,
							"add  a, [hl]", "", OCF_ADDRESS_HL,
							"add  a, a", "", 0,
							"adc  a, b", "", 0,		//0x88
							"adc  a, c", "", 0,
							"adc  a, d", "", 0,
							"adc  a, e", "", 0,
							"adc  a, h", "", 0,
							"adc  a, l", "", 0,
							"adc  a, [hl]", "", OCF_ADDRESS_HL,
							"adc  a, a", "", 0,
							"sub  a, b", "", 0,		//0x90
							"sub  a, c", "", 0,
							"sub  a, d", "", 0,
							"sub  a, e", "", 0,
							"sub  a, h", "", 0,
							"sub  a, l", "", 0,
							"sub  a, [hl]", "", OCF_ADDRESS_HL,
							"sub  a, a", "", 0,
							"sbc  a, b", "", 0,		//0x98
							"sbc  a, c", "", 0,
							"sbc  a, d", "", 0,
							"sbc  a, e", "", 0,
							"sbc  a, h", "", 0,
							"sbc  a, l", "", 0,
							"sbc  a, [hl]", "", OCF_ADDRESS_HL,
							"sbc  a, a", "", 0,
							"and  b", "", 0,		//0xA0
							"and  c", "", 0,
							"and  d", "", 0,
							"and  e", "", 0,
							"and  h", "", 0,
							"and  l", "", 0,
							"and  [hl]", "", OCF_ADDRESS_HL,
							"and  a", "", 0,
							"xor  b", "", 0,		//0xA8
							"xor  c", "", 0,
							"xor  d", "", 0,
							"xor  e", "", 0,
							"xor  h", "", 0,
							"xor  l", "", 0,
							"xor  [hl]", "", OCF_ADDRESS_HL,
							"xor  a", "", 0,
							"or   b", "", 0,		//0xB0
							"or   c", "", 0,
							"or   d", "", 0,
							"or   e", "", 0,
							"or   h", "", 0,
							"or   l", "", 0,
							"or   [hl]", "", OCF_ADDRESS_HL,
							"or   a", "", 0,
							"cp   b", "", 0,		//0xB8
							"cp   c", "", 0,
							"cp   d", "", 0,
							"cp   e", "", 0,
							"cp   h", "", 0,
							"cp   l", "", 0,
							"cp   [hl]", "", OCF_ADDRESS_HL,
							"cp   a", "", 0,
							"ret  nz", "", OCF_ADDRESS_SP | OCF_COND_NZ,	//0xC0
							"pop  bc", "", OCF_ADDRESS_SP,
							"jp   nz, ", "", OCF_DATA16 | OCF_JUMP | OCF_COND_NZ,
							"jp   ", "", OCF_DATA16 | OCF_JUMP,
							"call nz, ", "", OCF_DATA16 | OCF_JUMP | OCF_CALL | OCF_COND_NZ,
							"push bc", "", 0,
							"add  a, ", "", OCF_DATA8,
							"rst  00", "", OCF_CALL | OCF_RST00,
							"ret  z", "", OCF_ADDRESS_SP | OCF_COND_Z,		//0xC8
							"ret", "", OCF_ADDRESS_SP,
							"jp   z, ", "", OCF_DATA16 | OCF_JUMP | OCF_COND_Z,
							"", "", OCF_DATA8,
							"call z, ", "", OCF_DATA16 | OCF_JUMP | OCF_CALL | OCF_COND_Z,
							"call ", "", OCF_DATA16 | OCF_JUMP | OCF_CALL,
							"adc  a, ", "", OCF_DATA8,
							"rst  08", "", OCF_CALL | OCF_RST08,
							"ret  nc", "", OCF_ADDRESS_SP | OCF_COND_NC,	//0xD0
							"pop  de", "", OCF_ADDRESS_SP,
							"jp   nc, ", "", OCF_DATA16 | OCF_JUMP | OCF_COND_NC,
							"Undefined", "", 0,
							"call nc, ", "", OCF_DATA16 | OCF_JUMP | OCF_CALL | OCF_COND_NC,
							"push de", "", 0,
							"sub  a, ", "", OCF_DATA8,
							"rst  10", "", OCF_CALL | OCF_RST10,
							"ret  c", "", OCF_ADDRESS_SP | OCF_COND_C,		//0xD8
							"reti", "", OCF_ADDRESS_SP,
							"jp   c, ", "", OCF_DATA16 | OCF_JUMP | OCF_COND_C,
							"Undefined", "", 0,
							"call c, ", "", OCF_DATA16 | OCF_JUMP | OCF_CALL | OCF_COND_C,
							"Undefined", "", 0,
							"sbc  a, ", "", OCF_DATA8,
							"rst  18", "", OCF_CALL | OCF_RST18,
							"ld   [", "], a", OCF_DATA8 | OCF_ADDRESS,	//0xE0
							"pop  hl", "", OCF_ADDRESS_SP,
							"ld   [FF00 + c], a", "", OCF_ADDRESS_C,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"push hl", "", 0,
							"and  ", "", OCF_DATA8,
							"rst  20", "", OCF_CALL | OCF_RST20,
							"add  sp, ", "", OCF_DATA8,		//0xE8
							"jp   hl", "", OCF_REG_HL | OCF_JUMP,
							"ld   [", "], a", OCF_DATA16 | OCF_ADDRESS,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"xor  ", "", OCF_DATA8,
							"rst  28", "", OCF_CALL | OCF_RST28,
							"ld   a, [", "]", OCF_DATA8 | OCF_ADDRESS,	//0xF0
							"pop  af", "", OCF_ADDRESS_SP,
							"ld   a, [FF00 + c]", "", OCF_ADDRESS_C,
							"di", "", 0,
							"Undefined", "", 0,
							"push af", "", 0,
							"or   ", "", OCF_DATA8,
							"rst  30", "", OCF_CALL | OCF_RST30,
							"ld   hl, sp + ", "", OCF_DATA8,//0xF8
							"ld   sp, hl", "", 0,
							"ld   a, [", "]", OCF_DATA16 | OCF_ADDRESS,
							"ei", "", 0,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"cp   ", "", OCF_DATA8,
							"rst  38", "", OCF_CALL | OCF_RST38
						};



const char OpCodeCBNames[32][9] =
{
	"rlc  ",
	"rrc  ",
	"rl   ",
	"rr   ",
	"sla  ",
	"sra  ",
	"swap ",
	"srl  ",
	"bit  0, ",
	"bit  1, ",
	"bit  2, ",
	"bit  3, ",
	"bit  4, ",
	"bit  5, ",
	"bit  6, ",
	"bit  7, ",
	"res  0, ",
	"res  1, ",
	"res  2, ",
	"res  3, ",
	"res  4, ",
	"res  5, ",
	"res  6, ",
	"res  7, ",
	"set  0, ",
	"set  1, ",
	"set  2, ",
	"set  3, ",
	"set  4, ",
	"set  5, ",
	"set  6, ",
	"set  7, "
};



const char OpCodeCBArgument[8][5] =
{
	"b",
	"c",
	"d",
	"e",
	"h",
	"l",
	"[hl]",
	"a",
};



char *ToHex(unsigned int n, BOOL Word)
{
	if (Word)
	{
		NumBuffer[0] = ((n & 0xFFFF) >> 12) + 48;
		if (NumBuffer[0] > 57)
		{
			NumBuffer[0] += 7;
		}
		NumBuffer[1] = (((n & 0xFFFF) >> 8) & 0xF) + 48;
		if (NumBuffer[1] > 57)
		{
			NumBuffer[1] += 7;
		}
	}
	NumBuffer[Word ? 2 : 0] = ((n >> 4) & 0xF) + 48;
	if (NumBuffer[Word ? 2 : 0] > 57)
	{
		NumBuffer[Word ? 2 : 0] += 7;
	}
	NumBuffer[Word ? 3 : 1] = (n & 0xF) + 48;
	if (NumBuffer[Word ? 3 : 1] > 57)
	{
		NumBuffer[Word ? 3 : 1] += 7;
	}

	return NumBuffer;
}



LRESULT CALLBACK TileZoomWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	DWORD			TileNo, PaletteNo;
	RECT			rct;
	HBRUSH			hBrush[4];


	switch (uMsg)
	{
	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoys.GetActive(true))
			{
				TileNo = GetWindowLong(hWin, GWL_USERDATA);
				if (TileNo & 0x80000000)
				{
					pGameBoy = NULL;
				}
				else
				{
					PaletteNo = (TileNo >> 16) & 0x1F;
					TileNo &= 0x3FFF;
				}
			}


			if (pGameBoy)
			{
				if (pGameBoy->Flags & GB_ROM_COLOR && PaletteNo <= 15)
				{
					if (PaletteNo <= 7)
					{
						hBrush[0] = CreateSolidBrush((((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] & 0x1F) << 3));
						hBrush[1] = CreateSolidBrush((((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] & 0x1F) << 3));
						hBrush[2] = CreateSolidBrush((((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] & 0x1F) << 3));
						hBrush[3] = CreateSolidBrush((((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] & 0x1F) << 3));
					}
					else
					{
						hBrush[0] = (HBRUSH)GetStockObject(NULL_BRUSH);
						hBrush[1] = CreateSolidBrush((((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] & 0x1F) << 3));
						hBrush[2] = CreateSolidBrush((((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] & 0x1F) << 3));
						hBrush[3] = CreateSolidBrush((((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] >> 10) & 0x1F) << 19) | (((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] >> 5) & 0x1F) << 11) | ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] & 0x1F) << 3));
					}
				}
				else
				{
					switch (PaletteNo & 3)
					{
					case 1:
						hBrush[0] = (HBRUSH)GetStockObject(NULL_BRUSH);
						hBrush[1] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 2) & 3]);
						hBrush[2] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 4) & 3]);
						hBrush[3] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 6) & 3]);
						break;

					case 2:
						hBrush[0] = (HBRUSH)GetStockObject(NULL_BRUSH);
						hBrush[1] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 2) & 3]);
						hBrush[2] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 4) & 3]);
						hBrush[3] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 6) & 3]);
						break;

					default:
						hBrush[0] = CreateSolidBrush(GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3]);
						hBrush[1] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3]);
						hBrush[2] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3]);
						hBrush[3] = CreateSolidBrush(GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3]);
						break;
					}
				}
			}
			else
			{
				hBrush[0] = CreateSolidBrush(0xFFFFFF);
			}

			for (rct.top = 0; rct.top < 8 * 9; rct.top += 9)
			{
				rct.bottom = rct.top + 8;

				if (pGameBoy)
				{
					rct.left = 0;
					rct.right = 8;

					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 7) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 6) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 6) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 5) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 5) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 4) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 4) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 3) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 3) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 2) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 2) & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] >> 1) & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[((pGameBoy->MEM_VRAM[TileNo] >> 1) & 1) | (pGameBoy->MEM_VRAM[TileNo + 1] & 2)]);
					rct.left += 9;
					rct.right += 9;
					FillRect(Paint.hdc, &rct, hBrush[(pGameBoy->MEM_VRAM[TileNo] & 1) | ((pGameBoy->MEM_VRAM[TileNo + 1] << 1) & 2)]);
				}
				else
				{
					for (rct.left = 0; rct.left < 8 * 9; rct.left += 9)
					{
						rct.right = rct.left + 8;

						FillRect(Paint.hdc, &rct, hBrush[0]);
					}
				}

				TileNo += 2;
			}

			DeleteObject(hBrush[0]);
			if (pGameBoy)
			{
				DeleteObject(hBrush[1]);
				DeleteObject(hBrush[2]);
				DeleteObject(hBrush[3]);

				pGameBoy->Release();
			}


			EndPaint(hWin, &Paint);
		}
		return 0;
	}

	return DefWindowProc(hWin, uMsg, wParam, lParam);
}



LRESULT CALLBACK TileMapChildWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	BYTE			TileY;
	WORD			pMapTile, pTile;
	DWORD			Color1, Color2;
	BITMAPINFO		bmi;
	HBITMAP			hBitmap[8], hOldBitmap[8];
	HPEN			hPen, hOldPen;
	DWORD			pBitmap[8];
	HDC				hdc[8];
	RGBQUAD			Palette[4];
	DWORD			PaletteNo;
	RECT			rct;
	SCROLLINFO		si;
	long			x, y, x2, y2, x3;
	DWORD			TileNo;
	TRACKMOUSEEVENT	tme;
	long			SCX, SCXr, SCXr2, SCY, SCYb, SCYb2, WX, WY;


	switch (uMsg)
	{
	case WM_MOUSELEAVE:
		SendMessage(hTileMap, WM_APP_HOVERTILE, 0, 0x80000000);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWin;
		TrackMouseEvent(&tme);

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nPos + LOWORD(lParam);
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nPos + HIWORD(lParam);

		if (x < 5 || (unsigned)x > 80 + TileMap.Zoom * 512 || ((unsigned)x > 36 + TileMap.Zoom * 256 && (unsigned)x < 49 + TileMap.Zoom * 256) || y < 5 || (unsigned)y > 36 + TileMap.Zoom * 256)
		{
			SendMessage(hTileMap, WM_APP_HOVERTILE, 0, 0x80000000);
			return 0;
		}

		if ((unsigned)x > TileMap.Zoom * 256 + 44)
		{
			TileNo = 0x400;
			x -= TileMap.Zoom * 256 + 44;
		}
		else
		{
			TileNo = 0;
		}

		TileNo = 0x1800 + TileNo + (x - 5) / (TileMap.Zoom * 8 + 1) + 32 * ((y - 5) / (TileMap.Zoom * 8 + 1));
		SendMessage(hTileMap, uMsg == WM_MOUSEMOVE ? WM_APP_HOVERTILE : WM_APP_SELECTTILE, 0, TileNo);

		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(hWin, SB_HORZ, &si);
			x = si.nPos;
			GetScrollInfo(hWin, SB_VERT, &si);
			y = si.nPos;

			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoys.GetActive(true))
			{
				ZeroMemory(&bmi, sizeof(bmi));
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = 8;
				bmi.bmiHeader.biHeight = -8;
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 4;
				bmi.bmiHeader.biCompression = BI_RGB;

				if (pGameBoy->Flags & GB_ROM_COLOR)
				{
					for (PaletteNo = 0; PaletteNo < 8; PaletteNo++)
					{
						hBitmap[PaletteNo] = CreateDIBSection(Paint.hdc, &bmi, DIB_PAL_COLORS, (void **)&pBitmap[PaletteNo], NULL, 0);
						hdc[PaletteNo] = CreateCompatibleDC(Paint.hdc);
						hOldBitmap[PaletteNo] = (HBITMAP)SelectObject(hdc[PaletteNo], hBitmap[PaletteNo]);

						Palette[0].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 10) & 0x1F) << 3;
						Palette[0].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 5) & 0x1F) << 3;
						Palette[0].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8] & 0x1F) << 3;
						Palette[1].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 10) & 0x1F) << 3;
						Palette[1].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 5) & 0x1F) << 3;
						Palette[1].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] & 0x1F) << 3;
						Palette[2].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 10) & 0x1F) << 3;
						Palette[2].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 5) & 0x1F) << 3;
						Palette[2].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] & 0x1F) << 3;
						Palette[3].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 10) & 0x1F) << 3;
						Palette[3].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 5) & 0x1F) << 3;
						Palette[3].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] & 0x1F) << 3;

						SetDIBColorTable(hdc[PaletteNo], 0, 4, Palette);
					}
				}
				else
				{
					hBitmap[0] = CreateDIBSection(Paint.hdc, &bmi, DIB_PAL_COLORS, (void **)&pBitmap[0], NULL, 0);
					hdc[0] = CreateCompatibleDC(Paint.hdc);
					hOldBitmap[0] = (HBITMAP)SelectObject(hdc[0], hBitmap[0]);

					Palette[0].rgbBlue = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
					Palette[0].rgbGreen = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
					Palette[0].rgbRed = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
					Palette[1].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
					Palette[1].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
					Palette[1].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
					Palette[2].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
					Palette[2].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
					Palette[2].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
					Palette[3].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
					Palette[3].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
					Palette[3].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];

					SetDIBColorTable(hdc[0], 0, 4, Palette);
				}

				if (pGameBoy->MEM_CPU[0x8F40] & 0x20)
				{
					WX = pGameBoy->MEM_CPU[0x8F4B] >= 167 ? 160 : pGameBoy->MEM_CPU[0x8F4B] <= 7 ? 0 : 168 - pGameBoy->MEM_CPU[0x8F4B];
					WY = pGameBoy->MEM_CPU[0x8F4A] >= 144 ? 144 : pGameBoy->MEM_CPU[0x8F4A];
				}
				else
				{
					WX = 160;
					WY = 144;
				}
				if (pGameBoy->MEM_CPU[0x8F40] & 1 && !(WX == 0 && WY == 0))
				{
					if (pGameBoy->MEM_CPU[0x8F40] & 0x08)
					{
						x2 = 44 + 256 * TileMap.Zoom;
					}
					else
					{
						x2 = 0;
					}

					hPen = CreatePen(PS_SOLID, 0, RGB(0xFF, 0x00, 0x00));
					hOldPen = (HPEN)SelectObject(Paint.hdc, hPen);

					SCX = 4 + pGameBoy->MEM_CPU[0x8F43] * TileMap.Zoom + pGameBoy->MEM_CPU[0x8F43] / 8 - x;
					SCXr = SCX + 160 * TileMap.Zoom + 20;
					if (SCXr > (signed)(5 + 31 + 32 * 8 * TileMap.Zoom - x - x))
					{
						SCXr -= 32 + 32 * 8 * TileMap.Zoom;
					}
					SCXr2 = SCX + WX * TileMap.Zoom + WX / 8;
					if (SCXr2 > (signed)(5 + 31 + 32 * 8 * TileMap.Zoom - x - x))
					{
						SCXr2 -= 32 + 32 * 8 * TileMap.Zoom;
					}
					SCY = 4 + pGameBoy->MEM_CPU[0x8F42] * TileMap.Zoom + pGameBoy->MEM_CPU[0x8F42] / 8 - y;
					SCYb = SCY + 144 * TileMap.Zoom + 18;
					if (SCYb > (signed)(5 + 31 + 32 * 8 * TileMap.Zoom))
					{
						SCYb -= 31 + 32 * 8 * TileMap.Zoom;
					}
					SCYb2 = SCY + WY * TileMap.Zoom + WY / 8;
					if (SCYb2 > (signed)(5 + 31 + 32 * 8 * TileMap.Zoom))
					{
						SCYb2 -= 31 + 32 * 8 * TileMap.Zoom;
					}
					if (WX == 0)
					{
						SCYb = SCYb2;
					}

					SCX += x2;
					SCXr += x2;
					SCXr2 += x2;

					MoveToEx(Paint.hdc, SCX, SCY, NULL);
					if (SCXr < SCX)
					{
						LineTo(Paint.hdc, x2 + (37 + 256 * TileMap.Zoom) - x, SCY);
						MoveToEx(Paint.hdc, x2 + 4 - x, SCY, NULL);
					}
					LineTo(Paint.hdc, SCXr, SCY);
					if (SCYb2 < SCY)
					{
						LineTo(Paint.hdc, SCXr, (37 + 256 * TileMap.Zoom) - y);
						MoveToEx(Paint.hdc, SCXr, 4 - y, NULL);
					}
					LineTo(Paint.hdc, SCXr, SCYb2);
					if (SCXr != SCXr2)
					{
						if (SCXr2 > SCXr)
						{
							LineTo(Paint.hdc, x2 + 3 - x, SCYb2);
							MoveToEx(Paint.hdc, x2 + (37 + 256 * TileMap.Zoom) - x, SCYb2, NULL);
						}
						LineTo(Paint.hdc, SCXr2, SCYb2);
					}
					if (SCYb < SCYb2)
					{
						LineTo(Paint.hdc, SCXr2, (37 + 256 * TileMap.Zoom) - y);
						MoveToEx(Paint.hdc, SCXr2, 4 - y, NULL);
					}
					LineTo(Paint.hdc, SCXr2, SCYb);
					if (SCX > SCXr2)
					{
						LineTo(Paint.hdc, x2 + 3 - x, SCYb);
						MoveToEx(Paint.hdc, x2 + (36 + 256 * TileMap.Zoom) - x, SCYb, NULL);
					}
					LineTo(Paint.hdc, SCX, SCYb);
					if (SCY > SCYb)
					{
						LineTo(Paint.hdc, SCX, 3 - y);
						MoveToEx(Paint.hdc, SCX, (36 + 256 * TileMap.Zoom) - y, NULL);
					}
					LineTo(Paint.hdc, SCX, SCY);

					SelectObject(Paint.hdc, hOldPen);
					DeleteObject(hPen);
				}

				if (pGameBoy->MEM_CPU[0x8F40] & 0x20)
				{
					if (pGameBoy->MEM_CPU[0x8F40] & 0x40)
					{
						x2 = 44 + 256 * TileMap.Zoom;
					}
					else
					{
						x2 = 0;
					}
					if (pGameBoy->MEM_CPU[0x8F4B] <= 7)
					{
						x2 += (7 - pGameBoy->MEM_CPU[0x8F4B]) * TileMap.Zoom + 4 - x;
					}
					else
					{
						x2 += 4 - x;
					}
					if (pGameBoy->MEM_CPU[0x8F4A] >= 144)
					{
						y2 = 144 * TileMap.Zoom + 20 + 4 - y;
					}
					else
					{
						y2 = (144 - pGameBoy->MEM_CPU[0x8F4A]) * TileMap.Zoom + (144 - pGameBoy->MEM_CPU[0x8F4A]) / 8 + 4 - y;
					}

					hPen = CreatePen(PS_SOLID, 0, RGB(0x00, 0xFF, 0x00));
					hOldPen = (HPEN)SelectObject(Paint.hdc, hPen);

					MoveToEx(Paint.hdc, x2, 4 - y, NULL);
					LineTo(Paint.hdc, x2 + 160 * TileMap.Zoom + 20, 4 - y);
					LineTo(Paint.hdc, x2 + 160 * TileMap.Zoom + 20, y2);
					LineTo(Paint.hdc, x2, y2);
					LineTo(Paint.hdc, x2, 4 - y);

					SelectObject(Paint.hdc, hOldPen);
					DeleteObject(hPen);
				}
			}

			PaletteNo = 0;
			pMapTile = 0x1800;
			while (pMapTile < 0x2000)
			{
				for (y2 = 5 - y; y2 < (signed)((8 * TileMap.Zoom + 1) * 32); y2 += 8 * TileMap.Zoom + 1)
				{
					if (Paint.rcPaint.top < (signed)(y2 + 8 * TileMap.Zoom) && Paint.rcPaint.bottom > y2)
					{
						for (x2 = (signed)(5 - x + (pMapTile >= 0x1C00 ? 44 + TileMap.Zoom * 256 : 0)), x3 = x2 + 31 + 256 * TileMap.Zoom; x2 < x3; x2 += 8 * TileMap.Zoom + 1)
						{
							if (Paint.rcPaint.left < (signed)(x2 + 8 * TileMap.Zoom) && Paint.rcPaint.right > x2)
							{
								if (pGameBoy)
								{
									if (pGameBoy->MEM_CPU[0x8F40] & 0x10)
									{
										pTile = pGameBoy->MEM_VRAM[pMapTile] << 4;
									}
									else
									{
										pTile = 0x1000 + 16 * (signed char)pGameBoy->MEM_VRAM[pMapTile];
									}

									TileY = 0;
									while (TileY < 8)
									{
										if (pGameBoy->Flags & GB_ROM_COLOR)
										{
											PaletteNo = pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 7;
											Color1 = pGameBoy->MEM_VRAM[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 8) ? 0x2000 : 0) + pTile + (TileY << 1)];
											Color2 = pGameBoy->MEM_VRAM[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 8) ? 0x2000 : 0) + pTile + (TileY << 1) + 1];
											((DWORD *)(pBitmap[PaletteNo]))[(pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 0x40) ? 7 - TileY : TileY] = (pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 0x20) ? TileToBmpFlipX(Color1, Color2) : TileToBmp(Color1, Color2);
										}
										else
										{
											Color1 = pGameBoy->MEM_VRAM[pTile + (TileY << 1)];
											Color2 = pGameBoy->MEM_VRAM[pTile + (TileY << 1) + 1];
											((DWORD *)(pBitmap[0]))[TileY] = TileToBmp(Color1, Color2);
										}

										TileY++;
									}

									if (TileMap.Zoom == 1)
									{
										BitBlt(Paint.hdc, x2, y2, 8, 8, hdc[PaletteNo], 0, 0, SRCCOPY);
									}
									else
									{
										StretchBlt(Paint.hdc, x2, y2, TileMap.Zoom * 8, TileMap.Zoom * 8, hdc[PaletteNo], 0, 0, 8, 8, SRCCOPY);
									}
								}
								else
								{
									rct.left = x2;
									rct.right = x2 + TileMap.Zoom * 8;
									rct.top = y2;
									rct.bottom = y2 + TileMap.Zoom * 8;
									FillRect(Paint.hdc, &rct, (HBRUSH)GetStockObject(WHITE_BRUSH));
								}
							}

							pMapTile++;
						}
					}
					else
					{
						pMapTile += 32;
					}
				}
			}

			if (pGameBoy)
			{
				if (pGameBoy->Flags & GB_ROM_COLOR)
				{
					for (PaletteNo = 0; PaletteNo < 8; PaletteNo++)
					{
						SelectObject(hdc[PaletteNo], hOldBitmap[PaletteNo]);
						DeleteObject(hBitmap[PaletteNo]);
						DeleteDC(hdc[PaletteNo]);
					}
				}
				else
				{
					SelectObject(hdc[0], hOldBitmap[0]);
					DeleteObject(hBitmap[0]);
					DeleteDC(hdc[0]);
				}
			}

			if (!(SelectedTile & 0x80000000))
			{
				if (SelectedTile & 0x0400)
				{
					x2 = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 48 + 256 * TileMap.Zoom - x;
				}
				else
				{
					x2 = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 4 - x;
				}
				y2 = (TileMap.Zoom * 8 + 1) * ((SelectedTile >> 5) & 0x1F) + 4 - y;

				hPen = CreatePen(PS_SOLID, 0, RGB(0x00, 0x00, 0xFF));
				hOldPen = (HPEN)SelectObject(Paint.hdc, hPen);

				MoveToEx(Paint.hdc, x2, y2, NULL);
				LineTo(Paint.hdc, x2 + 8 * TileMap.Zoom + 1, y2);
				LineTo(Paint.hdc, x2 + 8 * TileMap.Zoom + 1, y2 + 8 * TileMap.Zoom + 1);
				LineTo(Paint.hdc, x2, y2 + 8 * TileMap.Zoom + 1);
				LineTo(Paint.hdc, x2, y2);

				SelectObject(Paint.hdc, hOldPen);
				DeleteObject(hPen);
			}

			if (pGameBoy)
			{
				pGameBoy->Release();
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SIZE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE;
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nMax;
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nMax;
		GetWindowRect(hWin, &rct);
		si.nPage = rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE) /*- GetSystemMetrics(SM_CXVSCROLL)*/;
		if (rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYSMCAPTION) < y + 1)
		{
			si.nPage -= GetSystemMetrics(SM_CXVSCROLL);
			x += GetSystemMetrics(SM_CXVSCROLL);
		}
		else
		{
			if (rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE) < x + 1 && rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYSMCAPTION) - GetSystemMetrics(SM_CYVSCROLL) < y + 1)
			{
				si.nPage -= GetSystemMetrics(SM_CXVSCROLL);
				x += GetSystemMetrics(SM_CXVSCROLL);
			}
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.fMask = SIF_PAGE | SIF_POS;
		GetScrollInfo(hWin, SB_VERT, &si);
		si.nPage = rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) /*- GetSystemMetrics(SM_CYHSCROLL)*/ - GetSystemMetrics(SM_CYSMCAPTION);
		if (rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE) < x + 1)
		{
			si.nPage -= GetSystemMetrics(SM_CYHSCROLL);
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		InvalidateRect(hWin, NULL, true);
		return 0;

	case WM_VSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN:
			si.nPos += 10;
			break;
		case SB_LINEUP:
			si.nPos -= 10;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, 0, y - si.nPos, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_HSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT:
			si.nPos += 10;
			break;
		case SB_LINELEFT:
			si.nPos -= 10;
			break;
		case SB_PAGERIGHT:
			si.nPos += si.nPage;
			break;
		case SB_PAGELEFT:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, x - si.nPos, 0, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_CREATE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
		si.nPos = 0;
		si.nMin = 0;
		si.nMax = 77 + TileMap.Zoom * 512;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.nMax = 20 + TileMap.Zoom * 256;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;
	}

	return DefWindowProc(hWin, uMsg, wParam, lParam);
}



HWND		hTileMapChild, hTileZoom, hFlipX, hFlipY, hPriority, hTileNo, hPaletteNo, hTileAddress;
HWND		hTileData, hBGEnabled, hBGAddress, hWNDEnabled, hWNDAddress;

BOOL		LockTileMapInfo = false;



void TileMapUpdateInfo()
{
	CGameBoy		*pGameBoy;
	DWORD			TileMapNo, TileNo;
	char			szTileAddress[8];


	if (pGameBoy = GameBoys.GetActive(true))
	{
		if (HoverTile & 0x80000000)
		{
			TileMapNo = SelectedTile;
		}
		else
		{
			TileMapNo = HoverTile;
		}
		if (!(TileMapNo & 0x80000000))
		{
			LockTileMapInfo = true;

			if (pGameBoy->MEM_CPU[0x8F40] & 0x10)
			{
				if (pGameBoy->Flags & GB_ROM_COLOR)
				{
					TileNo = ((pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 7) << 16) | ((pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 8) << 11) | (16 * pGameBoy->MEM_VRAM[TileMapNo]);
				}
				else
				{
					TileNo = 16 * pGameBoy->MEM_VRAM[TileMapNo];
				}
			}
			else
			{
				if (pGameBoy->Flags & GB_ROM_COLOR)
				{
					TileNo = ((pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 7) << 16) | ((pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 8) << 11) | (0x1000 + 16 * (signed char)pGameBoy->MEM_VRAM[TileMapNo]);
				}
				else
				{
					TileNo = 0x1000 + 16 * (signed char)pGameBoy->MEM_VRAM[TileMapNo];
				}
			}
			if (pGameBoy->Flags & GB_ROM_COLOR)
			{
				szTileAddress[0] = pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 8 ? '1' : '0';
			}
			else
			{
				szTileAddress[0] = '0';
			}
			szTileAddress[1] = ':';
			*(DWORD *)&szTileAddress[2] = *(DWORD *)ToHex(0x8000 + (TileNo & 0x1FF0), true);
			szTileAddress[6] = '\0';
			SetWindowText(hTileAddress, szTileAddress);
			*(WORD *)&szTileAddress[2] = *(WORD *)ToHex(pGameBoy->MEM_VRAM[TileMapNo], false);
			szTileAddress[4] = '\0';
			SetWindowText(hTileNo, szTileAddress);
			SetWindowLong(hTileZoom, GWL_USERDATA, TileNo);
			InvalidateRect(hTileZoom, NULL, true);

			if (pGameBoy->Flags & GB_ROM_COLOR)
			{
				SendMessage(hPaletteNo, CB_SETCURSEL, pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 7, 0);

				if (pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 0x20)
				{
					SendMessage(hFlipX, BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendMessage(hFlipX, BM_SETCHECK, BST_UNCHECKED, 0);
				}

				if (pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 0x40)
				{
					SendMessage(hFlipY, BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendMessage(hFlipY, BM_SETCHECK, BST_UNCHECKED, 0);
				}

				if (pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 0x80)
				{
					SendMessage(hPriority, BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendMessage(hPriority, BM_SETCHECK, BST_UNCHECKED, 0);
				}
			}

			LockTileMapInfo = false;

			EnableWindow(hTileAddress, true);
			EnableWindow(hTileNo, true);
			if (pGameBoy->Flags & GB_ROM_COLOR)
			{
				EnableWindow(hPaletteNo, true);
				EnableWindow(hFlipX, true);
				EnableWindow(hFlipY, true);
				EnableWindow(hPriority, true);
			}
			else
			{
				EnableWindow(hPaletteNo, false);
				EnableWindow(hFlipX, false);
				EnableWindow(hFlipY, false);
				EnableWindow(hPriority, false);
			}
		}
		else
		{
			SelectedTile = HoverTile = 0x80000000;
			SetWindowLong(hTileZoom, GWL_USERDATA, 0x80000000);
			InvalidateRect(hTileZoom, NULL, true);
			SetWindowText(hTileAddress, NULL);
			SetWindowText(hTileNo, NULL);
			SendMessage(hPaletteNo, CB_SETCURSEL, -1, 0);

			EnableWindow(hTileAddress, false);
			EnableWindow(hTileNo, false);
			EnableWindow(hPaletteNo, false);
			EnableWindow(hFlipX, false);
			EnableWindow(hFlipY, false);
			EnableWindow(hPriority, false);
		}

		EnableWindow(hTileData, true);
		EnableWindow(hBGEnabled, true);
		EnableWindow(hBGAddress, true);
		EnableWindow(hWNDEnabled, true);
		EnableWindow(hWNDAddress, true);

		SendMessage(hTileData, CB_SETCURSEL, pGameBoy->MEM_CPU[0x8F40] & 0x10 ? 0 : 1, 0);
		SendMessage(hBGEnabled, BM_SETCHECK, pGameBoy->MEM_CPU[0x8F40] & 0x01 ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(hBGAddress, CB_SETCURSEL, pGameBoy->MEM_CPU[0x8F40] & 0x08 ? 1 : 0, 0);
		SendMessage(hWNDEnabled, BM_SETCHECK, pGameBoy->MEM_CPU[0x8F40] & 0x20 ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(hWNDAddress, CB_SETCURSEL, pGameBoy->MEM_CPU[0x8F40] & 0x40 ? 1 : 0, 0);

		pGameBoy->Release();
	}
	else
	{
		SelectedTile = HoverTile = 0x80000000;
		SetWindowLong(hTileZoom, GWL_USERDATA, 0x80000000);
		InvalidateRect(hTileZoom, NULL, true);
		SetWindowText(hTileAddress, NULL);
		SetWindowText(hTileNo, NULL);
		SendMessage(hPaletteNo, CB_SETCURSEL, -1, 0);
		SendMessage(hFlipX, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hFlipY, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hPriority, BM_SETCHECK, BST_UNCHECKED, 0);

		EnableWindow(hTileAddress, false);
		EnableWindow(hTileNo, false);
		EnableWindow(hPaletteNo, false);
		EnableWindow(hFlipX, false);
		EnableWindow(hFlipY, false);
		EnableWindow(hPriority, false);

		SendMessage(hTileData, CB_SETCURSEL, -1, 0);
		SendMessage(hBGEnabled, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hBGAddress, CB_SETCURSEL, -1, 0);
		SendMessage(hWNDEnabled, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(hWNDAddress, CB_SETCURSEL, -1, 0);

		EnableWindow(hTileData, false);
		EnableWindow(hBGEnabled, false);
		EnableWindow(hBGAddress, false);
		EnableWindow(hWNDEnabled, false);
		EnableWindow(hWNDAddress, false);
	}
}



LRESULT CALLBACK TileMapWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy	*pGameBoy;
	RECT		rct, Rect2;
	DWORD		TileMapNo;
	char		szTileAddress[8], szBuffer[0x100];
	SCROLLINFO	si;
	POINT		Pt;
	long		x, y;


	switch (uMsg)
	{
	case WM_APP_SELECTTILE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		GetScrollInfo(hTileMapChild, SB_HORZ, &si);
		x = si.nPos;
		GetScrollInfo(hTileMapChild, SB_VERT, &si);
		y = si.nPos;

		//Erase last selection
		if (!(SelectedTile & 0x80000000))
		{
			if (SelectedTile & 0x0400)
			{
				rct.left = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 48 + 256 * TileMap.Zoom - x;
			}
			else
			{
				rct.left = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 4 - x;
			}
			rct.top = (TileMap.Zoom * 8 + 1) * ((SelectedTile >> 5) & 0x1F) + 4 - y;
			rct.right = rct.left + TileMap.Zoom * 8 + 2;
			rct.bottom = rct.top + TileMap.Zoom * 8 + 2;
			SelectedTile = 0x80000000;
			InvalidateRect(hTileMapChild, &rct, true);
			UpdateWindow(hTileMapChild);
		}

		SelectedTile = lParam;
		TileMapUpdateInfo();

		if (!(SelectedTile & 0x80000000))
		{
			if (SelectedTile & 0x0400)
			{
				rct.left = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 48 + 256 * TileMap.Zoom - x;
			}
			else
			{
				rct.left = (TileMap.Zoom * 8 + 1) * (SelectedTile & 0x1F) + 4 - x;
			}
			rct.top = (TileMap.Zoom * 8 + 1) * ((SelectedTile >> 5) & 0x1F) + 4 - y;
			rct.right = rct.left + TileMap.Zoom * 8 + 2;
			rct.bottom = rct.top + TileMap.Zoom * 8 + 2;
			InvalidateRect(hTileMapChild, &rct, true);
		}
		return 0;

	case WM_APP_HOVERTILE:
		HoverTile = lParam;
		TileMapUpdateInfo();
		return 0;

	case WM_PAINT:
		TileMapUpdateInfo();
		break;

	case WM_APP_SETTILE:
		if (SelectedTile & 0x80000000)
		{
			return 0;
		}
		szTileAddress[0] = lParam & 0x0200 ? '1' : '0';
		szTileAddress[1] = ':';
		*(WORD *)&szTileAddress[2] = *(WORD *)ToHex(lParam, false);
		szTileAddress[4] = '\0';
		SendMessage(hTileNo, WM_SETTEXT, 0, (LPARAM)&szTileAddress);
		return 0;

	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_TILEMAP));
		SetWindowText(hFlipX, String(IDS_WND_TILEMAP_XFLIP));
		SetWindowText(hFlipY, String(IDS_WND_TILEMAP_YFLIP));
		SetWindowText(hPriority, String(IDS_WND_TILEMAP_PRIORITY));
		SetWindowText(hBGEnabled, String(IDS_WND_TILEMAP_BACKGROUNDENABLED));
		SetWindowText(hWNDEnabled, String(IDS_WND_TILEMAP_WINDOWENABLED));
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TILEMAP_PALETTENO:
			if (LockTileMapInfo)
			{
				return 0;
			}
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				if (!(pGameBoy = GameBoys.GetActive(true)))
				{
					return 0;
				}
				if (!(pGameBoy->Flags & GB_ROM_COLOR) || pGameBoy->IsEmulating())
				{
					pGameBoy->Release();
					return 0;
				}
				if (HoverTile & 0x80000000)
				{
					TileMapNo = SelectedTile;
				}
				else
				{
					TileMapNo = HoverTile;
				}
				if (TileMapNo & 0x80000000)
				{
					pGameBoy->Release();
					return 0;
				}
				pGameBoy->MEM_VRAM[0x2000 + TileMapNo] &= ~7;
				pGameBoy->MEM_VRAM[0x2000 + TileMapNo] |= SendMessage(hPaletteNo, CB_GETCURSEL, 0, 0);
				InvalidateRect(hTileMapChild, NULL, true);
				pGameBoy->Release();
				return 0;
			}
			break;

		case ID_TILEMAP_FLIPX:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (!(pGameBoy->Flags & GB_ROM_COLOR) || pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			if (HoverTile & 0x80000000)
			{
				TileMapNo = SelectedTile;
			}
			else
			{
				TileMapNo = HoverTile;
			}
			if (TileMapNo & 0x80000000)
			{
				pGameBoy->Release();
				return 0;
			}
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] &= ~0x20;
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] |= SendMessage(hFlipX, BM_GETCHECK, 0, 0) == BST_CHECKED ? 0x20 : 0;
			InvalidateRect(hTileMapChild, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_TILEMAP_FLIPY:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (!(pGameBoy->Flags & GB_ROM_COLOR) || pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			if (HoverTile & 0x80000000)
			{
				TileMapNo = SelectedTile;
			}
			else
			{
				TileMapNo = HoverTile;
			}
			if (TileMapNo & 0x80000000)
			{
				pGameBoy->Release();
				return 0;
			}
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] &= ~0x40;
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] |= SendMessage(hFlipY, BM_GETCHECK, 0, 0) == BST_CHECKED ? 0x40 : 0;
			InvalidateRect(hTileMapChild, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_TILEMAP_PRIORITY:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (!(pGameBoy->Flags & GB_ROM_COLOR) || pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			if (HoverTile & 0x80000000)
			{
				TileMapNo = SelectedTile;
			}
			else
			{
				TileMapNo = HoverTile;
			}
			if (TileMapNo & 0x80000000)
			{
				pGameBoy->Release();
				return 0;
			}
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] &= ~0x80;
			pGameBoy->MEM_VRAM[0x2000 + TileMapNo] |= SendMessage(hPriority, BM_GETCHECK, 0, 0) == BST_CHECKED ? 0x80 : 0;
			InvalidateRect(hTileMapChild, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_TILEMAP_TILENO:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			if (HoverTile & 0x80000000)
			{
				TileMapNo = SelectedTile;
			}
			else
			{
				TileMapNo = HoverTile;
			}
			if (TileMapNo & 0x80000000)
			{
				pGameBoy->Release();
				return 0;
			}
			if (hTileNo == (HWND)lParam)
			{
				switch (HIWORD(wParam))
				{
				case EN_UPDATE:
					SendMessage(hTileNo, WM_GETTEXT, 6, (LPARAM)&szTileAddress);
					if (szTileAddress[0] < '0' || szTileAddress[0] > '1' || szTileAddress[1] != ':' || szTileAddress[4] != '\0' || (szTileAddress[0] > '0' && !(pGameBoy->Flags & GB_ROM_COLOR)))
					{
						pGameBoy->Release();
						return 0;
					}
					if (HexToNum(&szTileAddress[2]) || HexToNum(&szTileAddress[3]))
					{
						pGameBoy->Release();
						return 0;
					}
					if (pGameBoy->MEM_VRAM[TileMapNo] == ((szTileAddress[2] << 4) | szTileAddress[3]) &&
						(pGameBoy->MEM_VRAM[0x2000 + TileMapNo] & 8) == (szTileAddress[0] == '1' ? 8 : 0))
					{
						pGameBoy->Release();
						return 0;
					}
					pGameBoy->MEM_VRAM[TileMapNo] = (szTileAddress[2] << 4) | szTileAddress[3];
					pGameBoy->MEM_VRAM[0x2000 + TileMapNo] &= ~8;
					if (szTileAddress[0] == '1')
					{
						pGameBoy->MEM_VRAM[0x2000 + TileMapNo] |= 8;
					}
					InvalidateRect(hWin, NULL, true);
					pGameBoy->Release();
					return 0;
				}
			}
			pGameBoy->Release();
			break;

		case ID_TILEMAP_TILEDATA:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (!(pGameBoy = GameBoys.GetActive(true)))
				{
					return 0;
				}
				if (pGameBoy->IsEmulating())
				{
					pGameBoy->Release();
					return 0;
				}
				pGameBoy->MEM_CPU[0x8F40] &= ~0x10;
				pGameBoy->MEM_CPU[0x8F40] |= SendMessage(hTileData, CB_GETCURSEL, 0, 0) == 1 ? 0 : 0x10;
				InvalidateRect(hTileMapChild, NULL, true);
				pGameBoy->Release();
				return 0;
			}
			break;

		case ID_TILEMAP_BGADDRESS:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (!(pGameBoy = GameBoys.GetActive(true)))
				{
					return 0;
				}
				if (pGameBoy->IsEmulating())
				{
					pGameBoy->Release();
					return 0;
				}
				pGameBoy->MEM_CPU[0x8F40] &= ~0x08;
				pGameBoy->MEM_CPU[0x8F40] |= SendMessage(hBGAddress, CB_GETCURSEL, 0, 0) == 1 ? 0x08 : 0;
				InvalidateRect(hTileMapChild, NULL, true);
				pGameBoy->Release();
				return 0;
			}
			break;

		case ID_TILEMAP_BGENABLED:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			pGameBoy->MEM_CPU[0x8F40] &= ~0x01;
			pGameBoy->MEM_CPU[0x8F40] |= SendMessage(hBGEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED ? 0x01 : 0;
			InvalidateRect(hTileMapChild, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_TILEMAP_WNDADDRESS:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (!(pGameBoy = GameBoys.GetActive(true)))
				{
					return 0;
				}
				if (pGameBoy->IsEmulating())
				{
					pGameBoy->Release();
					return 0;
				}
				pGameBoy->MEM_CPU[0x8F40] &= ~0x40;
				pGameBoy->MEM_CPU[0x8F40] |= SendMessage(hWNDAddress, CB_GETCURSEL, 0, 0) == 1 ? 0x40 : 0;
				InvalidateRect(hTileMapChild, NULL, true);
				pGameBoy->Release();
				return 0;
			}
			break;

		case ID_TILEMAP_WNDENABLED:
			if (LockTileMapInfo)
			{
				return 0;
			}
			if (!(pGameBoy = GameBoys.GetActive(true)))
			{
				return 0;
			}
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			pGameBoy->MEM_CPU[0x8F40] &= ~0x20;
			pGameBoy->MEM_CPU[0x8F40] |= SendMessage(hWNDEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED ? 0x20 : 0;
			InvalidateRect(hTileMapChild, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_VIEW_ZOOM_100:
			TileMap.Zoom = 1;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 77 + 512;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_HORZ, &si, false);
			si.nMax = 20 + 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_VERT, &si, false);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 171 + 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 55 + 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_200:
			TileMap.Zoom = 2;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 77 + 2 * 512;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_HORZ, &si, false);
			si.nMax = 20 + 2 * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_VERT, &si, false);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 171 + 2 * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 55 + 2 * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_300:
			TileMap.Zoom = 3;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 77 + 3 * 512;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_HORZ, &si, false);
			si.nMax = 20 + 3 * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_VERT, &si, false);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 171 + 3 * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 55 + 3 * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_400:
			TileMap.Zoom = 4;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 77 + 4 * 512;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_HORZ, &si, false);
			si.nMax = 20 + 4 * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTileMapChild, SB_VERT, &si, false);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 171 + 4 * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 55 + 4 * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;
		}
		break;

	case WM_SIZE:
		GetClientRect(hWin, &rct);
		MoveWindow(hTileMapChild, 5, 5, rct.right - 85, rct.bottom - 10, true);
		MoveWindow(hTileZoom, rct.right - 75, 5, 71, 71, true);
		MoveWindow(hTileAddress, rct.right - 75, 79, 70, 12, true);
		MoveWindow(hTileNo, rct.right - 75, 94, 70, 20, true);
		MoveWindow(hPaletteNo, rct.right - 75, 119, 70, 200, true);
		MoveWindow(hFlipX, rct.right - 75, 143, 70, 12, true);
		MoveWindow(hFlipY, rct.right - 75, 158, 70, 12, true);
		MoveWindow(hPriority, rct.right - 75, 173, 70, 12, true);

		MoveWindow(hTileData, rct.right - 75, 200, 70, 200, true);
		MoveWindow(hBGEnabled, rct.right - 75, 225, 70, 12, true);
		MoveWindow(hBGAddress, rct.right - 75, 240, 70, 200, true);
		MoveWindow(hWNDEnabled, rct.right - 75, 265, 70, 12, true);
		MoveWindow(hWNDAddress, rct.right - 75, 280, 70, 200, true);
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		SelectedTile = HoverTile = 0x80000000;

		hTileMapChild = CreateWindowEx(WS_EX_CLIENTEDGE, "TileMapChild", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		hTileZoom = CreateWindow("TileZoom", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SetWindowLong(hTileZoom, GWL_USERDATA, 0x80000000);
		hTileAddress = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hTileAddress, WM_SETFONT, (WPARAM)hFixedFont, 0);
		hTileNo = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hTileNo, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hTileNo, GWL_ID, ID_TILEMAP_TILENO);
		hPaletteNo = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hPaletteNo, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP0");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP1");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP2");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP3");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP4");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP5");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP6");
		SendMessage(hPaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP7");
		SetWindowLong(hPaletteNo, GWL_ID, ID_TILEMAP_PALETTENO);
		hFlipX = CreateWindow("BUTTON", String(IDS_WND_TILEMAP_XFLIP), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hFlipX, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hFlipX, GWL_ID, ID_TILEMAP_FLIPX);
		hFlipY = CreateWindow("BUTTON", String(IDS_WND_TILEMAP_YFLIP), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hFlipY, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hFlipY, GWL_ID, ID_TILEMAP_FLIPY);
		hPriority = CreateWindow("BUTTON", String(IDS_WND_TILEMAP_PRIORITY), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hPriority, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hPriority, GWL_ID, ID_TILEMAP_PRIORITY);

		hTileData = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hTileData, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SendMessage(hTileData, CB_ADDSTRING, 0, (LPARAM)&"8000");
		SendMessage(hTileData, CB_ADDSTRING, 0, (LPARAM)&"8800");
		SetWindowLong(hTileData, GWL_ID, ID_TILEMAP_TILEDATA);
		hBGEnabled = CreateWindow("BUTTON", String(IDS_WND_TILEMAP_BACKGROUNDENABLED), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hBGEnabled, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hBGEnabled, GWL_ID, ID_TILEMAP_BGENABLED);
		hBGAddress = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hBGAddress, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SendMessage(hBGAddress, CB_ADDSTRING, 0, (LPARAM)&"9800");
		SendMessage(hBGAddress, CB_ADDSTRING, 0, (LPARAM)&"9C00");
		SetWindowLong(hBGAddress, GWL_ID, ID_TILEMAP_BGADDRESS);
		hWNDEnabled = CreateWindow("BUTTON", String(IDS_WND_TILEMAP_WINDOWENABLED), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hWNDEnabled, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SetWindowLong(hWNDEnabled, GWL_ID, ID_TILEMAP_WNDENABLED);
		hWNDAddress = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hWNDAddress, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SendMessage(hWNDAddress, CB_ADDSTRING, 0, (LPARAM)&"9800");
		SendMessage(hWNDAddress, CB_ADDSTRING, 0, (LPARAM)&"9C00");
		SetWindowLong(hWNDAddress, GWL_ID, ID_TILEMAP_WNDADDRESS);
		return 0;

	case WM_DESTROY:
		hTileMap = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		TileMap.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		TileMap.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		TileMap.Width = rct.right - rct.left;
		TileMap.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



LRESULT CALLBACK PalettesWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;
	BYTE			x, y;
	HBRUSH			hBrush, hOldBrush;
	DWORD			Color;
	char			c[1], szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_PALETTES));
		InvalidateRect(hWin, NULL, true);
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			pGameBoy = GameBoys.GetActive(true);

			SelectObject(Paint.hdc, GetStockObject(ANSI_FIXED_FONT));
			SetBkMode(Paint.hdc, TRANSPARENT);
			SetTextColor(Paint.hdc, 0);

			TextOut(Paint.hdc, 20, 10, String(IDS_WND_PALETTES_BACKGROUND), 10);
			TextOut(Paint.hdc, 200, 10, String(IDS_WND_PALETTES_SPRITE), 6);

			if (!pGameBoy)
			{
				Color = 0;
				hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
				hOldBrush = (HBRUSH)SelectObject(Paint.hdc, hBrush);
			}

			y = 0;
			while (y < 8)
			{
				x = 0;
				while (x < 4)
				{
					c[0] = y + 48;
					TextOut(Paint.hdc, 5, 40 + y * 40, c, 1);

					if (pGameBoy)
					{
						Color = *(WORD *)&pGameBoy->BGP[(y << 3) + (x << 1)];
						hBrush = CreateSolidBrush((((Color >> 10) & 0x1F) << 19) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 3));
						hOldBrush = (HBRUSH)SelectObject(Paint.hdc, hBrush);
					}
					Rectangle(Paint.hdc, 20 + x * 40, 40 + y * 40, 20 + x * 40 + 32, 40 + y * 40 + 16);
					if (pGameBoy)
					{
						SelectObject(Paint.hdc, hOldBrush);
						DeleteObject(hBrush);
					}

					TextOut(Paint.hdc, 20 + x * 40, 60 + y * 40, ToHex(Color, true), 4);

					x++;
				}
				y++;
			}

			y = 0;
			while (y < 8)
			{
				x = 0;
				while (x < 4)
				{
					if (pGameBoy)
					{
						Color = *(WORD *)&pGameBoy->OBP[(y << 3) + (x << 1)];
						hBrush = CreateSolidBrush((((Color >> 10) & 0x1F) << 19) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 3));
						hOldBrush = (HBRUSH)SelectObject(Paint.hdc, hBrush);
					}
					Rectangle(Paint.hdc, 200 + x * 40, 40 + y * 40, 200 + x * 40 + 32, 40 + y * 40 + 16);
					if (pGameBoy)
					{
						SelectObject(Paint.hdc, hOldBrush);
						DeleteObject(hBrush);
					}

					TextOut(Paint.hdc, 200 + x * 40, 60 + y * 40, ToHex(Color, true), 4);

					x++;
				}
				y++;
			}

			if (!pGameBoy)
			{
				SelectObject(Paint.hdc, hOldBrush);
				DeleteObject(hBrush);
			}
			else
			{
				pGameBoy->Release();
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		hPalettes = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		Palettes.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		Palettes.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		Palettes.Width = rct.right - rct.left;
		Palettes.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



HWND		hTilesChild, hTiles_TileZoom, hTiles_PaletteNo;
DWORD		Tiles_SelectedTile, Tiles_HoverTile;
BOOL		LockTilesUpdate;

LRESULT CALLBACK TilesChildWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy			*pGameBoy;
	PAINTSTRUCT			Paint;
	HPEN				hPen, hOldPen;
	SCROLLINFO			si;
	TRACKMOUSEEVENT		tme;
	BITMAPINFO			bmi;
	HBITMAP				hBitmap, hOldBitmap;
	DWORD				*pBitmap[8];
	HDC					hdc;
	RGBQUAD				Palette[4];
	DWORD				PaletteNo;
	WORD				Bank;
	WORD				pTile;
	BYTE				TileY;
	BYTE				Color1, Color2;
	DWORD				x, y, TileNo;
	long				x2, y2;
	RECT				rct;


	switch (uMsg)
	{
	case WM_MOUSELEAVE:
		SendMessage(hTiles, WM_APP_HOVERTILE, 0, 0x80000000);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWin;
		TrackMouseEvent(&tme);

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nPos + LOWORD(lParam);
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nPos + HIWORD(lParam);

		if (x < 5 || (unsigned)x > 40 + Tiles.Zoom * 256 || ((unsigned)x > 20 + Tiles.Zoom * 128 && (unsigned)x < 25 + Tiles.Zoom * 128) || y < 5 || (unsigned)y > 28 + Tiles.Zoom * 192)
		{
			SendMessage(hTiles, WM_APP_HOVERTILE, 0, 0x80000000);
			return 0;
		}

		if ((unsigned)x > 20 + Tiles.Zoom * 128)
		{
			x -= 20 + Tiles.Zoom * 128;
			Bank = 0x2000;
		}
		else
		{
			Bank = 0;
		}

		TileNo = Bank + 16 * ((x - 5) / (Tiles.Zoom * 8 + 1) + 16 * ((y - 5) / (Tiles.Zoom * 8 + 1)));
		SendMessage(hTiles, uMsg == WM_MOUSEMOVE ? WM_APP_HOVERTILE : WM_APP_SELECTTILE, 0, TileNo);

		if (uMsg == WM_LBUTTONDBLCLK && hTileMap)
		{
			SendMessage(hTileMap, WM_APP_SETTILE, 0, TileNo / 16);
		}

		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(hWin, SB_HORZ, &si);
			x = si.nPos;
			GetScrollInfo(hWin, SB_VERT, &si);
			y = si.nPos;

			BeginPaint(hWin, &Paint);

			pGameBoy = GameBoys.GetActive(true);

			ZeroMemory(&bmi, sizeof(bmi));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = 8;
			bmi.bmiHeader.biHeight = -8;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 4;
			bmi.bmiHeader.biCompression = BI_RGB;
			hBitmap = CreateDIBSection(Paint.hdc, &bmi, DIB_PAL_COLORS, (void **)&pBitmap, NULL, 0);

			hdc = CreateCompatibleDC(Paint.hdc);
			hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
			ZeroMemory(Palette, sizeof(Palette));
			if (pGameBoy)
			{
				PaletteNo = SendMessage(hTiles_PaletteNo, CB_GETCURSEL, 0, 0);
				if (pGameBoy->Flags & GB_ROM_COLOR && PaletteNo <= 15)
				{
					if (PaletteNo <= 7)
					{
						Palette[0].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 10) & 0x1F) << 3;
						Palette[0].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8] >> 5) & 0x1F) << 3;
						Palette[0].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8] & 0x1F) << 3;
						Palette[1].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 10) & 0x1F) << 3;
						Palette[1].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] >> 5) & 0x1F) << 3;
						Palette[1].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 4] & 0x1F) << 3;
						Palette[2].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 10) & 0x1F) << 3;
						Palette[2].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] >> 5) & 0x1F) << 3;
						Palette[2].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 2] & 0x1F) << 3;
						Palette[3].rgbBlue = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 10) & 0x1F) << 3;
						Palette[3].rgbGreen = ((*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] >> 5) & 0x1F) << 3;
						Palette[3].rgbRed = (*(WORD *)&pGameBoy->BGP[PaletteNo * 8 + 6] & 0x1F) << 3;
					}
					else
					{
						Palette[0].rgbBlue = 0xFF;
						Palette[0].rgbGreen = 0xFF;
						Palette[0].rgbRed = 0xFF;
						Palette[1].rgbBlue = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] >> 10) & 0x1F) << 3;
						Palette[1].rgbGreen = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] >> 5) & 0x1F) << 3;
						Palette[1].rgbRed = (*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 4] & 0x1F) << 3;
						Palette[2].rgbBlue = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] >> 10) & 0x1F) << 3;
						Palette[2].rgbGreen = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] >> 5) & 0x1F) << 3;
						Palette[2].rgbRed = (*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 2] & 0x1F) << 3;
						Palette[3].rgbBlue = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] >> 10) & 0x1F) << 3;
						Palette[3].rgbGreen = ((*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] >> 5) & 0x1F) << 3;
						Palette[3].rgbRed = (*(WORD *)&pGameBoy->OBP[(PaletteNo & 7) * 8 + 6] & 0x1F) << 3;
					}
				}
				else
				{
					switch (PaletteNo & 3)
					{
					case 1:
						Palette[0].rgbBlue = 0xFF;
						Palette[0].rgbGreen = 0xFF;
						Palette[0].rgbRed = 0xFF;
						Palette[1].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 4) & 3];
						Palette[1].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 4) & 3];
						Palette[1].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 4) & 3];
						Palette[2].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 2) & 3];
						Palette[2].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 2) & 3];
						Palette[2].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 2) & 3];
						Palette[3].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 6) & 3];
						Palette[3].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 6) & 3];
						Palette[3].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F48] >> 6) & 3];
						break;

					case 2:
						Palette[0].rgbBlue = 0xFF;
						Palette[0].rgbGreen = 0xFF;
						Palette[0].rgbRed = 0xFF;
						Palette[1].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 4) & 3];
						Palette[1].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 4) & 3];
						Palette[1].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 4) & 3];
						Palette[2].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 2) & 3];
						Palette[2].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 2) & 3];
						Palette[2].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 2) & 3];
						Palette[3].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 6) & 3];
						Palette[3].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 6) & 3];
						Palette[3].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F49] >> 6) & 3];
						break;

					default:
						Palette[0].rgbBlue = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
						Palette[0].rgbGreen = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
						Palette[0].rgbRed = (BYTE)GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
						Palette[1].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
						Palette[1].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
						Palette[1].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
						Palette[2].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
						Palette[2].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
						Palette[2].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
						Palette[3].rgbBlue = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
						Palette[3].rgbGreen = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
						Palette[3].rgbRed = (BYTE)GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
						break;
					}
				}
				SetDIBColorTable(hdc, 0, 4, Palette);
			}
			else
			{
				Palette[0].rgbBlue = 0xFF;
				Palette[0].rgbGreen = 0xFF;
				Palette[0].rgbRed = 0xFF;
				Palette[1].rgbBlue = 0xFF;
				Palette[1].rgbGreen = 0xFF;
				Palette[1].rgbRed = 0xFF;
				Palette[2].rgbBlue = 0xFF;
				Palette[2].rgbGreen = 0xFF;
				Palette[2].rgbRed = 0xFF;
				Palette[3].rgbBlue = 0xFF;
				Palette[3].rgbGreen = 0xFF;
				Palette[3].rgbRed = 0xFF;
				SetDIBColorTable(hdc, 0, 4, Palette);
			}

			if (!pGameBoy)
			{
				ZeroMemory(*pBitmap, sizeof(pBitmap));
			}

			Bank = 0;
			while (true)
			{
				pTile = 0;
				while (pTile < 16 * 24)
				{
					if (pGameBoy)
					{
						TileY = 0;
						while (TileY < 8)
						{
							Color1 = pGameBoy->MEM_VRAM[Bank + (pTile << 4) + (TileY << 1)];
							Color2 = pGameBoy->MEM_VRAM[Bank + (pTile << 4) + (TileY << 1) + 1];

							(*pBitmap)[TileY] = TileToBmp(Color1, Color2);
							TileY++;
						}
					}

					if (Tiles.Zoom == 1)
					{
						BitBlt(Paint.hdc, 5 + 9 * (pTile & 0x0F) + (Bank ? 149 : 0) - x, 5 + 9 * ((pTile & ~0xF) >> 4) - y, 8, 8, hdc, 0, 0, SRCCOPY);
					}
					else
					{
						StretchBlt(Paint.hdc, 5 + Tiles.Zoom * 8 * (pTile & 0x0F) + (pTile & 0x0F) + (Bank ? 21 + Tiles.Zoom * 128 : 0) - x, 5 + Tiles.Zoom * 8 * ((pTile & ~0xF) >> 4) + ((pTile & ~0xF) >> 4) - y, Tiles.Zoom * 8, Tiles.Zoom * 8, hdc, 0, 0, 8, 8, SRCCOPY);
					}

					pTile++;
				}
				if (Bank != 0)
				{
					break;
				}
				Bank = 0x2000;
			}

			SelectObject(hdc, hOldBitmap);
			DeleteDC(hdc);
			DeleteObject(hBitmap);

			if (!(Tiles_SelectedTile & 0x80000000))
			{
				if (Tiles_SelectedTile & 0x2000)
				{
					x2 = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 25 + 128 * Tiles.Zoom - x;
				}
				else
				{
					x2 = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 4 - x;
				}
				y2 = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 8) & 0x1F) + 4 - y;

				hPen = CreatePen(PS_SOLID, 0, RGB(0x00, 0x00, 0xFF));
				hOldPen = (HPEN)SelectObject(Paint.hdc, hPen);

				MoveToEx(Paint.hdc, x2, y2, NULL);
				LineTo(Paint.hdc, x2 + 8 * Tiles.Zoom + 1, y2);
				LineTo(Paint.hdc, x2 + 8 * Tiles.Zoom + 1, y2 + 8 * Tiles.Zoom + 1);
				LineTo(Paint.hdc, x2, y2 + 8 * Tiles.Zoom + 1);
				LineTo(Paint.hdc, x2, y2);

				SelectObject(Paint.hdc, hOldPen);
				DeleteObject(hPen);
			}

			if (pGameBoy)
			{
				pGameBoy->Release();
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SIZE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE;
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nMax;
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nMax;
		GetWindowRect(hWin, &rct);
		si.nPage = rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE);
		if ((unsigned)rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYSMCAPTION) < y + 1)
		{
			si.nPage -= GetSystemMetrics(SM_CXVSCROLL);
			x += GetSystemMetrics(SM_CXVSCROLL);
		}
		else
		{
			if ((unsigned)rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE) < x + 1 && (unsigned)rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYSMCAPTION) - GetSystemMetrics(SM_CYVSCROLL) < y + 1)
			{
				si.nPage -= GetSystemMetrics(SM_CXVSCROLL);
				x += GetSystemMetrics(SM_CXVSCROLL);
			}
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.fMask = SIF_PAGE | SIF_POS;
		GetScrollInfo(hWin, SB_VERT, &si);
		si.nPage = rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYSMCAPTION);
		if ((unsigned)rct.right - rct.left - 2 * GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXEDGE) < x + 1)
		{
			si.nPage -= GetSystemMetrics(SM_CYHSCROLL);
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		InvalidateRect(hWin, NULL, true);
		return 0;

	case WM_VSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN:
			si.nPos += 10;
			break;
		case SB_LINEUP:
			si.nPos -= 10;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, 0, y - si.nPos, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_HSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT:
			si.nPos += 10;
			break;
		case SB_LINELEFT:
			si.nPos -= 10;
			break;
		case SB_PAGERIGHT:
			si.nPos += si.nPage;
			break;
		case SB_PAGELEFT:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, x - si.nPos, 0, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_CREATE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
		si.nPos = 0;
		si.nMin = 0;
		si.nMax = 40 + Tiles.Zoom * 256;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.nMax = 10 + Tiles.Zoom * 192;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;
	}

	return DefWindowProc(hWin, uMsg, wParam, lParam);
}



void TilesUpdateInfo()
{
	CGameBoy		*pGameBoy;
	DWORD			TileNo, PaletteNo;


	if (pGameBoy = GameBoys.GetActive(true))
	{
		LockTilesUpdate = true;

		PaletteNo = SendMessage(hTiles_PaletteNo, CB_GETCURSEL, 0, 0);
		if (PaletteNo == CB_ERR)
		{
			PaletteNo = 0;
		}
		if (pGameBoy->Flags & GB_ROM_COLOR)
		{
			if (SendMessage(hTiles_PaletteNo, CB_GETCOUNT, 0, 0) == 3)
			{
				PaletteNo += 16;
			}
		}
		else
		{
			if (SendMessage(hTiles_PaletteNo, CB_GETCOUNT, 0, 0) == 19)
			{
				if (PaletteNo > 16)
				{
					PaletteNo -= 16;
				}
				else
				{
					PaletteNo = 0;
				}
			}
		}

		if ((SendMessage(hTiles_PaletteNo, CB_GETCOUNT, 0, 0) == 3 && (pGameBoy->Flags & GB_ROM_COLOR))
			|| (SendMessage(hTiles_PaletteNo, CB_GETCOUNT, 0, 0) == 19 && !(pGameBoy->Flags & GB_ROM_COLOR)))
		{
			SendMessage(hTiles_PaletteNo, CB_RESETCONTENT, 0, 0);
			if (pGameBoy->Flags & GB_ROM_COLOR)
			{
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP0");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP1");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP2");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP3");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP4");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP5");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP6");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP7");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP0");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP1");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP2");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP3");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP4");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP5");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP6");
				SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP7");
			}
			SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP");
			SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP0");
			SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP1");
			SendMessage(hTiles_PaletteNo, CB_SETCURSEL, PaletteNo, 0);
		}

		if (Tiles_HoverTile & 0x80000000)
		{
			TileNo = Tiles_SelectedTile;
		}
		else
		{
			TileNo = Tiles_HoverTile;
		}
		if (!(TileNo & 0x80000000))
		{
			SetWindowLong(hTiles_TileZoom, GWL_USERDATA, (Tiles_HoverTile & 0x80000000 ? Tiles_SelectedTile : Tiles_HoverTile) | (PaletteNo << 16));
			EnableWindow(hTiles_PaletteNo, true);
		}
		else
		{
			SetWindowLong(hTiles_TileZoom, GWL_USERDATA, 0x80000000);
			EnableWindow(hTiles_PaletteNo, true);
		}

		pGameBoy->Release();

		LockTilesUpdate = false;
	}
	else
	{
		Tiles_SelectedTile = Tiles_HoverTile = 0x80000000;
		SetWindowLong(hTiles_TileZoom, GWL_USERDATA, 0x80000000);
		EnableWindow(hTiles_PaletteNo, false);
	}

	InvalidateRect(hTiles_TileZoom, NULL, true);
}



LRESULT CALLBACK TilesWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT				rct, Rect2;
	SCROLLINFO			si;
	POINT				Pt;
	DWORD				x, y;
	char				szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_TILES));
		return 0;

	case WM_APP_SELECTTILE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		GetScrollInfo(hTilesChild, SB_HORZ, &si);
		x = si.nPos;
		GetScrollInfo(hTilesChild, SB_VERT, &si);
		y = si.nPos;

		//Erase last selection
		if (!(Tiles_SelectedTile & 0x80000000))
		{
			if (Tiles_SelectedTile & 0x2000)
			{
				rct.left = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 25 + 128 * Tiles.Zoom - x;
			}
			else
			{
				rct.left = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 4 - x;
			}
			rct.top = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 8) & 0x1F) + 4 - y;
			rct.right = rct.left + Tiles.Zoom * 8 + 2;
			rct.bottom = rct.top + Tiles.Zoom * 8 + 2;
			Tiles_SelectedTile = 0x80000000;
			InvalidateRect(hTilesChild, &rct, true);
			UpdateWindow(hTilesChild);
		}

		Tiles_SelectedTile = lParam;
		TilesUpdateInfo();

		if (!(Tiles_SelectedTile & 0x80000000))
		{
			if (Tiles_SelectedTile & 0x2000)
			{
				rct.left = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 25 + 128 * Tiles.Zoom - x;
			}
			else
			{
				rct.left = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 4) & 0x0F) + 4 - x;
			}
			rct.top = (Tiles.Zoom * 8 + 1) * ((Tiles_SelectedTile >> 8) & 0x1F) + 4 - y;
			rct.right = rct.left + Tiles.Zoom * 8 + 2;
			rct.bottom = rct.top + Tiles.Zoom * 8 + 2;
			InvalidateRect(hTilesChild, &rct, true);
		}
		return 0;

	case WM_APP_HOVERTILE:
		Tiles_HoverTile = lParam;
		TilesUpdateInfo();
		return 0;

	case WM_PAINT:
		TilesUpdateInfo();
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TILES_PALETTENO:
			if (LockTilesUpdate)
			{
				break;
			}
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				InvalidateRect(hWin, NULL, true);
				return 0;
			}
			break;

		case ID_VIEW_ZOOM_100:
			Tiles.Zoom = 1;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 40 + 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_HORZ, &si, true);
			si.nMax = 10 + 192;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_VERT, &si, true);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 134 + 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 45 + 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_200:
			Tiles.Zoom = 2;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 40 + 2 * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_HORZ, &si, true);
			si.nMax = 10 + 2 * 192;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_VERT, &si, true);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 134 + 2 * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 45 + 2 * 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_300:
			Tiles.Zoom = 3;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 40 + 3 * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_HORZ, &si, true);
			si.nMax = 10 + 3 * 192;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_VERT, &si, true);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 134 + 3 * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 45 + 3 * 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_VIEW_ZOOM_400:
			Tiles.Zoom = 4;
			GetWindowRect(hWin, &rct);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 40 + Tiles.Zoom * 256;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_HORZ, &si, true);
			si.nMax = 10 + Tiles.Zoom * 192;
			si.nPage = si.nMax + 1;
			SetScrollInfo(hTilesChild, SB_VERT, &si, true);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hWin, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 134 + 4 * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 4 * GetSystemMetrics(SM_CXEDGE), 45 + 4 * 192 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 4 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
			InvalidateRect(hWin, NULL, true);
			return 0;
		}
		break;

	case WM_SIZE:
		GetClientRect(hWin, &rct);
		MoveWindow(hTilesChild, 5, 5, rct.right - 85, rct.bottom - 10, true);
		MoveWindow(hTiles_TileZoom, rct.right - 75, 5, 71, 71, true);
		MoveWindow(hTiles_PaletteNo, rct.right - 75, 80, 70, 200, true);
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		Tiles_SelectedTile = Tiles_HoverTile = 0x80000000;

		hTilesChild = CreateWindowEx(WS_EX_CLIENTEDGE, "TilesChild", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		hTiles_TileZoom = CreateWindow("TileZoom", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SetWindowLong(hTiles_TileZoom, GWL_USERDATA, 0x80000000);
		hTiles_PaletteNo = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hTiles_PaletteNo, WM_SETFONT, (WPARAM)hFixedFont, 0);
		SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"BGP");
		SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP0");
		SendMessage(hTiles_PaletteNo, CB_ADDSTRING, 0, (LPARAM)&"OBP1");
		SetWindowLong(hTiles_PaletteNo, GWL_ID, ID_TILES_PALETTENO);
		return 0;

	case WM_DESTROY:
		hTiles = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		Tiles.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		Tiles.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		Tiles.Width = rct.right - rct.left;
		Tiles.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



BOOL		MemoryCaret;
int			MemoryCaretX, MemoryCaretY;
WORD		MemoryTopByte;

BYTE		MemoryROM, MemoryRAM, MemorySVBK, MemoryVBK;



BOOL CALLBACK SetAddressEnumChildProc(HWND hWin, LPARAM lParam)
{
	if (GetWindowLong(hWin, GWL_ID) != IDC_NUMBER)
	{
		return true;
	}

	SendMessage(hWin, WM_SETTEXT, 0, lParam);

	return false;
}



BOOL CALLBACK GotoEnumChildProc(HWND hWin, LPARAM lParam)
{
	char		szText[6];


	if (GetWindowLong(hWin, GWL_ID) != IDC_NUMBER)
	{
		return true;
	}

	SendMessage(hWin, WM_GETTEXT, 6, (LPARAM)&szText);
	if (strlen(szText) > 4)
	{
		return false;
	}

	if (HexToNum(&szText[0]))
	{
		return false;
	}

	if (szText[1] != '\0')
	{
		if (HexToNum(&szText[1]))
		{
			return false;
		}
		if (szText[2] != '\0')
		{
			if (HexToNum(&szText[2]))
			{
				return false;
			}
			if (szText[3] != '\0')
			{
				if (HexToNum(&szText[3]))
				{
					return false;
				}
				*(DWORD *)lParam = (szText[0] << 12) | (szText[1] << 8) | (szText[2] << 4) | szText[3];
			}
			else
			{
				*(DWORD *)lParam = (szText[0] << 8) | (szText[1] << 4) | szText[2];
			}
		}
		else
		{
			*(DWORD *)lParam = (szText[0] << 4) | szText[1];
		}
	}
	else
	{
		*(DWORD *)lParam = szText[0];
	}

	return false;
}



BOOL CALLBACK MemoryRomBankEnumChildProc(HWND hWin, LPARAM lParam)
{
	char		szText[4];


	if (GetWindowLong(hWin, GWL_ID) != IDC_NUMBER)
	{
		return true;
	}

	if (lParam)
	{
		itoa(MemoryROM, szText, 16);
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&szText);
	}
	else
	{
		SendMessage(hWin, WM_GETTEXT, 4, (LPARAM)&szText);
		if (szText[0] && szText[1] && szText[2])
		{
			return true;
		}

		if (HexToNum(&szText[0]))
		{
			return true;
		}

		if (szText[1] != '\0')
		{
			if (HexToNum(&szText[1]))
			{
				return true;
			}
			MemoryROM = (szText[0] << 4) | szText[1];
		}
		else
		{
			MemoryROM = szText[0];
		}

		MemoryFlags |= MEMORY_ROM;
	}

	return false;
}



BOOL CALLBACK MemoryGotoDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD		NewNumber;
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			NewNumber = MemoryTopByte;
			EnumChildWindows(hWin, GotoEnumChildProc, (LPARAM)&NewNumber);
			if (MemoryTopByte == NewNumber)
			{
				EndDialog(hWin, true);
				return true;
			}
			MemoryTopByte = (WORD)NewNumber;
			MemoryCaretY = 0;
			MemoryCaretX = 0;
			EndDialog(hWin, false);
			return true;

		case IDCANCEL:
			EndDialog(hWin, true);
			return true;
		}
		break;

	/*case WM_CLOSE:
		EndDialog(hWin, true);
		return true;*/

	case WM_INITDIALOG:
		SetWindowText(hWin, String(IDS_ENTERADDRESS));
		itoa(MemoryTopByte, NumBuffer, 16);
		EnumChildWindows(hWin, SetAddressEnumChildProc, (LPARAM)&NumBuffer);
		return true;
	}

	return false;
}



BOOL CALLBACK MemoryRomBankDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EnumChildWindows(hWin, MemoryRomBankEnumChildProc, false);
			EndDialog(hWin, false);
			return true;

		case IDCANCEL:
			EndDialog(hWin, true);
			return true;
		}
		break;

	/*case WM_CLOSE:
		EndDialog(hWin, true);
		return true;*/

	case WM_INITDIALOG:
		SetWindowText(hWin, String(IDS_ENTERROMBANKNO));
		EnumChildWindows(hWin, MemoryRomBankEnumChildProc, true);
		return true;
	}

	return false;
}



BOOL CreateBankMenu(CGameBoy *pGameBoy, HMENU hMenu, DWORD dwFirstPos)
{
	HMENU			hRAMMenu = GetSubMenu(hMenu, dwFirstPos + 2);
	BYTE			Banks;
	MENUITEMINFO	mmi;
	char			szBuffer[0x100];


	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_STATE;
	if (pGameBoy->Flags & GB_ROM_COLOR)
	{
		mmi.fState = MFS_ENABLED;
	}
	else
	{
		mmi.fState = MFS_GRAYED;
	}
	SetMenuItemInfo(hMenu, dwFirstPos + 1, true, &mmi);
	SetMenuItemInfo(hMenu, dwFirstPos + 3, true, &mmi);
	if (pGameBoy->MaxRomBank > 1)
	{
		mmi.fState = MFS_ENABLED;
	}
	else
	{
		mmi.fState = MFS_GRAYED;
	}
	SetMenuItemInfo(hMenu, dwFirstPos, true, &mmi);

	switch (pGameBoy->MEM_ROM[0x0149])
	{
	case 1:
		Banks = 1;
		break;
	case 2:
		Banks = 1;
		break;
	case 3:
		Banks = 4;
		break;
	case 4:
		Banks = 16;
		break;
	default:
		Banks = 0;
		break;
	}
	if (Banks <= 1)
	{
		mmi.fState = MFS_GRAYED;
		SetMenuItemInfo(hMenu, dwFirstPos + 2, true, &mmi);
	}
	else
	{
		mmi.fState = MFS_ENABLED;
		SetMenuItemInfo(hMenu, dwFirstPos + 2, true, &mmi);
		if (Banks >= 4)
		{
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK2, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK2, LoadString(IDS_VIEW_BANK_RAM_2_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK3, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK3, LoadString(IDS_VIEW_BANK_RAM_3_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
		}
		if (Banks >= 16)
		{
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK4, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK4, LoadString(IDS_VIEW_BANK_RAM_4_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK5, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK5, LoadString(IDS_VIEW_BANK_RAM_5_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK6, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK6, LoadString(IDS_VIEW_BANK_RAM_6_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK7, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK7, LoadString(IDS_VIEW_BANK_RAM_7_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK8, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK8, LoadString(IDS_VIEW_BANK_RAM_8_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK9, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK9, LoadString(IDS_VIEW_BANK_RAM_9_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK10, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK10, LoadString(IDS_VIEW_BANK_RAM_10_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK11, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK11, LoadString(IDS_VIEW_BANK_RAM_11_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK12, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK12, LoadString(IDS_VIEW_BANK_RAM_12_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK13, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK13, LoadString(IDS_VIEW_BANK_RAM_13_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK14, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK14, LoadString(IDS_VIEW_BANK_RAM_14_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
			DeleteMenu(hRAMMenu, ID_MEMORY_RAM_BANK15, MF_BYCOMMAND);
			if (!AppendMenu(hRAMMenu, MF_STRING, ID_MEMORY_RAM_BANK15, LoadString(IDS_VIEW_BANK_RAM_15_MENU, szBuffer, sizeof(szBuffer))))
			{
				DisplayErrorMessage();
				return true;
			}
		}
	}

	return false;
}



#define		CalcBytesPerLine	BytesPerLine = (rct.right / FixedFontWidth - 10) / 4; if (BytesPerLine == 0) { BytesPerLine = 1; }

LPARAM CALLBACK MemoryWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;
	POINT			Point;
	LOGBRUSH		lb;
	HBRUSH			hBrush;
	HPEN			hPen;
	SCROLLINFO		si;
	WORD			pByte;
	BYTE			Byte, Access, Banks, NewByte, *p;
	int				x, y, BytesPerLine, nBytes;
	char			szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_MEMORY));
		return 0;

	case WM_COMMAND:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			break;
		}
		switch (LOWORD(wParam))
		{
		case ID_EDIT_GOTO:
			if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENTERNUMBER), hWnd, MemoryGotoDlgProc))
			{
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_MEMORY_ROM:
			if (!(MemoryFlags & MEMORY_ROM))
			{
				MemoryROM = pGameBoy->ActiveRomBank;
			}
			Byte = MemoryROM;
			if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENTERNUMBER), hWnd, MemoryRomBankDlgProc))
			{
				if (MemoryROM <= pGameBoy->MaxRomBank)
				{
					InvalidateRect(hWin, NULL, true);
				}
				else
				{
					MemoryROM = Byte;
				}
			}
			return 0;

		case ID_MEMORY_VBK_BANK0:
			MemoryFlags |= MEMORY_VBK;
			MemoryVBK = 0;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_VBK_BANK1:
			MemoryFlags |= MEMORY_VBK;
			MemoryVBK = 1;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK0:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 0;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK1:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 1;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK2:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 2;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK3:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 3;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK4:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 4;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK5:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 5;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK6:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 6;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK7:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 7;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK8:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 8;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK9:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 9;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK10:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 10;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK11:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 11;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK12:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 12;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK13:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 13;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK14:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 14;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_RAM_BANK15:
			MemoryFlags |= MEMORY_RAM;
			MemoryRAM = 15;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK1:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 1;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK2:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 2;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK3:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 3;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK4:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 4;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK5:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 5;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK6:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 6;
			InvalidateRect(hWin, NULL, true);
			return 0;

		case ID_MEMORY_SVBK_BANK7:
			MemoryFlags |= MEMORY_SVBK;
			MemorySVBK = 7;
			InvalidateRect(hWin, NULL, true);
			return 0;
		}
		break;

	case WM_MENUSELECT:
		return SendMessage(hWnd, uMsg, wParam, lParam);

	case WM_EXITMENULOOP:
		SetStatus(NULL, SF_READY);
		return 0;

	case WM_SETFOCUS:
		if (GameBoys.GetActive(false))
		{
			CreateCaret(hWin, NULL, 2, 16);
			SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
			ShowCaret(hWin);
			MemoryCaret = true;
		}
		return 0;

	case WM_KILLFOCUS:
		if (MemoryCaret)
		{
			DestroyCaret();
			MemoryCaret = false;
		}
		return 0;

	case WM_KEYDOWN:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			break;
		}
		switch (wParam)
		{
		case VK_CONTROL:
		case VK_SHIFT:
		case VK_RWIN:
		case VK_LWIN:
		case VK_INSERT:
		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
		case VK_MENU:
			pGameBoy->Release();
			return DefMDIChildProc(hWin, uMsg, wParam, lParam);

		case VK_NUMPAD0:
			wParam = '0';
			break;

		case VK_NUMPAD1:
			wParam = '1';
			break;

		case VK_NUMPAD2:
			wParam = '2';
			break;

		case VK_NUMPAD3:
			wParam = '3';
			break;

		case VK_NUMPAD4:
			wParam = '4';
			break;

		case VK_NUMPAD5:
			wParam = '5';
			break;

		case VK_NUMPAD6:
			wParam = '6';
			break;

		case VK_NUMPAD7:
			wParam = '7';
			break;

		case VK_NUMPAD8:
			wParam = '8';
			break;

		case VK_NUMPAD9:
			wParam = '9';
			break;
		}
		GetClientRect(hWin, &rct);
		CalcBytesPerLine;
		while (LOWORD(lParam--))
		{
			if (MemoryCaretX == 2 || MemoryCaretX == 7 || MemoryCaretX == 8 || MemoryCaretX == 9 + 3 * BytesPerLine || ((MemoryCaretX - 9) % 3) == 2)
			{
				if (wParam == VK_SPACE)
				{
					MemoryCaretX++;
				}
			}
			else
			{
				if ((wParam >= '0' && wParam <= '9') || (wParam >= 'A' && wParam <= 'F'))
				{
					Byte = (wParam >= 'A') ? wParam - 'A' + 10 : wParam - '0';
					pByte = MemoryTopByte + BytesPerLine * MemoryCaretY;
					switch (MemoryCaretX)
					{
					case 0:
						if (pByte < 0x4000 || pByte > 0x7FFF || pGameBoy->MaxRomBank < Byte << 4)
						{
							if (Byte == 0)
							{
								MemoryCaretX++;
							}
							break;
						}
						if (MemoryFlags & MEMORY_ROM)
						{
							if (Byte == MemoryROM >> 4)
							{
								MemoryCaretX++;
								break;
							}
							MemoryROM = (Byte << 4) | (MemoryROM & 0x0F);
						}
						else
						{
							if (Byte == pGameBoy->ActiveRomBank >> 4)
							{
								MemoryCaretX++;
								break;
							}
							MemoryFlags |= MEMORY_ROM;
							MemoryROM = (Byte << 4) | (pGameBoy->ActiveRomBank & 0x0F);
						}
						MemoryCaretX++;
						InvalidateRect(hWin, &rct, true);
						break;

					case 1:
						if (pByte < 0x4000)
						{
							if (Byte == 0)
							{
								MemoryCaretX += 2;
							}
							break;
						}
						if (pByte < 0x8000)
						{
							if (MemoryFlags & MEMORY_ROM)
							{
								if (Byte == (MemoryROM & 0x0F))
								{
									MemoryCaretX += 2;
									break;
								}
								if (pGameBoy->MaxRomBank < ((MemoryROM & 0xF0) | Byte))
								{
									break;
								}
								MemoryROM = (MemoryROM & 0xF0) | Byte;
							}
							else
							{
								if (Byte == (pGameBoy->ActiveRomBank & 0x0F))
								{
									MemoryCaretX += 2;
									break;
								}
								if (pGameBoy->MaxRomBank < ((pGameBoy->ActiveRomBank & 0xF0) | Byte))
								{
									break;
								}
								MemoryFlags |= MEMORY_ROM;
								MemoryROM = (pGameBoy->ActiveRomBank & 0xF0) | Byte;
							}
						}
						else if (pByte < 0xA000)
						{
							if (MemoryFlags & MEMORY_VBK)
							{
								if (Byte == MemoryVBK)
								{
									MemoryCaretX += 2;
									break;
								}
								if (Byte > 1)
								{
									break;
								}
								MemoryVBK = Byte;
							}
							else
							{
								if (Byte > 1 || (Byte > 0 && !(pGameBoy->Flags & GB_ROM_COLOR)))
								{
									break;
								}
								if (Byte == (pGameBoy->MEM_CPU[0x8F4F] & 1))
								{
									MemoryCaretX += 2;
									break;
								}
								MemoryFlags |= MEMORY_VBK;
								MemoryVBK = Byte;
							}
						}
						else if (pByte < 0xC000)
						{
							switch (pGameBoy->MEM_ROM[0x0149])
							{
							case 1:
								Banks = 0;
								break;
							case 2:
								Banks = 0;
								break;
							case 3:
								Banks = 3;
								break;
							case 4:
								Banks = 15;
								break;
							default:
								Banks = 0;
								break;
							}
							if (Byte > Banks)
							{
								break;
							}
							if (MemoryFlags & MEMORY_RAM)
							{
								if (Byte == MemoryRAM)
								{
									MemoryCaretX += 2;
									break;
								}
								MemoryRAM = Byte;
							}
							else
							{
								if (Byte == pGameBoy->ActiveRamBank)
								{
									MemoryCaretX += 2;
									break;
								}
								MemoryFlags |= MEMORY_RAM;
								MemoryRAM = Byte;
							}
						}
						else if (pByte < 0xD000)
						{
							if (Byte == 0)
							{
								MemoryCaretX += 2;
								break;
							}
							break;
						}
						else if (pByte < 0xE000)
						{
							if (Byte == 0 || Byte > 7 || (Byte > 1 && !(pGameBoy->Flags & GB_ROM_COLOR)))
							{
								break;
							}
							if (MemoryFlags & MEMORY_SVBK)
							{
								if (Byte == MemorySVBK)
								{
									MemoryCaretX += 2;
									break;
								}
								MemorySVBK = Byte;
							}
							else
							{
								if (Byte == ((pGameBoy->MEM_CPU[0x8F70] & 7) ? pGameBoy->MEM_CPU[0x8F70] & 7 : 1))
								{
									MemoryCaretX += 2;
									break;
								}
								MemoryFlags |= MEMORY_SVBK;
								MemorySVBK = Byte;
							}
						}
						else
						{
							if (Byte == 0)
							{
								MemoryCaretX += 2;
								break;
							}
							break;
						}
						MemoryCaretX += 2;
						InvalidateRect(hWin, &rct, true);
						break;

					case 3:
						MemoryCaretX++;
						if ((Byte << 12) != (pByte & 0xF000))
						{
							if (MemoryTopByte + (Byte << 12) - (pByte & 0xF000) < 0)
							{
								MemoryTopByte = (Byte << 12) | (pByte & 0x0FFF);
								MemoryCaretY = 0;
							}
							else
							{
								MemoryTopByte += (Byte << 12) - (pByte & 0xF000);
							}
							InvalidateRect(hWin, &rct, true);
						}
						break;

					case 4:
						MemoryCaretX++;
						if ((Byte << 8) != (pByte & 0x0F00))
						{
							if (MemoryTopByte + (Byte << 8) - (pByte & 0x0F00) < 0)
							{
								MemoryTopByte = (Byte << 8) | (pByte & 0xF0FF);
								MemoryCaretY = 0;
							}
							else
							{
								MemoryTopByte += (Byte << 8) - (pByte & 0x0F00);
							}
							InvalidateRect(hWin, &rct, true);
						}
						break;

					case 5:
						MemoryCaretX++;
						if ((Byte << 4) != (pByte & 0x00F0))
						{
							if (MemoryTopByte + (Byte << 4) - (pByte & 0x00F0) < 0)
							{
								MemoryTopByte = (Byte << 4) | (pByte & 0xFF0F);
								MemoryCaretY = 0;
							}
							else
							{
								MemoryTopByte += (Byte << 4) - (pByte & 0x00F0);
							}
							InvalidateRect(hWin, &rct, true);
						}
						break;

					case 6:
						MemoryCaretX += 3;
						if (Byte != (pByte & 0x000F))
						{
							if (MemoryTopByte + Byte - (pByte & 0x000F) < 0)
							{
								MemoryTopByte = Byte | (pByte & 0xFFF0);
								MemoryCaretY = 0;
							}
							else
							{
								MemoryTopByte += Byte - (pByte & 0x000F);
							}
							InvalidateRect(hWin, &rct, true);
						}
						break;

					default:
						if (MemoryCaretX >= 9 && MemoryCaretX <= 8 + 3 * BytesPerLine)
						{
							pByte = MemoryTopByte + MemoryCaretY * BytesPerLine + (MemoryCaretX - 9) / 3;
							if (pByte < 0x8000)
							{
								break;
							}
							if (pByte < 0xA000)
							{
								if (MemoryFlags & MEMORY_VBK)
								{
									Access = pGameBoy->MemStatus_VRAM[(pByte & 0x1FFF) + MemoryVBK * 0x2000] |= MEM_READ | MEM_CHANGED;
									p = &pGameBoy->MEM_VRAM[(pByte & 0x1FFF) + MemoryVBK * 0x2000];
								}
								else
								{
									Access = pGameBoy->MemStatus_VRAM[(pByte & 0x1FFF) + (pGameBoy->MEM_CPU[0x8F4F] & 1) * 0x2000] |= MEM_READ | MEM_CHANGED;
									p = &pGameBoy->MEM_VRAM[(pByte & 0x1FFF) + (pGameBoy->MEM_CPU[0x8F4F] & 1) * 0x2000];
								}
							}
							else if (pByte < 0xC000)
							{
								if (pGameBoy->MEM_ROM[0x0149] == 0 || pGameBoy->MEM_ROM[0x0149] > 4 || (pByte >= 0xA800 && pGameBoy->MEM_ROM[0x0149] == 1))
								{
									break;
								}
								if (MemoryFlags & MEMORY_RAM)
								{
									Access = pGameBoy->MemStatus_RAM[(pByte & 0x1FFF) + MemoryRAM * 0x2000] |= MEM_READ | MEM_CHANGED;
									p = &pGameBoy->MEM_RAM[(pByte & 0x1FFF) + MemoryRAM * 0x2000];
								}
								else
								{
									Access = pGameBoy->MemStatus_RAM[(pByte & 0x1FFF) + pGameBoy->ActiveRamBank * 0x2000] |= MEM_READ | MEM_CHANGED;
									p = &pGameBoy->MEM_RAM[(pByte & 0x1FFF) + pGameBoy->ActiveRamBank * 0x2000];
								}
							}
							else if (pByte < 0xD000)
							{
								Access = RetrieveAccess(pGameBoy, pByte) | MEM_READ | MEM_CHANGED;
								p = &pGameBoy->MEM_CPU[pByte & 0x0FFF];
								SetAccess(pGameBoy, pByte, Access);
							}
							else if (pByte < 0xE000)
							{
								if (MemoryFlags & MEMORY_SVBK)
								{
									Access = pGameBoy->MemStatus_CPU[(pByte & 0x0FFF) + MemorySVBK * 0x1000] |= MEM_READ | MEM_CHANGED;
									p = &pGameBoy->MEM_CPU[(pByte & 0x0FFF) + MemorySVBK * 0x1000];
								}
								else
								{
									if (pGameBoy->MEM_CPU[0x8F70] & 7)
									{
										Access = pGameBoy->MemStatus_CPU[(pByte & 0x0FFF) + (pGameBoy->MEM_CPU[0x8F70] & 7) * 0x1000] |= MEM_READ | MEM_CHANGED;
										p = &pGameBoy->MEM_CPU[(pByte & 0x0FFF) + (pGameBoy->MEM_CPU[0x8F4F] & 7) * 0x1000];
									}
									else
									{
										Access = pGameBoy->MemStatus_CPU[(pByte & 0x0FFF) + 0x1000] |= MEM_READ | MEM_CHANGED;
										p = &pGameBoy->MEM_CPU[(pByte & 0x0FFF) + 0x1000];
									}
								}
							}
							else
							{
								Access = RetrieveAccess(pGameBoy, pByte) | MEM_READ | MEM_CHANGED;
								if (!(Access & MEM_WRITE))
								{
									break;
								}
								NewByte = (BYTE)ReadMem(pGameBoy, pByte);
								p = NULL;
								SetAccess(pGameBoy, pByte, Access);
							}

							if (((MemoryCaretX - 9) % 3) & 1)
							{
								if (p)
								{
									NewByte = Byte | (*p & 0xF0);
									*p = NewByte;
								}
								else
								{
									NewByte = Byte | (NewByte & 0xF0);
									__asm
									{
										xor		edx, edx
										mov		ecx, pGameBoy
										mov		al, NewByte
										mov		dx, pByte
										call	Debug_LD_mem8
									}
								}
								InvalidateRect(hWin, NULL, true);
								if (MemoryCaretX == 7 + 3 * BytesPerLine)
								{
									MemoryCaretX = 9;
									MemoryCaretY++;
								}
								else
								{
									MemoryCaretX += 2;
								}
							}
							else
							{
								if (p)
								{
									NewByte = (Byte << 4) | (*p & 0x0F);
									*p = NewByte;
								}
								else
								{
									NewByte = (Byte << 4) | (NewByte & 0x0F);
									__asm
									{
										xor		edx, edx
										mov		ecx, pGameBoy
										mov		al, NewByte
										mov		dx, pByte
										call	Debug_LD_mem8
									}
								}
								InvalidateRect(hWin, NULL, true);
								MemoryCaretX++;
							}
						}
					}
				}
			}
			switch (wParam)
			{
			case VK_LEFT:
				if (MemoryCaretX > 0)
				{
					MemoryCaretX--;
				}
				break;

			case VK_RIGHT:
				if (MemoryCaretX < 10 + 4 * BytesPerLine)
				{
					MemoryCaretX++;
				}
				break;

			case VK_UP:
				if (GetKeyState(VK_CONTROL) & 0x8000)
				{
					if (MemoryTopByte - BytesPerLine < 0)
					{
						if (MemoryTopByte != 0)
						{
							MemoryTopByte = 0;
							InvalidateRect(hWin, NULL, true);
						}
					}
					else
					{
						MemoryTopByte -= BytesPerLine;
						if (MemoryCaretY != rct.bottom / FixedFontHeight - 1)
						{
							MemoryCaretY++;
						}
						ScrollWindowEx(hWin, 0, FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
					}
				}
				else
				{
					MemoryCaretY--;
				}
				break;

			case VK_DOWN:
				if (GetKeyState(VK_CONTROL) & 0x8000)
				{
					if (MemoryTopByte + BytesPerLine <= 0xFFFF - BytesPerLine * (rct.bottom / FixedFontHeight - 1))
					{
						MemoryTopByte += BytesPerLine;
						if (MemoryCaretY != 0)
						{
							MemoryCaretY--;
						}
						ScrollWindowEx(hWin, 0, -FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
					}
				}
				else
				{
					MemoryCaretY++;
				}
				break;

			case VK_PRIOR:
				if (MemoryTopByte == 0 && MemoryCaretY == 0)
				{
					pGameBoy->Release();
					return 0;
				}

				if (MemoryTopByte + BytesPerLine * MemoryCaretY <= BytesPerLine * (rct.bottom / FixedFontHeight - 1))
				{
					MemoryTopByte = 0;
					MemoryCaretY = 0;
					InvalidateRect(hWin, NULL, true);
					pGameBoy->Release();
					return 0;
				}

				if (MemoryCaretY < 0)
				{
					MemoryTopByte += BytesPerLine * MemoryCaretY;
					MemoryCaretY = 0;
				}
				if (MemoryCaretY * FixedFontHeight > rct.bottom - FixedFontHeight)
				{
					MemoryTopByte += BytesPerLine * (MemoryCaretY - rct.bottom / FixedFontHeight + 1);
					MemoryCaretY = rct.bottom / FixedFontHeight - 1;
				}
				if (MemoryTopByte >= BytesPerLine * (rct.bottom / FixedFontHeight - 1))
				{
					MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
				}
				else
				{
					MemoryCaretY -= (BytesPerLine * (rct.bottom / FixedFontHeight - 1) - MemoryTopByte) / BytesPerLine;
					MemoryTopByte = 0;
				}
				InvalidateRect(hWin, NULL, true);
				break;

			case VK_NEXT:
				if (MemoryTopByte + BytesPerLine * (MemoryCaretY + rct.bottom / FixedFontHeight) > 0xFFFF)
				{
					while (MemoryTopByte + BytesPerLine < 0x10000)
					{
						MemoryTopByte += BytesPerLine;
					}
					MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
					MemoryCaretY = rct.bottom / FixedFontHeight - 1;
					InvalidateRect(hWin, NULL, true);
					return 0;
				}

				if (MemoryCaretY < 0)
				{
					MemoryTopByte += BytesPerLine * MemoryCaretY;
					MemoryCaretY = 0;
				}
				if (MemoryCaretY * FixedFontHeight > rct.bottom - FixedFontHeight)
				{
					MemoryTopByte += BytesPerLine * MemoryCaretY;
					MemoryCaretY = rct.bottom / FixedFontHeight - 1;
				}
				else
				{
					MemoryTopByte += BytesPerLine * (rct.bottom / FixedFontHeight - 1);
				}
				InvalidateRect(hWin, NULL, true);
				break;

			case VK_HOME:
				MemoryCaretX = 0;
				if (GetKeyState(VK_CONTROL) & 0x8000)
				{
					//Ctrl + Home
					if (MemoryTopByte != 0)
					{
						MemoryTopByte = 0;
						InvalidateRect(hWin, NULL, true);
					}
					MemoryCaretY = 0;
				}
				break;

			case VK_END:
				if (GetKeyState(VK_CONTROL) & 0x8000)
				{
					//Ctrl + End
					MemoryTopByte = 0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight);
					MemoryCaretY = rct.bottom / FixedFontHeight - 1;
					MemoryCaretX = 10 + 4 * BytesPerLine;
					InvalidateRect(hWin, NULL, true);
				}
				else
				{
					MemoryCaretX = 10 + 4 * BytesPerLine;
				}
				break;
			}
		}
		if (MemoryCaret)
		{
			HideCaret(hWin);
		}
		if (MemoryCaretY < 0)
		{
			if (MemoryTopByte + BytesPerLine * MemoryCaretY < 0)
			{
				if (MemoryTopByte != 0)
				{
					MemoryTopByte = 0;
					InvalidateRect(hWin, NULL, true);
				}
				MemoryCaretY = 0;
			}
			else
			{
				ScrollWindowEx(hWin, 0, -MemoryCaretY * FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
				MemoryTopByte += BytesPerLine * MemoryCaretY;
				MemoryCaretY = 0;
			}
		}
		if (MemoryCaretY * FixedFontHeight > rct.bottom - FixedFontHeight)
		{
			if (MemoryTopByte + BytesPerLine * (MemoryCaretY) > 0xFFFF)
			{
				if (MemoryTopByte != 0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight))
				{
					MemoryTopByte = 0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight);
					InvalidateRect(hWin, NULL, true);
				}
				MemoryCaretY = rct.bottom / FixedFontHeight - 1;
			}
			else
			{
				ScrollWindowEx(hWin, 0, -MemoryCaretY * FixedFontHeight + rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
				MemoryTopByte += BytesPerLine * (MemoryCaretY - rct.bottom / FixedFontHeight + 1);
				MemoryCaretY = rct.bottom / FixedFontHeight - 1;
			}
		}
		if (MemoryCaret)
		{
			SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
			ShowCaret(hWin);
		}
		if (wParam == VK_APPS)
		{
			if (!CreateBankMenu(pGameBoy, GetSubMenu(hPopupMenu, 0), 1))
			{
				Point.x = MemoryCaretX * FixedFontWidth;
				Point.y = (MemoryCaretY + 1) * FixedFontHeight;
				ClientToScreen(hWin, &Point);
				TrackPopupMenu(GetSubMenu(hPopupMenu, 0), TPM_LEFTBUTTON, Point.x, Point.y, 0, hWin, NULL);
			}
		}
		pGameBoy->Release();
		return 0;

	case WM_VSCROLL:
		if (!GameBoys.GetActive(false))
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		CalcBytesPerLine;
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			if (MemoryTopByte == 0)
			{
				return 0;
			}
			if (MemoryTopByte < BytesPerLine)
			{
				MemoryTopByte = 0;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}
			MemoryTopByte -= BytesPerLine;
			MemoryCaretY++;
			if (MemoryCaret)
			{
				HideCaret(hWin);
				SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
			}
			ScrollWindowEx(hWin, 0, FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWin);
			if (MemoryCaret)
			{
				ShowCaret(hWin);
			}
			break;

		case SB_LINEDOWN:
			if (MemoryTopByte + BytesPerLine * (rct.bottom / FixedFontHeight) > 0xFFFF)
			{
				return 0;
			}
			MemoryTopByte += BytesPerLine;
			MemoryCaretY--;
			if (MemoryCaret)
			{
				HideCaret(hWin);
				SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
			}
			ScrollWindowEx(hWin, 0, -FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWin);
			if (MemoryCaret)
			{
				ShowCaret(hWin);
			}
			break;

		case SB_PAGEUP:
			if (MemoryTopByte == 0)
			{
				return 0;
			}
			GetClientRect(hWin, &rct);
			if (MemoryTopByte <= BytesPerLine * (rct.bottom / FixedFontHeight))
			{
				MemoryCaretY += MemoryTopByte / BytesPerLine;
				MemoryTopByte = 0;
			}
			else
			{
				MemoryCaretY += rct.bottom / FixedFontHeight - 1;
				MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_PAGEDOWN:
			GetClientRect(hWin, &rct);
			if (MemoryTopByte + BytesPerLine * (rct.bottom / FixedFontHeight - 1) > 0xFFFF)
			{
				MemoryCaretY -= ((0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight - 1)) - MemoryTopByte) / BytesPerLine;
				MemoryTopByte = 0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			else
			{
				MemoryCaretY -= rct.bottom / FixedFontHeight - 1;
				MemoryTopByte += BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			MemoryCaretY -= ((MemoryTopByte % BytesPerLine + BytesPerLine * HIWORD(wParam)) - MemoryTopByte) / BytesPerLine;
			MemoryTopByte = MemoryTopByte % BytesPerLine + BytesPerLine * HIWORD(wParam);
			InvalidateRect(hWin, NULL, true);
			UpdateWindow(hWin);
			break;
		}
		return 0;

	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		CalcBytesPerLine;
		MemoryCaretX = LOWORD(lParam) / FixedFontWidth;
		MemoryCaretY = HIWORD(lParam) / FixedFontHeight;
		if (MemoryTopByte + BytesPerLine * MemoryCaretY > 0xFFFF)
		{
			MemoryCaretY--;
		}
		if (MemoryCaret)
		{
			HideCaret(hWin);
			SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
			ShowCaret(hWin);
		}
		if (MemoryCaretY * FixedFontHeight > rct.bottom - FixedFontHeight)
		{
			SendMessage(hWin, WM_VSCROLL, SB_LINEDOWN, 0);
		}
		pGameBoy->Release();
		return 0;

	case WM_RBUTTONUP:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		if (!CreateBankMenu(pGameBoy, GetSubMenu(hPopupMenu, 0), 1))
		{
			GetCursorPos(&Point);
			TrackPopupMenu(GetSubMenu(hPopupMenu, 0), TPM_LEFTBUTTON, Point.x, Point.y, 0, hWin, NULL);
		}
		pGameBoy->Release();
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoys.GetActive(true))
			{
				if (MemoryCaret)
				{
					HideCaret(hWin);
				}

				lb.lbStyle = BS_SOLID;
				lb.lbColor = 0xFF;
				hBrush = (HBRUSH)SelectObject(Paint.hdc, CreateBrushIndirect(&lb));
				hPen = (HPEN)SelectObject(Paint.hdc, CreatePen(PS_SOLID, 0, 0xFF));

				SelectObject(Paint.hdc, hFixedFont);
				SetBkColor(Paint.hdc, 0xFFFFFF);
				SetTextColor(Paint.hdc, 0);

				GetClientRect(hWin, &rct);

				CalcBytesPerLine;

				while (MemoryTopByte + BytesPerLine * (rct.bottom / FixedFontHeight - 1) > 0xFFFF)
				{
					MemoryTopByte -= BytesPerLine;
					MemoryCaretY++;
				}

				pByte = MemoryTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					if (pByte >= 0x4000 && pByte < 0x8000)
					{
						if (MemoryFlags & MEMORY_ROM)
						{
							TextOut(Paint.hdc, 0, y, ToHex(MemoryROM, false), 2);
						}
						else
						{
							TextOut(Paint.hdc, 0, y, ToHex(pGameBoy->ActiveRomBank, false), 2);
						}
					}
					else if (pByte >= 0x8000 && pByte < 0xA000)
					{
						if (MemoryFlags & MEMORY_VBK)
						{
							TextOut(Paint.hdc, 0, y, ToHex(MemoryVBK, false), 2);
						}
						else
						{
							TextOut(Paint.hdc, 0, y, ToHex(pGameBoy->MEM_CPU[0x8F4F] & 1, false), 2);
						}
					}
					else if (pByte >= 0xA000 && pByte < 0xC000)
					{
						if (MemoryFlags & MEMORY_RAM)
						{
							TextOut(Paint.hdc, 0, y, ToHex(MemoryRAM, false), 2);
						}
						else
						{
							TextOut(Paint.hdc, 0, y, ToHex(pGameBoy->ActiveRamBank, false), 2);
						}
					}
					else if (pByte >= 0xD000 && pByte < 0xE000)
					{
						if (MemoryFlags & MEMORY_SVBK)
						{
							TextOut(Paint.hdc, 0, y, ToHex(MemorySVBK, false), 2);
						}
						else
						{
							if (!(pGameBoy->MEM_CPU[0x8F70] & 7))
							{
								TextOut(Paint.hdc, 0, y, "01", 2);
							}
							else
							{
								TextOut(Paint.hdc, 0, y, ToHex(pGameBoy->MEM_CPU[0x8F70] & 7, false), 2);
							}
						}
					}
					else
					{
						TextOut(Paint.hdc, 0, y, "00", 2);
					}
					TextOut(Paint.hdc, 2 * FixedFontWidth, y, ":", 1);

					TextOut(Paint.hdc, 3 * FixedFontWidth, y, ToHex(pByte, true), 4);
					x = 0;
					nBytes = 0;
					while (nBytes++ < BytesPerLine)
					{
						if (pByte < 0x4000)
						{
							Byte = pGameBoy->MEM_ROM[pByte];
							Access = pGameBoy->MemStatus_ROM[pByte];
						}
						else if (pByte < 0x8000)
						{
							if (MemoryFlags & MEMORY_ROM)
							{
								Byte = pGameBoy->MEM_ROM[pByte - 0x4000 + 0x4000 * MemoryROM];
								Access = pGameBoy->MemStatus_ROM[pByte - 0x4000 + 0x4000 * MemoryROM];
							}
							else
							{
								Byte = pGameBoy->MEM_ROM[pByte - 0x4000 + 0x4000 * pGameBoy->ActiveRomBank];
								Access = pGameBoy->MemStatus_ROM[pByte - 0x4000 + 0x4000 * pGameBoy->ActiveRomBank];
							}
						}
						else if (pByte < 0xA000)
						{
							if (MemoryFlags & MEMORY_VBK)
							{
								Byte = pGameBoy->MEM_VRAM[pByte - 0x8000 + 0x2000 * MemoryVBK];
								Access = pGameBoy->MemStatus_VRAM[pByte - 0x8000 + 0x2000 * MemoryVBK];
							}
							else
							{
								Byte = pGameBoy->MEM_VRAM[pByte - 0x8000 + 0x2000 * (pGameBoy->MEM_CPU[0x8F4F] & 1)];
								Access = pGameBoy->MemStatus_VRAM[pByte - 0x8000 + 0x2000 * (pGameBoy->MEM_CPU[0x8F4F] & 1)];
							}
						}
						else if (pByte < 0xC000)
						{
							if (pGameBoy->MemStatus_RAM)
							{
								if (MemoryFlags & MEMORY_RAM)
								{
									Access = pGameBoy->MemStatus_RAM[pByte - 0xA000 + 0x2000 * MemoryRAM];
									if (Access & MEM_READ)
									{
										Byte = pGameBoy->MEM_RAM[pByte - 0xA000 + 0x2000 * MemoryRAM];
									}
								}
								else
								{
									Access = pGameBoy->MemStatus_RAM[pByte - 0xA000 + 0x2000 * pGameBoy->ActiveRamBank];
									if (Access & MEM_READ)
									{
										Byte = pGameBoy->MEM_RAM[pByte - 0xA000 + 0x2000 * pGameBoy->ActiveRamBank];
									}
								}
							}
							else
							{
								Access = 0;
							}
						}
						else if (pByte < 0xD000)
						{
							Byte = pGameBoy->MEM_CPU[pByte - 0xC000];
							Access = pGameBoy->MemStatus_CPU[pByte - 0xC000];
						}
						else if (pByte < 0xE000)
						{
							if (!(MemoryFlags & MEMORY_SVBK))
							{
								MemorySVBK = pGameBoy->MEM_CPU[0x8F70] & 7;
								if (MemorySVBK < 1)
								{
									MemorySVBK = 1;
								}
							}
							Byte = pGameBoy->MEM_CPU[pByte - 0xD000 + 0x1000 * MemorySVBK];
							Access = pGameBoy->MemStatus_CPU[pByte - 0xD000 + 0x1000 * MemorySVBK];
						}
						else
						{
							Access = RetrieveAccess(pGameBoy, pByte);
							if (Access & MEM_READ)
							{
								Byte = (BYTE)ReadMem(pGameBoy, pByte);
							}
						}
						if (!(Access & MEM_READ))
						{
							TextOut(Paint.hdc, 9 * FixedFontWidth + 3 * x, y, "??", 2);
							TextOut(Paint.hdc, (10 + 3 * BytesPerLine) * FixedFontWidth + x, y, "?", 1);
						}
						else
						{
							if (pByte & 0x8000)
							{
								if (Access & MEM_CHANGED)
								{
									SetTextColor(Paint.hdc, RGB(0xFF, 0x00, 0x00));
								}
							}
							TextOut(Paint.hdc, 9 * FixedFontWidth + 3 * x, y, ToHex(Byte, false), 2);
							TextOut(Paint.hdc, (10 + 3 * BytesPerLine) * FixedFontWidth + x, y, (char *)&Byte, 1);
							SetTextColor(Paint.hdc, 0);
						}

						if (++pByte == 0)
						{
							pByte--;
							y = rct.bottom + 1;
							break;
						}
						x += FixedFontWidth;
					}

					y += FixedFontHeight;
				}

				hBrush = (HBRUSH)SelectObject(Paint.hdc, hBrush);
				DeleteObject(hBrush);
				hPen = (HPEN)SelectObject(Paint.hdc, hPen);
				DeleteObject(hPen);

				si.cbSize = sizeof(si);
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nMin = 0;
				si.nMax = 0x10000 / BytesPerLine;
				si.nPage = rct.bottom / FixedFontHeight;
				si.nPos = MemoryTopByte / BytesPerLine;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				si.nMax = 10 + 32 * FixedFontWidth;
				si.nPage = rct.right;
				SetScrollInfo(hWin, SB_HORZ, &si, true);

				if (MemoryCaret)
				{
					SetCaretPos(MemoryCaretX * FixedFontWidth, MemoryCaretY * FixedFontHeight);
					ShowCaret(hWin);
				}

				pGameBoy->Release();
			}
			else
			{
				if (MemoryCaret)
				{
					DestroyCaret();
					MemoryCaret = false;
				}

				si.cbSize = sizeof(si);
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nMin = 0;
				si.nMax = 0;
				si.nPage = 1;
				si.nPos = 0;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				SetScrollInfo(hWin, SB_HORZ, &si, true);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SIZE:
		GetClientRect(hWin, &rct);
		CalcBytesPerLine;
		if (MemoryCaretX > 10 + 4 * BytesPerLine)
		{
			MemoryCaretX = 10 + 4 * BytesPerLine;
		}
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		MemoryCaret = false;
		MemoryTopByte = 0;
		MemoryCaretX = 0;
		MemoryCaretY = 0;
		return 0;

	case WM_DESTROY:
		hMemory = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		Memory.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		Memory.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		Memory.Width = rct.right - rct.left;
		Memory.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



CGameBoy	*DisAsmGameBoy;
BOOL		DisAsmCaret;
int			DisAsmCaretX, DisAsmCaretY;
WORD		DisAsmTopByte, DisAsmCaretByte, DisAsmLastPC;
int			DisAsmScroll;

BYTE		DisAsmROM, DisAsmRAM, DisAsmSVBK, DisAsmVBK;

BOOL CALLBACK DisAsmRomBankEnumChildProc(HWND hWin, LPARAM lParam)
{
	char		szText[4];


	if (GetWindowLong(hWin, GWL_ID) != IDC_NUMBER)
	{
		return true;
	}

	if (lParam)
	{
		itoa(DisAsmROM, szText, 16);
		SendMessage(hWin, WM_SETTEXT, 0, (LPARAM)&szText);
	}
	else
	{
		SendMessage(hWin, WM_GETTEXT, 4, (LPARAM)&szText);
		if (szText[0] && szText[1] && szText[2])
		{
			return true;
		}

		if (HexToNum(&szText[0]))
		{
			return true;
		}

		if (szText[1] != '\0')
		{
			if (HexToNum(&szText[1]))
			{
				return true;
			}
			DisAsmROM = (szText[0] << 4) | szText[1];
		}
		else
		{
			DisAsmROM = szText[0];
		}

		DisAsmFlags |= MEMORY_ROM;
	}

	return false;
}



BOOL CALLBACK DisAsmGotoDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD		NewNumber;
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			NewNumber = DisAsmCaretByte;
			EnumChildWindows(hWin, GotoEnumChildProc, (LPARAM)&NewNumber);
			if (DisAsmCaretByte == NewNumber)
			{
				EndDialog(hWin, true);
				return true;
			}
			DisAsmTopByte = DisAsmCaretByte = (WORD)NewNumber;
			MemoryCaretY = 0;
			MemoryCaretX = 0;
			EndDialog(hWin, false);
			return true;

		case IDCANCEL:
			EndDialog(hWin, true);
			return true;
		}
		break;

	/*case WM_CLOSE:
		EndDialog(hWin, true);
		return true;*/

	case WM_INITDIALOG:
		SetWindowText(hWin, String(IDS_ENTERADDRESS));
		itoa(DisAsmCaretByte, NumBuffer, 16);
		EnumChildWindows(hWin, SetAddressEnumChildProc, (LPARAM)&NumBuffer);
		return true;
	}

	return false;
}



BOOL CALLBACK DisAsmRomBankDlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char		szBuffer[0x100];


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EnumChildWindows(hWin, DisAsmRomBankEnumChildProc, false);
			EndDialog(hWin, false);
			return true;

		case IDCANCEL:
			EndDialog(hWin, true);
			return true;
		}
		break;

	/*case WM_CLOSE:
		EndDialog(hWin, true);
		return true;*/

	case WM_INITDIALOG:
		SetWindowText(hWin, String(IDS_ENTERROMBANKNO));
		EnumChildWindows(hWin, DisAsmRomBankEnumChildProc, true);
		return true;
	}

	return false;
}



void DisAsmReadMem(CGameBoy *pGameBoy, WORD pByte, BYTE **p, BYTE **Access, BYTE *pBank)
{
	if (pByte < 0x4000)
	{
		*pBank = 0;
		*p = &pGameBoy->MEM_ROM[pByte];
		*Access = &pGameBoy->MemStatus_ROM[pByte];
	}
	else if (pByte < 0x8000)
	{
		if (DisAsmFlags & MEMORY_ROM)
		{
			*pBank = DisAsmROM;
			*p = &pGameBoy->MEM_ROM[pByte - 0x4000 + 0x4000 * DisAsmROM];
			*Access = &pGameBoy->MemStatus_ROM[pByte - 0x4000 + 0x4000 * DisAsmROM];
		}
		else
		{
			*pBank = pGameBoy->ActiveRomBank;
			*p = &pGameBoy->MEM_ROM[pByte - 0x4000 + 0x4000 * pGameBoy->ActiveRomBank];
			*Access = &pGameBoy->MemStatus_ROM[pByte - 0x4000 + 0x4000 * pGameBoy->ActiveRomBank];
		}
	}
	else if (pByte < 0xA000)
	{
		if (DisAsmFlags & MEMORY_VBK)
		{
			*pBank = DisAsmVBK;
			*p = &pGameBoy->MEM_VRAM[pByte - 0x8000 + 0x2000 * DisAsmVBK];
			*Access = &pGameBoy->MemStatus_VRAM[pByte - 0x8000 + 0x2000 * DisAsmVBK];
		}
		else
		{
			*pBank = pGameBoy->MEM_CPU[0x8F4F] & 1;
			*p = &pGameBoy->MEM_VRAM[pByte - 0x8000 + 0x2000 * (pGameBoy->MEM_CPU[0x8F4F] & 1)];
			*Access = &pGameBoy->MemStatus_VRAM[pByte - 0x8000 + 0x2000 * (pGameBoy->MEM_CPU[0x8F4F] & 1)];
		}
	}
	else if (pByte < 0xC000)
	{
		if (pGameBoy->MemStatus_RAM)
		{
			if (DisAsmFlags & MEMORY_RAM)
			{
				*pBank = DisAsmRAM;
				*Access = &pGameBoy->MemStatus_RAM[pByte - 0xA000 + 0x2000 * DisAsmRAM];
				if (**Access & MEM_READ)
				{
					*p = &pGameBoy->MEM_RAM[pByte - 0xA000 + 0x2000 * DisAsmRAM];
				}
			}
			else
			{
				*pBank = pGameBoy->ActiveRamBank;
				*Access = &pGameBoy->MemStatus_RAM[pByte - 0xA000 + 0x2000 * pGameBoy->ActiveRamBank];
				if (**Access & MEM_READ)
				{
					*p = &pGameBoy->MEM_RAM[pByte - 0xA000 + 0x2000 * pGameBoy->ActiveRamBank];
				}
			}
		}
		else
		{
			*pBank = 0;
			*Access = ZeroStatus;
		}
	}
	else if (pByte < 0xD000)
	{
		*pBank = 0;
		*p = &pGameBoy->MEM_CPU[pByte - 0xC000];
		*Access = &pGameBoy->MemStatus_CPU[pByte - 0xC000];
	}
	else if (pByte < 0xE000)
	{
		if (!(DisAsmFlags & MEMORY_SVBK))
		{
			DisAsmSVBK = pGameBoy->MEM_CPU[0x8F70] & 7;
			if (DisAsmSVBK < 1)
			{
				DisAsmSVBK = 1;
			}
		}
		*pBank = DisAsmSVBK;
		*p = &pGameBoy->MEM_CPU[pByte - 0xD000 + 0x1000 * DisAsmSVBK];
		*Access = &pGameBoy->MemStatus_CPU[pByte - 0xD000 + 0x1000 * DisAsmSVBK];
	}
	else if (pByte < 0xFE00)
	{
		*pBank = 0;
		*Access = ZeroStatus;
	}
	else if (pByte < 0xFEA0)
	{
		*pBank = 0;
		*p = &pGameBoy->MEM_CPU[pByte - 0xFE00 + 0x8E00];
		*Access = &pGameBoy->MemStatus_CPU[pByte - 0xFE00 + 0x8E00];
	}
	else if (pByte < 0xFF00)
	{
		*pBank = 0;
		*Access = ZeroStatus;
	}
	else
	{
		*pBank = 0;
		*p = &pGameBoy->MEM_CPU[pByte - 0xFF00 + 0x8F00];
		*Access = &pGameBoy->MemStatus_CPU[pByte - 0xFF00 + 0x8F00];
	}
}



BOOL OutputLabel(CGameBoy *pGameBoy, WORD Offset, HDC hdc, int y, int *x)
{
	char		*pszLabel;
	SIZE		Size;
	BYTE		*p, *Access, Bank;


	if (pGameBoy->pDebugInfo)
	{
		DisAsmReadMem(pGameBoy, Offset, &p, &Access, &Bank);

		pGameBoy->pDebugInfo->ResetSearch();
		if (pszLabel = pGameBoy->pDebugInfo->GetNextLabel(Bank, Offset))
		{
			TextOut(hdc, *x, y, pszLabel, strlen(pszLabel));
			GetTextExtentPoint32(hdc, pszLabel, strlen(pszLabel), &Size);
			*x += Size.cx;

			return true;
		}
	}

	return false;
}



void OutputLabels(CGameBoy *pGameBoy, BYTE Bank, WORD Offset, HDC hdc, int *y)
{
	char		*pszLabel;


	if (pGameBoy->pDebugInfo)
	{
		pGameBoy->pDebugInfo->ResetSearch();
		while (pszLabel = pGameBoy->pDebugInfo->GetNextLabel(Bank, Offset))
		{
			TextOut(hdc, 15, *y, pszLabel, strlen(pszLabel));
			*y += FixedFontHeight;
		}
	}
}



DWORD GetNumberOfLabels(CGameBoy *pGameBoy, BYTE Bank, WORD Offset)
{
	DWORD		dwLabelNo;


	dwLabelNo = 0;

	if (pGameBoy->pDebugInfo)
	{
		pGameBoy->pDebugInfo->ResetSearch();
		while (pGameBoy->pDebugInfo->GetNextLabel(Bank, Offset))
		{
			dwLabelNo++;
		}
	}

	return dwLabelNo;
}



BOOL TransparentBlitter(HDC hdcDest, int Left, int Top, int Width, int Height, HDC hdcSrc, HANDLE hbmpSrc)
{
	HBITMAP			hbmp, hbmpOld, hbmpMask, hbmpMaskOld;
	HDC				hdc, hdcMask;
	DWORD			*pBitmap;
	int				x, y;
	BITMAPINFO		bmi;
	COLORREF		crTransparent;


	hbmpMask = CreateBitmap(Width, Height, 1, 1, NULL);
	hdcMask = CreateCompatibleDC(hdcDest);
	hbmpMaskOld = (HBITMAP)SelectObject(hdcMask, hbmpMask);

	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = Width;
	bmi.bmiHeader.biHeight = -Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hbmp = CreateDIBSection(hdcDest, &bmi, DIB_RGB_COLORS, (void **)&pBitmap, NULL, 0);
	hdc = CreateCompatibleDC(hdcDest);
	hbmpOld = (HBITMAP)SelectObject(hdc, hbmp);


	PatBlt(hdcMask, 0, 0, Width, Height, BLACKNESS);

	BitBlt(hdc, 0, 0, Width, Height, hdcSrc, 0, 0, SRCCOPY);
	crTransparent = pBitmap[0];

	for (y = 0; y < Height; y++)
	{
		for (x = 0; x < Width; x++)
		{
			if (pBitmap[y * Width + x] == crTransparent)
			{
				pBitmap[y * Width + x] = RGB(0x00, 0x00, 0x00);
				SetPixel(hdcMask, x, y, RGB(0xFF, 0xFF, 0xFF));
			}
			else
			{
			}
		}
	}

	BitBlt(hdcDest, Left, Top, Width, Height, hdcMask, 0, 0, SRCAND);
	BitBlt(hdcDest, Left, Top, Width, Height, hdc, 0, 0, SRCPAINT);


	hbmp = (HBITMAP)SelectObject(hdc, hbmpOld);
	DeleteObject(hbmp);
	DeleteDC(hdc);

	hbmpMask = (HBITMAP)SelectObject(hdcMask, hbmpMaskOld);
	DeleteObject(hbmpMask);
	DeleteDC(hdcMask);

	return false;
}



LPARAM CALLBACK DisAsmWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;
	POINT			Point;
	LOGBRUSH		lb;
	HDC				hdc;
	HBRUSH			hBrush;
	HPEN			hPen;
	HANDLE			hBitmap;
	SCROLLINFO		si;
	WORD			pByte, pByte2;
	BYTE			Byte, *p, *p2, *p3, *Access, *Access2, *Access3, Bank;
	DWORD			dw;
	int				x, y;
	BOOL			SetY, Label;
	EMULATIONINFO	EmulationInfo;
	char			szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_DISASSEMBLY));
		return 0;

	case WM_COMMAND:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			break;
		}
		switch (LOWORD(wParam))
		{
		case ID_EDIT_GOTO:
			if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENTERNUMBER), hWnd, DisAsmGotoDlgProc))
			{
				InvalidateRect(hWin, NULL, true);
			}
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_ROM:
			if (!(DisAsmFlags & MEMORY_ROM))
			{
				DisAsmROM = pGameBoy->ActiveRomBank;
			}
			Byte = DisAsmROM;
			if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENTERNUMBER), hWnd, DisAsmRomBankDlgProc))
			{
				if (DisAsmROM <= pGameBoy->MaxRomBank)
				{
					InvalidateRect(hWin, NULL, true);
				}
				else
				{
					DisAsmROM = Byte;
				}
			}
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_VBK_BANK0:
			DisAsmFlags |= MEMORY_VBK;
			DisAsmVBK = 0;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_VBK_BANK1:
			DisAsmFlags |= MEMORY_VBK;
			DisAsmVBK = 1;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK0:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 0;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK1:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 1;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK2:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 2;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK3:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 3;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK4:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 4;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK5:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 5;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK6:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 6;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK7:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 7;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK8:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 8;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK9:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 9;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK10:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 10;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK11:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 11;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK12:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 12;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK13:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 13;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK14:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 14;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_RAM_BANK15:
			DisAsmFlags |= MEMORY_RAM;
			DisAsmRAM = 15;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK1:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 1;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK2:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 2;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK3:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 3;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK4:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 4;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK5:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 5;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK6:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 6;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_MEMORY_SVBK_BANK7:
			DisAsmFlags |= MEMORY_SVBK;
			DisAsmSVBK = 7;
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case ID_EMULATION_STEPOVER:
			if (OpCodeNames[(BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC)].Flags & OCF_CALL)
			{
				if (pGameBoy->IsEmulating())
				{
					pGameBoy->Release();
					return 0;
				}
				EmulationInfo.Flags = EMU_RUNTO;
				EmulationInfo.RunToOffset = pGameBoy->Reg_PC + ((BYTE)OpCodeNames[(BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC)].Flags & OCF_BYTES) + 1;
				if (EmulationInfo.RunToOffset >= 0x4000 && EmulationInfo.RunToOffset < 0x8000)
				{
					EmulationInfo.RunToBank = pGameBoy->ActiveRomBank;
				}
				if (EmulationInfo.RunToOffset >= 0xA000 && EmulationInfo.RunToOffset < 0xC000)
				{
					EmulationInfo.RunToBank = pGameBoy->ActiveRamBank;
				}
				if (EmulationInfo.RunToOffset >= 0xD000 && EmulationInfo.RunToOffset < 0xE000 && pGameBoy->Flags & GB_ROM_COLOR)
				{
					if (pFF00_C(pGameBoy, 0x4F) & 7)
					{
						EmulationInfo.RunToBank = pFF00_C(pGameBoy, 0x4F) & 7;
					}
					else
					{
						EmulationInfo.RunToBank = 1;
					}
				}
				pGameBoy->Step(&EmulationInfo);
				pGameBoy->Release();
				return 0;
			}
			//If PC isn't on a call instruction, Step Over and Step Into are the same

		case ID_EMULATION_STEPINTO:
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			EmulationInfo.Flags = EMU_STEPINTO;
			pGameBoy->Step(&EmulationInfo);
			pGameBoy->Release();
			return 0;

		case ID_EMULATION_STEPOUT:
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			EmulationInfo.Flags = EMU_STEPOUT;
			pGameBoy->Step(&EmulationInfo);
			pGameBoy->Release();
			return 0;

		case ID_EMULATION_RUNTOCURSOR:
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Release();
				return 0;
			}
			EmulationInfo.Flags = EMU_RUNTO;
			EmulationInfo.RunToOffset = DisAsmCaretByte;
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &EmulationInfo.RunToBank);
			pGameBoy->Step(&EmulationInfo);
			pGameBoy->Release();
			return 0;

		case ID_EMULATION_SETNEXTSTATEMENT:
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);
			if (DisAsmCaretByte >= 0x4000 && DisAsmCaretByte < 0x8000)
			{
				if (pGameBoy->ActiveRomBank != Bank)
				{
					pGameBoy->SwitchRomBank(Bank);
					InvalidateRect(hWin, NULL, true);
					if (hRegisters)
					{
						InvalidateRect(hWin, NULL, true);
					}
				}
			}
			else if (DisAsmCaretByte >= 0x8000 && DisAsmCaretByte < 0xA000 && (pGameBoy->Flags & GB_ROM_COLOR))
			{
				if ((pGameBoy->MEM_CPU[0x8F4F] & 1) != Bank)
				{
					pGameBoy->SwitchVBK(Bank);
					InvalidateRect(hWin, NULL, true);
					if (hRegisters)
					{
						InvalidateRect(hWin, NULL, true);
					}
				}
			}
			else if (DisAsmCaretByte >= 0xA000 && DisAsmCaretByte < 0xC000)
			{
				if (pGameBoy->ActiveRamBank != Bank)
				{
					pGameBoy->SwitchRamBank(Bank);
					InvalidateRect(hWin, NULL, true);
					if (hRegisters)
					{
						InvalidateRect(hWin, NULL, true);
					}
				}
			}
			else if (DisAsmCaretByte >= 0xD000 && DisAsmCaretByte < 0xE000 && (pGameBoy->Flags & GB_ROM_COLOR))
			{
				if (pGameBoy->MEM_CPU[0x8F70] & 7)
				{
					if ((pGameBoy->MEM_CPU[0x8F70] & 7) != Bank)
					{
						pGameBoy->SwitchSVBK(Bank);
						InvalidateRect(hWin, NULL, true);
						if (hRegisters)
						{
							InvalidateRect(hWin, NULL, true);
						}
					}
				}
				else
				{
					if (Bank != 1)
					{
						pGameBoy->SwitchSVBK(Bank);
						InvalidateRect(hWin, NULL, true);
						if (hRegisters)
						{
							InvalidateRect(hWin, NULL, true);
						}
					}
				}
			}
			if (pGameBoy->Reg_PC != DisAsmCaretByte)
			{
				pGameBoy->Reg_PC = DisAsmCaretByte;
				InvalidateRect(hWin, NULL, true);
				if (hRegisters)
				{
					InvalidateRect(hRegisters, NULL, true);
				}
			}
			pGameBoy->Release();
			return 0;

		case ID_EMULATION_TOGGLEBREAKPOINT:
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);

			//Breakpoints cannot be placed on non-executable statements
			if (!(*Access & MEM_EXECUTE))
			{
				pGameBoy->Release();
				return 0;
			}

			if (*Access & MEM_BREAKPOINT)
			{
				*Access &= ~MEM_BREAKPOINT;
			}
			else
			{
				*Access |= MEM_BREAKPOINT;
			}
			GetClientRect(hWin, &rct);
			rct.right = 15;
			InvalidateRect(hWin, &rct, true);
			pGameBoy->Release();
			return 0;
		}
		pGameBoy->Release();
		break;

	case WM_MENUSELECT:
		return SendMessage(hWnd, uMsg, wParam, lParam);

	case WM_EXITMENULOOP:
		SetStatus(NULL, SF_READY);
		return 0;

	case WM_SETFOCUS:
		if (GameBoys.GetActive(false))
		{
			CreateCaret(hWin, NULL, 2, 16);
			SetCaretPos(DisAsmCaretX, DisAsmCaretY);
			ShowCaret(hWin);
			DisAsmCaret = true;
		}
		return 0;

	case WM_KILLFOCUS:
		if (DisAsmCaret)
		{
			DestroyCaret();
			DisAsmCaret = false;
		}
		return 0;

	case WM_KEYDOWN:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			break;
		}

		/*switch (wParam)
		{
		case VK_NUMPAD0:
			wParam = '0';
			break;

		case VK_NUMPAD1:
			wParam = '1';
			break;

		case VK_NUMPAD2:
			wParam = '2';
			break;

		case VK_NUMPAD3:
			wParam = '3';
			break;

		case VK_NUMPAD4:
			wParam = '4';
			break;

		case VK_NUMPAD5:
			wParam = '5';
			break;

		case VK_NUMPAD6:
			wParam = '6';
			break;

		case VK_NUMPAD7:
			wParam = '7';
			break;

		case VK_NUMPAD8:
			wParam = '8';
			break;

		case VK_NUMPAD9:
			wParam = '9';
			break;
		}*/

		switch (wParam)
		{
		case VK_UP:
			if (DisAsmCaretByte == 0)
			{
				pGameBoy->Release();
				return 0;
			}
			GetClientRect(hWin, &rct);
			if (DisAsmCaretY > rct.bottom || DisAsmCaretY < 0)
			{
				DisAsmCaretByte--;
				DisAsmTopByte = DisAsmCaretByte;
				DisAsmCaretY = 0;
				InvalidateRect(hWin, NULL, true);
			}
			else
			{
				pByte2 = DisAsmCaretByte--;
				pByte = DisAsmTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					DisAsmReadMem(pGameBoy, pByte, &p, &Access, &Bank);
					y += GetNumberOfLabels(pGameBoy, Bank, pByte) * FixedFontHeight;

					if (DisAsmCaretByte == pByte)
					{
						break;
					}

					if (*Access & MEM_READ)
					{
						if (*p != 0xCB)
						{
							if (OpCodeNames[*p].Flags & OCF_BYTES)
							{
								if (DisAsmCaretByte == ++pByte)
								{
									DisAsmCaretByte--;
									break;
								}

								if (OpCodeNames[*p].Flags & OCF_DATA16)
								{
									if (DisAsmCaretByte == ++pByte)
									{
										DisAsmCaretByte -= 2;
										break;
									}
								}
							}
						}
						else
						{
							if (DisAsmCaretByte == ++pByte)
							{
								DisAsmCaretByte--;
								break;
							}
						}
					}
					else
					{
						if (DisAsmCaretByte == ++pByte)
						{
							DisAsmCaretByte--;
							break;
						}
					}
					pByte++;
					y += FixedFontHeight;
				}

				DisAsmReadMem(pGameBoy, pByte2, &p, &Access, &Bank);
				if ((DisAsmCaretY -= (GetNumberOfLabels(pGameBoy, Bank, pByte2) + 1) * FixedFontHeight) < 0)
				{
					SendMessage(hWin, WM_VSCROLL, SB_LINEUP, NULL);
				}
				else
				{
					if (DisAsmCaret)
					{
						HideCaret(hWin);
						SetCaretPos(DisAsmCaretX, DisAsmCaretY);
						ShowCaret(hWin);
					}
				}
			}
			pGameBoy->Release();
			return 0;

		case VK_DOWN:
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);
			if (*Access & MEM_READ)
			{
				Byte = 1 + (BYTE)(OpCodeNames[*p].Flags & OCF_BYTES);
			}
			else
			{
				Byte = 1;
			}
			if (DisAsmCaretByte + Byte > 0xFFFF)
			{
				pGameBoy->Release();
				return 0;
			}
			DisAsmCaretByte += Byte;
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);
			dw = GetNumberOfLabels(pGameBoy, Bank, DisAsmCaretByte) * FixedFontHeight;
			if (DisAsmCaretByte == DisAsmTopByte)
			{
				DisAsmCaretY = dw;
				if (DisAsmCaret)
				{
					HideCaret(hWin);
					SetCaretPos(DisAsmCaretX, DisAsmCaretY);
					ShowCaret(hWin);
				}
				pGameBoy->Release();
				return 0;
			}
			GetClientRect(hWin, &rct);
			DisAsmCaretY += dw;
			if (DisAsmCaretY < 0 || DisAsmCaretY > rct.bottom)
			{
				DisAsmTopByte = DisAsmCaretByte;
				InvalidateRect(hWin, NULL, true);
			}
			else
			{
				if (DisAsmCaretY + FixedFontHeight - 1 > rct.bottom)
				{
					SendMessage(hWin, WM_VSCROLL, SB_LINEDOWN, NULL);
					SendMessage(hWin, WM_VSCROLL, SB_LINEDOWN, NULL);
				}
				else
				{
					if (DisAsmCaretY + 2 * FixedFontHeight - 1 > rct.bottom)
					{
						SendMessage(hWin, WM_VSCROLL, SB_LINEDOWN, NULL);
					}
					else
					{
						DisAsmCaretY += FixedFontHeight;
						if (DisAsmCaret)
						{
							HideCaret(hWin);
							SetCaretPos(DisAsmCaretX, DisAsmCaretY);
							ShowCaret(hWin);
						}
					}
				}
			}
			pGameBoy->Release();
			return 0;

		case VK_PRIOR:
			GetClientRect(hWin, &rct);
			if (DisAsmCaretY < 0 || DisAsmCaretY >= rct.bottom)
			{
				if (DisAsmCaretByte < rct.bottom / FixedFontHeight - 1)
				{
					DisAsmCaretByte = 0;
				}
				else
				{
					DisAsmCaretByte -= rct.bottom / FixedFontHeight - 1;
				}
				DisAsmTopByte = DisAsmCaretByte;
			}
			else
			{
				if (DisAsmTopByte == 0)
				{
					if (DisAsmCaretByte != 0)
					{
						DisAsmCaretByte = 0;
						DisAsmCaretY = 0;
						if (DisAsmCaret)
						{
							HideCaret(hWin);
							SetCaretPos(DisAsmCaretX, DisAsmCaretY);
							ShowCaret(hWin);
						}
					}
					pGameBoy->Release();
					return 0;
				}
				if (DisAsmTopByte < rct.bottom / FixedFontHeight - 1)
				{
					DisAsmTopByte = 0;
				}
				else
				{
					DisAsmTopByte -= rct.bottom / FixedFontHeight - 1;
				}
				DisAsmCaretByte = DisAsmTopByte;
				y = 0;
				while (y < DisAsmCaretY)
				{
					DisAsmReadMem(pGameBoy, DisAsmCaretByte++, &p, &Access, &Bank);

					if (*Access & MEM_READ)
					{
						DisAsmCaretByte += (BYTE)OpCodeNames[*p].Flags & OCF_BYTES;
					}

					dw = GetNumberOfLabels(pGameBoy, Bank, DisAsmCaretByte);
					DisAsmCaretByte += (WORD)dw;
					y += dw;

					y += FixedFontHeight;
				}
			}
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case VK_NEXT:
			GetClientRect(hWin, &rct);
			y = 0;
			while (y + 2 * FixedFontHeight < rct.bottom)
			{
				if (DisAsmCaretByte == 0xFFFF)
				{
					break;
				}
				DisAsmReadMem(pGameBoy, DisAsmCaretByte++, &p, &Access, &Bank);
				DisAsmTopByte++;

				if (DisAsmCaretByte == 0xFFFF)
				{
					break;
				}
				if (*Access & MEM_READ)
				{
					if (OpCodeNames[*p].Flags & OCF_BYTES)
					{
						DisAsmCaretByte++;
						DisAsmTopByte++;

						if (OpCodeNames[*p].Flags & OCF_DATA16)
						{
							if (DisAsmCaretByte == 0xFFFF)
							{
								break;
							}
							DisAsmCaretByte++;
							DisAsmTopByte++;
						}
					}
				}
				y += FixedFontHeight;
			}
			if (DisAsmCaretY < 0 || DisAsmCaretY >= rct.bottom)
			{
				DisAsmTopByte = DisAsmCaretByte;
			}
			InvalidateRect(hWin, NULL, true);
			pGameBoy->Release();
			return 0;

		case VK_APPS:
			GetClientRect(hWin, &rct);
			if (DisAsmCaretY > rct.bottom || DisAsmCaretY < 0)
			{
				DisAsmTopByte = DisAsmCaretByte;
				DisAsmCaretX = 15;
				DisAsmCaretY = 0;
				InvalidateRect(hWin, NULL, true);
				UpdateWindow(hWin);
			}
			if (!CreateBankMenu(pGameBoy, GetSubMenu(hPopupMenu, 1), 5))
			{
				Point.x = DisAsmCaretX;
				Point.y = DisAsmCaretY + FixedFontHeight;
				ClientToScreen(hWin, &Point);
				TrackPopupMenu(GetSubMenu(hPopupMenu, 1), TPM_LEFTBUTTON, Point.x, Point.y, 0, hWin, NULL);
			}
			pGameBoy->Release();
			return 0;
		}
		pGameBoy->Release();
		break;

	case WM_VSCROLL:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			if (DisAsmTopByte == 0)
			{
				pGameBoy->Release();
				return 0;
			}
			DisAsmTopByte--;
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_LINEDOWN:
			GetClientRect(hWin, &rct);
			DisAsmReadMem(pGameBoy, DisAsmTopByte, &p, &Access, &Bank);
			if (*Access & MEM_READ)
			{
				Byte = 1 + (BYTE)(OpCodeNames[*p].Flags & OCF_BYTES);
			}
			else
			{
				Byte = 1;
			}
			if (DisAsmTopByte + Byte > 0xFFFF)
			{
				pGameBoy->Release();
				return 0;
			}
			DisAsmTopByte += Byte;
			if (DisAsmCaret)
			{
				HideCaret(hWin);
				SetCaretPos(DisAsmCaretX, DisAsmCaretY);
			}
			InvalidateRect(hWin, NULL, true);
			if (DisAsmCaret)
			{
				ShowCaret(hWin);
			}
			break;

		case SB_PAGEUP:
			if (DisAsmTopByte == 0)
			{
				pGameBoy->Release();
				return 0;
			}
			GetClientRect(hWin, &rct);
			if (DisAsmTopByte - rct.bottom / FixedFontHeight > DisAsmTopByte)
			{
				DisAsmTopByte = 0;
			}
			else
			{
				DisAsmTopByte -= rct.bottom / FixedFontHeight;
			}
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_PAGEDOWN:
			GetClientRect(hWin, &rct);
			y = 0;
			while (y + 2 * FixedFontHeight <= rct.bottom)
			{
				DisAsmReadMem(pGameBoy, DisAsmTopByte++, &p, &Access, &Bank);

				if (*Access & MEM_READ)
				{
					DisAsmTopByte += (BYTE)OpCodeNames[*p].Flags & OCF_BYTES;
				}
				y += FixedFontHeight;
			}

			InvalidateRect(hWin, NULL, true);
			break;

		case SB_THUMBTRACK:
			pByte = DisAsmTopByte;
			DisAsmTopByte = HIWORD(wParam);
			InvalidateRect(hWin, NULL, true);
			UpdateWindow(hWin);
			DisAsmTopByte = pByte;
			break;

		case SB_THUMBPOSITION:
			DisAsmTopByte = HIWORD(wParam);
			InvalidateRect(hWin, NULL, true);
			break;
		}
		pGameBoy->Release();
		return 0;

	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		DisAsmCaretByte = DisAsmTopByte;
		y = 0;
		while (y + FixedFontHeight < HIWORD(lParam))
		{
			DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);
			dw = GetNumberOfLabels(pGameBoy, Bank, DisAsmCaretByte) * FixedFontHeight;
			if (++DisAsmCaretByte == 0)
			{
				DisAsmCaretByte = 0xFFFF;
				break;
			}

			if (*Access & MEM_READ)
			{
				if (OpCodeNames[*p].Flags & OCF_BYTES)
				{
					DisAsmCaretByte++;
					if (DisAsmCaretByte == 0)
					{
						DisAsmCaretByte = 0xFFFF;
						break;
					}

					if (OpCodeNames[*p].Flags & OCF_DATA16)
					{
						DisAsmCaretByte++;
						if (DisAsmCaretByte == 0)
						{
							DisAsmCaretByte = 0xFFFF;
							break;
						}
					}
				}
			}
			y += FixedFontHeight + dw;
		}
		DisAsmReadMem(pGameBoy, DisAsmCaretByte, &p, &Access, &Bank);
		DisAsmCaretY = y + GetNumberOfLabels(pGameBoy, Bank, DisAsmCaretByte) * FixedFontHeight;
		if (DisAsmCaret)
		{
			HideCaret(hWin);
			SetCaretPos(DisAsmCaretX, DisAsmCaretY);
			ShowCaret(hWin);
		}
		pGameBoy->Release();
		return 0;

	case WM_RBUTTONUP:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		if (!CreateBankMenu(pGameBoy, GetSubMenu(hPopupMenu, 1), 5))
		{
			GetCursorPos(&Point);
			TrackPopupMenu(GetSubMenu(hPopupMenu, 1), TPM_LEFTBUTTON, Point.x, Point.y, 0, hWin, NULL);
		}
		pGameBoy->Release();
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoys.GetActive(true))
			{
				if (DisAsmCaret)
				{
					HideCaret(hWin);
				}

				lb.lbStyle = BS_SOLID;
				lb.lbColor = 0xFF;
				hBrush = (HBRUSH)SelectObject(Paint.hdc, CreateBrushIndirect(&lb));
				hPen = (HPEN)SelectObject(Paint.hdc, CreatePen(PS_SOLID, 0, 0xFF));

				SelectObject(Paint.hdc, hFixedFont);
				SetBkColor(Paint.hdc, 0xFFFFFF);
				SetTextColor(Paint.hdc, 0);

				GetClientRect(hWin, &rct);

				if (DisAsmGameBoy != pGameBoy)
				{
					DisAsmGameBoy = pGameBoy;
					DisAsmCaretByte = DisAsmTopByte = DisAsmLastPC = pGameBoy->Reg_PC;
					DisAsmScroll = 0;
				}
				if (DisAsmLastPC != pGameBoy->Reg_PC)
				{
					DisAsmCaretByte = DisAsmLastPC = pGameBoy->Reg_PC;
					if (DisAsmCaretByte < DisAsmTopByte)
					{
						DisAsmTopByte = DisAsmCaretByte;
					}
					else
					{
						y = 0;
						pByte = DisAsmTopByte;
						while (true)
						{
							DisAsmReadMem(pGameBoy, pByte, &p, &Access, &Bank);
							if (*Access & MEM_READ)
							{
								pByte += 1 + (BYTE)(OpCodeNames[*p].Flags & OCF_BYTES);
							}
							else
							{
								pByte++;
							}
							y += FixedFontHeight;
							if (pByte > DisAsmCaretByte)
							{
								if (y > rct.bottom - 2 * FixedFontHeight)
								{
									DisAsmReadMem(pGameBoy, DisAsmTopByte, &p, &Access, &Bank);
									if (*Access & MEM_READ)
									{
										DisAsmTopByte += 1 + (BYTE)(OpCodeNames[*p].Flags & OCF_BYTES);
									}
									else
									{
										DisAsmTopByte++;
									}
								}
								break;
							}
							if (y > rct.bottom - FixedFontHeight)
							{
								DisAsmTopByte = DisAsmCaretByte;
								break;
							}
						}
					}
				}

				SetY = false;

				pByte = DisAsmTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					if (pByte >= 0x4000 && pByte < 0x8000)
					{
						if (DisAsmFlags & MEMORY_ROM)
						{
							OutputLabels(pGameBoy, DisAsmROM, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(DisAsmROM, false), 2);
						}
						else
						{
							OutputLabels(pGameBoy, pGameBoy->ActiveRomBank, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(pGameBoy->ActiveRomBank, false), 2);
						}
					}
					else if (pByte >= 0x8000 && pByte < 0xA000)
					{
						if (DisAsmFlags & MEMORY_VBK)
						{
							OutputLabels(pGameBoy, DisAsmVBK, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(DisAsmVBK, false), 2);
						}
						else
						{
							OutputLabels(pGameBoy, pGameBoy->MEM_CPU[0x8F4F] & 1, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(pGameBoy->MEM_CPU[0x8F4F] & 1, false), 2);
						}
					}
					else if (pByte >= 0xA000 && pByte < 0xC000)
					{
						if (DisAsmFlags & MEMORY_RAM)
						{
							OutputLabels(pGameBoy, DisAsmRAM, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(DisAsmRAM, false), 2);
						}
						else
						{
							OutputLabels(pGameBoy, pGameBoy->ActiveRamBank, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(pGameBoy->ActiveRamBank, false), 2);
						}
					}
					else if (pByte >= 0xD000 && pByte < 0xE000)
					{
						if (DisAsmFlags & MEMORY_SVBK)
						{
							OutputLabels(pGameBoy, DisAsmSVBK, pByte, Paint.hdc, &y);
							TextOut(Paint.hdc, 15, y, ToHex(DisAsmSVBK, false), 2);
						}
						else
						{
							if (!(pGameBoy->MEM_CPU[0x8F70] & 7))
							{
								OutputLabels(pGameBoy, 1, pByte, Paint.hdc, &y);
								TextOut(Paint.hdc, 15, y, "01", 2);
							}
							else
							{
								OutputLabels(pGameBoy, pGameBoy->MEM_CPU[0x8F70] & 7, pByte, Paint.hdc, &y);
								TextOut(Paint.hdc, 15, y, ToHex(pGameBoy->MEM_CPU[0x8F70] & 7, false), 2);
							}
						}
					}
					else
					{
						OutputLabels(pGameBoy, 0, pByte, Paint.hdc, &y);
						TextOut(Paint.hdc, 15, y, "00", 2);
					}
					TextOut(Paint.hdc, 15 + 2 * FixedFontWidth, y, ":", 1);

					TextOut(Paint.hdc, 15 + 3 * FixedFontWidth, y, ToHex(pByte, true), 4);

					if (DisAsmCaretByte == pByte)
					{
						DisAsmCaretY = y;
						SetY = true;
						if (DisAsmCaret)
						{
							SetCaretPos(DisAsmCaretX, DisAsmCaretY);
						}
					}

					DisAsmReadMem(pGameBoy, pByte, &p, &Access, &Bank);
					if (*Access & MEM_BREAKPOINT)
					{
						hBitmap = LoadImage(hInstance, MAKEINTRESOURCE(IDB_BREAKPOINT), IMAGE_BITMAP, 0, 0, 0);
						hdc = CreateCompatibleDC(Paint.hdc);
						hBitmap = SelectObject(hdc, hBitmap);
						BitBlt(Paint.hdc, 0, y + 1, 11, y + 12, hdc, 0, 0, SRCCOPY);
						hBitmap = SelectObject(hdc, hBitmap);
						DeleteObject(hBitmap);
						DeleteDC(hdc);
					}

					if (pGameBoy->Reg_PC == pByte && ((pByte < 0x4000 || (pByte >= 0xC000 && pByte < 0xD000) ||
						(pByte >= 0xD000 && !(pGameBoy->Flags & GB_ROM_COLOR)) || pByte >= 0xE000) ||
						(pByte >= 0x4000 && pByte < 0x8000 && Bank == pGameBoy->ActiveRomBank) ||
						(pByte >= 0x8000 && pByte < 0xA000 && Bank == (pGameBoy->MEM_CPU[0x8F4F] & 1)) ||
						(pByte >= 0xA000 && pByte < 0xC000 && Bank == pGameBoy->ActiveRamBank) ||
						(pByte >= 0xD000 && pByte < 0xE000 && ((Bank == 1 && (pGameBoy->MEM_CPU[0x8F70] & 7) == 0)
						|| Bank == (pGameBoy->MEM_CPU[0x8F70] & 7)))))
					{
						hBitmap = LoadImage(hInstance, MAKEINTRESOURCE(IDB_CURRENTSTATEMENT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
						hdc = CreateCompatibleDC(Paint.hdc);
						hBitmap = SelectObject(hdc, hBitmap);
						TransparentBlitter(Paint.hdc, 3, y + 1, 11, 11, hdc, hBitmap);
						hBitmap = SelectObject(hdc, hBitmap);
						DeleteObject(hBitmap);
						DeleteDC(hdc);
					}

					if (*Access & MEM_READ)
					{
						TextOut(Paint.hdc, 15 + 8 * FixedFontWidth, y, ToHex(*p, false), 2);
						if (*p != 0xCB)
						{
							TextOut(Paint.hdc, 15 + 17 * FixedFontWidth, y, OpCodeNames[*p].Text1, strlen(OpCodeNames[*p].Text1));
							x = 15 + (17 + strlen(OpCodeNames[*p].Text1)) * FixedFontWidth;
							if (OpCodeNames[*p].Flags & OCF_BYTES)
							{
								DisAsmReadMem(pGameBoy, ++pByte, &p2, &Access2, &Bank);

								if (pByte == 0)
								{
									pByte--;
									break;
								}

								if (DisAsmCaretByte == pByte)
								{
									DisAsmCaretByte--;
									DisAsmCaretY = y;
									SetY = true;
									if (DisAsmCaret)
									{
										SetCaretPos(DisAsmCaretX, DisAsmCaretY);
									}
								}

								Label = false;

								if (OpCodeNames[*p].Flags & OCF_DATA16)
								{
									DisAsmReadMem(pGameBoy, ++pByte, &p3, &Access3, &Bank);

									if (pByte == 0)
									{
										pByte--;
										break;
									}

									if (DisAsmCaretByte == pByte)
									{
										DisAsmCaretByte -= 2;
										DisAsmCaretY = y;
										if (DisAsmCaret)
										{
											SetCaretPos(DisAsmCaretX, DisAsmCaretY);
										}
									}

									if (*Access3 & MEM_READ)
									{
										TextOut(Paint.hdc, 15 + 14 * FixedFontWidth, y, ToHex(*p3, false), 2);
										if (*Access2 & MEM_READ && (OpCodeNames[*p].Flags & OCF_JUMP || OpCodeNames[*p].Flags & OCF_ADDRESS))
										{
											if (OutputLabel(pGameBoy, (*p3 << 8) | *p2, Paint.hdc, y, &x))
											{
												Label = true;
											}
											else
											{
												TextOut(Paint.hdc, x, y, NumBuffer, 2);
												x += 2 * FixedFontWidth;
											}
										}
										else
										{
											TextOut(Paint.hdc, x, y, NumBuffer, 2);
											x += 2 * FixedFontWidth;
										}
									}
									else
									{
										TextOut(Paint.hdc, 15 + 14 * FixedFontWidth, y, "??", 2);
										TextOut(Paint.hdc, x, y, "??", 2);
										x += 2 * FixedFontWidth;
									}
								}
								if (*Access2 & MEM_READ)
								{
									TextOut(Paint.hdc, 15 + 11 * FixedFontWidth, y, ToHex(*p2, false), 2);
									if (!Label)
									{
										if (OpCodeNames[*p].Flags & OCF_DISP)
										{
											if (OutputLabel(pGameBoy, pByte + 1 + (*p2 & 0x80 ? - (short)((BYTE)-*p2) : *p2), Paint.hdc, y, &x))
											{
												Label = true;
											}
											else
											{
												TextOut(Paint.hdc, x, y, ToHex(pByte + 1 + (*p2 & 0x80 ? - (short)((BYTE)-*p2) : *p2), true), 4);
												x += 4 * FixedFontWidth;
											}
										}
										else
										{
											if (OpCodeNames[*p].Flags & OCF_ADDRESS)
											{
												if (OutputLabel(pGameBoy, 0xFF00 | *p2, Paint.hdc, y, &x))
												{
													Label = true;
												}
											}
											if (!Label)
											{
												if (*p != 0x10)
												{
													if (OpCodeNames[*p].Flags & OCF_ADDRESS && OpCodeNames[*p].Flags & OCF_DATA8)
													{
														TextOut(Paint.hdc, x, y, "FF", 2);
														x += 2 * FixedFontWidth;
													}
													TextOut(Paint.hdc, x, y, NumBuffer, 2);
													x += 2 * FixedFontWidth;
												}
											}
										}
									}
								}
								else
								{
									TextOut(Paint.hdc, 15 + 11 * FixedFontWidth, y, "??", 2);
									if (OpCodeNames[*p].Flags & OCF_DISP)
									{
										TextOut(Paint.hdc, x, y, "????", 4);
										x += 4 * FixedFontWidth;
									}
									else
									{
										if (*p != 0x10)
										{
											TextOut(Paint.hdc, x, y, "??", 2);
											x += 2 * FixedFontWidth;
										}
									}
								}
							}
							TextOut(Paint.hdc, x, y, OpCodeNames[*p].Text2, strlen(OpCodeNames[*p].Text2));
						}
						else
						{
							DisAsmReadMem(pGameBoy, ++pByte, &p, &Access, &Bank);

							if (pByte == 0)
							{
								pByte--;
								break;
							}

							if (DisAsmCaretByte == pByte)
							{
								DisAsmCaretByte--;
								DisAsmCaretY = y;
								if (DisAsmCaret)
								{
									SetCaretPos(DisAsmCaretX, DisAsmCaretY);
								}
							}

							if (*Access & MEM_READ)
							{
								TextOut(Paint.hdc, 15 + 11 * FixedFontWidth, y, ToHex(*p, false), 2);
								TextOut(Paint.hdc, 15 + 17 * FixedFontWidth, y, OpCodeCBNames[*p >> 3], strlen(OpCodeCBNames[*p >> 3]));
								x = 15 + (17 + strlen(OpCodeCBNames[*p >> 3])) * FixedFontWidth;
								TextOut(Paint.hdc, x, y, OpCodeCBArgument[*p & 7], strlen(OpCodeCBArgument[*p & 7]));
							}
							else
							{
								TextOut(Paint.hdc, 15 + 11 * FixedFontWidth, y, "??", 2);
								TextOut(Paint.hdc, 15 + 17 * FixedFontWidth, y, "??", 2);
								x = 15 + (17 + strlen(OpCodeCBNames[*p >> 3])) * FixedFontWidth;
							}
						}
					}
					else
					{
						TextOut(Paint.hdc, 15 + 8 * FixedFontWidth, y, "??", 2);
					}
					if (++pByte == 0)
					{
						pByte--;
						break;
					}
					y += FixedFontHeight;
				}

				hBrush = (HBRUSH)SelectObject(Paint.hdc, hBrush);
				DeleteObject(hBrush);
				hPen = (HPEN)SelectObject(Paint.hdc, hPen);
				DeleteObject(hPen);

				si.cbSize = sizeof(si);
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nMin = 0;
				si.nMax = 0xFFFF;
				si.nPage = rct.bottom / FixedFontHeight;
				si.nPos = DisAsmTopByte;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				si.nMax = 10 + 32 * FixedFontWidth;
				si.nPage = rct.right;
				SetScrollInfo(hWin, SB_HORZ, &si, true);

				if (DisAsmCaretByte < DisAsmTopByte)
				{
					DisAsmCaretY = -16;
				}
				else
				{
					if (DisAsmCaretByte > pByte)
					{
						DisAsmCaretY = rct.bottom + 1;
					}
				}
				if (DisAsmCaret)
				{
					SetCaretPos(DisAsmCaretX, DisAsmCaretY);
					ShowCaret(hWin);
				}

				pGameBoy->Release();
			}
			else
			{
				DisAsmGameBoy = NULL;
				if (DisAsmCaret)
				{
					DestroyCaret();
					DisAsmCaret = false;
				}

				si.cbSize = sizeof(si);
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nMin = 0;
				si.nMax = 0;
				si.nPage = 1;
				si.nPos = 0;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				SetScrollInfo(hWin, SB_HORZ, &si, true);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		DisAsmGameBoy = NULL;
		DisAsmCaret = false;
		DisAsmCaretX = 15;
		DisAsmCaretY = 0;
		return 0;

	case WM_DESTROY:
		hDisAsm = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		DisAsm.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		DisAsm.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		DisAsm.Width = rct.right - rct.left;
		DisAsm.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



DWORD	RegisterCaretX, RegisterCaretY;
BOOL	RegisterCaret;



void PrintAddress(CGameBoy *pGameBoy, HDC hdc, WORD Address, BOOL Word)
{
	TextOut(hdc, 0, 6 * FixedFontHeight, ToHex(Address, true), 4);
	TextOut(hdc, 5 * FixedFontWidth, 6 * FixedFontHeight, "=", 1);
	if (RetrieveAccess(pGameBoy, Address) & MEM_READ)
	{
		TextOut(hdc, 7 * FixedFontWidth, 6 * FixedFontHeight, ToHex(ReadMem(pGameBoy, Address + (Word ? 1 : 0)), false), 2);
	}
	else
	{
		TextOut(hdc, 7 * FixedFontWidth, 6 * FixedFontHeight, "??", 2);
	}
	if (Word)
	{
		if (RetrieveAccess(pGameBoy, Address) & MEM_READ)
		{
			TextOut(hdc, 9 * FixedFontWidth, 6 * FixedFontHeight, ToHex(ReadMem(pGameBoy, Address), false), 2);
		}
		else
		{
			TextOut(hdc, 9 * FixedFontWidth, 6 * FixedFontHeight, "??", 2);
		}
	}
}



void PaintRegisters(HDC hdc, RECT *pRect)
{
	BYTE		Byte;
	CGameBoy	*pGameBoy;
	char		szBuffer[0x100];


	pGameBoy = GameBoys.GetActive(true);

	SelectObject(hdc, hFixedFont);
	//SetBkMode(Paint.hdc, TRANSPARENT);
	SetBkColor(hdc, 0xFFFFFF);
	SetTextColor(hdc, 0);
	TextOut(hdc, pRect->left, pRect->top, "AF", 2);
	TextOut(hdc, pRect->left, pRect->top + FixedFontHeight, "BC", 2);
	TextOut(hdc, pRect->left, pRect->top + 2 * FixedFontHeight, "DE", 2);
	TextOut(hdc, pRect->left, pRect->top + 3 * FixedFontHeight, "HL", 2);
	TextOut(hdc, pRect->left, pRect->top + 4 * FixedFontHeight, "SP", 2);
	TextOut(hdc, pRect->left, pRect->top + 5 * FixedFontHeight, "PC", 2);
	TextOut(hdc, pRect->left, pRect->top + 8 * FixedFontHeight, "ROM", 3);
	TextOut(hdc, pRect->left, pRect->top + 9 * FixedFontHeight, "SVBK", 4);
	TextOut(hdc, pRect->left, pRect->top + 10 * FixedFontHeight, "SRAM", 4);
	TextOut(hdc, pRect->left, pRect->top + 12 * FixedFontHeight, "LY", 2);
	if (pGameBoy)
	{
		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top, ToHex(pGameBoy->Reg_AF, true), 4);
		if (pGameBoy->Reg_F & Flag_Z)
		{
			TextOut(hdc, pRect->left + 8 * FixedFontWidth, pRect->top, "Z", 1);
		}
		if (pGameBoy->Reg_F & Flag_N)
		{
			TextOut(hdc, pRect->left + 9 * FixedFontWidth, pRect->top, "N", 1);
		}
		if (pGameBoy->Reg_F & Flag_H)
		{
			TextOut(hdc, pRect->left + 10 * FixedFontWidth, pRect->top, "H", 1);
		}
		if (pGameBoy->Reg_F & Flag_C)
		{
			TextOut(hdc, pRect->left + 11 * FixedFontWidth, pRect->top, "C", 1);
		}

		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top + FixedFontHeight, ToHex(pGameBoy->Reg_BC, true), 4);
		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top + 2 * FixedFontHeight, ToHex(pGameBoy->Reg_DE, true), 4);
		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top + 3 * FixedFontHeight, ToHex(pGameBoy->Reg_HL, true), 4);
		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top + 4 * FixedFontHeight, ToHex(pGameBoy->Reg_SP, true), 4);
		TextOut(hdc, pRect->left + 3 * FixedFontWidth, pRect->top + 5 * FixedFontHeight, ToHex(pGameBoy->Reg_PC, true), 4);

		if (RetrieveAccess(pGameBoy, pGameBoy->Reg_PC) & MEM_READ)
		{
			Byte = (BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC);
			if (OpCodeNames[Byte].Flags & OCF_RST00)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0000", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST08)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0008", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST10)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0010", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST18)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0018", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST20)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0020", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST28)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0028", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST30)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0030", 4);
			}
			if (OpCodeNames[Byte].Flags & OCF_RST38)
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, "0038", 4);
			}
			if ((OpCodeNames[Byte].Flags & OCF_COND_NZ && (pGameBoy->Reg_F & Flag_Z))
				|| (OpCodeNames[Byte].Flags & OCF_COND_Z && !(pGameBoy->Reg_F & Flag_Z))
				|| (OpCodeNames[Byte].Flags & OCF_COND_NC && (pGameBoy->Reg_F & Flag_C))
				|| (OpCodeNames[Byte].Flags & OCF_COND_C && !(pGameBoy->Reg_F & Flag_C)))
			{
				TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, ToHex(pGameBoy->Reg_PC + 1 + (OpCodeNames[Byte].Flags & OCF_BYTES), true), 4);
			}
			else
			{
				if (OpCodeNames[Byte].Flags & OCF_ADDRESS_BC)
				{
					PrintAddress(pGameBoy, hdc, pGameBoy->Reg_BC, false);
				}
				if (OpCodeNames[Byte].Flags & OCF_ADDRESS_DE)
				{
					PrintAddress(pGameBoy, hdc, pGameBoy->Reg_DE, false);
				}
				if ((OpCodeNames[Byte].Flags & OCF_ADDRESS_HL) || (Byte == 0xCB && (ReadMem(pGameBoy, pGameBoy->Reg_PC + 1) & 7) == 6))
				{
					PrintAddress(pGameBoy, hdc, pGameBoy->Reg_HL, false);
				}
				if (OpCodeNames[Byte].Flags & OCF_ADDRESS_SP)
				{
					PrintAddress(pGameBoy, hdc, pGameBoy->Reg_SP, true);
				}
				if (OpCodeNames[Byte].Flags & OCF_ADDRESS_C)
				{
					PrintAddress(pGameBoy, hdc, 0xFF00 + pGameBoy->Reg_C, false);
				}
				if (OpCodeNames[Byte].Flags & OCF_ADDRESS)
				{
					if (OpCodeNames[Byte].Flags & OCF_DATA16)
					{
						PrintAddress(pGameBoy, hdc, (WORD)(ReadMem(pGameBoy, pGameBoy->Reg_PC + 2) << 8) | ((BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC + 1)), (OpCodeNames[Byte].Flags & OCF_ADDRESS_16) ? true : false);
					}
					if (OpCodeNames[Byte].Flags & OCF_DATA8)
					{
						PrintAddress(pGameBoy, hdc, 0xFF00 | (BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC + 1), false);
					}
				}
				if (OpCodeNames[Byte].Flags & OCF_JUMP)
				{
					if (OpCodeNames[Byte].Flags & OCF_DATA16)
					{
						TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, ToHex((WORD)(ReadMem(pGameBoy, pGameBoy->Reg_PC + 2) << 8) | ((BYTE)ReadMem(pGameBoy, pGameBoy->Reg_PC + 1)), true), 4);
					}
					if (OpCodeNames[Byte].Flags & OCF_DISP)
					{
						TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, ToHex(pGameBoy->Reg_PC + 1 + (OpCodeNames[Byte].Flags & OCF_BYTES) - (signed short)(-(signed char)ReadMem(pGameBoy, pGameBoy->Reg_PC + 1)), true), 4);
					}
				}
				if (OpCodeNames[Byte].Flags & OCF_REG_HL)
				{
					TextOut(hdc, pRect->left, pRect->top + 6 * FixedFontHeight, ToHex(pGameBoy->Reg_HL, true), 4);
				}
			}
		}

		TextOut(hdc, pRect->left + 5 * FixedFontWidth, pRect->top + 8 * FixedFontHeight, ToHex(pGameBoy->ActiveRomBank, false), 2);
		NumBuffer[0] = (pGameBoy->FF00_C(0x70) & 7) + 48;
		if (NumBuffer[0] == 48)
		{
			NumBuffer[0]++;
		}
		TextOut(hdc, pRect->left + 5 * FixedFontWidth, pRect->top + 9 * FixedFontHeight, NumBuffer, 1);
		TextOut(hdc, pRect->left + 5 * FixedFontWidth, pRect->top + 10 * FixedFontHeight, ToHex(pGameBoy->ActiveRamBank, false), 2);
		TextOut(hdc, pRect->left + 8 * FixedFontWidth, pRect->top + 12 * FixedFontHeight, ToHex(pGameBoy->FF00_C(0x44), false), 2);
		switch (pGameBoy->FF00_C(0x41) & 3)
		{
		case 0:
			TextOut(hdc, pRect->left, pRect->top + 13 * FixedFontHeight, String(IDS_WND_REGISTERS_HBLANK), 7);
			break;

		case 1:
			TextOut(hdc, pRect->left, pRect->top + 13 * FixedFontHeight, String(IDS_WND_REGISTERS_VBLANK), 7);
			break;

		case 2:
			TextOut(hdc, pRect->left, pRect->top + 13 * FixedFontHeight, String(IDS_WND_REGISTERS_OAMSEARCH), 3);
			break;

		case 3:
			TextOut(hdc, pRect->left, pRect->top + 13 * FixedFontHeight, String(IDS_WND_REGISTERS_LCDTRANSFER), 3);
			break;
		}
		if ((pGameBoy->MEM_CPU[0x8F41] & 3) == 1)
		{
			if (pGameBoy->MEM_CPU[0x8F44] == 0)
			{
				TextOut(hdc, pRect->left + 8 * FixedFontWidth, pRect->top + 13 * FixedFontHeight, ToHex(pGameBoy->LCD_Ticks, true), 4);
			}
			else
			{
				TextOut(hdc, pRect->left + 8 * FixedFontWidth, pRect->top + 13 * FixedFontHeight, ToHex(pGameBoy->LCD_Ticks + 228 * (154 - pGameBoy->MEM_CPU[0x8F44]), true), 4);
			}
		}
		else
		{
			TextOut(hdc, pRect->left + 8 * FixedFontWidth, pRect->top + 13 * FixedFontHeight, ToHex(pGameBoy->LCD_Ticks, false), 2);
		}

		pGameBoy->Release();
	}
}



LPARAM CALLBACK RegisterWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;
	CGameBoy		*pGameBoy;
	BYTE			*pByte;
	BOOL			HiNibble;
	char			szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_REGISTERS));
		InvalidateRect(hWin, NULL, true);
		return 0;

	case WM_SETFOCUS:
		if (GameBoys.GetActive(false))
		{
			CreateCaret(hWin, NULL, 2, 16);
			SetCaretPos(RegisterCaretX * FixedFontWidth, RegisterCaretY * FixedFontHeight);
			ShowCaret(hWin);
			RegisterCaret = true;
		}
		return 0;

	case WM_KILLFOCUS:
		if (RegisterCaret)
		{
			DestroyCaret();
			RegisterCaret = false;
		}
		return 0;

	case WM_KEYDOWN:
		if (!(pGameBoy = GameBoys.GetActive(true)))
		{
			return 0;
		}
		while (LOWORD(lParam))
		{
			if ((wParam >= '0' && wParam <= '9') || (wParam >= 'A' && wParam <= 'F'))
			{
				switch (RegisterCaretY)
				{
				case 0:	pByte = (BYTE *)&pGameBoy->Reg_AF; break;
				case 1:	pByte = (BYTE *)&pGameBoy->Reg_BC; break;
				case 2:	pByte = (BYTE *)&pGameBoy->Reg_DE; break;
				case 3:	pByte = (BYTE *)&pGameBoy->Reg_HL; break;
				case 4:	pByte = (BYTE *)&pGameBoy->Reg_SP; break;
				case 5:	pByte = (BYTE *)&pGameBoy->Reg_PC;
					if (hDisAsm)
					{
						InvalidateRect(hDisAsm, NULL, true);
					}
					break;
				}
				HiNibble = false;
				if (RegisterCaretY < 6)
				{
					if (RegisterCaretX <= 4)
					{
						pByte++;
					}
					if (RegisterCaretX & 1)
					{
						HiNibble = true;
					}
				}
				else
				{
					pGameBoy->Release();
					return 0;
				}
				RegisterCaretX++;
				if (HiNibble)
				{
					*pByte &= 0x0F;
					*pByte |= ((wParam >= 'A') ? wParam - 'A' + 10 : wParam - '0') << 4;
				}
				else
				{
					*pByte &= 0xF0;
					*pByte |= (wParam >= 'A') ? wParam - 'A' + 10 : wParam - '0';
				}
				InvalidateRect(hWin, NULL, true);
			}
			switch (wParam)
			{
			case VK_LEFT:
				if (RegisterCaretX <= 3)
				{
					pGameBoy->Release();
					return 0;
				}
				if (RegisterCaretY >= 6 && RegisterCaretX <= 5)
				{
					pGameBoy->Release();
					return 0;
				}
				RegisterCaretX--;
				break;

			case VK_RIGHT:
				if (RegisterCaretX >= 7)
				{
					pGameBoy->Release();
					return 0;
				}
				if (RegisterCaretY == 9 && RegisterCaretX >= 6)
				{
					pGameBoy->Release();
					return 0;
				}
				RegisterCaretX++;
				break;

			case VK_UP:
				switch (RegisterCaretY)
				{
				case 0:
					pGameBoy->Release();
					return 0;

				case 8:
					RegisterCaretY -= 3;
					break;

				case 10:
					if (RegisterCaretX > 6)
					{
						RegisterCaretX = 6;
					}
				default:
					RegisterCaretY--;
				}
				break;

			case VK_DOWN:
				switch (RegisterCaretY)
				{
				case 5:
					if (RegisterCaretX < 5)
					{
						RegisterCaretX = 5;
					}
					RegisterCaretY += 3;
					break;

				case 10:
					pGameBoy->Release();
					return 0;

				case 8:
					if (RegisterCaretX > 6)
					{
						RegisterCaretX = 6;
					}
				default:
					RegisterCaretY++;
					break;
				}
			}
			if (RegisterCaret)
			{
				HideCaret(hWin);
				SetCaretPos(RegisterCaretX * FixedFontWidth, RegisterCaretY * FixedFontHeight);
				ShowCaret(hWin);
			}

			lParam--;
		}
		pGameBoy->Release();
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			if (GameBoys.GetActive(false))
			{
				InvalidateRect(hWin, NULL, true);
				GetUpdateRect(hWin, NULL, true);
			}
			BeginPaint(hWin, &Paint);

			rct.left = 0;
			rct.top = 0;
			PaintRegisters(Paint.hdc, &rct);

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		RegisterCaret = false;
		RegisterCaretX = 3;
		RegisterCaretY = 0;
		return 0;

	case WM_DESTROY:
		hRegisters = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		Registers.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		Registers.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		Registers.Width = rct.right - rct.left;
		Registers.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



LPARAM CALLBACK HardwareWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct, Rect2;
	DWORD			Size;
	DWORD			x, y;
	SCROLLINFO		si;
	char			szBuffer[0x100];


	switch (uMsg)
	{
	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_WND_HARDWARE));
		InvalidateRect(hWin, NULL, true);
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(hWin, SB_HORZ, &si);
			x = si.nPos * FixedFontWidth;
			GetScrollInfo(hWin, SB_VERT, &si);
			y = si.nPos * FixedFontHeight;
			BeginPaint(hWin, &Paint);
			SelectObject(Paint.hdc, hFixedFont);
			SetBkMode(Paint.hdc, TRANSPARENT);
			TextOut(Paint.hdc, FixedFontWidth - x, FixedFontHeight - y, String(IDS_WND_HARDWARE_ROMSIZE), 9);
			TextOut(Paint.hdc, FixedFontWidth - x, FixedFontHeight + FixedFontHeight - y, String(IDS_WND_HARDWARE_RAMSIZE), 9);
			if (pGameBoy = GameBoys.GetActive(true))
			{
				if (pGameBoy->MEM_ROM)
				{
					itoa((RomSize(pGameBoy->MEM_ROM[0x0148]) + 1) * 16, NumBuffer, 10);
					TextOut(Paint.hdc, FixedFontWidth + 10 * FixedFontWidth - x, FixedFontHeight - y, NumBuffer, strlen(NumBuffer));
					TextOut(Paint.hdc, FixedFontWidth + (11 + strlen(NumBuffer)) * FixedFontWidth - x, FixedFontHeight - y, "kB", 2);

					switch (pGameBoy->MEM_ROM[0x0149])
					{
					case 1:
						Size = 2;
						break;
					case 2:
						Size = 8;
						break;
					case 3:
						Size = 32;
						break;
					case 4:
						Size = 128;
						break;
					default:
						Size = 0;
						break;
					}
					itoa(Size, NumBuffer, 10);
					TextOut(Paint.hdc, FixedFontWidth + 10 * FixedFontWidth - x, FixedFontHeight + FixedFontHeight - y, NumBuffer, strlen(NumBuffer));
					TextOut(Paint.hdc, FixedFontWidth + (11 + strlen(NumBuffer)) * FixedFontWidth - x, FixedFontHeight + FixedFontHeight - y, "kB", 2);
				}

				pGameBoy->Release();
			}
			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_SIZE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		GetScrollInfo(hWin, SB_HORZ, &si);
		si.nPage = LOWORD(lParam) / FixedFontWidth;
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		GetScrollInfo(hWin, SB_VERT, &si);
		si.nPage = HIWORD(lParam) / FixedFontHeight;
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		si.fMask = SIF_PAGE | SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;

	case WM_VSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_VERT, &si);
		y = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN:
			si.nPos++;
			break;
		case SB_LINEUP:
			si.nPos--;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, 0, (y - si.nPos) * FixedFontHeight, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_HSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
		GetScrollInfo(hWin, SB_HORZ, &si);
		x = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT:
			si.nPos++;
			break;
		case SB_LINELEFT:
			si.nPos--;
			break;
		case SB_PAGERIGHT:
			si.nPos += si.nPage;
			break;
		case SB_PAGELEFT:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
		if (si.nPos < 0)
		{
			si.nPos = 0;
		}
		if ((unsigned)si.nPos > si.nMax - si.nPage + 1)
		{
			si.nPos = si.nMax - si.nPage + 1;
		}
		ScrollWindow(hWin, (x - si.nPos) * FixedFontWidth, 0, NULL, NULL);
		si.fMask = SIF_POS;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		UpdateWindow(hWin);
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE;
		si.nPos = 0;
		si.nMin = 0;
		si.nMax = 18;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.nMax = 3;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;

	case WM_DESTROY:
		hHardware = NULL;
		GetWindowRect(hClientWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		Hardware.x = rct.left - Rect2.left - GetSystemMetrics(SM_CXEDGE);
		Hardware.y = rct.top - Rect2.top - GetSystemMetrics(SM_CYEDGE);
		Hardware.Width = rct.right - rct.left;
		Hardware.Height = rct.bottom - rct.top;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



BOOL CreateDebugWindows()
{
	WNDCLASS	wc;


	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RegisterWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Registers";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hRegisters = NULL;
	wc.lpfnWndProc = DisAsmWndProc;
	wc.lpszClassName = "DisAsm";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hDisAsm = NULL;
	wc.lpfnWndProc = MemoryWndProc;
	wc.lpszClassName = "Memory";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hMemory = NULL;

	/*wc.lpfnWndProc = TileAddressWndProc;
	wc.lpszClassName = "TileAddress";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}*/

	wc.style = 0;
	wc.lpfnWndProc = TilesWndProc;
	wc.lpszClassName = "Tiles";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hTiles = NULL;
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = TilesChildWndProc;
	wc.lpszClassName = "TilesChild";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	wc.style = 0;
	wc.lpfnWndProc = PalettesWndProc;
	wc.lpszClassName = "Palettes";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hPalettes = NULL;
	wc.lpfnWndProc = TileMapWndProc;
	wc.lpszClassName = "TileMap";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hTileMap = NULL;
	wc.lpfnWndProc = TileMapChildWndProc;
	wc.lpszClassName = "TileMapChild";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	wc.lpfnWndProc = TileZoomWndProc;
	wc.lpszClassName = "TileZoom";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = HardwareWndProc;
	wc.lpszClassName = "Hardware";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage();
		return true;
	}
	hHardware = NULL;

	return false;
}

