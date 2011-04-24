/*
 * Z80.cpp	(part of Game Lad)
 *
 *
 * Emulates all operation codes in a gameboy.
 *
 *
 * void *OpCode[256]
 *	Pointers to all functions in this file (they are called through this list).
 *
 * BYTE __fastcall OpCode_...(CGameBoy *GB, DWORD PC)
 *	All functions should be declared like this. EAX, EBX, EDX may be altered.
 *
 */

#include	<windows.h>

#define		Z80_CPP
#include	"Game Lad.h"
#include	"Debugger.h"
#include	"resource.h"



/*BYTE	SgbCmd = 0;
DWORD	SgbCmdPtr = 0;
BYTE	SgbCmdBuffer[16 * 7];
DWORD	SgbCmdNBit;

void SendSgb(CGameBoy *GB, BYTE Data)
{
	DWORD	Stop;


	if (Data == 0)
	{
		SgbCmd = 1;
		SgbCmdPtr = 0;
		SgbCmdNBit = 0;
		GB->SgbReturn = 0x00;
		return;
	}
	if (SgbCmd)
	{
		if (Data != 0x30)
		{
			Data = (Data >> 4) & 1;
			if (++SgbCmdNBit == 129)
			{
				Stop = SgbCmdBuffer[0] & 0x03;
				Stop = Stop * 120 + 8;
				if (SgbCmdPtr == Stop)
				{
					SgbCmd = 2;
					return;
				}
				SgbCmdNBit = 0;
				return;
			}
			SgbCmdBuffer[SgbCmdPtr >> 3] &= ~(1 << (SgbCmdPtr & 0x07));
			SgbCmdBuffer[SgbCmdPtr >> 3] |= Data << (SgbCmdPtr & 0x07);
			SgbCmdPtr++;
		}
		else
		{
			if (SgbCmd == 2)
			{
				switch ((SgbCmdBuffer[0] & 0xF8) >> 3)
				{
				case 0x11:
					if (SgbCmdBuffer[1] & 0x01)
					{
						GB->SgbReturn = 0x01;
					}
					break;
				}
				SgbCmd = 0;
			}
		}
	}
	return;
}*/



DWORD __declspec(naked) __fastcall ReadMem(CGameBoy *GB, DWORD p)
{
	__asm
	{
		push	edx

		mov		eax, edx
		shr		edx, 12
		and		eax, 0x0FFF
		mov		edx, [ecx + Offset_MEM + edx * 4]
		mov		eax, [edx + eax]

		pop		edx
		ret
	}
}



BYTE __declspec(naked) __fastcall RetrieveAccess(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		push	edx

		mov		eax, edx
		shr		edx, 12
		and		eax, 0x0FFF
		mov		edx, dword ptr [ecx + Offset_MemStatus + edx * 4]
		mov		al, byte ptr [edx + eax]

		pop		edx
		ret
	}
}



void __declspec(naked) __fastcall SetAccess(CGameBoy *pGameBoy, DWORD Addr, BYTE Access)
{
	__asm
	{
		push	edx

		mov		eax, edx
		shr		edx, 12
		and		eax, 0x0FFF
		add		eax, dword ptr [ecx + Offset_MemStatus + edx * 4]
		mov		dl, byte ptr [esp + 8]
		mov		byte ptr [eax], dl
		pop		edx

		ret		4
	}
}



/*__forceinline __declspec(naked) __fastcall RetrieveAccessAddress(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		push	edx

		mov		eax, edx
		shr		edx, 12
		and		eax, 0x0FFF
		add		eax, dword ptr [ecx + Offset_MemStatus + edx * 4]

		pop		edx
		ret
	}
}*/
#define			RetrieveAccessAddress										\
		__asm	push	edx													\
		__asm	mov		eax, edx											\
		__asm	shr		edx, 12												\
		__asm	and		eax, 0x0FFF											\
		__asm	add		eax, dword ptr [ecx + Offset_MemStatus + edx * 4]	\
		__asm	pop		edx



void __fastcall WriteAccessNeeded(CGameBoy *pGameBoy, DWORD Addr)
{
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Access violation.", "Game Lad", MB_ICONWARNING | MB_OK);
	if (pGameBoy->hThread)
	{
		PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
	}
	pGameBoy->Flags |= GB_ERROR | GB_EXITLOOPDIRECTLY;
}



BOOL __declspec(naked) __fastcall CheckWriteAccess(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		push	eax
		push	ebx
		RetrieveAccessAddress
		mov		bl, byte ptr [eax]
		test	bl, MEM_WRITE
		jz		WriteAccessDenied
		or		bl, MEM_READ | MEM_CHANGED
		mov		byte ptr [eax], bl
		pop		ebx
		pop		eax

		clc
		ret

WriteAccessDenied:
		push	ecx
		push	edx
		call	WriteAccessNeeded
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax

		stc
		ret
	}
}



BOOL __declspec(naked) __fastcall CheckWriteAccessWord(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		push	eax
		push	ebx
		push	edx
		RetrieveAccessAddress
		mov		ebx, eax
		inc		dx
		RetrieveAccessAddress
		mov		dl, byte ptr [eax]
		mov		dh, dl
		and		dl, byte ptr [ebx]
		test	dl, MEM_WRITE
		jz		WriteAccessDenied
		mov		dl, byte ptr [ebx]
		or		dh, MEM_READ | MEM_CHANGED
		or		dl, MEM_READ | MEM_CHANGED
		mov		byte ptr [eax], dh
		mov		byte ptr [ebx], dl
		pop		edx
		pop		ebx
		pop		eax

		clc
		ret

WriteAccessDenied:
		push	ecx
		call	WriteAccessNeeded
		pop		ecx
		pop		edx
		pop		ebx
		pop		eax

		stc
		ret
	}
}



void __fastcall ReadAccessNeeded(CGameBoy *pGameBoy, DWORD Addr)
{
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Access violation.", "Game Lad", MB_ICONWARNING | MB_OK);
	if (pGameBoy->hThread)
	{
		PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
	}
	pGameBoy->Flags |= GB_ERROR | GB_EXITLOOPDIRECTLY;
}



BOOL __declspec(naked) __fastcall CheckReadAccess(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		push	eax
		RetrieveAccessAddress
		test	byte ptr [eax], MEM_READ
		pop		eax
		jz		AccessDenied

		clc
		ret

AccessDenied:
		push	ebx
		push	ecx
		push	edx
		call	WriteAccessNeeded
		pop		edx
		pop		ecx
		pop		ebx

		stc
		ret
	}
}



void __declspec(naked) __fastcall Port(CGameBoy *pGameBoy, BYTE Addr)
{
	__asm
	{
		//P1
		cmp		dl, 0x00
		je		P1
		cmp		dl, 0x04
		je		Port_DIV
		cmp		dl, 0x07
		je		TAC
		cmp		dl, 0x10
		jb		NoPort
		cmp		dl, 0x14
		je		NR14
		jb		Sound1Port
		cmp		dl, 0x16
		jb		NoPort
		cmp		dl, 0x19
		je		NR24
		jb		Sound2Port
		cmp		dl, 0x1E
		je		NR34
		jb		Sound3Port
		cmp		dl, 0x23
		je		NR44
		jb		Sound4Port
		cmp		dl, 0x26
		je		NR52
		cmp		dl, 0x40
		je		LCDC
		cmp		dl, 0x41
		je		STAT
		cmp		dl, 0x46
		je		DMA

		test	byte ptr [ecx + Offset_Flags], GB_ROM_COLOR
		jz		NoPort

		//GBC ports
		cmp		dl, 0x4F
		je		VRAMBankSelect
		cmp		dl, 0x55
		je		DMA2
		cmp		dl, 0x68
		je		BCPS
		cmp		dl, 0x69
		je		BCPD
		cmp		dl, 0x6A
		je		OCPS
		cmp		dl, 0x6B
		je		OCPD
		cmp		dl, 0x70
		je		SVBK


NoPort:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx
		ret


P1:
		push	edx
		and		al, 0x30
		xor		edx, edx
		test	al, 0x10
		jnz		P1_NotDirectionKeys

		mov		dl, byte ptr [ecx + Offset_DirectionKeys]

P1_NotDirectionKeys:
		test	al, 0x20
		jnz		P1_NotButtons

		or		dl, byte ptr [ecx + Offset_Buttons]

P1_NotButtons:
		not		dl
		and		dl, 0x0F
		or		dl, al
		or		dl, 0xC0

		mov		byte ptr [ecx + FF00_ASM + 0x00], dl

		/*push	eax
		push	ebx
		push	ecx
		xor		edx, edx
		mov		dl, al
		push	edx
		push	ecx
		call	SendSgb
		add		esp, 8
		pop		ecx
		pop		ebx
		pop		eax

		push	eax
		and		al, 0x30
		cmp		al, 0x30
		jne		SgbNoReturn
		mov		al, byte ptr [ecx + Offset_SgbReturn]
		not		al
		mov		byte ptr [ecx + FF00_ASM + 0x00], al
SgbNoReturn:
		pop		eax*/

		pop		edx
		ret

		/*//SC - SIO Control
		cmp		dl, 0x02
		jne		NotSC

		test	al, 0x80
		jz		NoTransfer

		/*test	al, 0x01
		jnz		ExternalClock*/

/*		push	eax
		mov		eax, [ecx + Offset_SIOClocks]
		or		al, [ecx + Offset_SIO]
		test	eax, eax
		pop		eax
		jnz		NoChange

		mov		dword ptr [ecx + Offset_SIOClocks], 0x800
		mov		byte ptr [ecx + Offset_SIO], 0
NoChange:
		pop		ebx
		ret

/*ExternalClock:
		mov		dword ptr [ecx + Offset_SIOClocks], 0
		mov		byte ptr [ecx + Offset_SIO], SIO_ACCEPT
		mov		byte ptr [ecx + Offset_ExitLoop], 1
		pop		ebx
		ret*/

/*NoTransfer:
		mov		dword ptr [ecx + Offset_SIOClocks], 0
		mov		byte ptr [ecx + Offset_SIO], 0
		pop		ebx
		ret

NotSC:*/
Port_DIV:
		//DIV - Divider register
		mov		byte ptr [ecx + FF00_ASM + 0x04], 0
		ret

TAC:
		//TAC - Timer control
		mov		byte ptr [ecx + FF00_ASM + 0x07], al

		test	al, 1
		jz		TAC_Not1
		test	al, 2
		jz		TAC_01
		//11 = 16.384 Hz
		mov		dword ptr [ecx + Offset_Hz], 256
		ret
TAC_01:
		//01 = 262.144 Hz
		mov		dword ptr [ecx + Offset_Hz], 16
		ret
TAC_Not1:
		test	al, 2
		jz		TAC_1
		//10 = 65.536 Hz
		mov		dword ptr [ecx + Offset_Hz], 64
		ret
TAC_1:
		//00 = 4.096 Hz
		mov		dword ptr [ecx + Offset_Hz], 1024
		ret

NR14:
		//NR14
		test	al, 0x80
		jz		NotNR14On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x01

NotNR14On:
Sound1Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound1
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR24:
		//NR24
		test	al, 0x80
		jz		NotNR24On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x02

NotNR24On:
Sound2Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound2
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR34:
		//NR34
		test	al, 0x80
		jz		NotNR34On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x04

NotNR34On:
Sound3Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound3
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR44:
		//NR44
		test	al, 0x80
		jz		NotNR44On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x08

NotNR44On:
Sound4Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	edx
#endif //_DEBUG
		push	ecx
		call	Sound4
		pop		ecx
#ifdef _DEBUG
		pop		edx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR52:
		//NR52
		mov		byte ptr [ecx + FF00_ASM + 0x26], al
#ifdef _DEBUG
		push	eax
		push	ebx
		push	edx
#endif //_DEBUG
		push	ecx
		call	Sound1
		mov		ecx, dword ptr [esp]
		call	Sound2
		mov		ecx, dword ptr [esp]
		call	Sound3
		mov		ecx, dword ptr [esp]
		call	Sound4
		pop		ecx
#ifdef _DEBUG
		pop		edx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

LCDC:
		//LCDC

		/*test	byte ptr [ebx], 0x80
		jz		LCDC_LcdOff

		test	al, 0x80
		jnz		LCDCNewValue

		mov		dl, [ecx + FF00_ASM + 0x41]
		and		dl, 3
		cmp		dl, 1
		mov		dl, 0x40
		je		LCDCNewValue
		or		al, 0x80

LCDC_LcdOff:*/
		/*push	eax
		shr		al, 7
		dec		al
		mov		[ecx + Offset_DrawLineMask], al
		pop		eax*/
//LCDCNewValue:
		mov		byte ptr [ecx + FF00_ASM + 0x40], al
		ret

STAT:
		//STAT
		push	eax
		and		al, ~3
		mov		ah, byte ptr [ecx + FF00_ASM + 0x41]
		and		ah, 0x83
		or		al, ah
		mov		byte ptr [ecx + FF00_ASM + 0x41], al
		pop		eax
		ret

DMA:
		//DMA
		push	eax
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		bh, al
		and		eax, 0xFF
		and		ebx, 0x0F00
		shr		eax, 4

		mov		esi, dword ptr [ecx + Offset_MEM + eax * 4]
		add		esi, ebx
		lea		edi, [ecx + FF00_ASM - 0x100]
		mov		ecx, 0x100 / 4
		rep		movsd
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		pop		eax

		ret

VRAMBankSelect:
		//VRAM bank select
		and		byte ptr [ecx + FF00_ASM + 0x4F], ~1

		push	eax
		and		eax, 1
		or		byte ptr [ecx + FF00_ASM + 0x4F], al
		shl		eax, 13
		add		eax, Offset_MEM_VRAM
		add		eax, ecx
		mov		dword ptr [ecx + Offset_MEM + 0x08 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x09 * 4], eax
		pop		eax
		ret

DMA2:
		//HDMA/GDMA
		test	al, 0x80
		jnz		HDMA

		push	edx

		mov		edx, dword ptr [ecx + Offset_Flags]
		test	edx, GB_HDMA
		jz		GDMA

		//Stop HDMA
		and		edx, ~GB_HDMA
		mov		byte ptr [ecx + FF00_ASM + 0x55], al
		mov		dword ptr [ecx + Offset_Flags], edx

		pop		edx
		ret

GDMA:
		push	esi
		push	edi
		push	ebx
		push	eax

		mov		byte ptr [ecx + FF00_ASM + 0x55], al

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		and		ebx, 1
		shl		ebx, 13

GDMA_NotFinished:
		mov		esi, dword ptr [ecx + FF00_ASM + 0x51]
		mov		edi, dword ptr [ecx + FF00_ASM + 0x53]
		rol		si, 8
		rol		di, 8
		and		esi, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		dec		byte ptr [ecx + FF00_ASM + 0x55]

		mov		eax, esi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah

		mov		eax, esi
		shr		esi, 12
		and		eax, 0x0FFF
		mov		esi, dword ptr [ecx + Offset_MEM + esi * 4]

		mov		edx, dword ptr [esi + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		mov		edx, dword ptr [esi + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		mov		edx, dword ptr [esi + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		mov		edx, dword ptr [esi + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx

		test	byte ptr [ecx + FF00_ASM + 0x55], 0x80
		jz		GDMA_NotFinished

		pop		eax
		pop		ebx
		pop		edi
		pop		esi

		pop		edx
		ret

HDMA:
		and		al, 0x7F
		or		dword ptr [ecx + Offset_Flags], GB_HDMA
		mov		byte ptr [ecx + FF00_ASM + 0x55], al
		ret

BCPS:
		//BCPS
		mov		byte ptr [ecx + FF00_ASM + 0x68], al

		push	eax

		and		eax, 0x3F
		mov		al, byte ptr [ecx + Offset_BGP + eax]
		mov		byte ptr [ecx + FF00_ASM + 0x69], al

		pop		eax
		ret

BCPD:
		//BCPD
		push	ebx
		mov		byte ptr [ecx + FF00_ASM + 0x69], al

		xor		ebx, ebx
		mov		bl, byte ptr [ecx + FF00_ASM + 0x68]
		test	bl, 0x80
		jz		NoBCPDInc
		and		byte ptr [ecx + FF00_ASM + 0x68], ~0x40
		inc		byte ptr [ecx + FF00_ASM + 0x68]
NoBCPDInc:
		and		bl, 0x3F
		mov		byte ptr [ecx + Offset_BGP + ebx], al
		pop		ebx
		ret

OCPS:
		//OCPS
		mov		byte ptr [ecx + FF00_ASM + 0x6A], al

		push	eax

		and		eax, 0x3F
		mov		al, byte ptr [ecx + Offset_OBP + eax]
		mov		byte ptr [ecx + FF00_ASM + 0x6B], al

		pop		eax
		ret

OCPD:
		//OCPD
		push	ebx

		mov		byte ptr [ecx + FF00_ASM + 0x6B], al

		xor		ebx, ebx
		mov		bl, byte ptr [ecx + FF00_ASM + 0x6A]
		test	bl, 0x80
		jz		NoOCPDInc
		and		byte ptr [ecx + FF00_ASM + 0x6A], ~0x40
		inc		byte ptr [ecx + FF00_ASM + 0x6A]
NoOCPDInc:
		and		bl, 0x3F
		mov		byte ptr [ecx + Offset_OBP + ebx], al
		pop		ebx
		ret

SVBK:
		//SVBK
		push	ebx
		mov		byte ptr [ecx + FF00_ASM + 0x70], al
		xor		ebx, ebx
		and		al, 7
		jnz		SVBK_NotZero
		inc		al
SVBK_NotZero:
		mov		bl, al
		shl		ebx, 12
		add		ebx, ecx
		add		ebx, Offset_MEM_CPU
		mov		dword ptr [ecx + Offset_MEM + 0xD * 4], ebx
		pop		ebx
		ret
	}
}



void __declspec(naked) __fastcall LD_mem8(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		cmp		dh, 0xFF
		jne		NotPorts

		call	Port
		ret

NotPorts:
		//Special events if address points to ROM
		test	dh, 0x80
		jz		IsRom

		//Write to memory
		push	ebx

		push	edx
		mov		ebx, edx
		shr		edx, 12
		and		ebx, 0x0FFF
		add		ebx, dword ptr [ecx + Offset_MEM + edx * 4]
		pop		edx

		mov		byte ptr [ebx], al

		//Echo?
		cmp		edx, 0xD000
		jb		NoEcho
		cmp		edx, 0xFE00
		ja		NoEcho
		cmp		edx, 0xDE00
		jae		MaybeHighEcho

		mov		byte ptr [ecx + FF00_ASM - 0xF00 + edx - 0xD000], al
		pop		ebx
		ret

MaybeHighEcho:
		cmp		edx, 0xF000
		jb		NoEcho

		mov		ebx, dword ptr [ecx + Offset_MEM + 0xD * 4]
		mov		byte ptr [ebx + edx - 0xF000], al
		pop		ebx
		ret

NoEcho:
		pop		ebx
		ret

IsRom:
		cmp		dh, 0x20
		jb		EnableRam

		cmp		dh, 0x40
		jb		RomBankSelect

		cmp		dh, 0x60
		jb		RamBankSelect

		//Clock latch not implemented
		ret

RomBankSelect:
		cmp		al, byte ptr [ecx + Offset_MaxRomBank]
		ja		IllegalBank

		mov		byte ptr [ecx + Offset_ActiveRomBank], al

		push	eax
		and		eax, 0xFF
		shl		eax, 14
		add		eax, dword ptr [ecx + Offset_MEM_ROM]
		mov		dword ptr [ecx + Offset_MEM + 0x04 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x05 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x06 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x07 * 4], eax
		pop		eax

IllegalBank:
		ret

RamBankSelect:
		cmp		al, [ecx + Offset_MaxRamBank]
		ja		IllegalBank		//return

		push	eax
		and		eax, 0xFF
		shl		eax, 13
		add		eax, Offset_MEM_RAM
		add		eax, ecx
		mov		[ecx + Offset_MEM + 0x0A * 4], eax
		add		eax, 0x1000
		mov		[ecx + Offset_MEM + 0x0B * 4], eax
		pop		eax

		ret

EnableRam:
		push	eax
		and		al, 0x0F
		cmp		al, 0x0A
		mov		al, byte ptr [ecx + Offset_Flags]
		jne		DisableRam
		or		al, GB_RAMENABLE
		mov		byte ptr [ecx + Offset_Flags], al
		pop		eax
		ret

DisableRam:
		and		al, ~GB_RAMENABLE
		mov		byte ptr [ecx + Offset_Flags], al
		pop		eax
		ret
	}
}



void __declspec(naked) __fastcall Debug_Port(CGameBoy *pGameBoy, BYTE Addr)
{
	__asm
	{
		//P1
		cmp		dl, 0x00
		je		P1
		cmp		dl, 0x04
		je		Port_DIV
		cmp		dl, 0x07
		je		TAC
		cmp		dl, 0x10
		jb		NoPort
		cmp		dl, 0x14
		je		NR14
		jb		Sound1Port
		cmp		dl, 0x16
		jb		NoPort
		cmp		dl, 0x19
		je		NR24
		jb		Sound2Port
		cmp		dl, 0x1E
		je		NR34
		jb		Sound3Port
		cmp		dl, 0x23
		je		NR44
		jb		Sound4Port
		cmp		dl, 0x26
		je		NR52
		cmp		dl, 0x40
		je		LCDC
		cmp		dl, 0x41
		je		STAT
		cmp		dl, 0x46
		je		DMA

		test	byte ptr [ecx + Offset_Flags], GB_ROM_COLOR
		jz		NoPort

		//GBC ports
		cmp		dl, 0x4F
		je		VRAMBankSelect
		cmp		dl, 0x55
		je		DMA2
		cmp		dl, 0x68
		je		BCPS
		cmp		dl, 0x69
		je		BCPD
		cmp		dl, 0x6A
		je		OCPS
		cmp		dl, 0x6B
		je		OCPD
		cmp		dl, 0x70
		je		SVBK


NoPort:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx
		ret


P1:
		push	edx
		and		al, 0x30
		xor		edx, edx
		test	al, 0x10
		jnz		P1_NotDirectionKeys

		mov		dl, byte ptr [ecx + Offset_DirectionKeys]

P1_NotDirectionKeys:
		test	al, 0x20
		jnz		P1_NotButtons

		or		dl, byte ptr [ecx + Offset_Buttons]

P1_NotButtons:
		not		dl
		and		dl, 0x0F
		or		dl, al
		or		dl, 0xC0

		mov		byte ptr [ecx + FF00_ASM + 0x00], dl

		/*push	eax
		push	ebx
		push	ecx
		xor		edx, edx
		mov		dl, al
		push	edx
		push	ecx
		call	SendSgb
		add		esp, 8
		pop		ecx
		pop		ebx
		pop		eax

		push	eax
		and		al, 0x30
		cmp		al, 0x30
		jne		SgbNoReturn
		mov		al, byte ptr [ecx + Offset_SgbReturn]
		not		al
		mov		byte ptr [ecx + FF00_ASM + 0x00], al
SgbNoReturn:
		pop		eax*/

		pop		edx
		ret

		/*//SC - SIO Control
		cmp		dl, 0x02
		jne		NotSC

		test	al, 0x80
		jz		NoTransfer

		/*test	al, 0x01
		jnz		ExternalClock*/

/*		push	eax
		mov		eax, [ecx + Offset_SIOClocks]
		or		al, [ecx + Offset_SIO]
		test	eax, eax
		pop		eax
		jnz		NoChange

		mov		dword ptr [ecx + Offset_SIOClocks], 0x800
		mov		byte ptr [ecx + Offset_SIO], 0
NoChange:
		pop		ebx
		ret

/*ExternalClock:
		mov		dword ptr [ecx + Offset_SIOClocks], 0
		mov		byte ptr [ecx + Offset_SIO], SIO_ACCEPT
		mov		byte ptr [ecx + Offset_ExitLoop], 1
		pop		ebx
		ret*/

/*NoTransfer:
		mov		dword ptr [ecx + Offset_SIOClocks], 0
		mov		byte ptr [ecx + Offset_SIO], 0
		pop		ebx
		ret

NotSC:*/
Port_DIV:
		//DIV - Divider register
		mov		byte ptr [ecx + FF00_ASM + 0x04], 0
		ret

TAC:
		//TAC - Timer control
		mov		byte ptr [ecx + FF00_ASM + 0x07], al

		test	al, 1
		jz		TAC_Not1
		test	al, 2
		jz		TAC_01
		//11 = 16.384 Hz
		mov		dword ptr [ecx + Offset_Hz], 256
		ret
TAC_01:
		//01 = 262.144 Hz
		mov		dword ptr [ecx + Offset_Hz], 16
		ret
TAC_Not1:
		test	al, 2
		jz		TAC_1
		//10 = 65.536 Hz
		mov		dword ptr [ecx + Offset_Hz], 64
		ret
TAC_1:
		//00 = 4.096 Hz
		mov		dword ptr [ecx + Offset_Hz], 1024
		ret

NR14:
		//NR14
		test	al, 0x80
		jz		NotNR14On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x01
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED

NotNR14On:
Sound1Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound1
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR24:
		//NR24
		test	al, 0x80
		jz		NotNR24On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x02
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED

NotNR24On:
Sound2Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound2
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR34:
		//NR34
		test	al, 0x80
		jz		NotNR34On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x04
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED

NotNR34On:
Sound3Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	ecx
		push	edx
#endif //_DEBUG
		call	Sound3
#ifdef _DEBUG
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR44:
		//NR44
		test	al, 0x80
		jz		NotNR44On

		or		byte ptr [ecx + FF00_ASM + 0x26], 0x08
		or		byte ptr [ecx + Offset_MemStatus_CPU + 0x8F26], MEM_CHANGED

NotNR44On:
Sound4Port:
		push	edx
		and		edx, 0xFF
		mov		byte ptr [ecx + FF00_ASM + edx], al
		pop		edx

#ifdef _DEBUG
		push	eax
		push	ebx
		push	edx
#endif //_DEBUG
		push	ecx
		call	Sound4
		pop		ecx
#ifdef _DEBUG
		pop		edx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

NR52:
		//NR52
		mov		byte ptr [ecx + FF00_ASM + 0x26], al
#ifdef _DEBUG
		push	eax
		push	ebx
		push	edx
#endif //_DEBUG
		push	ecx
		call	Sound1
		mov		ecx, dword ptr [esp]
		call	Sound2
		mov		ecx, dword ptr [esp]
		call	Sound3
		mov		ecx, dword ptr [esp]
		call	Sound4
		pop		ecx
#ifdef _DEBUG
		pop		edx
		pop		ebx
		pop		eax
#endif //_DEBUG

		ret

LCDC:
		//LCDC

		/*test	byte ptr [ebx], 0x80
		jz		LCDC_LcdOff

		test	al, 0x80
		jnz		LCDCNewValue

		mov		dl, [ecx + FF00_ASM + 0x41]
		and		dl, 3
		cmp		dl, 1
		mov		dl, 0x40
		je		LCDCNewValue
		or		al, 0x80

LCDC_LcdOff:*/
		/*push	eax
		shr		al, 7
		dec		al
		mov		[ecx + Offset_DrawLineMask], al
		pop		eax*/
//LCDCNewValue:
		mov		byte ptr [ecx + FF00_ASM + 0x40], al
		ret

STAT:
		//STAT
		push	eax
		and		al, ~3
		mov		ah, byte ptr [ecx + FF00_ASM + 0x41]
		and		ah, 0x83
		or		al, ah
		mov		byte ptr [ecx + FF00_ASM + 0x41], al
		pop		eax
		ret

DMA:
		//DMA
		push	eax
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		bh, al
		and		eax, 0xFF
		and		ebx, 0x0F00
		shr		eax, 4

		mov		esi, dword ptr [ecx + Offset_MEM + eax * 4]
		add		esi, ebx
		lea		edi, [ecx + FF00_ASM - 0x100]
		lea		ebx, [ecx + Offset_MemStatus + 0x8E00]
		mov		ecx, 0x100 / 4
		rep		movsd

		mov		ecx, 0x100 / 4
DMA_NextStatusDWORD:
		or		dword ptr [ebx + 4 * ecx], MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
		dec		ecx
		jns		DMA_NextStatusDWORD

		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		pop		eax

		ret

VRAMBankSelect:
		//VRAM bank select
		and		byte ptr [ecx + FF00_ASM + 0x4F], ~1

		push	eax
		and		eax, 1
		or		byte ptr [ecx + FF00_ASM + 0x4F], al
		shl		eax, 13
		add		eax, Offset_MEM_VRAM
		add		eax, ecx
		mov		dword ptr [ecx + Offset_MEM + 0x08 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x09 * 4], eax
		add		eax, Offset_MemStatus_VRAM - Offset_MEM_VRAM - 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0x08 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0x09 * 4], eax
		pop		eax
		ret

DMA2:
		//HDMA/GDMA
		test	al, 0x80
		jnz		HDMA

		push	edx

		mov		edx, dword ptr [ecx + Offset_Flags]
		test	edx, GB_HDMA
		jz		GDMA

		//Stop HDMA
		and		edx, ~GB_HDMA
		mov		byte ptr [ecx + FF00_ASM + 0x55], al
		mov		dword ptr [ecx + Offset_Flags], edx
		pop		edx
		ret

GDMA:
		push	esi
		push	edi
		push	ebx
		push	eax

		mov		byte ptr [ecx + FF00_ASM + 0x55], al

		mov		bl, byte ptr [ecx + FF00_ASM + 0x4F]
		and		ebx, 1
		shl		ebx, 13

GDMA_NotFinished:
		mov		si, word ptr [ecx + FF00_ASM + 0x51]
		mov		di, word ptr [ecx + FF00_ASM + 0x53]
		rol		si, 8
		rol		di, 8
		and		esi, 0xFFF0
		and		edi, 0x1FF0

		add		edi, ebx

		dec		byte ptr [ecx + FF00_ASM + 0x55]

		mov		eax, esi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x52], al
		mov		byte ptr [ecx + FF00_ASM + 0x51], ah
		mov		eax, edi
		add		eax, 0x0010
		mov		byte ptr [ecx + FF00_ASM + 0x54], al
		mov		byte ptr [ecx + FF00_ASM + 0x53], ah

		mov		eax, esi
		shr		esi, 12
		and		eax, 0x0FFF
		mov		esi, dword ptr [ecx + Offset_MEM + esi * 4]

		mov		edx, dword ptr [esi + eax]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi], edx
		mov		edx, dword ptr [ecx + Offset_MemStatus + edi]
		or		edx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
		mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi], edx
		mov		edx, dword ptr [esi + eax + 0x04]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x04], edx
		mov		edx, dword ptr [ecx + Offset_MemStatus + edi + 0x04]
		or		edx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
		mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x04], edx
		mov		edx, dword ptr [esi + eax + 0x08]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x08], edx
		mov		edx, dword ptr [ecx + Offset_MemStatus + edi + 0x08]
		or		edx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
		mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x08], edx
		mov		edx, dword ptr [esi + eax + 0x0C]
		mov		dword ptr [ecx + Offset_MEM_VRAM + edi + 0x0C], edx
		mov		edx, dword ptr [ecx + Offset_MemStatus + edi + 0x0C]
		or		edx, MEM_CHANGED | (MEM_CHANGED << 8) | (MEM_CHANGED << 16) | (MEM_CHANGED << 24)
		mov		dword ptr [ecx + Offset_MemStatus_VRAM + edi + 0x0C], edx

		test	byte ptr [ecx + FF00_ASM + 0x55], 0x80
		jz		GDMA_NotFinished

		pop		eax
		pop		ebx
		pop		edi
		pop		esi

		pop		edx
		ret

HDMA:
		and		al, 0x7F
		or		dword ptr [ecx + Offset_Flags], GB_HDMA
		mov		byte ptr [ecx + FF00_ASM + 0x55], al
		ret

BCPS:
		//BCPS
		mov		byte ptr [ecx + FF00_ASM + 0x68], al

		push	eax

		and		eax, 0x3F
		mov		al, byte ptr [ecx + Offset_BGP + eax]
		mov		byte ptr [ecx + FF00_ASM + 0x69], al

		pop		eax
		ret

BCPD:
		//BCPD
		push	ebx
		mov		byte ptr [ecx + FF00_ASM + 0x69], al

		xor		ebx, ebx
		mov		bl, byte ptr [ecx + FF00_ASM + 0x68]
		test	bl, 0x80
		jz		NoBCPDInc
		and		byte ptr [ecx + FF00_ASM + 0x68], ~0x40
		inc		byte ptr [ecx + FF00_ASM + 0x68]
NoBCPDInc:
		and		bl, 0x3F
		mov		byte ptr [ecx + Offset_BGP + ebx], al
		pop		ebx
		ret

OCPS:
		//OCPS
		mov		byte ptr [ecx + FF00_ASM + 0x6A], al

		push	eax

		and		eax, 0x3F
		mov		al, byte ptr [ecx + Offset_OBP + eax]
		mov		byte ptr [ecx + FF00_ASM + 0x6B], al

		pop		eax
		ret

OCPD:
		//OCPD
		push	ebx

		mov		byte ptr [ecx + FF00_ASM + 0x6B], al

		xor		ebx, ebx
		mov		bl, byte ptr [ecx + FF00_ASM + 0x6A]
		test	bl, 0x80
		jz		NoOCPDInc
		and		byte ptr [ecx + FF00_ASM + 0x6A], ~0x40
		inc		byte ptr [ecx + FF00_ASM + 0x6A]
NoOCPDInc:
		and		bl, 0x3F
		mov		byte ptr [ecx + Offset_OBP + ebx], al
		pop		ebx
		ret

SVBK:
		//SVBK
		push	ebx
		mov		byte ptr [ecx + FF00_ASM + 0x70], al
		xor		ebx, ebx
		and		al, 7
		jnz		SVBK_NotZero
		inc		al
SVBK_NotZero:
		mov		bl, al
		shl		ebx, 12
		add		ebx, ecx
		add		ebx, Offset_MEM_CPU
		mov		dword ptr [ecx + Offset_MEM + 0xD * 4], ebx
		add		ebx, Offset_MemStatus_CPU - Offset_MEM_CPU
		mov		dword ptr [ecx + Offset_MemStatus + 0xD * 4], ebx
		pop		ebx
		ret
	}
}



void __declspec(naked) __fastcall Debug_LD_mem8(CGameBoy *pGameBoy, DWORD Addr)
{
	__asm
	{
		cmp		dh, 0xFF
		jne		NotPorts

		call	Port
		ret

NotPorts:
		//Special events if address points to ROM
		test	dh, 0x80
		jz		IsRom

		//Write to memory
		push	ebx

		push	edx
		mov		ebx, edx
		shr		edx, 12
		and		ebx, 0x0FFF
		add		ebx, dword ptr [ecx + Offset_MEM + edx * 4]
		pop		edx

		mov		byte ptr [ebx], al

		//Echo?
		cmp		edx, 0xD000
		jb		NoEcho
		cmp		edx, 0xFE00
		ja		NoEcho
		cmp		edx, 0xDE00
		jae		MaybeHighEcho

		mov		byte ptr [ecx + FF00_ASM - 0xF00 + edx - 0xD000], al
		pop		ebx
		ret

MaybeHighEcho:
		cmp		edx, 0xF000
		jb		NoEcho

		mov		ebx, dword ptr [ecx + Offset_MEM + 0xD * 4]
		mov		byte ptr [ebx + edx - 0xF000], al
		pop		ebx
		ret

NoEcho:
		pop		ebx
		ret

IsRom:
		cmp		dh, 0x20
		jb		EnableRam

		cmp		dh, 0x40
		jb		RomBankSelect

		cmp		dh, 0x60
		jb		RamBankSelect

		//Clock latch not implemented
		ret

RomBankSelect:
		cmp		al, byte ptr [ecx + Offset_MaxRomBank]
		ja		IllegalBank

		mov		byte ptr [ecx + Offset_ActiveRomBank], al

		push	eax
		and		eax, 0xFF
		shl		eax, 14
		add		eax, dword ptr [ecx + Offset_MEM_ROM]
		mov		dword ptr [ecx + Offset_MEM + 0x04 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x05 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x06 * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x07 * 4], eax
		sub		eax, dword ptr [ecx + Offset_MEM_ROM]
		add		eax, dword ptr [ecx + Offset_MemStatus_ROM]
		mov		dword ptr [ecx + Offset_MemStatus + 0x07 * 4], eax
		sub		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0x06 * 4], eax
		sub		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0x05 * 4], eax
		sub		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0x04 * 4], eax
		pop		eax

IllegalBank:
		ret

RamBankSelect:
		cmp		al, [ecx + Offset_MaxRamBank]
		ja		IllegalBank		//return

		push	eax
		and		eax, 0xFF
		shl		eax, 13
		push	eax
		lea		eax, [eax + ecx + Offset_MEM_RAM]
		mov		dword ptr [ecx + Offset_MEM + 0x0A * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MEM + 0x0B * 4], eax
		mov		al, byte ptr [ecx + Offset_Flags]
		test	al, GB_RAMENABLE
		pop		eax
		jz		RamDisabled
		add		eax, dword ptr [ecx + Offset_MemStatus_RAM]
		mov		dword ptr [ecx + Offset_MemStatus + 0xA * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0xB * 4], eax
RamDisabled:
		pop		eax
		ret

EnableRam:
		push	eax
		and		eax, 0x0F
		cmp		al, 0x0A
		mov		al, byte ptr [ecx + Offset_Flags]
		jne		DisableRam
		or		al, GB_RAMENABLE
		mov		byte ptr [ecx + Offset_Flags], al
		mov		al, byte ptr [ecx + Offset_ActiveRamBank]
		shl		eax, 13
		add		eax, dword ptr [ecx + Offset_MemStatus_RAM]
		mov		dword ptr [ecx + Offset_MemStatus + 0xA * 4], eax
		add		eax, 0x1000
		mov		dword ptr [ecx + Offset_MemStatus + 0xB * 4], eax
		pop		eax
		ret

DisableRam:
		and		al, ~GB_RAMENABLE
		mov		byte ptr [ecx + Offset_Flags], al
		mov		dword ptr [ecx + Offset_MemStatus + 0xA * 4], offset ZeroStatus
		mov		dword ptr [ecx + Offset_MemStatus + 0xB * 4], offset ZeroStatus
		pop		eax
		ret
	}
}





void __declspec(naked) __fastcall OpCode_Undefined(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		call	ReadMem
		or		byte ptr [ecx + Offset_Flags], GB_INVALIDOPCODE
		ret
	}
}



//0x00	nop
void __declspec(naked) __fastcall OpCode_Nop(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x01	ld		bc, nnnn
void __declspec(naked) __fastcall OpCode_LD_BC_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_BC], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0x01	ld		bc, nnnn
void __declspec(naked) __fastcall Debug_OpCode_LD_BC_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_BC], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x11	ld		de, nnnn
void __declspec(naked) __fastcall OpCode_LD_DE_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_DE], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0x11	ld		de, nnnn
void __declspec(naked) __fastcall Debug_OpCode_LD_DE_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_DE], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x21	ld		hl, nnnn
void __declspec(naked) __fastcall OpCode_LD_HL_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0x21	ld		hl, nnnn
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_HL], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x31	ld		sp, nnnn
void __declspec(naked) __fastcall OpCode_LD_SP_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_SP], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0x31	ld		sp, nnnn
void __declspec(naked) __fastcall Debug_OpCode_LD_SP_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		word ptr [ecx + Offset_Reg_SP], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x06	ld		b, nn
void __declspec(naked) __fastcall OpCode_LD_B_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x06	ld		b, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_B_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x0E	ld		c, nn
void __declspec(naked) __fastcall OpCode_LD_C_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_C], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x0E	ld		c, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_C_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_C], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x16	ld		d, nn
void __declspec(naked) __fastcall OpCode_LD_D_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x16	ld		d, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_D_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x1E	ld		e, nn
void __declspec(naked) __fastcall OpCode_LD_E_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_E], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x1E	ld		e, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_E_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_E], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x26	ld		h, nn
void __declspec(naked) __fastcall OpCode_LD_H_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x26	ld		h, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_H_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x2E	ld		l, nn
void __declspec(naked) __fastcall OpCode_LD_L_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x2E	ld		l, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_L_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x3E	ld		a, nn
void __declspec(naked) __fastcall OpCode_LD_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x3E	ld		a, nn
void __declspec(naked) __fastcall Debug_OpCode_LD_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x0A	ld		a, [bc]
void __declspec(naked) __fastcall OpCode_LD_A_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_BC]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		mov		eax, 2
		ret
	}
}

