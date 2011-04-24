include		"hardware.inc"



section		"Rst_Irq", home[0]

RST_00:
	ds		8

RST_08:
	ds		8

RST_10:
	ds		8

RST_18:
	ds		8

RST_20:
	ds		8

RST_28:
	ds		8

RST_30:
	ds		8

RST_38:
	reti
	ds		7

;VBlank
	jp		irq_VBlank
	ds		5

;LCDC
	reti
	ds		7

;Timer
	reti
	ds		7

;Serial
	reti
	ds		7

;Key
	reti
	ds		7



section		"Main", home[$100]

	nop
	jp		Main

	db		$CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	db		$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	db		$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

			;123456789ABCDEF
	db		"GLDemo         "

	db		$C0	;GBC dedicated

	db		0	;No Super Gameboy support

	dw		0	;Licensee code

	db		0	;Cart type

	db		0	;ROM size
	db		0	;RAM size

	db		1	;Non-Japanese

	db		$33	;Old licensee code

	db		0	;Version
	db		0	;Complement check
	dw		0	;Checksum



Main:
	di

	ld		sp, StackTop

	ld		b, a

	ld		a, bank(Init)
	ldh		[ActiveBank], a
	ld		[$2100], a
	call	Init



	;End of program
	di
Hang:
	jr		Hang





section		"Irq", home

irq_VBlank:
	reti



;irq_LCDC:	reti
;irq_Timer:	reti
;irq_Serial:	reti
;irq_HiLo:	reti





section		"General", home

ScreenOff:
	;Disable V-Blank interrupt
	ldh		a, [rIE]
	push	af
	and		a, ~IEF_VBLANK
	ldh		[rIE], a

	;Wait for V-Blank
.Wait:
	ldh		a, [rLY]
	cp		a, 144
	jr		c, .Wait

	;Turn screen off
	ldh		a, [rLCDC]
	and		a, (~LCDCF_ON) & $FF
	ldh		[rLCDC], a

	;Restore interrupts
	pop		af
	ldh		[rIE], a

	ret



; MemCopy - Copies memory
;
; input:
;	hl	pSource
;	de	pDest
;	bc	bytecount

MemCopy:
	inc		b
	inc		c
	jr		.skip
.loop:
	ld		a, [hl+]
	ld		[de], a
	inc		de
.skip:
	dec		c
	jr		nz, .loop
	dec		b
	jr		nz, .loop
	ret



; MemSet
;
; input:
;	a	Value
;	hl	pDest
;	bc	bytecount

MemSet:
	inc		b
	inc		c
	jr		.skip
.loop:
	ld		[hl+], a
.skip:
	dec		c
	jr		nz, .loop
	dec		b
	jr		nz, .loop
	ret



; MemCopyBank - Copies memory from a ROM bank
;
; input:
;	a	Bank
;	hl	pSource
;	de	pDest
;	bc	bytecount

MemCopyBank:
	ld		[rROMB0], a

	inc		b
	inc		c
	jr		.skip
.loop:
	ld		a, [hl+]
	ld		[de], a
	inc		de
.skip:
	dec		c
	jr		nz, .loop
	dec		b
	jr		nz, .loop

	;Restore bank
	ldh		a, [ActiveBank]
	ld		[rROMB0], a

	ret





section		"Startup", code

Init:
	call	ScreenOff

	ld		a, b
	push	af
	cp		a, $11
	jr		nz, .NoSpeedChange
	ld		a, 1
	ldh		[rKEY1], a
	stop
.NoSpeedChange:

	xor		a
	ldh		[rVBK], a

	ld		hl, Font
	ld		de, $9000
	ld		bc, 16 * 16
	call	MemCopy

	ld		hl, Img
	ld		de, $9800
	ld		bc, 16
	call	MemCopy

	ld		a, 1
	ldh		[rVBK], a

	ld		de, $9800
	ld		bc, 20
	xor		a
	call	MemSet

	xor		a
	ldh		[rNR52], a	;All sound off
	ldh		[rSCY], a
	ldh		[rSCX], a
	ld		a, %11100100
	ldh		[rBGP], a
	ldh		[rOBP0], a
	ldh		[rOBP1], a

	pop		af
	cp		a, $11
	jr		nz, NotGBC

	ld		a, $80
	ldh		[rBCPS], a
	ld		hl, Palette1
	ld		b, 4 * 2
.Palette:
	ld		a, [hl+]
	ldh		[rBCPD], a
	dec		b
	jr		nz, .Palette

	ld		a, IEF_VBLANK
	ldh		[rIE], a
	ld		a, LCDCF_ON | LCDCF_BG8800 | LCDCF_BG9800 | LCDCF_BGON
	ldh		[rLCDC], a
	ei
	ret

NotGBC:
	di
.Hang:
	jp		.Hang



Palette1:
	dw	$0000, $1111, $2222, $3333
Img:
	db	"0123456789ABCDEF"

Font:
	DW	`03333300
	DW	`33333330
	DW	`33000330
	DW	`33000330
	DW	`33000330
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`00000330
	DW	`03333330
	DW	`33300000
	DW	`33333330
	DW	`33333330
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`00003330
	DW	`00033300
	DW	`00003330
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`33000330
	DW	`33000330
	DW	`33333330
	DW	`03333330
	DW	`00000330
	DW	`00000330
	DW	`00000330
	DW	`00000000

	DW	`33333330
	DW	`33333330
	DW	`33000000
	DW	`33333300
	DW	`00000330
	DW	`33333330
	DW	`33333300
	DW	`00000000

	DW	`03333330
	DW	`33333330
	DW	`33000000
	DW	`33333300
	DW	`33000330
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`33333330
	DW	`33333330
	DW	`00000330
	DW	`00003300
	DW	`00033000
	DW	`00033000
	DW	`00033000
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`33000330
	DW	`03333330
	DW	`33000330
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`33000330
	DW	`03333330
	DW	`00000330
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`33000330
	DW	`33333330
	DW	`33000330
	DW	`33000330
	DW	`33000330
	DW	`00000000

	DW	`33333300
	DW	`33333330
	DW	`33000330
	DW	`33333300
	DW	`33000330
	DW	`33333330
	DW	`33333300
	DW	`00000000

	DW	`03333300
	DW	`33333330
	DW	`33000000
	DW	`33000000
	DW	`33000000
	DW	`33333330
	DW	`03333300
	DW	`00000000

	DW	`33333300
	DW	`33333330
	DW	`33000330
	DW	`33000330
	DW	`33000330
	DW	`33333330
	DW	`33333300
	DW	`00000000

	DW	`33333330
	DW	`33333330
	DW	`33000000
	DW	`33333000
	DW	`33000000
	DW	`33333330
	DW	`33333330
	DW	`00000000

	DW	`33333330
	DW	`33333330
	DW	`33000000
	DW	`33333000
	DW	`33000000
	DW	`33000000
	DW	`33000000
	DW	`00000000





section		"HiRAM", hram

ActiveBank:
	ds		1





section		"Stack", bss

Stack:
	ds		$200
StackTop:

