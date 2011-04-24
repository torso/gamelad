#include	<windows.h>
#include	<commctrl.h>
#include	<vfw.h>

#define		GAME_BOY_CPP
#include	"CDebugInfo.h"
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CCheats.h"
#include	"CGameBoys.h"
#include	"Z80.h"
#include	"Debugger.h"
#include	"resource.h"
#include	"Input.h"
#include	"Gfx.h"
//#include	"Network.h"



#define		GAME_LAD_SAVE_STATE_VERSION			1

#define		STOPEMULATION									\
	CloseSound();											\
	CloseAVI();												\
	RefreshScreen();										\
	PostMessage(hWnd, WM_APP_RELEASEGFX, 0, (LPARAM)this);	\
	if (m_hSearchCheatWnd)									\
	{														\
		InvalidateRect(m_hSearchCheatWnd, NULL, true);		\
	}

#define		PAUSEEMULATION									\
	SetEvent(hStartStopEvent);								\
	SoundBuffer.IsPlaying = false;

#define		RESUMEEMULATION									\
	SetEvent(hStartStopEvent);								\
	SetRTCReg(this);										\
	SoundBuffer.IsPlaying = true;


#define		STOPEMULATION2									\
	CloseSound();											\
	CloseAVI();												\
	RefreshScreen();										\
	PostMessage(hWnd, WM_APP_RELEASEGFX, 0, (LPARAM)this);	\
	if (m_hSearchCheatWnd)									\
	{														\
		InvalidateRect(m_hSearchCheatWnd, NULL, true);		\
	}														\
	/*if (pLinkGameBoy)										\
	{*/														\
		pLinkGameBoy->CloseSound();							\
		pLinkGameBoy->CloseAVI();							\
		pLinkGameBoy->RefreshScreen();						\
		PostMessage(hWnd, WM_APP_RELEASEGFX, 0, (LPARAM)pLinkGameBoy);\
		if (pLinkGameBoy->m_hSearchCheatWnd)				\
		{													\
			InvalidateRect(pLinkGameBoy->m_hSearchCheatWnd, NULL, true);\
		}/*													\
	}														\
	else													\
	{														\
		CloseNetworkLinkCable(1);							\
	}*/

#define		PAUSEEMULATION2									\
	SetEvent(hStartStopEvent);								\
	SoundBuffer.IsPlaying = false;							\
	/*if (pLinkGameBoy)										\
	{*/														\
		pLinkGameBoy->SoundBuffer.IsPlaying = false;/*		\
	}*/

#define		RESUMEEMULATION2								\
	SetEvent(hStartStopEvent);								\
	SetRTCReg(this);										\
	SoundBuffer.IsPlaying = true;							\
	/*if (pLinkGameBoy)										\
	{*/														\
		SetRTCReg(pLinkGameBoy);							\
		pLinkGameBoy->SoundBuffer.IsPlaying = true;/*		\
	}*/



void CloseSound_asm(CGameBoy *pGameBoy);

AVICOMPRESSOPTIONS	aco, *aaco[1] = {&aco};



/*BYTE		NintendoGraphic[48] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};*/
WORD		GreyScales[4] =	{	0x7FFF,
								15855, //011110111101111
								11627, //010110101101011
								0
							};



BYTE RomSize(BYTE Byte148)
{
	if (Byte148 <= 7)
	{
		return (2 << Byte148) - 1;
	}

	switch (Byte148)
	{
	case 0x52:
		return 71;

	case 0x53:
		return 79;

	case 0x54:
		return 95;
	}

	return 0;
}



/*WORD	Palette[32768], RealPalette[32768];



int GetValue(int min,int max,int v)
{
	return (int)(min + (float)(max - min) * (2.0 * (v / 31.0) - (v / 31.0) * (v / 31.0)));
}



void CreatePalettes()
{
	int		r, g, b;
	int		nr, ng, nb;


	for (r = 0; r < 32; r++)
	{
		for (g = 0; g < 32; g++)
		{
			for (b = 0; b < 32; b++)
			{
				nr = GetValue(GetValue(4, 14, g), GetValue(24, 29, g), r) - 4;
				ng = GetValue(GetValue(4 + GetValue(0, 5, r), 14 + GetValue(0, 3, r), b),
					GetValue(24 + GetValue(0, 3, r), 29 + GetValue(0, 1, r), b), g) - 4;
				nb = GetValue(GetValue(4 + GetValue(0, 5, r), 14 + GetValue(0, 3, r), g),
					GetValue(24 + GetValue(0, 3, r), 29 + GetValue(0, 1, r), g), b) - 4;
				Palette[(b << 10) | (g << 5) | r] = (nr << 10) | (ng << 5) | nb;
			}
		}
	}
}*/



/*#ifdef LCD

__forceinline __declspec(naked) __fastcall EraseLine(WORD *Dest)
{
	__asm
	{
		push	ebx

		mov		eax, 0x50
FillMore:
		dec		eax
		mov		ebx, dword ptr [ecx + 4 * eax]
		add		ebx, (10570 << 16) | 10570
		jns		HiOk
		or		ebx, 0x7FFF0000
HiOk:
		test	ebx, 0x00008000
		jz		LowOk
		or		ebx, 0x00007FFF
LowOk:
		and		ebx, 0x7FFF7FFF
		mov		[ecx + 4 * eax], ebx
		test	eax, eax
		jnz		FillMore

		pop		ebx
		ret
	}
}

#else*/

/*__forceinline __declspec(naked) __fastcall EraseLine(WORD *Dest)
{
	__asm
	{
		mov		eax, 0x50
FillMore:
		dec		eax
		mov		dword ptr [ecx + 4 * eax], 0xFFFFFFFF
		jnz		FillMore

		ret
	}
}*/

//#endif



CGameBoy::CGameBoy(BYTE Flags)
{
	ZeroMemory(this, sizeof(*this));

	this->Flags = Flags & GB_COLOR;
	if (Settings.FrameSkipAuto)
	{
		FrameSkip = 0xFF;
	}
	else
	{
		FrameSkip = Settings.FrameSkip;
	}

	/*CreatePalettes();
	pPalette = Palette;*/
}



CGameBoy::~CGameBoy()
{
	HANDLE		hTempThread;


	if (hTempThread = hThread)
	{
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(hTempThread, INFINITE);
	}
	CloseSound();

	delete MEM_ROM;
	delete MemStatus_ROM;
	delete MemStatus_RAM;

	if (pDebugInfo)
	{
		delete pDebugInfo;
	}

	if (m_pCheatList)
	{
		delete m_pCheatList;
	}

	if (hGBWnd)
	{
		SetWindowLong(hGBWnd, GWL_USERDATA, NULL);
		SendMessage(hGBWnd, WM_CLOSE, 0, 0);
	}
	if (hGBBitmap)
	{
		SelectObject(hGBDC, hOldBitmap);
		DeleteObject(hGBBitmap);
	}
	if (hGBDC)
	{
		DeleteDC(hGBDC);
	}
	if (m_hSearchCheatWnd)
	{
		DestroyWindow(m_hSearchCheatWnd);
	}
}



void CGameBoy::AddRef()
{
	EnterCriticalSection(&csGameBoy);
	if (!GameBoys.GameBoyExists(this))
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		LeaveCriticalSection(&csGameBoy);
		return;
	}
	RefCount++;
	LeaveCriticalSection(&csGameBoy);
}



void CGameBoy::Release()
{
	BOOL		cs;


	if (!Terminating)
	{
		EnterCriticalSection(&csGameBoy);
		cs = true;
	}
	else
	{
		cs = false;
	}

	if (!GameBoys.GameBoyExists(this))
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		if (cs)
		{
			LeaveCriticalSection(&csGameBoy);
		}
		return;
	}
	if (RefCount > 0)
	{
		RefCount--;
	}
	if (cs)
	{
		LeaveCriticalSection(&csGameBoy);
	}
}



BOOL CGameBoy::CanUnload()
{
	EnterCriticalSection(&csGameBoy);

	if (RefCount == 0)
	{
		LeaveCriticalSection(&csGameBoy);
		return true;
	}

	LeaveCriticalSection(&csGameBoy);
	return false;
}



/*BYTE	BitNo, BitByte;

void ResetReadBits()
{
	BitNo = 8;
	BitByte = 0;
}



BOOL ReadBits(HANDLE hFile, BYTE nBits, DWORD *pByte)
{
	DWORD		nBytes;
	BYTE		BitsCopied;


	if (8 - (signed)BitNo >= (signed)nBits)
	{
		*pByte = BitByte & ~(-1 << nBits);
		BitByte >>= nBits;
		BitNo += nBits;
		return true;
	}

	*pByte = BitByte;
	BitsCopied = 8 - BitNo;
	while (true)
	{
		if (!ReadFile(hFile, &BitByte, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage();
			return false;
		}
		*pByte |= (BitByte << (BitsCopied)) & ~(-1 << nBits);
		if (BitsCopied + 8 >= nBits)
		{
			BitNo = nBits - BitsCopied;
			BitByte >>= BitNo;
			break;
		}
		BitsCopied += 8;
	}
	return true;
}



//Order of the bit length code lengths
BYTE border[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};



#define BMAX 16         //maximum bit length of any code (16 for explode)
#define N_MAX 288       //maximum number of codes in any set



/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */
/*struct huft {
  BYTE e;                //number of extra bits or operation
  BYTE b;                //number of bits in this code or subcode
  union {
    WORD n;              //literal, length base, or distance base
    struct huft *t;     //pointer to next level of table
  } v;
};



WORD mask[] =
{
	0x0000,
	0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
	0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};


static WORD cplens[] = {         //Copy lengths for literal codes 257..285
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        //note: see note #13 above about the 258 in this list.
static WORD cplext[] = {         //Extra bits for literal codes 257..285
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; //99==invalid
static WORD cpdist[] = {         //Copy offsets for distance codes 0..29
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
static WORD cpdext[] = {         //Extra bits for distance codes
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};


int lbits = 9;          //bits in base literal/length lookup table
int dbits = 6;          //bits in base distance lookup table



/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
/*int huft_build(
DWORD *b,				//code lengths in bits (all assumed <= BMAX)
DWORD n,				//number of codes (assumed <= N_MAX)
DWORD s,				//number of simple-valued codes (0..s-1)
WORD *d,					//list of base values for non-simple codes
WORD *e,					//list of extra bits for non-simple codes
struct huft **t,		//result: starting table
int *m)					//maximum lookup bits, returns actual
{
	DWORD a;                   //counter for codes of length k
	DWORD c[BMAX+1];           //bit length count table
	DWORD el;                  //length of EOB code (value 256)
	DWORD f;                   //i repeats in table every f entries
	int g;                        //maximum code length
	int h;                        //table level
	DWORD i;          //counter, current code
	DWORD j;          //counter
	int k;               //number of bits in current code
	int lx[BMAX+1];               //memory for l[-1..BMAX-1]
	int *l = lx+1;                //stack of bits per table
	DWORD *p;         //pointer into c[], b[], or v[]
	struct huft *q;      //points to current table
	struct huft r;                //table entry for structure assignment
	struct huft *u[BMAX];         //table stack
	static DWORD v[N_MAX];     //values in order of bit length
	int w;               //bits before this table == (l * h)
	DWORD x[BMAX+1];           //bit offsets, then code stack
	DWORD *xp;                 //pointer into x
	int y;                        //number of dummy codes added
	DWORD z;                   //number of entries in current table


	//Generate counts for each bit length
	el = n > 256 ? b[256] : BMAX; //set length of EOB code, if any
	ZeroMemory(c, sizeof(c));
	p = b;
	i = n;
	do
	{
		c[*p]++;
		p++;               //assume all entries <= BMAX
	}
	while (--i);
	if (c[0] == n)                //null input--all zero length codes
	{
		*t = (struct huft *)NULL;
		*m = 0;
		return 0;
	}


	//Find minimum and maximum length, bound *m by those
	for (j = 1; j <= BMAX; j++)
	{
		if (c[j])
		{
			break;
		}
	}
	k = j;                        //minimum code length
	if ((unsigned)*m < j)
	{
		*m = j;
	}
	for (i = BMAX; i; i--)
	{
		if (c[i])
		{
			break;
		}
	}
	g = i;                        //maximum code length
	if ((unsigned)*m > i)
	{
		*m = i;
	}


	//Adjust last length count to fill out codes, if needed
	for (y = 1 << j; j < i; j++, y <<= 1)
	{
		if ((y -= c[j]) < 0)
		{
			return 2;                 //bad input: more codes than bits
		}
	}
	if ((y -= c[i]) < 0)
	{
		return 2;
	}
	c[i] += y;


	//Generate starting offsets into the value table for each length
	x[1] = j = 0;
	p = c + 1;  xp = x + 2;
	while (--i)                 //note that i == g from above
	{
		*xp++ = (j += *p++);
	}


	//Make a table of values in order of bit lengths
	p = b;
	i = 0;
	do
	{
		if ((j = *p++) != 0)
		{
			v[x[j]++] = i;
		}
	}
	while (++i < n);


	//Generate the Huffman codes and for each, make the table entries
	x[0] = i = 0;                 //first Huffman code is zero
	p = v;                        //grab values in bit order
	h = -1;                       //no tables yet--level -1
	w = l[-1] = 0;                //no bits decoded yet
	u[0] = (struct huft *)NULL;   //just to keep compilers happy
	q = (struct huft *)NULL;      //ditto
	z = 0;                        //ditto

	//go through the bit lengths (k already is bits in shortest code)
	for (; k <= g; k++)
	{
		a = c[k];
		while (a--)
		{
			//here i is the Huffman code of length k bits for value *p
			//make tables up to required level
			while (k > w + l[h])
			{
				w += l[h++];            //add bits already decoded

				//compute minimum size table less than or equal to *m bits
				z = (z = g - w) > (unsigned)*m ? *m : z;        //upper limit
				if ((f = 1 << (j = k - w)) > a + 1)     //try a k-w bit table
				{                       //too few codes for k-w bit table
					f -= a + 1;           //deduct codes from patterns left
					xp = c + k;
					while (++j < z)       //try smaller tables up to z bits
					{
						if ((f <<= 1) <= *++xp)
						{
							break;            //enough codes to use up j bits
						}
						f -= *xp;           //else deduct codes from patterns
					}
				}
				if ((unsigned)w + j > el && (unsigned)w < el)
				{
					j = el - w;           //make EOB code end at table
				}
				z = 1 << j;             //table entries for j-bit table
				l[h] = j;               //set table size in stack

				//allocate and link in new table
				//if ((q = (struct huft *)malloc((z + 1)*sizeof(struct huft))) == (struct huft *)NULL)
				if (!(q = new huft[z + 1]))
				{
					if (h)
					{
						delete u[0];
					}
					return 3;             //not enough memory
				}
				//hufts += z + 1;         //track memory usage
				*t = q + 1;             //link to list for huft_free()
				*(t = &(q->v.t)) = (struct huft *)NULL;
				u[h] = ++q;             //table starts after link

				//connect to last table, if there is one
				if (h)
				{
					x[h] = i;             //save pattern for backing up
					r.b = (BYTE)l[h-1];    //bits to dump before this table
					r.e = (BYTE)(16 + j);  //bits in this table
					r.v.t = q;            //pointer to this table
					j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
					u[h-1][j] = r;        //connect to last table
				}
			}

			//set up table entry in r
			r.b = (BYTE)(k - w);
			if (p >= v + n)
			{
				r.e = 99;               //out of values--invalid code
			}
			else if (*p < s)
			{
				r.e = (BYTE)(*p < 256 ? 16 : 15);    //256 is end-of-block code
				r.v.n = (WORD)*p++;           //simple code is just the value
			}
			else
			{
				r.e = (BYTE)e[*p - s];   //non-simple--look up in lists
				r.v.n = d[*p++ - s];
			}

			//fill code-like entries with r
			f = 1 << (k - w);
			for (j = i >> w; j < z; j += f)
			{
				q[j] = r;
			}

			//backwards increment the k-bit code i
			for (j = 1 << (k - 1); i & j; j >>= 1)
			{
				i ^= j;
			}
			i ^= j;

			//backup over finished tables
			while ((i & ((1 << w) - 1)) != x[h])
			{
				w -= l[--h];            //don't need to update q
			}
		}
	}


	//return actual size of base table
	*m = l[0];


	//Return true (1) if we were given an incomplete table
	return y != 0 && g != 1;
}*/



/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */

/*
tl, td		literal/length and distance decoder tables
bl, bd		number of bits decoded by tl[] and td[]
*/
/*int inflate_codes(HANDLE hFile, struct huft *tl, struct huft *td, int bl, int bd, BYTE *pOutput, DWORD dwOutputOffset)
{
	DWORD			e;		//table entry flag/number of extra bits
	DWORD			n, d;	//length and index for copy
	struct huft		*t;		//pointer to table entry
	DWORD			ml, md;	//masks for bl and bd bits
	DWORD			b;		//bit buffer


	//Inflate the coded data
	ml = mask[bl];           //precompute masks for speed
	md = mask[bd];
	//while (true)
	//{
		//NEEDBITS((unsigned)bl)
		ReadBits(hFile, bl, &b);
		if ((e = (t = tl + (b & ml))->e) > 16)
		{
			do
			{
				if (e == 99)
				{
					return 1;
				}
				//DUMPBITS(t->b)
				e -= 16;
				ReadBits(hFile, (BYTE)e, &b);
				//NEEDBITS(e)
			}
			while ((e = (t = t->v.t + (b & mask[e]))->e) > 16);
		}
		//DUMPBITS(t->b)
		/*if (e == 16)                //then it's a literal
		{
			slide[w++] = (uch)t->v.n;
			if (w == WSIZE)
			{
				FLUSH(w);
				w = 0;
			}
		}
		else                        //it's an EOB or a length
		{
			//exit if end of block
			if (e == 15)
			{
				break;
			}

			//get length of block to copy
			NEEDBITS(e)
			n = t->v.n + ((unsigned)b & mask[e]);
			DUMPBITS(e);

			//decode distance of block to copy
			NEEDBITS((unsigned)bd)
			if ((e = (t = td + ((unsigned)b & md))->e) > 16)
			{
				do
				{
					if (e == 99)
					{
						return 1;
					}
					DUMPBITS(t->b)
					e -= 16;
					NEEDBITS(e)
				}
				while ((e = (t = t->v.t + ((unsigned)b & mask[e]))->e) > 16);
			}
			DUMPBITS(t->b)
			NEEDBITS(e)
			d = w - t->v.n - ((unsigned)b & mask[e]);
			DUMPBITS(e)

			//do the copy
			do
			{
				n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
#ifndef NOMEMCPY
				if (w - d >= e)         //(this test assumes unsigned comparison)
				{
					memcpy(slide + w, slide + d, e);
					w += e;
					d += e;
				}
				else                      //do it slow to avoid memcpy() overlap
#endif //!NOMEMCPY
				do
				{
					slide[w++] = slide[d++];
				}
				while (--e);
				if (w == WSIZE)
				{
					FLUSH(w);
					w = 0;
				}
			}
			while (n);
		}
	}


	//restore the globals from the locals
	wp = w;                       //restore global window pointer
	*/

/*	return 0;
}*/



BOOL CGameBoy::Init(char *pszROMFilename, char *pszStateFilename, char *pszBatteryFilename)
{
	HANDLE			hFile;
	DWORD			FileSize, nBytes;//, Value;
	BITMAPINFO		bmi;
	BOOL			Maximized;//, zip;
	char			szBuffer[0x100 * 2];
	SYSTEMTIME		SystemTime;


	LOG("CGameBoy::Init(\"");
	LOG(pszROMFilename);
	LOG("\", \"");
	LOG(pszStateFilename);
	LOG("\", \"");
	LOG(pszBatteryFilename);
	LOG("\")\n");

	//zip = false;

	if (!pszROMFilename || pszROMFilename[0] == '\0')
	{
		if (!pszStateFilename)
		{
			LOG("CGameBoy::Init: true");
			return true;
		}
		if (pszStateFilename[0] == '\0')
		{
			LOG("CGameBoy::Init: true");
			return true;
		}

		strcpy(Rom, pszStateFilename);
		if (strchr(Rom, '.') > Rom)
		{
			*strrchr(Rom, '.') = '\0';
		}

		if (Rom[0] == '\0')
		{
			LOG("CGameBoy::Init: true");
			return true;
		}

		if (SearchPath(NULL, Rom, ".gb", MAX_PATH, Rom, NULL) == 0)
		{
			if (SearchPath(NULL, Rom, ".gbc", MAX_PATH, Rom, NULL) == 0)
			{
				if (Rom[0] == '\0')
				{
					MessageBox(hMsgParent, String(IDS_STATUS_ROMNOTFOUND), NULL, MB_OK | MB_ICONERROR);
					LOG("CGameBoy::Init: true");
					return true;
				}
				if (Rom[strlen(Rom) - 1] >= '0' && Rom[strlen(Rom) - 1] <= '9')
				{
					StateSlot = Rom[strlen(Rom) - 1] - '0';

					Rom[strlen(Rom) - 1] = '\0';
					if (SearchPath(NULL, Rom, ".gb", MAX_PATH, Rom, NULL) == 0)
					{
						if (SearchPath(NULL, Rom, ".gbc", MAX_PATH, Rom, NULL) == 0)
						{
							MessageBox(hMsgParent, String(IDS_STATUS_ROMNOTFOUND), NULL, MB_OK | MB_ICONERROR);
							LOG("CGameBoy::Init: true");
							return true;
						}
					}
				}
				else
				{
					MessageBox(hMsgParent, String(IDS_STATUS_ROMNOTFOUND), NULL, MB_OK | MB_ICONERROR);
					LOG("CGameBoy::Init: true");
					return true;
				}
			}
		}
	}
	else
	{
		/*if (strchr(pszROMFilename, '.'))
		{
			if (!stricmp(strrchr(pszROMFilename, '.') + 1, "zip"))
			{
				zip = true;
			}
		}*/

		strcpy(Rom, pszROMFilename);
	}

	//Load ROM
	LOGFUNCTION("CreateFile");
	if ((hFile = CreateFile(Rom, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		LOGERROR;
		DisplayErrorMessage();
		LOG("CGameBoy::Init: true");
		return true;
	}
	LOGSUCCESS;

	/*if (!zip)
	{*/
		FileSize = GetFileSize(hFile, NULL);
		if (SetFilePointer(hFile, 0x148, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			CloseHandle(hFile);
			DisplayErrorMessage();
			return true;
		}
		if (!ReadFile(hFile, &MaxRomBank, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage();
			return true;
		}
		MaxRomBank = RomSize(MaxRomBank);
		if (FileSize != (((DWORD)MaxRomBank + 1) * 16384))
		{
			if (MessageBox(hMsgParent, String(IDS_STATUS_ROMSIZEDIFFER), NULL, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				return true;
			}
			if (FileSize > (((DWORD)MaxRomBank + 1) * 16384))
			{
				FileSize = ((DWORD)MaxRomBank + 1) * 16384;
			}
		}
		if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			CloseHandle(hFile);
			DisplayErrorMessage();
			return true;
		}
		if (!(MEM_ROM = new BYTE[((DWORD)MaxRomBank + 1) * 16384]))
		{
			CloseHandle(hFile);
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
		ZeroMemory(MEM_ROM, ((DWORD)MaxRomBank + 1) * 16384);
		if (!(MemStatus_ROM = new BYTE[((DWORD)MaxRomBank + 1) * 16384]))
		{
			CloseHandle(hFile);
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
		if (!ReadFile(hFile, MEM_ROM, FileSize, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DisplayErrorMessage();
			return true;
		}
	/*}
	else
	{
#define		ReadFromFile(p, size)						\
		if (!ReadFile(hFile, &p, size, &nBytes, NULL))	\
		{												\
			CloseHandle(hFile);							\
			DisplayErrorMessage();						\
			return true;								\
		}

#define		ReadFromFile2(p, size)						\
		if (!ReadFile(hFile, p, size, &nBytes, NULL))	\
		{												\
			CloseHandle(hFile);							\
			DisplayErrorMessage();						\
			return true;								\
		}

		WORD		Zip_Flags, Zip_Method;
		DWORD		Zip_FileSize;

		ReadFromFile(Value, 4);
		if (Value != 0x04034B50)
		{
			CloseHandle(hFile);
			MessageBox(hMsgParent, "Unsupported format", NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		ReadFromFile(Value, 2);
		ReadFromFile(Zip_Flags, 2);
		ReadFromFile(Zip_Method, 2);
		ReadFromFile(Value, 4);
		ReadFromFile(Value, 4);
		ReadFromFile(Zip_FileSize, 4);
		ReadFromFile(FileSize, 4);
		ReadFromFile(Value, 4);
		if (HIWORD(Value))
		{
			CloseHandle(hFile);
			MessageBox(hMsgParent, "Unsupported format", NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		if (strchr(Rom, '\\'))
		{
			*(strrchr(Rom, '\\') + 1) = '\0';
		}
		*(Rom + strlen(Rom) + LOWORD(Value)) = '\0';
		ReadFromFile2(Rom + strlen(Rom), LOWORD(Value));

		switch (Zip_Method)
		{
		case 0:
			if (!(MEM_ROM = new BYTE[FileSize]))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return true;
			}
			ZeroMemory(MEM_ROM, ((DWORD)MaxRomBank + 1) * 16384);
			if (!(MemStatus_ROM = new BYTE[FileSize]))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return true;
			}
			ReadFromFile2(MEM_ROM, FileSize);
			break;

		case 8:
			BYTE			Zip_Header;
			int				i;				//temporary variables
			DWORD			j;
			DWORD			l;				//last length
			DWORD			m;				//mask for bit lengths table
			DWORD			n;				//number of lengths to get
			struct huft		*tl;			//literal/length code table
			struct huft		*td;			//distance code table
			int				bl;				//lookup bits for tl
			int				bd;				//lookup bits for td
			DWORD			nb;				//number of bit length codes
			DWORD			nl;				//number of literal/length codes
			DWORD			nd;				//number of distance codes
			DWORD			ll[286 + 30];	//literal/length and distance code lengths
			DWORD			b;				//bit buffer

			do
			{
				ReadFromFile(Zip_Header, 1);
				switch (Zip_Header & 6)
				{
				case 0:
					//Store
					ReadFromFile(Value, 2);
					break;

				case 2:
					break;

				case 4:
					tl = NULL;
					td = NULL;

					ResetReadBits();
					if (!ReadBits(hFile, 5, &nl))
					{
						return true;
					}
					nl = (nl & 0x1F) + 257;
					if (!ReadBits(hFile, 5, &nd))
					{
						return true;
					}
					nd = (nd & 0x1F) + 1;
					if (!ReadBits(hFile, 5, &nb))
					{
						return true;
					}
					nb = (nb & 0x0F) + 4;

					if (nl > 286 || nd > 30)
					{
						CloseHandle(hFile);
						MessageBox(hMsgParent, "Error in file.", "Game Lad", MB_OK | MB_ICONERROR);
						return true;
					}

					for (j = 0; j < nb; j++)
					{
						if (!ReadBits(hFile, 3, &ll[border[j]]))
						{
							return true;
						}
						ll[border[j]] &= 7;
					}
					for (; j < 19; j++)
					{
						ll[border[j]] = 0;
					}

					bl = 7;
					if (huft_build(ll, 19, 19, NULL, NULL, &tl, &bl))
					{
						CloseHandle(hFile);
						delete tl;
						return true;
					}

					n = nl + nd;
					m = mask[bl];
					i = l = 0;
					while ((unsigned)i < n)
					{
						if (!ReadBits(hFile, bl, &b))
						{
							delete tl;
							return true;
						}
						j = (td = tl + (b & m))->b;
						j = td->v.n;
						if (j < 16)                 //length of code in bits (0..15)
						{
							ll[i++] = l = j;          //save last length in l
						}
						else if (j == 16)           //repeat last length 3 to 6 times
						{
							if (!ReadBits(hFile, 2, &b))
							{
								delete tl;
								return true;
							}
							j = 3 + (b & 3);
							if ((unsigned)i + j > n)
							{
								CloseHandle(hFile);
								delete tl;
								return true;
							}
							while (j--)
							{
								ll[i++] = l;
							}
						}
						else if (j == 17)           //3 to 10 zero length codes
						{
							ReadBits(hFile, 3, &b);
							j = 3 + (b & 7);
							if ((unsigned)i + j > n)
							{
								CloseHandle(hFile);
								delete tl;
								return true;
							}
							while (j--)
							{
								ll[i++] = 0;
							}
							l = 0;
						}
						else                        //j == 18: 11 to 138 zero length codes
						{
							ReadBits(hFile, 7, &b);
							j = 11 + (b & 0x7f);
							if ((unsigned)i + j > n)
							{
								CloseHandle(hFile);
								delete tl;
								return true;
							}
							while (j--)
							{
								ll[i++] = 0;
							}
							l = 0;
						}
					}

					delete tl;
					tl = NULL;
					td = NULL;

					//build the decoding tables for literal/length and distance codes
					bl = lbits;
					if ((i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl)) != 0)
					{
						CloseHandle(hFile);
						delete tl;
						return true;                   //incomplete code set
					}
					bd = dbits;
					if ((i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd)) != 0)
					{
						CloseHandle(hFile);
						delete td;
						delete tl;
						return true;                   //incomplete code set
					}

					if (inflate_codes(hFile, tl, td, bl, bd, NULL, 0))
					{
						CloseHandle(hFile);
						delete td;
						delete tl;
						return true;
					}

					delete tl;
					delete td;

					break;

				case 6:
					//Invalid value
					CloseHandle(hFile);
					MessageBox(hMsgParent, "Error in file.", "Game Lad", MB_OK | MB_ICONERROR);
					return true;
				}
			}
			while (!(Zip_Header & 1));

			CloseHandle(hFile);
			return true;
			break;

		default:
			CloseHandle(hFile);
			MessageBox(hMsgParent, "Unsupported format", itoa(Zip_Method, NumBuffer, 16), MB_OK | MB_ICONERROR);
			return true;
		}

		MaxRomBank = RomSize(MEM_ROM[0x148]);
		if (FileSize != (((DWORD)MaxRomBank + 1) * 16384))
		{
			if (MessageBox(hMsgParent, String(IDS_STATUS_ROMSIZEDIFFER), NULL, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				return true;
			}
			if (FileSize > (((DWORD)MaxRomBank + 1) * 16384))
			{
				FileSize = ((DWORD)MaxRomBank + 1) * 16384;
			}
		}

#undef		ReadFromFile
#undef		ReadFromFile2
	}*/

	CloseHandle(hFile);

	switch (MEM_ROM[0x0149])
	{
	case 1:
		MaxRamBank = 0;
		SaveRamSize = 2 * 1024;
		break;

	case 2:
		MaxRamBank = 0;
		SaveRamSize = 8 * 1024;
		break;

	case 3:
		MaxRamBank = 3;
		SaveRamSize = 32 * 1024;
		break;

	case 4:
		MaxRamBank = 15;
		SaveRamSize = 128 * 1024;
		break;

	default:
		MaxRamBank = 0;
		SaveRamSize = 0;
	}

	ZeroMemory(MemStatus_ROM, 16384 * (RomSize(MEM_ROM[0x148]) + 1));
	ZeroMemory(MemStatus_VRAM, sizeof(MemStatus_VRAM));
	if (SaveRamSize)
	{
		//External Battery RAM
		if (!(MemStatus_RAM = new BYTE[(MaxRamBank + 1) * 0x2000]))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
		ZeroMemory(MemStatus_RAM, (MaxRamBank + 1) * 0x2000);
	}
	ZeroMemory(MemStatus_CPU, sizeof(MemStatus_CPU));

	switch (MEM_ROM[0x0147])
	{
	case 0x03:
	case 0x06:
	case 0x09:
	case 0x0D:
	case 0x0F:
	case 0x10:
	case 0x13:
	case 0x1B:
	case 0x1E:
		break;

	default:
		SaveRamSize = 0;
	}
	switch (MEM_ROM[0x0147])
	{
	case 0x1C:
	case 0x1D:
	case 0x1E:
		Flags |= GB_HASRUMBLEPACK;
		break;
	}

	if (!hGBDC)
	{
		if (!(hGBDC = CreateCompatibleDC(NULL)))
		{
			DisplayErrorMessage();
			return true;
		}

		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = 160 + 14;
		bmi.bmiHeader.biHeight = -144;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		if (!(hGBBitmap = CreateDIBSection(hGBDC, &bmi, DIB_RGB_COLORS, (void **)&pGBBitmap, NULL, 0)))
		{
			DisplayErrorMessage();
			return true;
		}

		if (!(hOldBitmap = (HBITMAP)SelectObject(hGBDC, hGBBitmap)))
		{
			DisplayErrorMessage();
			return true;
		}
	}

	GetLocalTime(&SystemTime);
	SystemTimeToFileTime(&SystemTime, &RTC_Day0);

	if (pszStateFilename)
	{
		Reset();
		if (pszBatteryFilename)
		{
			strcpy(Battery, pszBatteryFilename);
		}
		if (LoadState(pszStateFilename, false))
		{
			LoadBattery(pszBatteryFilename);
			Reset();
		}
	}
	else
	{
		LoadBattery(pszBatteryFilename);
		Reset();
	}

	if (pDebugInfo = new CDebugInfo())
	{
		if (pDebugInfo->LoadFile(pszROMFilename))
		{
			delete pDebugInfo;
			pDebugInfo = NULL;
		}
		else
		{
			if (!pDebugInfo->nLabels())
			{
				delete pDebugInfo;
				pDebugInfo = NULL;
			}
		}
	}

	SendMessage(hClientWnd, WM_MDIGETACTIVE, 0, (LPARAM)&Maximized);

	//Create window
	if (!(hGBWnd = CreateWindowEx(WS_EX_MDICHILD, "Game Boy", Rom, Maximized ? WS_VISIBLE | WS_MAXIMIZE : WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 160 * Settings.Zoom + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * Settings.Zoom + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), hClientWnd, NULL, hInstance, NULL)))
	{
		DisplayErrorMessage();
		return true;
	}
	SetWindowLong(hGBWnd, GWL_USERDATA, (long)this);

	SetStatus(LoadString(IDS_STATUS_LOADED, szBuffer, sizeof(szBuffer), Rom), SF_MESSAGE);

	return false;
}



BOOL CGameBoy::LoadBattery(char *BatteryFilename)
{
	HANDLE		hFile;
	DWORD		nBytes, dwFileSize;
	char		szBuffer[0x200], szBuffer2[0x100];
	SYSTEMTIME	SystemTime;


	if (!SaveRamSize)
	{
		return false;
	}

	GetLocalTime(&SystemTime);
	SystemTimeToFileTime(&SystemTime, &RTC_Day0);

	ZeroMemory(&MEM_RAM, sizeof(MEM_RAM));

	if (BatteryFilename)
	{
		strcpy(Battery, BatteryFilename);
	}
	else
	{
		Battery[0] = '\0';
		Reset();
		return false;
	}

	GetBatteryFilename(Battery);

	if ((hFile = CreateFile(Battery, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
	{
		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize > SaveRamSize)
		{
			dwFileSize = SaveRamSize;
		}
		if (!ReadFile(hFile, &MEM_RAM, dwFileSize, &nBytes, NULL) || nBytes != dwFileSize)
		{
			CloseHandle(hFile);

			SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);

			ZeroMemory(&MEM_RAM, sizeof(MEM_RAM));
			BatteryAvailable = false;
			Battery[0] = 0;

			Reset();
			return true;
		}
		CloseHandle(hFile);
		if (MEM_ROM[0x147] >= 0xF && MEM_ROM[0x147] <= 0x10)
		{
			strcpy(szBuffer2, Battery);
			if (strchr(szBuffer2, '.'))
			{
				*strrchr(szBuffer2, '.') = '\0';
			}
			strcat(szBuffer2, ".clk");

			if ((hFile = CreateFile(szBuffer2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
			{
				if (!ReadFile(hFile, &RTC_Day0, sizeof(RTC_Day0), &nBytes, NULL) || nBytes != sizeof(RTC_Day0))
				{
					GetLocalTime(&SystemTime);
					SystemTimeToFileTime(&SystemTime, &RTC_Day0);
				}
				else if (!ReadFile(hFile, &RTC_DAY, sizeof(RTC_DAY), &nBytes, NULL) || nBytes != sizeof(RTC_DAY))
				{
					RTC_DAY = 0;
				}
				CloseHandle(hFile);
			}
			else
			{
				GetLocalTime(&SystemTime);
				SystemTimeToFileTime(&SystemTime, &RTC_Day0);
				RTC_DAY = 0;
			}
		}
	}
	else
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			SetStatus(LoadString(IDS_STATUS_NOTFOUND, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);
		}
		else
		{
			SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);
		}
		return true;
	}

	SetStatus(LoadString(IDS_STATUS_LOADED, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);

	Reset();
	return false;
}



BOOL CGameBoy::SaveBattery(BOOL Prompt, BOOL SaveAs)
{
	HANDLE			hFile;
	DWORD			nBytes, dw;
	BYTE			*Buffer;
	OPENFILENAME	of;
	char			szBuffer[0x200], szBuffer2[0x100];


	if (!SaveRamSize || (Prompt && !BatteryAvailable))
	{
		return false;
	}

	if (!Battery[0])
	{
		strcpy(Battery, Rom);
		if (strchr(Battery, '.'))
		{
			*strrchr(Battery, '.') = 0;
		}
		strcat(Battery, ".sav");
		if (Prompt)
		{
			switch (MessageBox(hMsgParent, String(IDS_PROMPT_SAVEBATTERY), "Game Lad", MB_YESNOCANCEL | MB_ICONQUESTION))
			{
			case IDNO:
				return false;

			case IDCANCEL:
				return true;
			}
		}
		ZeroMemory(&of, sizeof(of));
		of.lStructSize = sizeof(of);
		of.hwndOwner = hWnd;
		String(IDS_OPEN_BATTERY);
		dw = strlen(szBuffer) + 1;
		strcpy(szBuffer + dw, "*.SAV");
		dw += strlen(szBuffer + dw) + 1;
		LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
		dw += strlen(szBuffer + dw) + 1;
		strcpy(szBuffer + dw, "*.*");
		dw += strlen(szBuffer + dw) + 1;
		*(szBuffer + dw) = '\0';
		of.lpstrFilter = szBuffer;
		of.lpstrFile = Battery;
		of.nMaxFile = sizeof(Battery);
		of.lpstrTitle = LoadString(IDS_SAVEAS, szBuffer2, sizeof(szBuffer2));
		of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		of.lpstrDefExt = "sav";
		if (!GetSaveFileName(&of))
		{
			Battery[0] = 0;
			return true;
		}
		Prompt = false;
	}
	else
	{
		if (SaveAs)
		{
			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWnd;
			String(IDS_OPEN_BATTERY);
			dw = strlen(szBuffer) + 1;
			strcpy(szBuffer + dw, "*.SAV");
			dw += strlen(szBuffer + dw) + 1;
			LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
			dw += strlen(szBuffer + dw) + 1;
			strcpy(szBuffer + dw, "*.*");
			dw += strlen(szBuffer + dw) + 1;
			*(szBuffer + dw) = '\0';
			of.lpstrFilter = szBuffer;
			//of.nFilterIndex = 1;
			of.lpstrFile = Battery;
			of.nMaxFile = sizeof(Battery);
			of.lpstrTitle = LoadString(IDS_SAVEAS, szBuffer2, sizeof(szBuffer2));
			of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
			of.lpstrDefExt = "sav";
			if (!GetSaveFileName(&of))
			{
				return true;
			}
		}
	}
	if (Prompt && !SaveAs)
	{
		//Compare file with RAM
		if ((hFile = CreateFile(Battery, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
		{
			if (Buffer = new BYTE[SaveRamSize])
			{
				if (ReadFile(hFile, Buffer, SaveRamSize, &nBytes, NULL))
				{
					if (!memcmp(Buffer, &MEM_RAM, SaveRamSize))
					{
						//File same as RAM
						delete Buffer;
						CloseHandle(hFile);
						return false;
					}
				}

				delete Buffer;
			}
			CloseHandle(hFile);
		}

		switch (MessageBox(hMsgParent, String(IDS_PROMPT_SAVEBATTERY), "Game Lad", MB_YESNOCANCEL | MB_ICONQUESTION))
		{
		case IDNO:
			return false;

		case IDCANCEL:
			return true;
		}
	}

	while ((hFile = CreateFile(Battery, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), Battery), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);
			return true;
		}
	}

	while (!WriteFile(hFile, &MEM_RAM, SaveRamSize, &nBytes, NULL) || nBytes != SaveRamSize)
	{
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		SetEndOfFile(hFile);
		if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), Battery), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			CloseHandle(hFile);
			DeleteFile(Battery);
			SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);
			return true;
		}
	}

	CloseHandle(hFile);

	if (MEM_ROM[0x147] >= 0xF && MEM_ROM[0x147] <= 0x10)
	{
		strcpy(szBuffer2, Battery);
		if (strchr(szBuffer2, '.'))
		{
			*strrchr(szBuffer2, '.') = '\0';
		}
		strcat(szBuffer2, ".clk");

		while ((hFile = CreateFile(szBuffer2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
		{
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), SF_MESSAGE);
				return true;
			}
		}

		while (!WriteFile(hFile, &RTC_Day0, sizeof(RTC_Day0), &nBytes, NULL) || nBytes != sizeof(RTC_Day0))
		{
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				DeleteFile(szBuffer2);
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), SF_MESSAGE);
				return true;
			}
		}
		while (!WriteFile(hFile, &RTC_DAY, sizeof(RTC_DAY), &nBytes, NULL) || nBytes != sizeof(RTC_DAY))
		{
			SetFilePointer(hFile, sizeof(RTC_Day0), NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				DeleteFile(szBuffer2);
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szBuffer2), SF_MESSAGE);
				return true;
			}
		}

		CloseHandle(hFile);
	}

	SetStatus(LoadString(IDS_STATUS_SAVED, szBuffer, sizeof(szBuffer), Battery), SF_MESSAGE);

	return false;
}



void CGameBoy::GetBatteryFilename(char *Filename)
{
	if (Battery[0])
	{
		if (Filename != Battery)
		{
			strcpy(Filename, Battery);
		}
	}
	else
	{
		strcpy(Filename, Rom);
		if (strchr(Filename, '.'))
		{
			*strrchr(Filename, '.') = 0;
		}
		strcat(Filename, ".sav");
	}
}



HDC				hPreviewDC;
HBITMAP			hPreviewBmp, hOldPreviewBmp;
WORD			*pPreviewBmp;
BOOL			PreviewFileSelected;
HWND			hPreviewBmpWnd;
WNDPROC			OldStaticWndProc;



LPARAM CALLBACK BitmapWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	HBRUSH			hBrush;
	RECT			rct;


	switch (uMsg)
	{
	case WM_PAINT:
		if (GetUpdateRect(hWin, NULL, true))
		{
			BeginPaint(hWin, &Paint);

			hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
			rct.left = 0;
			rct.top = 0;
			rct.right = 162;
			rct.bottom = 146;
			FrameRect(Paint.hdc, &rct, hBrush);
			if (hPreviewDC && PreviewFileSelected)
			{
				BitBlt(Paint.hdc, 1, 1, 160, 144, hPreviewDC, 0, 0, SRCCOPY);
			}

			EndPaint(hWin, &Paint);
		}
		return 0;
	}

	return CallWindowProc(OldStaticWndProc, hWin, uMsg, wParam, lParam);
}