//0x0A	ld		a, [bc]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_BC]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x1A	ld		a, [de]
void __declspec(naked) __fastcall OpCode_LD_A_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_DE]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		mov		eax, 2
		ret
	}
}

//0x1A	ld		a, [de]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_DE]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x46	ld		b, [hl]
void __declspec(naked) __fastcall OpCode_LD_B_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al

		mov		eax, 2
		ret
	}
}

//0x46	ld		b, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_B_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x4E	ld		c, [hl]
void __declspec(naked) __fastcall OpCode_LD_C_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_C], al

		mov		eax, 2
		ret
	}
}

//0x4E	ld		c, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_C_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_C], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x56	ld		d, [hl]
void __declspec(naked) __fastcall OpCode_LD_D_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al

		mov		eax, 2
		ret
	}
}

//0x56	ld		d, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_D_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x5E	ld		e, [hl]
void __declspec(naked) __fastcall OpCode_LD_E_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_E], al

		mov		eax, 2
		ret
	}
}

//0x5E	ld		e, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_E_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_E], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x66	ld		h, [hl]
void __declspec(naked) __fastcall OpCode_LD_H_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al

		mov		eax, 2
		ret
	}
}

//0x66	ld		h, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_H_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x6E	ld		l, [hl]
void __declspec(naked) __fastcall OpCode_LD_L_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al

		mov		eax, 2
		ret
	}
}

//0x6E	ld		l, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_L_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x7E	ld		a, [hl]
void __declspec(naked) __fastcall OpCode_LD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		mov		eax, 2
		ret
	}
}

//0x7E	ld		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x2A	ldi		a, [hl]
void __declspec(naked) __fastcall OpCode_LDI_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		inc		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		mov		eax, 2
		ret
	}
}

//0x2A	ldi		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LDI_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		inc		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x3A	ldd		a, [hl]
void __declspec(naked) __fastcall OpCode_LDD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		dec		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		mov		eax, 2
		ret
	}
}

//0x3A	ldd		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_LDD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		dec		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x02	ld		[bc], a
void __declspec(naked) __fastcall OpCode_LD_BC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_BC]
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x02	ld		[bc], a
void __declspec(naked) __fastcall Debug_OpCode_LD_BC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_BC]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x12	ld		[de], a
void __declspec(naked) __fastcall OpCode_LD_DE_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_DE]
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x12	ld		[de], a
void __declspec(naked) __fastcall Debug_OpCode_LD_DE_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_DE]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x70	ld		[hl], b
void __declspec(naked) __fastcall OpCode_LD_HL_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_B]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x70	ld		[hl], b
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_B]
		call	LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x71	ld		[hl], c
void __declspec(naked) __fastcall OpCode_LD_HL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_C]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x71	ld		[hl], c
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_C]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x72	ld		[hl], d
void __declspec(naked) __fastcall OpCode_LD_HL_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_D]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x72	ld		[hl], d
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_D]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x73	ld		[hl], e
void __declspec(naked) __fastcall OpCode_LD_HL_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_E]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x73	ld		[hl], e
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_E]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x74	ld		[hl], h
void __declspec(naked) __fastcall OpCode_LD_HL_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_H]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x74	ld		[hl], h
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_H]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x75	ld		[hl], l
void __declspec(naked) __fastcall OpCode_LD_HL_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_L]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x75	ld		[hl], l
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_L]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x77	ld		[hl], a
void __declspec(naked) __fastcall OpCode_LD_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	LD_mem8

		mov		eax, 2
		ret
	}
}

//0x77	ld		[hl], a
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x36	ld		[hl], nn
void __declspec(naked) __fastcall OpCode_LD_HL_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	LD_mem8

		mov		eax, 3
		ret
	}
}

//0x36	ld		[hl], nn
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		call	Debug_LD_mem8

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x22	ldi		[hl], a
void __declspec(naked) __fastcall OpCode_LDI_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_A]
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	LD_mem8
		inc		word ptr [ecx + Offset_Reg_HL]

		mov		eax, 2
		ret
	}
}

//0x22	ldi		[hl], a
void __declspec(naked) __fastcall Debug_OpCode_LDI_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8
		inc		word ptr [ecx + Offset_Reg_HL]

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x32	ldd		[hl], a
void __declspec(naked) __fastcall OpCode_LDD_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_A]
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	LD_mem8
		dec		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		mov		eax, 2
		ret
	}
}

//0x32	ldd		[hl], a
void __declspec(naked) __fastcall Debug_OpCode_LDD_HL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8
		dec		edx
		mov		word ptr [ecx + Offset_Reg_HL], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x08	ld		[nnnn], sp
void __declspec(naked) __fastcall OpCode_LD_nnnn_SP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		ebx, ebx
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		edx, ebx

		mov		eax, dword ptr [ecx + Offset_Reg_SP]
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		eax, 5
		ret
	}
}

//0x08	ld		[nnnn], sp
void __declspec(naked) __fastcall Debug_OpCode_LD_nnnn_SP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		ebx, ebx
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		push	edx

		mov		edx, ebx
		call	CheckWriteAccessWord
		jc		AccessDenied

		mov		eax, dword ptr [ecx + Offset_Reg_SP]
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		pop		edx
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 5
AccessDenied:
		ret
	}
}

//0x03	inc		bc
void __declspec(naked) __fastcall OpCode_INC_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ax, word ptr [ecx + Offset_Reg_BC]
		inc		ax
		mov		word ptr [ecx + Offset_Reg_BC], ax

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x13	inc		de
void __declspec(naked) __fastcall OpCode_INC_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		word ptr [ecx + Offset_Reg_DE]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x23	inc		hl
void __declspec(naked) __fastcall OpCode_INC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		word ptr [ecx + Offset_Reg_HL]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x33	inc		sp
void __declspec(naked) __fastcall OpCode_INC_SP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		word ptr [ecx + Offset_Reg_SP]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x0B	dec		bc
void __declspec(naked) __fastcall OpCode_DEC_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ax, word ptr [ecx + Offset_Reg_BC]
		dec		ax
		mov		word ptr [ecx + Offset_Reg_BC], ax

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x1B	dec		de
void __declspec(naked) __fastcall OpCode_DEC_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		dec		word ptr [ecx + Offset_Reg_DE]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x2B	dec		hl
void __declspec(naked) __fastcall OpCode_DEC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		dec		word ptr [ecx + Offset_Reg_HL]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x3B	dec		sp
void __declspec(naked) __fastcall OpCode_DEC_SP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		dec		word ptr [ecx + Offset_Reg_SP]

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x04	inc		b
void __declspec(naked) __fastcall OpCode_INC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_B], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x0C	inc		c
void __declspec(naked) __fastcall OpCode_INC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_C], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x14	inc		d
void __declspec(naked) __fastcall OpCode_INC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_D], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x1C	inc		e
void __declspec(naked) __fastcall OpCode_INC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_E], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x24	inc		h
void __declspec(naked) __fastcall OpCode_INC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_H], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x2C	inc		l
void __declspec(naked) __fastcall OpCode_INC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		and		ah, Flag_C
		mov		bh, bl
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		bl
		jnz		NotZero
		or		ah, Flag_Z
NotZero:

		mov		byte ptr [ecx + Offset_Reg_L], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x3C	inc		a
void __declspec(naked) __fastcall OpCode_INC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ax, word ptr [ecx + Offset_Reg_AF]
		and		al, Flag_C
		mov		bh, ah
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		inc		ah
		jnz		NotZero
		or		al, Flag_Z
NotZero:

		mov		word ptr [ecx + Offset_Reg_AF], ax

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x34	inc		[hl]
void __declspec(naked) __fastcall OpCode_INC__HL_(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		and		ah, Flag_C
		mov		bh, al
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		al
		jnz		NotZero
		or		ah, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], ah
		call	LD_mem8

		mov		eax, 3
		ret
	}
}

//0x34	inc		[hl]
void __declspec(naked) __fastcall Debug_OpCode_INC__HL_(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		call	ReadMem
		and		ah, Flag_C
		mov		bh, al
		and		bh, 0x0F
		cmp		bh, 0x0F
		jne		NoHalfCarry
		or		ah, Flag_H
NoHalfCarry:
		inc		al
		jnz		NotZero
		or		ah, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x05	dec		b
void __declspec(naked) __fastcall OpCode_DEC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_B], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x0D	dec		c
void __declspec(naked) __fastcall OpCode_DEC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_C], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x15	dec		d
void __declspec(naked) __fastcall OpCode_DEC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_D], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x1D	dec		e
void __declspec(naked) __fastcall OpCode_DEC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_E], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x25	dec		h
void __declspec(naked) __fastcall OpCode_DEC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_H], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x2D	dec		l
void __declspec(naked) __fastcall OpCode_DEC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		and		ah, Flag_C
		or		ah, Flag_N
		test	bl, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		bl
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:

		mov		byte ptr [ecx + Offset_Reg_L], bl
		mov		byte ptr [ecx + Offset_Reg_F], ah

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x3D	dec		a
void __declspec(naked) __fastcall OpCode_DEC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]
		and		bl, Flag_C
		or		bl, Flag_N
		test	bh, 0x0F
		jnz		NotZero
		or		bl, Flag_H
NotZero:
		dec		bh
		jnz		NotZero2
		or		bl, Flag_Z
NotZero2:

		mov		word ptr [ecx + Offset_Reg_AF], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x35	dec		[hl]
void __declspec(naked) __fastcall OpCode_DEC__HL_(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		ah, byte ptr [ecx + Offset_Reg_F]
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		and		ah, Flag_C
		or		ah, Flag_N
		test	al, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		al
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:
		mov		byte ptr [ecx + Offset_Reg_F], ah
		call	LD_mem8

		mov		eax, 3
		ret
	}
}

//0x35	dec		[hl]
void __declspec(naked) __fastcall Debug_OpCode_DEC__HL_(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		ah, byte ptr [ecx + Offset_Reg_F]
		call	ReadMem
		and		ah, Flag_C
		or		ah, Flag_N
		test	al, 0x0F
		jnz		NotZero
		or		ah, Flag_H
NotZero:
		dec		al
		jnz		NotZero2
		or		ah, Flag_Z
NotZero2:
		mov		byte ptr [ecx + Offset_Reg_F], ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x07	rlca
void __declspec(naked) __fastcall OpCode_RLCA(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ah, byte ptr [ecx + Offset_Reg_A]
		mov		al, ah

		rol		ah, 1
		shr		al, 3
		and		al, Flag_C
		mov		word ptr [ecx + Offset_Reg_AF], ax

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x0F	rrca
void __declspec(naked) __fastcall OpCode_RRCA(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_A]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		mov		word ptr [ecx + Offset_Reg_AF], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x17	rla
void __declspec(naked) __fastcall OpCode_RLA(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, byte ptr [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, byte ptr [ecx + Offset_Reg_A]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		mov		byte ptr [ecx + Offset_Reg_F], bl
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x1F	rra
void __declspec(naked) __fastcall OpCode_RRA(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, byte ptr [ecx + Offset_Reg_F]
		mov		bl, byte ptr [ecx + Offset_Reg_A]
		and		bh, Flag_C
		shl		bh, 3
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		mov		byte ptr [ecx + Offset_Reg_F], bl
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x09	add		hl, bc
void __declspec(naked) __fastcall OpCode_ADD_HL_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_F]
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		mov		dx, word ptr [ecx + Offset_Reg_BC]
		and		al, Flag_Z
		push	ebx
		push	edx
		shl		ebx, 20
		shl		edx, 20
		add		ebx, edx
		jnc		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		pop		edx
		pop		ebx
		add		ebx, edx
		jnc		NoCarry
		or		al, Flag_C
NoCarry:
		mov		word ptr [ecx + Offset_Reg_HL], bx
		mov		byte ptr [ecx + Offset_Reg_F], al

		mov		eax, 2
		ret
	}
}

//0x19	add		hl, de
void __declspec(naked) __fastcall OpCode_ADD_HL_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_F]
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		mov		dx, word ptr [ecx + Offset_Reg_DE]
		and		al, Flag_Z
		push	ebx
		push	edx
		shl		ebx, 20		//0x????...1234 -> 0x2340...0000
		shl		edx, 20
		add		ebx, edx
		jnc		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		pop		edx
		pop		ebx
		add		ebx, edx
		jnc		NoCarry
		or		al, Flag_C
NoCarry:
		mov		word ptr [ecx + Offset_Reg_HL], bx
		mov		byte ptr [ecx + Offset_Reg_F], al

		mov		eax, 2
		ret
	}
}

//0x29	add		hl, hl
void __declspec(naked) __fastcall OpCode_ADD_HL_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_F]
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		and		al, Flag_Z
		mov		edx, ebx
		shl		ebx, 20		//0x????...1234 -> 0x2340...0000
		add		ebx, ebx
		jnc		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		add		edx, edx
		jnc		NoCarry
		or		al, Flag_C
NoCarry:
		mov		word ptr [ecx + Offset_Reg_HL], dx
		mov		byte ptr [ecx + Offset_Reg_F], al

		mov		eax, 2
		ret
	}
}

//0x39	add		hl, sp
void __declspec(naked) __fastcall OpCode_ADD_HL_SP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_F]
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		and		al, Flag_Z
		push	ebx
		push	edx
		shl		ebx, 20		//0x????...1234 -> 0x2340...0000
		shl		edx, 20
		add		ebx, edx
		jnc		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		pop		edx
		pop		ebx
		add		ebx, edx
		jnc		NoCarry
		or		al, Flag_C
NoCarry:
		mov		word ptr [ecx + Offset_Reg_HL], bx
		mov		byte ptr [ecx + Offset_Reg_F], al

		mov		eax, 2
		ret
	}
}

//0x10	stop
void __declspec(naked) __fastcall OpCode_STOP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		test	byte ptr [ecx + Offset_Flags], GB_ROM_COLOR
		jz		NotColorGB

		mov		al, byte ptr [ecx + FF00_ASM + 0x4D]
		shl		al, 7
		mov		byte ptr [ecx + FF00_ASM + 0x4D], al

NotColorGB:
		add		edx, 2
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x27	daa
void __declspec(naked) __fastcall OpCode_DAA(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_F]
		mov		al, byte ptr [ecx + Offset_Reg_A]
		test	bl, Flag_N
		jz		NotSub

		test	bl, Flag_C
		jz		Sub_NoCarry
		sub		al, 0x60
Sub_NoCarry:
		test	bl, Flag_H
		jz		Complete
		shl		eax, 4
		sub		al, 0x60
		shr		eax, 4
		jmp		Complete

NotSub:
		test	bl, Flag_C
		jz		Add_NoCarry
		add		al, 0x60
Add_NoCarry:
		test	bl, Flag_H
		jz		Add_NoHalfCarry
		shl		eax, 4
		add		al, 0x60
		shr		eax, 4
Add_NoHalfCarry:

		test	bl, Flag_C | Flag_H
		jnz		Complete
		add		al, 0	//Set flags in order to get DAA work correctly
		daa
		jnc		Complete
		or		bl, Flag_C

Complete:
		and		bl, Flag_N | Flag_C
		test	al, al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], al
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x2F	cpl
void __declspec(naked) __fastcall OpCode_CPL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ax, word ptr [ecx + Offset_Reg_AF]
		not		ah
		or		al, Flag_N | Flag_H
		mov		word ptr [ecx + Offset_Reg_AF], ax

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x37	scf
void __declspec(naked) __fastcall OpCode_SCF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, byte ptr [ecx + Offset_Reg_F]
		and		al, Flag_Z
		or		al, Flag_C
		mov		byte ptr [ecx + Offset_Reg_F], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x3F	ccf
void __declspec(naked) __fastcall OpCode_CCF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_F]
		mov		bh, bl
		not		bl
		and		bh, Flag_Z
		and		bl, Flag_C
		or		bh, bl
		mov		byte ptr [ecx + Offset_Reg_F], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x18	jr		nn
void __declspec(naked) __fastcall OpCode_JR(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret
	}
}

//0x18	jr		nn
void __declspec(naked) __fastcall Debug_OpCode_JR(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
AccessDenied:
		ret
	}
}

//0x20	jr		nz, nn
void __declspec(naked) __fastcall OpCode_JR_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoJump

		inc		dx
		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		add		edx, 2
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x20	jr		nz, nn
void __declspec(naked) __fastcall Debug_OpCode_JR_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		test	byte ptr [ecx + Offset_Reg_F], Flag_Z
		jnz		NoJump

		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x28	jr		z, nn
void __declspec(naked) __fastcall OpCode_JR_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		inc		dx
		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		add		edx, 2
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x28	jr		z, nn
void __declspec(naked) __fastcall Debug_OpCode_JR_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x30	jr		nc, nn
void __declspec(naked) __fastcall OpCode_JR_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		inc		dx
		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		add		edx, 2
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x30	jr		nc, nn
void __declspec(naked) __fastcall Debug_OpCode_JR_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x38	jr		c, nn
void __declspec(naked) __fastcall OpCode_JR_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		inc		dx
		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		add		edx, 2
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0x38	jr		c, nn
void __declspec(naked) __fastcall Debug_OpCode_JR_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		call	ReadMem

		movsx	eax, al

		lea		edx, [edx + eax + 1]
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		eax, 3
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

/*void __fastcall DeadLock(CGameBoy *pGameBoy)
{
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Deadlock", "Game Lad", MB_ICONWARNING | MB_OK);
	if (pGameBoy->hThread)
	{
		PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
	}
	pGameBoy->Flags |= GB_ERROR | GB_EXITLOOPDIRECTLY;
}*/

void __fastcall HaltError(CGameBoy *pGameBoy)
{
	SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)pGameBoy->hGBWnd, 0);
	if (!hDisAsm)
	{
		SendMessage(hWnd, WM_COMMAND, ID_VIEW_DISASSEMBLY, 0);
	}
	else
	{
		SendMessage(hClientWnd, WM_MDIACTIVATE, (WPARAM)hDisAsm, 0);
	}
	MessageBox(hWnd, "Halt quirk", "Game Lad", MB_ICONWARNING | MB_OK);
	if (pGameBoy->hThread)
	{
		PostThreadMessage(pGameBoy->ThreadId, WM_QUIT, 0, 0);
	}
	pGameBoy->Flags |= GB_ERROR | GB_EXITLOOPDIRECTLY;
}

//0x76	halt
void __declspec(naked) __fastcall OpCode_HALT(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ebx, dword ptr [ecx + Offset_Flags]
		inc		edx
		test	ebx, GB_IE
		jz		Halt_NotEI
		or		ebx, GB_HALT
		mov		dword ptr [ecx + Offset_Flags], ebx

		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret

Halt_NotEI:
		test	ebx, GB_ROM_COLOR
		jnz		Halt_GBC
		call	ReadMem
		test	al, al
		jz		Halt_Nop

		push	ebx
		push	ecx
		push	edx
		/*cmp		al, 0x76
		jne		Halt_NotHalt

		call	DeadLock
		pop		edx
		pop		ecx
		pop		ebx
		ret

Halt_NotHalt:*/
		call	HaltError
		pop		edx
		pop		ecx
		pop		ebx
		ret

Halt_Nop:
Halt_GBC:
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}






































//0x40	ld		b, b
#define		OpCode_LD_B_B		OpCode_Nop

//0x41	ld		b, c
void __declspec(naked) __fastcall OpCode_LD_B_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_B], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x42	ld		b, d
void __declspec(naked) __fastcall OpCode_LD_B_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_B], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x43	ld		b, e
void __declspec(naked) __fastcall OpCode_LD_B_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_B], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x44	ld		b, h
void __declspec(naked) __fastcall OpCode_LD_B_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_B], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x45	ld		b, l
void __declspec(naked) __fastcall OpCode_LD_B_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_B], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x47	ld		b, a
void __declspec(naked) __fastcall OpCode_LD_B_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_B], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x48	ld		c, b
void __declspec(naked) __fastcall OpCode_LD_C_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_C], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x49	ld		c, c
#define		OpCode_LD_C_C		OpCode_Nop

