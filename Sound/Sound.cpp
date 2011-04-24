#include	<windows.h>
#include	<dsound.h>

#define		SOUND_CPP
#include	"Sound.h"





LPDIRECTSOUND8			lpds = NULL;
LPDIRECTSOUNDBUFFER		lpdsbPrimary = NULL;





#ifdef _DEBUG

char		NumBuffer[10];

#define		Error(Text)												\
			OutputDebugString(__FILE__ "(");					\
			OutputDebugString(ultoa(__LINE__, NumBuffer, 10));	\
			OutputDebugString(") : ");							\
			OutputDebugString(Text);							\
			OutputDebugString("\n");

#else //_DEBUG

#define		Error(Text)

#endif //_DEBUG





void CloseSound()
{
	if (lpds)
	{
		if (lpdsbPrimary)
		{
			lpdsbPrimary->Stop();
			lpdsbPrimary->Release();
			lpdsbPrimary = NULL;
		}

		lpds->Release();
		lpds = NULL;
	}

	return;
}



/*extern "C" BOOL RestoreSound()
{
	if (lpds)
	{
		if (lpdsbPrimary)
		{
			if (lpdsbPrimary->Restore())
			{
				Error("lpdsbPrimary->Restore()");
				return true;
			}
		}
	}

	return false;
}*/



BOOL InitSound(HWND hWnd)
{
	DSBUFFERDESC	dsbdesc;


	if (!hWnd)
	{
		return true;
	}

	if (!lpds)
	{
		//Create DirectSound interface
		if (DirectSoundCreate8(NULL, &lpds, NULL))
		{
			Error("DirectSoundCreate8(NULL, &lpds, NULL)");
			return true;
		}

		//Set cooperative level
		if (lpds->SetCooperativeLevel(hWnd, DSSCL_NORMAL))
		{
			Error("lpds->SetCooperativeLevel(hWnd, DSSCL_NORMAL)");
			CloseSound();
			return true;
		}
	}


	//Get primary buffer and set it to run silently
	if (!lpdsbPrimary)
	{
		ZeroMemory(&dsbdesc, sizeof(dsbdesc));
		dsbdesc.dwSize = sizeof(dsbdesc);
		dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
		if (lpds->CreateSoundBuffer(&dsbdesc, &lpdsbPrimary, NULL))
		{
			Error("lpds->CreateSoundBuffer(&dsbdesc, &lpdsbPrimary, NULL)");
			CloseSound();
			return true;
		}

		if (lpdsbPrimary->Play(0, 0, DSBPLAY_LOOPING))
		{
			Error("lpdsbPrimary->Play(0, 0, DSBPLAY_LOOPING)");
			CloseSound();
			return true;
		}
	}

	return false;
}



void DestroySoundBuffer(SOUNDBUFFER *pSoundBuffer)
{
	if (pSoundBuffer->lpdsn)
	{
		pSoundBuffer->lpdsn->Release();
		pSoundBuffer->lpdsn = NULL;
	}
	if (pSoundBuffer->lpdsb)
	{
		pSoundBuffer->lpdsb->Stop();
		pSoundBuffer->lpdsb->Release();
		pSoundBuffer->lpdsb = NULL;
	}
	if (pSoundBuffer->hEvent[0])
	{
		CloseHandle(pSoundBuffer->hEvent[0]);
		pSoundBuffer->hEvent[0] = NULL;
	}
	if (pSoundBuffer->hEvent[1])
	{
		CloseHandle(pSoundBuffer->hEvent[1]);
		pSoundBuffer->hEvent[1] = NULL;
	}
	if (pSoundBuffer->hEvent[2])
	{
		CloseHandle(pSoundBuffer->hEvent[2]);
		pSoundBuffer->hEvent[2] = NULL;
	}

	DeleteCriticalSection(&pSoundBuffer->csSound);
}