UINT CALLBACK StateOFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	BITMAPINFO		bmi;
	HANDLE			hFile;
	char			szFilename[MAX_PATH], c;
	DWORD			nBytes, y, Value;


	switch (uiMsg)
	{
	case WM_NOTIFY:
		switch (((OFNOTIFY *)lParam)->hdr.code)
		{
		/*case CDN_FILEOK:
			break;*/

		case CDN_SELCHANGE:
			PreviewFileSelected = false;
			InvalidateRect(hPreviewBmpWnd, NULL, true);
			if (!SendMessage(GetParent(hdlg), CDM_GETFILEPATH, sizeof(szFilename), (long)&szFilename))
			{
				return 0;
			}
			if ((hFile = CreateFile(szFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				return 0;
			}
			if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != sizeof(Value))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (Value != ('g' | ('l' << 8) | ('s' << 16)))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != sizeof(Value))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (Value != 0)
			{
				CloseHandle(hFile);
				return 0;
			}
			do
			{
				if (!ReadFile(hFile, &c, 1, &nBytes, NULL))
				{
					CloseHandle(hFile);
					return 0;
				}
				if (nBytes != 1)
				{
					CloseHandle(hFile);
					return 0;
				}
			}
			while (c != '\0');
			if (!ReadFile(hFile, &Value, 2, &nBytes, NULL))
			{
				CloseHandle(hFile);
				return 0;
			}
			if (nBytes != 2)
			{
				CloseHandle(hFile);
				return 0;
			}
			for (y = 0; y < 144; y++)
			{
				if (!ReadFile(hFile, pPreviewBmp + y * 160, 160 * 2, &nBytes, NULL))
				{
					CloseHandle(hFile);
					return 0;
				}
				if (nBytes != 160 * 2)
				{
					CloseHandle(hFile);
					return 0;
				}
			}
			CloseHandle(hFile);
			PreviewFileSelected = true;
			break;
		}
		break;

	case WM_INITDIALOG:
		SetWindowLong(hdlg, GWL_STYLE, GetWindowLong(hdlg, GWL_STYLE) | WS_CLIPSIBLINGS);
		MoveWindow(hdlg, 0, 0, 167, 149, true);

		PreviewFileSelected = false;

		if (!(hPreviewDC = CreateCompatibleDC(NULL)))
		{
			return 0;
		}
		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = 160;
		bmi.bmiHeader.biHeight = -144;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		if (!(hPreviewBmp = CreateDIBSection(hPreviewDC, &bmi, DIB_RGB_COLORS, (void **)&pPreviewBmp, NULL, 0)))
		{
			ReleaseDC(NULL, hPreviewDC);
			hPreviewDC = NULL;
		}
		hOldPreviewBmp = (HBITMAP)SelectObject(hPreviewDC, hPreviewBmp);

		hPreviewBmpWnd = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BLACKFRAME, 5, 0, 162, 146, hdlg, NULL, hInstance, NULL);
		OldStaticWndProc = (WNDPROC)SetWindowLong(hPreviewBmpWnd, GWL_WNDPROC, (long)BitmapWndProc);
		break;
	}

	SetWindowLong(GetParent(hdlg), DWL_MSGRESULT, 0);
	return 0;
}



char *CGameBoy::GetStateFilename(char *pszFilename)
{
	strcpy(pszFilename, Rom);
	if (strchr(pszFilename, '.'))
	{
		*strrchr(pszFilename, '.') = '\0';
	}
	pszFilename[strlen(pszFilename) + 1] = '\0';
	pszFilename[strlen(pszFilename)] = StateSlot + '0';
	strcat(pszFilename, ".gls");

	return pszFilename;
}



char *CGameBoy::GetStateFilename(char *pszFilename, DWORD dwStateSlot)
{
	strcpy(pszFilename, Rom);
	if (strchr(pszFilename, '.'))
	{
		*strrchr(pszFilename, '.') = '\0';
	}
	pszFilename[strlen(pszFilename) + 1] = '\0';
	pszFilename[strlen(pszFilename)] = (char)dwStateSlot + '0';
	strcat(pszFilename, ".gls");

	return pszFilename;
}



BOOL CGameBoy::SaveState()
{
	char		szFilename[MAX_PATH + 5];

	return SaveState(GetStateFilename(szFilename));
}



BOOL CGameBoy::SaveStateAs()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH], szBuffer[0x100], szBuffer2[0x100];
	DWORD				dw;


	Pause();

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	String(IDS_OPEN_SAVESTATE);
	dw = strlen(szBuffer) + 1;
	strcpy(szBuffer + dw, "*.GLS");
	dw += strlen(szBuffer + dw) + 1;
	LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
	dw += strlen(szBuffer + dw) + 1;
	strcpy(szBuffer + dw, "*.*");
	dw += strlen(szBuffer + dw) + 1;
	*(szBuffer + dw) = '\0';
	of.lpstrFilter = szBuffer;
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.lpstrTitle = LoadString(IDS_SAVEAS, szBuffer2, sizeof(szBuffer2));
	of.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	of.lpfnHook = StateOFNHookProc;
	of.lpstrDefExt = "gls";
	szFilename[0] = 0;
	if (!GetSaveFileName(&of))
	{
		Resume();
		return false;
	}

	return SaveState(szFilename);
}



#define		WriteToFile(source)										\
	if (!WriteFile(hFile, &source, sizeof(source), &nBytes, NULL))	\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		Resume();													\
		return true;												\
	}																\
	if (nBytes != sizeof(source))									\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		Resume();													\
		return true;												\
	}

#define		WriteToFile2(source, size)								\
	if (!WriteFile(hFile, &source, (size), &nBytes, NULL))			\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		Resume();													\
		return true;												\
	}																\
	if (nBytes != (size))											\
	{																\
		CloseHandle(hFile);											\
		DeleteFile(pszFilename);									\
		SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		Resume();													\
		return true;												\
	}

BOOL CGameBoy::SaveState(char *pszFilename)
{
	HANDLE		hFile;
	DWORD		nBytes, Value, y;
	BYTE		b = 0;
	char		szBuffer[0x200];


	Pause();

	if ((hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}

	Value = 'g' | ('l' << 8) | ('s' << 16);
	WriteToFile(Value);
	if (MEM_ROM[0x147] >= 0xF && MEM_ROM[0x147] <= 0x10)
	{
		//Includes RTC
		Value = 1;
		WriteToFile(Value);
		WriteToFile2("1.6", 4);
	}
	else
	{
		Value = 0;
		WriteToFile(Value);
		WriteToFile2("1.2", 4);
	}

	WriteToFile(MaxRomBank);
	WriteToFile(MaxRamBank);

	for (y = 0; y < 144; y++)
	{
		if (!WriteFile(hFile, pGBBitmap + 7 + y * (160 + 14), 2 * 160, &nBytes, NULL))
		{
			CloseHandle(hFile);
			DeleteFile(pszFilename);
			SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			Resume();
			return true;
		}
		if (nBytes != 2 * 160)
		{
			CloseHandle(hFile);
			DeleteFile(pszFilename);
			SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			Resume();
			return true;
		}
	}

	Value = Flags & (GB_DEBUGRUNINFO | GB_COLOR | GB_ROM_COLOR | GB_RAMENABLE | GB_HALT | GB_ENABLEIE | GB_IE | GB_HDMA);
	WriteToFile(Value);

	WriteToFile(Reg_AF);
	WriteToFile(Reg_BC);
	WriteToFile(Reg_DE);
	WriteToFile(Reg_HL);
	WriteToFile(Reg_SP);
	WriteToFile(Reg_PC);
	WriteToFile(ActiveRomBank);
	WriteToFile(ActiveRamBank);
	WriteToFile(MEM_CPU);
	WriteToFile2(*MEM_RAM, SaveRamSize);
	WriteToFile(MEM_VRAM);
	WriteToFile(BGP);
	WriteToFile(OBP);

	if (Flags & GB_DEBUGRUNINFO)
	{
		WriteToFile2(*MemStatus_ROM, 16384 * (DWORD)RomSize(MEM_ROM[0x148]));
		WriteToFile(MemStatus_CPU);
		WriteToFile2(*MemStatus_RAM, SaveRamSize);
		WriteToFile(MemStatus_VRAM);
	}

	WriteToFile(LCD_Ticks);
	WriteToFile(DIV_Ticks);
	WriteToFile(TIMA_Ticks);
	WriteToFile(Hz);
	WriteToFile(b);
	WriteToFile(WindowY2);
	WriteToFile(b);
	WriteToFile(SoundTicks);
	WriteToFile(Sound1Enabled);
	WriteToFile(Sound2Enabled);
	WriteToFile(Sound3Enabled);
	WriteToFile(Sound4Enabled);
	WriteToFile(Sound1Stage);
	WriteToFile(Sound2Stage);
	WriteToFile(Sound3Stage);
	WriteToFile(Sound4Bit);
	WriteToFile(Sound1Volume);
	WriteToFile(Sound2Volume);
	WriteToFile(Sound4Volume);
	WriteToFile(Sound1Ticks);
	WriteToFile(Sound1TimeOut);
	WriteToFile(Sound1Frequency);
	WriteToFile(Sound1Envelope);
	WriteToFile(Sound1Sweep);
	WriteToFile(Sound2Ticks);
	WriteToFile(Sound2TimeOut);
	WriteToFile(Sound2Frequency);
	WriteToFile(Sound2Envelope);
	WriteToFile(Sound3Ticks);
	WriteToFile(Sound3TimeOut);
	WriteToFile(Sound3Frequency);
	WriteToFile(Sound4Ticks);
	WriteToFile(Sound4TimeOut);
	WriteToFile(Sound4Frequency);
	WriteToFile(Sound4Envelope);
	if (MEM_ROM[0x147] >= 0xF && MEM_ROM[0x147] <= 0x10)
	{
		WriteToFile(RTC_Day0);
		WriteToFile(RTC_DAYH);
		WriteToFile(RTC_Reg);
	}

	CloseHandle(hFile);

	SetStatus(LoadString(IDS_STATUS_SAVED, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);

	Resume();

	return false;
}



BOOL CGameBoy::LoadStateAs()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH], szBuffer[0x100], szBuffer2[0x100];
	DWORD				dw;


	Pause();

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	String(IDS_OPEN_SAVESTATE);
	dw = strlen(szBuffer) + 1;
	strcpy(szBuffer + dw, "*.GLS");
	dw += strlen(szBuffer + dw) + 1;
	LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
	dw += strlen(szBuffer + dw) + 1;
	strcpy(szBuffer + dw, "*.*");
	dw += strlen(szBuffer + dw) + 1;
	*(szBuffer + dw) = '\0';
	of.lpstrFilter = szBuffer;
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.lpstrTitle = LoadString(IDS_OPEN, szBuffer2, sizeof(szBuffer2));
	of.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	of.lpfnHook = StateOFNHookProc;
	szFilename[0] = 0;
	if (!GetOpenFileName(&of))
	{
		Resume();
		return false;
	}

	return LoadState(szFilename, false);
}



BOOL CGameBoy::LoadState()
{
	char		szFilename[MAX_PATH + 5];

	return LoadState(GetStateFilename(szFilename), true);
}



#define		ReadFromFile(dest)									\
	if (!ReadFile(hFile, &dest, sizeof(dest), &nBytes, NULL))	\
	{															\
		CloseHandle(hFile);										\
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		ReApplyCheats();										\
		Resume();												\
		return true;											\
	}															\
	if (nBytes != sizeof(dest))									\
	{															\
		CloseHandle(hFile);										\
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		ReApplyCheats();										\
		Resume();												\
		return true;											\
	}

#define		ReadFromFile2(dest, size)							\
	if (!ReadFile(hFile, &dest, (size), &nBytes, NULL))			\
	{															\
		CloseHandle(hFile);										\
		Stop();													\
		Reset();												\
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		ReApplyCheats();										\
		return true;											\
	}															\
	if (nBytes != (size))										\
	{															\
		CloseHandle(hFile);										\
		Stop();													\
		Reset();												\
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);\
		ReApplyCheats();										\
		return true;											\
	}