//0x4A	ld		c, d
void __declspec(naked) __fastcall OpCode_LD_C_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_C], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x4B	ld		c, e
void __declspec(naked) __fastcall OpCode_LD_C_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_C], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x4C	ld		c, h
void __declspec(naked) __fastcall OpCode_LD_C_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_C], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x4D	ld		c, l
void __declspec(naked) __fastcall OpCode_LD_C_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_C], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x4F	ld		c, a
void __declspec(naked) __fastcall OpCode_LD_C_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_C], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x50	ld		d, b
void __declspec(naked) __fastcall OpCode_LD_D_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_D], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x51	ld		d, c
void __declspec(naked) __fastcall OpCode_LD_D_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_D], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x52	ld		d, d
#define		OpCode_LD_D_D		OpCode_Nop

//0x53	ld		d, e
void __declspec(naked) __fastcall OpCode_LD_D_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_D], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x54	ld		d, h
void __declspec(naked) __fastcall OpCode_LD_D_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_D], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x55	ld		d, l
void __declspec(naked) __fastcall OpCode_LD_D_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_D], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x57	ld		d, a
void __declspec(naked) __fastcall OpCode_LD_D_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_D], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x58	ld		e, b
void __declspec(naked) __fastcall OpCode_LD_E_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_E], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x59	ld		e, c
void __declspec(naked) __fastcall OpCode_LD_E_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_E], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x5A	ld		e, d
void __declspec(naked) __fastcall OpCode_LD_E_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_E], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x5B	ld		e, e
#define		OpCode_LD_E_E		OpCode_Nop

//0x5C	ld		e, h
void __declspec(naked) __fastcall OpCode_LD_E_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_E], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x5D	ld		e, l
void __declspec(naked) __fastcall OpCode_LD_E_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_E], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x5F	ld		e, a
void __declspec(naked) __fastcall OpCode_LD_E_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_E], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x60	ld		h, b
void __declspec(naked) __fastcall OpCode_LD_H_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_H], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x61	ld		h, c
void __declspec(naked) __fastcall OpCode_LD_H_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_H], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x62	ld		h, d
void __declspec(naked) __fastcall OpCode_LD_H_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_H], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x63	ld		h, e
void __declspec(naked) __fastcall OpCode_LD_H_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_H], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x64	ld		h, h
#define		OpCode_LD_H_H		OpCode_Nop

//0x65	ld		h, l
void __declspec(naked) __fastcall OpCode_LD_H_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_H], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x67	ld		h, a
void __declspec(naked) __fastcall OpCode_LD_H_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_H], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x68	ld		l, b
void __declspec(naked) __fastcall OpCode_LD_L_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_L], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x69	ld		l, c
void __declspec(naked) __fastcall OpCode_LD_L_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_L], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x6A	ld		l, d
void __declspec(naked) __fastcall OpCode_LD_L_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_L], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x6B	ld		l, e
void __declspec(naked) __fastcall OpCode_LD_L_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_L], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x6C	ld		l, h
void __declspec(naked) __fastcall OpCode_LD_L_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_L], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x6D	ld		l, l
#define		OpCode_LD_L_L		OpCode_Nop

//0x6F	ld		l, a
void __declspec(naked) __fastcall OpCode_LD_L_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_AF]		//Offset_Reg_AF = 0, no displacement
		mov		byte ptr [ecx + Offset_Reg_L], bh

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x78	ld		a, b
void __declspec(naked) __fastcall OpCode_LD_A_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_B]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x79	ld		a, c
void __declspec(naked) __fastcall OpCode_LD_A_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_C]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x7A	ld		a, d
void __declspec(naked) __fastcall OpCode_LD_A_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_D]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x7B	ld		a, e
void __declspec(naked) __fastcall OpCode_LD_A_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_E]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x7C	ld		a, h
void __declspec(naked) __fastcall OpCode_LD_A_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_H]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x7D	ld		a, l
void __declspec(naked) __fastcall OpCode_LD_A_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, byte ptr [ecx + Offset_Reg_L]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0x7F	ld		a, a
#define		OpCode_LD_A_A		OpCode_Nop



//0x80	add		a, b
void __declspec(naked) __fastcall OpCode_ADD_A_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_B]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		shl		al, 4
		shl		ah, 4
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x81	add		a, c
void __declspec(naked) __fastcall OpCode_ADD_A_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_C]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x82	add		a, d
void __declspec(naked) __fastcall OpCode_ADD_A_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_D]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x83	add		a, e
void __declspec(naked) __fastcall OpCode_ADD_A_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_E]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x84	add		a, h
void __declspec(naked) __fastcall OpCode_ADD_A_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_H]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x85	add		a, l
void __declspec(naked) __fastcall OpCode_ADD_A_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_L]
		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x87	add		a, a
void __declspec(naked) __fastcall OpCode_ADD_A_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		dl, al
		and		al, 0x0F
		add		al, al
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x86	add		a, [hl]
void __declspec(naked) __fastcall OpCode_ADD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		ah, byte ptr [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], dl
		mov		byte ptr [ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0x86	add		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_ADD_A_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		ah, byte ptr [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], dl
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xC6	add		a, nn
void __declspec(naked) __fastcall OpCode_ADD_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		mov		ah, [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dl
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xC6	add		a, nn
void __declspec(naked) __fastcall Debug_OpCode_ADD_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		mov		ah, byte ptr [ecx + Offset_Reg_A]
		xor		bl, bl
		mov		edx, eax
		and		eax, 0x00000F0F
		add		al, ah
		jnc		NoHalfCarry
		mov		bl, Flag_H
NoHalfCarry:
		add		dl, dh
		jnc		NoCarry
		or		bl, Flag_C
		test	dl, dl	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], dl
		mov		byte ptr [ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xE8	add		sp, nn
void __declspec(naked) __fastcall OpCode_ADD_SP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		inc		edx

		movsx	ax, al

		add		word ptr [ecx + Offset_Reg_SP], ax
		jnc		NoCarry
		mov		byte ptr [ecx + Offset_Reg_F], Flag_C

		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 4
		ret

NoCarry:
		mov		byte ptr [ecx + Offset_Reg_F], 0

		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 4
		ret
	}
}

//0xE8	add		sp, nn
void __declspec(naked) __fastcall Debug_OpCode_ADD_SP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		inc		edx

		movsx	ax, al

		add		word ptr [ecx + Offset_Reg_SP], ax
		jnc		NoCarry
		mov		byte ptr [ecx + Offset_Reg_F], Flag_C

		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 4
		ret

NoCarry:
		mov		byte ptr [ecx + Offset_Reg_F], 0

		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0x88	adc		a, b
void __declspec(naked) __fastcall OpCode_ADC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_B]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x89	adc		a, c
void __declspec(naked) __fastcall OpCode_ADC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_C]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8A	adc		a, d
void __declspec(naked) __fastcall OpCode_ADC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_D]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8B	adc		a, e
void __declspec(naked) __fastcall OpCode_ADC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_E]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8C	adc		a, h
void __declspec(naked) __fastcall OpCode_ADC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_H]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8D	adc		a, l
void __declspec(naked) __fastcall OpCode_ADC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_L]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8F	adc		a, a
void __declspec(naked) __fastcall OpCode_ADC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, bh
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x8E	adc		a, [hl]
void __declspec(naked) __fastcall OpCode_ADC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		ebx, dword ptr [ecx + Offset_Reg_AF]
		call	ReadMem
		and		eax, 0xFF
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], bl
		mov		byte ptr [ecx + Offset_Reg_F], dl

		mov		eax, 2
		ret
	}
}

//0x8E	adc		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_ADC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		mov		ebx, dword ptr [ecx + Offset_Reg_AF]
		call	ReadMem
		and		eax, 0xFF
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], bl
		mov		byte ptr [ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xCE	adc		a, nn
void __declspec(naked) __fastcall OpCode_ADC_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		and		eax, 0xFF

		mov		ebx, [ecx + Offset_Reg_AF]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xCE	adc		a, nn
void __declspec(naked) __fastcall Debug_OpCode_ADC_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		and		eax, 0xFF

		mov		ebx, [ecx + Offset_Reg_AF]
		xor		edx, edx
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh

		push	eax
		push	ebx

		shl		al, 4
		shl		bl, 4
		add		bl, al
		jnc		NoHalfCarry
		mov		dl, Flag_H
NoHalfCarry:
		pop		ebx
		pop		eax

		add		bx, ax
		test	bh, bh
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x90	sub		a, b
void __declspec(naked) __fastcall OpCode_SUB_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_B]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x91	sub		a, c
void __declspec(naked) __fastcall OpCode_SUB_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_C]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x92	sub		a, d
void __declspec(naked) __fastcall OpCode_SUB_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_D]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x93	sub		a, e
void __declspec(naked) __fastcall OpCode_SUB_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_E]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x94	sub		a, h
void __declspec(naked) __fastcall OpCode_SUB_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_H]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x95	sub		a, l
void __declspec(naked) __fastcall OpCode_SUB_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_L]
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x97	sub		a, a
void __declspec(naked) __fastcall OpCode_SUB_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		word ptr [ecx + Offset_Reg_AF], Flag_N | Flag_Z

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x96	sub		a, [hl]
void __declspec(naked) __fastcall OpCode_SUB_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x0F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], dh
		mov		byte ptr [ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0x96	sub		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SUB_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x0F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], dh
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xD6	sub		a, nn
void __declspec(naked) __fastcall OpCode_SUB_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xD6	sub		a, nn
void __declspec(naked) __fastcall Debug_OpCode_SUB_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		mov		ah, [ecx + Offset_Reg_A]
		mov		bl, Flag_N
		mov		edx, eax
		and		eax, 0x00000F0F
		cmp		ah, al
		jae		NoHalfCarry
		mov		bl, Flag_N | Flag_H
NoHalfCarry:
		sub		dh, dl
		jnc		NoCarry
		or		bl, Flag_C
		test	dh, dh	//restore Z flag
NoCarry:
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], dh
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0x98	sbc		a, b
void __declspec(naked) __fastcall OpCode_SBC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_B]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		//ax	= Reg_B + Carry
		//bx	= Reg_A
		//dl	= 0 (flags)

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x99	sbc		a, c
void __declspec(naked) __fastcall OpCode_SBC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_C]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9A	sbc		a, d
void __declspec(naked) __fastcall OpCode_SBC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_D]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9B	sbc		a, e
void __declspec(naked) __fastcall OpCode_SBC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_E]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9C	sbc		a, h
void __declspec(naked) __fastcall OpCode_SBC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_H]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9D	sbc		a, l
void __declspec(naked) __fastcall OpCode_SBC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		eax, eax
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		al, [ecx + Offset_Reg_L]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9F	sbc		a, a
void __declspec(naked) __fastcall OpCode_SBC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		test	byte ptr [ecx + Offset_Reg_F], Flag_C
		jz		NoCarry
		mov		dword ptr [ecx + Offset_Reg_AF], 0xFF00 | Flag_C | Flag_H | Flag_N

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
NoCarry:
		mov		dword ptr [ecx + Offset_Reg_AF], Flag_N | Flag_Z

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0x9E	sbc		a, [hl]
void __declspec(naked) __fastcall OpCode_SBC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		mov		ebx, dword ptr [ecx + Offset_Reg_AF]
		call	ReadMem
		and		eax, 0xFF
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], bl
		mov		byte ptr [ecx + Offset_Reg_F], dl

		mov		eax, 2
		ret
	}
}

//0x9E	sbc		a, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SBC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		mov		ebx, dword ptr [ecx + Offset_Reg_AF]
		call	ReadMem
		and		eax, 0xFF
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_A], bl
		mov		byte ptr [ecx + Offset_Reg_F], dl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xDE	sbc		a, nn
void __declspec(naked) __fastcall OpCode_SBC_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		and		eax, 0xFF

		mov		ebx, [ecx + Offset_Reg_AF]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xDE	sbc		a, nn
void __declspec(naked) __fastcall Debug_OpCode_SBC_A_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		and		eax, 0xFF

		mov		ebx, [ecx + Offset_Reg_AF]
		and		bl, Flag_C
		shr		bl, 4
		add		al, bl
		adc		ah, 0
		mov		bl, bh
		xor		bh, bh
		mov		dl, Flag_N

		push	eax
		push	ebx

		and		al, 0x0F
		and		bl, 0x0F
		cmp		bl, al
		jae		NoHalfCarry
		mov		dl, Flag_H | Flag_N
NoHalfCarry:
		pop		ebx
		pop		eax

		sub		ebx, eax
		test	ebx, 0xFFFFFF00
		jz		NoCarry
		or		dl, Flag_C
NoCarry:
		test	bl, bl
		jnz		NotZero
		or		dl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_A], bl
		mov		[ecx + Offset_Reg_F], dl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xA0	and		b
void __declspec(naked) __fastcall OpCode_AND_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_B]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA1	and		c
void __declspec(naked) __fastcall OpCode_AND_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_C]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA2	and		d
void __declspec(naked) __fastcall OpCode_AND_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_D]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA3	and		e
void __declspec(naked) __fastcall OpCode_AND_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_E]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA4	and		h
void __declspec(naked) __fastcall OpCode_AND_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_H]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA5	and		l
void __declspec(naked) __fastcall OpCode_AND_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_L]
		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA7	and		a
void __declspec(naked) __fastcall OpCode_AND_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, Flag_H
		cmp		[ecx + Offset_Reg_A], 0
		jne		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA6	and		[hl]
void __declspec(naked) __fastcall OpCode_AND_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, Flag_H
		and		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xA6	and		[hl]
void __declspec(naked) __fastcall Debug_OpCode_AND_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, Flag_H
		and		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xE6	and		nn
void __declspec(naked) __fastcall OpCode_AND_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xE6	and		nn
void __declspec(naked) __fastcall Debug_OpCode_AND_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		mov		bl, Flag_H
		and		[ecx + Offset_Reg_A], al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xA8	xor		b
void __declspec(naked) __fastcall OpCode_XOR_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_B]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xA9	xor		c
void __declspec(naked) __fastcall OpCode_XOR_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_C]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAA	xor		d
void __declspec(naked) __fastcall OpCode_XOR_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_D]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAB	xor		e
void __declspec(naked) __fastcall OpCode_XOR_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_E]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAC	xor		h
void __declspec(naked) __fastcall OpCode_XOR_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_H]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAD	xor		l
void __declspec(naked) __fastcall OpCode_XOR_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_L]
		xor		bl, bl
		xor		[ecx + Offset_Reg_A], bh
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAF	xor		a
void __declspec(naked) __fastcall OpCode_XOR_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dword ptr [ecx + Offset_Reg_AF], Flag_Z	//GB->Reg_A = 0, GB->Reg_F = Flag_Z

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xAE	xor		[hl]
void __declspec(naked) __fastcall OpCode_XOR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		xor		bl, bl
		xor		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xAE	xor		[hl]
void __declspec(naked) __fastcall Debug_OpCode_XOR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		xor		bl, bl
		xor		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xEE	xor		nn
void __declspec(naked) __fastcall OpCode_XOR_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		xor		bl, bl
		xor		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xEE	xor		nn
void __declspec(naked) __fastcall Debug_OpCode_XOR_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		xor		bl, bl
		xor		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xB0	or		b
void __declspec(naked) __fastcall OpCode_OR_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_B]		//al	= Reg_B
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB1	or		c
void __declspec(naked) __fastcall OpCode_OR_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_C]
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB2	or		d
void __declspec(naked) __fastcall OpCode_OR_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_D]
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB3	or		e
void __declspec(naked) __fastcall OpCode_OR_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_E]
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB4	or		h
void __declspec(naked) __fastcall OpCode_OR_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_H]
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB5	or		l
void __declspec(naked) __fastcall OpCode_OR_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_L]
		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB7	or		a
void __declspec(naked) __fastcall OpCode_OR_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		bl, bl
		cmp		[ecx + Offset_Reg_A], 0
		jne		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB6	or		[hl]
void __declspec(naked) __fastcall OpCode_OR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		xor		bl, bl
		or		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xB6	or		[hl]
void __declspec(naked) __fastcall Debug_OpCode_OR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		xor		bl, bl
		or		byte ptr [ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xF6	or		nn
void __declspec(naked) __fastcall OpCode_OR_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xF6	or		nn
void __declspec(naked) __fastcall Debug_OpCode_OR_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		xor		bl, bl
		or		[ecx + Offset_Reg_A], al
		jnz		NotZero
		mov		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xB8	cp		b
void __declspec(naked) __fastcall OpCode_CP_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xB9	cp		c
void __declspec(naked) __fastcall OpCode_CP_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBA	cp		d
void __declspec(naked) __fastcall OpCode_CP_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBB	cp		e
void __declspec(naked) __fastcall OpCode_CP_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBC	cp		h
void __declspec(naked) __fastcall OpCode_CP_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBD	cp		l
void __declspec(naked) __fastcall OpCode_CP_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBF	cp		a
void __declspec(naked) __fastcall OpCode_CP_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		[ecx + Offset_Reg_F], Flag_N | Flag_Z

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 1
		ret
	}
}

//0xBE	cp		[hl]
void __declspec(naked) __fastcall OpCode_CP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, al
		mov		bh, byte ptr [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		byte ptr [ecx + Offset_Reg_F], al

		mov		eax, 2
		ret
	}
}

//0xBE	cp		[hl]
void __declspec(naked) __fastcall Debug_OpCode_CP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		mov		bh, byte ptr [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		byte ptr [ecx + Offset_Reg_F], al

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xFE	cp		nn
void __declspec(naked) __fastcall OpCode_CP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al

		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
		ret
	}
}

//0xFE	cp		nn
void __declspec(naked) __fastcall Debug_OpCode_CP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al

		mov		bh, [ecx + Offset_Reg_A]
		mov		al, Flag_N
		cmp		bh, bl
		jae		NoCarry
		mov		al, Flag_N | Flag_C
NoCarry:
		jnz		NotZero
		or		al, Flag_Z
NotZero:
		and		ebx, 0x00000F0F
		cmp		bh, bl
		jae		NoHalfCarry
		or		al, Flag_H
NoHalfCarry:
		mov		[ecx + Offset_Reg_F], al

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 2
AccessDenied:
		ret
	}
}









































//0xC3	jp		nnnn
void __declspec(naked) __fastcall OpCode_JP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret
	}
}

//0xC3	jp		nnnn
void __declspec(naked) __fastcall Debug_OpCode_JP(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xC2	jp		nz, nnnn
void __declspec(naked) __fastcall OpCode_JP_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoJump

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xC2	jp		nz, nnnn
void __declspec(naked) __fastcall Debug_OpCode_JP_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoJump

		call	ReadMem
		mov		bh, al
		dec		dx
		call	ReadMem
		mov		bl, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xCA	jp		z, nnnn
void __declspec(naked) __fastcall OpCode_JP_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xCA	jp		z, nnnn
void __declspec(naked) __fastcall Debug_OpCode_JP_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		call	ReadMem
		mov		bh, al
		dec		dx
		call	ReadMem
		mov		bl, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xD2	jp		nc, nnnn
void __declspec(naked) __fastcall OpCode_JP_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xD2	jp		nc, nnnn
void __declspec(naked) __fastcall Debug_OpCode_JP_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		call	ReadMem
		mov		bh, al
		dec		dx
		call	ReadMem
		mov		bl, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xDA	jp		c, nnnn
void __declspec(naked) __fastcall OpCode_JP_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xDA	jp		c, nnnn
void __declspec(naked) __fastcall Debug_OpCode_JP_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		call	ReadMem
		mov		bh, al
		dec		dx
		call	ReadMem
		mov		bl, al
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xE9	jp		hl
void __declspec(naked) __fastcall OpCode_JP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 4
		ret
	}
}

//0xCD	call	nnnn
void __declspec(naked) __fastcall OpCode_CALL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret
	}
}

//0xCD	call	nnnn
void __declspec(naked) __fastcall Debug_OpCode_CALL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
AccessDenied:
		ret
	}
}

//0xC4	call	nz, nnnn
void __declspec(naked) __fastcall OpCode_CALL_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoCall

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xC4	call	nz, nnnn
void __declspec(naked) __fastcall Debug_OpCode_CALL_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoCall

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xCC	call	z, nnnn
void __declspec(naked) __fastcall OpCode_CALL_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoCall

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xCC	call	z, nnnn
void __declspec(naked) __fastcall Debug_OpCode_CALL_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoCall

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xD4	call	nc, nnnn
void __declspec(naked) __fastcall OpCode_CALL_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoCall

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xD4	call	nc, nnnn
void __declspec(naked) __fastcall Debug_OpCode_CALL_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoCall

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xDC	call	c, nnnn
void __declspec(naked) __fastcall OpCode_CALL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoCall

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		add		edx, 3
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xDC	call	c, nnnn
void __declspec(naked) __fastcall Debug_OpCode_CALL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bh, al

		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoCall

		mov		eax, edx
		inc		ax
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], bx

		mov		eax, 6
		ret