DWORD CALLBACK SoundThreadProc(LPVOID lpParameter)
{
	BYTE		*p1;
	DWORD		dwSize1, dw;


	((SOUNDBUFFER *)lpParameter)->lpdsb->Lock(0, BufferSize * 3, (void **)&p1, &dwSize1, NULL, NULL, NULL);
	FillMemory(p1, dwSize1, 0x80);
	((SOUNDBUFFER *)lpParameter)->lpdsb->Unlock(p1, dwSize1, NULL, NULL);
	((SOUNDBUFFER *)lpParameter)->lpdsb->SetCurrentPosition(0);
	((SOUNDBUFFER *)lpParameter)->lpdsb->Play(NULL, NULL, DSBPLAY_LOOPING);

	while (!((SOUNDBUFFER *)lpParameter)->Close)
	{
		if (!((SOUNDBUFFER *)lpParameter)->IsPlaying)
		{
			((SOUNDBUFFER *)lpParameter)->lpdsb->Stop();
			((SOUNDBUFFER *)lpParameter)->lpdsb->Lock(0, BufferSize * 3, (void **)&p1, &dwSize1, NULL, NULL, NULL);
			FillMemory(p1, dwSize1, 0x80);
			((SOUNDBUFFER *)lpParameter)->lpdsb->Unlock(p1, dwSize1, NULL, NULL);
			((SOUNDBUFFER *)lpParameter)->lpdsb->SetCurrentPosition(0);

			while (!((SOUNDBUFFER *)lpParameter)->IsPlaying && ((SOUNDBUFFER *)lpParameter)->dwPosition < BufferSize)
			{
				if (((SOUNDBUFFER *)lpParameter)->Close)
				{
					break;
				}
				Sleep(0);
			}
			if (((SOUNDBUFFER *)lpParameter)->Close)
			{
				break;
			}

			((SOUNDBUFFER *)lpParameter)->lpdsb->Play(NULL, NULL, DSBPLAY_LOOPING);
		}


		ResetEvent(((SOUNDBUFFER *)lpParameter)->hEvent[0]);
		ResetEvent(((SOUNDBUFFER *)lpParameter)->hEvent[1]);
		ResetEvent(((SOUNDBUFFER *)lpParameter)->hEvent[2]);
		WaitForMultipleObjects(3, ((SOUNDBUFFER *)lpParameter)->hEvent, false, INFINITE);
		((SOUNDBUFFER *)lpParameter)->lpdsb->GetCurrentPosition(&dw, NULL);
		if (dw < BufferSize)
		{
			dw = BufferSize * 2;
		}
		else if (dw < BufferSize * 2)
		{
			dw = 0;
		}
		else
		{
			dw = BufferSize;
		}


		((SOUNDBUFFER *)lpParameter)->lpdsb->Lock(dw, BufferSize, (void **)&p1, &dwSize1, NULL, NULL, NULL);
		EnterCriticalSection(&((SOUNDBUFFER *)lpParameter)->csSound);
		if ((dw = ((SOUNDBUFFER *)lpParameter)->dwPosition) <= BufferSize)
		{
			CopyMemory(p1, &((SOUNDBUFFER *)lpParameter)->Buffer, ((SOUNDBUFFER *)lpParameter)->dwPosition);
			FillMemory(p1 + dw, dwSize1 - dw, *(p1 + dw - 1));
			((SOUNDBUFFER *)lpParameter)->lpdsb->Unlock(p1, dwSize1, NULL, NULL);
			((SOUNDBUFFER *)lpParameter)->dwPosition = 0;
		}
		else
		{
			CopyMemory(p1, &((SOUNDBUFFER *)lpParameter)->Buffer, BufferSize);
			((SOUNDBUFFER *)lpParameter)->lpdsb->Unlock(p1, dwSize1, NULL, NULL);
			((SOUNDBUFFER *)lpParameter)->dwPosition -= BufferSize;
			CopyMemory(&((SOUNDBUFFER *)lpParameter)->Buffer, (BYTE *)&((SOUNDBUFFER *)lpParameter)->Buffer + BufferSize, ((SOUNDBUFFER *)lpParameter)->dwPosition);
		}
		LeaveCriticalSection(&((SOUNDBUFFER *)lpParameter)->csSound);
	}


	DestroySoundBuffer((SOUNDBUFFER *)lpParameter);

	((SOUNDBUFFER *)lpParameter)->Close = false;

	return 0;
}



BOOL NewSoundBuffer(SOUNDBUFFER *pSoundBuffer)
{
	DSBPOSITIONNOTIFY	dsbpn[3];
	DSBUFFERDESC		dsbd;
	WAVEFORMATEX		wfx;
	DWORD				dw;


	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 22050;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY;
	dsbd.lpwfxFormat = &wfx;
	dsbd.dwBufferBytes = BufferSize * 3;

	if (lpds->CreateSoundBuffer(&dsbd, &pSoundBuffer->lpdsb, NULL))
	{
		return true;
	}

	if (!(pSoundBuffer->hEvent[0] = CreateEvent(NULL, false, false, NULL)))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}
	if (!(pSoundBuffer->hEvent[1] = CreateEvent(NULL, false, false, NULL)))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}
	if (!(pSoundBuffer->hEvent[2] = CreateEvent(NULL, false, false, NULL)))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}

	if (pSoundBuffer->lpdsb->QueryInterface(IID_IDirectSoundNotify, (void **)&pSoundBuffer->lpdsn))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}

	dsbpn[0].hEventNotify = pSoundBuffer->hEvent[0];
	dsbpn[0].dwOffset = 0;
	dsbpn[1].hEventNotify = pSoundBuffer->hEvent[1];
	dsbpn[1].dwOffset = BufferSize;
	dsbpn[2].hEventNotify = pSoundBuffer->hEvent[2];
	dsbpn[2].dwOffset = BufferSize * 2;
	if (pSoundBuffer->lpdsn->SetNotificationPositions(3, dsbpn))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}

	InitializeCriticalSection(&pSoundBuffer->csSound);

	if (!CreateThread(NULL, NULL, SoundThreadProc, pSoundBuffer, NULL, &dw))
	{
		DestroySoundBuffer(pSoundBuffer);
		return true;
	}

	return false;
}



BOOL		SND_VERSION_CalledOnce = false;

extern "C" DWORD GameLadSoundMain(DWORD dwAction, DWORD dwData, void *pData)
{
	if (dwAction == SND_VERSION)
	{
		if (SND_VERSION_CalledOnce)
		{
			return SND_FAIL;
		}
		if (!pData)
		{
			return SND_INVALIDPARAMETER;
		}
		if (((SND_VERSIONSTRUCT *)pData)->dwId != 0x4C47)
		{
			return SND_FAIL;
		}
		*((SND_VERSIONSTRUCT *)pData)->dwDllReleaseNo = SND_DLL_RELEASENO;
		if (((SND_VERSIONSTRUCT *)pData)->dwReleaseNo < SND_COMPATIBLERELEASE)
		{
			return SND_FAIL;
		}
		SND_VERSION_CalledOnce = true;
		return SND_OK;
	}

	if (!SND_VERSION_CalledOnce)
	{
		return SND_FAIL;
	}

	switch (dwAction)
	{
	case SND_INITSOUND:
		if (InitSound((HWND)dwData))
		{
			return SND_ERROR;
		}
		return SND_OK;

	case SND_CLOSESOUND:
		CloseSound();
		return SND_OK;

	case SND_CREATEBUFFER:
		return NewSoundBuffer((SOUNDBUFFER *)pData) ? SND_ERROR : SND_OK;
	}

	return SND_NOTIMPLEMENTED;
}

