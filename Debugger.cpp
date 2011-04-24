#include	<windows.h>
#include	"resource.h"

#define		DEBUGGER_CPP
#include	"Game Lad.h"
#include	"Debugger.h"
#include	"Emulation.h"
#include	"Z80.h"



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
	BYTE	Bytes;
};



const _OpCodeNames	OpCodeNames[256] = {
							"nop", "", 0,			//0x00
							"ld   bc, ", "", 2,
							"ld   [bc], a", "", 0,
							"inc  bc", "", 0,
							"inc  b", "", 0,
							"dec  b", "", 0,
							"ld   b, ", "", 1,
							"rlca", "", 0,
							"ld   [", "], sp", 2,	//0x08
							"add  hl, bc", "", 0,
							"ld   a, [bc]", "", 0,
							"dec  bc", "", 0,
							"inc  c", "", 0,
							"dec  c", "", 0,
							"ld   c, ", "", 1,
							"rrca", "", 0,
							"stop", "", 1,			//0x10
							"ld   de, ", "", 2,
							"ld   [de], a", "", 0,
							"inc  de", "", 0,
							"inc  d", "", 0,
							"dec  d", "", 0,
							"ld   d, ", "", 1,
							"rla", "", 0,
							"jr   ", "", 4,			//0x18
							"add  hl, de", "", 0,
							"ld   a, [de]", "", 0,
							"dec  de", "", 0,
							"inc  e", "", 0,
							"dec  e", "", 0,
							"ld   e, ", "", 1,
							"rra", "", 0,
							"jr   nz, ", "", 4,		//0x20
							"ld   hl, ", "", 2,
							"ldi  [hl], a", "", 0,
							"inc  hl", "", 0,
							"inc  h", "", 0,
							"dec  h", "", 0,
							"ld   h, ", "", 1,
							"daa", "", 0,
							"jr   z, ", "", 4,		//0x28
							"add  hl, hl", "", 0,
							"ldi  a, [hl]", "", 0,
							"dec  hl", "", 0,
							"inc  l", "", 0,
							"dec  l", "", 0,
							"ld   l, ", "", 1,
							"cpl", "", 0,
							"jr   nc, ", "", 4,		//0x30
							"ld   sp, ", "", 2,
							"ldd  [hl], a", "", 0,
							"inc  sp", "", 0,
							"inc  [hl]", "", 0,
							"dec  [hl]", "", 0,
							"ld   [hl], ", "", 1,
							"scf", "", 0,
							"jr   c, ", "", 4,		//0x38
							"add  hl, sp", "", 0,
							"ldd  a, [hl]", "", 0,
							"dec  sp", "", 0,
							"inc  a", "", 0,
							"dec  a", "", 0,
							"ld   a, ", "", 1,
							"ccf", "", 0,
							"ld   b, b", "", 0,		//0x40
							"ld   b, c", "", 0,
							"ld   b, d", "", 0,
							"ld   b, e", "", 0,
							"ld   b, h", "", 0,
							"ld   b, l", "", 0,
							"ld   b, [hl]", "", 0,
							"ld   b, a", "", 0,
							"ld   c, b", "", 0,		//0x48
							"ld   c, c", "", 0,
							"ld   c, d", "", 0,
							"ld   c, e", "", 0,
							"ld   c, h", "", 0,
							"ld   c, l", "", 0,
							"ld   c, [hl]", "", 0,
							"ld   c, a", "", 0,
							"ld   d, b", "", 0,		//0x50
							"ld   d, c", "", 0,
							"ld   d, d", "", 0,
							"ld   d, e", "", 0,
							"ld   d, h", "", 0,
							"ld   d, l", "", 0,
							"ld   d, [hl]", "", 0,
							"ld   d, a", "", 0,
							"ld   e, b", "", 0,		//0x58
							"ld   e, c", "", 0,
							"ld   e, d", "", 0,
							"ld   e, e", "", 0,
							"ld   e, h", "", 0,
							"ld   e, l", "", 0,
							"ld   e, [hl]", "", 0,
							"ld   e, a", "", 0,
							"ld   h, b", "", 0,		//0x60
							"ld   h, c", "", 0,
							"ld   h, d", "", 0,
							"ld   h, e", "", 0,
							"ld   h, h", "", 0,
							"ld   h, l", "", 0,
							"ld   h, [hl]", "", 0,
							"ld   h, a", "", 0,
							"ld   l, b", "", 0,		//0x68
							"ld   l, c", "", 0,
							"ld   l, d", "", 0,
							"ld   l, e", "", 0,
							"ld   l, h", "", 0,
							"ld   l, l", "", 0,
							"ld   l, [hl]", "", 0,
							"ld   l, a", "", 0,
							"ld   [hl], b", "", 0,	//0x70
							"ld   [hl], c", "", 0,
							"ld   [hl], d", "", 0,
							"ld   [hl], e", "", 0,
							"ld   [hl], h", "", 0,
							"ld   [hl], l", "", 0,
							"halt", "", 0,
							"ld   [hl], a", "", 0,
							"ld   a, b", "", 0,		//0x78
							"ld   a, c", "", 0,
							"ld   a, d", "", 0,
							"ld   a, e", "", 0,
							"ld   a, h", "", 0,
							"ld   a, l", "", 0,
							"ld   a, [hl]", "", 0,
							"ld   a, a", "", 0,
							"add  a, b", "", 0,		//0x80
							"add  a, c", "", 0,
							"add  a, d", "", 0,
							"add  a, e", "", 0,
							"add  a, h", "", 0,
							"add  a, l", "", 0,
							"add  a, [hl]", "", 0,
							"add  a, a", "", 0,
							"adc  a, b", "", 0,		//0x88
							"adc  a, c", "", 0,
							"adc  a, d", "", 0,
							"adc  a, e", "", 0,
							"adc  a, h", "", 0,
							"adc  a, l", "", 0,
							"adc  a, [hl]", "", 0,
							"adc  a, a", "", 0,
							"sub  a, b", "", 0,		//0x90
							"sub  a, c", "", 0,
							"sub  a, d", "", 0,
							"sub  a, e", "", 0,
							"sub  a, h", "", 0,
							"sub  a, l", "", 0,
							"sub  a, [hl]", "", 0,
							"sub  a, a", "", 0,
							"sbc  a, b", "", 0,		//0x98
							"sbc  a, c", "", 0,
							"sbc  a, d", "", 0,
							"sbc  a, e", "", 0,
							"sbc  a, h", "", 0,
							"sbc  a, l", "", 0,
							"sbc  a, [hl]", "", 0,
							"sbc  a, a", "", 0,
							"and  b", "", 0,		//0xA0
							"and  c", "", 0,
							"and  d", "", 0,
							"and  e", "", 0,
							"and  h", "", 0,
							"and  l", "", 0,
							"and  [hl]", "", 0,
							"and  a", "", 0,
							"xor  b", "", 0,		//0xA8
							"xor  c", "", 0,
							"xor  d", "", 0,
							"xor  e", "", 0,
							"xor  h", "", 0,
							"xor  l", "", 0,
							"xor  [hl]", "", 0,
							"xor  a", "", 0,
							"or   b", "", 0,		//0xB0
							"or   c", "", 0,
							"or   d", "", 0,
							"or   e", "", 0,
							"or   h", "", 0,
							"or   l", "", 0,
							"or   [hl]", "", 0,
							"or   a", "", 0,
							"cp   b", "", 0,		//0xB8
							"cp   c", "", 0,
							"cp   d", "", 0,
							"cp   e", "", 0,
							"cp   h", "", 0,
							"cp   l", "", 0,
							"cp   [hl]", "", 0,
							"cp   a", "", 0,
							"ret  nz", "", 0,		//0xC0
							"pop  bc", "", 0,
							"jp   nz, ", "", 2,
							"jp   ", "", 2,
							"call nz, ", "", 2 | 8,
							"push bc", "", 0,
							"add  a, ", "", 1,
							"rst  00", "", 8,
							"ret  z", "", 0,		//0xC8
							"ret", "", 0,
							"jp   z, ", "", 2,
							"", "", 1,
							"call z, ", "", 2 | 8,
							"call ", "", 2 | 8,
							"adc  a, ", "", 1,
							"rst  08", "", 8,
							"ret  nc", "", 0,		//0xD0
							"pop  de", "", 0,
							"jp   nc, ", "", 2,
							"Undefined", "", 0,
							"call nc, ", "", 2 | 8,
							"push de", "", 0,
							"sub  a, ", "", 1,
							"rst  10", "", 8,
							"ret  c", "", 0,		//0xD8
							"reti", "", 0,
							"jp   c, ", "", 2,
							"Undefined", "", 0,
							"call c, ", "", 2 | 8,
							"Undefined", "", 0,
							"sbc  a, ", "", 1,
							"rst  18", "", 8,
							"ld   [FF", "], a", 1,	//0xE0
							"pop  hl", "", 0,
							"ld   [FF00 + c], a", "", 0,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"push hl", "", 0,
							"and  ", "", 1,
							"rst  20", "", 8,
							"add  sp, ", "", 1,		//0xE8
							"jp   hl", "", 0,
							"ld   [", "], a", 2,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"xor  ", "", 1,
							"rst  28", "", 8,
							"ld   a, [FF", "]", 1,	//0xF0
							"pop  af", "", 0,
							"ld   a, [FF00 + c]", "", 0,
							"di", "", 0,
							"Undefined", "", 0,
							"push af", "", 0,
							"or   ", "", 1,
							"rst  30", "", 8,
							"ld   hl, sp + ", "", 1,//0xF8
							"ld   sp, hl", "", 0,
							"ld   a, [", "]", 2,
							"ei", "", 0,
							"Undefined", "", 0,
							"Undefined", "", 0,
							"cp   ", "", 1,
							"rst  38", "", 8
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
		NumBuffer[0] = (n >> 12) + 48;
		if (NumBuffer[0] > 57)
		{
			NumBuffer[0] += 7;
		}
		NumBuffer[1] = ((n >> 8) & 0xF) + 48;
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



DWORD		TileMapZoom = 1;

LRESULT CALLBACK TileMapWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	BYTE			TileY;
	WORD			pMapTile, pTile;
	DWORD			Color1, Color2;
	WORD			Color;
	BITMAPINFO		bmi;
	HBITMAP			hBitmap, hOldBitmap;
	DWORD			*pBitmap[8];
	HDC				hdc;
	RGBQUAD			Palette[4];
	RECT			rct, Rect2;
	SCROLLINFO		si;
	DWORD			x, y;


	switch (uMsg)
	{
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

			pGameBoy = GameBoyList.GetActive();

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
			if (pGameBoy)
			{
				if (!(pGameBoy->Flags & GB_ROM_COLOR))
				{
					*(DWORD *)&Palette[0] = GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
					*(DWORD *)&Palette[2] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
					*(DWORD *)&Palette[1] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
					*(DWORD *)&Palette[3] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
					SetDIBColorTable(hdc, 0, 4, Palette);
				}
			}
			else
			{
				SetDIBColorTable(hdc, 0, 4, (RGBQUAD *)&GreyScales);
			}

			if (!pGameBoy)
			{
				ZeroMemory(*pBitmap, sizeof(pBitmap));
			}

			pMapTile = 0x1800;
			while (pMapTile < 0x2000)
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
							Color = *(WORD *)&pGameBoy->BGP[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 7) << 3)];
							*(DWORD *)&Palette[0] = (((Color >> 10) & 0x1F) << 3) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 19);
							Color = *(WORD *)&pGameBoy->BGP[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 7) << 3) + 2];
							*(DWORD *)&Palette[2] = (((Color >> 10) & 0x1F) << 3) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 19);
							Color = *(WORD *)&pGameBoy->BGP[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 7) << 3) + 4];
							*(DWORD *)&Palette[1] = (((Color >> 10) & 0x1F) << 3) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 19);
							Color = *(WORD *)&pGameBoy->BGP[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 7) << 3) + 6];
							*(DWORD *)&Palette[3] = (((Color >> 10) & 0x1F) << 3) | (((Color >> 5) & 0x1F) << 11) | ((Color & 0x1F) << 19);
							SetDIBColorTable(hdc, 0, 4, Palette);

							Color1 = pGameBoy->MEM_VRAM[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 8) ? 0x2000 : 0) + pTile + (TileY << 1)];
							Color2 = pGameBoy->MEM_VRAM[((pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 8) ? 0x2000 : 0) + pTile + (TileY << 1) + 1];
							(*pBitmap)[(pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 0x40) ? 7 - TileY : TileY] = (pGameBoy->MEM_VRAM[pMapTile + 0x2000] & 0x20) ? TileToBmpFlipX(Color1, Color2) : TileToBmp(Color1, Color2);
						}
						else
						{
							Color1 = pGameBoy->MEM_VRAM[pTile + (TileY << 1)];
							Color2 = pGameBoy->MEM_VRAM[pTile + (TileY << 1) + 1];
							(*pBitmap)[TileY] = TileToBmp(Color1, Color2);
						}

						TileY++;
					}
				}

				if (TileMapZoom == 1)
				{
					BitBlt(Paint.hdc, 5 + 9 * ((pMapTile - 0x1800) & 0x1F) + (pMapTile >= 0x1C00 ? 300 : 0) - x, 5 + 9 * (((pMapTile - 0x1800) & ~0x1F) >> 5) - (pMapTile >= 0x1C00 ? 288 : 0) - y, 8, 8, hdc, 0, 0, SRCCOPY);
				}
				else
				{
					StretchBlt(Paint.hdc, 5 + TileMapZoom * 8 * ((pMapTile - 0x1800) & 0x1F) + ((pMapTile - 0x1800) & 0x1F) + (pMapTile >= 0x1C00 ? 42 + TileMapZoom * 256 : 0) - x, 5 + TileMapZoom * 8 * (((pMapTile - 0x1800) & ~0x1F) >> 5) + (((pMapTile - 0x1800) & ~0x1F) >> 5) - (pMapTile >= 0x1C00 ? 32 + TileMapZoom * 256 : 0) - y, TileMapZoom * 8, TileMapZoom * 8, hdc, 0, 0, 8, 8, SRCCOPY);
				}

				pMapTile++;
			}

			SelectObject(hdc, hOldBitmap);
			DeleteDC(hdc);
			DeleteObject(hBitmap);

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
		si.nPage = rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) /*- GetSystemMetrics(SM_CYHSCROLL)*/ - GetSystemMetrics(SM_CYSMCAPTION);
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

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_VIEW_ZOOM_100:
			if (TileMapZoom != 1)
			{
				TileMapZoom = 1;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 83 + TileMapZoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 40 + 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 82 + TileMapZoom * 512;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 39 + TileMapZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_200:
			if (TileMapZoom != 2)
			{
				TileMapZoom = 2;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 83 + TileMapZoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 40 + TileMapZoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 82 + TileMapZoom * 512;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 39 + TileMapZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_300:
			if (TileMapZoom != 3)
			{
				TileMapZoom = 3;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 83 + TileMapZoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 40 + TileMapZoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 82 + TileMapZoom * 512;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 39 + TileMapZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_400:
			if (TileMapZoom != 4)
			{
				TileMapZoom = 4;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 83 + TileMapZoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 40 + TileMapZoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 82 + TileMapZoom * 512;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 39 + TileMapZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;
		}
		break;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		GetWindowRect(hWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 83 + TileMapZoom * 512 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 40 + TileMapZoom * 256 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
		si.nPos = 0;
		si.nMin = 0;
		si.nMax = 82 + TileMapZoom * 512;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.nMax = 39 + TileMapZoom * 256;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;

	case WM_DESTROY:
		hTileMap = NULL;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



LRESULT CALLBACK PalettesWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	BYTE			x, y;
	HBRUSH			hBrush, hOldBrush;
	DWORD			Color;
	char			c[1];


	switch (uMsg)
	{
	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			pGameBoy = GameBoyList.GetActive();

			SelectObject(Paint.hdc, GetStockObject(ANSI_FIXED_FONT));
			SetBkMode(Paint.hdc, TRANSPARENT);
			SetTextColor(Paint.hdc, 0);

			TextOut(Paint.hdc, 20, 10, "Background", 10);
			TextOut(Paint.hdc, 200, 10, "Sprite", 6);

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

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		hPalettes = NULL;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



DWORD		TileZoom = 1;

LRESULT CALLBACK TilesWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy			*pGameBoy;
	PAINTSTRUCT			Paint;
	WORD				pTile;
	BYTE				TileY;
	BYTE				Color1, Color2;
	BITMAPINFO			bmi;
	HBITMAP				hBitmap, hOldBitmap;
	DWORD				*pBitmap[8];
	HDC					hdc;
	RGBQUAD				Palette[4];
	WORD				Bank;
	RECT				rct, Rect2;
	SCROLLINFO			si;
	DWORD				x, y;


	switch (uMsg)
	{
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

			pGameBoy = GameBoyList.GetActive();

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
			if (pGameBoy)
			{
				if (pGameBoy->Flags & GB_ROM_COLOR)
				{
					SetDIBColorTable(hdc, 0, 4, (RGBQUAD *)&GreyScales);
				}
				else
				{
					*(DWORD *)&Palette[0] = GreyScales[pGameBoy->MEM_CPU[0x8F47] & 3];
					*(DWORD *)&Palette[2] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 2) & 3];
					*(DWORD *)&Palette[1] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 4) & 3];
					*(DWORD *)&Palette[3] = GreyScales[(pGameBoy->MEM_CPU[0x8F47] >> 6) & 3];
					SetDIBColorTable(hdc, 0, 4, Palette);
				}
			}
			else
			{
				SetDIBColorTable(hdc, 0, 4, (RGBQUAD *)&GreyScales);
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

					if (TileZoom == 1)
					{
						BitBlt(Paint.hdc, 5 + 9 * (pTile & 0x0F) + (Bank ? 149 : 0) - x, 5 + 9 * ((pTile & ~0xF) >> 4) - y, 8, 8, hdc, 0, 0, SRCCOPY);
					}
					else
					{
						StretchBlt(Paint.hdc, 5 + TileZoom * 8 * (pTile & 0x0F) + (pTile & 0x0F) + (Bank ? 21 + TileZoom * 128 : 0) - x, 5 + TileZoom * 8 * ((pTile & ~0xF) >> 4) + ((pTile & ~0xF) >> 4) - y, TileZoom * 8, TileZoom * 8, hdc, 0, 0, 8, 8, SRCCOPY);
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
		si.nPage = rct.bottom - rct.top - 2 * GetSystemMetrics(SM_CYSIZEFRAME) - 2 * GetSystemMetrics(SM_CYEDGE) /*- GetSystemMetrics(SM_CYHSCROLL)*/ - GetSystemMetrics(SM_CYSMCAPTION);
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

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_VIEW_ZOOM_100:
			if (TileZoom != 1)
			{
				TileZoom = 1;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 46 + TileZoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 34 + TileZoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 43 + TileZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 31 + TileZoom * 194;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_200:
			if (TileZoom != 2)
			{
				TileZoom = 2;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 46 + TileZoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 34 + TileZoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 43 + TileZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 31 + TileZoom * 194;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_300:
			if (TileZoom != 3)
			{
				TileZoom = 3;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 46 + TileZoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 34 + TileZoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 43 + TileZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 31 + TileZoom * 194;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;

		case ID_VIEW_ZOOM_400:
			if (TileZoom != 4)
			{
				TileZoom = 4;
				GetWindowRect(hWnd, &Rect2);
				GetWindowRect(hWin, &rct);
				MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 46 + TileZoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 34 + TileZoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
				si.nPos = 0;
				si.nMin = 0;
				si.nMax = 43 + TileZoom * 256;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_HORZ, &si, true);
				si.nMax = 31 + TileZoom * 194;
				si.nPage = si.nMax + 1;
				SetScrollInfo(hWin, SB_VERT, &si, true);
				InvalidateRect(hWin, NULL, true);
			}
			return 0;
		}
		break;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		GetWindowRect(hWnd, &Rect2);
		GetWindowRect(hWin, &rct);
		MoveWindow(hWin, rct.left - Rect2.left - GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXEDGE), rct.top - Rect2.top - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE) - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYMENU), 46 + TileZoom * 256 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 2 * GetSystemMetrics(SM_CXEDGE), 34 + TileZoom * 194 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + 2 * GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYSMCAPTION), true);
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
		si.nPos = 0;
		si.nMin = 0;
		si.nMax = 43 + TileZoom * 256;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_HORZ, &si, true);
		si.nMax = 31 + TileZoom * 194;
		si.nPage = si.nMax + 1;
		SetScrollInfo(hWin, SB_VERT, &si, true);
		return 0;

	case WM_DESTROY:
		hTiles = NULL;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