BOOL CGameBoy::LoadState(char *pszFilename, BOOL QuickLoad)
{
	HANDLE		hFile;
	DWORD		nBytes, Value, y, Pos, pByte, RamSize;
	char		szBuffer[0x200], szPath[MAX_PATH + 2];
	int			Slot1, Slot2;
	FILETIME	FileTime1, FileTime2;
	SYSTEMTIME	SystemTime;
	BYTE		b;


	Pause();


	//Load latest modified saved state
	if (pszFilename[0] == '\0')
	{
		strcpy(szPath, Rom);
		if (strchr(szPath, '.'))
		{
			*strrchr(szPath, '.') = '\0';
		}
		strcat(szPath, "0.gls");
		Slot1 = -1;
		for (Slot2 = 0; Slot2 <= 9; Slot2++)
		{
			szPath[strlen(szPath) - 5] = Slot2 + '0';
			if ((hFile = CreateFile(szPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
			{
				GetFileTime(hFile, NULL, NULL, &FileTime2);
				if (Slot1 >= 0)
				{
					if (FileTime2.dwHighDateTime > FileTime1.dwHighDateTime || (FileTime2.dwHighDateTime == FileTime1.dwHighDateTime && FileTime2.dwLowDateTime > FileTime1.dwLowDateTime))
					{
						Slot1 = Slot2;
						FileTime1.dwHighDateTime = FileTime2.dwHighDateTime;
						FileTime1.dwLowDateTime = FileTime2.dwLowDateTime;
					}
				}
				else
				{
					Slot1 = Slot2;
					FileTime1.dwHighDateTime = FileTime2.dwHighDateTime;
					FileTime1.dwLowDateTime = FileTime2.dwLowDateTime;
				}
				CloseHandle(hFile);
			}
		}
		if (Slot1 >= 0)
		{
			szPath[strlen(szPath) - 5] = Slot1 + '0';
			pszFilename = szPath;
			StateSlot = Slot1;
		}
		else
		{
			return true;
		}
	}


	if ((hFile = CreateFile(pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			SetStatus(LoadString(IDS_STATUS_NOTFOUND, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			Resume();
			return true;
		}
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}

	if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	if (nBytes != sizeof(Value))
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	if (Value != ('g' | ('l' << 8) | ('s' << 16)))
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	if (!ReadFile(hFile, &Value, sizeof(Value), &nBytes, NULL))
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	if (nBytes != sizeof(Value))
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	strcpy(szPath, "Game Lad ");
	Pos = strlen(szPath);
	do
	{
		if (Pos < sizeof(szPath) - 12)
		{
			if (!ReadFile(hFile, &szPath[Pos++], sizeof(szPath[Pos]), &nBytes, NULL))
			{
				CloseHandle(hFile);
				SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
				Resume();
				return true;
			}
			if (nBytes != sizeof(szPath[Pos]))
			{
				CloseHandle(hFile);
				SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
				Resume();
				return true;
			}
			szPath[Pos] = '\0';
		}
		else
		{
			CloseHandle(hFile);
			SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			Resume();
			return true;
		}
	}
	while (szPath[Pos - 1] != '\0');
	if (Value > GAME_LAD_SAVE_STATE_VERSION)
	{
		CloseHandle(hFile);
		LoadString(IDS_HIGHERVERSIONREQUIRED, szBuffer, sizeof(szBuffer), szPath);
		if (QuickLoad)
		{
			SetStatus(szBuffer, SF_MESSAGE);
		}
		else
		{
			MessageBox(hMsgParent, szBuffer, "Game Lad", MB_OK | MB_ICONWARNING);
		}
		Resume();
		return true;
	}

	ReadFromFile2(Value, 1);
	if (MaxRomBank != (BYTE)Value)
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}
	ReadFromFile2(Value, 1);
	if (MaxRamBank != (BYTE)Value)
	{
		CloseHandle(hFile);
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		Resume();
		return true;
	}

	for (y = 0; y < 144; y++)
	{
		if (!ReadFile(hFile, pGBBitmap + 7 + y * (160 + 14), 2 * 160, &nBytes, NULL))
		{
			CloseHandle(hFile);
			Stop();
			Reset();
			SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			return true;
		}
		if (nBytes != 2 * 160)
		{
			CloseHandle(hFile);
			Stop();
			Reset();
			SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
			return true;
		}
	}

	ReadFromFile(Flags);

	ReadFromFile(Reg_AF);
	ReadFromFile(Reg_BC);
	ReadFromFile(Reg_DE);
	ReadFromFile(Reg_HL);
	ReadFromFile(Reg_SP);
	ReadFromFile(Reg_PC);
	ReadFromFile(ActiveRomBank);
	ReadFromFile(ActiveRamBank);
	ReadFromFile(MEM_CPU);
	ReadFromFile2(*MEM_RAM, SaveRamSize);
	ReadFromFile(MEM_VRAM);
	ReadFromFile(BGP);
	ReadFromFile(OBP);

	if (Flags & GB_DEBUGRUNINFO)
	{
		ReadFromFile2(*MemStatus_ROM, 16384 * (DWORD)RomSize(MEM_ROM[0x148]));
		ReadFromFile(MemStatus_CPU);
		ReadFromFile2(*MemStatus_RAM, SaveRamSize);
		ReadFromFile(MemStatus_VRAM);
	}
	else
	{
		ClearDebugRunInfo();
	}

	ReadFromFile(LCD_Ticks);
	ReadFromFile(DIV_Ticks);
	ReadFromFile(TIMA_Ticks);
	ReadFromFile(Hz);
	ReadFromFile(b);
	ReadFromFile(WindowY2);
	ReadFromFile(b);
	ReadFromFile(SoundTicks);
	ReadFromFile(Sound1Enabled);
	ReadFromFile(Sound2Enabled);
	ReadFromFile(Sound3Enabled);
	ReadFromFile(Sound4Enabled);
	ReadFromFile(Sound1Stage);
	ReadFromFile(Sound2Stage);
	ReadFromFile(Sound3Stage);
	ReadFromFile(Sound4Bit);
	ReadFromFile(Sound1Volume);
	ReadFromFile(Sound2Volume);
	ReadFromFile(Sound4Volume);
	ReadFromFile(Sound1Ticks);
	ReadFromFile(Sound1TimeOut);
	ReadFromFile(Sound1Frequency);
	ReadFromFile(Sound1Envelope);
	ReadFromFile(Sound1Sweep);
	ReadFromFile(Sound2Ticks);
	ReadFromFile(Sound2TimeOut);
	ReadFromFile(Sound2Frequency);
	ReadFromFile(Sound2Envelope);
	ReadFromFile(Sound3Ticks);
	ReadFromFile(Sound3TimeOut);
	ReadFromFile(Sound3Frequency);
	ReadFromFile(Sound4Ticks);
	ReadFromFile(Sound4TimeOut);
	ReadFromFile(Sound4Frequency);
	ReadFromFile(Sound4Envelope);

	if (MEM_ROM[0x147] >= 0xF && MEM_ROM[0x147] <= 0x10)
	{
		if (!ReadFile(hFile, &RTC_Day0, sizeof(RTC_Day0), &nBytes, NULL) || nBytes != sizeof(RTC_Day0))
		{
			GetLocalTime(&SystemTime);
			SystemTimeToFileTime(&SystemTime, &RTC_Day0);
		}
		if (!ReadFile(hFile, &RTC_DAYH, sizeof(RTC_DAYH), &nBytes, NULL) || nBytes != sizeof(RTC_DAYH))
		{
			RTC_DAYH = 0;
		}
		if (!ReadFile(hFile, &RTC_Reg, sizeof(RTC_Reg), &nBytes, NULL) || nBytes != sizeof(RTC_Reg))
		{
			RTC_Reg = 0x80;
		}
	}

	CloseHandle(hFile);

	InvalidateRect(hGBWnd, NULL, false);

	SetStatus(LoadString(IDS_STATUS_LOADED, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);

	BatteryAvailable = true;


	MEM[0x0] = &MEM_ROM[0x000000];
	MEM[0x1] = &MEM_ROM[0x001000];
	MEM[0x2] = &MEM_ROM[0x002000];
	MEM[0x3] = &MEM_ROM[0x003000];
	MEM[0x4] = &MEM_ROM[0x000000 + ActiveRomBank * 0x4000];
	MEM[0x5] = &MEM_ROM[0x001000 + ActiveRomBank * 0x4000];
	MEM[0x6] = &MEM_ROM[0x002000 + ActiveRomBank * 0x4000];
	MEM[0x7] = &MEM_ROM[0x003000 + ActiveRomBank * 0x4000];
	MEM[0x8] = &MEM_VRAM[0x0000 + (FF00_C(0x4F) & 1) * 0x2000];
	MEM[0x9] = &MEM_VRAM[0x1000 + (FF00_C(0x4F) & 1) * 0x2000];
	MEM[0xA] = &MEM_RAM[0x0000 + ActiveRamBank * 0x2000];
	MEM[0xB] = &MEM_RAM[0x1000 + ActiveRamBank * 0x2000];
	MEM[0xC] = &MEM_CPU[0x0000];
	if (FF00_C(0x70) & 7)
	{
		MEM[0xD] = &MEM_CPU[0x0000 + (FF00_C(0x70) & 7) * 0x1000];
	}
	else
	{
		MEM[0xD] = &MEM_CPU[0x1000];
	}
	MEM[0xE] = &MEM_CPU[0x0000];
	MEM[0xF] = &MEM_CPU[0x8000];


	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		break;
	case 2:
		RamSize = 0x2000;
		break;
	case 3:
		RamSize = 0x8000;
		break;
	case 4:
		RamSize = 0x20000;
		break;
	default:
		RamSize = 0;
		break;
	}
	for (pByte = 0; pByte < RamSize; pByte++)
	{
		MemStatus_RAM[pByte] &= ~MEM_FIXED;
	}
	for (pByte = 0; pByte < 0x9000; pByte++)
	{
		MemStatus_CPU[pByte] &= ~MEM_FIXED;
	}

	switch (MEM_ROM[0x0147])
	{
	case 0x1C:
	case 0x1D:
	case 0x1E:
		Flags |= GB_HASRUMBLEPACK;
		break;
	}


	ReApplyCheats();

	Resume();

	return false;
}



void CGameBoy::SetStateSlot(int nSlot)
{
	char		szBuffer[0x100], szSlot[2];


	if (nSlot >= 0)
	{
		if (nSlot <= 9)
		{
			StateSlot = nSlot;
		}
		else
		{
			StateSlot = 9;
		}
	}
	else
	{
		StateSlot = 0;
	}

	szSlot[0] = StateSlot + '0';
	szSlot[1] = '\0';
	SetStatus(LoadString(IDS_STATUS_SELECTSTATESLOT, szBuffer, sizeof(szBuffer), szSlot), SF_MESSAGE);
}



int CGameBoy::GetStateSlot()
{
	return StateSlot;
}



LRESULT CGameBoy::GameBoyWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT		Paint;
	RECT			rct;
	POINT			Pt;


	switch (uMsg)
	{
	case WM_PAINT:
		if (GetUpdateRect(hGBWnd, NULL, true))
		{
			BeginPaint(hGBWnd, &Paint);
			if (!m_pd3dd)
			{
				GetClientRect(hGBWnd, &rct);
				if (rct.right == 160 && rct.bottom == 144)
				{
					//StretchBlt(Paint.hdc, 0, 0, rct.right, rct.bottom, hGBDC, 7, 0, 160, 144, SRCCOPY);
					BitBlt(Paint.hdc, Paint.rcPaint.left, Paint.rcPaint.top, Paint.rcPaint.right, Paint.rcPaint.bottom, hGBDC, Paint.rcPaint.left + 7, Paint.rcPaint.top, SRCCOPY);
				}
				else
				{
					StretchBlt(Paint.hdc, 0, 0, rct.right, rct.bottom, hGBDC, 7, 0, 160, 144, SRCCOPY);
				}
			}
			EndPaint(hGBWnd, &Paint);
		}
		return 0;

	case WM_ERASEBKGND:
		//Picture covers whole window, no need to erase background.
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_VIEW_ZOOM_100:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_200:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 2 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 2 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_300:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 3 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 3 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;

		case ID_VIEW_ZOOM_400:
			SendMessage(hClientWnd, WM_MDIRESTORE, (WPARAM)hGBWnd, 0);
			GetWindowRect(hGBWnd, &rct);
			Pt.x = rct.left;
			Pt.y = rct.top;
			ScreenToClient(hWnd, &Pt);
			MoveWindow(hGBWnd, Pt.x - GetSystemMetrics(SM_CXEDGE), Pt.y - GetSystemMetrics(SM_CYEDGE), 160 * 4 + 2 * GetSystemMetrics(SM_CXSIZEFRAME), 144 * 4 + 2 * GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION), true);
			return 0;
		}
		break;
	}

	return DefMDIChildProc(hGBWnd, uMsg, wParam, lParam);
}



void CGameBoy::Reset(DWORD Flags)
{
	this->Flags = (Flags & GB_COLOR) | (this->Flags & (GB_DEBUG | GB_HASRUMBLEPACK));
	Reset();
}



void CGameBoy::Reset()
{
	DWORD		RamSize, pByte;


	ZeroMemory(&MEM_CPU, sizeof(MEM_CPU));
	ZeroMemory(&MEM_VRAM, sizeof(MEM_VRAM));
	ZeroMemory(&BGP, sizeof(BGP));
	ZeroMemory(&OBP, sizeof(OBP));

	*(WORD *)&BGP[0] = GreyScales[0];
	*(WORD *)&BGP[2] = GreyScales[1];
	*(WORD *)&BGP[4] = GreyScales[2];
	*(WORD *)&BGP[6] = GreyScales[3];
	*(WORD *)&OBP[0] = GreyScales[0];
	*(WORD *)&OBP[2] = GreyScales[1];
	*(WORD *)&OBP[4] = GreyScales[2];
	*(WORD *)&OBP[6] = GreyScales[3];

	Flags &= GB_COLOR | GB_DEBUG;
	switch (MEM_ROM[0x0147])
	{
	case 0x1C:
	case 0x1D:
	case 0x1E:
		Flags |= GB_HASRUMBLEPACK;
		break;
	}
	if (Flags & GB_COLOR && MEM_ROM[0x0143] & 0x80)
	{
		Reg_AF = 0x11B0;
		Flags |= GB_ROM_COLOR;
	}
	else
	{
		Reg_AF = 0x01B0;
	}
	Reg_BC = 0x0013;
	Reg_DE = 0x00D8;
	Reg_HL = 0x014D;
	Reg_SP = 0xFFFE;
	Reg_PC = 0x0100;

	ActiveRomBank = 1;
	ActiveRamBank = 0;

	RTC_Reg = 0x80;

	//DrawLineMask = 0;

	DIV_Ticks = 0;
	Hz = 2048;

	FF00_C(0x40) = 0x91;
	FF00_C(0x41) = 0x80;
	LCD_Ticks = 102;
	FF00_C(0x55) = 0xFF;

	MEM[0x0] = &MEM_ROM[0x000000];
	MEM[0x1] = &MEM_ROM[0x001000];
	MEM[0x2] = &MEM_ROM[0x002000];
	MEM[0x3] = &MEM_ROM[0x003000];
	MEM[0x4] = &MEM_ROM[0x004000];
	MEM[0x5] = &MEM_ROM[0x005000];
	MEM[0x6] = &MEM_ROM[0x006000];
	MEM[0x7] = &MEM_ROM[0x007000];
	MEM[0x8] = &MEM_VRAM[0x0000];
	MEM[0x9] = &MEM_VRAM[0x1000];
	MEM[0xA] = &MEM_RAM[0x0000];
	MEM[0xB] = &MEM_RAM[0x1000];
	MEM[0xC] = &MEM_CPU[0x0000];
	MEM[0xD] = &MEM_CPU[0x1000];
	MEM[0xE] = &MEM_CPU[0x0000];
	MEM[0xF] = &MEM_CPU[0x8000];

	FillMemory(pGBBitmap, (160 + 14) * 144 * 2, 0xFF);


	MemStatus[0x0] = pMemStatus_ROM(0x000000);
	MemStatus[0x1] = pMemStatus_ROM(0x001000);
	MemStatus[0x2] = pMemStatus_ROM(0x002000);
	MemStatus[0x3] = pMemStatus_ROM(0x003000);
	MemStatus[0x4] = pMemStatus_ROM(0x004000);
	MemStatus[0x5] = pMemStatus_ROM(0x005000);
	MemStatus[0x6] = pMemStatus_ROM(0x006000);
	MemStatus[0x7] = pMemStatus_ROM(0x007000);
	MemStatus[0x8] = &MemStatus_VRAM[0x0000];
	MemStatus[0x9] = &MemStatus_VRAM[0x1000];
	/*if (MemStatus_RAM)
	{
		MemStatus[0xA] = MemStatus_RAM;
		if (MEM_ROM[0x0149] != 1)
		{
			MemStatus[0xB] = MemStatus_RAM + 0x1000;
		}
		else
		{
			MemStatus[0xB] = ZeroStatus;
		}
	}
	else*/
	{
		MemStatus[0xA] = ZeroStatus;
		MemStatus[0xB] = ZeroStatus;
	}
	MemStatus[0xC] = &MemStatus_CPU[0x0000];
	MemStatus[0xD] = &MemStatus_CPU[0x1000];
	MemStatus[0xE] = ZeroStatus;
	MemStatus[0xF] = &MemStatus_CPU[0x8000];

	//FillMemory(MemStatus_ROM, 16384 * RomSize(MEM_ROM[0x148]), MEM_WRITE | MEM_READ | MEM_EXECUTE);
	for (pByte = 0; pByte < (unsigned)16384 * (RomSize(MEM_ROM[0x148]) + 1); pByte++)
	{
		MemStatus_ROM[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
	}

	//VRAM
	//FillMemory(MemStatus_VRAM, sizeof(MemStatus_VRAM), MEM_WRITE | MEM_READ);
	for (pByte = 0; pByte < sizeof(MemStatus_VRAM); pByte++)
	{
		MemStatus_VRAM[pByte] |= MEM_WRITE | MEM_READ;
		MemStatus_VRAM[pByte] &= ~MEM_CHANGED;
	}

	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		ZeroMemory(&MemStatus_RAM[0x800], 0x1000 - 0x800);
		break;

	case 2:
		RamSize = 0x2000;
		break;

	case 3:
		RamSize = 0x8000;
		break;

	case 4:
		RamSize = 0x20000;
		break;

	default:
		//No external RAM
		RamSize = 0;
		break;
	}

	if (SaveRamSize)
	{
		//External Battery RAM
		//FillMemory(MemStatus_RAM, RamSize, MEM_WRITE | MEM_READ | MEM_EXECUTE);
		for (pByte = 0; pByte < RamSize; pByte++)
		{
			MemStatus_RAM[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
			MemStatus_RAM[pByte] &= ~MEM_CHANGED;
		}
	}
	else
	{
		//External RAM
		//FillMemory(MemStatus_RAM, RamSize, MEM_WRITE | MEM_EXECUTE);
		for (pByte = 0; pByte < RamSize; pByte++)
		{
			MemStatus_RAM[pByte] |= MEM_WRITE | MEM_EXECUTE;
			MemStatus_RAM[pByte] &= ~(MEM_CHANGED | MEM_READ);
		}
	}

	//Internal RAM
	//FillMemory(MemStatus_CPU, 0x8000, MEM_WRITE | MEM_EXECUTE);
	for (pByte = 0; pByte < 0x8000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~(MEM_CHANGED | MEM_READ);
	}
	//reserved
	ZeroMemory(&MemStatus_CPU[0x8000], 0x0E00);
	//Sprite RAM
	//FillMemory(&MemStatus_CPU[0x8E00], 0xA0, MEM_WRITE | MEM_READ);
	for (pByte = 0x8E00; pByte < 0x8EA0; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_READ | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}
	//Reserved
	ZeroMemory(&MemStatus_CPU[0x8EA0], 0x60);
	//IO ports
	//FillMemory(&MemStatus_CPU[0x8F00], 0x80, MEM_WRITE | MEM_READ);
	for (pByte = 0x8F00; pByte < 0x8F80; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_READ;
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}
	//HiRAM
	//FillMemory(&MemStatus_CPU[0x8F80], 0x80, MEM_WRITE | MEM_EXECUTE);
	for (pByte = 0x8F80; pByte < 0x9000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_WRITE | MEM_EXECUTE;
		MemStatus_CPU[pByte] &= ~(MEM_CHANGED | MEM_READ);
	}
	//Interrupt
	MemStatus_CPU[0x8FFF] = MEM_WRITE | MEM_READ;

	Flags |= GB_DEBUGRUNINFO;

	RefreshScreen();

	ReApplyCheats();

	SendMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
}



void CGameBoy::RemoveCheats()
{
	CHEATDATA		*pCheatData;


	if (!m_pCheatList)
	{
		return;
	}

	m_pCheatList->ResetSearch();
	while (pCheatData = (CHEATDATA *)m_pCheatList->GetNextItem())
	{
		if (pCheatData->Offset < 0xA000)
		{
			MEM_ROM[(pCheatData->Offset & 0x3FFF) + pCheatData->Bank * 0x4000] = pCheatData->OldValue;
		}
		else
		{
			if (pCheatData->Offset < 0xC000)
			{
				MemStatus_RAM[pCheatData->Offset - 0xA000 + pCheatData->Bank * 0x2000] &= ~MEM_FIXED;
			}
			else
			{
				MemStatus_CPU[pCheatData->Offset - 0xC000] &= ~MEM_FIXED;
			}
		}
	}

	delete m_pCheatList;
	m_pCheatList = NULL;
}



BYTE CGameBoy::GetRealByte(WORD Offset, BYTE Bank)
{
	CHEATDATA		*pCheatData;


	if (!m_pCheatList)
	{
		return MEM_ROM[(Offset & 0x3FFF) + 0x4000 * Bank];
	}

	m_pCheatList->ResetSearch();
	while (pCheatData = (CHEATDATA *)m_pCheatList->GetNextItem())
	{
		if (pCheatData->Offset == Offset && pCheatData->Bank == Bank)
		{
			return pCheatData->OldValue;
		}
	}

	return MEM_ROM[(Offset & 0x3FFF) + 0x4000 * Bank];
}



int CGameBoy::VerifyCode(char *pszCode, BOOL CompareValue)
{
	int			Pos;
	DWORD		Offset;
	BYTE		Value;


	if (pszCode[5] <= 7)
	{
		return CODE_INVALID;
	}

	if (CompareValue)
	{
		if ((pszCode[6] ^ pszCode[7]) >= 1 && (pszCode[6] ^ pszCode[7]) <= 7)
		{
			return CODE_INVALID;
		}
		Value = ~((pszCode[6] << 2) | (pszCode[8] >> 2) | (pszCode[8] << 6)) ^ 0x45;
		if (Value == ((pszCode[0] << 4) | pszCode[1]))
		{
			return CODE_INEFFECTIVE;
		}
	}

	if (!this)
	{
		return CODE_GENIE;
	}

	Offset = (~pszCode[5] << 12) | (pszCode[2] << 8) | (pszCode[3] << 4) | pszCode[4];
	if (CompareValue)
	{
		if (Offset < 0x4000 || MaxRomBank == 0)
		{
			if (GetRealByte((WORD)Offset, 0) == ((pszCode[0] << 4) | pszCode[1]) || GetRealByte((WORD)Offset, 0) != Value)
			{
				return CODE_INEFFECTIVE;
			}
		}
		else
		{
			for (Pos = MaxRomBank; Pos >= 1; Pos--)
			{
				if (GetRealByte((WORD)Offset, (BYTE)Pos) == Value)
				{
					return CODE_GENIE;
				}
			}

			return CODE_INEFFECTIVE;
		}
	}
	else
	{
		if (Offset < 0x4000 || MaxRomBank == 0)
		{
			if (GetRealByte((WORD)Offset, 0) == ((pszCode[0] << 4) | pszCode[1]))
			{
				return CODE_INEFFECTIVE;
			}
		}
		else
		{
			for (Pos = MaxRomBank; Pos >= 1; Pos--)
			{
				if (GetRealByte((WORD)Offset, (BYTE)Pos) != ((pszCode[0] << 4) | pszCode[1]))
				{
					return CODE_GENIE;
				}
			}

			return CODE_INEFFECTIVE;
		}
	}

	return CODE_GENIE;
}



BOOL CGameBoy::AddCheat(WORD Offset, BYTE Value, char *pszCode)
{
	CHEATDATA		CheatData;


	if (!m_pCheatList)
	{
		if (!(m_pCheatList = new CList()))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
	}

	CheatData.Offset = Offset;
	CheatData.Bank = 1;
	CheatData.pszCode = pszCode;
	if (Offset >= 0x4000)
	{
		do
		{
			CheatData.OldValue = MEM_ROM[(Offset & 0x3FFF) + CheatData.Bank * 0x4000];
			if (!m_pCheatList->NewItem(sizeof(CheatData), &CheatData))
			{
				return true;
			}
			MEM_ROM[(Offset & 0x3FFF) + CheatData.Bank * 0x4000] = Value;
		}
		while (++CheatData.Bank < MaxRomBank);
	}
	else
	{
		CheatData.OldValue = MEM_ROM[Offset];
		if (!m_pCheatList->NewItem(sizeof(CheatData), &CheatData))
		{
			return true;
		}
		MEM_ROM[Offset] = Value;
	}

	return false;
}



BOOL CGameBoy::AddCheat(WORD Offset, BYTE Value, BYTE CompareValue, char *pszCode)
{
	CHEATDATA		CheatData;


	if (!m_pCheatList)
	{
		if (!(m_pCheatList = new CList()))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
	}

	CheatData.Offset = Offset;
	CheatData.Bank = 1;
	CheatData.pszCode = pszCode;
	if (Offset >= 0x4000)
	{
		do
		{
			if (CompareValue == MEM_ROM[(Offset & 0x3FFF) + CheatData.Bank * 0x4000])
			{
				CheatData.OldValue = MEM_ROM[Offset + CheatData.Bank * 0x4000];
				if (!m_pCheatList->NewItem(sizeof(CheatData), &CheatData))
				{
					return true;
				}
				MEM_ROM[(Offset & 0x3FFF) + CheatData.Bank * 0x4000] = Value;
			}
		}
		while (++CheatData.Bank < MaxRomBank);
	}
	else
	{
		CheatData.OldValue = MEM_ROM[Offset];
		if (!m_pCheatList->NewItem(sizeof(CheatData), &CheatData))
		{
			return true;
		}
		MEM_ROM[Offset] = Value;
	}

	return false;
}



BOOL CGameBoy::AddCheat(BYTE Bank, WORD Offset, BYTE Value, char *pszCode)
{
	CHEATDATA		CheatData;


	if (!m_pCheatList)
	{
		if (!(m_pCheatList = new CList()))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
	}

	CheatData.Offset = Offset;
	CheatData.Bank = Bank;
	CheatData.Value = Value;
	CheatData.pszCode = pszCode;
	if (!m_pCheatList->NewItem(sizeof(CheatData), &CheatData))
	{
		return true;
	}

	if (Offset < 0xC000)
	{
		if (Bank > MaxRamBank)
		{
			return false;
		}
		if (MEM_ROM[0x149] == 1 && Offset >= 0xA800)
		{
			return false;
		}

		MEM_RAM[Offset - 0xA000 + Bank * 0x2000] = Value;
		MemStatus_RAM[Offset - 0xA000 + Bank * 0x2000] |= MEM_FIXED;
	}
	else
	{
		MEM_CPU[Offset - 0xC000] = Value;
		MemStatus_CPU[Offset - 0xC000] |= MEM_FIXED;
	}

	return false;
}



BOOL CGameBoy::AddCheat(char *pszCode)
{
	char		szCode[10];
	int			Pos, Pos2;
	WORD		Offset;
	BYTE		Value, Bank;


	for (Pos = Pos2 = 0; pszCode[Pos]; Pos++)
	{
		if (pszCode[Pos] != '-')
		{
			szCode[Pos2] = pszCode[Pos];
			if (HexToNum(&szCode[Pos2]))
			{
				return true;
			}
			Pos2++;
		}
	}

	switch (Pos2)
	{
	case 6:
	case 9:
		Offset = (~szCode[5] << 12) | (szCode[2] << 8) | (szCode[3] << 4) | szCode[4];
		Value = (szCode[0] << 4) | szCode[1];
		if (Pos2 == 9)
		{
			return AddCheat(Offset, Value, ~((szCode[6] << 2) | (szCode[8] >> 2) | (szCode[8] << 6)) ^ 0x45, pszCode);
		}
		return AddCheat(Offset, Value, pszCode);

	case 8:
		Offset = (szCode[6] << 12) | (szCode[7] << 8) | (szCode[4] << 4) | szCode[5];
		Value = (szCode[2] << 4) | szCode[3];
		if (Offset < 0xC000)
		{
			Bank = ((szCode[0] << 4) | szCode[1]) & 0x7F;
			return AddCheat(Bank, Offset, Value, pszCode);
		}
		return AddCheat(0, Offset, Value, pszCode);
	}

	return true;
}



void CGameBoy::ReApplyCheats()
{
	CHEATDATA		*pCheatData;


	if (!m_pCheatList)
	{
		return;
	}

	m_pCheatList->ResetSearch();
	while (pCheatData = (CHEATDATA *)m_pCheatList->GetNextItem())
	{
		if (pCheatData->Offset >= 0xA000)
		{
			if (pCheatData->Offset < 0xC000)
			{
				MEM_RAM[pCheatData->Offset - 0xA000 + pCheatData->Bank * 0x2000] = pCheatData->Value;
				MemStatus_RAM[pCheatData->Offset - 0xA000 + pCheatData->Bank * 0x2000] |= MEM_FIXED;
			}
			else
			{
				MEM_CPU[pCheatData->Offset - 0xC000] = pCheatData->Value;
				MemStatus_CPU[pCheatData->Offset - 0xC000] |= MEM_FIXED;
			}
		}
	}
}



BOOL CGameBoy::IsApplied(char *pszCode)
{
	CHEATDATA		*pCheatData;


	if (!m_pCheatList)
	{
		return false;
	}

	m_pCheatList->ResetSearch();
	while (pCheatData = (CHEATDATA *)m_pCheatList->GetNextItem())
	{
		if (pCheatData->pszCode == pszCode)
		{
			return true;
		}
	}

	return false;
}



	/*if (!pSearchCheatBytes)
	{
		if (!(pSearchCheatBytes = new BYTE[0x2000]))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return;
		}
	}
	if (!pSearchCheatBytesLeft)
	{
		if (!(pSearchCheatBytesLeft = new BYTE[0x2000 / 8]))
		{
			delete pSearchCheatBytes;
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return;
		}
	}*/


void CGameBoy::PerformSearch(UINT uID)
{
	char			szBuffer[0x100];
	LVITEM			lvi;
	DWORD			dwPos, dwPos2, dwValue, dwValue2;
	BOOL			FillList, Keep;


	Pause();

	if (!ListView_GetItemCount(m_hList))
	{
		FillList = true;
		if (SendMessage(m_hRelLast, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			uID = NULL;
		}
		m_SearchSize = (BYTE)SendMessage(m_hSize, CB_GETCURSEL, 0, 0);
	}
	else
	{
		FillList = false;
	}

	if (uID)
	{
		if (SendMessage(m_hRelValue, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			GetWindowText(m_hValue, szBuffer, sizeof(szBuffer));
			if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				dwValue = 0;
				for (dwPos = 0; szBuffer[dwPos]; dwPos++)
				{
					if (HexToNum(&szBuffer[dwPos]))
					{
						MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
						Resume();
						return;
					}
					dwValue = (dwValue << 4) | szBuffer[dwPos];
				}
				if (dwPos == 0)
				{
					MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
					Resume();
					return;
				}
			}
			else
			{
				for (dwPos2 = 0; szBuffer[dwPos2]; dwPos2++)
				{
					if (szBuffer[dwPos2] < '0' || szBuffer[dwPos2] > '9')
					{
						MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
						Resume();
						return;
					}
				}
				if (dwPos2 == 0)
				{
					MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
					Resume();
					return;
				}
				dwValue = atoi(szBuffer);
			}
		}
	}

	lvi.iItem = 0;
	lvi.pszText = szBuffer;
	lvi.cchTextMax = sizeof(szBuffer);
	dwPos = 0xC000;

	while (true)
	{
		lvi.iSubItem = 0;
		if (FillList)
		{
			if (dwPos > (unsigned)0xE000 - (1 << m_SearchSize))
			{
				break;
			}
		}
		else
		{
			lvi.mask = LVIF_PARAM;
			if (!ListView_GetItem(m_hList, &lvi))
			{
				break;
			}
			dwPos = lvi.lParam;
			if (SendMessage(m_hRelLast, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				ListView_GetItem(m_hList, &lvi);
				if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					dwValue = 0;
					for (dwPos2 = 0; szBuffer[dwPos2]; dwPos2++)
					{
						HexToNum(&szBuffer[dwPos2]);
						dwValue = (dwValue << 4) | szBuffer[dwPos2];
					}
				}
				else
				{
					dwValue = atoi(szBuffer);
				}
			}
		}

		switch (m_SearchSize)
		{
		case 0:
			dwValue2 = MEM_CPU[dwPos - 0xC000];
			break;
		case 1:
			dwValue2 = *(WORD *)&MEM_CPU[dwPos - 0xC000];
			break;
		case 2:
			dwValue2 = *(DWORD *)&MEM_CPU[dwPos - 0xC000];
			break;
		}

		Keep = true;
		switch (uID)
		{
		case ID_SEARCHCHEAT_EQUAL:
			if (dwValue2 != dwValue)
			{
				Keep = false;
			}
			break;

		case ID_SEARCHCHEAT_NOTEQUAL:
			if (dwValue2 == dwValue)
			{
				Keep = false;
			}
			break;

		case ID_SEARCHCHEAT_LESSEQUAL:
			if (dwValue2 > dwValue)
			{
				Keep = false;
			}
			break;

		case ID_SEARCHCHEAT_MOREEQUAL:
			if (dwValue2 < dwValue)
			{
				Keep = false;
			}
			break;

		case ID_SEARCHCHEAT_LESS:
			if (dwValue2 >= dwValue)
			{
				Keep = false;
			}
			break;

		case ID_SEARCHCHEAT_MORE:
			if (dwValue2 <= dwValue)
			{
				Keep = false;
			}
			break;
		}

		if (FillList)
		{
			if (Keep)
			{
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iSubItem = 0;
				lvi.pszText = DwordToHex(dwPos, szBuffer);
				lvi.lParam = dwPos;
				lvi.iItem = ListView_InsertItem(m_hList, &lvi);
				if (lvi.iItem == -1)
				{
					Resume();
					return;
				}
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					lvi.pszText = DwordToHex(dwValue2, szBuffer);
				}
				else
				{
					lvi.pszText = ultoa(dwValue2, szBuffer, 10);
				}
				ListView_SetItem(m_hList, &lvi);
				lvi.iSubItem = 2;
				lvi.pszText = LPSTR_TEXTCALLBACK;
				ListView_SetItem(m_hList, &lvi);
			}
			dwPos++;
			lvi.iItem++;
		}
		else
		{
			if (Keep)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					lvi.pszText = DwordToHex(dwValue2, szBuffer);
				}
				else
				{
					lvi.pszText = ultoa(dwValue2, szBuffer, 10);
				}
				ListView_SetItem(m_hList, &lvi);
				lvi.iItem++;
			}
			else
			{
				ListView_DeleteItem(m_hList, lvi.iItem);
			}
		}
	}

	Resume();
}



int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	char			sz1[0x10], sz2[0x10];
	DWORD			dwPos;
	LVITEM			lvi;
	LVFINDINFO		lvfi;


	switch (((CGameBoy *)lParamSort)->m_SortBy & 0x7F)
	{
	case 1:
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = lParam1;
		lvi.mask = LVIF_TEXT;
		lvi.iItem = ListView_FindItem(((CGameBoy *)lParamSort)->m_hList, -1, &lvfi);
		lvi.iSubItem = 1;
		lvi.pszText = sz1;
		lvi.cchTextMax = sizeof(sz1);
		ListView_GetItem(((CGameBoy *)lParamSort)->m_hList, &lvi);
		lvi.pszText = sz2;
		lvi.cchTextMax = sizeof(sz2);
		lvfi.lParam = lParam2;
		lvi.iItem = ListView_FindItem(((CGameBoy *)lParamSort)->m_hList, -1, &lvfi);
		ListView_GetItem(((CGameBoy *)lParamSort)->m_hList, &lvi);
		if (SendMessage(((CGameBoy *)lParamSort)->m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			lParam1 = 0;
			for (dwPos = 0; sz1[dwPos]; dwPos++)
			{
				HexToNum(&sz1[dwPos]);
				lParam1 = (lParam1 << 4) | sz1[dwPos];
			}
			lParam2 = 0;
			for (dwPos = 0; sz2[dwPos]; dwPos++)
			{
				HexToNum(&sz2[dwPos]);
				lParam2 = (lParam2 << 4) | sz2[dwPos];
			}
		}
		else
		{
			lParam1 = atoi(sz1);
			lParam2 = atoi(sz2);
		}
		break;

	case 2:
		lParam1 = ((CGameBoy *)lParamSort)->MEM_CPU[lParam1 - 0xC000];
		lParam2 = ((CGameBoy *)lParamSort)->MEM_CPU[lParam2 - 0xC000];
		break;
	}

	if (((CGameBoy *)lParamSort)->m_SortBy & 0x80)
	{
		if (lParam1 < lParam2)
		{
			return 1;
		}
		if (lParam1 > lParam2)
		{
			return -1;
		}
	}
	else
	{
		if (lParam1 < lParam2)
		{
			return -1;
		}
		if (lParam1 > lParam2)
		{
			return 1;
		}
	}
	return 0;
}



LRESULT CGameBoy::SearchCheatWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char			szBuffer[0x100];
	RECT			rct;
	int				width, height;
	DWORD			dwPos, dwValue;
	LVCOLUMN		lvc;
	LVITEM			lvi;
	BOOL			UseLastValue;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SEARCHCHEAT_NEWSEARCH:
			ListView_DeleteAllItems(m_hList);
			EnableWindow(m_hSize, true);
			EnableWindow(m_hNewSearch, false);
			return 0;

		case ID_SEARCHCHEAT_HEX:
			lvi.mask = LVIF_TEXT;
			lvi.iSubItem = 1;
			lvi.pszText = szBuffer;
			lvi.cchTextMax = sizeof(szBuffer);
			for (lvi.iItem = 0; ListView_GetItem(m_hList, &lvi); lvi.iItem++)
			{
				dwValue = atoi(szBuffer);
				DwordToHex(dwValue, szBuffer);
				ListView_SetItem(m_hList, &lvi);
			}
			InvalidateRect(m_hList, NULL, true);
			return 0;

		case ID_SEARCHCHEAT_DEC:
			lvi.mask = LVIF_TEXT;
			lvi.iSubItem = 1;
			lvi.pszText = szBuffer;
			lvi.cchTextMax = sizeof(szBuffer);
			for (lvi.iItem = 0; ListView_GetItem(m_hList, &lvi); lvi.iItem++)
			{
				dwValue = 0;
				for (dwPos = 0; szBuffer[dwPos]; dwPos++)
				{
					HexToNum(&szBuffer[dwPos]);
					dwValue = (dwValue << 4) | szBuffer[dwPos];
				}
				ultoa(dwValue, szBuffer, 10);
				ListView_SetItem(m_hList, &lvi);
			}
			InvalidateRect(m_hList, NULL, true);
			return 0;

		case ID_SEARCHCHEAT_RELATIVEVALUE:
			EnableWindow(m_hValue, true);
			return 0;

		case ID_SEARCHCHEAT_RELATIVELAST:
			EnableWindow(m_hValue, false);
			return 0;

		case ID_SEARCHCHEAT_EQUAL:
		case ID_SEARCHCHEAT_NOTEQUAL:
		case ID_SEARCHCHEAT_LESSEQUAL:
		case ID_SEARCHCHEAT_MOREEQUAL:
		case ID_SEARCHCHEAT_LESS:
		case ID_SEARCHCHEAT_MORE:
			PerformSearch(LOWORD(wParam));
			ListView_SortItems(m_hList, CompareFunc, this);
			if (ListView_GetItemCount(m_hList))
			{
				EnableWindow(m_hSize, false);
				EnableWindow(m_hNewSearch, true);
			}
			else
			{
				EnableWindow(m_hSize, true);
				EnableWindow(m_hNewSearch, false);
			}
			return 0;

		case ID_SEARCHCHEAT_NAME:
			if (HIWORD(wParam) == EN_UPDATE)
			{
				GetWindowText(m_hName, szBuffer, sizeof(szBuffer));
				if (szBuffer[0])
				{
					EnableWindow(m_hAddCheat, true);
				}
				else
				{
					EnableWindow(m_hAddCheat, false);
				}
			}
			break;

		case ID_ADDCHEAT:
			if (!ListView_GetSelectedCount(m_hList))
			{
				MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_NOSELECTION), "Game Lad", MB_OK | MB_ICONWARNING);
				return 0;
			}
			if (SendMessage(m_hRelLast, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				UseLastValue = true;
			}
			else
			{
				GetWindowText(m_hValue, szBuffer, sizeof(szBuffer));
				if (szBuffer[0])
				{
					if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
					{
						dwValue = 0;
						for (dwPos = 0; szBuffer[dwPos]; dwPos++)
						{
							if (HexToNum(&szBuffer[dwPos]))
							{
								MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
								return 0;
							}
							dwValue = (dwValue << 4) | szBuffer[dwPos];
						}
					}
					else
					{
						for (dwPos = 0; szBuffer[dwPos]; dwPos++)
						{
							if (szBuffer[dwPos] < '0' || szBuffer[dwPos] > '9')
							{
								MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
								return 0;
							}
						}
						dwValue = atoi(szBuffer);
					}
					UseLastValue = false;
				}
				else
				{
					MessageBox(hMsgParent, String(IDS_SEARCHCHEAT_ENTERNUMBER), "Game Lad", MB_OK | MB_ICONWARNING);
					return 0;
				}
			}
			GetWindowText(m_hName, szBuffer, sizeof(szBuffer));
			if (!szBuffer[0])
			{
				MessageBeep(MB_ICONASTERISK);
				return 0;
			}
			if (Cheats.AddCheat(this, szBuffer, NULL))
			{
				return 0;
			}
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = -1;
			lvi.iSubItem = 0;
			lvi.pszText = szBuffer;
			lvi.cchTextMax = sizeof(szBuffer);
			while ((lvi.iItem = ListView_GetNextItem(m_hList, lvi.iItem, LVNI_SELECTED)) != -1)
			{
				ListView_GetItem(m_hList, &lvi);
				if (UseLastValue)
				{
					dwValue = MEM_CPU[lvi.lParam - 0xC000];
				}
				szBuffer[9] = '\0';
				szBuffer[7] = szBuffer[0];
				szBuffer[8] = szBuffer[1];
				szBuffer[5] = szBuffer[2];
				szBuffer[6] = szBuffer[3];
				szBuffer[4] = '-';
				szBuffer[3] = NibbleToHex((BYTE)(dwValue & 0x0F));
				szBuffer[2] = NibbleToHex((BYTE)(dwValue & 0xF0));
				szBuffer[1] = '1';
				szBuffer[0] = '0';
				if (Cheats.AddCheat(this, NULL, szBuffer))
				{
					Cheats.Save();
					return 0;
				}
				if (m_SearchSize)
				{
					if (szBuffer[6] == '9')
					{
						szBuffer[6] = 'A';
					}
					else if (szBuffer[6] != 'F')
					{
						szBuffer[6]++;
					}
					else
					{
						szBuffer[6] = '0';
						if (szBuffer[5] == '9')
						{
							szBuffer[5] = 'A';
						}
						else if (szBuffer[5] != 'F')
						{
							szBuffer[5]++;
						}
						else
						{
							szBuffer[5] = '0';
							if (szBuffer[8] == '9')
							{
								szBuffer[8] = 'A';
							}
							else if (szBuffer[8] != 'F')
							{
								szBuffer[8]++;
							}
							else
							{
								szBuffer[8] = '0';
								szBuffer[9]++;
							}
						}
					}
					szBuffer[3] = NibbleToHex((BYTE)((dwValue >> 8) & 0x0F));
					szBuffer[2] = NibbleToHex((BYTE)((dwValue >> 8) & 0xF0));
					if (Cheats.AddCheat(this, NULL, szBuffer))
					{
						Cheats.Save();
						return 0;
					}
				}
				if (m_SearchSize > 1)
				{
					if (szBuffer[6] == '9')
					{
						szBuffer[6] = 'A';
					}
					else if (szBuffer[6] != 'F')
					{
						szBuffer[6]++;
					}
					else
					{
						szBuffer[6] = '0';
						if (szBuffer[5] == '9')
						{
							szBuffer[5] = 'A';
						}
						else if (szBuffer[5] != 'F')
						{
							szBuffer[5]++;
						}
						else
						{
							szBuffer[5] = '0';
							if (szBuffer[8] == '9')
							{
								szBuffer[8] = 'A';
							}
							else if (szBuffer[8] != 'F')
							{
								szBuffer[8]++;
							}
							else
							{
								szBuffer[8] = '0';
								szBuffer[9]++;
							}
						}
					}
					szBuffer[3] = NibbleToHex((BYTE)((dwValue >> 16) & 0x0F));
					szBuffer[2] = NibbleToHex((BYTE)((dwValue >> 16) & 0xF0));
					if (Cheats.AddCheat(this, NULL, szBuffer))
					{
						Cheats.Save();
						return 0;
					}
					if (szBuffer[6] == '9')
					{
						szBuffer[6] = 'A';
					}
					else if (szBuffer[6] != 'F')
					{
						szBuffer[6]++;
					}
					else
					{
						szBuffer[6] = '0';
						if (szBuffer[5] == '9')
						{
							szBuffer[5] = 'A';
						}
						else if (szBuffer[5] != 'F')
						{
							szBuffer[5]++;
						}
						else
						{
							szBuffer[5] = '0';
							if (szBuffer[8] == '9')
							{
								szBuffer[8] = 'A';
							}
							else if (szBuffer[8] != 'F')
							{
								szBuffer[8]++;
							}
							else
							{
								szBuffer[8] = '0';
								szBuffer[9]++;
							}
						}
					}
					szBuffer[3] = NibbleToHex((BYTE)((dwValue >> 24) & 0x0F));
					szBuffer[2] = NibbleToHex((BYTE)((dwValue >> 24) & 0xF0));
					if (Cheats.AddCheat(this, NULL, szBuffer))
					{
						Cheats.Save();
						return 0;
					}
				}
			}
			Cheats.Save();
			return 0;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->idFrom == IDC_LIST)
		{
			switch (((NMHDR *)lParam)->code)
			{
			case LVN_GETDISPINFO:
				lvi.mask = LVIF_TEXT;
				lvi.iItem = ((NMLVDISPINFO *)lParam)->item.iItem;
				lvi.iSubItem = 0;
				lvi.pszText = szBuffer;
				lvi.cchTextMax = sizeof(szBuffer);
				ListView_GetItem(m_hList, &lvi);
				HexToNum(&szBuffer[0]);
				HexToNum(&szBuffer[1]);
				HexToNum(&szBuffer[2]);
				HexToNum(&szBuffer[3]);
				dwPos = (szBuffer[0] << 12) | (szBuffer[1] << 8) | (szBuffer[2] << 4) | szBuffer[3];
				switch (m_SearchSize)
				{
				case 0:
					dwValue = MEM_CPU[dwPos - 0xC000];
					break;
				case 1:
					dwValue = *(WORD *)&MEM_CPU[dwPos - 0xC000];
					break;
				case 2:
					dwValue = *(DWORD *)&MEM_CPU[dwPos - 0xC000];
					break;
				}
				if (SendMessage(m_hHex, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					DwordToHex(dwValue, ((NMLVDISPINFO *)lParam)->item.pszText);
				}
				else
				{
					ultoa(dwValue, ((NMLVDISPINFO *)lParam)->item.pszText, 10);
				}
				return 0;

			case LVN_COLUMNCLICK:
				if ((m_SortBy & ~0x80) == ((NMLISTVIEW *)lParam)->iSubItem)
				{
					if (m_SortBy & 0x80)
					{
						m_SortBy &= ~0x80;
					}
					else
					{
						m_SortBy |= 0x80;
					}
				}
				else
				{
					m_SortBy = ((NMLISTVIEW *)lParam)->iSubItem;
				}
				ListView_SortItems(((NMHDR *)lParam)->hwndFrom, CompareFunc, this);
				return 0;
			}
		}
		break;

	case WM_APP_CHANGELANGUAGE:
		SetWindowText(m_hSearchCheatWnd, String(IDS_SEARCHCHEAT_TITLE));
		dwPos = SendMessage(m_hSize, CB_GETCURSEL, 0, 0);
		if (dwPos == CB_ERR)
		{
			dwPos = 0;
		}
		SendMessage(m_hSize, CB_RESETCONTENT, 0, 0);
		SendMessage(m_hSize, CB_ADDSTRING, 0, (LPARAM)String(IDS_SEARCHCHEAT_BYTE));
		SendMessage(m_hSize, CB_ADDSTRING, 0, (LPARAM)String(IDS_SEARCHCHEAT_WORD));
		SendMessage(m_hSize, CB_ADDSTRING, 0, (LPARAM)String(IDS_SEARCHCHEAT_DWORD));
		SendMessage(m_hSize, CB_SETCURSEL, dwPos, 0);
		SetWindowText(m_hNewSearch, String(IDS_SEARCHCHEAT_NEWSEARCH));
		SetWindowText(m_hHex, String(IDS_SEARCHCHEAT_HEXADECIMAL));
		SetWindowText(m_hDec, String(IDS_SEARCHCHEAT_DECIMAL));
		SetWindowText(m_hRelValue, String(IDS_SEARCHCHEAT_RELATIVEVALUE));
		SetWindowText(m_hRelLast, String(IDS_SEARCHCHEAT_RELATIVELAST));
		SetWindowText(m_hEqual, String(IDS_SEARCHCHEAT_EQUAL));
		SetWindowText(m_hNotEqual, String(IDS_SEARCHCHEAT_NOTEQUAL));
		SetWindowText(m_hLessEqual, String(IDS_SEARCHCHEAT_LESSEQUAL));
		SetWindowText(m_hMoreEqual, String(IDS_SEARCHCHEAT_MOREEQUAL));
		SetWindowText(m_hLess, String(IDS_SEARCHCHEAT_LESS));
		SetWindowText(m_hMore, String(IDS_SEARCHCHEAT_MORE));
		SetWindowText(m_hNameStatic, String(IDS_SEARCHCHEAT_CHEATNAME));
		SetWindowText(m_hAddCheat, String(IDS_SEARCHCHEAT_ADDCHEAT));
		lvc.mask = LVCF_TEXT;
		lvc.pszText = String(IDS_SEARCHCHEAT_ADDRESS);
		ListView_SetColumn(m_hList, 0, &lvc);
		lvc.pszText = String(IDS_SEARCHCHEAT_LASTVALUE);
		ListView_SetColumn(m_hList, 1, &lvc);
		lvc.pszText = String(IDS_SEARCHCHEAT_CURRENTVALUE);
		ListView_SetColumn(m_hList, 2, &lvc);
		return 0;

	case WM_SYSCOMMAND:
		//Window cannot be maximized
		if ((wParam & 0xFFF0) == SC_MAXIMIZE)
		{
			return 0;
		}
		break;

	case WM_NCHITTEST:
		//Disable resizing
		switch (DefMDIChildProc(m_hSearchCheatWnd, uMsg, wParam, lParam))
		{
		case HTBOTTOM:
		case HTBOTTOMLEFT:
		case HTBOTTOMRIGHT:
		case HTLEFT:
		case HTRIGHT:
		case HTTOP:
		case HTTOPLEFT:
		case HTTOPRIGHT:
			return HTBORDER;
		}
		break;

	case WM_DESTROY:
		m_hSearchCheatWnd = NULL;
		return 0;

	case WM_CREATE:
		//Center dialog
		GetWindowRect(m_hSearchCheatWnd, &rct);
		width = rct.right - rct.left;
		height = rct.bottom - rct.top;
		GetWindowRect(hWnd, &rct);
		rct.right -= rct.left; //width
		rct.bottom -= rct.top; //height
		MoveWindow(m_hSearchCheatWnd, rct.left + ((rct.right - width) >> 1), rct.top + ((rct.bottom - height) >> 1), width, height, true);

		m_hSize = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hSize, WM_SETFONT, (WPARAM)hFont, 0);
		m_hNewSearch = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hNewSearch, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hNewSearch, GWL_ID, ID_SEARCHCHEAT_NEWSEARCH);
		m_hValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | WS_DISABLED | ES_AUTOHSCROLL, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hValue, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(m_hValue, EM_LIMITTEXT, 10, 0);
		m_hHex = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hHex, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hHex, GWL_ID, ID_SEARCHCHEAT_HEX);
		m_hDec = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hDec, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hDec, GWL_ID, ID_SEARCHCHEAT_DEC);
		SendMessage(m_hDec, BM_SETCHECK, BST_CHECKED, 0);
		m_hRelValue = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hRelValue, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hRelValue, GWL_ID, ID_SEARCHCHEAT_RELATIVEVALUE);
		m_hRelLast = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hRelLast, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hRelLast, GWL_ID, ID_SEARCHCHEAT_RELATIVELAST);
		SendMessage(m_hRelLast, BM_SETCHECK, BST_CHECKED, 0);

		m_hEqual = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hEqual, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hEqual, GWL_ID, ID_SEARCHCHEAT_EQUAL);
		m_hNotEqual = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hNotEqual, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hNotEqual, GWL_ID, ID_SEARCHCHEAT_NOTEQUAL);
		m_hLessEqual = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hLessEqual, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hLessEqual, GWL_ID, ID_SEARCHCHEAT_LESSEQUAL);
		m_hMoreEqual = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hMoreEqual, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hMoreEqual, GWL_ID, ID_SEARCHCHEAT_MOREEQUAL);
		m_hLess = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hLess, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hLess, GWL_ID, ID_SEARCHCHEAT_LESS);
		m_hMore = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hMore, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hMore, GWL_ID, ID_SEARCHCHEAT_MORE);

		m_hNameStatic = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hNameStatic, WM_SETFONT, (WPARAM)hFont, 0);
		m_hName = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hName, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hName, GWL_ID, ID_SEARCHCHEAT_NAME);
		SendMessage(m_hName, EM_LIMITTEXT, 255, 0);
		m_hAddCheat = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hAddCheat, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hAddCheat, GWL_ID, ID_ADDCHEAT);
		m_hList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 0, 0, m_hSearchCheatWnd, NULL, hInstance, NULL);
		SendMessage(m_hList, WM_SETFONT, (WPARAM)hFont, 0);
		SetWindowLong(m_hList, GWL_ID, IDC_LIST);
		ListView_SetExtendedListViewStyleEx(m_hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.pszText = "";
		lvc.cx = 75;
		ListView_InsertColumn(m_hList, 0, &lvc);
		ListView_InsertColumn(m_hList, 1, &lvc);
		ListView_InsertColumn(m_hList, 2, &lvc);

		SendMessage(m_hSearchCheatWnd, WM_APP_CHANGELANGUAGE, 0, 0);

	case WM_SIZE:
		MoveWindow(m_hSize, 5, 5, 70, 200, true);
		SendMessage(m_hSize, CB_SETITEMHEIGHT, -1, 14);
		MoveWindow(m_hNewSearch, 80, 5, 70, 20, true);
		MoveWindow(m_hValue, 5, 30, 70, 20, true);
		MoveWindow(m_hHex, 80, 30, 90, 20, true);
		MoveWindow(m_hDec, 170, 30, 90, 20, true);
		MoveWindow(m_hRelValue, 5, 55, 125, 20, true);
		MoveWindow(m_hRelLast, 135, 55, 125, 20, true);
		MoveWindow(m_hEqual, 90, 80, 80, 20, true);
		MoveWindow(m_hLessEqual, 5, 80, 80, 20, true);
		MoveWindow(m_hMoreEqual, 175, 80, 80, 20, true);
		MoveWindow(m_hNotEqual, 90, 105, 80, 20, true);
		MoveWindow(m_hLess, 5, 105, 80, 20, true);
		MoveWindow(m_hMore, 175, 105, 80, 20, true);
		MoveWindow(m_hNameStatic, 5, 130, 70, 20, true);
		MoveWindow(m_hName, 80, 130, 100, 20, true);
		MoveWindow(m_hAddCheat, 185, 130, 70, 20, true);
		MoveWindow(m_hList, 5, 155, 250, 150, true);
		return 0;
	}

	return DefMDIChildProc(m_hSearchCheatWnd, uMsg, wParam, lParam);
}