NoCall:
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xC7	rst		0x00
void __declspec(naked) __fastcall OpCode_RST_0(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x00
		mov		eax, 4
		ret
	}
}

//0xC7	rst		0x00
void __declspec(naked) __fastcall Debug_OpCode_RST_0(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x00
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xCF	rst		0x08
void __declspec(naked) __fastcall OpCode_RST_8(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x08
		mov		eax, 4
		ret
	}
}

//0xCF	rst		0x08
void __declspec(naked) __fastcall Debug_OpCode_RST_8(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x08
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xD7	rst		0x10
void __declspec(naked) __fastcall OpCode_RST_10(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x10
		mov		eax, 4
		ret
	}
}

//0xD7	rst		0x10
void __declspec(naked) __fastcall Debug_OpCode_RST_10(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x10
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xDF	rst		0x18
void __declspec(naked) __fastcall OpCode_RST_18(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x18
		mov		eax, 4
		ret
	}
}

//0xDF	rst		0x18
void __declspec(naked) __fastcall Debug_OpCode_RST_18(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x18
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xE7	rst		0x20
void __declspec(naked) __fastcall OpCode_RST_20(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x20
		mov		eax, 4
		ret
	}
}

//0xE7	rst		0x20
void __declspec(naked) __fastcall Debug_OpCode_RST_20(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x20
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xEF	rst		0x28
void __declspec(naked) __fastcall OpCode_RST_28(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x28
		mov		eax, 4
		ret
	}
}

//0xEF	rst		0x28
void __declspec(naked) __fastcall Debug_OpCode_RST_28(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x28
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xF7	rst		0x30
void __declspec(naked) __fastcall OpCode_RST_30(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x30
		mov		eax, 4
		ret
	}
}

//0xF7	rst		0x30
void __declspec(naked) __fastcall Debug_OpCode_RST_30(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x30
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xFF	rst		0x38
void __declspec(naked) __fastcall OpCode_RST_38(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		inc		ax
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x38
		mov		eax, 4
		ret
	}
}

//0xFF	rst		0x38
void __declspec(naked) __fastcall Debug_OpCode_RST_38(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		eax, edx
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		inc		ax
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		mov		word ptr [ecx + Offset_Reg_PC], 0x38
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xC5	push	bc
void __declspec(naked) __fastcall OpCode_PUSH_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		mov		ax, word ptr [ecx + Offset_Reg_BC]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xC5	push	bc
void __declspec(naked) __fastcall Debug_OpCode_PUSH_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		ax, word ptr [ecx + Offset_Reg_BC]
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xD5	push	de
void __declspec(naked) __fastcall OpCode_PUSH_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		mov		ax, word ptr [ecx + Offset_Reg_DE]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xD5	push	de
void __declspec(naked) __fastcall Debug_OpCode_PUSH_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		ax, word ptr [ecx + Offset_Reg_DE]
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xE5	push		hl
void __declspec(naked) __fastcall OpCode_PUSH_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		mov		ax, word ptr [ecx + Offset_Reg_HL]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xE5	push		hl
void __declspec(naked) __fastcall Debug_OpCode_PUSH_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		ax, word ptr [ecx + Offset_Reg_HL]
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xF5	push	af
void __declspec(naked) __fastcall OpCode_PUSH_AF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		mov		ax, word ptr [ecx + Offset_Reg_AF]
		sub		dx, 2
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	LD_mem8
		inc		dx
		mov		al, ah
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xF5	push	af
void __declspec(naked) __fastcall Debug_OpCode_PUSH_AF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		sub		dx, 2
		call	CheckWriteAccessWord
		jc		AccessDenied
		mov		ax, word ptr [ecx + Offset_Reg_AF]
		mov		word ptr [ecx + Offset_Reg_SP], dx
		call	Debug_LD_mem8
		inc		dx
		mov		al, ah
		call	Debug_LD_mem8

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xC1	pop		bc
void __declspec(naked) __fastcall OpCode_POP_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_C], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 3
		ret
	}
}

//0xC1	pop		bc
void __declspec(naked) __fastcall Debug_OpCode_POP_BC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_C], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_B], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xD1	pop		de
void __declspec(naked) __fastcall OpCode_POP_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_E], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 3
		ret
	}
}

//0xD1	pop		de
void __declspec(naked) __fastcall Debug_OpCode_POP_DE(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_E], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_D], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xE1	pop		hl
void __declspec(naked) __fastcall OpCode_POP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_L], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 3
		ret
	}
}

//0xE1	pop		hl
void __declspec(naked) __fastcall Debug_OpCode_POP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_L], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_H], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xF1	pop		af
void __declspec(naked) __fastcall OpCode_POP_AF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_F], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 3
		ret
	}
}

//0xF1	pop		af
void __declspec(naked) __fastcall Debug_OpCode_POP_AF(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_F], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xC9	ret
void __declspec(naked) __fastcall OpCode_RET(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 4
		ret
	}
}

//0xC9	ret
void __declspec(naked) __fastcall Debug_OpCode_RET(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xC0	ret		nz
void __declspec(naked) __fastcall OpCode_RET_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0xC0	ret		nz
void __declspec(naked) __fastcall Debug_OpCode_RET_NZ(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jnz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xC8	ret		z
void __declspec(naked) __fastcall OpCode_RET_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0xC8	ret		z
void __declspec(naked) __fastcall Debug_OpCode_RET_Z(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_Z
		jz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xD0	ret		nc
void __declspec(naked) __fastcall OpCode_RET_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0xD0	ret		nc
void __declspec(naked) __fastcall Debug_OpCode_RET_NC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jnz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xD8	ret		c
void __declspec(naked) __fastcall OpCode_RET_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0xD8	ret		c
void __declspec(naked) __fastcall Debug_OpCode_RET_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		al, [ecx + Offset_Reg_F]
		test	al, Flag_C
		jz		NoJump

		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx

		mov		eax, 5
		ret

NoJump:
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xD9	reti
void __declspec(naked) __fastcall OpCode_RETI(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC], al
		inc		dx
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx
		or		dword ptr [ecx + Offset_Flags], GB_ENABLEIE

		mov		eax, 4
		ret
	}
}

//0xD9	reti
void __declspec(naked) __fastcall Debug_OpCode_RETI(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dx, word ptr [ecx + Offset_Reg_SP]
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		byte ptr [ecx + Offset_Reg_PC], al
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_PC + 1], al
		inc		dx
		mov		word ptr [ecx + Offset_Reg_SP], dx
		or		dword ptr [ecx + Offset_Flags], GB_ENABLEIE

		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xFB	ei
void __declspec(naked) __fastcall OpCode_EI(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		dword ptr [ecx + Offset_Flags], GB_ENABLEIE

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}

//0xF3	di
void __declspec(naked) __fastcall OpCode_DI(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		dword ptr [ecx + Offset_Flags], ~(GB_IE | GB_ENABLEIE)

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 1
		ret
	}
}































//0xE0	ld		[0xFF00 + nn], a
void __declspec(naked) __fastcall OpCode_LD_FFnn_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		dl, al
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Port

		mov		eax, 3
		ret
	}
}

//0xE0	ld		[0xFF00 + nn], a
void __declspec(naked) __fastcall Debug_OpCode_LD_FFnn_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		dh, 0xFF
		mov		dl, al
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_Port

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xE2	ld		[0xFF00 + c], a
void __declspec(naked) __fastcall OpCode_LD_FFC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		dl, byte ptr [ecx + Offset_Reg_C]
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Port

		mov		eax, 2
		ret
	}
}

//0xE2	ld		[0xFF00 + c], a
void __declspec(naked) __fastcall Debug_OpCode_LD_FFC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dl, byte ptr [ecx + Offset_Reg_C]
		mov		dh, 0xFF
		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_Port

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xEA	ld		[nnnn], a
void __declspec(naked) __fastcall OpCode_LD_nnnn_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		xor		ebx, ebx

		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		mov		bh, al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		mov		al, byte ptr [ecx + Offset_Reg_A]
		mov		edx, ebx
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xEA	ld		[nnnn], a
void __declspec(naked) __fastcall Debug_OpCode_LD_nnnn_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		mov		bl, al
		call	ReadMem
		mov		dl, bl
		mov		dh, al

		call	CheckWriteAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + Offset_Reg_A]
		call	Debug_LD_mem8

		add		word ptr [ecx + Offset_Reg_PC], 3
		mov		eax, 4
AccessDenied:
		ret
	}
}

//0xF0	ld		a, [0xFF00 + nn]
void __declspec(naked) __fastcall OpCode_LD_A_FFnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		and		eax, 0xFF
		mov		al, byte ptr [ecx + FF00_ASM + eax]
		mov		byte ptr [ecx + Offset_Reg_A], al

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xF0	ld		a, [0xFF00 + nn]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_FFnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		and		eax, 0xFF
		mov		dl, al
		mov		dh, 0xFF
		call	CheckReadAccess
		jc		AccessDenied
		mov		al, byte ptr [ecx + FF00_ASM + eax]
		mov		byte ptr [ecx + Offset_Reg_A], al

		add		word ptr [ecx + Offset_Reg_PC], 2
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xF2	ld		a, [0xFF00 + c]
void __declspec(naked) __fastcall OpCode_LD_A_FFC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx

		xor		edx, edx
		mov		dl, byte ptr [ecx + Offset_Reg_C]
		mov		bl, byte ptr [ecx + FF00_ASM + edx]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		mov		eax, 2
		ret
	}
}

//0xF2	ld		a, [0xFF00 + c]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_FFC(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		dl, byte ptr [ecx + Offset_Reg_C]
		mov		dh, 0xFF
		call	CheckReadAccess
		jc		AccessDenied
		mov		bl, byte ptr [ecx + FF00_ASM + edx - 0xFF00]
		mov		byte ptr [ecx + Offset_Reg_A], bl

		inc		word ptr [ecx + Offset_Reg_PC]
		mov		eax, 2
AccessDenied:
		ret
	}
}

//0xF8	ld		hl, sp + nn
void __declspec(naked) __fastcall OpCode_LD_HL_SP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem

		mov		bx, word ptr [ecx + Offset_Reg_SP]

		movsx	eax, al

		add		ebx, eax
		mov		word ptr [ecx + Offset_Reg_HL], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
		ret
	}
}

//0xF8	ld		hl, sp + nn
void __declspec(naked) __fastcall Debug_OpCode_LD_HL_SP_nn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem

		mov		bx, word ptr [ecx + Offset_Reg_SP]

		movsx	eax, al

		add		ebx, eax
		mov		word ptr [ecx + Offset_Reg_HL], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 3
AccessDenied:
		ret
	}
}

//0xF9	ld		sp, hl
void __declspec(naked) __fastcall OpCode_LD_SP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bx, word ptr [ecx + Offset_Reg_HL]
		mov		word ptr [ecx + Offset_Reg_SP], bx

		inc		edx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		eax, 2
		ret
	}
}

//0xFA	ld		a, [nnnn]
void __declspec(naked) __fastcall OpCode_LD_A_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		mov		bl, al
		inc		dx
		call	ReadMem
		inc		dx
		mov		word ptr [ecx + Offset_Reg_PC], dx
		mov		dh, al
		mov		dl, bl
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		mov		eax, 4
		ret
	}
}

//0xFA	ld		a, [nnnn]
void __declspec(naked) __fastcall Debug_OpCode_LD_A_nnnn(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		dh, al
		mov		dl, bl
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		mov		byte ptr [ecx + Offset_Reg_A], al

		add		word ptr [ecx + Offset_Reg_PC], 3
		mov		eax, 4
AccessDenied:
		ret
	}
}









//0xCB00	rlc		b
void __declspec(naked) __fastcall OpCode_RLC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB01	rlc		c
void __declspec(naked) __fastcall OpCode_RLC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB02	rlc		d
void __declspec(naked) __fastcall OpCode_RLC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB03	rlc		e
void __declspec(naked) __fastcall OpCode_RLC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB04	rlc		h
void __declspec(naked) __fastcall OpCode_RLC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB05	rlc		l
void __declspec(naked) __fastcall OpCode_RLC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB06	rlc		[hl]
void __declspec(naked) __fastcall OpCode_RLC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem

		mov		bl, al
		shr		bl, 7

		shl		al, 1
		or		al, bl
		shl		bl, 4
		test	al, al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB06	rlc		[hl]
void __declspec(naked) __fastcall Debug_OpCode_RLC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem

		mov		bl, al
		shr		bl, 7

		shl		al, 1
		or		al, bl
		shl		bl, 4
		test	al, al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		byte ptr [ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB07	rlc		a
void __declspec(naked) __fastcall OpCode_RLC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_A]
		mov		bh, bl
		shr		bl, 7

		shl		bh, 1
		or		bh, bl
		shl		bl, 4
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_AF], bx

		mov		eax, 2
		ret
	}
}

//0xCB08	rrc		b
void __declspec(naked) __fastcall OpCode_RRC_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB09	rrc		c
void __declspec(naked) __fastcall OpCode_RRC_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB0A	rrc		d
void __declspec(naked) __fastcall OpCode_RRC_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB0B	rrc		e
void __declspec(naked) __fastcall OpCode_RRC_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB0C	rrc		h
void __declspec(naked) __fastcall OpCode_RRC_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB0D	rrc		l
void __declspec(naked) __fastcall OpCode_RRC_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB0E	rrc		[hl]
void __declspec(naked) __fastcall OpCode_RRC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, al
		shl		bl, 7

		shr		al, 1
		or		al, bl
		shr		bl, 3
		test	al, al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB0E	rrc		[hl]
void __declspec(naked) __fastcall Debug_OpCode_RRC_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		shl		bl, 7

		shr		al, 1
		or		al, bl
		shr		bl, 3
		test	al, al
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB0F	rrc		a
void __declspec(naked) __fastcall OpCode_RRC_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_A]
		mov		bh, bl
		shl		bl, 7

		shr		bh, 1
		or		bh, bl
		shr		bl, 3
		test	bh, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_AF], bx

		mov		eax, 2
		ret
	}
}

//0xCB10	rl		b
void __declspec(naked) __fastcall OpCode_RL_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_B]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], al

		mov		eax, 2
		ret
	}
}

//0xCB11	rl		c
void __declspec(naked) __fastcall OpCode_RL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_C]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], al

		mov		eax, 2
		ret
	}
}

//0xCB12	rl		d
void __declspec(naked) __fastcall OpCode_RL_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_D]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], al

		mov		eax, 2
		ret
	}
}

//0xCB13	rl		e
void __declspec(naked) __fastcall OpCode_RL_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_E]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], al

		mov		eax, 2
		ret
	}
}

//0xCB14	rl		h
void __declspec(naked) __fastcall OpCode_RL_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_H]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], al

		mov		eax, 2
		ret
	}
}

//0xCB15	rl		l
void __declspec(naked) __fastcall OpCode_RL_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_L]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], al

		mov		eax, 2
		ret
	}
}

//0xCB16	rl		[hl]
void __declspec(naked) __fastcall OpCode_RL_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		call	ReadMem
		mov		bl, al
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB16	rl		[hl]
void __declspec(naked) __fastcall Debug_OpCode_RL_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		call	ReadMem
		mov		bl, al
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB17	rl		a
void __declspec(naked) __fastcall OpCode_RL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shr		bh, 4
		mov		bl, [ecx + Offset_Reg_A]
		mov		al, bl
		shr		bl, 3
		and		bl, Flag_C

		shl		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_A], al

		mov		eax, 2
		ret
	}
}

//0xCB18	rr		b
void __declspec(naked) __fastcall OpCode_RR_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_B]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], al

		mov		eax, 2
		ret
	}
}

//0xCB19	rr		c
void __declspec(naked) __fastcall OpCode_RR_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_C]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], al

		mov		eax, 2
		ret
	}
}

//0xCB1A	rr		d
void __declspec(naked) __fastcall OpCode_RR_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_D]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], al

		mov		eax, 2
		ret
	}
}

//0xCB1B	rr		e
void __declspec(naked) __fastcall OpCode_RR_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_E]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], al

		mov		eax, 2
		ret
	}
}

//0xCB1C	rr		h
void __declspec(naked) __fastcall OpCode_RR_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_H]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], al

		mov		eax, 2
		ret
	}
}

//0xCB1D	rr		l
void __declspec(naked) __fastcall OpCode_RR_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_L]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], al

		mov		eax, 2
		ret
	}
}

//0xCB1E	rr		[hl]
void __declspec(naked) __fastcall OpCode_RR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		call	ReadMem
		mov		bl, al
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB1E	rr		[hl]
void __declspec(naked) __fastcall Debug_OpCode_RR_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		call	ReadMem
		mov		bl, al
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB1F	rr		a
void __declspec(naked) __fastcall OpCode_RR_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_F]
		and		bh, Flag_C
		shl		bh, 3
		mov		bl, [ecx + Offset_Reg_A]
		mov		al, bl
		shl		bl, 4
		and		bl, Flag_C

		shr		al, 1
		or		al, bh
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_A], al

		mov		eax, 2
		ret
	}
}

//0xCB20	sla		b
void __declspec(naked) __fastcall OpCode_SLA_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB21	sla		c
void __declspec(naked) __fastcall OpCode_SLA_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB22	sla		d
void __declspec(naked) __fastcall OpCode_SLA_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB23	sla		e
void __declspec(naked) __fastcall OpCode_SLA_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB24	sla		h
void __declspec(naked) __fastcall OpCode_SLA_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB25	sla		l
void __declspec(naked) __fastcall OpCode_SLA_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB26	sla		[hl]
void __declspec(naked) __fastcall OpCode_SLA_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, al
		shr		bl, 3
		and		bl, Flag_C
		shl		al, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB26	sla		[hl]
void __declspec(naked) __fastcall Debug_OpCode_SLA_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		shr		bl, 3
		and		bl, Flag_C
		shl		al, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB27	sla		a
void __declspec(naked) __fastcall OpCode_SLA_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_A]
		mov		bh, bl
		shr		bl, 3
		and		bl, Flag_C
		shl		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_AF], bx

		mov		eax, 2
		ret
	}
}

//0xCB28	sra		b
void __declspec(naked) __fastcall OpCode_SRA_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB29	sra		c
void __declspec(naked) __fastcall OpCode_SRA_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB2A	sra		d
void __declspec(naked) __fastcall OpCode_SRA_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB2B	sra		e
void __declspec(naked) __fastcall OpCode_SRA_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB2C	sra		h
void __declspec(naked) __fastcall OpCode_SRA_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB2D	sra		l
void __declspec(naked) __fastcall OpCode_SRA_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB2E	sra		[hl]
void __declspec(naked) __fastcall OpCode_SRA_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, al
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		al, 1
		or		al, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB2E	sra		[hl]
void __declspec(naked) __fastcall Debug_OpCode_SRA_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		al, 1
		or		al, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB2F	sra		a
void __declspec(naked) __fastcall OpCode_SRA_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_A]
		mov		bh, bl
		mov		ah, bl
		shl		bl, 4
		and		ah, 0x80
		and		bl, Flag_C
		shr		bh, 1
		or		bh, ah
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_A], bh

		mov		eax, 2
		ret
	}
}

//0xCB30	swap	b
void __declspec(naked) __fastcall OpCode_SWAP_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_B]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB31	swap	c
void __declspec(naked) __fastcall OpCode_SWAP_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_C]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB32	swap	d
void __declspec(naked) __fastcall OpCode_SWAP_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_D]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB33	swap	e
void __declspec(naked) __fastcall OpCode_SWAP_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_E]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB34	swap	h
void __declspec(naked) __fastcall OpCode_SWAP_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_H]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB35	swap	l
void __declspec(naked) __fastcall OpCode_SWAP_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bh, [ecx + Offset_Reg_L]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB36	swap	[hl]
void __declspec(naked) __fastcall OpCode_SWAP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, Flag_Z
		test	al, al
		jz		Zero

		mov		bl, al
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		al, 4
		or		al, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB36	swap	[hl]
