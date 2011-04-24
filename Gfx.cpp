#include	<windows.h>
#include	"resource.h"

#define		GFX_CPP
#include	"Gfx.h"
#include	"Input.h"
#include	"Game Lad.h"



D3DFORMAT		OldD3DDisplayFormat;



BOOL InitGfx(IDirect3DDevice8 **pd3dd, HWND *hWin)
{
	D3DPRESENT_PARAMETERS	d3dpp;
	D3DDISPLAYMODE			d3ddm;


	if (!pd3d)
	{
		if (!(pd3d = Direct3DCreate8(D3D_SDK_VERSION)))
		{
			Error("Direct3DCreate8(D3D_SDK_VERSION)");
			return false;
		}
	}

	pd3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	D3DDisplayFormat = d3ddm.Format;
	switch (D3DDisplayFormat)
	{
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8R8G8B8:
		break;

	default:
		//MessageBox(hMsgParent, "Invalid display mode. Set screen to 15 bit colors or more.", NULL, MB_ICONERROR | MB_OK);
		return false;
	}

	if (!pd3dd)
	{
		return true;
	}

	if (Settings.AutoFullscreen && hWin)
	{
		ToggleFullscreen(pd3dd, hWin);
		return true;
	}

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferCount = 1;
	d3dpp.Windowed = true;
	d3dpp.BackBufferWidth = 160;
	d3dpp.BackBufferHeight = 144;
	d3dpp.BackBufferFormat = D3DDisplayFormat;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	if (pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, pd3dd))
	{
		Error("pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, pd3dd)");
		return false;
	}

	return true;
}



void SetZoomRects(DWORD dwZoom)
{
	rctSource.left = 0;
	rctSource.top = 0;
	rctSource.right = 160;
	rctSource.bottom = 144;
	rctDest.left = (Settings.FullscreenX - 160 * dwZoom) / 2;
	rctDest.top = (Settings.FullscreenY - 144 * dwZoom) / 2;
	rctDest.right = Settings.FullscreenX - (Settings.FullscreenX - 160 * dwZoom) / 2;
	rctDest.bottom = Settings.FullscreenY - (Settings.FullscreenY - 144 * dwZoom) / 2;
}



BOOL ToggleFullscreen(IDirect3DDevice8 **pd3dd, HWND *hWin)
{
	D3DPRESENT_PARAMETERS	d3dpp;
	DWORD					dwMaxZoom;


	EnterCriticalSection(&csGraphic);

	if (Settings.FullscreenBpp == D3DFMT_UNKNOWN)
	{
		LastOptionPage = OPTIONPAGE_GFX;
		PostMessage(hWnd, WM_COMMAND, ID_TOOLS_OPTIONS, 0);
		LeaveCriticalSection(&csGraphic);
		return false;
	}

	if (*pd3dd)
	{
		(*pd3dd)->Release();
		*pd3dd = NULL;
	}

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferCount = 1;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	if (*hWin)
	{
		DestroyWindow(*hWin);
		*hWin = NULL;

		d3dpp.Windowed = true;

		d3dpp.hDeviceWindow = hWnd;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferWidth = 160;
		d3dpp.BackBufferHeight = 144;
		d3dpp.BackBufferFormat = D3DDisplayFormat = OldD3DDisplayFormat;
	}
	else
	{
		dwMaxZoom = Settings.FullscreenX / 160 < Settings.FullscreenY / 144 ? Settings.FullscreenX / 160 : Settings.FullscreenY / 144;
		if (dwMaxZoom < 1)
		{
			LeaveCriticalSection(&csGraphic);
			return false;
		}

		if (!(*hWin = CreateWindowEx(WS_EX_TOPMOST, "Graphic", "Game Lad", WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL)))
		{
			Error("*hWin = CreateWindowEx(WS_EX_TOPMOST, \"Graphic\", \"Game Lad\", WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL)");
			LeaveCriticalSection(&csGraphic);
			return false;
		}

		d3dpp.Windowed = false;

		d3dpp.hDeviceWindow = *hWin;
		d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		d3dpp.BackBufferWidth = Settings.FullscreenX;
		d3dpp.BackBufferHeight = Settings.FullscreenY;
		OldD3DDisplayFormat = D3DDisplayFormat;
		d3dpp.BackBufferFormat = D3DDisplayFormat = (D3DFORMAT)Settings.FullscreenBpp;
		d3dpp.FullScreen_RefreshRateInHz = Settings.FullscreenRefreshRate;
		d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;

		switch (Settings.Fullscreen)
		{
		case FULLSCREEN_FIT:
			if (Settings.Fullscreen10_9)
			{
				SetZoomRects(dwMaxZoom);
			}
			else
			{
				rctSource.right = 160;
				rctSource.bottom = 144;
				rctDest.right = Settings.FullscreenX;
				rctDest.bottom = Settings.FullscreenY;
			}
			break;

		case FULLSCREEN_ZOOM:
			SetZoomRects(Settings.FullscreenZoom <= dwMaxZoom ? Settings.FullscreenZoom : dwMaxZoom);
			break;

		/*case FULLSCREEN_KEEPSIZE:
			rctSource.left = 0;
			rctSource.top = 0;
			rctSource.right = 160;
			rctSource.bottom = 144;
			GetClientRect(, );
			rctDest.left = (Settings.FullscreenX - 160 * dwZoom) / 2;
			rctDest.top = (Settings.FullscreenY - 144 * dwZoom) / 2;
			rctDest.right = Settings.FullscreenX - (Settings.FullscreenX - 160 * dwZoom) / 2;
			rctDest.bottom = Settings.FullscreenY - (Settings.FullscreenY - 160 * dwZoom) / 2;
			break;*/
		}
	}

	if (pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, *hWin, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, pd3dd))
	{
		Error("pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, *hWin, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, pd3dd)");
		DestroyWindow(*hWin);
		*hWin = NULL;
		LeaveCriticalSection(&csGraphic);
		return false;
	}

	//Make sure the screen is black
	(*pd3dd)->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
	(*pd3dd)->Present(NULL, NULL, *hWin, NULL);
	(*pd3dd)->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
	(*pd3dd)->Present(NULL, NULL, *hWin, NULL);

	LeaveCriticalSection(&csGraphic);

	return true;
}



void CloseGfx()
{
	if (pd3d)
	{
		pd3d->Release();
		pd3d = NULL;
	}
}