void CGameBoy::SearchCheat()
{
	if (m_hSearchCheatWnd)
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)m_hSearchCheatWnd, 0);
		return;
	}

	if (!(m_hSearchCheatWnd = CreateWindowEx(WS_EX_MDICHILD | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW | WS_EX_CONTROLPARENT, "SearchCheat", NULL, WS_VISIBLE | WS_CAPTION | WS_BORDER, 0, 0, 270, 335, hClientWnd, NULL, hInstance, NULL)))
	{
		DisplayErrorMessage();
	}
	SetWindowLong(m_hSearchCheatWnd, GWL_USERDATA, (LONG)this);
	SendMessage(m_hSearchCheatWnd, WM_CREATE, 0, 0);
}



void CGameBoy::SetFrameSkip(int nFrameSkip)
{
	char		szBuffer[0x100], szFrameSkip[2];


	if (nFrameSkip >= 0)
	{
		if (nFrameSkip <= 9)
		{
			FrameSkip = nFrameSkip;
		}
		else
		{
			FrameSkip = 9;
		}
	}
	else
	{
		if (nFrameSkip == -1)
		{
			FrameSkip = -1;
			SetStatus(String(IDS_STATUS_SELECTFRAMESKIPAUTO), SF_MESSAGE);
			return;
		}
		FrameSkip = 0;
	}

	szFrameSkip[0] = FrameSkip + '0';
	szFrameSkip[1] = '\0';
	SetStatus(LoadString(IDS_STATUS_SELECTFRAMESKIP, szBuffer, sizeof(szBuffer), szFrameSkip), SF_MESSAGE);
}



BOOL CGameBoy::SaveSnapshot()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH], szBuffer[0x100], szBuffer2[0x100];
	DWORD				dw, dwFilePointer, nBytes;
	HANDLE				hFile;
	BITMAPINFOHEADER	bmi;
	BITMAPFILEHEADER	bfh;


	Pause();

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	String(IDS_OPEN_BITMAP);
	dw = strlen(szBuffer) + 1;
	strcpy(szBuffer + dw, "*.BMP");
	dw += strlen(szBuffer + dw) + 1;
	LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
	//strcpy(szBuffer + dw, "GIF files");
	dw += strlen(szBuffer + dw) + 1;
	//strcpy(szBuffer + dw, "*.GIF");
	strcpy(szBuffer + dw, "*.*");
	dw += strlen(szBuffer + dw) + 1;
	*(szBuffer + dw) = '\0';
	of.lpstrFilter = szBuffer;
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.lpstrTitle = LoadString(IDS_SAVEAS, szBuffer2, sizeof(szBuffer2));
	of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	of.lpstrDefExt = "bmp";
	szFilename[0] = 0;
	if (!GetSaveFileName(&of))
	{
		Resume();
		return true;
	}

	while ((hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			Resume();
			return true;
		}
	}

	/*switch (of.nFilterIndex)
	{
	case 0:*/
		//Save as bitmap

		bfh.bfType = 0x4D42;
		bfh.bfSize = sizeof(bfh) + sizeof(bmi) + 160 * 144 * 2;
		bfh.bfReserved1 = 0;
		bfh.bfReserved2 = 0;
		bfh.bfOffBits = 0;

		dwFilePointer = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		while (!WriteFile(hFile, &bfh, sizeof(bfh), &nBytes, NULL) || nBytes != sizeof(bfh))
		{
			SetFilePointer(hFile, dwFilePointer, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				DeleteFile(szFilename);
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), SF_MESSAGE);
				Resume();
				return true;
			}
		}

		ZeroMemory(&bmi, sizeof(bmi));
		bmi.biSize = sizeof(BITMAPINFOHEADER);
		bmi.biWidth = 160;
		bmi.biHeight = 144;
		bmi.biPlanes = 1;
		bmi.biBitCount = 16;
		bmi.biCompression = BI_RGB;

		dwFilePointer = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		while (!WriteFile(hFile, &bmi, sizeof(bmi), &nBytes, NULL) || nBytes != sizeof(bmi))
		{
			SetFilePointer(hFile, dwFilePointer, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
			{
				CloseHandle(hFile);
				DeleteFile(szFilename);
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), SF_MESSAGE);
				Resume();
				return true;
			}
		}

		for (dw = 0; dw < 144; dw++)
		{
			dwFilePointer = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
			while (!WriteFile(hFile, pGBBitmap + (143 - dw) * (160 + 14) + 7, 160 * 2, &nBytes, NULL) || nBytes != 160 * 2)
			{
				SetFilePointer(hFile, dwFilePointer, NULL, FILE_BEGIN);
				SetEndOfFile(hFile);
				if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)
				{
					CloseHandle(hFile);
					DeleteFile(szFilename);
					SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), SF_MESSAGE);
					Resume();
					return true;
				}
			}
		}
		/*break;

	case 1:
		//Save as GIF

#undef		WriteToFile
#define		WriteToFile(p, size)									\
		dwFilePointer = SetFilePointer(hFile, 0, 0, FILE_CURRENT);	\
		while (!WriteFile(hFile, p, size, &nBytes, NULL) || nBytes != size)	\
		{	\
			SetFilePointer(hFile, dwFilePointer, NULL, FILE_BEGIN);	\
			SetEndOfFile(hFile);	\
			if (MessageBox(hMsgParent, LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), NULL, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL)	\
			{	\
				CloseHandle(hFile);	\
				DeleteFile(szFilename);	\
				SetStatus(LoadString(IDS_STATUS_WRITEERROR, szBuffer, sizeof(szBuffer), szFilename), SF_MESSAGE);	\
				Resume();	\
				return true;	\
			}	\
		}
		WriteToFile("GIF87a", 6);
		Value = 160;
		WriteToFile(Value, 2);
		Value = 144;
		WriteToFile(Value, 2);
		Value = 0x80 | (7 << 4) | 7;
		WriteToFile(Value, 1);
		Value = 0;
		WriteToFile(Value, 2);
		WriteToFile(Value, 3);
		break;
	}*/

	CloseHandle(hFile);

	SetStatus(LoadString(IDS_STATUS_SAVED, szBuffer, sizeof(szBuffer), szFilename), SF_MESSAGE);

	Resume();

	return false;
}



struct AVIINFO
{
	BYTE				AVISoundBuffer[22050 * 2];
	PAVIFILE			pfile;
	PAVISTREAM			pavi, pavic, psnd;
	DWORD				frame;
	DWORD				dwAVIStreamPos;
};



BOOL CGameBoy::SaveVideo()
{
	OPENFILENAME		of;
	char				szFilename[MAX_PATH], szBuffer[0x100], szBuffer2[0x100];
	DWORD				dw;
	AVISTREAMINFO		asi;
	BITMAPINFOHEADER	bmih;
	WAVEFORMATEX		wfx;
	AVIINFO				*pAVIInfo;


	Pause();

	if (pAVISoundBuffer)
	{
		CloseAVI();
	}

	if (!(pAVIInfo = new AVIINFO))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return true;
	}

	ZeroMemory(&of, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = hWnd;
	String(IDS_OPEN_AVI);
	dw = strlen(szBuffer) + 1;
	strcpy(szBuffer + dw, "*.AVI");
	dw += strlen(szBuffer + dw) + 1;
	LoadString(IDS_OPEN_ALLFILES, szBuffer + dw, sizeof(szBuffer) - dw);
	dw += strlen(szBuffer + dw) + 1;
	strcpy(szBuffer + dw, "*.*");
	dw += strlen(szBuffer + dw) + 1;
	*(szBuffer + dw) = '\0';
	of.lpstrFilter = szBuffer;
	of.nFilterIndex = 1;
	of.lpstrFile = szFilename;
	of.nMaxFile = sizeof(szFilename);
	of.lpstrTitle = LoadString(IDS_SAVEAS, szBuffer2, sizeof(szBuffer2));
	of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	of.lpstrDefExt = "avi";
	szFilename[0] = 0;
	if (!GetSaveFileName(&of))
	{
		delete pAVIInfo;
		Resume();
		return true;
	}

	AVIFileInit();

	AVIFileOpen(&pAVIInfo->pfile, szFilename, OF_CREATE | OF_WRITE, NULL);
	ZeroMemory(&asi, sizeof(asi));
	asi.fccType = streamtypeVIDEO;
	asi.fccHandler = 0;
	asi.dwScale = 1;
	asi.dwRate = 60;
	//asi.dwSuggestedBufferSize = 160 * 144 * 2;
	asi.rcFrame.left = 0;
	asi.rcFrame.right = 160;
	asi.rcFrame.bottom = 144;
	AVIFileCreateStream(pAVIInfo->pfile, &pAVIInfo->pavi, &asi);
	if (AVISaveOptions(hWnd, 0, 1, &pAVIInfo->pavi, (AVICOMPRESSOPTIONS **)&aaco) != TRUE)
	{
		AVIStreamClose(pAVIInfo->pavi);
		AVIFileClose(pAVIInfo->pfile);
		AVIFileExit();
		delete pAVIInfo;
		return true;
	}
	AVIMakeCompressedStream(&pAVIInfo->pavic, pAVIInfo->pavi, &aco, NULL);
	pAVIInfo->frame = 0;
	ZeroMemory(&bmih, sizeof(bmih));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = 160;
	bmih.biHeight = 144;
	bmih.biPlanes = 1;
	bmih.biBitCount = 16;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 160 * 144 * 2;
	AVIStreamSetFormat(pAVIInfo->pavic, 0, &bmih, sizeof(bmih));

	ZeroMemory(&asi, sizeof(asi));
	asi.fccType = streamtypeAUDIO;
	asi.dwScale = 2;
	asi.dwRate = 22050 * 2;
	asi.dwSampleSize = 2;
	AVIFileCreateStream(pAVIInfo->pfile, &pAVIInfo->psnd, &asi);
	ZeroMemory(&wfx, sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 22050;
	wfx.nAvgBytesPerSec = 22050 * 2;
	wfx.nBlockAlign = 2;
	wfx.wBitsPerSample = 8;
	AVIStreamSetFormat(pAVIInfo->psnd, 0, &wfx, sizeof(wfx));
	dwAVISoundPos = 0;
	pAVIInfo->dwAVIStreamPos = 0;
	pAVISoundBuffer = pAVIInfo;

	Resume();

	return false;
}



void CGameBoy::CloseAVI()
{
	if (!pAVISoundBuffer)
	{
		return;
	}

	Pause();

	AVIStreamClose(((AVIINFO *)pAVISoundBuffer)->psnd);
	AVIStreamClose(((AVIINFO *)pAVISoundBuffer)->pavic);
	AVIStreamClose(((AVIINFO *)pAVISoundBuffer)->pavi);
	AVIFileClose(((AVIINFO *)pAVISoundBuffer)->pfile);
	AVIFileExit();

	delete pAVISoundBuffer;
	pAVISoundBuffer = NULL;

	Resume();
}



BOOL CGameBoy::WriteAVI()
{
	BYTE		Bitmap[144][160 * 2];
	DWORD		y;


	if (!pAVISoundBuffer)
	{
		return true;
	}

	for (y = 0; y < 144; y++)
	{
		CopyMemory(Bitmap[y], pGBBitmap + (160 + 14) * (143 - y) + 7, 160 * 2);
	}
	if (AVIStreamWrite(((AVIINFO *)pAVISoundBuffer)->pavic, ((AVIINFO *)pAVISoundBuffer)->frame++, 1, &Bitmap, 160 * 144 * 2, AVIIF_KEYFRAME, NULL, NULL))
	{
		CloseAVI();
		MessageBox(hMsgParent, "Error writing to avi file.", NULL, MB_OK | MB_ICONERROR);
		return true;
	}
	if (dwAVISoundPos != 0)
	{
		if (AVIStreamWrite(((AVIINFO *)pAVISoundBuffer)->psnd, ((AVIINFO *)pAVISoundBuffer)->dwAVIStreamPos, dwAVISoundPos / 2, pAVISoundBuffer, dwAVISoundPos, 0, NULL, NULL))
		{
			CloseAVI();
			MessageBox(hMsgParent, "Error writing to avi file.", NULL, MB_OK | MB_ICONERROR);
			return true;
		}
		((AVIINFO *)pAVISoundBuffer)->dwAVIStreamPos += dwAVISoundPos / 2;
		dwAVISoundPos = 0;
	}

	return false;
}



void CGameBoy::LinkExecuteLoop()
{
	char			szBuffer[0x100];
	BYTE			b;
	//DWORD			SerialTicks2;
	//BYTE			SerialBit2, FF01, FF02;


	if (Settings.SoundEnabled)
	{
		RestoreSound();
		/*if (pLinkGameBoy)
		{*/
			pLinkGameBoy->RestoreSound();
		//}
	}
	MemoryFlags = DisAsmFlags = 0;

	PrepareEmulation(false);
	Flags |= GB_LINKCABLE;
	/*if (pLinkGameBoy)
	{*/
		pLinkGameBoy->Stop();
		pLinkGameBoy->PrepareEmulation(false);
		pLinkGameBoy->Flags |= GB_LINKCABLE;
	/*}
	else
	{
		SyncNetworkLinkCable(1, SerialTicks, 0, FF00_C(0x01), FF00_C(0x02), &SerialTicks2, &SerialBit2, &FF01, &FF02);
	}*/


	while (true)
	{
		EnterCriticalSection(&csTerminate);
		if (Terminating != TERMINATING_STOP)
		{
			GameBoys.UpdateKeys(this);
			/*if (pLinkGameBoy)
			{*/
				GameBoys.UpdateKeys(pLinkGameBoy);
			//}
		}
		LeaveCriticalSection(&csTerminate);

		/*if (pLinkGameBoy)
		{*/
			do
			{
				if ((signed)SerialTicks > 0 && (signed)pLinkGameBoy->SerialTicks > 0)
				{
					if (SerialTicks > pLinkGameBoy->SerialTicks)
					{
						SerialTicks -= pLinkGameBoy->SerialTicks;
						pLinkGameBoy->SerialTicks = 0;
					}
					else
					{
						pLinkGameBoy->SerialTicks -= SerialTicks;
						SerialTicks = 0;
					}
				}
				SerialTicks += 128;
				MainLoop();
				pLinkGameBoy->SerialTicks += 128 - SerialTicks;
				if ((signed)pLinkGameBoy->SerialTicks > 0)
				{
					pLinkGameBoy->MainLoop();
				}

				if (Flags & GB_INVALIDOPCODE)
				{
					STOPEMULATION2;
					SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
					SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
					MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
					return;
				}

				if (pLinkGameBoy->Flags & GB_INVALIDOPCODE)
				{
					STOPEMULATION2;
					SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pLinkGameBoy->hGBWnd, 0);
					SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
					MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
					return;
				}

				if ((Flags & GB_SERIALBIT) || (pLinkGameBoy->Flags & GB_SERIALBIT))
				{
					Flags &= ~GB_SERIALBIT;
					pLinkGameBoy->Flags &= ~GB_SERIALBIT;

					if (FF00_C(0x02) & 0x80)
					{
						if ((FF00_C(0x02) & 1) || ((pFF00_C(pLinkGameBoy, 0x02) & 0x81) == 0x81))
						{
							if (SerialBit < 8)
							{
								if (pFF00_C(pLinkGameBoy, 0x02) & 0x80)
								{
									b = FF00_C(0x01) >> 7;
									FF00_C(0x01) = (FF00_C(0x01) << 1) | (pFF00_C(pLinkGameBoy, 0x01) >> 7);
									pFF00_C(pLinkGameBoy, 0x01) = (pFF00_C(pLinkGameBoy, 0x01) << 1) | b;
								}
								else
								{
									FF00_C(0x01) <<= 1;
								}
								if (++SerialBit == 8)
								{
									FF00_C(0x02) &= ~0x80;
									FF00_C(0x0F) |= 0x08;
								}
								if (++pLinkGameBoy->SerialBit == 8)
								{
									pFF00_C(pLinkGameBoy, 0x02) &= ~0x80;
									pFF00_C(pLinkGameBoy, 0x0F) |= 0x08;
								}
							}
							else
							{
								SerialBit = 0;
								pLinkGameBoy->SerialBit = 0;
							}
						}
					}
					else if ((pFF00_C(pLinkGameBoy, 0x02) & 0x81) == 0x81)
					{
						if (pLinkGameBoy->SerialBit < 8)
						{
							pFF00_C(pLinkGameBoy, 0x01) <<= 1;
							if (++pLinkGameBoy->SerialBit == 8)
							{
								pFF00_C(pLinkGameBoy, 0x02) &= ~0x80;
								pFF00_C(pLinkGameBoy, 0x0F) |= 0x08;
							}
						}
						else
						{
							SerialBit = 0;
							pLinkGameBoy->SerialBit = 0;
						}
					}
				}
			}
			while (nCycles < 35112 * 4);
		/*}
		else
		{
			do
			{
				if ((signed)SerialTicks > 0 && (signed)SerialTicks2 > 0)
				{
					if (SerialTicks > SerialTicks2)
					{
						SerialTicks -= SerialTicks2;
						SerialTicks2 = 0;
					}
					else
					{
						SerialTicks2 -= SerialTicks;
						SerialTicks = 0;
					}
				}
				SerialTicks += 128;
				MainLoop();

				SyncNetworkLinkCable(1, SerialTicks, Flags & GB_SERIALBIT ? 1 : 0, FF00_C(0x01), FF00_C(0x02), &SerialTicks2, &SerialBit2, &FF01, &FF02);

				if (Flags & GB_SERIALBIT || SerialBit2)
				{
					Flags &= ~GB_SERIALBIT;

					if (FF00_C(0x02) & 0x80)
					{
						if ((FF00_C(0x02) & 1) || ((FF02 & 0x81) == 0x81))
						{
							if (SerialBit < 8)
							{
								if (FF02 & 0x80)
								{
									FF00_C(0x01) = (FF00_C(0x01) << 1) | (FF01 >> 7);
								}
								else
								{
									FF00_C(0x01) <<= 1;
								}
								if (++SerialBit == 8)
								{
									FF00_C(0x02) &= ~0x80;
									FF00_C(0x0F) |= 0x08;
								}
							}
						}
					}
				}

				if (Flags & GB_INVALIDOPCODE)
				{
					STOPEMULATION2;
					SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
					SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
					MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
					return;
				}
			}
			while (nCycles < 35112 * 4);
		}*/


		if (pAVISoundBuffer)
		{
			WriteAVI();
		}
		/*if (pLinkGameBoy)
		{*/
			if (pLinkGameBoy->pAVISoundBuffer)
			{
				pLinkGameBoy->WriteAVI();
			}
		//}


		Delay();

		RefreshScreen();
		/*if (pLinkGameBoy)
		{*/
			pLinkGameBoy->RefreshScreen();
		//}

		if (Terminating)
		{
			EnterCriticalSection(&csTerminate);

			switch (Terminating)
			{
			case TERMINATING_STOP:
				LeaveCriticalSection(&csTerminate);
				STOPEMULATION2;
				return;

			case TERMINATING_PAUSE:
				LeaveCriticalSection(&csTerminate);
				PAUSEEMULATION2;
				while (true)
				{
					while (Terminating == TERMINATING_PAUSE)
					{
						Sleep(0);
					}
					EnterCriticalSection(&csTerminate);
					if (Terminating == TERMINATING_STOP)
					{
						LeaveCriticalSection(&csTerminate);
						STOPEMULATION2;
						return;
					}
					if (Terminating == TERMINATING_FALSE)
					{
						LeaveCriticalSection(&csTerminate);
						break;
					}
				}
				RESUMEEMULATION2;
				break;

			default:
				LeaveCriticalSection(&csTerminate);
			}
		}
	}
}



