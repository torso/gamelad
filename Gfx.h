#ifndef		GFX_H
#define		GFX_H

#include	<d3d8.h>



#ifdef		GFX_CPP
#define		EQUALNULL	= NULL
#else		//GFX_CPP
#define		GFX_CPP		extern
#define		EQUALNULL
#endif		//GFX_CPP



#ifndef		Error
#ifdef		_DEBUG

extern		char		NumBuffer[10];

#define		Error(Text)											\
			OutputDebugString(__FILE__ "(");					\
			OutputDebugString(ultoa(__LINE__, NumBuffer, 10));	\
			OutputDebugString(") : ");							\
			OutputDebugString(Text);							\
			OutputDebugString("\n");

#else		//_DEBUG

#define		Error(Text)

#endif		//_DEBUG
#endif		//Error



GFX_CPP		IDirect3D8			*pd3d EQUALNULL;
GFX_CPP		D3DFORMAT			D3DDisplayFormat;
GFX_CPP		RECT				rctSource, rctDest;


GFX_CPP		BOOL				InitGfx(IDirect3DDevice8 **pd3dd, HWND *hWin);
GFX_CPP		BOOL				ToggleFullscreen(IDirect3DDevice8 **pd3dd, HWND *hWin);
GFX_CPP		void				CloseGfx();



#undef		EQUALNULL

#endif		//GFX_H