void __declspec(naked) __fastcall Debug_OpCode_SWAP_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, Flag_Z
		test	al, al
		jz		Zero

		mov		bl, al
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		al, 4
		or		al, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB37	swap	a
void __declspec(naked) __fastcall OpCode_SWAP_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		ebx, [ecx + Offset_Reg_AF]
		mov		bl, Flag_Z
		test	bh, bh
		jz		Zero

		mov		bl, bh
		and		ebx, 0x0000F00F
		shl		bl, 4
		shr		bh, 4
		or		bh, bl
		xor		bl, bl
Zero:
		mov		[ecx + Offset_Reg_AF], bx

		mov		eax, 2
		ret
	}
}

//0xCB38	srl		b
void __declspec(naked) __fastcall OpCode_SRL_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_B]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_B], bh

		mov		eax, 2
		ret
	}
}

//0xCB39	srl		c
void __declspec(naked) __fastcall OpCode_SRL_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_C]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_C], bh

		mov		eax, 2
		ret
	}
}

//0xCB3A	srl		d
void __declspec(naked) __fastcall OpCode_SRL_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_D]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_D], bh

		mov		eax, 2
		ret
	}
}

//0xCB3B	srl		e
void __declspec(naked) __fastcall OpCode_SRL_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_E]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_E], bh

		mov		eax, 2
		ret
	}
}

//0xCB3C	srl		h
void __declspec(naked) __fastcall OpCode_SRL_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_H]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_H], bh

		mov		eax, 2
		ret
	}
}

//0xCB3D	srl		l
void __declspec(naked) __fastcall OpCode_SRL_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_L]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		mov		[ecx + Offset_Reg_L], bh

		mov		eax, 2
		ret
	}
}

//0xCB3E	srl		[hl]
void __declspec(naked) __fastcall OpCode_SRL_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		mov		bl, al
		shl		bl, 4
		and		bl, Flag_C
		shr		al, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB3E	srl		[hl]
void __declspec(naked) __fastcall Debug_OpCode_SRL_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		mov		bl, al
		shl		bl, 4
		and		bl, Flag_C
		shr		al, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB3F	srl		a
void __declspec(naked) __fastcall OpCode_SRL_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_A]
		mov		bh, bl
		shl		bl, 4
		and		bl, Flag_C
		shr		bh, 1
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_AF], bx

		mov		eax, 2
		ret
	}
}

//0xCB40	bit		0, b
void __declspec(naked) __fastcall OpCode_BIT_0_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB41	bit		0, c
void __declspec(naked) __fastcall OpCode_BIT_0_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB42	bit		0, d
void __declspec(naked) __fastcall OpCode_BIT_0_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB43	bit		0, e
void __declspec(naked) __fastcall OpCode_BIT_0_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB44	bit		0, h
void __declspec(naked) __fastcall OpCode_BIT_0_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB45	bit		0, l
void __declspec(naked) __fastcall OpCode_BIT_0_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB46	bit		0, [hl]
void __declspec(naked) __fastcall OpCode_BIT_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		test	al, 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB46	bit		0, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB47	bit		0, a
void __declspec(naked) __fastcall OpCode_BIT_0_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x01
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB48	bit		1, b
void __declspec(naked) __fastcall OpCode_BIT_1_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB49	bit		1, c
void __declspec(naked) __fastcall OpCode_BIT_1_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB4A	bit		1, d
void __declspec(naked) __fastcall OpCode_BIT_1_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB4B	bit		1, e
void __declspec(naked) __fastcall OpCode_BIT_1_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB4C	bit		1, h
void __declspec(naked) __fastcall OpCode_BIT_1_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB4D	bit		1, l
void __declspec(naked) __fastcall OpCode_BIT_1_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB4E	bit		1, [hl]
void __declspec(naked) __fastcall OpCode_BIT_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB4E	bit		1, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB4F	bit		1, a
void __declspec(naked) __fastcall OpCode_BIT_1_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x02
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB50	bit		2, b
void __declspec(naked) __fastcall OpCode_BIT_2_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB51	bit		2, c
void __declspec(naked) __fastcall OpCode_BIT_2_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB52	bit		2, d
void __declspec(naked) __fastcall OpCode_BIT_2_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB53	bit		2, e
void __declspec(naked) __fastcall OpCode_BIT_2_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB54	bit		2, h
void __declspec(naked) __fastcall OpCode_BIT_2_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB55	bit		2, l
void __declspec(naked) __fastcall OpCode_BIT_2_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB56	bit		2, [hl]
void __declspec(naked) __fastcall OpCode_BIT_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB56	bit		2, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB57	bit		2, a
void __declspec(naked) __fastcall OpCode_BIT_2_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x04
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB58	bit		3, b
void __declspec(naked) __fastcall OpCode_BIT_3_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB59	bit		3, c
void __declspec(naked) __fastcall OpCode_BIT_3_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB5A	bit		3, d
void __declspec(naked) __fastcall OpCode_BIT_3_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB5B	bit		3, e
void __declspec(naked) __fastcall OpCode_BIT_3_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB5C	bit		3, h
void __declspec(naked) __fastcall OpCode_BIT_3_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB5D	bit		3, l
void __declspec(naked) __fastcall OpCode_BIT_3_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB5E	bit		3, [hl]
void __declspec(naked) __fastcall OpCode_BIT_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB5E	bit		3, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB5F	bit		3, a
void __declspec(naked) __fastcall OpCode_BIT_3_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x08
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB60	bit		4, b
void __declspec(naked) __fastcall OpCode_BIT_4_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB61	bit		4, c
void __declspec(naked) __fastcall OpCode_BIT_4_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB62	bit		4, d
void __declspec(naked) __fastcall OpCode_BIT_4_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB63	bit		4, e
void __declspec(naked) __fastcall OpCode_BIT_4_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB64	bit		4, h
void __declspec(naked) __fastcall OpCode_BIT_4_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB65	bit		4, l
void __declspec(naked) __fastcall OpCode_BIT_4_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB66	bit		4, [hl]
void __declspec(naked) __fastcall OpCode_BIT_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB66	bit		4, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB67	bit		4, a
void __declspec(naked) __fastcall OpCode_BIT_4_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x10
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB68	bit		5, b
void __declspec(naked) __fastcall OpCode_BIT_5_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB69	bit		5, c
void __declspec(naked) __fastcall OpCode_BIT_5_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB6A	bit		5, d
void __declspec(naked) __fastcall OpCode_BIT_5_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB6B	bit		5, e
void __declspec(naked) __fastcall OpCode_BIT_5_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB6C	bit		5, h
void __declspec(naked) __fastcall OpCode_BIT_5_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB6D	bit		5, l
void __declspec(naked) __fastcall OpCode_BIT_5_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB6E	bit		5, [hl]
void __declspec(naked) __fastcall OpCode_BIT_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB6E	bit		5, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB6F	bit		5, a
void __declspec(naked) __fastcall OpCode_BIT_5_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x20
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB70	bit		6, b
void __declspec(naked) __fastcall OpCode_BIT_6_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB71	bit		6, c
void __declspec(naked) __fastcall OpCode_BIT_6_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB72	bit		6, d
void __declspec(naked) __fastcall OpCode_BIT_6_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB73	bit		6, e
void __declspec(naked) __fastcall OpCode_BIT_6_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB74	bit		6, h
void __declspec(naked) __fastcall OpCode_BIT_6_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB75	bit		6, l
void __declspec(naked) __fastcall OpCode_BIT_6_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB76	bit		6, [hl]
void __declspec(naked) __fastcall OpCode_BIT_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB76	bit		6, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB77	bit		6, a
void __declspec(naked) __fastcall OpCode_BIT_6_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x40
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB78	bit		7, b
void __declspec(naked) __fastcall OpCode_BIT_7_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_B], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB79	bit		7, c
void __declspec(naked) __fastcall OpCode_BIT_7_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_C], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB7A	bit		7, d
void __declspec(naked) __fastcall OpCode_BIT_7_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_D], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB7B	bit		7, e
void __declspec(naked) __fastcall OpCode_BIT_7_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_E], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB7C	bit		7, h
void __declspec(naked) __fastcall OpCode_BIT_7_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_H], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB7D	bit		7, l
void __declspec(naked) __fastcall OpCode_BIT_7_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_L], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB7E	bit		7, [hl]
void __declspec(naked) __fastcall OpCode_BIT_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret
	}
}

//0xCB7E	bit		7, [hl]
void __declspec(naked) __fastcall Debug_OpCode_BIT_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		call	ReadMem
		test	al, 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 3
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB7F	bit		7, a
void __declspec(naked) __fastcall OpCode_BIT_7_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		bl, [ecx + Offset_Reg_F]
		and		bl, Flag_C
		or		bl, Flag_H
		test	[ecx + Offset_Reg_A], 0x80
		jnz		NotZero
		or		bl, Flag_Z
NotZero:
		mov		[ecx + Offset_Reg_F], bl

		mov		eax, 2
		ret
	}
}

//0xCB80	res		0, b
void __declspec(naked) __fastcall OpCode_RES_0_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB81	res		0, c
void __declspec(naked) __fastcall OpCode_RES_0_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB82	res		0, d
void __declspec(naked) __fastcall OpCode_RES_0_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB83	res		0, e
void __declspec(naked) __fastcall OpCode_RES_0_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB84	res		0, h
void __declspec(naked) __fastcall OpCode_RES_0_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB85	res		0, l
void __declspec(naked) __fastcall OpCode_RES_0_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB86	res		0, [hl]
void __declspec(naked) __fastcall OpCode_RES_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xFE
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB86	res		0, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xFE
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB87	res		0, a
void __declspec(naked) __fastcall OpCode_RES_0_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xFE

		mov		eax, 2
		ret
	}
}

//0xCB88	res		1, b
void __declspec(naked) __fastcall OpCode_RES_1_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB89	res		1, c
void __declspec(naked) __fastcall OpCode_RES_1_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB8A	res		1, d
void __declspec(naked) __fastcall OpCode_RES_1_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB8B	res		1, e
void __declspec(naked) __fastcall OpCode_RES_1_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB8C	res		1, h
void __declspec(naked) __fastcall OpCode_RES_1_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB8D	res		1, l
void __declspec(naked) __fastcall OpCode_RES_1_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB8E	res		1, [hl]
void __declspec(naked) __fastcall OpCode_RES_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xFD
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB8E	res		1, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xFD
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB8F	res		1, a
void __declspec(naked) __fastcall OpCode_RES_1_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xFD

		mov		eax, 2
		ret
	}
}

//0xCB90	res		2, b
void __declspec(naked) __fastcall OpCode_RES_2_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB91	res		2, c
void __declspec(naked) __fastcall OpCode_RES_2_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB92	res		2, d
void __declspec(naked) __fastcall OpCode_RES_2_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB93	res		2, e
void __declspec(naked) __fastcall OpCode_RES_2_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB94	res		2, h
void __declspec(naked) __fastcall OpCode_RES_2_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB95	res		2, l
void __declspec(naked) __fastcall OpCode_RES_2_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB96	res		2, [hl]
void __declspec(naked) __fastcall OpCode_RES_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xFB
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB96	res		2, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xFB
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB97	res		2, a
void __declspec(naked) __fastcall OpCode_RES_2_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xFB

		mov		eax, 2
		ret
	}
}

//0xCB98	res		3, b
void __declspec(naked) __fastcall OpCode_RES_3_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB99	res		3, c
void __declspec(naked) __fastcall OpCode_RES_3_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB9A	res		3, d
void __declspec(naked) __fastcall OpCode_RES_3_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB9B	res		3, e
void __declspec(naked) __fastcall OpCode_RES_3_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB9C	res		3, h
void __declspec(naked) __fastcall OpCode_RES_3_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB9D	res		3, l
void __declspec(naked) __fastcall OpCode_RES_3_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCB9E	res		3, [hl]
void __declspec(naked) __fastcall OpCode_RES_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xF7
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCB9E	res		3, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xF7
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCB9F	res		3, a
void __declspec(naked) __fastcall OpCode_RES_3_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xF7

		mov		eax, 2
		ret
	}
}

//0xCBA0	res		4, b
void __declspec(naked) __fastcall OpCode_RES_4_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA1	res		4, c
void __declspec(naked) __fastcall OpCode_RES_4_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA2	res		4, d
void __declspec(naked) __fastcall OpCode_RES_4_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA3	res		4, e
void __declspec(naked) __fastcall OpCode_RES_4_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA4	res		4, h
void __declspec(naked) __fastcall OpCode_RES_4_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA5	res		4, l
void __declspec(naked) __fastcall OpCode_RES_4_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA6	res		4, [hl]
void __declspec(naked) __fastcall OpCode_RES_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xEF
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBA6	res		4, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xEF
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBA7	res		4, a
void __declspec(naked) __fastcall OpCode_RES_4_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xEF

		mov		eax, 2
		ret
	}
}

//0xCBA8	res		5, b
void __declspec(naked) __fastcall OpCode_RES_5_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBA9	res		5, c
void __declspec(naked) __fastcall OpCode_RES_5_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBAA	res		5, d
void __declspec(naked) __fastcall OpCode_RES_5_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBAB	res		5, e
void __declspec(naked) __fastcall OpCode_RES_5_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBAC	res		5, h
void __declspec(naked) __fastcall OpCode_RES_5_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBAD	res		5, l
void __declspec(naked) __fastcall OpCode_RES_5_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBAE	res		5, [hl]
void __declspec(naked) __fastcall OpCode_RES_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xDF
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBAE	res		5, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xDF
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBA8	res		5, a
void __declspec(naked) __fastcall OpCode_RES_5_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xDF

		mov		eax, 2
		ret
	}
}

//0xCBB0	res		6, b
void __declspec(naked) __fastcall OpCode_RES_6_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB1	res		6, c
void __declspec(naked) __fastcall OpCode_RES_6_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB2	res		6, d
void __declspec(naked) __fastcall OpCode_RES_6_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB3	res		6, e
void __declspec(naked) __fastcall OpCode_RES_6_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB4	res		6, h
void __declspec(naked) __fastcall OpCode_RES_6_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB5	res		6, l
void __declspec(naked) __fastcall OpCode_RES_6_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_L], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB6	res		6, [hl]
void __declspec(naked) __fastcall OpCode_RES_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0xBF
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBB6	res		6, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0xBF
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBB7	res		6, a
void __declspec(naked) __fastcall OpCode_RES_6_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0xBF

		mov		eax, 2
		ret
	}
}

//0xCBB8	res		7, b
void __declspec(naked) __fastcall OpCode_RES_7_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBB9	res		7, c
void __declspec(naked) __fastcall OpCode_RES_7_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_C], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBBA	res		7, d
void __declspec(naked) __fastcall OpCode_RES_7_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_D], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBBB	res		7, e
void __declspec(naked) __fastcall OpCode_RES_7_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_E], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBBC	res		7, h
void __declspec(naked) __fastcall OpCode_RES_7_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_H], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBBD	res		7, l
void __declspec(naked) __fastcall OpCode_RES_7_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_B], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBBE	res		7, [hl]
void __declspec(naked) __fastcall OpCode_RES_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		and		al, 0x7F
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBBE	res		7, [hl]
void __declspec(naked) __fastcall Debug_OpCode_RES_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		and		al, 0x7F
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBBF	res		7, a
void __declspec(naked) __fastcall OpCode_RES_7_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		and		byte ptr [ecx + Offset_Reg_A], 0x7F

		mov		eax, 2
		ret
	}
}

//0xCBC0	set		0, b
void __declspec(naked) __fastcall OpCode_SET_0_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC1	set		0, c
void __declspec(naked) __fastcall OpCode_SET_0_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC2	set		0, d
void __declspec(naked) __fastcall OpCode_SET_0_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC3	set		0, e
void __declspec(naked) __fastcall OpCode_SET_0_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC4	set		0, h
void __declspec(naked) __fastcall OpCode_SET_0_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC5	set		0, l
void __declspec(naked) __fastcall OpCode_SET_0_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC6	set		0, [hl]
void __declspec(naked) __fastcall OpCode_SET_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x01
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBC6	set		0, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_0_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x01
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBC7	set		0, a
void __declspec(naked) __fastcall OpCode_SET_0_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x01

		mov		eax, 2
		ret
	}
}

//0xCBC8	set		1, b
void __declspec(naked) __fastcall OpCode_SET_1_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBC9	set		1, c
void __declspec(naked) __fastcall OpCode_SET_1_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBCA	set		1, d
void __declspec(naked) __fastcall OpCode_SET_1_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBCB	set		1, e
void __declspec(naked) __fastcall OpCode_SET_1_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBCC	set		1, h
void __declspec(naked) __fastcall OpCode_SET_1_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBCD	set		1, l
void __declspec(naked) __fastcall OpCode_SET_1_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBCE	set		1, [hl]
void __declspec(naked) __fastcall OpCode_SET_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x02
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBCE	set		1, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_1_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x02
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBCF	set		1, a
void __declspec(naked) __fastcall OpCode_SET_1_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x02

		mov		eax, 2
		ret
	}
}

//0xCBD0	set		2, b
void __declspec(naked) __fastcall OpCode_SET_2_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD1	set		2, c
void __declspec(naked) __fastcall OpCode_SET_2_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD2	set		2, d
void __declspec(naked) __fastcall OpCode_SET_2_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD3	set		2, e
void __declspec(naked) __fastcall OpCode_SET_2_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD4	set		2, h
void __declspec(naked) __fastcall OpCode_SET_2_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD5	set		2, l
void __declspec(naked) __fastcall OpCode_SET_2_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD6	set		2, [hl]
void __declspec(naked) __fastcall OpCode_SET_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x04
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBD6	set		2, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_2_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x04
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBD7	set		2, a
void __declspec(naked) __fastcall OpCode_SET_2_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x04

		mov		eax, 2
		ret
	}
}

//0xCBD8	set		3, b
void __declspec(naked) __fastcall OpCode_SET_3_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBD9	set		3, c
void __declspec(naked) __fastcall OpCode_SET_3_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBDA	set		3, d
void __declspec(naked) __fastcall OpCode_SET_3_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBDB	set		3, e
void __declspec(naked) __fastcall OpCode_SET_3_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBDC	set		3, h
void __declspec(naked) __fastcall OpCode_SET_3_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBDD	set		3, l
void __declspec(naked) __fastcall OpCode_SET_3_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBDE	set		3, [hl]
void __declspec(naked) __fastcall OpCode_SET_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x08
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBDE	set		3, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_3_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x08
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBDF	set		3, a
void __declspec(naked) __fastcall OpCode_SET_3_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x08

		mov		eax, 2
		ret
	}
}

//0xCBE0	set		4, b
void __declspec(naked) __fastcall OpCode_SET_4_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE1	set		4, c
void __declspec(naked) __fastcall OpCode_SET_4_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE2	set		4, d
void __declspec(naked) __fastcall OpCode_SET_4_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE3	set		4, e
void __declspec(naked) __fastcall OpCode_SET_4_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE4	set		4, h
void __declspec(naked) __fastcall OpCode_SET_4_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE5	set		4, l
void __declspec(naked) __fastcall OpCode_SET_4_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE6	set		4, [hl]
void __declspec(naked) __fastcall OpCode_SET_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x10
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBE6	set		4, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_4_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x10
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBE7	set		4, a
void __declspec(naked) __fastcall OpCode_SET_4_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x10

		mov		eax, 2
		ret
	}
}

//0xCBE8	set		5, b
void __declspec(naked) __fastcall OpCode_SET_5_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBE9	set		5, c
void __declspec(naked) __fastcall OpCode_SET_5_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBEA	set		5, d
void __declspec(naked) __fastcall OpCode_SET_5_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBEB	set		5, e
void __declspec(naked) __fastcall OpCode_SET_5_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBEC	set		5, h
void __declspec(naked) __fastcall OpCode_SET_5_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBED	set		5, l
void __declspec(naked) __fastcall OpCode_SET_5_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBEE	set		5, [hl]
void __declspec(naked) __fastcall OpCode_SET_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x20
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBEE	set		5, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_5_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x20
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBEF	set		5, a
void __declspec(naked) __fastcall OpCode_SET_5_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x20

		mov		eax, 2
		ret
	}
}