//#define		TIMEDEMO



void CGameBoy::ExecuteLoop()
{
	char			szBuffer[0x100];
	BOOL			cs;


#ifdef TIMEDEMO
	LARGE_INTEGER	StartTime, StopTime, TimerFrequency;
	DWORD			Count = 0;
	char			NumBuffer2[10];
#endif //TIMEDEMO


	EnterCriticalSection(&csGameBoy);
	if (pLinkGameBoy)
	{
		LeaveCriticalSection(&csGameBoy);
		return;
	}
	if (pLinkGameBoy = GameBoys.GetPlayer2(true))
	{
		/*if (pLinkGameBoy == INVALID_HANDLE_VALUE)
		{
			pLinkGameBoy = NULL;

			LeaveCriticalSection(&csGameBoy);

			if (NetworkLinkCable == NLC_FALSE)
			{
				return;
			}
			while (NetworkLinkCable != NLC_TRUE)
			{
				if (NetworkLinkCable == NLC_FALSE)
				{
					return;
				}
				Sleep(0);
			}

			if (NetworkLinkCable == NLC_TRUE)
			{
				SendMessage(hWnd, WM_APP_INITGFX, 0, (LPARAM)this);
				LinkExecuteLoop();
			}
			return;
		}
		else
		{*/
			if (!pLinkGameBoy->pLinkGameBoy && !pLinkGameBoy->IsEmulating())
			{
				pLinkGameBoy->pLinkGameBoy = this;
				LeaveCriticalSection(&csGameBoy);

				SendMessage(hWnd, WM_APP_INITGFX, 0, (LPARAM)this);
				SendMessage(hWnd, WM_APP_INITGFX, 0, (LPARAM)pLinkGameBoy);
				LinkExecuteLoop();

				EnterCriticalSection(&csTerminate);
				if (Terminating != TERMINATING_STOP)
				{
					EnterCriticalSection(&csGameBoy);
					cs = true;
				}
				else
				{
					cs = false;
				}
				LeaveCriticalSection(&csTerminate);
				pLinkGameBoy->pLinkGameBoy = NULL;
				pLinkGameBoy->Release();
				pLinkGameBoy = NULL;
				if (cs)
				{
					LeaveCriticalSection(&csGameBoy);
				}
				return;
			}
			pLinkGameBoy->Release();
			pLinkGameBoy = NULL;
		//}
	}
	LeaveCriticalSection(&csGameBoy);


	SendMessage(hWnd, WM_APP_INITGFX, 0, (LPARAM)this);

	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}
	PrepareEmulation(false);
	MemoryFlags = DisAsmFlags = 0;


#ifdef TIMEDEMO
	QueryPerformanceFrequency(&TimerFrequency);
	QueryPerformanceCounter(&StartTime);
#endif //TIMEDEMO

	while (true)
	{
		GameBoys.UpdateKeys(this);

		MainLoop();
		if (Flags & GB_ERROR)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			if (Flags & GB_INVALIDOPCODE)
			{
				MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
			}
			return;
		}


		if (pAVISoundBuffer)
		{
			WriteAVI();
		}


#ifndef TIMEDEMO
		Delay();
#endif

		RefreshScreen();


		if (Terminating)
		{
			EnterCriticalSection(&csTerminate);

			switch (Terminating)
			{
			case TERMINATING_STOP:
				LeaveCriticalSection(&csTerminate);
				STOPEMULATION;
				return;

			case TERMINATING_PAUSE:
				LeaveCriticalSection(&csTerminate);
				PAUSEEMULATION;
				while (true)
				{
					while (Terminating == TERMINATING_PAUSE)
					{
						Sleep(0);
					}
					EnterCriticalSection(&csTerminate);
					if (Terminating == TERMINATING_STOP)
					{
						LeaveCriticalSection(&csTerminate);
						STOPEMULATION;
						return;
					}
					if (Terminating == TERMINATING_FALSE)
					{
						LeaveCriticalSection(&csTerminate);
						break;
					}
				}
				RESUMEEMULATION;
				break;

			default:
				LeaveCriticalSection(&csTerminate);
			}
		}


#ifdef TIMEDEMO
		if (Count += nCycles > 0x10000)
		{
			QueryPerformanceCounter(&StopTime);
			STOPEMULATION;
			MessageBox(hMsgParent, ultoa(Count, NumBuffer, 10), ultoa((DWORD)(StopTime.QuadPart - StartTime.QuadPart), NumBuffer2, 10), MB_OK | MB_ICONINFORMATION);
			return;
		}
		nCycles = 0;
#endif //TIMEDEMO
	}
}



void CGameBoy::LinkDebugLoop()
{
	char			szBuffer[0x100];
	BYTE			b;


	if (Settings.SoundEnabled)
	{
		RestoreSound();
		pLinkGameBoy->RestoreSound();
	}
	PrepareEmulation(false);
	pLinkGameBoy->Stop();
	pLinkGameBoy->PrepareEmulation(false);
	MemoryFlags = DisAsmFlags = 0;


	SerialTicks = 0;
	pLinkGameBoy->SerialTicks = 0;

	Flags |= GB_LINKCABLE;
	pLinkGameBoy->Flags |= GB_LINKCABLE;


	while (true)
	{
		EnterCriticalSection(&csTerminate);
		if (Terminating != TERMINATING_STOP)
		{
			GameBoys.UpdateKeys(this);
			GameBoys.UpdateKeys(pLinkGameBoy);
		}
		LeaveCriticalSection(&csTerminate);

		do
		{
			if ((signed)SerialTicks > 0 && (signed)pLinkGameBoy->SerialTicks > 0)
			{
				if (SerialTicks > pLinkGameBoy->SerialTicks)
				{
					SerialTicks -= pLinkGameBoy->SerialTicks;
					pLinkGameBoy->SerialTicks = 0;
				}
				else
				{
					pLinkGameBoy->SerialTicks -= SerialTicks;
					SerialTicks = 0;
				}
			}
			SerialTicks += 128;
			DebugMainLoop();
			pLinkGameBoy->SerialTicks += 128 - SerialTicks;
			if ((signed)pLinkGameBoy->SerialTicks > 0)
			{
				pLinkGameBoy->DebugMainLoop();
			}

			if (Flags & GB_INVALIDOPCODE)
			{
				STOPEMULATION2;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
				return;
			}

			if (Flags & GB_ERROR)
			{
				STOPEMULATION2;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}

			if (pLinkGameBoy->Flags & GB_INVALIDOPCODE)
			{
				STOPEMULATION2;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pLinkGameBoy->hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
				return;
			}

			if (pLinkGameBoy->Flags & GB_ERROR)
			{
				STOPEMULATION2;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pLinkGameBoy->hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}

			if ((Flags & GB_SERIALBIT) || (pLinkGameBoy->Flags & GB_SERIALBIT))
			{
				Flags &= ~GB_SERIALBIT;
				pLinkGameBoy->Flags &= ~GB_SERIALBIT;

				if (FF00_C(0x02) & 0x80)
				{
					if ((FF00_C(0x02) & 1) || ((pFF00_C(pLinkGameBoy, 0x02) & 0x81) == 0x81))
					{
						if (SerialBit < 8)
						{
							if (pFF00_C(pLinkGameBoy, 0x02) & 0x80)
							{
								b = FF00_C(0x01) >> 7;
								FF00_C(0x01) = (FF00_C(0x01) << 1) | (pFF00_C(pLinkGameBoy, 0x01) >> 7);
								pFF00_C(pLinkGameBoy, 0x01) = (pFF00_C(pLinkGameBoy, 0x01) << 1) | b;
								MemStatus_CPU[0x8F01] |= MEM_CHANGED;
								pLinkGameBoy->MemStatus_CPU[0x8F01] |= MEM_CHANGED;
							}
							else
							{
								FF00_C(0x01) <<= 1;
							}
							if (++SerialBit == 8)
							{
								FF00_C(0x02) &= ~0x80;
								FF00_C(0x0F) |= 0x08;
								MemStatus_CPU[0x8F02] |= MEM_CHANGED;
								MemStatus_CPU[0x8F0F] |= MEM_CHANGED;
							}
							if (++pLinkGameBoy->SerialBit == 8)
							{
								pFF00_C(pLinkGameBoy, 0x02) &= ~0x80;
								pFF00_C(pLinkGameBoy, 0x0F) |= 0x08;
								pLinkGameBoy->MemStatus_CPU[0x8F02] |= MEM_CHANGED;
								pLinkGameBoy->MemStatus_CPU[0x8F0F] |= MEM_CHANGED;
							}
						}
						else
						{
							SerialBit = 0;
							pLinkGameBoy->SerialBit = 0;
						}
					}
				}
				else if ((pFF00_C(pLinkGameBoy, 0x02) & 0x81) == 0x81)
				{
					if (pLinkGameBoy->SerialBit < 8)
					{
						pFF00_C(pLinkGameBoy, 0x01) <<= 1;
						pLinkGameBoy->MemStatus_CPU[0x8F01] |= MEM_CHANGED;
						if (++pLinkGameBoy->SerialBit == 8)
						{
							pFF00_C(pLinkGameBoy, 0x02) &= ~0x80;
							pFF00_C(pLinkGameBoy, 0x0F) |= 0x08;
							pLinkGameBoy->MemStatus_CPU[0x8F02] |= MEM_CHANGED;
							pLinkGameBoy->MemStatus_CPU[0x8F0F] |= MEM_CHANGED;
						}
					}
					else
					{
						SerialBit = 0;
						pLinkGameBoy->SerialBit = 0;
					}
				}
			}
		}
		while (nCycles < 35112 * 4);


		if (pAVISoundBuffer)
		{
			WriteAVI();
		}
		if (pLinkGameBoy->pAVISoundBuffer)
		{
			pLinkGameBoy->WriteAVI();
		}


		Delay();

		RefreshScreen();
		pLinkGameBoy->RefreshScreen();

		if (Terminating)
		{
			EnterCriticalSection(&csTerminate);

			switch (Terminating)
			{
			case TERMINATING_STOP:
				LeaveCriticalSection(&csTerminate);
				STOPEMULATION2;
				return;

			case TERMINATING_PAUSE:
				LeaveCriticalSection(&csTerminate);
				PAUSEEMULATION2;
				while (true)
				{
					while (Terminating == TERMINATING_PAUSE)
					{
						Sleep(0);
					}
					EnterCriticalSection(&csTerminate);
					if (Terminating == TERMINATING_STOP)
					{
						LeaveCriticalSection(&csTerminate);
						STOPEMULATION2;
						return;
					}
					if (Terminating == TERMINATING_FALSE)
					{
						LeaveCriticalSection(&csTerminate);
						break;
					}
				}
				RESUMEEMULATION2;
				break;

			default:
				LeaveCriticalSection(&csTerminate);
			}
		}
	}
}



void CGameBoy::DebugLoop()
{
	char			szBuffer[0x100];
	BOOL			cs;


	if (Settings.LinkCable)
	{
		EnterCriticalSection(&csGameBoy);
		if (pLinkGameBoy)
		{
			LeaveCriticalSection(&csGameBoy);
			return;
		}
		if (pLinkGameBoy = GameBoys.GetPlayer2(true))
		{
			if (!pLinkGameBoy->pLinkGameBoy && !pLinkGameBoy->IsEmulating())
			{
				pLinkGameBoy->pLinkGameBoy = this;
				LeaveCriticalSection(&csGameBoy);

				SendMessage(hWnd, WM_APP_INITGFX, 0, (LPARAM)pLinkGameBoy);

				LinkDebugLoop();

				EnterCriticalSection(&csTerminate);
				if (Terminating != TERMINATING_STOP)
				{
					EnterCriticalSection(&csGameBoy);
					cs = true;
				}
				else
				{
					cs = false;
				}
				LeaveCriticalSection(&csTerminate);
				pLinkGameBoy->pLinkGameBoy = NULL;
				pLinkGameBoy->Release();
				pLinkGameBoy = NULL;
				if (cs)
				{
					LeaveCriticalSection(&csGameBoy);
				}
				return;
			}
			pLinkGameBoy->Release();
			pLinkGameBoy = NULL;
		}
		LeaveCriticalSection(&csGameBoy);
	}


	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}
	PrepareEmulation(true);
	MemoryFlags = DisAsmFlags = 0;

	while (true)
	{
		GameBoys.UpdateKeys(this);

		DebugMainLoop();
		if (Flags & GB_ERROR)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			if (Flags & GB_INVALIDOPCODE)
			{
				MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
			}
			return;
		}


		if (pAVISoundBuffer)
		{
			WriteAVI();
		}

		Delay();

		RefreshScreen();

		if (Terminating)
		{
			EnterCriticalSection(&csTerminate);

			switch (Terminating)
			{
			case TERMINATING_STOP:
				LeaveCriticalSection(&csTerminate);
				STOPEMULATION;
				return;

			case TERMINATING_PAUSE:
				LeaveCriticalSection(&csTerminate);
				PAUSEEMULATION;
				while (true)
				{
					while (Terminating == TERMINATING_PAUSE)
					{
						Sleep(0);
					}
					EnterCriticalSection(&csTerminate);
					if (Terminating == TERMINATING_STOP)
					{
						LeaveCriticalSection(&csTerminate);
						STOPEMULATION;
						return;
					}
					if (Terminating == TERMINATING_FALSE)
					{
						LeaveCriticalSection(&csTerminate);
						break;
					}
				}
				RESUMEEMULATION;
				break;

			default:
				LeaveCriticalSection(&csTerminate);
			}
		}
	}
}



void CGameBoy::StepLoop(EMULATIONINFO *pEmulationInfo)
{
	WORD			pByte;
	char			szBuffer[0x100];


	switch (pEmulationInfo->Flags)
	{
	case EMU_STEPINTO:
		pByte = Reg_PC;
		break;

	case EMU_STEPOUT:
		pByte = Reg_SP;
		break;
	}


	PrepareEmulation(true);
	MemoryFlags = DisAsmFlags = 0;

	Flags |= GB_EXITLOOP;
	DebugMainLoop();
	if (Flags & GB_INVALIDOPCODE)
	{
		STOPEMULATION;
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
		return;
	}

	if (Flags & GB_ERROR)
	{
		STOPEMULATION;
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
		PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
		return;
	}

	switch (pEmulationInfo->Flags)
	{
	case EMU_STEPINTO:
		if (pByte != Reg_PC)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			return;
		}
		break;

	case EMU_RUNTO:
		if (pEmulationInfo->RunToOffset == Reg_PC)
		{
			if (Reg_PC >= 0x4000 && Reg_PC < 0x8000)
			{
				if (pEmulationInfo->RunToBank != ActiveRomBank)
				{
					break;
				}
			}
			if (Reg_PC >= 0xA000 && Reg_PC < 0xC000)
			{
				if (pEmulationInfo->RunToBank != ActiveRamBank)
				{
					break;
				}
			}
			if (Reg_PC >= 0xD000 && Reg_PC < 0xE000 && Flags & GB_ROM_COLOR)
			{
				if (FF00_C(0x70) & 7)
				{
					if (pEmulationInfo->RunToBank != (FF00_C(0x70) & 7))
					{
						break;
					}
				}
				else
				{
					if (pEmulationInfo->RunToBank != 1)
					{
						break;
					}
				}
			}
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			return;
		}
		break;

	case EMU_STEPOUT:
		if (pByte < Reg_SP)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			return;
		}
		break;
	}

	if (Settings.SoundEnabled)
	{
		RestoreSound();
	}
	if (Settings.Direct3D)
	{
		if (!m_pd3dd)
		{
			InitGfx(&m_pd3dd);
		}
	}

	while (true)
	{
		Flags |= GB_EXITLOOP;
		DebugMainLoop();
		if (Flags & GB_INVALIDOPCODE)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			MessageBox(hMsgParent, String(IDS_EMU_INVALIDOPCODE), "Game Lad", MB_OK | MB_ICONWARNING);
			return;
		}

		if (Flags & GB_ERROR)
		{
			STOPEMULATION;
			SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
			PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
			return;
		}

		switch (pEmulationInfo->Flags)
		{
		case EMU_STEPINTO:
			if (pByte != Reg_PC)
			{
				STOPEMULATION;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}
			break;

		case EMU_RUNTO:
			if (pEmulationInfo->RunToOffset == Reg_PC)
			{
				if (Reg_PC >= 0x4000 && Reg_PC < 0x8000)
				{
					if (pEmulationInfo->RunToBank != ActiveRomBank)
					{
						break;
					}
				}
				if (Reg_PC >= 0xA000 && Reg_PC < 0xC000)
				{
					if (pEmulationInfo->RunToBank != ActiveRamBank)
					{
						break;
					}
				}
				if (Reg_PC >= 0xD000 && Reg_PC < 0xE000 && Flags & GB_ROM_COLOR)
				{
					if (FF00_C(0x70) & 7)
					{
						if (pEmulationInfo->RunToBank != (FF00_C(0x70) & 7))
						{
							break;
						}
					}
					else
					{
						if (pEmulationInfo->RunToBank != 1)
						{
							break;
						}
					}
				}
				STOPEMULATION;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}
			break;

		case EMU_STEPOUT:
			if (pByte < Reg_SP)
			{
				STOPEMULATION;
				SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hGBWnd, 0);
				SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}
			break;
		}


		/*while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_CLOSE || msg.message == WM_QUIT)
			{
				//Put back WM_QUIT to message que
				if (msg.message == WM_QUIT)
				{
					PostThreadMessage(ThreadId, WM_QUIT, msg.wParam, msg.lParam);
				}
				STOPEMULATION;
				PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				return;
			}
			DispatchMessage(&msg);
		}*/
		if (Terminating)
		{
			EnterCriticalSection(&csTerminate);

			switch (Terminating)
			{
			case TERMINATING_STOP:
				LeaveCriticalSection(&csTerminate);
				STOPEMULATION;
				return;

			case TERMINATING_PAUSE:
				LeaveCriticalSection(&csTerminate);
				PAUSEEMULATION;
				while (true)
				{
					while (Terminating == TERMINATING_PAUSE)
					{
						Sleep(0);
					}
					EnterCriticalSection(&csTerminate);
					if (Terminating == TERMINATING_STOP)
					{
						LeaveCriticalSection(&csTerminate);
						STOPEMULATION;
						return;
					}
					if (Terminating == TERMINATING_FALSE)
					{
						LeaveCriticalSection(&csTerminate);
						break;
					}
				}
				RESUMEEMULATION;
				break;

			default:
				LeaveCriticalSection(&csTerminate);
			}
		}


		if (FF00_C(0x44) == 0x90 && (FF00_C(0x40) & 0x80))
		{
			GameBoys.UpdateKeys(this);

			Delay();

			RefreshScreen();
			if (pAVISoundBuffer)
			{
				WriteAVI();
			}
		}
	}
}



DWORD CGameBoy::ThreadProc()
{
	MSG				msg;
	EMULATIONINFO	EmulationInfo;


	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(hStartStopEvent);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch (msg.message)
		{
		case WM_COMMAND:
			switch (LOWORD(msg.wParam))
			{
			case ID_EMULATION_STARTDEBUG:
				if (Emulating)
				{
					SetEvent(hStartStopEvent);
					break;
				}
				Emulating = true;
				SetEvent(hStartStopEvent);
				DebugLoop();
				Emulating = false;
				break;

			case ID_EMULATION_EXECUTE:
				if (Emulating)
				{
					SetEvent(hStartStopEvent);
					break;
				}
				Emulating = true;
				SetEvent(hStartStopEvent);
				ExecuteLoop();
				Emulating = false;
				break;
			}
			break;

		case WM_APP_STEP:
			if (Emulating)
			{
				SetEvent(hStartStopEvent);
				break;
			}
			Emulating = true;
			if (msg.lParam)
			{
				EmulationInfo.Flags = ((EMULATIONINFO *)msg.lParam)->Flags;
				EmulationInfo.RunToBank = ((EMULATIONINFO *)msg.lParam)->RunToBank;
				EmulationInfo.RunToOffset = ((EMULATIONINFO *)msg.lParam)->RunToOffset;
			}
			SetEvent(hStartStopEvent);
			StepLoop(&EmulationInfo);
			Emulating = false;
			break;

		default:
			DispatchMessage(&msg);
		}
	}

	hThread = NULL;

	return msg.wParam;
}



DWORD WINAPI GameBoyThreadProc(void *pGameBoy)
{
	return ((CGameBoy *)pGameBoy)->ThreadProc();
}



BOOL CGameBoy::StartThread()
{
	//Create a new thread if it doesn't exist
	if (!hThread)
	{
		ResetEvent(hStartStopEvent);

		if (!(hThread = CreateThread(NULL, 0, GameBoyThreadProc, this, 0, &ThreadId)))
		{
			return true;
		}

		//Wait for thread to initialize
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	return false;
}



BOOL CGameBoy::StartDebug()
{
	EnterCriticalSection(&csGameBoy);

	if (Settings.Direct3D)
	{
		if (!m_pd3dd)
		{
			InitGfx(&m_pd3dd);
		}
	}


	//Make sure a thread is running
	if (StartThread())
	{
		LeaveCriticalSection(&csGameBoy);
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_STARTDEBUG, 0);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	LeaveCriticalSection(&csGameBoy);

	return false;
}



BOOL CGameBoy::Execute()
{
	EnterCriticalSection(&csGameBoy);


	//Make sure a thread is running
	if (StartThread())
	{
		LeaveCriticalSection(&csGameBoy);
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_COMMAND, ID_EMULATION_EXECUTE, 0);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	LeaveCriticalSection(&csGameBoy);

	return false;
}



BOOL CGameBoy::Step(EMULATIONINFO *pEmulationInfo)
{
	EnterCriticalSection(&csGameBoy);

	/*if (Settings.Direct3D)
	{
		if (!m_pd3dd)
		{
			m_pd3dd = InitGfx();
		}
	}*/


	//Make sure a thread is running
	if (StartThread())
	{
		LeaveCriticalSection(&csGameBoy);
		return true;
	}

	if (!Emulating)
	{
		ResetEvent(hStartStopEvent);
		PostThreadMessage(ThreadId, WM_APP_STEP, 0, (LPARAM)pEmulationInfo);

		//Wait for emulation to start before leaving critical section
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}

	LeaveCriticalSection(&csGameBoy);

	return false;
}



void CGameBoy::Resume()
{
	CGameBoy		*pGameBoy;


	EnterCriticalSection(&csTerminate);

	/*if (Terminating == TERMINATING_STOP || (Terminating == TERMINATING_FALSE && Emulating))
	{
		LeaveCriticalSection(&csTerminate);
		return;
	}*/
	if (hThread)
	{
		if (Terminating != TERMINATING_PAUSE)
		{
			LeaveCriticalSection(&csTerminate);
			return;
		}

		if (Settings.Direct3D)
		{
			if (!m_pd3dd)
			{
				InitGfx(&m_pd3dd);
			}
		}

		ResetEvent(hStartStopEvent);
		/*if (Terminating != TERMINATING_PAUSE)
		{
			PostThreadMessage(ThreadId, WM_APP_RESUME, 0, 0);
		}*/

		Terminating = TERMINATING_FALSE;

		LeaveCriticalSection(&csTerminate);

		if (GetCurrentThreadId() != ThreadId)
		{
			//Wait for emulation to start
			WaitForSingleObject(hStartStopEvent, INFINITE);
		}
	}
	else
	{
		Terminating = TERMINATING_FALSE;
		if (pGameBoy = pLinkGameBoy)
		{
			pGameBoy->Resume();
		}
		LeaveCriticalSection(&csTerminate);
	}
}



//Stops emulation, but does not release DirectX
void CGameBoy::Pause()
{
	HANDLE			hTempThread;
	CGameBoy		*pGameBoy;


	if (hTempThread = hThread)
	{
		EnterCriticalSection(&csTerminate);

		if (Terminating != TERMINATING_FALSE || !Emulating)
		{
			LeaveCriticalSection(&csTerminate);
			return;
		}

		Terminating = TERMINATING_PAUSE;
		ResetEvent(hStartStopEvent);

		LeaveCriticalSection(&csTerminate);

		//Wait for emulation to stop
		WaitForSingleObject(hStartStopEvent, INFINITE);
	}
	else
	{
		if (pGameBoy = pLinkGameBoy)
		{
			if (Settings.LinkCable && pGameBoy->IsEmulating())
			{
				pGameBoy->Pause();
			}
		}
	}
}



void CGameBoy::Stop()
{
	MSG			msg;
	HANDLE		hTempThread;
	CGameBoy	*pGameBoy;


	if (hTempThread = hThread)
	{
		//If emulating, just stop it.
		//If not, unload thread.

		//Emulation is restarted faster (i.e. step commands)

		if (Emulating)
		{
			EnterCriticalSection(&csTerminate);
			if (Terminating == TERMINATING_STOP)
			{
				LeaveCriticalSection(&csTerminate);
				return;
			}
			Terminating = TERMINATING_STOP;
			LeaveCriticalSection(&csTerminate);
			if (GetCurrentThreadId() != ThreadId)
			{
				while (Emulating)
				{
					Sleep(0);
				}
				while (PeekMessage(&msg, NULL, WM_APP_RELEASEGFX, WM_APP_RELEASEGFX, true))
				{
					DispatchMessage(&msg);
				}
				if (GameBoys.GetActive(false) == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
			}
			EnterCriticalSection(&csTerminate);
			Terminating = TERMINATING_FALSE;
			LeaveCriticalSection(&csTerminate);
		}
		else
		{
			EnterCriticalSection(&csTerminate);
			if (Terminating == TERMINATING_STOP)
			{
				LeaveCriticalSection(&csTerminate);
				return;
			}
			Terminating = TERMINATING_STOP;
			LeaveCriticalSection(&csTerminate);
			PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
			if (GetCurrentThreadId() != ThreadId)
			{
				WaitForSingleObject(hTempThread, INFINITE);
				while (PeekMessage(&msg, NULL, WM_APP_RELEASEGFX, WM_APP_RELEASEGFX, true))
				{
					DispatchMessage(&msg);
				}
				if (GameBoys.GetActive(false) == this)
				{
					PostMessage(hWnd, WM_APP_REFRESHDEBUG, 0, 0);
				}
			}
			EnterCriticalSection(&csTerminate);
			Terminating = TERMINATING_FALSE;
			LeaveCriticalSection(&csTerminate);
		}
	}
	else
	{
		if (pGameBoy = pLinkGameBoy)
		{
			if (pGameBoy->IsEmulating())
			{
				pGameBoy->Stop();
			}
		}
	}
}



BOOL CGameBoy::IsEmulating()
{
	CGameBoy		*pGameBoy;


	if (pGameBoy = pLinkGameBoy)
	{
		if (pGameBoy->Emulating)
		{
			return true;
		}
	}

	return Emulating;
}



/*void CGameBoy::InsertLinkCable()
{
	if (pLinkGameBoy)
	{
		Stop();
		Resume();
		if (!Settings.LinkCable)
		{
			pLinkGameBoy->Resume();
		}
	}

	return;
}*/



BOOL CGameBoy::HasBattery()
{
	return SaveRamSize ? true : false;
}



BOOL CGameBoy::SwitchRomBank(BYTE Bank)
{
	if (Emulating || Bank > MaxRomBank)
	{
		return true;
	}

	ActiveRomBank = Bank;

	MEM[0x4] = &MEM_ROM[0x0000 + 0x4000 * ActiveRomBank];
	MEM[0x5] = &MEM_ROM[0x1000 + 0x4000 * ActiveRomBank];
	MEM[0x6] = &MEM_ROM[0x2000 + 0x4000 * ActiveRomBank];
	MEM[0x7] = &MEM_ROM[0x3000 + 0x4000 * ActiveRomBank];

	return false;
}



BOOL CGameBoy::SwitchRamBank(BYTE Bank)
{
	if (Emulating || Bank > MaxRamBank)
	{
		return true;
	}

	ActiveRamBank = Bank;

	MEM[0xA] = &MEM_RAM[0x0000 + 0x2000 * ActiveRamBank];
	MEM[0xB] = &MEM_RAM[0x1000 + 0x2000 * ActiveRamBank];

	return false;
}



BOOL CGameBoy::SwitchVBK(BYTE Bank)
{
	if (Emulating || Bank > 1)
	{
		return true;
	}

	FF00_C(0x4F) = (FF00_C(0x4F) & ~1) | (Bank & 1);

	MEM[0x8] = &MEM_VRAM[0x0000 + 0x2000 * (FF00_C(0x4F) & 1)];
	MEM[0x9] = &MEM_VRAM[0x1000 + 0x2000 * (FF00_C(0x4F) & 1)];

	return false;
}



BOOL CGameBoy::SwitchSVBK(BYTE Bank)
{
	if (Emulating || Bank > 7 || Bank < 1)
	{
		return true;
	}

	FF00_C(0x70) = (FF00_C(0x70) & ~7) | (Bank & 7);

	MEM[0xD] = &MEM_CPU[0x1000 + 0x1000 * (FF00_C(0x70) & 7)];

	return false;
}



void CGameBoy::ClearDebugRunInfo()
{
	DWORD	pByte, RamSize;


	Flags &= ~GB_DEBUGRUNINFO;

	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		break;

	case 2:
		RamSize = 0x2000;
		break;

	case 3:
		RamSize = 0x8000;
		break;

	case 4:
		RamSize = 0x20000;
		break;

	default:
		RamSize = 0;
		break;
	}

	//External Battery RAM
	for (pByte = 0; pByte < RamSize; pByte++)
	{
		MemStatus_RAM[pByte] |= MEM_READ;
	}

	//Internal RAM
	for (pByte = 0; pByte < 0x8000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_READ;
	}
	//HiRAM
	for (pByte = 0x8F80; pByte < 0x9000; pByte++)
	{
		MemStatus_CPU[pByte] |= MEM_READ;
	}
}



void CGameBoy::PrepareEmulation(BOOL Debug)
{
	DWORD			pByte, RamSize;


	Flags &= ~(GB_ERROR | GB_INVALIDOPCODE | GB_LINKCABLE);
	BatteryAvailable = true;
	DirectionKeys = 0;
	Buttons = 0;

	if (Debug)
	{
		Flags |= GB_DEBUG;

		MemStatus[0x4] = pMemStatus_ROM(ActiveRomBank * 0x4000);
		MemStatus[0x5] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x1000);
		MemStatus[0x6] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x2000);
		MemStatus[0x7] = pMemStatus_ROM(ActiveRomBank * 0x4000 + 0x3000);

		MemStatus[0x8] = &MemStatus_VRAM[(FF00_C(0x4F) & 1) * 0x2000];
		MemStatus[0x9] = &MemStatus_VRAM[(FF00_C(0x4F) & 1) * 0x2000 + 0x1000];
		if (Flags & GB_RAMENABLE && MemStatus_RAM)
		{
			MemStatus[0xA] = &MemStatus_RAM[ActiveRamBank * 0x2000];
			if (MEM_ROM[0x0149] != 1)
			{
				MemStatus[0xB] = &MemStatus_RAM[ActiveRamBank * 0x2000 + 0x1000];
			}
			else
			{
				MemStatus[0xB] = ZeroStatus;
			}
		}
		else
		{
			MemStatus[0xA] = ZeroStatus;
			MemStatus[0xB] = ZeroStatus;
		}
		MemStatus[0xC] = &MemStatus_CPU[0x0000];
		if (FF00_C(0x70) != 0)
		{
			MemStatus[0xD] = &MemStatus_CPU[(FF00_C(0x70) & 7) * 0x1000];
		}
		else
		{
			MemStatus[0xD] = &MemStatus_CPU[0x1000];
		}
	}
	else
	{
		Flags &= ~GB_DEBUG;
		if (Flags & GB_DEBUGRUNINFO)
		{
			ClearDebugRunInfo();
		}
	}

	//VRAM
	for (pByte = 0; pByte < sizeof(MemStatus_VRAM); pByte++)
	{
		MemStatus_VRAM[pByte] &= ~MEM_CHANGED;
	}

	switch (MEM_ROM[0x0149])
	{
	case 1:
		RamSize = 0x800;
		break;

	case 2:
		RamSize = 0x2000;
		break;

	case 3:
		RamSize = 0x8000;
		break;

	case 4:
		RamSize = 0x20000;
		break;

	default:
		RamSize = 0;
		break;
	}

	//External Battery RAM
	for (pByte = 0; pByte < RamSize; pByte++)
	{
		MemStatus_RAM[pByte] &= ~MEM_CHANGED;
	}

	for (pByte = 0; pByte < sizeof(MemStatus_CPU); pByte++)
	{
		MemStatus_CPU[pByte] &= ~MEM_CHANGED;
	}

	GameBoys.UpdateKeys(this);
	if (pLinkGameBoy)
	{
		GameBoys.UpdateKeys(pLinkGameBoy);
	}

	SetRTCReg(this);

	SetStartDelay();
}