BOOL		MemoryCaret;
int			MemoryCaretX, MemoryCaretY;
WORD		MemoryTopByte;

LPARAM CALLBACK MemoryWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct;
	LOGBRUSH		lb;
	HBRUSH			hBrush;
	HPEN			hPen;
	SCROLLINFO		si;
	WORD			pByte;
	BYTE			Byte;
	int				x, y, BytesPerLine, nBytes;


	switch (uMsg)
	{
	case WM_SETFOCUS:
		if (GameBoyList.GetActive())
		{
			CreateCaret(hWin, NULL, 2, 16);
			SetCaretPos(MemoryCaretX, MemoryCaretY);
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
		if (!GameBoyList.GetActive())
		{
			break;
		}
		GetClientRect(hWin, &rct);
		BytesPerLine = (rct.right / FixedFontWidth - 7) / 4;
		if (BytesPerLine == 0)
		{
			BytesPerLine = 1;
		}
		switch (wParam)
		{
		case VK_UP:
			if (MemoryTopByte == 0 && MemoryCaretY == 0)
			{
				return 0;
			}
			if (MemoryTopByte < BytesPerLine && MemoryCaretY == 0)
			{
				MemoryTopByte = 0;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}

			if (MemoryCaretY < 0)
			{
				MemoryTopByte += BytesPerLine * (MemoryCaretY / FixedFontHeight - 1);
				MemoryCaretY = 0;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}
			if (MemoryCaretY > rct.bottom)
			{
				MemoryTopByte += BytesPerLine * ((MemoryCaretY - rct.bottom) / FixedFontHeight + 1);
				MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}

			if (MemoryCaret)
			{
				HideCaret(hWin);
			}
			if (MemoryCaretY == 0)
			{
				MemoryTopByte -= BytesPerLine;
				ScrollWindowEx(hWin, 0, FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			}
			else
			{
				MemoryCaretY -= FixedFontHeight;
			}
			UpdateWindow(hWin);
			if (MemoryCaret)
			{
				SetCaretPos(MemoryCaretX, MemoryCaretY);
				ShowCaret(hWin);
			}
			return 0;

		case VK_DOWN:
			if (MemoryTopByte + BytesPerLine * (MemoryCaretY / FixedFontHeight + 1) > 0xFFFF)
			{
				if (MemoryCaretY > rct.bottom - FixedFontHeight)
				{
					MemoryTopByte += BytesPerLine * ((MemoryCaretY - rct.bottom) / FixedFontHeight + 1);
					if (MemoryCaretY > rct.bottom)
					{
						MemoryTopByte += BytesPerLine;
					}
					MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
					InvalidateRect(hWin, NULL, true);
				}
				return 0;
			}

			if (MemoryCaretY < 0)
			{
				MemoryTopByte += BytesPerLine * (MemoryCaretY / FixedFontHeight + 1);
				MemoryCaretY = 0;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}
			if (MemoryCaretY > rct.bottom - FixedFontHeight)
			{
				MemoryTopByte += BytesPerLine * ((MemoryCaretY - rct.bottom) / FixedFontHeight + 3);
				MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}

			if (MemoryCaret)
			{
				HideCaret(hWin);
			}
			if (MemoryCaretY < rct.bottom - 2 * FixedFontHeight)
			{
				MemoryCaretY += FixedFontHeight;
			}
			else
			{
				MemoryTopByte += BytesPerLine;
				ScrollWindowEx(hWin, 0, -FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			}
			UpdateWindow(hWin);
			if (MemoryCaret)
			{
				SetCaretPos(MemoryCaretX, MemoryCaretY);
				ShowCaret(hWin);
			}
			return 0;

		case VK_PRIOR:
			if (MemoryTopByte + BytesPerLine * (MemoryCaretY / FixedFontHeight) <= BytesPerLine * (rct.bottom / FixedFontHeight - 1))
			{
				MemoryTopByte = 0;
				MemoryCaretY = 0;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}

			if (MemoryCaretY < 0)
			{
				MemoryTopByte += BytesPerLine * (MemoryCaretY / FixedFontHeight);
				MemoryCaretY = 0;
			}
			if (MemoryCaretY > rct.bottom - FixedFontHeight)
			{
				MemoryTopByte += BytesPerLine * ((MemoryCaretY - rct.bottom) / FixedFontHeight + 2);
				MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
			}
			if (MemoryTopByte >= BytesPerLine * (rct.bottom / FixedFontHeight - 1))
			{
				MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			else
			{
				MemoryCaretY -= FixedFontHeight * ((BytesPerLine * (rct.bottom / FixedFontHeight - 1) - MemoryTopByte) / BytesPerLine);
				MemoryTopByte = 0;
			}
			InvalidateRect(hWin, NULL, true);
			return 0;

		case VK_NEXT:
			if (MemoryTopByte + BytesPerLine * ((MemoryCaretY + rct.bottom) / FixedFontHeight) > 0xFFFF)
			{
				while (MemoryTopByte + BytesPerLine < 0x10000)
				{
					MemoryTopByte += BytesPerLine;
				}
				MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
				MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
				InvalidateRect(hWin, NULL, true);
				return 0;
			}

			if (MemoryCaretY < 0)
			{
				MemoryTopByte += BytesPerLine * (MemoryCaretY / FixedFontHeight);
				MemoryCaretY = 0;
			}
			if (MemoryCaretY > rct.bottom - FixedFontHeight)
			{
				MemoryTopByte += BytesPerLine * ((MemoryCaretY) / FixedFontHeight);
				MemoryCaretY = rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
			}
			else
			{
				MemoryTopByte += BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			InvalidateRect(hWin, NULL, true);
			return 0;

		case VK_HOME:
			if (GetKeyState(VK_CONTROL) & 0x8000)
			{
				//Ctrl + Home
				if (MemoryTopByte != 0)
				{
					MemoryTopByte = 0;
					InvalidateRect(hWin, NULL, true);
				}
				if (MemoryCaretX != 0 || MemoryCaretY != 0)
				{
					MemoryCaretX = 0;
					MemoryCaretY = 0;
					if (MemoryCaret)
					{
						HideCaret(hWin);
						SetCaretPos(MemoryCaretX, MemoryCaretY);
						ShowCaret(hWin);
					}
				}
			}
			else
			{
				if (MemoryCaretX != 0)
				{
					MemoryCaretX = 0;
					si.cbSize = sizeof(si);
					si.fMask = SIF_POS;
					GetScrollInfo(hWin, SB_HORZ, &si);
					if (si.nPos != 0)
					{
						si.nPos = 0;
						SetScrollInfo(hWin, SB_HORZ, &si, true);
						InvalidateRect(hWin, NULL, true);
					}
					else
					{
						if (MemoryCaret)
						{
							HideCaret(hWin);
							SetCaretPos(MemoryCaretX, MemoryCaretY);
							ShowCaret(hWin);
						}
					}
				}
			}
			return 0;

		case VK_END:
			if (GetKeyState(VK_CONTROL) & 0x8000)
			{
				//Ctrl + End
			}
			else
			{
			}
			return 0;
		}
		break;

	case WM_VSCROLL:
		if (!GameBoyList.GetActive())
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		BytesPerLine = (rct.right / FixedFontWidth - 7) / 4;
		if (BytesPerLine == 0)
		{
			BytesPerLine = 1;
		}
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
			MemoryCaretY += FixedFontHeight;
			if (MemoryCaret)
			{
				HideCaret(hWin);
				SetCaretPos(MemoryCaretX, MemoryCaretY);
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
			MemoryCaretY -= FixedFontHeight;
			if (MemoryCaret)
			{
				HideCaret(hWin);
				SetCaretPos(MemoryCaretX, MemoryCaretY);
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
				MemoryCaretY += FixedFontHeight * (MemoryTopByte / BytesPerLine);
				MemoryTopByte = 0;
			}
			else
			{
				MemoryCaretY += rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
				MemoryTopByte -= BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_PAGEDOWN:
			GetClientRect(hWin, &rct);
			if (MemoryTopByte + BytesPerLine * (rct.bottom / FixedFontHeight - 1) > 0xFFFF)
			{
				MemoryCaretY -= FixedFontHeight * (((0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight - 1)) - MemoryTopByte) / BytesPerLine);
				MemoryTopByte = 0x10000 - BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			else
			{
				MemoryCaretY -= rct.bottom - rct.bottom % FixedFontHeight - FixedFontHeight;
				MemoryTopByte += BytesPerLine * (rct.bottom / FixedFontHeight - 1);
			}
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			MemoryCaretY -= FixedFontHeight * (((MemoryTopByte % BytesPerLine + BytesPerLine * HIWORD(wParam)) - MemoryTopByte) / BytesPerLine);
			MemoryTopByte = MemoryTopByte % BytesPerLine + BytesPerLine * HIWORD(wParam);
			InvalidateRect(hWin, NULL, true);
			UpdateWindow(hWin);
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (!GameBoyList.GetActive())
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		BytesPerLine = (rct.right / FixedFontWidth - 7) / 4;
		if (BytesPerLine == 0)
		{
			BytesPerLine = 1;
		}
		MemoryCaretY = HIWORD(lParam) - HIWORD(lParam) % FixedFontHeight;
		if (MemoryTopByte + BytesPerLine * (MemoryCaretY / FixedFontHeight) > 0xFFFF)
		{
			MemoryCaretY -= FixedFontHeight;
		}
		if (MemoryCaret)
		{
			HideCaret(hWin);
			SetCaretPos(MemoryCaretX, MemoryCaretY);
			ShowCaret(hWin);
		}
		if (MemoryCaretY > rct.bottom - FixedFontHeight)
		{
			SendMessage(hWin, WM_VSCROLL, SB_LINEDOWN, 0);
		}
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoyList.GetActive())
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

				BytesPerLine = (rct.right / FixedFontWidth - 7) / 4;
				if (BytesPerLine == 0)
				{
					BytesPerLine = 1;
				}

				while (MemoryTopByte + BytesPerLine * (rct.bottom / FixedFontHeight - 1) > 0xFFFF)
				{
					MemoryTopByte -= BytesPerLine;
					MemoryCaretY += FixedFontHeight;
				}

				pByte = MemoryTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					TextOut(Paint.hdc, 0, y, ToHex(pByte, true), 4);
					x = 0;
					nBytes = 0;
					while (nBytes++ < BytesPerLine)
					{
						if (!(RetrieveAccess(pGameBoy, pByte) & MEM_READ))
						{
							TextOut(Paint.hdc, 6 * FixedFontWidth + 3 * x, y, "??", 2);
							TextOut(Paint.hdc, (7 + 3 * BytesPerLine) * FixedFontWidth + x, y, "?", 1);
						}
						else
						{
							if (pByte & 0x8000)
							{
								if (RetrieveAccess(pGameBoy, pByte) & MEM_CHANGED)
								{
									SetTextColor(Paint.hdc, RGB(0xFF, 0x00, 0x00));
								}
							}
							Byte = (BYTE)ReadMem(pGameBoy, pByte);
							TextOut(Paint.hdc, 6 * FixedFontWidth + 3 * x, y, ToHex(Byte, false), 2);
							TextOut(Paint.hdc, (7 + 3 * BytesPerLine) * FixedFontWidth + x, y, (char *)&Byte, 1);
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
					SetCaretPos(MemoryCaretX, MemoryCaretY);
					ShowCaret(hWin);
				}
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

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
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
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



CGameBoy	*DisAsmGameBoy;
BOOL		DisAsmCaret;
int			DisAsmCaretX, DisAsmCaretY;
WORD		DisAsmTopByte, DisAsmCaretByte, DisAsmLastPC;
int			DisAsmScroll;

LPARAM CALLBACK DisAsmWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	RECT			rct;
	LOGBRUSH		lb;
	HDC				hdc;
	HBRUSH			hBrush;
	HPEN			hPen;
	HANDLE			hBitmap;
	SCROLLINFO		si;
	WORD			pByte;
	BYTE			Byte, Byte2, Byte3, Status;
	int				x, y;
	BOOL			SetY;
	EMULATIONINFO	*pEmulationInfo;


	switch (uMsg)
	{
	case WM_SETFOCUS:
		if (GameBoyList.GetActive())
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
		if (!DisAsmGameBoy)
		{
			break;
		}
		switch (wParam)
		{
		case VK_UP:
			if (DisAsmCaretByte == 0)
			{
				return 0;
			}
			GetClientRect(hWin, &rct);
			if (DisAsmCaretY > rct.bottom || DisAsmCaretY < 0)
			{
				DisAsmCaretByte--;
				DisAsmTopByte = DisAsmCaretByte;
				InvalidateRect(hWin, NULL, true);
			}
			else
			{
				DisAsmCaretByte--;
				pByte = DisAsmTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					if (DisAsmCaretByte == pByte)
					{
						break;
					}

					Byte = (BYTE)ReadMem(DisAsmGameBoy, pByte);

					if (Byte != 0xCB)
					{
						if (OpCodeNames[Byte].Bytes & 7)
						{
							if (DisAsmCaretByte == ++pByte)
							{
								DisAsmCaretByte--;
								break;
							}

							if (OpCodeNames[Byte].Bytes & 2)
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
					pByte++;
					y += FixedFontHeight;
				}

				if ((DisAsmCaretY -= FixedFontHeight) < 0)
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
			return 0;

		case VK_DOWN:
			Byte = OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmCaretByte)].Bytes;
			Byte = (Byte & 3) + 1 + ((Byte >> 2) & 1);
			if (DisAsmCaretByte + Byte > 0xFFFF)
			{
				return 0;
			}
			DisAsmCaretByte += Byte;
			if (DisAsmCaretByte == DisAsmTopByte)
			{
				DisAsmCaretY = 0;
				if (DisAsmCaret)
				{
					HideCaret(hWin);
					SetCaretPos(DisAsmCaretX, DisAsmCaretY);
					ShowCaret(hWin);
				}
				return 0;
			}
			GetClientRect(hWin, &rct);
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
					Byte = (BYTE)ReadMem(DisAsmGameBoy, DisAsmCaretByte++);

					if (Byte != 0xCB)
					{
						if (OpCodeNames[Byte].Bytes & 7)
						{
							DisAsmCaretByte++;

							if (OpCodeNames[Byte].Bytes & 2)
							{
								DisAsmCaretByte++;
							}
						}
					}
					else
					{
						DisAsmCaretByte++;
					}
					y += FixedFontHeight;
				}
			}
			InvalidateRect(hWin, NULL, true);
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
				Byte = (BYTE)ReadMem(DisAsmGameBoy, DisAsmCaretByte++);
				DisAsmTopByte++;

				if (DisAsmCaretByte == 0xFFFF)
				{
					break;
				}
				if (Byte != 0xCB)
				{
					if (OpCodeNames[Byte].Bytes & 7)
					{
						DisAsmCaretByte++;
						DisAsmTopByte++;

						if (OpCodeNames[Byte].Bytes & 2)
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
				else
				{
					DisAsmCaretByte++;
					DisAsmTopByte++;
				}
				y += FixedFontHeight;
			}
			if (DisAsmCaretY < 0 || DisAsmCaretY >= rct.bottom)
			{
				DisAsmTopByte = DisAsmCaretByte;
			}
			InvalidateRect(hWin, NULL, true);
			return 0;
		}
		break;

	case WM_APP_STEPOVER:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		if (OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmGameBoy->Reg_PC)].Bytes & 8)
		{
			if (!DisAsmGameBoy->hThread)
			{
				pEmulationInfo = new EMULATIONINFO;
				pEmulationInfo->GameBoy1 = DisAsmGameBoy;
				pEmulationInfo->Flags = EMU_RUNTO;
				pEmulationInfo->RunToOffset = DisAsmGameBoy->Reg_PC + (OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmGameBoy->Reg_PC)].Bytes & 3) + 1;
				if (pEmulationInfo->RunToOffset >= 0x4000 && pEmulationInfo->RunToOffset < 0x8000)
				{
					pEmulationInfo->RunToBank = DisAsmGameBoy->ActiveRomBank;
				}
				if (pEmulationInfo->RunToOffset >= 0xA000 && pEmulationInfo->RunToOffset < 0xC000)
				{
					pEmulationInfo->RunToBank = DisAsmGameBoy->ActiveRamBank;
				}
				if (pEmulationInfo->RunToOffset >= 0xD000 && pEmulationInfo->RunToOffset < 0xE000 && DisAsmGameBoy->Flags & GB_ROM_COLOR)
				{
					pEmulationInfo->RunToBank = pFF00_C(DisAsmGameBoy, 0x4F) & 7;
				}
				DisAsmGameBoy->hThread = CreateThread(NULL, 0, StepGameLoop, pEmulationInfo, 0, &DisAsmGameBoy->ThreadId);
			}
			/*pByte = DisAsmGameBoy->Reg_PC + (OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmGameBoy->Reg_PC)].Bytes & 3) + 1;
			do
			{
				DisAsmGameBoy->Flags |= GB_EXITLOOP;
				DisAsmGameBoy->DebugMainLoop();
			}
			while (DisAsmGameBoy->Reg_PC == pByte && !(DisAsmGameBoy->Flags & GB_ERROR));
			DisAsmGameBoy->Flags &= ~(GB_EXITLOOP | GB_ERROR);
			DisAsmCaretByte = DisAsmGameBoy->Reg_PC;
			//SendMessage(hWin, WM_APP_SCROLLTOCURSOR, 0, 0);

			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);*/
			break;
		}

	case WM_APP_STEPINTO:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		if (!DisAsmGameBoy->hThread)
		{
			pEmulationInfo = new EMULATIONINFO;
			pEmulationInfo->GameBoy1 = DisAsmGameBoy;
			pEmulationInfo->Flags = EMU_STEPINTO;
			DisAsmGameBoy->hThread = CreateThread(NULL, 0, StepGameLoop, pEmulationInfo, 0, &DisAsmGameBoy->ThreadId);
		}
		/*pByte = DisAsmGameBoy->Reg_PC;
		do
		{
			DisAsmGameBoy->Flags |= GB_EXITLOOP;
			DisAsmGameBoy->DebugMainLoop();
		}
		while (DisAsmGameBoy->Reg_PC == pByte && !(DisAsmGameBoy->Flags & GB_ERROR));
		DisAsmGameBoy->Flags &= ~(GB_EXITLOOP | GB_ERROR);
		DisAsmCaretByte = DisAsmGameBoy->Reg_PC;
		//SendMessage(hWin, WM_APP_SCROLLTOCURSOR, 0, 0);

		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);*/
		return 0;

	case WM_APP_STEPOUT:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		if (!DisAsmGameBoy->hThread)
		{
			pEmulationInfo = new EMULATIONINFO;
			pEmulationInfo->GameBoy1 = DisAsmGameBoy;
			pEmulationInfo->Flags = EMU_STEPOUT;
			DisAsmGameBoy->hThread = CreateThread(NULL, 0, StepGameLoop, pEmulationInfo, 0, &DisAsmGameBoy->ThreadId);
		}
		return 0;

	case WM_APP_RUNTOCURSOR:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		if (!DisAsmGameBoy->hThread)
		{
			pEmulationInfo = new EMULATIONINFO;
			pEmulationInfo->GameBoy1 = DisAsmGameBoy;
			pEmulationInfo->Flags = EMU_RUNTO;
			pEmulationInfo->RunToOffset = DisAsmCaretByte;
			if (DisAsmCaretByte >= 0x4000 && DisAsmCaretByte < 0x8000)
			{
				pEmulationInfo->RunToBank = DisAsmGameBoy->ActiveRomBank;
			}
			if (DisAsmCaretByte >= 0xA000 && DisAsmCaretByte < 0xC000)
			{
				pEmulationInfo->RunToBank = DisAsmGameBoy->ActiveRamBank;
			}
			if (DisAsmCaretByte >= 0xD000 && DisAsmCaretByte < 0xE000 && DisAsmGameBoy->Flags & GB_ROM_COLOR)
			{
				pEmulationInfo->RunToBank = pFF00_C(DisAsmGameBoy, 0x4F) & 7;
			}
			DisAsmGameBoy->hThread = CreateThread(NULL, 0, StepGameLoop, pEmulationInfo, 0, &DisAsmGameBoy->ThreadId);
		}
		return 0;

	case WM_APP_SETNEXTSTATEMENT:
		if (DisAsmGameBoy)
		{
			DisAsmGameBoy->Reg_PC = DisAsmCaretByte;
			GetClientRect(hWin, &rct);
			rct.right = 15;
			InvalidateRect(hWin, &rct, true);
		}
		return 0;

	case WM_APP_TOGGLEBREAKPOINT:
		if (DisAsmGameBoy)
		{
			Status = RetrieveAccess(DisAsmGameBoy, DisAsmCaretByte);

			//Breakpoints cannot be placed on non-executable statements
			if (!(Status & MEM_EXECUTE))
			{
				return 0;
			}

			if (Status & MEM_BREAKPOINT)
			{
				Status &= ~MEM_BREAKPOINT;
			}
			else
			{
				Status |= MEM_BREAKPOINT;
			}
			SetAccess(DisAsmGameBoy, DisAsmCaretByte, Status);
			GetClientRect(hWin, &rct);
			rct.right = 15;
			InvalidateRect(hWin, &rct, true);
		}
		return 0;

	case WM_VSCROLL:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			if (DisAsmTopByte == 0)
			{
				return 0;
			}
			DisAsmTopByte--;
			InvalidateRect(hWin, NULL, true);
			break;

		case SB_LINEDOWN:
			GetClientRect(hWin, &rct);
			Byte = OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmTopByte)].Bytes;
			Byte = (Byte & 3) + 1 + ((Byte >> 2) & 1);
			if (DisAsmTopByte + Byte > 0xFFFF)
			{
				return 0;
			}
			DisAsmTopByte += Byte;
			if (DisAsmCaret)
			{
				HideCaret(hWin);
				SetCaretPos(DisAsmCaretX, DisAsmCaretY);
			}
			ScrollWindowEx(hWin, 0, -FixedFontHeight, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWin);
			if (DisAsmCaret)
			{
				ShowCaret(hWin);
			}
			break;

		case SB_PAGEUP:
			if (DisAsmTopByte == 0)
			{
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
				Byte = (BYTE)ReadMem(DisAsmGameBoy, DisAsmTopByte++);

				if (Byte != 0xCB)
				{
					if (OpCodeNames[Byte].Bytes & 7)
					{
						DisAsmTopByte++;
						if (OpCodeNames[Byte].Bytes & 2)
						{
							DisAsmTopByte++;
						}
					}
				}
				else
				{
					DisAsmTopByte++;
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
		return 0;

	case WM_LBUTTONDOWN:
		if (!DisAsmGameBoy)
		{
			return 0;
		}
		GetClientRect(hWin, &rct);
		DisAsmCaretByte = DisAsmTopByte;
		y = 0;
		while (y + FixedFontHeight < HIWORD(lParam))
		{
			Byte = (BYTE)ReadMem(DisAsmGameBoy, DisAsmCaretByte++);
			if (DisAsmCaretByte == 0)
			{
				DisAsmCaretByte = 0xFFFF;
				break;
			}

			if (Byte != 0xCB)
			{
				if (OpCodeNames[Byte].Bytes & 7)
				{
					DisAsmCaretByte++;
					if (DisAsmCaretByte == 0)
					{
						DisAsmCaretByte = 0xFFFF;
						break;
					}

					if (OpCodeNames[Byte].Bytes & 2)
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
			else
			{
				DisAsmCaretByte++;
				if (DisAsmCaretByte == 0)
				{
					DisAsmCaretByte = 0xFFFF;
					break;
				}
			}
			y += FixedFontHeight;
		}
		DisAsmCaretY = y;
		if (DisAsmCaret)
		{
			HideCaret(hWin);
			SetCaretPos(DisAsmCaretX, DisAsmCaretY);
			ShowCaret(hWin);
		}
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			if (pGameBoy = GameBoyList.GetActive())
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
					DisAsmCaretByte = DisAsmTopByte = DisAsmLastPC = DisAsmGameBoy->Reg_PC;
					DisAsmScroll = 0;
				}
				if (DisAsmLastPC != DisAsmGameBoy->Reg_PC)
				{
					DisAsmCaretByte = DisAsmLastPC = DisAsmGameBoy->Reg_PC;
					if (DisAsmCaretByte < DisAsmTopByte)
					{
						DisAsmTopByte = DisAsmCaretByte;
					}
					else
					{
						if (DisAsmCaretByte - DisAsmTopByte > rct.bottom / FixedFontHeight)
						{
							Byte = OpCodeNames[(BYTE)ReadMem(DisAsmGameBoy, DisAsmTopByte)].Bytes;
							Byte = (Byte & 3) + 1 + ((Byte >> 2) & 1);
							if (DisAsmTopByte + Byte <= 0xFFFF)
							{
								DisAsmTopByte += Byte;
							}
						}
						if (DisAsmCaretByte - DisAsmTopByte > rct.bottom / FixedFontHeight)
						{
							DisAsmTopByte = DisAsmCaretByte;
						}
					}
				}

				SetY = false;

				pByte = DisAsmTopByte;
				y = 0;
				while (y <= rct.bottom)
				{
					TextOut(Paint.hdc, 15, y, ToHex(pByte, true), 4);

					if (DisAsmCaretByte == pByte)
					{
						DisAsmCaretY = y;
						SetY = true;
						if (DisAsmCaret)
						{
							SetCaretPos(DisAsmCaretX, DisAsmCaretY);
						}
					}

					Status = RetrieveAccess(pGameBoy, pByte);
					if (Status & MEM_BREAKPOINT)
					{
						hBitmap = LoadImage(hInstance, MAKEINTRESOURCE(IDB_BREAKPOINT), IMAGE_BITMAP, 0, 0, 0);
						hdc = CreateCompatibleDC(Paint.hdc);
						hBitmap = SelectObject(hdc, hBitmap);
						BitBlt(Paint.hdc, 0, y + 1, 11, y + 12, hdc, 0, 0, SRCCOPY);
						hBitmap = SelectObject(hdc, hBitmap);
						DeleteObject(hBitmap);
						DeleteDC(hdc);
					}

					if (DisAsmGameBoy->Reg_PC == pByte)
					{
						hBitmap = LoadImage(hInstance, MAKEINTRESOURCE(IDB_CURRENTSTATEMENT), IMAGE_BITMAP, 0, 0, 0);
						hdc = CreateCompatibleDC(Paint.hdc);
						hBitmap = SelectObject(hdc, hBitmap);
						//BitBlt(Paint.hdc, 3, y + 1, 14, y + 12, hdc, 0, 0, SRCCOPY);
						TransparentBlt(Paint.hdc, 3, y + 1, 11, 11, hdc, 0, 0, 11, 11, RGB(0xFF, 0xFF, 0xFF));
						hBitmap = SelectObject(hdc, hBitmap);
						DeleteObject(hBitmap);
						DeleteDC(hdc);
					}

					if (Status & MEM_READ)
					{
						Byte = (BYTE)ReadMem(pGameBoy, pByte);
						TextOut(Paint.hdc, 15 + 5 * FixedFontWidth, y, ToHex(Byte, false), 2);
						if (Byte != 0xCB)
						{
							TextOut(Paint.hdc, 15 + 14 * FixedFontWidth, y, OpCodeNames[Byte].Text1, strlen(OpCodeNames[Byte].Text1));
							x = 15 + (14 + strlen(OpCodeNames[Byte].Text1)) * FixedFontWidth;
							if (OpCodeNames[Byte].Bytes & 7)
							{
								Byte2 = (BYTE)ReadMem(pGameBoy, ++pByte);

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

								if (OpCodeNames[Byte].Bytes & 2)
								{
									Byte3 = (BYTE)ReadMem(pGameBoy, ++pByte);

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

									TextOut(Paint.hdc, 15 + 11 * FixedFontWidth, y, ToHex(Byte3, false), 2);
									TextOut(Paint.hdc, x, y, NumBuffer, 2);
									x += 2 * FixedFontWidth;
								}
								TextOut(Paint.hdc, 15 + 8 * FixedFontWidth, y, ToHex(Byte2, false), 2);
								if (OpCodeNames[Byte].Bytes & 4)
								{
									TextOut(Paint.hdc, x, y, ToHex(pByte + 1 + (Byte2 & 0x80 ? - (short)((BYTE)-Byte2) : Byte2), true), 4);
									x += 4 * FixedFontWidth;
								}
								else
								{
									if (Byte != 0x10)
									{
										TextOut(Paint.hdc, x, y, NumBuffer, 2);
										x += 2 * FixedFontWidth;
									}
								}
							}
							TextOut(Paint.hdc, x, y, OpCodeNames[Byte].Text2, strlen(OpCodeNames[Byte].Text2));
						}
						else
						{
							Byte = (BYTE)ReadMem(pGameBoy, ++pByte);

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

							TextOut(Paint.hdc, 15 + 8 * FixedFontWidth, y, ToHex(Byte, false), 2);
							TextOut(Paint.hdc, 15 + 14 * FixedFontWidth, y, OpCodeCBNames[Byte >> 3], strlen(OpCodeCBNames[Byte >> 3]));
							x = 15 + (14 + strlen(OpCodeCBNames[Byte >> 3])) * FixedFontWidth;
							TextOut(Paint.hdc, x, y, OpCodeCBArgument[Byte & 7], strlen(OpCodeCBArgument[Byte & 7]));
						}
					}
					else
					{
						TextOut(Paint.hdc, 15 + 5 * FixedFontWidth, y, "??", 2);
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

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
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
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



int		RegisterCaretX, RegisterCaretY;
BOOL	RegisterCaret;

LPARAM CALLBACK RegisterWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	CGameBoy		*pGameBoy;
	BYTE			*pByte;
	BOOL			HiNibble;


	switch (uMsg)
	{
	case WM_SETFOCUS:
		if (GameBoyList.GetActive())
		{
			CreateCaret(hWin, NULL, 2, 16);
			SetCaretPos(RegisterCaretX, RegisterCaretY);
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
		if (!(pGameBoy = GameBoyList.GetActive()))
		{
			return 0;
		}
		while (LOWORD(lParam))
		{
			if ((wParam >= '0' && wParam <= '9') || (wParam >= 'A' && wParam <= 'F'))
			{
				switch (RegisterCaretY)
				{
				case 0:		pByte = (BYTE *)&pGameBoy->Reg_AF; break;
				case 15:	pByte = (BYTE *)&pGameBoy->Reg_BC; break;
				case 30:	pByte = (BYTE *)&pGameBoy->Reg_DE; break;
				case 45:	pByte = (BYTE *)&pGameBoy->Reg_HL; break;
				case 60:	pByte = (BYTE *)&pGameBoy->Reg_SP; break;
				case 75:	pByte = (BYTE *)&pGameBoy->Reg_PC; break;
				}
				HiNibble = false;
				if (RegisterCaretY < 100)
				{
					if (RegisterCaretX >= 20 + 4 * FixedFontWidth)
					{
						return 0;
					}
					if (RegisterCaretX <= 20 + FixedFontWidth)
					{
						pByte++;
					}
					if (RegisterCaretX == 20 || RegisterCaretX == 20 + 2 * FixedFontWidth)
					{
						HiNibble = true;
					}
				}
				else
				{
					return 0;
				}
				RegisterCaretX += FixedFontWidth;
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
				if (RegisterCaretX <= 20)
				{
					return 0;
				}
				if (RegisterCaretY >= 100 && RegisterCaretX <= 40)
				{
					return 0;
				}
				RegisterCaretX -= FixedFontWidth;
				break;

			case VK_RIGHT:
				if (RegisterCaretY < 100)
				{
					if (RegisterCaretX >= 20 + 4 * FixedFontWidth)
					{
						return 0;
					}
				}
				if (RegisterCaretY == 115)
				{
					if (RegisterCaretX >= 40 + FixedFontWidth)
					{
						return 0;
					}
				}
				else
				{
					if (RegisterCaretX >= 40 + 2 * FixedFontWidth)
					{
						return 0;
					}
				}
				RegisterCaretX += FixedFontWidth;
				break;

			case VK_UP:
				switch (RegisterCaretY)
				{
				case 0:
					return 0;

				case 100:
					if (RegisterCaretX > 20 + 4 * FixedFontWidth)
					{
						RegisterCaretX = 20 + 4 * FixedFontWidth;
					}
					if ((RegisterCaretX - 20) % FixedFontWidth)
					{
						if ((RegisterCaretX - 20) % FixedFontWidth <= FixedFontWidth / 2)
						{
							RegisterCaretX -= (RegisterCaretX - 20) % FixedFontWidth;
						}
						else
						{
							RegisterCaretX += FixedFontWidth - ((RegisterCaretX - 20) % FixedFontWidth);
						}
					}
					RegisterCaretY -= 25;
					break;

				case 130:
					if (RegisterCaretX > 40 + FixedFontWidth)
					{
						RegisterCaretX = 40 + FixedFontWidth;
					}
				default:
					RegisterCaretY -= 15;
				}
				break;

			case VK_DOWN:
				switch (RegisterCaretY)
				{
				case 75:
					if (RegisterCaretX < 40)
					{
						RegisterCaretX = 40;
					}
					if ((RegisterCaretX - 40) % FixedFontWidth)
					{
						if ((RegisterCaretX - 40) % FixedFontWidth <= FixedFontWidth / 2)
						{
							RegisterCaretX -= (RegisterCaretX - 40) % FixedFontWidth;
						}
						else
						{
							RegisterCaretX += FixedFontWidth - ((RegisterCaretX - 40) % FixedFontWidth);
						}
					}
					if (RegisterCaretX > 40 + 2 * FixedFontWidth)
					{
						RegisterCaretX = 40 + 2 * FixedFontWidth;
					}
					RegisterCaretY += 25;
					break;

				case 130:
					return 0;

				case 100:
					if (RegisterCaretX > 40 + FixedFontWidth)
					{
						RegisterCaretX = 40 + FixedFontWidth;
					}
				default:
					RegisterCaretY += 15;
					break;
				}
			}
			if (RegisterCaret)
			{
				HideCaret(hWin);
				SetCaretPos(RegisterCaretX, RegisterCaretY);
				ShowCaret(hWin);
			}

			lParam--;
		}
		return 0;

	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			if (pGameBoy = GameBoyList.GetActive())
			{
				if (pGameBoy->hThread)
				{
					InvalidateRect(hWin, NULL, true);
					GetUpdateRect(hWin, NULL, true);
				}
			}
			BeginPaint(hWin, &Paint);

			SelectObject(Paint.hdc, hFixedFont);
			//SetBkMode(Paint.hdc, TRANSPARENT);
			SetBkColor(Paint.hdc, 0xFFFFFF);
			SetTextColor(Paint.hdc, 0);
			TextOut(Paint.hdc, 0, 0, "AF", 2);
			TextOut(Paint.hdc, 0, 15, "BC", 2);
			TextOut(Paint.hdc, 0, 30, "DE", 2);
			TextOut(Paint.hdc, 0, 45, "HL", 2);
			TextOut(Paint.hdc, 0, 60, "SP", 2);
			TextOut(Paint.hdc, 0, 75, "PC", 2);
			TextOut(Paint.hdc, 0, 100, "ROM", 3);
			TextOut(Paint.hdc, 0, 115, "SVBK", 4);
			TextOut(Paint.hdc, 0, 130, "SRAM", 4);
			TextOut(Paint.hdc, 0, 150, "LY", 2);
			if (pGameBoy)
			{
				TextOut(Paint.hdc, 20, 0, ToHex(pGameBoy->Reg_AF, true), 4);
				if (pGameBoy->Reg_F & Flag_Z)
				{
					TextOut(Paint.hdc, 60, 0, "Z", 1);
				}
				if (pGameBoy->Reg_F & Flag_N)
				{
					TextOut(Paint.hdc, 70, 0, "N", 1);
				}
				if (pGameBoy->Reg_F & Flag_H)
				{
					TextOut(Paint.hdc, 80, 0, "H", 1);
				}
				if (pGameBoy->Reg_F & Flag_C)
				{
					TextOut(Paint.hdc, 90, 0, "C", 1);
				}

				TextOut(Paint.hdc, 20, 15, ToHex(pGameBoy->Reg_BC, true), 4);
				TextOut(Paint.hdc, 20, 30, ToHex(pGameBoy->Reg_DE, true), 4);
				TextOut(Paint.hdc, 20, 45, ToHex(pGameBoy->Reg_HL, true), 4);
				TextOut(Paint.hdc, 20, 60, ToHex(pGameBoy->Reg_SP, true), 4);
				TextOut(Paint.hdc, 20, 75, ToHex(pGameBoy->Reg_PC, true), 4);
				TextOut(Paint.hdc, 40, 100, ToHex(pGameBoy->ActiveRomBank, false), 2);
				NumBuffer[0] = (pGameBoy->FF00_C(0x70) & 7) + 48;
				if (NumBuffer[0] == 48)
				{
					NumBuffer[0]++;
				}
				TextOut(Paint.hdc, 40, 115, NumBuffer, 1);
				TextOut(Paint.hdc, 40, 130, ToHex(pGameBoy->ActiveRamBank, false), 2);
				TextOut(Paint.hdc, 60, 150, ToHex(pGameBoy->FF00_C(0x44), false), 2);
				switch (pGameBoy->FF00_C(0x41) & 3)
				{
				case 0:
					TextOut(Paint.hdc, 0, 165, "H-Blank", 7);
					break;

				case 1:
					TextOut(Paint.hdc, 0, 165, "V-Blank", 7);
					break;

				case 2:
					TextOut(Paint.hdc, 0, 165, "OAM", 3);
					break;

				case 3:
					TextOut(Paint.hdc, 0, 165, "LCD", 3);
					break;
				}
				TextOut(Paint.hdc, 60, 165, ToHex(pGameBoy->LCD_Ticks, false), 2);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			return 0;
		}
		break;

	case WM_CREATE:
		RegisterCaret = false;
		RegisterCaretX = 20;
		RegisterCaretY = 0;
		return 0;

	case WM_DESTROY:
		hRegisters = NULL;
		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}



LPARAM CALLBACK HardwareWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CGameBoy		*pGameBoy;
	PAINTSTRUCT		Paint;
	DWORD			Size;
	DWORD			x, y;
	SCROLLINFO		si;


	switch (uMsg)
	{
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
			TextOut(Paint.hdc, FixedFontWidth - x, FixedFontHeight - y, "ROM Size:", 9);
			TextOut(Paint.hdc, FixedFontWidth - x, FixedFontHeight + FixedFontHeight - y, "RAM Size:", 9);
			if (pGameBoy = GameBoyList.GetActive())
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

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
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
		DisplayErrorMessage(NULL);
		return true;
	}
	hRegisters = NULL;
	wc.lpfnWndProc = DisAsmWndProc;
	wc.lpszClassName = "DisAsm";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hDisAsm = NULL;
	wc.lpfnWndProc = MemoryWndProc;
	wc.lpszClassName = "Memory";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hMemory = NULL;

	wc.style = 0;
	wc.lpfnWndProc = TilesWndProc;
	wc.lpszClassName = "Tiles";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hTiles = NULL;
	wc.lpfnWndProc = PalettesWndProc;
	wc.lpszClassName = "Palettes";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hPalettes = NULL;
	wc.lpfnWndProc = TileMapWndProc;
	wc.lpszClassName = "TileMap";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hTileMap = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = HardwareWndProc;
	wc.lpszClassName = "Hardware";
	if (!RegisterClass(&wc))
	{
		DisplayErrorMessage(NULL);
		return true;
	}
	hHardware = NULL;

	return false;
}