//0xCBF0	set		6, b
void __declspec(naked) __fastcall OpCode_SET_6_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF1	set		6, c
void __declspec(naked) __fastcall OpCode_SET_6_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF2	set		6, d
void __declspec(naked) __fastcall OpCode_SET_6_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF3	set		6, e
void __declspec(naked) __fastcall OpCode_SET_6_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF4	set		6, h
void __declspec(naked) __fastcall OpCode_SET_6_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF5	set		6, l
void __declspec(naked) __fastcall OpCode_SET_6_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF6	set		6, [hl]
void __declspec(naked) __fastcall OpCode_SET_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x40
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBF6	set		6, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_6_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x40
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBF7	set		6, a
void __declspec(naked) __fastcall OpCode_SET_6_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x40

		mov		eax, 2
		ret
	}
}

//0xCBF8	set		7, b
void __declspec(naked) __fastcall OpCode_SET_7_B(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_B], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBF9	set		7, c
void __declspec(naked) __fastcall OpCode_SET_7_C(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_C], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBFA	set		7, d
void __declspec(naked) __fastcall OpCode_SET_7_D(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_D], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBFB	set		7, e
void __declspec(naked) __fastcall OpCode_SET_7_E(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_E], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBFC	set		7, h
void __declspec(naked) __fastcall OpCode_SET_7_H(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_H], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBFD	set		7, l
void __declspec(naked) __fastcall OpCode_SET_7_L(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_L], 0x80

		mov		eax, 2
		ret
	}
}

//0xCBFE	set		7, [hl]
void __declspec(naked) __fastcall OpCode_SET_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	ReadMem
		or		al, 0x80
		call	LD_mem8

		mov		eax, 4
		ret
	}
}

//0xCBFE	set		7, [hl]
void __declspec(naked) __fastcall Debug_OpCode_SET_7_HL(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		mov		edx, [ecx + Offset_Reg_HL]
		call	CheckReadAccess
		jc		AccessDenied
		call	CheckWriteAccess
		jc		AccessDenied
		call	ReadMem
		or		al, 0x80
		call	LD_mem8

		mov		eax, 4
		ret

AccessDenied:
		pop		eax
		ret
	}
}

//0xCBFF	set		7, a
void __declspec(naked) __fastcall OpCode_SET_7_A(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		or		byte ptr [ecx + Offset_Reg_A], 0x80

		mov		eax, 2
		ret
	}
}

void	*OpCodes_CB[256] = {	OpCode_RLC_B,		//0x00
								OpCode_RLC_C,
								OpCode_RLC_D,
								OpCode_RLC_E,
								OpCode_RLC_H,
								OpCode_RLC_L,
								OpCode_RLC_HL,
								OpCode_RLC_A,
								OpCode_RRC_B,		//0x08
								OpCode_RRC_C,
								OpCode_RRC_D,
								OpCode_RRC_E,
								OpCode_RRC_H,
								OpCode_RRC_L,
								OpCode_RRC_HL,
								OpCode_RRC_A,
								OpCode_RL_B,		//0x10
								OpCode_RL_C,
								OpCode_RL_D,
								OpCode_RL_E,
								OpCode_RL_H,
								OpCode_RL_L,
								OpCode_RL_HL,
								OpCode_RL_A,
								OpCode_RR_B,		//0x18
								OpCode_RR_C,
								OpCode_RR_D,
								OpCode_RR_E,
								OpCode_RR_H,
								OpCode_RR_L,
								OpCode_RR_HL,
								OpCode_RR_A,
								OpCode_SLA_B,		//0x20
								OpCode_SLA_C,
								OpCode_SLA_D,
								OpCode_SLA_E,
								OpCode_SLA_H,
								OpCode_SLA_L,
								OpCode_SLA_HL,
								OpCode_SLA_A,
								OpCode_SRA_B,		//0x28
								OpCode_SRA_C,
								OpCode_SRA_D,
								OpCode_SRA_E,
								OpCode_SRA_H,
								OpCode_SRA_L,
								OpCode_SRA_HL,
								OpCode_SRA_A,
								OpCode_SWAP_B,		//0x30
								OpCode_SWAP_C,
								OpCode_SWAP_D,
								OpCode_SWAP_E,
								OpCode_SWAP_H,
								OpCode_SWAP_L,
								OpCode_SWAP_HL,
								OpCode_SWAP_A,
								OpCode_SRL_B,		//0x38
								OpCode_SRL_C,
								OpCode_SRL_D,
								OpCode_SRL_E,
								OpCode_SRL_H,
								OpCode_SRL_L,
								OpCode_SRL_HL,
								OpCode_SRL_A,
								OpCode_BIT_0_B,		//0x40
								OpCode_BIT_0_C,
								OpCode_BIT_0_D,
								OpCode_BIT_0_E,
								OpCode_BIT_0_H,
								OpCode_BIT_0_L,
								OpCode_BIT_0_HL,
								OpCode_BIT_0_A,
								OpCode_BIT_1_B,		//0x48
								OpCode_BIT_1_C,
								OpCode_BIT_1_D,
								OpCode_BIT_1_E,
								OpCode_BIT_1_H,
								OpCode_BIT_1_L,
								OpCode_BIT_1_HL,
								OpCode_BIT_1_A,
								OpCode_BIT_2_B,		//0x50
								OpCode_BIT_2_C,
								OpCode_BIT_2_D,
								OpCode_BIT_2_E,
								OpCode_BIT_2_H,
								OpCode_BIT_2_L,
								OpCode_BIT_2_HL,
								OpCode_BIT_2_A,
								OpCode_BIT_3_B,		//0x58
								OpCode_BIT_3_C,
								OpCode_BIT_3_D,
								OpCode_BIT_3_E,
								OpCode_BIT_3_H,
								OpCode_BIT_3_L,
								OpCode_BIT_3_HL,
								OpCode_BIT_3_A,
								OpCode_BIT_4_B,		//0x60
								OpCode_BIT_4_C,
								OpCode_BIT_4_D,
								OpCode_BIT_4_E,
								OpCode_BIT_4_H,
								OpCode_BIT_4_L,
								OpCode_BIT_4_HL,
								OpCode_BIT_4_A,
								OpCode_BIT_5_B,		//0x68
								OpCode_BIT_5_C,
								OpCode_BIT_5_D,
								OpCode_BIT_5_E,
								OpCode_BIT_5_H,
								OpCode_BIT_5_L,
								OpCode_BIT_5_HL,
								OpCode_BIT_5_A,
								OpCode_BIT_6_B,		//0x70
								OpCode_BIT_6_C,
								OpCode_BIT_6_D,
								OpCode_BIT_6_E,
								OpCode_BIT_6_H,
								OpCode_BIT_6_L,
								OpCode_BIT_6_HL,
								OpCode_BIT_6_A,
								OpCode_BIT_7_B,		//0x78
								OpCode_BIT_7_C,
								OpCode_BIT_7_D,
								OpCode_BIT_7_E,
								OpCode_BIT_7_H,
								OpCode_BIT_7_L,
								OpCode_BIT_7_HL,
								OpCode_BIT_7_A,
								OpCode_RES_0_B,	//0x80
								OpCode_RES_0_C,
								OpCode_RES_0_D,
								OpCode_RES_0_E,
								OpCode_RES_0_H,
								OpCode_RES_0_L,
								OpCode_RES_0_HL,
								OpCode_RES_0_A,
								OpCode_RES_1_B,	//0x88
								OpCode_RES_1_C,
								OpCode_RES_1_D,
								OpCode_RES_1_E,
								OpCode_RES_1_H,
								OpCode_RES_1_L,
								OpCode_RES_1_HL,
								OpCode_RES_1_A,
								OpCode_RES_2_B,	//0x90
								OpCode_RES_2_C,
								OpCode_RES_2_D,
								OpCode_RES_2_E,
								OpCode_RES_2_H,
								OpCode_RES_2_L,
								OpCode_RES_2_HL,
								OpCode_RES_2_A,
								OpCode_RES_3_B,	//0x98
								OpCode_RES_3_C,
								OpCode_RES_3_D,
								OpCode_RES_3_E,
								OpCode_RES_3_H,
								OpCode_RES_3_L,
								OpCode_RES_3_HL,
								OpCode_RES_3_A,
								OpCode_RES_4_B,	//0xA0
								OpCode_RES_4_C,
								OpCode_RES_4_D,
								OpCode_RES_4_E,
								OpCode_RES_4_H,
								OpCode_RES_4_L,
								OpCode_RES_4_HL,
								OpCode_RES_4_A,
								OpCode_RES_5_B,	//0xA8
								OpCode_RES_5_C,
								OpCode_RES_5_D,
								OpCode_RES_5_E,
								OpCode_RES_5_H,
								OpCode_RES_5_L,
								OpCode_RES_5_HL,
								OpCode_RES_5_A,
								OpCode_RES_6_B,		//0xB0
								OpCode_RES_6_C,
								OpCode_RES_6_D,
								OpCode_RES_6_E,
								OpCode_RES_6_H,
								OpCode_RES_6_L,
								OpCode_RES_6_HL,
								OpCode_RES_6_A,
								OpCode_RES_7_B,	//0xB8
								OpCode_RES_7_C,
								OpCode_RES_7_D,
								OpCode_RES_7_E,
								OpCode_RES_7_H,
								OpCode_RES_7_L,
								OpCode_RES_7_HL,
								OpCode_RES_7_A,
								OpCode_SET_0_B,	//0xC0
								OpCode_SET_0_C,
								OpCode_SET_0_D,
								OpCode_SET_0_E,
								OpCode_SET_0_H,
								OpCode_SET_0_L,
								OpCode_SET_0_HL,
								OpCode_SET_0_A,
								OpCode_SET_1_B,	//0xC8
								OpCode_SET_1_C,
								OpCode_SET_1_D,
								OpCode_SET_1_E,
								OpCode_SET_1_H,
								OpCode_SET_1_L,
								OpCode_SET_1_HL,
								OpCode_SET_1_A,
								OpCode_SET_2_B,	//0xD0
								OpCode_SET_2_C,
								OpCode_SET_2_D,
								OpCode_SET_2_E,
								OpCode_SET_2_H,
								OpCode_SET_2_L,
								OpCode_SET_2_HL,
								OpCode_SET_2_A,
								OpCode_SET_3_B,	//0xD8
								OpCode_SET_3_C,
								OpCode_SET_3_D,
								OpCode_SET_3_E,
								OpCode_SET_3_H,
								OpCode_SET_3_L,
								OpCode_SET_3_HL,
								OpCode_SET_3_A,
								OpCode_SET_4_B,	//0xE0
								OpCode_SET_4_C,
								OpCode_SET_4_D,
								OpCode_SET_4_E,
								OpCode_SET_4_H,
								OpCode_SET_4_L,
								OpCode_SET_4_HL,
								OpCode_SET_4_A,
								OpCode_SET_5_B,	//0xE8
								OpCode_SET_5_C,
								OpCode_SET_5_D,
								OpCode_SET_5_E,
								OpCode_SET_5_H,
								OpCode_SET_5_L,
								OpCode_SET_5_HL,
								OpCode_SET_5_A,
								OpCode_SET_6_B,	//0xF0
								OpCode_SET_6_C,
								OpCode_SET_6_D,
								OpCode_SET_6_E,
								OpCode_SET_6_H,
								OpCode_SET_6_L,
								OpCode_SET_6_HL,
								OpCode_SET_6_A,
								OpCode_SET_7_B,	//0xF8
								OpCode_SET_7_C,
								OpCode_SET_7_D,
								OpCode_SET_7_E,
								OpCode_SET_7_H,
								OpCode_SET_7_L,
								OpCode_SET_7_HL,
								OpCode_SET_7_A
							};

void	*Debug_OpCodes_CB[256] = {
								OpCode_RLC_B,		//0x00
								OpCode_RLC_C,
								OpCode_RLC_D,
								OpCode_RLC_E,
								OpCode_RLC_H,
								OpCode_RLC_L,
								Debug_OpCode_RLC_HL,
								OpCode_RLC_A,
								OpCode_RRC_B,		//0x08
								OpCode_RRC_C,
								OpCode_RRC_D,
								OpCode_RRC_E,
								OpCode_RRC_H,
								OpCode_RRC_L,
								Debug_OpCode_RRC_HL,
								OpCode_RRC_A,
								OpCode_RL_B,		//0x10
								OpCode_RL_C,
								OpCode_RL_D,
								OpCode_RL_E,
								OpCode_RL_H,
								OpCode_RL_L,
								Debug_OpCode_RL_HL,
								OpCode_RL_A,
								OpCode_RR_B,		//0x18
								OpCode_RR_C,
								OpCode_RR_D,
								OpCode_RR_E,
								OpCode_RR_H,
								OpCode_RR_L,
								Debug_OpCode_RR_HL,
								OpCode_RR_A,
								OpCode_SLA_B,		//0x20
								OpCode_SLA_C,
								OpCode_SLA_D,
								OpCode_SLA_E,
								OpCode_SLA_H,
								OpCode_SLA_L,
								Debug_OpCode_SLA_HL,
								OpCode_SLA_A,
								OpCode_SRA_B,		//0x28
								OpCode_SRA_C,
								OpCode_SRA_D,
								OpCode_SRA_E,
								OpCode_SRA_H,
								OpCode_SRA_L,
								Debug_OpCode_SRA_HL,
								OpCode_SRA_A,
								OpCode_SWAP_B,		//0x30
								OpCode_SWAP_C,
								OpCode_SWAP_D,
								OpCode_SWAP_E,
								OpCode_SWAP_H,
								OpCode_SWAP_L,
								Debug_OpCode_SWAP_HL,
								OpCode_SWAP_A,
								OpCode_SRL_B,		//0x38
								OpCode_SRL_C,
								OpCode_SRL_D,
								OpCode_SRL_E,
								OpCode_SRL_H,
								OpCode_SRL_L,
								Debug_OpCode_SRL_HL,
								OpCode_SRL_A,
								OpCode_BIT_0_B,		//0x40
								OpCode_BIT_0_C,
								OpCode_BIT_0_D,
								OpCode_BIT_0_E,
								OpCode_BIT_0_H,
								OpCode_BIT_0_L,
								Debug_OpCode_BIT_0_HL,
								OpCode_BIT_0_A,
								OpCode_BIT_1_B,		//0x48
								OpCode_BIT_1_C,
								OpCode_BIT_1_D,
								OpCode_BIT_1_E,
								OpCode_BIT_1_H,
								OpCode_BIT_1_L,
								Debug_OpCode_BIT_1_HL,
								OpCode_BIT_1_A,
								OpCode_BIT_2_B,		//0x50
								OpCode_BIT_2_C,
								OpCode_BIT_2_D,
								OpCode_BIT_2_E,
								OpCode_BIT_2_H,
								OpCode_BIT_2_L,
								Debug_OpCode_BIT_2_HL,
								OpCode_BIT_2_A,
								OpCode_BIT_3_B,		//0x58
								OpCode_BIT_3_C,
								OpCode_BIT_3_D,
								OpCode_BIT_3_E,
								OpCode_BIT_3_H,
								OpCode_BIT_3_L,
								Debug_OpCode_BIT_3_HL,
								OpCode_BIT_3_A,
								OpCode_BIT_4_B,		//0x60
								OpCode_BIT_4_C,
								OpCode_BIT_4_D,
								OpCode_BIT_4_E,
								OpCode_BIT_4_H,
								OpCode_BIT_4_L,
								Debug_OpCode_BIT_4_HL,
								OpCode_BIT_4_A,
								OpCode_BIT_5_B,		//0x68
								OpCode_BIT_5_C,
								OpCode_BIT_5_D,
								OpCode_BIT_5_E,
								OpCode_BIT_5_H,
								OpCode_BIT_5_L,
								Debug_OpCode_BIT_5_HL,
								OpCode_BIT_5_A,
								OpCode_BIT_6_B,		//0x70
								OpCode_BIT_6_C,
								OpCode_BIT_6_D,
								OpCode_BIT_6_E,
								OpCode_BIT_6_H,
								OpCode_BIT_6_L,
								Debug_OpCode_BIT_6_HL,
								OpCode_BIT_6_A,
								OpCode_BIT_7_B,		//0x78
								OpCode_BIT_7_C,
								OpCode_BIT_7_D,
								OpCode_BIT_7_E,
								OpCode_BIT_7_H,
								OpCode_BIT_7_L,
								Debug_OpCode_BIT_7_HL,
								OpCode_BIT_7_A,
								OpCode_RES_0_B,	//0x80
								OpCode_RES_0_C,
								OpCode_RES_0_D,
								OpCode_RES_0_E,
								OpCode_RES_0_H,
								OpCode_RES_0_L,
								Debug_OpCode_RES_0_HL,
								OpCode_RES_0_A,
								OpCode_RES_1_B,	//0x88
								OpCode_RES_1_C,
								OpCode_RES_1_D,
								OpCode_RES_1_E,
								OpCode_RES_1_H,
								OpCode_RES_1_L,
								Debug_OpCode_RES_1_HL,
								OpCode_RES_1_A,
								OpCode_RES_2_B,	//0x90
								OpCode_RES_2_C,
								OpCode_RES_2_D,
								OpCode_RES_2_E,
								OpCode_RES_2_H,
								OpCode_RES_2_L,
								Debug_OpCode_RES_2_HL,
								OpCode_RES_2_A,
								OpCode_RES_3_B,	//0x98
								OpCode_RES_3_C,
								OpCode_RES_3_D,
								OpCode_RES_3_E,
								OpCode_RES_3_H,
								OpCode_RES_3_L,
								Debug_OpCode_RES_3_HL,
								OpCode_RES_3_A,
								OpCode_RES_4_B,	//0xA0
								OpCode_RES_4_C,
								OpCode_RES_4_D,
								OpCode_RES_4_E,
								OpCode_RES_4_H,
								OpCode_RES_4_L,
								Debug_OpCode_RES_4_HL,
								OpCode_RES_4_A,
								OpCode_RES_5_B,	//0xA8
								OpCode_RES_5_C,
								OpCode_RES_5_D,
								OpCode_RES_5_E,
								OpCode_RES_5_H,
								OpCode_RES_5_L,
								Debug_OpCode_RES_5_HL,
								OpCode_RES_5_A,
								OpCode_RES_6_B,		//0xB0
								OpCode_RES_6_C,
								OpCode_RES_6_D,
								OpCode_RES_6_E,
								OpCode_RES_6_H,
								OpCode_RES_6_L,
								Debug_OpCode_RES_6_HL,
								OpCode_RES_6_A,
								OpCode_RES_7_B,	//0xB8
								OpCode_RES_7_C,
								OpCode_RES_7_D,
								OpCode_RES_7_E,
								OpCode_RES_7_H,
								OpCode_RES_7_L,
								Debug_OpCode_RES_7_HL,
								OpCode_RES_7_A,
								OpCode_SET_0_B,	//0xC0
								OpCode_SET_0_C,
								OpCode_SET_0_D,
								OpCode_SET_0_E,
								OpCode_SET_0_H,
								OpCode_SET_0_L,
								Debug_OpCode_SET_0_HL,
								OpCode_SET_0_A,
								OpCode_SET_1_B,	//0xC8
								OpCode_SET_1_C,
								OpCode_SET_1_D,
								OpCode_SET_1_E,
								OpCode_SET_1_H,
								OpCode_SET_1_L,
								Debug_OpCode_SET_1_HL,
								OpCode_SET_1_A,
								OpCode_SET_2_B,	//0xD0
								OpCode_SET_2_C,
								OpCode_SET_2_D,
								OpCode_SET_2_E,
								OpCode_SET_2_H,
								OpCode_SET_2_L,
								Debug_OpCode_SET_2_HL,
								OpCode_SET_2_A,
								OpCode_SET_3_B,	//0xD8
								OpCode_SET_3_C,
								OpCode_SET_3_D,
								OpCode_SET_3_E,
								OpCode_SET_3_H,
								OpCode_SET_3_L,
								Debug_OpCode_SET_3_HL,
								OpCode_SET_3_A,
								OpCode_SET_4_B,	//0xE0
								OpCode_SET_4_C,
								OpCode_SET_4_D,
								OpCode_SET_4_E,
								OpCode_SET_4_H,
								OpCode_SET_4_L,
								Debug_OpCode_SET_4_HL,
								OpCode_SET_4_A,
								OpCode_SET_5_B,	//0xE8
								OpCode_SET_5_C,
								OpCode_SET_5_D,
								OpCode_SET_5_E,
								OpCode_SET_5_H,
								OpCode_SET_5_L,
								Debug_OpCode_SET_5_HL,
								OpCode_SET_5_A,
								OpCode_SET_6_B,	//0xF0
								OpCode_SET_6_C,
								OpCode_SET_6_D,
								OpCode_SET_6_E,
								OpCode_SET_6_H,
								OpCode_SET_6_L,
								Debug_OpCode_SET_6_HL,
								OpCode_SET_6_A,
								OpCode_SET_7_B,	//0xF8
								OpCode_SET_7_C,
								OpCode_SET_7_D,
								OpCode_SET_7_E,
								OpCode_SET_7_H,
								OpCode_SET_7_L,
								Debug_OpCode_SET_7_HL,
								OpCode_SET_7_A
							};