void CGameBoy::SetStartDelay()
{
	nCycles = 0;
	//DelayTime = 0;
	SkipNextFrame = false;
	FramesSkipped = 0;
	QueryPerformanceCounter(&LastTimerCount);
}


void CGameBoy::Delay()
{
	LONGLONG		StopTimerCount;
	LARGE_INTEGER	CurrentTimerCount;


	if (FastFwd)
	{
		SetStartDelay();
		return;
	}

	StopTimerCount = nCycles * TimerFrequency.QuadPart;
	nCycles = 0;
	//CurrentTimerCount.QuadPart = StopTimerCount % 35340;
	StopTimerCount = StopTimerCount / 35340 /*+ DelayTime*/;
	//DelayTime = CurrentTimerCount.QuadPart;

	QueryPerformanceCounter(&CurrentTimerCount);
	if (CurrentTimerCount.QuadPart - LastTimerCount.QuadPart < StopTimerCount)
	{
		SkipNextFrame = false;
		do
		{
			QueryPerformanceCounter(&CurrentTimerCount);
		}
		while (CurrentTimerCount.QuadPart - LastTimerCount.QuadPart < StopTimerCount);

		LastTimerCount = CurrentTimerCount;
	}
	else
	{
		if (FrameSkip == 0xFF)
		{
			SkipNextFrame = true;
			if (FramesToSkip)
			{
				LastTimerCount.QuadPart += StopTimerCount;
			}
			else
			{
				LastTimerCount = CurrentTimerCount;
			}
		}
		else
		{
			LastTimerCount = CurrentTimerCount;
		}
	}
}



const int	Volume[16] = {0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 127};

void __declspec(naked) CGameBoy::MainLoop()
{
	__asm
	{
		push	ebp
		push	esi
		push	edi
		push	edx
		push	ebx

		mov		esi, dword ptr [ecx + Offset_Flags]

ContinueLoop:
		//esi	= Flags
		test	esi, GB_HALT
		jz		NotHalt

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		mov		eax, 1
		//eax	= Ticks
		inc		ebx
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free
		jmp		HaltComplete

NotHalt:
		//esi	= Flags
		mov		dword ptr [ecx + Offset_Flags], esi

		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		//edx	= PC
		call	ReadMem
		//al	= mem[PC]
		and		eax, 0xFF
		call	dword ptr [OpCodes + 4 * eax]
		//eax	= Ticks

		mov		esi, dword ptr [ecx + Offset_Flags]

		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jnz		FastCPU
		shl		al, 1
FastCPU:

		test	esi, GB_EXITLOOPDIRECTLY
		jnz		ExitLoop

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		add		ebx, eax
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free

HaltComplete:
		//eax	= Ticks
		//esi	= Flags





		//LCD
		mov		bl, byte ptr [ecx + Offset_LCD_Ticks]
		//bl	= LCD_Ticks
		cmp		bl, al
		ja		LCD_NotEnoughTicks

		mov		dx, word ptr [ecx + FF00_ASM + 0x40]
		//dl	= FF40
		//dh	= FF41
		test	dl, 0x80
		jz		LCD_Off

		test	dh, 2
		jnz		LCD_Mode2or3

		test	dh, 1
		jnz		LCD_VBlank



//LCD_HBlank:
		mov		bh, byte ptr [ecx + FF00_ASM + 0x44]
		sub		bl, al

		cmp		bh, 143
		ja		LCD_ExitHBlank

		add		bl, 40
		inc		bh
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		//bh	free
		//ebx	free

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free

		jmp		LCD_Done

LCD_ExitHBlank:
		add		bl, 228
		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		//bh	= [FF0F]
		mov		byte ptr [ecx + FF00_ASM + 0x44], 145
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		and		dh, ~3
		or		dh, 1
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		or		bh, 1
		or		esi, GB_EXITLOOP

		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

		test	dh, 0x10
		jnz		LCD_SetLCDC01

		test	dh, 0x40
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bl, 144
		jne		LCD_Done

LCD_SetLCDC01:
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

		jmp		LCD_Done



LCD_VBlank:
		mov		bh, [ecx + FF00_ASM + 0x44]
		sub		bl, al

		//bh	= [FF44]
		test	bh, bh
		jz		LCD_ExitVBlank

		add		bl, 228
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		cmp		bh, 154
		ja		LCD_Line0

		inc		bh
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bh, bl
		//bl	free
		//bh	free
		//ebx	free
		jne		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		jmp		LCD_Done

LCD_Line0:
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0

		test	dh, 0x40
		//dh	free
		jz		LCD_Done

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		test	bl, bl
		jnz		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		jmp		LCD_Done

LCD_ExitVBlank:
		add		bl, 40
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		mov		byte ptr [ecx + Offset_WindowY2], 0

		test	dh, 0x20
		jz		LCD_Done

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

		jmp		LCD_Done

LCD_Mode2or3:
		test	dh, 1
		jz		LCD_OAM



//LCD_ToLCD:
		add		bl, 104
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		//ebx	free

		and		dh, ~3
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		test	dh, 0x08
		jz		LCD_NoSetLCDC00

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

LCD_NoSetLCDC00:
		//dh	free
		//edx	free

		test	esi, GB_HDMA
		jz		LCD_NoHDMA

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		//bl	= [FF4F]
		and		ebx, 1
		shl		ebx, 13

		mov		ebp, dword ptr [ecx + FF00_ASM + 0x51]
		//ebp	= [FF51-52]
		mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
		//edi	= [FF53-54]
		rol		bp, 8
		rol		di, 8
		and		ebp, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		mov		eax, ebp
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah

		mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
		//bl	= [FF55]

		mov		eax, ebp
		shr		ebp, 12
		and		eax, 0x0FFF
		mov		ebp, [ecx + Offset_MEM + ebp * 4]

		dec		bl	//[FF55]--

		mov		edx, [ebp + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		mov		edx, [ebp + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		mov		edx, [ebp + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		mov		edx, [ebp + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx

		mov		byte ptr [ecx + FF00_ASM + 0x55], bl
		test	bl, 0x80
		//bl	free
		jz		HDMA_NotFinished
		and		esi, ~GB_HDMA
HDMA_NotFinished:

LCD_NoHDMA:
		jmp		LCD_Done



LCD_OAM:
		//eax	= Ticks
		//esi	= Flags
		//dl	= [FF40]
		//dh	= [FF41]

		sub		bl, al
		add		bl, 84
		or		dh, 3
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		mov		bx, word ptr [ecx + FF00_ASM + 0x44]
		//bl	= [FF44]
		//bh	= [FF45]

		test	dh, 0x40
		jz		LCD_NoLYC

		cmp		bl, bh
		jne		LCD_NoLYC

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh

LCD_NoLYC:
		//eax	= Ticks
		//esi	= Flags
		//bl	= [FF44]

		cmp		bl, 144
		ja		LCD_Done

		mov		bh, byte ptr [ecx + Offset_FramesToSkip]
		test	bh, bh
		jnz		LCD_Done


		//Set pointer to bitmap
		movzx	edi, bl
		//bl	free
		imul	edi, (160 + 14) * 2
		add		edi, dword ptr [ecx + Offset_pGBBitmap]

		push	eax
		push	ecx
		push	esi

		push	ebp
		mov		ebp, esp
		sub		esp, 28 + 2 * (160 + 14)

#define		LineX		(ebp - 4)
#define		LCD_X		(ebp - 5)
#define		WndX		(ebp - 6)
#define		Sprites		(ebp - 26)
#define		Line		(ebp - 28 - 2 * (160 + 14))

		push	edi

		mov		bl, byte ptr [ecx + FF00_ASM + 0x43]
		mov		byte ptr [LCD_X], bl

		lea		ebx, [Line]
		mov		dword ptr [LineX], ebx

		//WORD			Sprites[10];
				/*	BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (FF00_C(0x44) < 144)
					{
						BYTE	WndX;*/


		mov		dword ptr [Line], 0
		mov		dword ptr [Line + 4], 0
		mov		dword ptr [Line + 8], 0
		mov		dword ptr [Line + 12], 0


		test	esi, GB_ROM_COLOR
		jz		Gfx_NoColor


		/////////////
		//Background
		/////////////

							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		Bg_WndXOK
Bg_NoWnd:
							mov		bl, 160
Bg_WndXOK:
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
Bg_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Bg_9000
Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Bg_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Bg_NoVFlip
							mov		al, 14
							sub		al, ah
Bg_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Bg_HFlip

							shr		bl, 7
							or		bl, 2
Bg_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shl		dx, 1
							jnc		Bg_NoLow
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoLow:
							test	dh, 1
							jz		Bg_NoHigh
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoHigh:

							add		dword ptr [LineX], 2

							/*mov		eax, dword ptr [ecx + Offset_pPalette]
							push	ecx
							mov		cx, word ptr [esi + Offset_BGP]
							and		ecx, 0x7FFF
							mov		ax, word ptr [eax + ecx * 2]
							pop		ecx*/
							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_HFlip:
							shr		bl, 7
							or		bl, 2
Bg_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shr		edx, 1
							jnc		Bg_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoLow_HFlip:
							test	dl, 0x80
							jz		Bg_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel_HFlip

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_Complete:
							pop		edx
							pop		eax
Bg_NoDraw:


							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Wnd_9000
Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Wnd_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Wnd_NoVFlip
							mov		al, 14
							sub		al, ah
Wnd_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Wnd_HFlip

							shr		bl, 7
							or		bl, 2
Wnd_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shl		dx, 1
							jnc		Wnd_NoLow
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoLow:
							test	dh, 1
							jz		Wnd_NoHigh
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_HFlip:
							shr		bl, 7
							or		bl, 2
Wnd_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shr		edx, 1
							jnc		Wnd_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoLow_HFlip:
							test	dl, 0x80
							jz		Wnd_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel_HFlip

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_Complete:
							pop		edx
							pop		eax
Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		Spr_On
							pop		edi
							jmp		Spr_NoDraw

Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		Spr_FindNextSprite1 //Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		Spr_FindNextSprite1	//Sprite outside right side of screen

							mov		byte ptr [Sprites + esi], dl
							inc		esi
							cmp		esi, 10
							je		Spr_Draw

Spr_FindNextSprite1:
							inc		edx
							xor		dh, dh
							cmp		dl, 0xA0 / 4
							jne		Spr_FindNextSprite


Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		Spr_NoDraw

Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + esi - 1]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							dec		al
							mov		dh, bh
							and		dh, 8
							shr		dh, 2
							shl		edx, 4
							test	bh, 0x40
							jnz		Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		Spr_Draw_VFlipOk
Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 2) * 2

							push	edi
							add		edi, eax
							mov		dword ptr [LineX], eax
							add		dword ptr [LineX], 2

							mov		esi, ebx
							and		esi, 0x0700
							shr		esi, 5
							add		esi, ecx

							test	bh, 0x20
							jnz		Spr_HFlip

Spr_NextPixel:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel
							test	bh, 0x80
							jnz		Spr_NoDrawPixel
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel
Spr_NoDrawPixel:
							shl		dl, 1
							shl		dh, 1
							jmp		Spr_NextPixel
Spr_DrawPixel:
							push	ebx
							xor		ebx, ebx

							shl		dh, 1
							jnc		Spr_NoLow
							mov		ebx, 4
Spr_NoLow:
							shl		dl, 1
							jnc		Spr_NoHigh
							add		ebx, 2
Spr_NoHigh:

							test	ebx, ebx
							jz		Spr_NextPixel1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel1:
							pop		ebx

							jmp		Spr_NextPixel

Spr_HFlip:
Spr_NextPixel_HFlip:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel_HFlip
							test	bh, 0x80
							jnz		Spr_NoDrawPixel_HFlip
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel_HFlip
Spr_NoDrawPixel_HFlip:
							shr		dl, 1
							shr		dh, 1
							jmp		Spr_NextPixel_HFlip
Spr_DrawPixel_HFlip:
							push	ebx
							xor		ebx, ebx

							shr		dl, 1
							jnc		Spr_NoLow_HFlip
							mov		ebx, 2
Spr_NoLow_HFlip:
							shr		dh, 1
							jnc		Spr_NoHigh_HFlip
							add		ebx, 4
Spr_NoHigh_HFlip:

							test	ebx, ebx
							jz		Spr_NextPixel_HFlip1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel_HFlip1:
							pop		ebx

							jmp		Spr_NextPixel_HFlip

Spr_Complete:
							pop		edi

Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		Spr_NextSprite
							jmp		Gfx_Done


							//------------------------------------------------------------------------
							//No color
Gfx_NoColor:
							/////////////
							//Background
							/////////////


							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		NC_Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		NC_Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		NC_Bg_WndXOK
NC_Bg_NoWnd:
							mov		bl, 160
NC_Bg_WndXOK:
							//Divide number of pixels to draw by tile width (8)
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
NC_Bg_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Bg_9000
NC_Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Bg_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Bg_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Bg_NoLow
							shr		al, 4
							mov		bl, 0x80
NC_Bg_NoLow:
							test	dh, 1
							jz		NC_Bg_NoHigh
							shr		al, 2
							mov		bl, 0x80
NC_Bg_NoHigh:

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax

							add		edi, 2
							dec		bh
							jnz		NC_Bg_NextPixel


							dec		byte ptr [WndX]
							jz		NC_Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		NC_Bg_NextTile

NC_Bg_Complete:
							pop		edx
							pop		eax
NC_Bg_NoDraw:

							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		NC_Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		NC_Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		NC_Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
NC_Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Wnd_9000
NC_Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Wnd_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Wnd_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Wnd_NoLow
							shr		eax, 4
							mov		bl, 0x80
NC_Wnd_NoLow:
							test	dh, 1
							jz		NC_Wnd_NoHigh
							shr		eax, 2
							mov		bl, 0x80
NC_Wnd_NoHigh:

							add		dword ptr [LineX], 2

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax


							add		edi, 2
							dec		bh
							jnz		NC_Wnd_NextPixel


							cmp		dword ptr [LineX], 167 * 2
							jae		NC_Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		NC_Wnd_NextTile

NC_Wnd_Complete:
							pop		edx
							pop		eax
NC_Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		NC_Spr_On
							pop		edi
							jmp		NC_Spr_NoDraw

NC_Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
NC_Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		NC_Spr_FindNextSprite1	//Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		NC_Spr_FindNextSprite1	//Sprite outside right side of screen

							xor		edi, edi
NC_Spr_NextSpriteList:
							cmp		edi, esi
							jae		NC_Spr_MarkToDraw
							cmp		dh, byte ptr [Sprites + 2 * edi + 1]
							jae		NC_Spr_NextSpriteList1

							push	esi
NC_Spr_MoveSpriteList:
							mov		ebx, dword ptr [Sprites + 2 * esi - 2]
							mov		word ptr [Sprites + 2 * esi], bx
							dec		esi
							cmp		esi, edi
							jne		NC_Spr_MoveSpriteList
							pop		esi
							jmp		NC_Spr_MarkToDraw

NC_Spr_NextSpriteList1:
							inc		edi
							jmp		NC_Spr_NextSpriteList

NC_Spr_MarkToDraw:
							mov		word ptr [Sprites + 2 * edi], dx
							inc		esi
							cmp		esi, 10
							je		NC_Spr_Draw

NC_Spr_FindNextSprite1:
							xor		dh, dh
							inc		edx
							cmp		dl, 0xA0 / 4
							jne		NC_Spr_FindNextSprite


NC_Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		NC_Spr_NoDraw

NC_Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + 2 * esi - 2]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		NC_Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		NC_Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							movzx	esi, bh
							mov		bl, bh
							dec		al
							shr		esi, 4
							mov		dh, bh
							and		esi, 1
							shr		bl, 7
							and		dh, 8
							shr		dh, 1
							shl		edx, 4
							test	bh, 0x40
							jnz		NC_Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		NC_Spr_Draw_VFlipOk
NC_Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
NC_Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 1) * 2

							push	edi
							add		edi, eax

							test	bh, 0x20
							jnz		NC_Spr_HFlip

NC_Spr_NextPixel:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shl		dx, 1
							jnc		NC_Spr_NoLow
							or		bl, 2
							shr		al, 4
NC_Spr_NoLow:
							test	dh, 1
							jz		NC_Spr_NoHigh
							or		bl, 2
							shr		al, 2
							and		dh, 0xFE		//Clear bit
NC_Spr_NoHigh:

							test	bl, 2
							jz		NC_Spr_NextPixel	//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel
NC_Spr_Priority:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel

NC_Spr_HFlip:
NC_Spr_NextPixel_HFlip:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shr		edx, 1
							jnc		NC_Spr_NoLow_HFlip
							or		bl, 2
							shr		al, 2
NC_Spr_NoLow_HFlip:
							test	dl, 0x80
							jz		NC_Spr_NoHigh_HFlip
							or		bl, 2
							shr		al, 4
							and		dl, 0x7F	//Clear bit
NC_Spr_NoHigh_HFlip:

							test	bl, 2
							jz		NC_Spr_NextPixel_HFlip		//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority_HFlip
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel_HFlip
NC_Spr_Priority_HFlip:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel_HFlip

NC_Spr_Complete:
							pop		edi

NC_Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		NC_Spr_NextSprite
NC_Spr_NoDraw:
Spr_NoDraw:
Gfx_Done:

		add		esp, 28 + 2 * (160 + 14)
		pop		ebp
		pop		esi
		pop		ecx
		pop		eax

		jmp		LCD_Done



LCD_Off:
		//eax	= Ticks
		//esi	= Flags
		//dl	= FF40
		//dh	= FF41

		and		dh, ~3
		mov		byte ptr [ecx + Offset_LCD_Ticks], 0
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh

		mov		dword ptr [ecx + Offset_dwAVISoundPos], 0

		mov		dl, byte ptr [ecx + Offset_Terminating]
		test	dl, dl
		jz		LCD_Done

		or		esi, GB_EXITLOOP
		jmp		LCD_Done



LCD_NotEnoughTicks:
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
LCD_Done:
		//eax	= Ticks
		//esi	= Flags







		//*******
		//Sound

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebp, ecx
		//ebp	= this
		//ecx	free

		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_Off

		mov		ebx, dword ptr [ebp + Offset_Sound1TimeOut]
		//bh	= Sound1TimeOut
		test	ebx, ebx
		jz		Sound1_NoTimeOut

		cmp		ebx, eax
		ja		Sound1_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound1_Off

Sound1_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound1TimeOut], ebx
		//ebx	free

Sound1_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound1Sweep]
		test	ebx, ebx
		jz		Sound1_NoSweep

		sub		ebx, eax
		ja		Sound1_NotSweepYet

		mov		cl, byte ptr [ebp + FF00_ASM + 0x10]
		//cl	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Sweep], 0
		mov		ch, cl
		//ch	= [FF10]
		test	cl, 8
		jz		Sound1_SweepIncrease

		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		cmp		edx, ebx
		jae		Sound1_Sweep_NotZero

		xor		ebx, ebx
Sound1_Sweep_NotZero:

		sub		edx, ebx
		jmp		Sound1_Sweeped

Sound1_SweepIncrease:
		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		add		edx, ebx
		cmp		edx, 2047
		jb		Sound1_Sweep_NotAbove2047
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		mov		edx, 2047
Sound1_Sweep_NotAbove2047:

Sound1_Sweeped:
		//edx	= Sound1Frequency
		//ch	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Frequency], edx
		not		edx
		and		dh, 7
		and		ch, 0x70
		xor		ebx, ebx
		mov		byte ptr [ebp + FF00_ASM + 0x13], dl
		mov		byte ptr [ebp + FF00_ASM + 0x14], dh
		mov		bl, ch
		shl		ebx, 9
Sound1_NotSweepYet:
		mov		dword ptr [ebp + Offset_Sound1Sweep], ebx
Sound1_NoSweep:

		mov		ebx, dword ptr [ebp + Offset_Sound1Envelope]
		test	ebx, ebx
		jz		Sound1_NoEnvelope

		sub		ebx, eax
		ja		Sound1_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x12]
		//dl	= [FF12]
		test	dl, 8
		jz		Sound1_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound1_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
		shl		ebx, 14
		jmp		Sound1_Envelope_Done

Sound1_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound1_Off

Sound1_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound1_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
Sound1_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound1_Envelope_Done

Sound1_Envelope_NotYet:
Sound1_Envelope_Done:
		//ebx	= Sound1Envelope
		mov		dword ptr [ebp + Offset_Sound1Envelope], ebx
		//ebx	free
Sound1_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound1_TicksSet
		mov		edi, eax
Sound1_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		jmp		Sound1_MixEbxEcx

Sound1_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		//jmp		Sound1_MixEbxEcx
Sound1_MixEbxEcx:
		add		ebx, ecx

Sound1_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound1_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x10
		jz		Sound1_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound1_NotRight:

Sound1_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_Off

		mov		ebx, dword ptr [ebp + Offset_Sound2TimeOut]
		//bh	= Sound2TimeOut
		test	ebx, ebx
		jz		Sound2_NoTimeOut

		cmp		ebx, eax
		ja		Sound2_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound2_Off

Sound2_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound2TimeOut], ebx
		//ebx	free

Sound2_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound2Envelope]
		test	ebx, ebx
		jz		Sound2_NoEnvelope

		sub		ebx, eax
		ja		Sound2_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x17]
		//dl	= [FF17]
		test	dl, 8
		jz		Sound2_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound2_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
		shl		ebx, 14
		jmp		Sound2_Envelope_Done

Sound2_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound2_Off

Sound2_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound2_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
Sound2_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound2_Envelope_Done

Sound2_Envelope_NotYet:
Sound2_Envelope_Done:
		//ebx	= Sound2Envelope
		mov		dword ptr [ebp + Offset_Sound2Envelope], ebx
		//ebx	free
Sound2_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound2_TicksSet
		mov		edi, eax
Sound2_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		jmp		Sound2_MixEbxEcx

Sound2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_MixEbxEcx
Sound2_MixEbxEcx:
		add		ebx, ecx

Sound2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x20
		jz		Sound2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound2_NotRight:

Sound2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_Off

		mov		ebx, dword ptr [ebp + Offset_Sound3TimeOut]
		//bh	= Sound3TimeOut
		test	ebx, ebx
		jz		Sound3_NoTimeOut

		cmp		ebx, eax
		ja		Sound3_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound3Enabled], 0
		and		bl, ~0x04
		mov		byte ptr [ebp + FF00_ASM + 0x1A], 0x7F
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound3_Off

Sound3_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound3TimeOut], ebx
		//ebx	free

Sound3_NoTimeOut:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound3_TicksSet
		mov		edi, eax
Sound3_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume
		test	ch, 0x20
		jnz		Sound3_VolumeSet
		mov		bl, 8
		jmp		Sound3_VolumeSet
Sound3_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet
Sound3_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ToBuffer

Sound3_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume2
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		mov		bl, 8
		jmp		Sound3_VolumeSet2
Sound3_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet2
Sound3_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ChangeStage

Sound3_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume3
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		mov		dl, 8
		jmp		Sound3_VolumeSet3
Sound3_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_VolumeSet3
Sound3_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_ToBuffer

Sound3_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound3_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x40
		jz		Sound3_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_NotRight:

Sound3_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_Off

		mov		ebx, dword ptr [ebp + Offset_Sound4TimeOut]
		//bh	= Sound4TimeOut
		test	ebx, ebx
		jz		Sound4_NoTimeOut

		cmp		ebx, eax
		ja		Sound4_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound4_Off

Sound4_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound4TimeOut], ebx
		//ebx	free

Sound4_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound4Envelope]
		test	ebx, ebx
		jz		Sound4_NoEnvelope

		sub		ebx, eax
		ja		Sound4_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x21]
		//dl	= [FF21]
		test	dl, 8
		jz		Sound4_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound4_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
		shl		ebx, 14
		jmp		Sound4_Envelope_Done

Sound4_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		jmp		Sound4_Off

Sound4_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound4_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
Sound4_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound4_Envelope_Done

Sound4_Envelope_NotYet:
Sound4_Envelope_Done:
		//ebx	= Sound4Envelope
		mov		dword ptr [ebp + Offset_Sound4Envelope], ebx
		//ebx	free
Sound4_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound4_TicksSet
		mov		edi, eax
Sound4_TicksSet:

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound4Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		////edx	= (2048 - Sound4Frequency) << 3
		cmp		ecx, edx
		jae		Sound4_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_ToBuffer
		neg		ebx
		jmp		Sound4_ToBuffer

Sound4_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_ChangeStage
		neg		ebx
		jmp		Sound4_ChangeStage

Sound4_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		//dl	= Sound4Bit
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jz		Sound4_MixEbxEcx
		neg		ecx
		//jmp		Sound4_MixEbxEcx

Sound4_MixEbxEcx:
		add		ebx, ecx

Sound4_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound4_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x80
		jz		Sound4_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound4_NotRight:

Sound4_Off:



		mov		dl, byte ptr [ebp + Offset_SoundTicks]
		sub		dl, al
		mov		byte ptr [ebp + Offset_SoundTicks], dl
		jae		Sound_NoUpdate

		mov		ecx, ebp

		add		edx, eax
		and		edx, 0xFF
		mov		edi, eax
		sub		edi, edx

		push	eax
		push	esi
		push	edi
		call	SoundUpdate
		pop		edi
		pop		esi
		pop		eax



		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound1Frequency) << 1
		cmp		ecx, edx
		jae		Sound1_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		jmp		Sound1_2_MixEbxEcx

Sound1_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		//jmp		Sound1_2_MixEbxEcx
Sound1_2_MixEbxEcx:
		add		ebx, ecx

Sound1_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		dword ptr [ebp + Offset_SoundL], ebx
		pop		ebx
Sound1_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x10
		jz		Sound1_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		dword ptr [ebp + Offset_SoundR], ebx
Sound1_2_NotRight:
		pop		edi
Sound1_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound2Frequency) << 1
		cmp		ecx, edx
		jae		Sound2_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		jmp		Sound2_2_MixEbxEcx

Sound2_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_2_MixEbxEcx
Sound2_2_MixEbxEcx:
		add		ebx, ecx

Sound2_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
		pop		ebx
Sound2_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x20
		jz		Sound2_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound2_2_NotRight:

		pop		edi

Sound2_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume
		test	ch, 0x20
		jnz		Sound3_2_VolumeSet
		mov		bl, 8
		jmp		Sound3_2_VolumeSet
Sound3_2_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet
Sound3_2_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ToBuffer

Sound3_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_2_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume2
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		mov		bl, 8
		jmp		Sound3_2_VolumeSet2
Sound3_2_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet2
Sound3_2_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ChangeStage

Sound3_2_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_2_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume3
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		mov		dl, 8
		jmp		Sound3_2_VolumeSet3
Sound3_2_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_2_VolumeSet3
Sound3_2_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_2_ToBuffer

Sound3_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound3_2_NotLeft:
		test	cl, 0x40
		jz		Sound3_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_2_NotRight:

		pop		edi

Sound3_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_2_Off

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		//edx	= Sound4Frequency
		cmp		ecx, edx
		jae		Sound4_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_2_ToBuffer
		neg		ebx
		jmp		Sound4_2_ToBuffer

Sound4_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_2_ChangeStage
		neg		ebx
		jmp		Sound4_2_ChangeStage

Sound4_2_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_2_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jnz		Sound4_2_MixEbxEcx
		neg		ecx
		//jmp		Sound4_2_MixEbxEcx

Sound4_2_MixEbxEcx:
		add		ebx, ecx

Sound4_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
		pop		ebx
Sound4_2_NotLeft:
		test	cl, 0x80
		jz		Sound4_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound4_2_NotRight:

Sound4_2_Off:



Sound_NoUpdate:
		mov		ecx, ebp



		//******
		//Serial

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebx, dword ptr [ecx + Offset_SerialTicks]
		test	ebx, ebx
		jz		NoSerialTransfer
		sub		ebx, eax
		ja		NoSerialBitShift

		test	esi, GB_LINKCABLE
		jz		NoLinkCable
		or		esi, GB_SERIALBIT | GB_EXITLOOP
		jmp		SerialFlagSet

NoLinkCable:
		mov		dx, word ptr [ecx + FF00_ASM + 0x01]
		add		ebx, 128
		and		dh, 0x81
		cmp		dh, 0x81
		jne		StopSerialTransfer
		mov		dh, byte ptr [ecx + Offset_SerialBit]
		shl		dl, 1
		inc		dh
		cmp		dh, 8
		mov		byte ptr [ecx + FF00_ASM + 0x01], dl
		mov		byte ptr [ecx + Offset_SerialBit], dh
		jb		SerialBitShiftNotComplete
		mov		dl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dl, 0x08
		mov		byte ptr [ecx + FF00_ASM + 0x02], 0x7F
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dl

StopSerialTransfer:
		xor		ebx, ebx

SerialBitShiftNotComplete:
SerialFlagSet:
NoSerialBitShift:
		mov		dword ptr [ecx + Offset_SerialTicks], ebx
NoSerialTransfer:


		//*******
		//Divider

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + Offset_DIV_Ticks]
		//bl	= DIV_Ticks
		sub		bl, al	//DIV_Ticks -= Ticks
		jnc		DIV_NotEnoughTicks
		mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
		//dl	= FF00(0x04)
		add		bl, 32	//DIV_Ticks += 32
		inc		dl
		mov		byte ptr [ecx + FF00_ASM + 0x04], dl
		//dl	free
		//edx	free
DIV_NotEnoughTicks:
		mov		byte ptr [ecx + Offset_DIV_Ticks], bl
		//bl	free
		//ebx	free

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags


		//*****
		//Timer

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + FF00_ASM + 0x07]
		test	bl, 0x04
		jz		TimerDisabled
		mov		ebx, dword ptr [ecx + Offset_TIMA_Ticks]
		//ebx	= TIMA_Ticks
		add		ebx, eax
		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jz		Timer_FastCPU
		add		ebx, eax
Timer_FastCPU:
		//eax	free
		mov		edx, dword ptr [ecx + Offset_Hz]
		//edx	= Hz
		//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
		cmp		ebx, edx
		jb		TIMA_NotEnoughTicks

		sub		ebx, edx
		//edx	free
		mov		dl, byte ptr [ecx + FF00_ASM + 0x05]
		//dl	= FF00(0x05)
		inc		dl
		jnz		TIMA_NotZero
		mov		dl, byte ptr [ecx + FF00_ASM + 0x06]
		//dl	= FF00(0x06) (new value of FF00(0x05))
		mov		dh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dh, 0x04
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dh
		//or		byte ptr [ecx + FF00_ASM + 0x0F], 0x04
TIMA_NotZero:
		mov		byte ptr [ecx + FF00_ASM + 0x05], dl
		//dl	free
		//edx	free

TIMA_NotEnoughTicks:
		mov		dword ptr [ecx + Offset_TIMA_Ticks], ebx
		//ebx	free

TimerDisabled:
		//ecx	= this
		//esi	= Flags


		//**********
		//Interrupts

		//ecx	= this
		//esi	= Flags

		test	esi, GB_IE
		jz		NoInterrupt
		mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
		//ah	= FF00(0x0F)
		mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
		and		al, ah
		//al	= FF00(0xFF) & FF00(0x0F)
		test	al, 0x0F
		jz		InterruptServiced

		and		esi, ~(GB_IE | GB_ENABLEIE | GB_HALT)
		mov		dword ptr [ecx + Offset_Flags], esi


		push	ecx
		xor		ecx, ecx
		shr		al, 1
		jc		InterruptFound
TestNextInterrupt:
		inc		cl
		shr		al, 1
		jnc		TestNextInterrupt

InterruptFound:
		//al	free
		mov		bl, 1
		shl		bl, cl
		not		bl
		and		ah, bl
		//ah	= new value of FF0F
		lea		ebx, [ecx * 8 + 0x40]
		//ebx	= interrupt address
		pop		ecx
		//ecx	= this

		mov		edx, dword ptr [ecx + Offset_Reg_SP]
		//edx	= Reg_SP
		sub		dx, 2

		mov		byte ptr [ecx + FF00_ASM + 0x0F], ah
		//ah	free
		//eax	free

		mov		eax, dword ptr [ecx + Offset_Reg_PC]
		//eax	= Reg_PC

		mov		word ptr [ecx + Offset_Reg_PC], bx
		//ebx	free

		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		//eax	free
		//edx	free

		jmp		InterruptServiced

NoInterrupt:
		//ecx	= this
		//esi	= Flags

		test	esi, GB_ENABLEIE
		jz		InterruptServiced
		or		esi, GB_IE

InterruptServiced:
		//ecx	= this
		//esi	= Flags


		//*********
		//Exit loop

		//ecx	= this
		//esi	= Flags

		test	esi, GB_EXITLOOP
		jz		ContinueLoop

ExitLoop:
		and		esi, ~(GB_EXITLOOP | GB_EXITLOOPDIRECTLY)
		mov		dword ptr [ecx + Offset_Flags], esi
		//esi	free

		pop		ebx
		pop		edx
		pop		edi
		pop		esi
		pop		ebp
		ret
	}
}

#undef		Sprites
#undef		LineX
#undef		Line
#undef		LCD_X
#undef		WndX



void AccessDenied(CGameBoy *pGameBoy, UINT uID)
{
	char		szBuffer[0x100];


	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hMsgParent, String(uID), "Game Lad", MB_ICONWARNING | MB_OK);
	pGameBoy->Stop();
	pGameBoy->Flags |= GB_ERROR;
}



void __declspec(naked) CGameBoy::DebugMainLoop()
{
	__asm
	{
		push	ebp
		push	esi
		push	edi
		push	edx
		push	ebx

		mov		esi, dword ptr [ecx + Offset_Flags]

ContinueLoop:
		//esi	= Flags
		test	esi, GB_HALT
		jz		NotHalt

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		mov		eax, 1
		//eax	= Ticks
		inc		ebx
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free
		jmp		HaltComplete

NotHalt:
		//esi	= Flags
		mov		dword ptr [ecx + Offset_Flags], esi

		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		//edx	= Reg_PC

		call	RetrieveAccess
		test	al, MEM_EXECUTE
		jz		ExecuteAccessDenied

		test	al, MEM_READ
		jz		ReadAccessDenied

		call	ReadMem
		and		eax, 0xFF
		call	dword ptr [DebugOpCodes + 4 * eax]

		mov		esi, dword ptr [ecx + Offset_Flags]

		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jnz		FastCPU
		shl		al, 1
FastCPU:

		test	esi, GB_EXITLOOPDIRECTLY
		jnz		ExitLoop

		mov		ebx, dword ptr [ecx + Offset_nCycles]
		//ebx	= Cycles
		add		ebx, eax
		mov		dword ptr [ecx + Offset_nCycles], ebx
		//ebx	free

		push	eax
		mov		edx, dword ptr [ecx + Offset_Reg_PC]
		call	RetrieveAccess
		test	al, MEM_BREAKPOINT
		pop		eax
		jz		NotBreakPoint
		or		esi, GB_EXITLOOP | GB_ERROR
NotBreakPoint:

HaltComplete:
		//eax	= Ticks
		//esi	= Flags




		//LCD
		mov		bl, byte ptr [ecx + Offset_LCD_Ticks]
		//bl	= LCD_Ticks
		cmp		bl, al
		ja		LCD_NotEnoughTicks

		mov		dx, word ptr [ecx + FF00_ASM + 0x40]

		//dl	= FF40
		//dh	= FF41
		test	dl, 0x80
		jz		LCD_Off

		test	dh, 2
		jnz		LCD_Mode2or3

		test	dh, 1
		jnz		LCD_VBlank



//LCD_HBlank:
		//Reset coincidence flag
		and		dh, ~4

		mov		bh, byte ptr [ecx + FF00_ASM + 0x44]
		sub		bl, al

		cmp		bh, 143
		ja		LCD_ExitHBlank

		add		bl, 40
		inc		bh
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		//bh	free
		//ebx	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		jmp		LCD_Done

LCD_ExitHBlank:
		add		bl, 228
		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		//bh	= [FF0F]
		mov		byte ptr [ecx + FF00_ASM + 0x44], 145
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		and		dh, ~3
		or		dh, 1
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		//dh	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		or		bh, 1
		or		esi, GB_EXITLOOP

		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bl, 144
		jne		LCD_NoCoincidence

		or		byte ptr [ecx + FF00_ASM + 0x41], 0x04
		or		byte ptr [ecx + Offset_MemStatus + 0x8F41], MEM_CHANGED

		test	dh, 0x40
		jnz		LCD_SetLCDC01

LCD_NoCoincidence:
		test	dh, 0x10
		jz		LCD_Done

LCD_SetLCDC01:
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		jmp		LCD_Done



LCD_VBlank:
		mov		bh, [ecx + FF00_ASM + 0x44]
		sub		bl, al

		//bh	= [FF44]
		test	bh, bh
		jz		LCD_ExitVBlank

		add		bl, 228
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free

		cmp		bh, 154
		ja		LCD_Line0

		inc		bh
		mov		byte ptr [ecx + FF00_ASM + 0x44], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		cmp		bh, bl
		//bl	free
		//bh	free
		//ebx	free
		jne		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
		jmp		LCD_Done

LCD_Line0:
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x45]
		test	bl, bl
		jnz		LCD_Done

		or		byte ptr [ecx + FF00_ASM + 0x41], 0x04
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		test	dh, 0x40
		//dh	free
		jz		LCD_Done

		//LYC interrupt
		mov		bl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bl, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
		jmp		LCD_Done

LCD_ExitVBlank:
		add		bl, 40
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl

		and		dh, ~3
		or		dh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		mov		byte ptr [ecx + Offset_WindowY2], 0

		test	dh, 0x20
		jz		LCD_Done

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		jmp		LCD_Done

LCD_Mode2or3:
		test	dh, 1
		jz		LCD_OAM



//LCD_ToLCD:
		add		bl, 104
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		//ebx	free

		and		dh, ~3
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED
		test	dh, 0x08
		jz		LCD_NoSetLCDC00

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

LCD_NoSetLCDC00:
		//dh	free
		//edx	free

		test	esi, GB_HDMA
		jz		LCD_NoHDMA

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		//bl	= [FF4F]
		and		ebx, 1
		shl		ebx, 13

		mov		ebp, dword ptr [ecx + FF00_ASM + 0x51]
		//ebp	= [FF51-52]
		mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
		//edi	= [FF53-54]
		rol		bp, 8
		rol		di, 8
		and		ebp, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		mov		eax, ebp
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F52], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F51], MEM_CHANGED
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F54], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F53], MEM_CHANGED

		mov		bl, byte ptr [ecx + FF00_ASM + 0x55]
		//bl	= [FF55]

		mov		eax, ebp
		shr		ebp, 12
		and		eax, 0x0FFF
		mov		ebp, [ecx + Offset_MEM + ebp * 4]

		dec		bl	//[FF55]--

		mov		edx, [ebp + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x04], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x08], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED
		mov		edx, [ebp + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx
		or		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x0C], (MEM_CHANGED << 24) | (MEM_CHANGED << 16) | (MEM_CHANGED << 8) | MEM_CHANGED

		mov		byte ptr [ecx + FF00_ASM + 0x55], bl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F55], MEM_CHANGED
		test	bl, 0x80
		//bl	free
		jz		HDMA_NotFinished
		and		esi, ~GB_HDMA
HDMA_NotFinished:

LCD_NoHDMA:
		jmp		LCD_Done



LCD_OAM:
		//eax	= Ticks
		//esi	= Flags
		//dl	= [FF40]
		//dh	= [FF41]

		sub		bl, al
		add		bl, 84
		or		dh, 3
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
		//bl	free
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		mov		bx, word ptr [ecx + FF00_ASM + 0x44]
		//bl	= [FF44]
		//bh	= [FF45]

		test	dh, 0x40
		jz		LCD_NoLYC

		cmp		bl, bh
		jne		LCD_NoLYC

		mov		bh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		bh, 2
		mov		byte ptr [ecx + FF00_ASM + 0x0F], bh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

LCD_NoLYC:
		//eax	= Ticks
		//esi	= Flags
		//bl	= [FF44]

		cmp		bl, 144
		ja		LCD_Done

		mov		bh, byte ptr [ecx + Offset_FramesToSkip]
		test	bh, bh
		jnz		LCD_Done


		//Set pointer to bitmap
		movzx	edi, bl
		//bl	free
		imul	edi, (160 + 14) * 2
		add		edi, dword ptr [ecx + Offset_pGBBitmap]

		push	eax
		push	ecx
		push	esi

		push	ebp
		mov		ebp, esp
		sub		esp, 28 + 2 * (160 + 14)

#define		LineX		(ebp - 4)
#define		LCD_X		(ebp - 5)
#define		WndX		(ebp - 6)
#define		Sprites		(ebp - 26)
#define		Line		(ebp - 28 - 2 * (160 + 14))

		push	edi

		mov		bl, byte ptr [ecx + FF00_ASM + 0x43]
		mov		byte ptr [LCD_X], bl

		lea		ebx, [Line]
		mov		dword ptr [LineX], ebx

		//WORD			Sprites[10];
				/*	BYTE	LCD_X = FF00_C(0x43);
					WORD	Line[160 + 14];
					WORD	*LineX;
					LineX = Line;
					if (FF00_C(0x44) < 144)
					{
						BYTE	WndX;*/


		mov		dword ptr [Line], 0
		mov		dword ptr [Line + 4], 0
		mov		dword ptr [Line + 8], 0
		mov		dword ptr [Line + 12], 0


		test	esi, GB_ROM_COLOR
		jz		Gfx_NoColor


		/////////////
		//Background
		/////////////

							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		Bg_WndXOK
Bg_NoWnd:
							mov		bl, 160
Bg_WndXOK:
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
Bg_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Bg_9000
Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Bg_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Bg_NoVFlip
							mov		al, 14
							sub		al, ah
Bg_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Bg_HFlip

							shr		bl, 7
							or		bl, 2
Bg_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shl		dx, 1
							jnc		Bg_NoLow
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoLow:
							test	dh, 1
							jz		Bg_NoHigh
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_HFlip:
							shr		bl, 7
							or		bl, 2
Bg_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [eax], 0

							shr		edx, 1
							jnc		Bg_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [eax], bl
Bg_NoLow_HFlip:
							test	dl, 0x80
							jz		Bg_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [eax], bl
Bg_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Bg_NextPixel_HFlip

							pop		ecx

							dec		byte ptr [WndX]
							jz		Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		Bg_NextTile

Bg_Complete:
							pop		edx
							pop		eax
Bg_NoDraw:


							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al
							mov		bl, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800 + 0x2000]
							movzx	esi, bl
							and		esi, 7
							shl		esi, 3
							add		esi, ecx
							//bl = tile attributes
							//esi = &GB->BGP[palette] - Offset_BGP

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		Wnd_9000
Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
Wnd_9000:
							//edx = tile no

							mov		al, bl
							and		al, 8
							shr		al, 2
							add		dh, al
							mov		al, ah
							test	bl, 0x40
							jz		Wnd_NoVFlip
							mov		al, 14
							sub		al, ah
Wnd_NoVFlip:
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]

							push	ecx

							mov		bh, 8

							test	bl, 0x20
							jnz		Wnd_HFlip

							shr		bl, 7
							or		bl, 2
Wnd_NextPixel:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shl		dx, 1
							jnc		Wnd_NoLow
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoLow:
							test	dh, 1
							jz		Wnd_NoHigh
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_HFlip:
							shr		bl, 7
							or		bl, 2
Wnd_NextPixel_HFlip:
							push	esi

							mov		eax, dword ptr [LineX]
							mov		byte ptr [Line + eax], 0

							shr		edx, 1
							jnc		Wnd_NoLow_HFlip
							add		esi, 2
							mov		byte ptr [Line + eax], bl
Wnd_NoLow_HFlip:
							test	dl, 0x80
							jz		Wnd_NoHigh_HFlip
							add		esi, 4
							mov		byte ptr [Line + eax], bl
Wnd_NoHigh_HFlip:

							add		dword ptr [LineX], 2

							mov		ax, word ptr [esi + Offset_BGP]
							mov		ecx, eax
							shr		ch, 2
							shl		cl, 2
							and		ecx, 0x1F7C
							and		eax, 0x03E0
							or		al, ch
							or		ah, cl
							mov		word ptr [edi], ax

							pop		esi

							add		edi, 2
							dec		bh
							jnz		Wnd_NextPixel_HFlip

							pop		ecx

							cmp		dword ptr [LineX], 167 * 2
							jae		Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		Wnd_NextTile

Wnd_Complete:
							pop		edx
							pop		eax
Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		Spr_On
							pop		edi
							jmp		Spr_NoDraw

Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		Spr_FindNextSprite1 //Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		Spr_FindNextSprite1	//Sprite outside right side of screen

							mov		byte ptr [Sprites + esi], dl
							inc		esi
							cmp		esi, 10
							je		Spr_Draw

Spr_FindNextSprite1:
							inc		edx
							xor		dh, dh
							cmp		dl, 0xA0 / 4
							jne		Spr_FindNextSprite


Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		Spr_NoDraw

Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + esi - 1]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							dec		al
							mov		dh, bh
							and		dh, 8
							shr		dh, 2
							shl		edx, 4
							test	bh, 0x40
							jnz		Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		Spr_Draw_VFlipOk
Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 2) * 2

							push	edi
							add		edi, eax
							mov		dword ptr [LineX], eax
							add		dword ptr [LineX], 2

							mov		esi, ebx
							and		esi, 0x0700
							shr		esi, 5
							add		esi, ecx

							test	bh, 0x20
							jnz		Spr_HFlip

Spr_NextPixel:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel
							test	bh, 0x80
							jnz		Spr_NoDrawPixel
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel
Spr_NoDrawPixel:
							shl		dl, 1
							shl		dh, 1
							jmp		Spr_NextPixel
Spr_DrawPixel:
							push	ebx
							xor		ebx, ebx

							shl		dh, 1
							jnc		Spr_NoLow
							mov		ebx, 4
Spr_NoLow:
							shl		dl, 1
							jnc		Spr_NoHigh
							add		ebx, 2
Spr_NoHigh:

							test	ebx, ebx
							jz		Spr_NextPixel1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel1:
							pop		ebx

							jmp		Spr_NextPixel

Spr_HFlip:
Spr_NextPixel_HFlip:
							test	edx, edx
							jz		Spr_Complete

							add		edi, 2

							mov		eax, dword ptr [LineX]
							add		dword ptr [LineX], 2

							test	byte ptr [Line + eax], 2
							jz		Spr_DrawPixel_HFlip
							test	bh, 0x80
							jnz		Spr_NoDrawPixel_HFlip
							test	byte ptr [Line + eax], 1
							jz		Spr_DrawPixel_HFlip
Spr_NoDrawPixel_HFlip:
							shr		dl, 1
							shr		dh, 1
							jmp		Spr_NextPixel_HFlip
Spr_DrawPixel_HFlip:
							push	ebx
							xor		ebx, ebx

							shr		dl, 1
							jnc		Spr_NoLow_HFlip
							mov		ebx, 2
Spr_NoLow_HFlip:
							shr		dh, 1
							jnc		Spr_NoHigh_HFlip
							add		ebx, 4
Spr_NoHigh_HFlip:

							test	ebx, ebx
							jz		Spr_NextPixel_HFlip1	//Don't draw transparent pixels

							mov		ax, word ptr [esi + Offset_OBP + ebx]
							mov		ebx, eax
							shr		bh, 2
							shl		bl, 2
							and		ebx, 0x1F7C
							and		eax, 0x03E0
							or		al, bh
							or		ah, bl
							and		word ptr [edi], 0x8000
							or		word ptr [edi], ax

Spr_NextPixel_HFlip1:
							pop		ebx

							jmp		Spr_NextPixel_HFlip

Spr_Complete:
							pop		edi

Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		Spr_NextSprite
							jmp		Gfx_Done


							//------------------------------------------------------------------------
							//No color
Gfx_NoColor:
							/////////////
							//Background
							/////////////


							//Clip window
							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Bg_NoWnd	//Window disabled, draw whole line
							mov		dl, byte ptr [ecx + FF00_ASM + 0x44]
							cmp		byte ptr [ecx + FF00_ASM + 0x4A], dl
							ja		NC_Bg_NoWnd	//Window below current line, draw whole line
							mov		bl, byte ptr [ecx + FF00_ASM + 0x4B]
							cmp		bl, 7
							jbe		NC_Bg_NoDraw	//Window is completely hiding the background (no draw)
							//160 is the highest WndX possible
							cmp		bl, 160
							jna		NC_Bg_WndXOK
NC_Bg_NoWnd:
							mov		bl, 160
NC_Bg_WndXOK:
							//Divide number of pixels to draw by tile width (8)
							shr		bl, 3
							mov		byte ptr [WndX], bl


							mov		bl, [ecx + FF00_ASM + 0x42]
							mov		al, [ecx + FF00_ASM + 0x43]
							add		bl, [ecx + FF00_ASM + 0x44]
							mov		bh, al
							shr		al, 3
							not		bh
							mov		ah, bl
							shr		bl, 3
							and		ah, 7
							shl		ah, 1
							and		bh, 7

							//Adjust number of tiles to be drawn
							cmp		bh, 7
							adc		byte ptr [WndX], 0

							//Scroll background (0 - 7 pixels)
							shl		bh, 1
							movzx	edx, bh
							add		edi, edx
							add		dword ptr [LineX], edx


							//bl	Y in tile map
							//ah	Y in tile * 2
							//al	X in tile map


							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x08
							shl		edx, 2
							or		dl, bl
							shl		edx, 5
NC_Bg_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Bg_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Bg_9000
NC_Bg_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Bg_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Bg_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Bg_NoLow
							shr		al, 4
							mov		bl, 0x80
NC_Bg_NoLow:
							test	dh, 1
							jz		NC_Bg_NoHigh
							shr		al, 2
							mov		bl, 0x80
NC_Bg_NoHigh:

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax

							add		edi, 2
							dec		bh
							jnz		NC_Bg_NextPixel


							dec		byte ptr [WndX]
							jz		NC_Bg_Complete

							pop		edx
							pop		eax
							inc		al
							and		al, 31
							jmp		NC_Bg_NextTile

NC_Bg_Complete:
							pop		edx
							pop		eax
NC_Bg_NoDraw:

							/////////
							//Window
							/////////


							//Restore pointer to bitmap
							mov		edi, dword ptr [esp]

							movzx	eax, byte ptr [ecx + FF00_ASM + 0x4B]
							shl		eax, 1
							add		edi, eax
							mov		dword ptr [LineX], eax

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x20
							jz		NC_Wnd_NoDraw		//Window disabled

							cmp		byte ptr [ecx + FF00_ASM + 0x4B], 166
							ja		NC_Wnd_NoDraw		//WX too high

							movzx	ebx, byte ptr [ecx + FF00_ASM + 0x44]
							sub		bl, byte ptr [ecx + FF00_ASM + 0x4A]
							jb		NC_Wnd_NoDraw		//LY too low

							cmp		bl, 143
							ja		NC_Wnd_NoDraw		//WY too high
							mov		bl, byte ptr [ecx + Offset_WindowY2]
							inc		byte ptr [ecx + Offset_WindowY2]
							mov		ah, bl
							and		bl, 0xF8
							and		ah, 7
							shl		ah, 1
							shl		ebx, 2
							movzx	edx, byte ptr [ecx + FF00_ASM + 0x40]
							and		dl, 0x40
							shl		edx, 4
							add		edx, ebx
							xor		al, al
NC_Wnd_NextTile:
							push	eax
							push	edx
							or		dl, al

							test	byte ptr [ecx + FF00_ASM + 0x40], 0x10
							jnz		NC_Wnd_8000
							movsx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
							add		edx, 0x0100
							jmp		NC_Wnd_9000
NC_Wnd_8000:
							movzx	edx, byte ptr [ecx + Offset_MEM_VRAM + edx + 0x1800]
NC_Wnd_9000:
							//edx = tile no

							mov		al, ah
							and		eax, 0xFF
							shl		edx, 4
							add		edx, eax
							mov		edx, dword ptr [ecx + Offset_MEM_VRAM + edx]


							mov		bh, 8

NC_Wnd_NextPixel:
							mov		al, byte ptr [ecx + FF00_ASM + 0x47]

							xor		bl, bl

							shl		dx, 1
							jnc		NC_Wnd_NoLow
							shr		eax, 4
							mov		bl, 0x80
NC_Wnd_NoLow:
							test	dh, 1
							jz		NC_Wnd_NoHigh
							shr		eax, 2
							mov		bl, 0x80
NC_Wnd_NoHigh:

							add		dword ptr [LineX], 2

							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		ah, bl
							mov		word ptr [edi], ax


							add		edi, 2
							dec		bh
							jnz		NC_Wnd_NextPixel


							cmp		dword ptr [LineX], 167 * 2
							jae		NC_Wnd_Complete

							pop		edx
							pop		eax
							inc		al
							jmp		NC_Wnd_NextTile

NC_Wnd_Complete:
							pop		edx
							pop		eax
NC_Wnd_NoDraw:


							//////////
							//Sprites
							//////////


							test	[ecx + FF00_ASM + 0x40], 2
							jnz		NC_Spr_On
							pop		edi
							jmp		NC_Spr_NoDraw

NC_Spr_On:
							mov		ah, [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shl		ah, 1
							or		ah, 7


							xor		edx, edx
							xor		esi, esi

							mov		al, byte ptr [ecx + FF00_ASM + 0x44]
							add		al, 16
NC_Spr_FindNextSprite:
							mov		bl, al
							sub		bl, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx]
							cmp		bl, ah
							ja		NC_Spr_FindNextSprite1	//Sprite not on current line

							mov		dh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * edx + 1]
							cmp		dh, 160 + 8
							jae		NC_Spr_FindNextSprite1	//Sprite outside right side of screen

							xor		edi, edi
NC_Spr_NextSpriteList:
							cmp		edi, esi
							jae		NC_Spr_MarkToDraw
							cmp		dh, byte ptr [Sprites + 2 * edi + 1]
							jae		NC_Spr_NextSpriteList1

							push	esi
NC_Spr_MoveSpriteList:
							mov		ebx, dword ptr [Sprites + 2 * esi - 2]
							mov		word ptr [Sprites + 2 * esi], bx
							dec		esi
							cmp		esi, edi
							jne		NC_Spr_MoveSpriteList
							pop		esi
							jmp		NC_Spr_MarkToDraw

NC_Spr_NextSpriteList1:
							inc		edi
							jmp		NC_Spr_NextSpriteList

NC_Spr_MarkToDraw:
							mov		word ptr [Sprites + 2 * edi], dx
							inc		esi
							cmp		esi, 10
							je		NC_Spr_Draw

NC_Spr_FindNextSprite1:
							xor		dh, dh
							inc		edx
							cmp		dl, 0xA0 / 4
							jne		NC_Spr_FindNextSprite


NC_Spr_Draw:
							//Restore pointer to bitmap
							pop		edi

							test	esi, esi
							jz		NC_Spr_NoDraw

NC_Spr_NextSprite:
							push	esi
							movzx	esi, byte ptr [Sprites + 2 * esi - 2]

							movzx	eax, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 1]
							test	eax, eax
							jz		NC_Spr_NextSprite1	//Sprite outside left side of screen
							//cmp		al, 160 + 8
							//jae		NC_Spr_NextSprite1	//Sprite outside right side of screen

							movzx	edx, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 2]
							mov		ah, byte ptr [ecx + FF00_ASM + 0x40]
							and		ah, 4
							shr		ah, 2
							not		ah
							and		dl, ah
							mov		bh, byte ptr [ecx + (FF00_ASM - 0x100) + 4 * esi + 3]
							mov		ax, word ptr [ecx + (FF00_ASM - 0x100) + 4 * esi]
							movzx	esi, bh
							mov		bl, bh
							dec		al
							shr		esi, 4
							mov		dh, bh
							and		esi, 1
							shr		bl, 7
							and		dh, 8
							shr		dh, 1
							shl		edx, 4
							test	bh, 0x40
							jnz		NC_Spr_Draw_VFlip
							add		dl, 30
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							add		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, al
							sub		dl, al
							jmp		NC_Spr_Draw_VFlipOk
NC_Spr_Draw_VFlip:
							add		dl, al
							add		dl, al
							mov		al, byte ptr [ecx + FF00_ASM + 0x40]
							not		al
							and		al, 4
							shl		al, 2
							sub		dl, al
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
							sub		dl, byte ptr [ecx + FF00_ASM + 0x44]
NC_Spr_Draw_VFlipOk:

							mov		dx, word ptr [ecx + Offset_MEM_VRAM + edx]

							xor		al, al
							shr		eax, 7
							sub		eax, 4
							//eax = (SpriteX - 1) * 2

							push	edi
							add		edi, eax

							test	bh, 0x20
							jnz		NC_Spr_HFlip

NC_Spr_NextPixel:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shl		dx, 1
							jnc		NC_Spr_NoLow
							or		bl, 2
							shr		al, 4
NC_Spr_NoLow:
							test	dh, 1
							jz		NC_Spr_NoHigh
							or		bl, 2
							shr		al, 2
							and		dh, 0xFE		//Clear bit
NC_Spr_NoHigh:

							test	bl, 2
							jz		NC_Spr_NextPixel	//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel
NC_Spr_Priority:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel

NC_Spr_HFlip:
NC_Spr_NextPixel_HFlip:
							test	edx, edx
							jz		NC_Spr_Complete

							add		edi, 2

							mov		al, byte ptr [ecx + FF00_ASM + 0x48 + esi]
							and		bl, 0xFD

							shr		edx, 1
							jnc		NC_Spr_NoLow_HFlip
							or		bl, 2
							shr		al, 2
NC_Spr_NoLow_HFlip:
							test	dl, 0x80
							jz		NC_Spr_NoHigh_HFlip
							or		bl, 2
							shr		al, 4
							and		dl, 0x7F	//Clear bit
NC_Spr_NoHigh_HFlip:

							test	bl, 2
							jz		NC_Spr_NextPixel_HFlip		//Don't draw transparent pixels

							test	bl, 1
							jz		NC_Spr_Priority_HFlip
							test	dword ptr [edi], 0x8000
							jnz		NC_Spr_NextPixel_HFlip
NC_Spr_Priority_HFlip:

							and		word ptr [edi], 0x8000
							and		eax, 3
							mov		eax, dword ptr [GreyScales + 2 * eax]
							or		word ptr [edi], ax

							jmp		NC_Spr_NextPixel_HFlip

NC_Spr_Complete:
							pop		edi

NC_Spr_NextSprite1:
							pop		esi
							dec		esi
							jnz		NC_Spr_NextSprite
NC_Spr_NoDraw:
Spr_NoDraw:
Gfx_Done:

		add		esp, 28 + 2 * (160 + 14)
		pop		ebp
		pop		esi
		pop		ecx
		pop		eax

		jmp		LCD_Done



LCD_Off:
		//eax	= Ticks
		//esi	= Flags
		//dl	= FF40
		//dh	= FF41

		and		dh, ~3
		mov		byte ptr [ecx + Offset_LCD_Ticks], 0
		mov		byte ptr [ecx + FF00_ASM + 0x44], 0
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F44], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x41], dh
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F41], MEM_CHANGED

		mov		dword ptr [ecx + Offset_dwAVISoundPos], 0

		mov		dl, byte ptr [ecx + Offset_Terminating]
		test	dl, dl
		jz		LCD_Done

		or		esi, GB_EXITLOOP
		jmp		LCD_Done


LCD_NotEnoughTicks:
		sub		bl, al
		mov		byte ptr [ecx + Offset_LCD_Ticks], bl
LCD_Done:
		//eax	= Ticks
		//esi	= Flags







		//*******
		//Sound

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebp, ecx
		//ebp	= this
		//ecx	free

		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_Off

		mov		ebx, dword ptr [ebp + Offset_Sound1TimeOut]
		//bh	= Sound1TimeOut
		test	ebx, ebx
		jz		Sound1_NoTimeOut

		cmp		ebx, eax
		ja		Sound1_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound1_Off

Sound1_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound1TimeOut], ebx
		//ebx	free

Sound1_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound1Sweep]
		test	ebx, ebx
		jz		Sound1_NoSweep

		sub		ebx, eax
		ja		Sound1_NotSweepYet

		mov		cl, byte ptr [ebp + FF00_ASM + 0x10]
		//cl	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Sweep], 0
		mov		ch, cl
		//ch	= [FF10]
		test	cl, 8
		jz		Sound1_SweepIncrease

		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		cmp		edx, ebx
		jae		Sound1_Sweep_NotZero

		xor		ebx, ebx
Sound1_Sweep_NotZero:

		sub		edx, ebx
		jmp		Sound1_Sweeped

Sound1_SweepIncrease:
		mov		ebx, dword ptr [ebp + Offset_Sound1Frequency]
		and		cl, 7
		mov		edx, ebx
		shr		ebx, cl
		add		edx, ebx
		cmp		edx, 2047
		jb		Sound1_Sweep_NotAbove2047
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		mov		edx, 2047
Sound1_Sweep_NotAbove2047:

Sound1_Sweeped:
		//edx	= Sound1Frequency
		//ch	= [FF10]
		mov		dword ptr [ebp + Offset_Sound1Frequency], edx
		not		edx
		and		dh, 7
		and		ch, 0x70
		xor		ebx, ebx
		mov		byte ptr [ebp + FF00_ASM + 0x13], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F13], MEM_CHANGED
		mov		byte ptr [ebp + FF00_ASM + 0x14], dh
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F14], MEM_CHANGED
		mov		bl, ch
		shl		ebx, 9
Sound1_NotSweepYet:
		mov		dword ptr [ebp + Offset_Sound1Sweep], ebx
Sound1_NoSweep:

		mov		ebx, dword ptr [ebp + Offset_Sound1Envelope]
		test	ebx, ebx
		jz		Sound1_NoEnvelope

		sub		ebx, eax
		ja		Sound1_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x12]
		//dl	= [FF12]
		test	dl, 8
		jz		Sound1_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound1_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F12], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
		shl		ebx, 14
		jmp		Sound1_Envelope_Done

Sound1_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound1Enabled], 0
		and		bl, ~1
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound1_Off

Sound1_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound1_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x12], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F12], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound1Volume], dl
Sound1_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound1_Envelope_Done

Sound1_Envelope_NotYet:
Sound1_Envelope_Done:
		//ebx	= Sound1Envelope
		mov		dword ptr [ebp + Offset_Sound1Envelope], ebx
		//ebx	free
Sound1_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound1_TicksSet
		mov		edi, eax
Sound1_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound1Frequency) << 1
		cmp		ecx, edx
		jae		Sound1_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_ToBuffer
		neg		ebx
		jmp		Sound1_ToBuffer

Sound1_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_ChangeStage
		neg		ebx
		jmp		Sound1_ChangeStage

Sound1_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		jmp		Sound1_MixEbxEcx

Sound1_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_MixEbxEcx
		neg		ecx
		//jmp		Sound1_MixEbxEcx
Sound1_MixEbxEcx:
		add		ebx, ecx

Sound1_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound1_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x10
		jz		Sound1_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound1_NotRight:

Sound1_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_Off

		mov		ebx, dword ptr [ebp + Offset_Sound2TimeOut]
		//bh	= Sound2TimeOut
		test	ebx, ebx
		jz		Sound2_NoTimeOut

		cmp		ebx, eax
		ja		Sound2_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound2_Off

Sound2_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound2TimeOut], ebx
		//ebx	free

Sound2_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound2Envelope]
		test	ebx, ebx
		jz		Sound2_NoEnvelope

		sub		ebx, eax
		ja		Sound2_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x17]
		//dl	= [FF17]
		test	dl, 8
		jz		Sound2_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound2_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F17], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
		shl		ebx, 14
		jmp		Sound2_Envelope_Done

Sound2_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound2Enabled], 0
		and		bl, ~2
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound2_Off

Sound2_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound2_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x17], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F17], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound2Volume], dl
Sound2_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound2_Envelope_Done