//0xCB
void __declspec(naked) __fastcall OpCode_CB(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	ReadMem
		inc		edx
		and		eax, 0xFF
		mov		word ptr [ecx + Offset_Reg_PC], dx

		call	dword ptr [OpCodes_CB + 4 * eax]

		ret
	}
}

//0xCB
void __declspec(naked) __fastcall Debug_OpCode_CB(CGameBoy *GB, DWORD PC)
{
	__asm
	{
		inc		dx
		call	CheckReadAccess
		jc		AccessDenied
		call	ReadMem
		and		eax, 0xFF

		call	dword ptr [Debug_OpCodes_CB + 4 * eax]

		add		word ptr [ecx + Offset_Reg_PC], 2
AccessDenied:
		ret
	}
}





void	*OpCodes[256] = {	OpCode_Nop,			//0x00
							OpCode_LD_BC_nnnn,
							OpCode_LD_BC_A,
							OpCode_INC_BC,
							OpCode_INC_B,
							OpCode_DEC_B,
							OpCode_LD_B_nn,
							OpCode_RLCA,
							OpCode_LD_nnnn_SP,	//0x08
							OpCode_ADD_HL_BC,
							OpCode_LD_A_BC,
							OpCode_DEC_BC,
							OpCode_INC_C,
							OpCode_DEC_C,
							OpCode_LD_C_nn,
							OpCode_RRCA,
							OpCode_STOP,		//0x10
							OpCode_LD_DE_nnnn,
							OpCode_LD_DE_A,
							OpCode_INC_DE,
							OpCode_INC_D,
							OpCode_DEC_D,
							OpCode_LD_D_nn,
							OpCode_RLA,
							OpCode_JR,			//0x18
							OpCode_ADD_HL_DE,
							OpCode_LD_A_DE,
							OpCode_DEC_DE,
							OpCode_INC_E,
							OpCode_DEC_E,
							OpCode_LD_E_nn,
							OpCode_RRA,
							OpCode_JR_NZ,		//0x20
							OpCode_LD_HL_nnnn,
							OpCode_LDI_HL_A,
							OpCode_INC_HL,
							OpCode_INC_H,
							OpCode_DEC_H,
							OpCode_LD_H_nn,
							OpCode_DAA,
							OpCode_JR_Z,		//0x28
							OpCode_ADD_HL_HL,
							OpCode_LDI_A_HL,
							OpCode_DEC_HL,
							OpCode_INC_L,
							OpCode_DEC_L,
							OpCode_LD_L_nn,
							OpCode_CPL,
							OpCode_JR_NC,		//0x30
							OpCode_LD_SP_nnnn,
							OpCode_LDD_HL_A,
							OpCode_INC_SP,
							OpCode_INC__HL_,
							OpCode_DEC__HL_,
							OpCode_LD_HL_nn,
							OpCode_SCF,
							OpCode_JR_C,		//0x38
							OpCode_ADD_HL_SP,
							OpCode_LDD_A_HL,
							OpCode_DEC_SP,
							OpCode_INC_A,
							OpCode_DEC_A,
							OpCode_LD_A_nn,
							OpCode_CCF,
							OpCode_LD_B_B,		//0x40
							OpCode_LD_B_C,
							OpCode_LD_B_D,
							OpCode_LD_B_E,
							OpCode_LD_B_H,
							OpCode_LD_B_L,
							OpCode_LD_B_HL,
							OpCode_LD_B_A,
							OpCode_LD_C_B,		//0x48
							OpCode_LD_C_C,
							OpCode_LD_C_D,
							OpCode_LD_C_E,
							OpCode_LD_C_H,
							OpCode_LD_C_L,
							OpCode_LD_C_HL,
							OpCode_LD_C_A,
							OpCode_LD_D_B,		//0x50
							OpCode_LD_D_C,
							OpCode_LD_D_D,
							OpCode_LD_D_E,
							OpCode_LD_D_H,
							OpCode_LD_D_L,
							OpCode_LD_D_HL,
							OpCode_LD_D_A,
							OpCode_LD_E_B,		//0x58
							OpCode_LD_E_C,
							OpCode_LD_E_D,
							OpCode_LD_E_E,
							OpCode_LD_E_H,
							OpCode_LD_E_L,
							OpCode_LD_E_HL,
							OpCode_LD_E_A,
							OpCode_LD_H_B,		//0x60
							OpCode_LD_H_C,
							OpCode_LD_H_D,
							OpCode_LD_H_E,
							OpCode_LD_H_H,
							OpCode_LD_H_L,
							OpCode_LD_H_HL,
							OpCode_LD_H_A,
							OpCode_LD_L_B,		//0x68
							OpCode_LD_L_C,
							OpCode_LD_L_D,
							OpCode_LD_L_E,
							OpCode_LD_L_H,
							OpCode_LD_L_L,
							OpCode_LD_L_HL,
							OpCode_LD_L_A,
							OpCode_LD_HL_B,		//0x70
							OpCode_LD_HL_C,
							OpCode_LD_HL_D,
							OpCode_LD_HL_E,
							OpCode_LD_HL_H,
							OpCode_LD_HL_L,
							OpCode_HALT,
							OpCode_LD_HL_A,
							OpCode_LD_A_B,		//0x78
							OpCode_LD_A_C,
							OpCode_LD_A_D,
							OpCode_LD_A_E,
							OpCode_LD_A_H,
							OpCode_LD_A_L,
							OpCode_LD_A_HL,
							OpCode_LD_A_A,
							OpCode_ADD_A_B,		//0x80
							OpCode_ADD_A_C,
							OpCode_ADD_A_D,
							OpCode_ADD_A_E,
							OpCode_ADD_A_H,
							OpCode_ADD_A_L,
							OpCode_ADD_A_HL,
							OpCode_ADD_A_A,
							OpCode_ADC_B,		//0x88
							OpCode_ADC_C,
							OpCode_ADC_D,
							OpCode_ADC_E,
							OpCode_ADC_H,
							OpCode_ADC_L,
							OpCode_ADC_HL,
							OpCode_ADC_A,
							OpCode_SUB_B,		//0x90
							OpCode_SUB_C,
							OpCode_SUB_D,
							OpCode_SUB_E,
							OpCode_SUB_H,
							OpCode_SUB_L,
							OpCode_SUB_HL,
							OpCode_SUB_A,
							OpCode_SBC_B,		//0x98
							OpCode_SBC_C,
							OpCode_SBC_D,
							OpCode_SBC_E,
							OpCode_SBC_H,
							OpCode_SBC_L,
							OpCode_SBC_HL,
							OpCode_SBC_A,
							OpCode_AND_B,		//0xA0
							OpCode_AND_C,
							OpCode_AND_D,
							OpCode_AND_E,
							OpCode_AND_H,
							OpCode_AND_L,
							OpCode_AND_HL,
							OpCode_AND_A,
							OpCode_XOR_B,		//0xA8
							OpCode_XOR_C,
							OpCode_XOR_D,
							OpCode_XOR_E,
							OpCode_XOR_H,
							OpCode_XOR_L,
							OpCode_XOR_HL,
							OpCode_XOR_A,
							OpCode_OR_B,		//0xB0
							OpCode_OR_C,
							OpCode_OR_D,
							OpCode_OR_E,
							OpCode_OR_H,
							OpCode_OR_L,
							OpCode_OR_HL,
							OpCode_OR_A,
							OpCode_CP_B,		//0xB8
							OpCode_CP_C,
							OpCode_CP_D,
							OpCode_CP_E,
							OpCode_CP_H,
							OpCode_CP_L,
							OpCode_CP_HL,
							OpCode_CP_A,
							OpCode_RET_NZ,		//0xC0
							OpCode_POP_BC,
							OpCode_JP_NZ,
							OpCode_JP,
							OpCode_CALL_NZ,
							OpCode_PUSH_BC,
							OpCode_ADD_A_nn,
							OpCode_RST_0,
							OpCode_RET_Z,		//0xC8
							OpCode_RET,
							OpCode_JP_Z,
							OpCode_CB,
							OpCode_CALL_Z,
							OpCode_CALL,
							OpCode_ADC_nn,
							OpCode_RST_8,
							OpCode_RET_NC,		//0xD0
							OpCode_POP_DE,
							OpCode_JP_NC,
							OpCode_Undefined,
							OpCode_CALL_NC,
							OpCode_PUSH_DE,
							OpCode_SUB_nn,
							OpCode_RST_10,
							OpCode_RET_C,		//0xD8
							OpCode_RETI,
							OpCode_JP_C,
							OpCode_Undefined,
							OpCode_CALL_C,
							OpCode_Undefined,
							OpCode_SBC_A_nn,
							OpCode_RST_18,
							OpCode_LD_FFnn_A,	//0xE0
							OpCode_POP_HL,
							OpCode_LD_FFC_A,
							OpCode_Undefined,
							OpCode_Undefined,
							OpCode_PUSH_HL,
							OpCode_AND_nn,
							OpCode_RST_20,
							OpCode_ADD_SP_nn,	//0xE8
							OpCode_JP_HL,
							OpCode_LD_nnnn_A,
							OpCode_Undefined,
							OpCode_Undefined,
							OpCode_Undefined,
							OpCode_XOR_nn,
							OpCode_RST_28,
							OpCode_LD_A_FFnn,	//0xF0
							OpCode_POP_AF,
							OpCode_LD_A_FFC,
							OpCode_DI,
							OpCode_Undefined,
							OpCode_PUSH_AF,
							OpCode_OR_nn,
							OpCode_RST_30,
							OpCode_LD_HL_SP_nn,	//0xF8
							OpCode_LD_SP_HL,
							OpCode_LD_A_nnnn,
							OpCode_EI,
							OpCode_Undefined,
							OpCode_Undefined,
							OpCode_CP_nn,
							OpCode_RST_38
						};



void	*DebugOpCodes[256] = {
							OpCode_Nop,			//0x00
							Debug_OpCode_LD_BC_nnnn,
							Debug_OpCode_LD_BC_A,
							OpCode_INC_BC,
							OpCode_INC_B,
							OpCode_DEC_B,
							Debug_OpCode_LD_B_nn,
							OpCode_RLCA,
							Debug_OpCode_LD_nnnn_SP,	//0x08
							OpCode_ADD_HL_BC,
							Debug_OpCode_LD_A_BC,
							OpCode_DEC_BC,
							OpCode_INC_C,
							OpCode_DEC_C,
							Debug_OpCode_LD_C_nn,
							OpCode_RRCA,
							OpCode_STOP,		//0x10
							Debug_OpCode_LD_DE_nnnn,
							Debug_OpCode_LD_DE_A,
							OpCode_INC_DE,
							OpCode_INC_D,
							OpCode_DEC_D,
							Debug_OpCode_LD_D_nn,
							OpCode_RLA,
							Debug_OpCode_JR,			//0x18
							OpCode_ADD_HL_DE,
							Debug_OpCode_LD_A_DE,
							OpCode_DEC_DE,
							OpCode_INC_E,
							OpCode_DEC_E,
							Debug_OpCode_LD_E_nn,
							OpCode_RRA,
							Debug_OpCode_JR_NZ,		//0x20
							Debug_OpCode_LD_HL_nnnn,
							Debug_OpCode_LDI_HL_A,
							OpCode_INC_HL,
							OpCode_INC_H,
							OpCode_DEC_H,
							Debug_OpCode_LD_H_nn,
							OpCode_DAA,
							Debug_OpCode_JR_Z,		//0x28
							OpCode_ADD_HL_HL,
							Debug_OpCode_LDI_A_HL,
							OpCode_DEC_HL,
							OpCode_INC_L,
							OpCode_DEC_L,
							Debug_OpCode_LD_L_nn,
							OpCode_CPL,
							Debug_OpCode_JR_NC,		//0x30
							Debug_OpCode_LD_SP_nnnn,
							Debug_OpCode_LDD_HL_A,
							OpCode_INC_SP,
							Debug_OpCode_INC__HL_,
							Debug_OpCode_DEC__HL_,
							Debug_OpCode_LD_HL_nn,
							OpCode_SCF,
							OpCode_JR_C,		//0x38
							OpCode_ADD_HL_SP,
							Debug_OpCode_LDD_A_HL,
							OpCode_DEC_SP,
							OpCode_INC_A,
							OpCode_DEC_A,
							Debug_OpCode_LD_A_nn,
							OpCode_CCF,
							OpCode_LD_B_B,		//0x40
							OpCode_LD_B_C,
							OpCode_LD_B_D,
							OpCode_LD_B_E,
							OpCode_LD_B_H,
							OpCode_LD_B_L,
							Debug_OpCode_LD_B_HL,
							OpCode_LD_B_A,
							OpCode_LD_C_B,		//0x48
							OpCode_LD_C_C,
							OpCode_LD_C_D,
							OpCode_LD_C_E,
							OpCode_LD_C_H,
							OpCode_LD_C_L,
							Debug_OpCode_LD_C_HL,
							OpCode_LD_C_A,
							OpCode_LD_D_B,		//0x50
							OpCode_LD_D_C,
							OpCode_LD_D_D,
							OpCode_LD_D_E,
							OpCode_LD_D_H,
							OpCode_LD_D_L,
							Debug_OpCode_LD_D_HL,
							OpCode_LD_D_A,
							OpCode_LD_E_B,		//0x58
							OpCode_LD_E_C,
							OpCode_LD_E_D,
							OpCode_LD_E_E,
							OpCode_LD_E_H,
							OpCode_LD_E_L,
							Debug_OpCode_LD_E_HL,
							OpCode_LD_E_A,
							OpCode_LD_H_B,		//0x60
							OpCode_LD_H_C,
							OpCode_LD_H_D,
							OpCode_LD_H_E,
							OpCode_LD_H_H,
							OpCode_LD_H_L,
							Debug_OpCode_LD_H_HL,
							OpCode_LD_H_A,
							OpCode_LD_L_B,		//0x68
							OpCode_LD_L_C,
							OpCode_LD_L_D,
							OpCode_LD_L_E,
							OpCode_LD_L_H,
							OpCode_LD_L_L,
							Debug_OpCode_LD_L_HL,
							OpCode_LD_L_A,
							Debug_OpCode_LD_HL_B,		//0x70
							Debug_OpCode_LD_HL_C,
							Debug_OpCode_LD_HL_D,
							Debug_OpCode_LD_HL_E,
							Debug_OpCode_LD_HL_H,
							Debug_OpCode_LD_HL_L,
							OpCode_HALT,
							Debug_OpCode_LD_HL_A,
							OpCode_LD_A_B,		//0x78
							OpCode_LD_A_C,
							OpCode_LD_A_D,
							OpCode_LD_A_E,
							OpCode_LD_A_H,
							OpCode_LD_A_L,
							Debug_OpCode_LD_A_HL,
							OpCode_LD_A_A,
							OpCode_ADD_A_B,		//0x80
							OpCode_ADD_A_C,
							OpCode_ADD_A_D,
							OpCode_ADD_A_E,
							OpCode_ADD_A_H,
							OpCode_ADD_A_L,
							Debug_OpCode_ADD_A_HL,
							OpCode_ADD_A_A,
							OpCode_ADC_B,		//0x88
							OpCode_ADC_C,
							OpCode_ADC_D,
							OpCode_ADC_E,
							OpCode_ADC_H,
							OpCode_ADC_L,
							Debug_OpCode_ADC_HL,
							OpCode_ADC_A,
							OpCode_SUB_B,		//0x90
							OpCode_SUB_C,
							OpCode_SUB_D,
							OpCode_SUB_E,
							OpCode_SUB_H,
							OpCode_SUB_L,
							Debug_OpCode_SUB_HL,
							OpCode_SUB_A,
							OpCode_SBC_B,		//0x98
							OpCode_SBC_C,
							OpCode_SBC_D,
							OpCode_SBC_E,
							OpCode_SBC_H,
							OpCode_SBC_L,
							Debug_OpCode_SBC_HL,
							OpCode_SBC_A,
							OpCode_AND_B,		//0xA0
							OpCode_AND_C,
							OpCode_AND_D,
							OpCode_AND_E,
							OpCode_AND_H,
							OpCode_AND_L,
							Debug_OpCode_AND_HL,
							OpCode_AND_A,
							OpCode_XOR_B,		//0xA8
							OpCode_XOR_C,
							OpCode_XOR_D,
							OpCode_XOR_E,
							OpCode_XOR_H,
							OpCode_XOR_L,
							Debug_OpCode_XOR_HL,
							OpCode_XOR_A,
							OpCode_OR_B,		//0xB0
							OpCode_OR_C,
							OpCode_OR_D,
							OpCode_OR_E,
							OpCode_OR_H,
							OpCode_OR_L,
							Debug_OpCode_OR_HL,
							OpCode_OR_A,
							OpCode_CP_B,		//0xB8
							OpCode_CP_C,
							OpCode_CP_D,
							OpCode_CP_E,
							OpCode_CP_H,
							OpCode_CP_L,
							Debug_OpCode_CP_HL,
							OpCode_CP_A,
							Debug_OpCode_RET_NZ,		//0xC0
							Debug_OpCode_POP_BC,
							Debug_OpCode_JP_NZ,
							Debug_OpCode_JP,
							Debug_OpCode_CALL_NZ,
							Debug_OpCode_PUSH_BC,
							Debug_OpCode_ADD_A_nn,
							Debug_OpCode_RST_0,
							Debug_OpCode_RET_Z,		//0xC8
							Debug_OpCode_RET,
							Debug_OpCode_JP_Z,
							Debug_OpCode_CB,
							Debug_OpCode_CALL_Z,
							Debug_OpCode_CALL,
							Debug_OpCode_ADC_nn,
							Debug_OpCode_RST_8,
							Debug_OpCode_RET_NC,		//0xD0
							Debug_OpCode_POP_DE,
							Debug_OpCode_JP_NC,
							OpCode_Undefined,
							Debug_OpCode_CALL_NC,
							Debug_OpCode_PUSH_DE,
							Debug_OpCode_SUB_nn,
							Debug_OpCode_RST_10,
							Debug_OpCode_RET_C,		//0xD8
							Debug_OpCode_RETI,
							Debug_OpCode_JP_C,
							OpCode_Undefined,
							Debug_OpCode_CALL_C,
							OpCode_Undefined,
							Debug_OpCode_SBC_A_nn,
							Debug_OpCode_RST_18,
							Debug_OpCode_LD_FFnn_A,	//0xE0
							Debug_OpCode_POP_HL,
							Debug_OpCode_LD_FFC_A,
							OpCode_Undefined,
							OpCode_Undefined,
							Debug_OpCode_PUSH_HL,
							Debug_OpCode_AND_nn,
							Debug_OpCode_RST_20,
							Debug_OpCode_ADD_SP_nn,	//0xE8
							OpCode_JP_HL,
							Debug_OpCode_LD_nnnn_A,
							OpCode_Undefined,
							OpCode_Undefined,
							OpCode_Undefined,
							Debug_OpCode_XOR_nn,
							Debug_OpCode_RST_28,
							Debug_OpCode_LD_A_FFnn,	//0xF0
							Debug_OpCode_POP_AF,
							Debug_OpCode_LD_A_FFC,
							OpCode_DI,
							OpCode_Undefined,
							Debug_OpCode_PUSH_AF,
							Debug_OpCode_OR_nn,
							Debug_OpCode_RST_30,
							Debug_OpCode_LD_HL_SP_nn,	//0xF8
							OpCode_LD_SP_HL,
							Debug_OpCode_LD_A_nnnn,
							OpCode_EI,
							OpCode_Undefined,
							OpCode_Undefined,
							Debug_OpCode_CP_nn,
							Debug_OpCode_RST_38
						};