Sound2_Envelope_NotYet:
Sound2_Envelope_Done:
		//ebx	= Sound2Envelope
		mov		dword ptr [ebp + Offset_Sound2Envelope], ebx
		//ebx	free
Sound2_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound2_TicksSet
		mov		edi, eax
Sound2_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 1
		//edx	= (2048 - Sound2Frequency) << 1
		cmp		ecx, edx
		jae		Sound2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_ToBuffer
		neg		ebx
		jmp		Sound2_ToBuffer

Sound2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_ChangeStage
		neg		ebx
		jmp		Sound2_ChangeStage

Sound2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		jmp		Sound2_MixEbxEcx

Sound2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_MixEbxEcx
Sound2_MixEbxEcx:
		add		ebx, ecx

Sound2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x20
		jz		Sound2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound2_NotRight:

Sound2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_Off

		mov		ebx, dword ptr [ebp + Offset_Sound3TimeOut]
		//bh	= Sound3TimeOut
		test	ebx, ebx
		jz		Sound3_NoTimeOut

		cmp		ebx, eax
		ja		Sound3_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		bh, byte ptr [ebp + FF00_ASM + 0x1A]
		mov		byte ptr [ebp + Offset_Sound3Enabled], 0
		and		bx, ~0x8004
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		mov		byte ptr [ebp + FF00_ASM + 0x1A], bh
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F1A], MEM_CHANGED
		jmp		Sound3_Off

Sound3_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound3TimeOut], ebx
		//ebx	free

Sound3_NoTimeOut:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound3_TicksSet
		mov		edi, eax
Sound3_TicksSet:

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume
		test	ch, 0x20
		jnz		Sound3_VolumeSet
		mov		bl, 8
		jmp		Sound3_VolumeSet
Sound3_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet
Sound3_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ToBuffer

Sound3_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume2
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		mov		bl, 8
		jmp		Sound3_VolumeSet2
Sound3_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_VolumeSet2
Sound3_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_ChangeStage

Sound3_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_HighVolume3
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		mov		dl, 8
		jmp		Sound3_VolumeSet3
Sound3_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_VolumeSet3
Sound3_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_ToBuffer

Sound3_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound3_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x40
		jz		Sound3_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_NotRight:

Sound3_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_Off

		mov		ebx, dword ptr [ebp + Offset_Sound4TimeOut]
		//bh	= Sound4TimeOut
		test	ebx, ebx
		jz		Sound4_NoTimeOut

		cmp		ebx, eax
		ja		Sound4_NotTimeOutYet

		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound4_Off

Sound4_NotTimeOutYet:
		sub		ebx, eax
		mov		dword ptr [ebp + Offset_Sound4TimeOut], ebx
		//ebx	free

Sound4_NoTimeOut:

		mov		ebx, dword ptr [ebp + Offset_Sound4Envelope]
		test	ebx, ebx
		jz		Sound4_NoEnvelope

		sub		ebx, eax
		ja		Sound4_Envelope_NotYet

		mov		dl, byte ptr [ebp + FF00_ASM + 0x21]
		//dl	= [FF21]
		test	dl, 8
		jz		Sound4_Envelope_Decrease

		cmp		dl, 0xF0
		jae		Sound4_Envelope_Max

		xor		ebx, ebx
		mov		bl, dl
		add		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F21], MEM_CHANGED
		shr		dl, 4
		and		bl, 7
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
		shl		ebx, 14
		jmp		Sound4_Envelope_Done

Sound4_Envelope_Max:
		mov		bl, byte ptr [ebp + FF00_ASM + 0x26]
		mov		byte ptr [ebp + Offset_Sound4Enabled], 0
		and		bl, ~8
		mov		byte ptr [ebp + FF00_ASM + 0x26], bl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED
		jmp		Sound4_Off

Sound4_Envelope_Decrease:
		xor		ebx, ebx
		mov		bl, dl
		test	dl, 0xF0
		jz		Sound4_Envelope_Min
		sub		dl, 0x10
		mov		byte ptr [ebp + FF00_ASM + 0x21], dl
		or		byte ptr [ebp + Offset_MemStatus_CPU + 0x8F21], MEM_CHANGED
		shr		dl, 4
		mov		byte ptr [ebp + Offset_Sound4Volume], dl
Sound4_Envelope_Min:
		and		bl, 7
		shl		ebx, 14
		//jmp		Sound4_Envelope_Done

Sound4_Envelope_NotYet:
Sound4_Envelope_Done:
		//ebx	= Sound4Envelope
		mov		dword ptr [ebp + Offset_Sound4Envelope], ebx
		//ebx	free
Sound4_NoEnvelope:

		mov		edi, dword ptr [ebp + Offset_SoundTicks]
		cmp		eax, edi
		ja		Sound4_TicksSet
		mov		edi, eax
Sound4_TicksSet:

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound4Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		////edx	= (2048 - Sound4Frequency) << 3
		cmp		ecx, edx
		jae		Sound4_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_ToBuffer
		neg		ebx
		jmp		Sound4_ToBuffer

Sound4_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_ChangeStage
		neg		ebx
		jmp		Sound4_ChangeStage

Sound4_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		//dl	= Sound4Bit
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jz		Sound4_MixEbxEcx
		neg		ecx
		//jmp		Sound4_MixEbxEcx

Sound4_MixEbxEcx:
		add		ebx, ecx

Sound4_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound4_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x80
		jz		Sound4_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound4_NotRight:

Sound4_Off:



		mov		dl, byte ptr [ebp + Offset_SoundTicks]
		sub		dl, al
		mov		byte ptr [ebp + Offset_SoundTicks], dl
		jae		Sound_NoUpdate

		mov		ecx, ebp

		add		edx, eax
		and		edx, 0xFF
		mov		edi, eax
		sub		edi, edx

		push	eax
		push	esi
		push	edi
		call	SoundUpdate
		pop		edi
		pop		esi
		pop		eax



		mov		bl, byte ptr [ebp + Offset_Sound1Enabled]
		test	bl, bl
		jz		Sound1_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound1Frequency]
		//edx	= Sound1Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound1Ticks]
		//ecx	= Sound1Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound1Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound1Frequency) << 3
		cmp		ecx, edx
		jae		Sound1_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound1Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x11]
		//cl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound1Stage]
		//ch	= Sound1Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound1_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound1_2_ToBuffer
		neg		ebx
		jmp		Sound1_2_ToBuffer

Sound1_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound1_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		//dh	= Sound1Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_ChangeStage
		neg		ebx
		jmp		Sound1_2_ChangeStage

Sound1_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound1Stage]
		xor		ebx, ebx
Sound1_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound1Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound1Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound1Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound1Volume]
		//bl	= Sound1Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x11]
		//dl	= [FF11]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound1Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound1_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		jmp		Sound1_2_MixEbxEcx

Sound1_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound1_2_MixEbxEcx
		neg		ecx
		//jmp		Sound1_2_MixEbxEcx
Sound1_2_MixEbxEcx:
		add		ebx, ecx

Sound1_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound1Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x01
		jz		Sound1_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		dword ptr [ebp + Offset_SoundL], ebx
		pop		ebx
Sound1_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x10
		jz		Sound1_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		dword ptr [ebp + Offset_SoundR], ebx
Sound1_2_NotRight:
		pop		edi
Sound1_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound2Enabled]
		test	bl, bl
		jz		Sound2_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound2Frequency]
		//edx	= Sound2Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound2Ticks]
		//ecx	= Sound2Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		add		edx, 2048
		shl		edx, 3
		//edx	= (2048 - Sound2Frequency) << 3
		cmp		ecx, edx
		jae		Sound2_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound2Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		cl, byte ptr [ebp + FF00_ASM + 0x16]
		//cl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		ch, byte ptr [ebp + Offset_Sound2Stage]
		//ch	= Sound2Stage
		imul	ebx, edi
		test	cl, 0x80
		jnz		Sound2_2_50_75

		shr		cl, 6
		and		cl, 1
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_50_75:
		shr		cl, 6
		add		cl, cl
		dec		cl
		cmp		ch, cl
		jbe		Sound2_2_ToBuffer
		neg		ebx
		jmp		Sound2_2_ToBuffer

Sound2_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound2_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ebx, dword ptr [Volume + 4 * ebx]
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		//dh	= Sound2Stage
		imul	ebx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_2

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_50_75_2:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_ChangeStage
		neg		ebx
		jmp		Sound2_2_ChangeStage

Sound2_2_ChangeStage2:
		mov		ecx, eax
		mov		dh, byte ptr [ebp + Offset_Sound2Stage]
		xor		ebx, ebx
Sound2_2_ChangeStage:
		mov		edi, ecx
		//dh	= Sound2Stage
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound2Ticks], 0
		inc		dh
		and		dh, 7
		mov		byte ptr [ebp + Offset_Sound2Stage], dh
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound2Volume]
		//bl	= Sound2Volume
		mov		dl, byte ptr [ebp + FF00_ASM + 0x16]
		//dl	= [FF16]
		mov		ecx, dword ptr [Volume + 4 * ecx]
		//dh	= Sound2Stage
		imul	ecx, edi
		test	dl, 0x80
		jnz		Sound2_2_50_75_3

		shr		dl, 6
		and		dl, 1
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		jmp		Sound2_2_MixEbxEcx

Sound2_2_50_75_3:
		shr		dl, 6
		add		dl, dl
		dec		dl
		cmp		dh, dl
		jbe		Sound2_2_MixEbxEcx
		neg		ecx
		//jmp		Sound2_2_MixEbxEcx
Sound2_2_MixEbxEcx:
		add		ebx, ecx

Sound2_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x02
		jz		Sound2_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
		pop		ebx
Sound2_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x20
		jz		Sound2_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound2_2_NotRight:

		pop		edi

Sound2_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound3Enabled]
		test	bl, bl
		jz		Sound3_2_Off

		push	edi

		mov		edx, dword ptr [ebp + Offset_Sound3Frequency]
		//edx	= Sound3Frequency
		mov		ecx, dword ptr [ebp + Offset_Sound3Ticks]
		//ecx	= Sound3Ticks
		neg		edx
		add		ecx, edi
		//ecx	= Sound3Ticks + Ticks
		add		edx, 2048
		//shl		edx, 3
		//edx	= 2048 - Sound3Frequency
		cmp		ecx, edx
		jae		Sound3_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume
		test	ch, 0x20
		jnz		Sound3_2_VolumeSet
		mov		bl, 8
		jmp		Sound3_2_VolumeSet
Sound3_2_HighVolume:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet
Sound3_2_VolumeSet:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ToBuffer

Sound3_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound3_2_ChangeStage2

		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound3Ticks], ecx
		mov		edx, ecx
		//ecx	free
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		bl, cl
		//bl	= Sound3Stage
		and		cl, 1
		shr		bl, 1
		shl		cl, 2
		mov		bl, byte ptr [ebp + FF00_ASM + 0x30 + ebx]
		shr		bl, cl
		//bl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume2
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		mov		bl, 8
		jmp		Sound3_2_VolumeSet2
Sound3_2_HighVolume2:
		shr		bl, 1
		or		bl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet2
		shr		bl, 1
		or		bl, 8
		//jmp		Sound3_2_VolumeSet2
Sound3_2_VolumeSet2:
		shl		bl, 4
		sub		ebx, 0x80
		imul	ebx, edi
		jmp		Sound3_2_ChangeStage

Sound3_2_ChangeStage2:
		mov		edx, eax
		xor		ebx, ebx
Sound3_2_ChangeStage:
		mov		cl, byte ptr [ebp + Offset_Sound3Stage]
		//cl	= Sound3Stage
		mov		edi, edx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound3Ticks], 0
		inc		cl
		and		cl, 31
		mov		byte ptr [ebp + Offset_Sound3Stage], cl
		xor		edx, edx
		mov		ch, byte ptr [ebp + FF00_ASM + 0x1C]
		//ch	= [FF1C]
		mov		dl, cl
		//dl	= Sound3Stage
		and		cl, 1
		shr		dl, 1
		shl		cl, 2
		mov		dl, byte ptr [ebp + FF00_ASM + 0x30 + edx]
		shr		dl, cl
		//dl	= wave data
		test	ch, 0x40
		jnz		Sound3_2_HighVolume3
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		mov		dl, 8
		jmp		Sound3_2_VolumeSet3
Sound3_2_HighVolume3:
		shr		dl, 1
		or		dl, 8
		test	ch, 0x20
		jz		Sound3_2_VolumeSet3
		shr		dl, 1
		or		dl, 8
		//jmp		Sound3_2_VolumeSet3
Sound3_2_VolumeSet3:
		shl		dl, 4
		sub		edx, 0x80
		imul	edx, edi
		add		ebx, edx
		//jmp		Sound3_2_ToBuffer

Sound3_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound2Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x04
		jz		Sound3_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundL]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundL], edi
		pop		ebx
Sound3_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x40
		jz		Sound3_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edi, dword ptr [ebp + Offset_SoundR]
		add		edi, ebx
		mov		dword ptr [ebp + Offset_SoundR], edi
Sound3_2_NotRight:

		pop		edi

Sound3_2_Off:



		mov		bl, byte ptr [ebp + Offset_Sound4Enabled]
		test	bl, bl
		jz		Sound4_2_Off

		mov		ecx, dword ptr [ebp + Offset_Sound4Ticks]
		//ecx	= Sound4Ticks
		mov		edx, dword ptr [ebp + Offset_Sound4Frequency]
		//edx	= Sound4Frequency
		//neg		edx
		add		ecx, edi
		//ecx	= Sound2Ticks + Ticks
		//add		edx, 2048
		//shl		edx, 3
		//edx	= Sound4Frequency
		cmp		ecx, edx
		jae		Sound4_2_StageWillChange

		//edi	= Ticks

		//edx	free
		xor		ebx, ebx
		mov		dword ptr [ebp + Offset_Sound4Ticks], ecx
		//ecx	free
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		cl, byte ptr [ebp + Offset_Sound4Bit]
		//cl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	cl, cl
		jz		Sound4_2_ToBuffer
		neg		ebx
		jmp		Sound4_2_ToBuffer

Sound4_2_StageWillChange:
		sub		ecx, edx
		//edx	free
		sub		edi, ecx
		jbe		Sound4_2_ChangeStage2

		xor		ebx, ebx
		mov		bl, byte ptr [ebp + Offset_Sound4Volume]
		//bl	= Sound4Volume
		mov		dl, byte ptr [ebp + Offset_Sound4Bit]
		//dl	= Sound4Bit
		mov		ebx, dword ptr [Volume + 4 * ebx]
		imul	ebx, edi
		test	dl, dl
		jz		Sound4_2_ChangeStage
		neg		ebx
		jmp		Sound4_2_ChangeStage

Sound4_2_ChangeStage2:
		mov		ecx, eax
		xor		ebx, ebx
Sound4_2_ChangeStage:
		mov		edi, ecx
		//edi	= Ticks
		mov		dword ptr [ebp + Offset_Sound4Ticks], 0
		push	eax
		call	rand
		and		al, 1
		mov		dl, al
		mov		byte ptr [ebp + Offset_Sound4Bit], al
		pop		eax
		xor		ecx, ecx
		mov		cl, byte ptr [ebp + Offset_Sound4Volume]
		//cl	= Sound4Volume
		mov		ecx, dword ptr [Volume + 4 * ecx]
		imul	ecx, edi
		test	dl, dl
		jnz		Sound4_2_MixEbxEcx
		neg		ecx
		//jmp		Sound4_2_MixEbxEcx

Sound4_2_MixEbxEcx:
		add		ebx, ecx

Sound4_2_ToBuffer:
		//ebx	= Ticks * +-Volume[Sound4Volume]
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x08
		jz		Sound4_2_NotLeft
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		push	ebx
		not		cl
		and		cl, 0x07
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundL]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundL], edx
		pop		ebx
Sound4_2_NotLeft:
		mov		cl, byte ptr [ebp + FF00_ASM + 0x25]
		test	cl, 0x80
		jz		Sound4_2_NotRight
		mov		cl, byte ptr [ebp + FF00_ASM + 0x24]
		shr		cl, 4
		not		cl
		and		cl, 7
		sar		ebx, cl
		mov		edx, dword ptr [ebp + Offset_SoundR]
		add		edx, ebx
		mov		dword ptr [ebp + Offset_SoundR], edx
Sound4_2_NotRight:

Sound4_2_Off:



Sound_NoUpdate:
		mov		ecx, ebp



		//******
		//Serial

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		ebx, dword ptr [ecx + Offset_SerialTicks]
		test	ebx, ebx
		jz		NoSerialTransfer
		sub		ebx, eax
		ja		NoSerialBitShift

		test	esi, GB_LINKCABLE
		jz		NoLinkCable
		or		esi, GB_SERIALBIT | GB_EXITLOOP
		jmp		SerialFlagSet

NoLinkCable:
		mov		dx, word ptr [ecx + FF00_ASM + 0x01]
		add		ebx, 128
		and		dh, 0x81
		cmp		dh, 0x81
		jne		StopSerialTransfer
		mov		dh, byte ptr [ecx + Offset_SerialBit]
		shl		dl, 1
		inc		dh
		cmp		dh, 8
		mov		byte ptr [ecx + FF00_ASM + 0x01], dl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F01], MEM_CHANGED
		mov		byte ptr [ecx + Offset_SerialBit], dh
		jb		SerialBitShiftNotComplete
		mov		dl, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dl, 0x08
		mov		byte ptr [ecx + FF00_ASM + 0x02], 0x7F
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F02], MEM_CHANGED
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dl
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

StopSerialTransfer:
		xor		ebx, ebx

SerialBitShiftNotComplete:
SerialFlagSet:
NoSerialBitShift:
		mov		dword ptr [ecx + Offset_SerialTicks], ebx
NoSerialTransfer:


		//*******
		//Divider

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + Offset_DIV_Ticks]
		//bl	= DIV_Ticks
		sub		bl, al	//DIV_Ticks -= Ticks
		jnc		DIV_NotEnoughTicks
		mov		dl, byte ptr [ecx + FF00_ASM + 0x04]
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F04], MEM_CHANGED
		//dl	= FF00(0x04)
		add		bl, 32	//DIV_Ticks += 32
		inc		dl
		mov		byte ptr [ecx + FF00_ASM + 0x04], dl
		//dl	free
		//edx	free
DIV_NotEnoughTicks:
		mov		byte ptr [ecx + Offset_DIV_Ticks], bl
		//bl	free
		//ebx	free

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags


		//*****
		//Timer

		//ecx	= this
		//eax	= Ticks
		//esi	= Flags

		mov		bl, byte ptr [ecx + FF00_ASM + 0x07]
		test	bl, 0x04
		jz		TimerDisabled
		mov		ebx, dword ptr [ecx + Offset_TIMA_Ticks]
		//ebx	= TIMA_Ticks
		add		ebx, eax
		test	byte ptr [ecx + FF00_ASM + 0x4D], 0x80
		jz		Timer_FastCPU
		add		ebx, eax
Timer_FastCPU:
		//eax	free
		mov		edx, dword ptr [ecx + Offset_Hz]
		//edx	= Hz
		//mov		dword ptr [ecx + Offset_TIMA_Ticks], eax
		cmp		ebx, edx
		jb		TIMA_NotEnoughTicks

		sub		ebx, edx
		//edx	free
		mov		dl, byte ptr [ecx + FF00_ASM + 0x05]
		//dl	= FF00(0x05)
		inc		dl
		jnz		TIMA_NotZero
		mov		dl, byte ptr [ecx + FF00_ASM + 0x06]
		//dl	= FF00(0x06) (new value of FF00(0x05))
		mov		dh, byte ptr [ecx + FF00_ASM + 0x0F]
		or		dh, 0x04
		mov		byte ptr [ecx + FF00_ASM + 0x0F], dh
		//or		byte ptr [ecx + FF00_ASM + 0x0F], 0x04
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED
TIMA_NotZero:
		mov		byte ptr [ecx + FF00_ASM + 0x05], dl
		//dl	free
		//edx	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F05], MEM_CHANGED

TIMA_NotEnoughTicks:
		mov		dword ptr [ecx + Offset_TIMA_Ticks], ebx
		//ebx	free

TimerDisabled:
		//ecx	= this
		//esi	= Flags


		//**********
		//Interrupts

		//ecx	= this
		//esi	= Flags

		test	esi, GB_IE
		jz		NoInterrupt
		mov		ah, byte ptr [ecx + FF00_ASM + 0x0F]
		//ah	= FF00(0x0F)
		mov		al, byte ptr [ecx + FF00_ASM + 0xFF]
		and		al, ah
		//al	= FF00(0xFF) & FF00(0x0F)
		test	al, 0x0F
		jz		InterruptServiced

		and		esi, ~(GB_IE | GB_ENABLEIE | GB_HALT)
		mov		dword ptr [ecx + Offset_Flags], esi


		push	ecx
		xor		ecx, ecx
		shr		al, 1
		jc		InterruptFound
TestNextInterrupt:
		inc		cl
		shr		al, 1
		jnc		TestNextInterrupt

InterruptFound:
		//al	free
		mov		bl, 1
		shl		bl, cl
		not		bl
		and		ah, bl
		//ah	= new value of FF0F
		lea		ebx, [ecx * 8 + 0x40]
		//ebx	= interrupt address
		pop		ecx
		//ecx	= this

		mov		edx, dword ptr [ecx + Offset_Reg_SP]
		//edx	= Reg_SP
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		Interrupt_WriteAccessDenied

		mov		byte ptr [ecx + FF00_ASM + 0x0F], ah
		//ah	free
		//eax	free
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F0F], MEM_CHANGED

		mov		eax, dword ptr [ecx + Offset_Reg_PC]
		//eax	= Reg_PC

		mov		word ptr [ecx + Offset_Reg_PC], bx
		//ebx	free

		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		edx, ebx
		//edx	= Reg_PC (new)
		//ebx	free

		call	RetrieveAccess
		//al	= access
		//edx	free
		test	al, MEM_BREAKPOINT
		//al	free
		jz		Int_NotBreakPoint
		or		esi, GB_EXITLOOP | GB_ERROR
Int_NotBreakPoint:

		jmp		InterruptServiced

NoInterrupt:
		//ecx	= this
		//esi	= Flags

		test	esi, GB_ENABLEIE
		jz		InterruptServiced
		or		esi, GB_IE

InterruptServiced:
		//ecx	= this
		//esi	= Flags


		//*********
		//Exit loop

		//ecx	= this
		//esi	= Flags

		test	esi, GB_EXITLOOP
		jz		ContinueLoop

ExitLoop:
		and		esi, ~(GB_EXITLOOP | GB_EXITLOOPDIRECTLY)
		mov		dword ptr [ecx + Offset_Flags], esi
		//esi	free

		pop		ebx
		pop		edx
		pop		edi
		pop		esi
		pop		ebp
		ret

ExecuteAccessDenied:
		push	ecx
		push	IDS_EMU_EXECUTEACCESSDENIED
		push	ecx
		call	AccessDenied
		add		esp, 8
		pop		ecx
		mov		esi, dword ptr [ecx + Offset_Flags]
		jmp		ExitLoop

ReadAccessDenied:
		push	ecx
		push	IDS_EMU_READACCESSDENIED
		push	ecx
		call	AccessDenied
		add		esp, 8
		pop		ecx
		mov		esi, dword ptr [ecx + Offset_Flags]
		jmp		ExitLoop

Interrupt_WriteAccessDenied:
		push	ecx
		push	IDS_EMU_WRITEACCESSDENIED_INT
		push	ecx
		call	AccessDenied
		add		esp, 8
		pop		ecx
		mov		esi, dword ptr [ecx + Offset_Flags]
		jmp		ExitLoop
	}
}


void CGameBoy::RefreshScreen()
{
	IDirect3DSurface8		*pd3ds;
	D3DLOCKED_RECT			d3dlr;
	DWORD					y, x, c;


	if (FramesToSkip)
	{
		if (FastFwd)
		{
			FramesToSkip--;
		}
		else
		{
			if (SkipNextFrame)
			{
				if (FramesSkipped < 8)
				{
					FramesSkipped++;
				}
				else
				{
					FramesToSkip = 0;
				}
			}
			else
			{
				FramesToSkip--;
			}
		}
	}
	else
	{
		FramesSkipped = 0;
		if (FastFwd)
		{
			FramesToSkip = 10;
		}
		else
		{
			if (FrameSkip == 0xFF)
			{
				if (!SkipNextFrame)
				{
					SetStartDelay();
				}
				FramesToSkip = SkipNextFrame ? 1 : 0;
			}
			else
			{
				FramesToSkip = FrameSkip;
			}
		}

		EnterCriticalSection(&csGraphic);
		if (m_pd3dd)
		{
			if (hGfxWnd && !(Settings.Fullscreen == FULLSCREEN_FIT && !Settings.Fullscreen10_9))
			{
				m_pd3dd->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
			}
			m_pd3dd->GetRenderTarget(&pd3ds);
			if (!pd3ds)
			{
				return;
			}
			if (pd3ds->LockRect(&d3dlr, NULL, 0))
			{
				Error("pd3ds->LockRect(&d3dlr, NULL, 0)");
				return;
			}
			for (y = 0; y < 144; y++)
			{
				switch (D3DDisplayFormat)
				{
				case D3DFMT_R5G6B5:
					for (x = 0; x < 160; x++)
					{
						c = *(pGBBitmap + 7 + (160 + 14) * y + x);
						c = ((c & 0x7FE0) << 1) | (c & 0x001F);
						*(WORD *)((BYTE *)d3dlr.pBits + d3dlr.Pitch * y + x * 2) = (WORD)c;
					}
					break;

				case D3DFMT_X1R5G5B5:
					CopyMemory((BYTE *)d3dlr.pBits + d3dlr.Pitch * y, pGBBitmap + 7 + (160 + 14) * y, 160 * 2);
					break;

				case D3DFMT_X8R8G8B8:
				case D3DFMT_A8R8G8B8:
					for (x = 0; x < 160; x++)
					{
						c = *(pGBBitmap + 7 + (160 + 14) * y + x);
						c = (((c & 0x7C00) << 6) | ((c & 0x03E0) << 3) | (c & 0x001F)) << 3;
						*(DWORD *)((BYTE *)d3dlr.pBits + d3dlr.Pitch * y + x * 4) = c;
					}
					break;
				}
			}
			pd3ds->UnlockRect();
			pd3ds->Release();
			if (hGfxWnd)
			{
				if (m_pd3dd->Present(&rctSource, &rctDest, hGfxWnd, NULL) == D3DERR_DEVICELOST)
				{
					LeaveCriticalSection(&csGraphic);
					SendMessage(hWnd, WM_COMMAND, ID_VIEW_FULLSCREEN, 0);
					EnterCriticalSection(&csGraphic);
				}
			}
			else
			{
				if (m_pd3dd->Present(NULL, NULL, hGBWnd, NULL) == D3DERR_DEVICELOST)
				{
					m_pd3dd->Release();
					m_pd3dd = NULL;
					InitGfx(&m_pd3dd);
				}
			}
		}
		else
		{
			InvalidateRect(hGBWnd, NULL, false);
			//PostMessage(hGBWnd, WM_PAINT, 0, 0);
		}

		LeaveCriticalSection(&csGraphic);
	}
}



void __declspec(naked) __fastcall SoundUpdate(CGameBoy *GB)
{
	//
	__asm
	{
		mov		esi, dword ptr [ecx + Offset_SoundBuffer + Offset_lpdsb]
		mov		al, byte ptr [ecx + Offset_FastFwd]
		test	esi, esi
		jz		NoSound
		test	al, al
		jnz		NoSound

		mov		ebx, dword ptr [ecx + Offset_SoundR]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		R_AboveMin
		xor		al, al
		jmp		R_WithinLimits
R_AboveMin:
		cmp		eax, 127
		jle		R_BelowMax
		mov		al, 127
R_BelowMax:
		add		al, 0x80

R_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundR], 0

		//mov		edi, eax
		push	eax


		mov		ebx, dword ptr [ecx + Offset_SoundL]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		L_AboveMin
		xor		al, al
		jmp		L_WithinLimits
L_AboveMin:
		cmp		eax, 127
		jle		L_BelowMax
		mov		al, 127
L_BelowMax:
		add		al, 0x80

L_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundL], 0
		push	eax


		mov		esi, dword ptr [ecx + Offset_pAVISoundBuffer]
		test	esi, esi
		jz		NoAVI
		mov		ebx, dword ptr [ecx + Offset_dwAVISoundPos]
		cmp		ebx, 22050 * 2
		jae		BufferFull
		pop		eax
		pop		edx
		push	edx
		push	eax
		mov		byte ptr [esi + ebx], dl
		mov		byte ptr [esi + ebx + 1], al
		add		ebx, 2
		mov		dword ptr [ecx + Offset_dwAVISoundPos], ebx
BufferFull:
NoAVI:


		lea		edi, [ecx + Offset_SoundBuffer]
		lea		edi, [ecx + Offset_SoundBuffer + Offset_csSound]
		push	edi
		mov		esi, ecx
		call	dword ptr [EnterCriticalSection]

		mov		ebx, dword ptr [esi + Offset_SoundBuffer + Offset_dwPosition]
		pop		eax
		pop		edx
		mov		byte ptr [esi + Offset_SoundBuffer + Offset_Buffer + ebx], dl
		mov		byte ptr [esi + Offset_SoundBuffer + Offset_Buffer + ebx + 1], al
		add		ebx, 2
		mov		dword ptr [esi + Offset_SoundBuffer + Offset_dwPosition], ebx

		push	edi
		call	dword ptr [LeaveCriticalSection]


		mov		al, byte ptr [esi + Offset_SoundTicks]
		add		al, 96
		mov		byte ptr [esi + Offset_SoundTicks], al

		mov		ebx, dword ptr [esi + Offset_SoundBuffer + Offset_dwPosition]
		cmp		ebx, BufferSize * 2
		jb		BufferFilled
Delay:
		push	0
		call	dword ptr [Sleep]
		mov		ebx, dword ptr [esi + Offset_SoundBuffer + Offset_dwPosition]
		cmp		ebx, BufferSize * 2
		jae		Delay

BufferFilled:
		ret


NoSound:
		mov		al, byte ptr [ecx + Offset_SoundTicks]
		add		al, 96
		mov		byte ptr [ecx + Offset_SoundTicks], al

		mov		ebx, dword ptr [ecx + Offset_pAVISoundBuffer]
		test	ebx, ebx
		jnz		AVI
		ret
AVI:
		mov		ebx, dword ptr [ecx + Offset_SoundR]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		AVI_R_AboveMin
		xor		al, al
		jmp		AVI_R_WithinLimits
AVI_R_AboveMin:
		cmp		eax, 127
		jle		AVI_R_BelowMax
		mov		al, 127
AVI_R_BelowMax:
		add		al, 0x80

AVI_R_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundR], 0

		//mov		edi, eax
		push	eax


		mov		ebx, dword ptr [ecx + Offset_SoundL]
		mov		eax, 0x2AAAAAAB
		imul	ebx
		sar		edx, 6
		mov		eax, edx
		shr		edx, 0x1F
		add		eax, edx

		cmp		eax, -128
		jge		AVI_L_AboveMin
		xor		al, al
		jmp		AVI_L_WithinLimits
AVI_L_AboveMin:
		cmp		eax, 127
		jle		AVI_L_BelowMax
		mov		al, 127
AVI_L_BelowMax:
		add		al, 0x80

AVI_L_WithinLimits:
		mov		dword ptr [ecx + Offset_SoundL], 0
		push	eax


		mov		esi, dword ptr [ecx + Offset_pAVISoundBuffer]
		pop		eax
		pop		edx
		mov		ebx, dword ptr [ecx + Offset_dwAVISoundPos]
		cmp		ebx, 22050 * 2
		jae		AVI_BufferFull
		mov		byte ptr [esi + ebx], dl
		mov		byte ptr [esi + ebx + 1], al
		add		ebx, 2
		mov		dword ptr [ecx + Offset_dwAVISoundPos], ebx
AVI_BufferFull:
		ret
	}
}



BOOL CGameBoy::RestoreSound()
{
	ZeroMemory(&SoundBuffer, sizeof(SoundBuffer));
	NewSoundBuffer(&SoundBuffer);
	SoundBuffer.IsPlaying = true;

	return false;
}



void CGameBoy::CloseSound()
{
	if (!SoundBuffer.lpdsb)
	{
		return;
	}

	SoundBuffer.Close = true;

	while (SoundBuffer.Close)
	{
		Sleep(0);
	}
}



void CloseSound_asm(CGameBoy *pGameBoy)
{
	pGameBoy->CloseSound();
}

