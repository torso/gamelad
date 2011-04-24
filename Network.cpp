#include	<windows.h>
#include	<dplay8.h>
#include	<dplobby8.h>
#include	"resource.h"

#define		NETWORK_CPP
#include	"Game Lad.h"
#include	"Network.h"



#define		RELEASEWND(hWin)	\
	if (hWin)					\
	{							\
		DestroyWindow(hWin);	\
		hWin = NULL;			\
	}



enum
{
	WM_APP_UPDATESTATUS = WM_APP_FIRSTFREEMESSAGE,
	WM_APP_PREPARENETWORKWINDOW,
	WM_APP_SAVEINCOMINGFILE
};



void PrepareNetworkWindow(HWND hWin);



struct		GAMELADINFO8
{
	char		Id[8];

	DWORD		dwSize;

	DWORD		ReleaseNo;
	DWORD		VersionMajor, VersionMinor;
	DWORD		LowestCompatibleReleaseNo;

	DWORD		dwStatus;

	char		szPlayerName[0x20], szComment[0x100];
};

typedef		GAMELADINFO8		GAMELADINFO;



GAMELADINFO		GameLadInfo = {"", sizeof(GameLadInfo), GAME_LAD_RELEASENO, VERSION_MAJOR, VERSION_MINOR, 8, 0};
GAMELADINFO		RemoteGameLadInfo = {"", 0};



struct		MSGHEADER
{
	DWORD		dwType;
	DWORD		dwSize;
};



struct		CHATMESSAGE
{
	MSGHEADER	MsgHeader;

	char		szMessage;
};



struct		SENDFILE
{
	MSGHEADER	MsgHeader;

	DWORD		dwStatus;
	DWORD		dwOffset;
	static union
	{
		char		szFile;
		BYTE		bData;
	};
};

enum
{
	SFS_OK = 0,
	SFS_CANCEL,
	SFS_ERROR,
	SFS_EOF,

	SFS_REQUESTSENDFILE,
};

char			szSendFile[MAX_PATH];
HANDLE			hSendFile = NULL;
BOOL			SendingFile;
DWORD			dwSendFileSize, dwSendFilePos;

struct		SENDFILE_1024
{
	SENDFILE	SendFile;
	BYTE		dummy[8191];
};



struct		EMULATIONSTATUS
{
	MSGHEADER	MsgHeader;		//Standard header for Game Lad messages

	DWORD		dwStatus;		//SE_ (see below)
	DWORD		dwLinkCableNo;	//Used when many link cables are emulated at the same time.
								//0 = all (only SE_STOP)
								//Only one cable is supported, should be 1 (or 0 for SE_STOP)

	DWORD		dwTicks;
	BYTE		SerialBit;
	BYTE		FF01, FF02;
};

enum
{
	SE_START = 0,				//Sent when emulation should be started.
								//Return DPNSUCCESS_PENDING if the other player has not started emulation.
								//Return S_OK if waiting for the other player.
	SE_STOP,					//Stop emulation or a start emulation request.
								//dwLinkCableNo == 0 stops all

	SE_SYNC,					//Send every ... emulated ticks
};



enum
{
	MSGTYPE_CHATMESSAGE = 0,
	MSGTYPE_SENDFILE,
	MSGTYPE_EMULATIONSTATUS
};



enum
{
	NS_DISCONNECTED = 0,

	NS_QUERYCONNECT,
	NS_WAITCONNECT,

	NS_CONNECTED_HOST,
	NS_CONNECTED
};



struct		PLAYER
{
	DPN_APPLICATION_DESC		dnAppDesc;
	IDirectPlay8Address			*pdpaHost, *pdpaDevice;
};



//{acdece20-a9d8-11d4-ace1-e0ae57c10001}
GUID							guidGameLad = {0xacdece20, 0xa9d8, 0x11d4, {0xac, 0xe1, 0xe0, 0xae, 0x57, 0xc1, 0x00, 0x01}};

IDirectPlay8Peer				*pdpp = NULL;
IDirectPlay8Address				*pdpaHost = NULL, *pdpaDevice = NULL;
DPN_SERVICE_PROVIDER_INFO		*pdnSPInfo = NULL;
DPN_APPLICATION_DESC			dnAppDesc;
DPNHANDLE						hEnumAsyncOp = NULL, hConnectAsyncOp = NULL, hMsgAsyncOp = NULL;

HWND							hNetworkList, hNext, hPrevious, hNetworkStatus;
HWND							hTextBox1_Static, hTextBox1, hTextBox2_Static, hTextBox2, hTextBox3_Static, hTextBox3;
HWND							hHost, hSend, hLinkCable, hSendFileWnd;

DWORD							dwPage;
UINT							NetworkStatusText;

char							szRemotePlayerName[0x20];
DPNID							dpnidRemotePlayer;
IDirectPlay8Address				*pdpaRemoteHost = NULL, *pdpaRemoteDevice = NULL;

SENDFILE_1024					SendFile_1024;

WNDPROC							pOldEditBoxProc;

DPNHANDLE						hEmulationStatusBuffer;
EMULATIONSTATUS					*pEmulationStatus = NULL;



DWORD InitNetworkLinkCable()
{
	static EMULATIONSTATUS		EmulationStatus;
	DPN_BUFFER_DESC				dpBufferDesc;


	EnterCriticalSection(&csNetwork);
	switch (NetworkLinkCable)
	{
	case NLC_TRUE:
	case NLC_WAITING:
	case NLC_INITIALIZING:
		LeaveCriticalSection(&csNetwork);
		return 0;
	}
	if (!hNetworkWnd)
	{
		LeaveCriticalSection(&csNetwork);
		return 0;
	}
	if (!hLinkCable)
	{
		LeaveCriticalSection(&csNetwork);
		return 0;
	}
	if (!Settings.NetworkLinkCable)
	{
		LeaveCriticalSection(&csNetwork);
		return 0;
	}

	if (NetworkLinkCable == NLC_FALSE)
	{
		NetworkLinkCable = NLC_WAITING;
	}
	else if (NetworkLinkCable == NLC_REMOTEWAITING)
	{
		NetworkLinkCable = NLC_INITIALIZING;
	}

	ZeroMemory(&EmulationStatus, sizeof(EmulationStatus));
	EmulationStatus.MsgHeader.dwSize = sizeof(EmulationStatus);
	EmulationStatus.MsgHeader.dwType = MSGTYPE_EMULATIONSTATUS;
	EmulationStatus.dwStatus = SE_START;
	EmulationStatus.dwLinkCableNo = 1;
	dpBufferDesc.dwBufferSize = EmulationStatus.MsgHeader.dwSize;
	dpBufferDesc.pBufferData = (BYTE *)&EmulationStatus;
	if (FAILED(pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, &EmulationStatus, &hMsgAsyncOp, DPNSEND_GUARANTEED)))
	{
		NetworkLinkCable = NLC_FALSE;
		LeaveCriticalSection(&csNetwork);
		return 0;
	}

	LeaveCriticalSection(&csNetwork);

	return 1;
}



void CloseNetworkLinkCable(DWORD dwLinkCableNo)
{
	static EMULATIONSTATUS		EmulationStatus;
	DPN_BUFFER_DESC				dpBufferDesc;


	EnterCriticalSection(&csNetwork);
	switch (NetworkLinkCable)
	{
	case NLC_FALSE:
	case NLC_REMOTEWAITING:
		LeaveCriticalSection(&csNetwork);
		return;
	}

	NetworkLinkCable = NLC_FALSE;

	ZeroMemory(&EmulationStatus, sizeof(EmulationStatus));
	EmulationStatus.MsgHeader.dwSize = sizeof(EmulationStatus);
	EmulationStatus.MsgHeader.dwType = MSGTYPE_EMULATIONSTATUS;
	EmulationStatus.dwStatus = SE_STOP;
	EmulationStatus.dwLinkCableNo = dwLinkCableNo;
	dpBufferDesc.dwBufferSize = EmulationStatus.MsgHeader.dwSize;
	dpBufferDesc.pBufferData = (BYTE *)&EmulationStatus;
	pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);

	LeaveCriticalSection(&csNetwork);
	return;
}



BOOL SyncNetworkLinkCable(DWORD dwLinkCableNo, DWORD dwTicks, BYTE SerialBit, BYTE FF01, BYTE FF02, DWORD *pdwTicks, BYTE *pSerialBit, BYTE *pFF01, BYTE *pFF02)
{
	static EMULATIONSTATUS		EmulationStatus;
	DPN_BUFFER_DESC				dpBufferDesc;


	EnterCriticalSection(&csNetwork);
	if (NetworkLinkCable != NLC_TRUE)
	{
		LeaveCriticalSection(&csNetwork);
		return false;
	}

	EmulationStatus.MsgHeader.dwSize = sizeof(EmulationStatus);
	EmulationStatus.MsgHeader.dwType = MSGTYPE_EMULATIONSTATUS;
	EmulationStatus.dwStatus = SE_SYNC;
	EmulationStatus.dwLinkCableNo = dwLinkCableNo;
	EmulationStatus.dwTicks = dwTicks;
	EmulationStatus.SerialBit = SerialBit;
	EmulationStatus.FF01 = FF01;
	EmulationStatus.FF02 = FF02;
	dpBufferDesc.dwBufferSize = EmulationStatus.MsgHeader.dwSize;
	dpBufferDesc.pBufferData = (BYTE *)&EmulationStatus;
	if (FAILED(pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, &EmulationStatus, &hMsgAsyncOp, DPNSEND_GUARANTEED)))
	{
		NetworkLinkCable = NLC_FALSE;
		LeaveCriticalSection(&csNetwork);
		return false;
	}

	LeaveCriticalSection(&csNetwork);

	while (!pEmulationStatus)
	{
		if (NetworkLinkCable != NLC_TRUE)
		{
			return false;
		}

		Sleep(0);
	}

	*pdwTicks = pEmulationStatus->dwTicks;
	*pSerialBit = pEmulationStatus->SerialBit;
	*pFF01 = pEmulationStatus->FF01;
	*pFF02 = pEmulationStatus->FF02;
	pdpp->ReturnBuffer(hEmulationStatusBuffer, 0);
	pEmulationStatus = NULL;

	return true;
}



void SendFileChunk()
{
	DWORD						nBytes;
	DPN_BUFFER_DESC				dpBufferDesc;


	ZeroMemory(&SendFile_1024.SendFile, sizeof(SendFile_1024.SendFile));
	SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
	dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;

	if (!ReadFile(hSendFile, &SendFile_1024.SendFile.bData, sizeof(SendFile_1024.SendFile.bData) + sizeof(SendFile_1024.dummy), &nBytes, NULL))
	{
		CloseHandle(hSendFile);
		hSendFile = NULL;
		SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
		SendFile_1024.SendFile.dwStatus = SFS_ERROR;
		dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
		pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
		NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
		PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
		LeaveCriticalSection(&csNetwork);
		return;
	}
	SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData) + nBytes;
	SendFile_1024.SendFile.dwOffset = dwSendFilePos;
	dwSendFilePos += nBytes;
	if (dwSendFilePos == dwSendFileSize)
	{
		SendFile_1024.SendFile.dwStatus = SFS_EOF;
	}
	else
	{
		SendFile_1024.SendFile.dwStatus = SFS_OK;
	}
	dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
	pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, &SendFile_1024, &hMsgAsyncOp, DPNSEND_GUARANTEED);
	NetworkStatusText = IDS_NETWORK_TRANSFERPROGRESS;
	PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
}



HRESULT WINAPI DirectPlayMessageHandler(void *pvUserContext, DWORD dwMessageId, void *pMsgBuffer)
{
	HRESULT						hr;
	PLAYER						*pPlayer;
	char						szBuffer[0x100];
	DWORD						dwPos, nBytes, nBytesWritten;
	BOOL						Selected;
	GAMELADINFO					*pgli;
	SENDFILE_1024				SendFile_1024;
	DPN_BUFFER_DESC				dpBufferDesc;


	switch (dwMessageId)
	{
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		EnterCriticalSection(&csNetwork);

		//Fail if no enumeration
		if (!hEnumAsyncOp)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}

		//Check duplicates
		if (dwPos = SendMessage(hNetworkList, LB_GETCOUNT, 0, 0))
		{
			while (dwPos--)
			{
				pPlayer = (PLAYER *)SendMessage(hNetworkList, LB_GETITEMDATA, dwPos, 0);
				if (!memcmp(&pPlayer->dnAppDesc.guidInstance, &((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pApplicationDescription->guidInstance, sizeof(GUID)))
				{
					//Refresh name
					Selected = (unsigned)SendMessage(hNetworkList, LB_GETCURSEL, 0, 0) == dwPos;
					WideCharToMultiByte(CP_ACP, 0, ((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pApplicationDescription->pwszSessionName, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
					SendMessage(hNetworkList, LB_DELETESTRING, dwPos, 0);
					switch (dwPos = SendMessage(hNetworkList, LB_ADDSTRING, 0, (LPARAM)szBuffer))
					{
					case LB_ERR:
					case LB_ERRSPACE:
						pPlayer->pdpaHost->Release();
						pPlayer->pdpaDevice->Release();
						delete pPlayer;
						LeaveCriticalSection(&csNetwork);
						return E_FAIL;
					}
					SendMessage(hNetworkList, LB_SETITEMDATA, dwPos, (LPARAM)pPlayer);
					if (Selected)
					{
						SendMessage(hNetworkList, LB_SETCURSEL, dwPos, 0);
					}
					LeaveCriticalSection(&csNetwork);
					return S_OK;
				}
			}
		}

		if (!(pPlayer = new PLAYER))
		{
			LeaveCriticalSection(&csNetwork);
			return E_OUTOFMEMORY;
		}
		CopyMemory(&pPlayer->dnAppDesc, ((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pApplicationDescription, sizeof(DPN_APPLICATION_DESC));
		if (FAILED(hr = ((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pAddressSender->Duplicate(&pPlayer->pdpaHost)))
		{
			delete pPlayer;
			LeaveCriticalSection(&csNetwork);
			return hr;
		}
		if (FAILED(hr = ((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pAddressDevice->Duplicate(&pPlayer->pdpaDevice)))
		{
			pPlayer->pdpaHost->Release();
			delete pPlayer;
			LeaveCriticalSection(&csNetwork);
			return hr;
		}

		WideCharToMultiByte(CP_ACP, 0, ((PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer)->pApplicationDescription->pwszSessionName, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

		switch (dwPos = SendMessage(hNetworkList, LB_ADDSTRING, 0, (LPARAM)szBuffer))
		{
		case LB_ERR:
		case LB_ERRSPACE:
			pPlayer->pdpaHost->Release();
			pPlayer->pdpaDevice->Release();
			delete pPlayer;
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}
		SendMessage(hNetworkList, LB_SETITEMDATA, dwPos, (LPARAM)pPlayer);
		LeaveCriticalSection(&csNetwork);
		return S_OK;

#define	GLI		((GAMELADINFO *)((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->pvUserConnectData)
	case DPN_MSGID_INDICATE_CONNECT:
		EnterCriticalSection(&csNetwork);
		if (((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->dwUserConnectDataSize < 12)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}
		if (memcmp(GLI->Id, "Game Lad", 8) || ((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->dwUserConnectDataSize != GLI->dwSize)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}

		//ID was correct. Return elaborate, Game Lad specific codes
		if (!(pgli = new GAMELADINFO))
		{
			LeaveCriticalSection(&csNetwork);
			return E_OUTOFMEMORY;
		}
		if (sizeof(GameLadInfo) >= GLI->dwSize)
		{
			CopyMemory(pgli, &GameLadInfo, sizeof(GameLadInfo));
		}
		else
		{
			CopyMemory(pgli, &GameLadInfo, GLI->dwSize);
		}

		((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->pvReplyData = pgli;
		((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->dwReplyDataSize = pgli->dwSize;
		((PDPNMSG_INDICATE_CONNECT)pMsgBuffer)->pvReplyContext = (void *)1;

		if (GLI->LowestCompatibleReleaseNo > GAME_LAD_RELEASENO)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}
		if (GameLadInfo.dwStatus != NS_WAITCONNECT)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}

		if (RemoteGameLadInfo.dwSize)
		{
			LeaveCriticalSection(&csNetwork);
			return E_FAIL;
		}

		CopyMemory(&RemoteGameLadInfo, GLI, sizeof(RemoteGameLadInfo));

		LeaveCriticalSection(&csNetwork);
		return S_OK;
#undef	GLI

	case DPN_MSGID_INDICATED_CONNECT_ABORTED:
		EnterCriticalSection(&csNetwork);
		RemoteGameLadInfo.dwSize = 0;
		LeaveCriticalSection(&csNetwork);
		return S_OK;

#define	GLI		((GAMELADINFO *)((PDPNMSG_CONNECT_COMPLETE)pMsgBuffer)->pvApplicationReplyData)
	case DPN_MSGID_CONNECT_COMPLETE:
		EnterCriticalSection(&csNetwork);
		if (!FAILED(((PDPNMSG_CONNECT_COMPLETE)pMsgBuffer)->hResultCode))
		{
			CopyMemory(&RemoteGameLadInfo, GLI, sizeof(RemoteGameLadInfo));
			dwPage = 2;
			GameLadInfo.dwStatus = NS_CONNECTED;
		}
		else
		{
			GameLadInfo.dwStatus = NS_DISCONNECTED;
		}
		PostMessage(hNetworkWnd, WM_APP_PREPARENETWORKWINDOW, 0, 0);
		LeaveCriticalSection(&csNetwork);
		return S_OK;
#undef	GLI

	case DPN_MSGID_CREATE_PLAYER:
		EnterCriticalSection(&csNetwork);
		if (((PDPNMSG_CREATE_PLAYER)pMsgBuffer)->pvPlayerContext != &GameLadInfo)
		{
			dpnidRemotePlayer = ((PDPNMSG_CREATE_PLAYER)pMsgBuffer)->dpnidPlayer;
			if (GameLadInfo.dwStatus == NS_WAITCONNECT)
			{
				GameLadInfo.dwStatus = NS_CONNECTED_HOST;
			}
			else
			{
				GameLadInfo.dwStatus = NS_CONNECTED;
			}
			PostMessage(hNetworkWnd, WM_APP_PREPARENETWORKWINDOW, 0, 0);
		}
		LeaveCriticalSection(&csNetwork);
		return S_OK;

	case DPN_MSGID_DESTROY_PLAYER:
		EnterCriticalSection(&csNetwork);
		if (((PDPNMSG_DESTROY_PLAYER)pMsgBuffer)->dpnidPlayer == dpnidRemotePlayer)
		{
			if (GameLadInfo.dwStatus == NS_CONNECTED_HOST)
			{
				GameLadInfo.dwStatus = NS_WAITCONNECT;
			}
			else
			{
				GameLadInfo.dwStatus = NS_DISCONNECTED;
				dwPage = 1;
			}
			RemoteGameLadInfo.dwSize = 0;
			if (pdpaRemoteHost)
			{
				pdpaRemoteHost->Release();
				pdpaRemoteHost = NULL;
			}
			if (pdpaRemoteDevice)
			{
				pdpaRemoteDevice->Release();
				pdpaRemoteDevice = NULL;
			}
			if (hSendFile)
			{
				CloseHandle(hSendFile);
				hSendFile = NULL;
			}
			PostMessage(hNetworkWnd, WM_APP_PREPARENETWORKWINDOW, 0, 0);
		}
		LeaveCriticalSection(&csNetwork);
		return S_OK;

#define	CM		((CHATMESSAGE *)((PDPNMSG_RECEIVE)pMsgBuffer)->pReceiveData)
	case DPN_MSGID_RECEIVE:
		if (((PDPNMSG_RECEIVE)pMsgBuffer)->dpnidSender != dpnidRemotePlayer)
		{
			return E_FAIL;
		}

		EnterCriticalSection(&csNetwork);

		switch (*((DWORD *)((PDPNMSG_RECEIVE)pMsgBuffer)->pReceiveData))
		{
		case MSGTYPE_CHATMESSAGE:
			SendMessage(hNetworkList, LB_ADDSTRING, 0, (LPARAM)&CM->szMessage);
			break;

#define	psf		((SENDFILE *)((PDPNMSG_RECEIVE)pMsgBuffer)->pReceiveData)
		case MSGTYPE_SENDFILE:
			switch (psf->dwStatus)
			{
			case SFS_EOF:
				if (!hSendFile)
				{
					break;
				}
				if (SendingFile)
				{
					CloseHandle(hSendFile);
					hSendFile = NULL;
					NetworkStatusText = IDS_NETWORK_TRANSFERCOMPLETE;
					PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
					break;
				}

			case SFS_OK:
				if (!hSendFile)
				{
					break;
				}
				if (SendingFile)
				{
					if (dwSendFilePos != psf->dwOffset)
					{
						if (SetFilePointer(hSendFile, psf->dwOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
						{
							CloseHandle(hSendFile);
							hSendFile = NULL;
							ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
							SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
							SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
							SendFile_1024.SendFile.dwStatus = SFS_ERROR;
							dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
							dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
							pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
							NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
							PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
							break;
						}
						dwSendFilePos = psf->dwOffset;
					}

					SendFileChunk();
				}
				else
				{
					nBytes = psf->MsgHeader.dwSize - (sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData));
					if (dwSendFilePos != psf->dwOffset)
					{
						if (psf->dwOffset != GetFileSize(hSendFile, NULL))
						{
							if (psf->dwOffset + nBytes == GetFileSize(hSendFile, NULL))
							{
								break;
							}
							ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
							SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
							SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
							SendFile_1024.SendFile.dwStatus = SFS_OK;
							dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
							dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
							pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
							break;
						}

						if (SetFilePointer(hSendFile, psf->dwOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
						{
							CloseHandle(hSendFile);
							hSendFile = NULL;
							ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
							SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
							SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
							SendFile_1024.SendFile.dwStatus = SFS_ERROR;
							dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
							dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
							pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
							NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
							PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
							break;
						}
						dwSendFilePos = psf->dwOffset;
					}

					if (!WriteFile(hSendFile, &psf->bData, nBytes, &nBytesWritten, NULL))
					{
						CloseHandle(hSendFile);
						hSendFile = NULL;
						ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
						SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
						SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
						SendFile_1024.SendFile.dwStatus = SFS_ERROR;
						dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
						dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
						pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
						NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
						PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
						break;
					}
					dwSendFilePos += nBytesWritten;
					if (nBytes != nBytesWritten)
					{
						CloseHandle(hSendFile);
						hSendFile = NULL;
						ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
						SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
						SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
						SendFile_1024.SendFile.dwStatus = SFS_ERROR;
						dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
						dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
						pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
						NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
						PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
						break;
					}
					if (psf->dwStatus == SFS_EOF)
					{
						ZeroMemory(&SendFile_1024, sizeof(SendFile_1024.SendFile));
						SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
						SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
						SendFile_1024.SendFile.dwStatus = SFS_EOF;
						SendFile_1024.SendFile.dwOffset = dwSendFilePos;
						dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
						dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
						pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, &SendFile_1024, &hMsgAsyncOp, DPNSEND_GUARANTEED);
					}

					NetworkStatusText = IDS_NETWORK_TRANSFERPROGRESS;
					PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
				}
				break;

			case SFS_CANCEL:
				if (!hSendFile)
				{
					break;
				}
				CloseHandle(hSendFile);
				hSendFile = NULL;
				NetworkStatusText = IDS_NETWORK_TRANSFERCANCELLED;
				PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
				break;

			case SFS_ERROR:
				if (!hSendFile)
				{
					break;
				}
				CloseHandle(hSendFile);
				hSendFile = NULL;
				NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
				PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
				break;

			case SFS_REQUESTSENDFILE:
				if (hSendFile)
				{
					ZeroMemory(&SendFile_1024.SendFile, sizeof(SendFile_1024.SendFile));
					SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
					SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
					dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
					dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
					SendFile_1024.SendFile.dwStatus = SFS_ERROR;
					pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
					break;
				}
				CopyString(szSendFile, &psf->szFile, sizeof(szSendFile));
				dwSendFileSize = psf->dwOffset;
				PostMessage(hNetworkWnd, WM_APP_SAVEINCOMINGFILE, 0, 0);
				break;
			}
			break;
#undef	psf

#define	pes		((EMULATIONSTATUS *)((PDPNMSG_RECEIVE)pMsgBuffer)->pReceiveData)
		case MSGTYPE_EMULATIONSTATUS:
			switch (pes->dwStatus)
			{
			case SE_START:
				if (pes->dwLinkCableNo != 1)
				{
					LeaveCriticalSection(&csNetwork);
					return E_FAIL;
				}
				switch (NetworkLinkCable)
				{
				case NLC_FALSE:
					NetworkLinkCable = NLC_REMOTEWAITING;
					break;

				case NLC_WAITING:
					NetworkLinkCable = NLC_TRUE;
					break;

				default:
					LeaveCriticalSection(&csNetwork);
					return E_FAIL;
				}
				break;

			case SE_STOP:
				if (pes->dwLinkCableNo > 1)
				{
					LeaveCriticalSection(&csNetwork);
					return E_FAIL;
				}
				switch (NetworkLinkCable)
				{
				case NLC_TRUE:
					NetworkLinkCable = NLC_FALSE;
					break;

				case NLC_REMOTEWAITING:
					NetworkLinkCable = NLC_FALSE;
					break;
				}
				break;

			case SE_SYNC:
				LeaveCriticalSection(&csNetwork);
				while (pEmulationStatus)
				{
					if (NetworkLinkCable != NLC_TRUE)
					{
						return E_FAIL;
					}
					Sleep(0);
				}
				hEmulationStatusBuffer = ((PDPNMSG_RECEIVE)pMsgBuffer)->hBufferHandle;
				pEmulationStatus = pes;
				return DPNSUCCESS_PENDING;
			}
			break;
#undef	pes
		}

		LeaveCriticalSection(&csNetwork);
		return S_OK;
#undef	CM

	case DPN_MSGID_SEND_COMPLETE:
		if (!((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->pvUserContext)
		{
			return S_OK;
		}
		switch (((MSGHEADER *)((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->pvUserContext)->dwType)
		{
#define	psf		((SENDFILE *)((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->pvUserContext)
		case MSGTYPE_SENDFILE:
			if (!hSendFile)
			{
				return S_OK;
			}

			EnterCriticalSection(&csNetwork);

			if (FAILED(((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->hResultCode))
			{
				CloseHandle(hSendFile);
				hSendFile = NULL;
				NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
				LeaveCriticalSection(&csNetwork);
				PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
				return S_OK;
			}

			if (!SendingFile)
			{
				if (psf->dwStatus == SFS_EOF)
				{
					CloseHandle(hSendFile);
					hSendFile = NULL;
					NetworkStatusText = IDS_NETWORK_TRANSFERCOMPLETE;
					LeaveCriticalSection(&csNetwork);
					PostMessage(hNetworkWnd, WM_APP_UPDATESTATUS, 0, 0);
				}
				return S_OK;
			}

			switch (psf->dwStatus)
			{
			case SFS_OK:
				SendFileChunk();
				break;

			case SFS_REQUESTSENDFILE:
				delete psf;
				break;
			}

			LeaveCriticalSection(&csNetwork);
			return S_OK;
#undef	psf

#define	pes		((EMULATIONSTATUS *)((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->pvUserContext)
		case MSGTYPE_EMULATIONSTATUS:
			if (FAILED(((PDPNMSG_SEND_COMPLETE)pMsgBuffer)->hResultCode))
			{
				EnterCriticalSection(&csNetwork);
				NetworkLinkCable = NLC_FALSE;
				LeaveCriticalSection(&csNetwork);
			}
			else if (NetworkLinkCable == NLC_INITIALIZING)
			{
				EnterCriticalSection(&csNetwork);
				NetworkLinkCable = NLC_TRUE;
				LeaveCriticalSection(&csNetwork);
			}
			return S_OK;
#undef	pes
		}
		return S_OK;

	case DPN_MSGID_RETURN_BUFFER:
		if (((PDPNMSG_RETURN_BUFFER)pMsgBuffer)->pvUserContext == (void *)1)
		{
			delete ((PDPNMSG_RETURN_BUFFER)pMsgBuffer)->pvBuffer;
		}
		return S_OK;
	}

	return S_OK;
}



void DeletePlayerList()
{
	DWORD			dwItem;
	PLAYER			*pPlayer;


	EnterCriticalSection(&csNetwork);

	if (hEnumAsyncOp)
	{
		pdpp->CancelAsyncOperation(hEnumAsyncOp, 0);
		hEnumAsyncOp = NULL;
	}

	dwItem = SendMessage(hNetworkList, LB_GETCOUNT, 0, 0);
	while (dwItem--)
	{
		pPlayer = (PLAYER *)SendMessage(hNetworkList, LB_GETITEMDATA, dwItem, 0);
		if (pPlayer->pdpaHost != pdpaRemoteHost)
		{
			pPlayer->pdpaHost->Release();
		}
		if (pPlayer->pdpaDevice != pdpaRemoteDevice)
		{
			pPlayer->pdpaDevice->Release();
		}
		delete pPlayer;
	}
	SendMessage(hNetworkList, LB_RESETCONTENT, 0, 0);

	LeaveCriticalSection(&csNetwork);
}



void CloseNetwork()
{
	EnterCriticalSection(&csNetwork);
	if (dwPage == 1)
	{
		DeletePlayerList();
	}

	if (pdnSPInfo)
	{
		delete pdnSPInfo;
		pdnSPInfo = NULL;
	}

	if (pdpp)
	{
		if (GameLadInfo.dwStatus == NS_CONNECTED_HOST)
		{
			pdpp->TerminateSession(NULL, 0, 0);
			GameLadInfo.dwStatus = NS_DISCONNECTED;
		}
		if (pdpaHost)
		{
			pdpaHost->Release();
			pdpaHost = NULL;
		}
		if (pdpaDevice)
		{
			pdpaDevice->Release();
			pdpaDevice = NULL;
		}
		if (pdpaRemoteHost)
		{
			pdpaRemoteHost->Release();
			pdpaRemoteHost = NULL;
		}
		if (pdpaRemoteDevice)
		{
			pdpaRemoteDevice->Release();
			pdpaRemoteDevice = NULL;
		}
		if (pdpp)
		{
			pdpp->Close(0);
			pdpp->Release();
			pdpp = NULL;
		}
		DestroyWindow(hNetworkWnd);
		hNetworkWnd = NULL;
	}
	NetworkLinkCable = NLC_FALSE;
	LeaveCriticalSection(&csNetwork);
}



BOOL InitNetwork()
{
	if (!pdpp)
	{
		if (FAILED(DirectPlay8Create(&IID_IDirectPlay8Peer, (void **)&pdpp, NULL)))
		{
			return false;
		}
	}

	if (FAILED(pdpp->Initialize(NULL, DirectPlayMessageHandler, 0)))
	{
		CloseNetwork();
		return false;
	}

	/*if (CoCreateInstance(CLSID_DirectPlay8Peer, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Peer, (void **)&pdpp))
	{
		CloseNetwork();
		return false;
	}
	pdpp->Initialize(NULL, DirectPlayMessageHandler, 0);*/

	return true;
}



BOOL EnumServiceProviders()
{
	DWORD			dwSize, dwItems, dwItem, dwPos;
	char			szBuffer[0x100];


	SendMessage(hNetworkList, LB_RESETCONTENT, 0, 0);

	if (pdnSPInfo)
	{
		delete pdnSPInfo;
		pdnSPInfo = NULL;
	}

	dwSize = dwItems = 0;
	if (FAILED(pdpp->EnumServiceProviders(NULL, NULL, NULL, &dwSize, &dwItems, 0) != DPNERR_BUFFERTOOSMALL))
	{
		CloseNetwork();
		return false;
	}
	if (!(pdnSPInfo = (DPN_SERVICE_PROVIDER_INFO *)new BYTE[dwSize]))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		CloseNetwork();
		return false;
	}
	if (FAILED(pdpp->EnumServiceProviders(NULL, NULL, pdnSPInfo, &dwSize, &dwItems, 0)))
	{
		CloseNetwork();
		return false;
	}

	for (dwItem = 0; dwItem < dwItems; dwItem++)
	{
		WideCharToMultiByte(CP_ACP, 0, pdnSPInfo[dwItem].pwszName, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
		switch (dwPos = SendMessage(hNetworkList, LB_ADDSTRING, 0, (LPARAM)szBuffer))
		{
		case LB_ERR:
		case LB_ERRSPACE:
			return false;
		}
		SendMessage(hNetworkList, LB_SETITEMDATA, dwPos, (LPARAM)&pdnSPInfo[dwItem].guid);
		if (!memcmp(&ServiceProviderGuid, &pdnSPInfo[dwItem].guid, sizeof(GUID)))
		{
			SendMessage(hNetworkList, LB_SETCURSEL, dwPos, 0);
		}
	}

	EnableWindow(hNext, true);

	return true;
}



LPARAM CALLBACK MsgEditBoxProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			if (dwPage == 2)
			{
				SendMessage(hNetworkWnd, WM_COMMAND, ID_SEND, 0);
				return 0;
			}
		}
		break;
	}

	return CallWindowProc(pOldEditBoxProc, hWin, uMsg, wParam, lParam);
}



void PrepareNetworkWindow(HWND hWin)
{
	RECT				rct;


	if (!pdpp)
	{
		return;
	}

	if (hNetworkWnd)
	{
		hWin = hNetworkWnd;
	}

	switch (dwPage)
	{
	case 0:
		if (hEnumAsyncOp)
		{
			pdpp->CancelAsyncOperation(hEnumAsyncOp, 0);
			hEnumAsyncOp = NULL;
		}
		if (pdpaHost)
		{
			pdpaHost->Release();
			pdpaHost = NULL;
		}
		if (pdpaDevice)
		{
			pdpaDevice->Release();
			pdpaDevice = NULL;
		}
		DeletePlayerList();

		RELEASEWND(hTextBox1_Static);
		RELEASEWND(hTextBox1);
		RELEASEWND(hTextBox2_Static);
		RELEASEWND(hTextBox2);
		RELEASEWND(hTextBox3_Static);
		RELEASEWND(hTextBox3);
		RELEASEWND(hHost);
		RELEASEWND(hSend);
		RELEASEWND(hLinkCable);
		RELEASEWND(hSendFileWnd);
		EnableWindow(hPrevious, false);
		EnumServiceProviders();
		break;

	case 1:
		if (GameLadInfo.dwStatus == NS_CONNECTED_HOST || GameLadInfo.dwStatus == NS_WAITCONNECT)
		{
			pdpp->TerminateSession(NULL, 0, 0);
			GameLadInfo.dwStatus = NS_DISCONNECTED;
		}
		else if (GameLadInfo.dwStatus == NS_CONNECTED /*|| GameLadInfo.dwStatus == NS_QUERYCONNECT*/)
		{
			pdpp->Close(0);
			GameLadInfo.dwStatus = NS_DISCONNECTED;
		}

		if (GameLadInfo.dwStatus != NS_DISCONNECTED)
		{
			if (hEnumAsyncOp)
			{
				pdpp->CancelAsyncOperation(hEnumAsyncOp, 0);
				hEnumAsyncOp = NULL;
				DeletePlayerList();
			}
		}
		else
		{
			if (!hEnumAsyncOp)
			{
				SendMessage(hNetworkList, LB_RESETCONTENT, 0, 0);

				//Enumerating hosts fail if there is not enough delay after stop hosting.
				Sleep(0);
				Sleep(0);
				Sleep(0);

				ZeroMemory(&dnAppDesc, sizeof(dnAppDesc));
				dnAppDesc.dwSize = sizeof(dnAppDesc);
				dnAppDesc.guidApplication = guidGameLad;
				switch (pdpp->EnumHosts(&dnAppDesc, pdpaHost, pdpaDevice, NULL, 0, INFINITE, 0, INFINITE, NULL, &hEnumAsyncOp, DPNENUMHOSTS_OKTOQUERYFORADDRESSING))
				{
				case S_OK:
				case DPNSUCCESS_PENDING:
					break;

				case DPNERR_INVALIDDEVICEADDRESS:
					__asm nop;
				case DPNERR_INVALIDFLAGS:
					__asm nop;
				case DPNERR_INVALIDHOSTADDRESS:
					__asm nop;
				case DPNERR_INVALIDPARAM:
					__asm nop;
				case DPNERR_ENUMQUERYTOOLARGE:
					__asm nop;
				default:
					pdpaHost->Release();
					pdpaHost = NULL;
					pdpaDevice->Release();
					pdpaDevice = NULL;
					dwPage = 0;
					MessageBox(hMsgParent, "pdpp->EnumHosts(&dnAppDesc, pdpaHost, pdpaDevice, NULL, 0, INFINITE, 0, INFINITE, NULL, &hEnumAsyncOp, DPNENUMHOSTS_OKTOQUERYFORADDRESSING)", NULL, MB_OK);
					PrepareNetworkWindow(hWin);
					return;
				}
			}
		}

		if (!hTextBox1_Static)
		{
			hTextBox1_Static = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox1_Static, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hTextBox1)
		{
			hTextBox1 = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_PASSWORD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox1, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hTextBox2_Static)
		{
			hTextBox2_Static = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox2_Static, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hTextBox2)
		{
			hTextBox2 = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox2, WM_SETFONT, (WPARAM)hFont, false);
			pOldEditBoxProc = (WNDPROC)SetWindowLong(hTextBox2, GWL_WNDPROC, (long)MsgEditBoxProc);
		}
		if (!hTextBox3_Static)
		{
			hTextBox3_Static = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox3_Static, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hTextBox3)
		{
			hTextBox3 = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox3, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hHost)
		{
			hHost = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hHost, WM_SETFONT, (WPARAM)hFont, false);
			SetWindowLong(hHost, GWL_ID, ID_HOST);
		}
		RELEASEWND(hSend);
		RELEASEWND(hLinkCable);
		RELEASEWND(hSendFileWnd);

		EnableWindow(hPrevious, true);
		if (GameLadInfo.dwStatus == NS_DISCONNECTED)
		{
			EnableWindow(hNext, true);
			EnableWindow(hNetworkList, true);
			EnableWindow(hTextBox1, true);
			EnableWindow(hTextBox2, true);
			EnableWindow(hTextBox3, true);
			EnableWindow(hHost, true);
		}
		else
		{
			EnableWindow(hNext, false);
			EnableWindow(hNetworkList, false);
			EnableWindow(hTextBox1, false);
			EnableWindow(hTextBox2, false);
			EnableWindow(hTextBox3, false);
			EnableWindow(hHost, false);
		}

		SetWindowText(hTextBox3, szPlayerName);
		break;

	case 2:
		if (hEnumAsyncOp)
		{
			pdpp->CancelAsyncOperation(hEnumAsyncOp, 0);
			hEnumAsyncOp = NULL;
			DeletePlayerList();
			SendMessage(hNetworkList, LB_RESETCONTENT, 0, 0);
		}

		RELEASEWND(hTextBox1_Static);
		RELEASEWND(hTextBox1);
		if (!hTextBox2_Static)
		{
			hTextBox2_Static = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox2_Static, WM_SETFONT, (WPARAM)hFont, false);
		}
		if (!hTextBox2)
		{
			hTextBox2 = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hTextBox2, WM_SETFONT, (WPARAM)hFont, false);
			pOldEditBoxProc = (WNDPROC)SetWindowLong(hTextBox2, GWL_WNDPROC, (long)MsgEditBoxProc);
		}
		RELEASEWND(hTextBox3_Static);
		RELEASEWND(hTextBox3);
		RELEASEWND(hHost);
		if (!hSend)
		{
			hSend = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hSend, WM_SETFONT, (WPARAM)hFont, false);
			SetWindowLong(hSend, GWL_ID, ID_SEND);
		}
		if (!hLinkCable)
		{
			hLinkCable = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hLinkCable, WM_SETFONT, (WPARAM)hFont, false);
			SetWindowLong(hLinkCable, GWL_ID, ID_NETWORKLINKCABLE);
		}
		if (!hSendFileWnd)
		{
			hSendFileWnd = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
			SendMessage(hSendFileWnd, WM_SETFONT, (WPARAM)hFont, false);
			SetWindowLong(hSendFileWnd, GWL_ID, ID_SENDFILE);
		}

		EnableWindow(hNetworkList, true);
		EnableWindow(hPrevious, true);
		EnableWindow(hNext, false);
		EnableWindow(hTextBox2, true);
		switch (GameLadInfo.dwStatus)
		{
		case NS_CONNECTED:
		case NS_CONNECTED_HOST:
			EnableWindow(hSend, true);
			EnableWindow(hLinkCable, true);
			EnableWindow(hSendFileWnd, true);
			break;

		default:
			EnableWindow(hSend, false);
			EnableWindow(hLinkCable, false);
			EnableWindow(hSendFileWnd, false);
		}
		break;
	}

	SendMessage(hWin, WM_APP_CHANGELANGUAGE, 0, 0);
	GetClientRect(hWin, &rct);
	SendMessage(hWin, WM_SIZE, 0, ((rct.bottom & 0xFFFF) << 16) | (rct.top & 0xFFFF));
}



LRESULT CALLBACK NetworkWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD				dwPos, dwSize, dwErrCode;
	RECT				rct;
	PLAYER				*pPlayer;
	DPN_PLAYER_INFO		dpPlayerInfo;
	DPN_BUFFER_DESC		dpBufferDesc;
	char				szBuffer[0x100], szBuffer2[0x100];
	WCHAR				wszBuffer[0x100], wszBuffer2[0x100];
	CHATMESSAGE			*pChatMessage;
	SENDFILE			*pSendFile;
	SENDFILE_1024		SendFile_1024;
	OPENFILENAME		of;


	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_PREVIOUS:
			EnterCriticalSection(&csNetwork);
			switch (dwPage)
			{
			case 1:
				if (hConnectAsyncOp)
				{
					//Cancel connect
					pdpp->CancelAsyncOperation(hConnectAsyncOp, 0);
					EnableWindow(hNetworkList, true);
					EnableWindow(hTextBox1, true);
					EnableWindow(hTextBox2, true);
					EnableWindow(hTextBox3, true);
					if (SendMessage(hNetworkList, LB_GETCURSEL, 0, 0) != LB_ERR)
					{
						EnableWindow(hNext, true);
					}
					EnableWindow(hHost, true);
				}
				else
				{
					dwPage = 0;
					PrepareNetworkWindow(NULL);
				}
				break;

			case 2:
				if (hConnectAsyncOp)
				{
					pdpp->CancelAsyncOperation(hConnectAsyncOp, 0);
					hConnectAsyncOp = NULL;
				}
				pdpp->Close(0);
				GameLadInfo.dwStatus = NS_DISCONNECTED;

				dwPage = 1;
				PrepareNetworkWindow(NULL);
				break;
			}
			LeaveCriticalSection(&csNetwork);
			return 0;

		case ID_NEXT:
			switch (dwPage)
			{
			case 0:
				EnterCriticalSection(&csNetwork);
				dwPos = SendMessage(hNetworkList, LB_GETCURSEL, 0, 0);
				if (dwPos == LB_ERR)
				{
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, String(IDS_NETWORK_SELECTSERVICEPROVIDER), "Game Lad", MB_OK | MB_ICONWARNING);
					return 0;
				}
				CopyMemory(&ServiceProviderGuid, (GUID *)SendMessage(hNetworkList, LB_GETITEMDATA, dwPos, 0), sizeof(GUID));
				if (FAILED(DirectPlay8AddressCreate(&IID_IDirectPlay8Address, (void **)&pdpaHost, NULL)))
				{
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, "DirectPlay8AddressCreate(&IID_IDirectPlay8Address, (void **)&pdpaHost, NULL)", NULL, MB_OK);
					return 0;
				}
				if (FAILED(pdpaHost->SetSP(&ServiceProviderGuid)))
				{
					pdpaHost->Release();
					pdpaHost = NULL;
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, "pdpaHost->SetSP(pGuid)", NULL, MB_OK);
					return 0;
				}
				if (FAILED(DirectPlay8AddressCreate(&IID_IDirectPlay8Address, (void **)&pdpaDevice, NULL)))
				{
					pdpaHost->Release();
					pdpaHost = NULL;
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, "DirectPlay8AddressCreate(&IID_IDirectPlay8Address, (void **)&pdpaDevice, NULL)", NULL, MB_OK);
					return 0;
				}
				if (FAILED(pdpaDevice->SetSP(&ServiceProviderGuid)))
				{
					pdpaHost->Release();
					pdpaHost = NULL;
					pdpaDevice->Release();
					pdpaDevice = NULL;
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, "pdpaDevice->SetSP(pGuid)", NULL, MB_OK);
					return 0;
				}

				dwPage = 1;
				PrepareNetworkWindow(NULL);

				LeaveCriticalSection(&csNetwork);
				break;

			case 1:
				EnterCriticalSection(&csNetwork);

				dwPos = SendMessage(hNetworkList, LB_GETCURSEL, 0, 0);
				if (dwPos == LB_ERR)
				{
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, String(IDS_NETWORK_SELECTPLAYER), "Game Lad", MB_OK | MB_ICONWARNING);
					SetFocus(hNetworkList);
					return 0;
				}

				GetWindowText(hTextBox3, GameLadInfo.szPlayerName, sizeof(GameLadInfo.szPlayerName));
				strcpy(szPlayerName, GameLadInfo.szPlayerName);
				if (szPlayerName[0] == '\0')
				{
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, String(IDS_NETWORK_ENTERNAME), "Game Lad", MB_OK | MB_ICONWARNING);
					SetFocus(hTextBox3);
					return 0;
				}
				ZeroMemory(&dpPlayerInfo, sizeof(dpPlayerInfo));
				dpPlayerInfo.dwSize = sizeof(dpPlayerInfo);
				dpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
				MultiByteToWideChar(CP_ACP, 0, szPlayerName, -1, wszBuffer, sizeof(wszBuffer));
				dpPlayerInfo.pwszName = wszBuffer;
				pdpp->SetPeerInfo(&dpPlayerInfo, NULL, NULL, DPNSETPEERINFO_SYNC);

				pPlayer = (PLAYER *)SendMessage(hNetworkList, LB_GETITEMDATA, dwPos, 0);
				GetWindowText(hTextBox2, GameLadInfo.szComment, sizeof(GameLadInfo.szComment));

				ZeroMemory(&dnAppDesc, sizeof(dnAppDesc));
				dnAppDesc.dwSize = sizeof(dnAppDesc);
				dnAppDesc.guidApplication = guidGameLad;
				GetWindowText(hTextBox1, szBuffer, sizeof(szBuffer));
				if (szBuffer[0] != '\0')
				{
					dnAppDesc.dwFlags = DPNSESSION_REQUIREPASSWORD;
					MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, wszBuffer2, sizeof(wszBuffer2));
					dnAppDesc.pwszPassword = wszBuffer2;
				}

				pdpaRemoteHost = pPlayer->pdpaHost;
				pdpaRemoteDevice = pPlayer->pdpaDevice;

				DeletePlayerList();

				switch (pdpp->Connect(&dnAppDesc, pdpaRemoteHost, pdpaRemoteDevice, NULL, NULL, &GameLadInfo, sizeof(GameLadInfo), &GameLadInfo, NULL, &hConnectAsyncOp, DPNCONNECT_OKTOQUERYFORADDRESSING))
				{
				case S_OK:
				case DPNSUCCESS_PENDING:
					break;

				default:
					LeaveCriticalSection(&csNetwork);
					MessageBox(hMsgParent, "pdpp->Connect(&dnAppDesc, pPlayer->pdpaHost, pPlayer->pdpaDevice, NULL, NULL, &GameLadInfo, sizeof(GameLadInfo), NULL, NULL, &hConnectAsyncOp, DPNCONNECT_OKTOQUERYFORADDRESSING)", NULL, MB_OK);
					PrepareNetworkWindow(NULL);
					return 0;
				}

				GameLadInfo.dwStatus = NS_QUERYCONNECT;
				PrepareNetworkWindow(NULL);
				LeaveCriticalSection(&csNetwork);
				break;
			}
			return 0;

		case ID_HOST:
			EnterCriticalSection(&csNetwork);
			GetWindowText(hTextBox3, szPlayerName, sizeof(szPlayerName));
			if (szPlayerName[0] == '\0')
			{
				LeaveCriticalSection(&csNetwork);
				MessageBox(hMsgParent, String(IDS_NETWORK_ENTERNAME), "Game Lad", MB_OK | MB_ICONWARNING);
				SetFocus(hTextBox3);
				return 0;
			}
			ZeroMemory(&dpPlayerInfo, sizeof(dpPlayerInfo));
			dpPlayerInfo.dwSize = sizeof(dpPlayerInfo);
			dpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
			MultiByteToWideChar(CP_ACP, 0, szPlayerName, -1, wszBuffer, sizeof(wszBuffer));
			dpPlayerInfo.pwszName = wszBuffer;
			pdpp->SetPeerInfo(&dpPlayerInfo, NULL, NULL, DPNSETPEERINFO_SYNC);

			strcpy(GameLadInfo.szPlayerName, szPlayerName);
			GetWindowText(hTextBox3, GameLadInfo.szComment, sizeof(GameLadInfo.szComment));

			ZeroMemory(&dnAppDesc, sizeof(dnAppDesc));
			dnAppDesc.dwSize = sizeof(dnAppDesc);
			dnAppDesc.guidApplication = guidGameLad;
			dnAppDesc.pwszSessionName = wszBuffer;
			GetWindowText(hTextBox1, szBuffer, sizeof(szBuffer));
			if (szBuffer[0] != '\0')
			{
				dnAppDesc.dwFlags = DPNSESSION_REQUIREPASSWORD;
				MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, wszBuffer2, sizeof(wszBuffer2));
				dnAppDesc.pwszPassword = wszBuffer2;
			}
			if (FAILED(pdpp->Host(&dnAppDesc, &pdpaDevice, 1, NULL, NULL, &GameLadInfo, DPNHOST_OKTOQUERYFORADDRESSING)))
			{
				pdpaHost->Release();
				pdpaHost = NULL;
				pdpaDevice->Release();
				pdpaDevice = NULL;
				LeaveCriticalSection(&csNetwork);
				MessageBox(hMsgParent, "pdpp->Host(&dnAppDesc, &pdpaDevice, 1, NULL, NULL, NULL, DPNHOST_OKTOQUERYFORADDRESSING)", NULL, MB_OK);
				return 0;
			}
			GameLadInfo.dwStatus = NS_WAITCONNECT;
			dwPage = 2;
			PrepareNetworkWindow(NULL);
			LeaveCriticalSection(&csNetwork);
			return 0;

		case ID_SEND:
			dwSize = GetWindowTextLength(hTextBox2) + 1;
			if (!(pChatMessage = (CHATMESSAGE *)new BYTE[sizeof(CHATMESSAGE) + strlen(GameLadInfo.szPlayerName) + 2 + dwSize]))
			{
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return 0;
			}
			ZeroMemory(pChatMessage, sizeof(CHATMESSAGE) + strlen(GameLadInfo.szPlayerName) + 2 + dwSize);
			pChatMessage->MsgHeader.dwType = MSGTYPE_CHATMESSAGE;
			pChatMessage->MsgHeader.dwSize = sizeof(CHATMESSAGE) + strlen(GameLadInfo.szPlayerName) + 2 + dwSize;
			(&pChatMessage->szMessage)[0] = '<';
			(&pChatMessage->szMessage)[1] = '\0';
			strcat(&pChatMessage->szMessage, GameLadInfo.szPlayerName);
			strcat(&pChatMessage->szMessage, "> ");
			GetWindowText(hTextBox2, &(&pChatMessage->szMessage)[strlen(&pChatMessage->szMessage)], dwSize);
			SendMessage(hNetworkList, LB_ADDSTRING, 0, (LPARAM)&pChatMessage->szMessage);
			dpBufferDesc.dwBufferSize = pChatMessage->MsgHeader.dwSize;
			dpBufferDesc.pBufferData = (BYTE *)pChatMessage;
			pdpp->SendTo(DPNID_ALL_PLAYERS_GROUP, &dpBufferDesc, 1, 0, (void *)1, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			SetWindowText(hTextBox2, NULL);
			return 0;

		case ID_SENDFILE:
			EnterCriticalSection(&csNetwork);
			if (hSendFile)
			{
				//Cancel transfer
				CloseHandle(hSendFile);
				hSendFile = NULL;

				ZeroMemory(&SendFile_1024.SendFile, sizeof(SendFile_1024.SendFile));
				SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
				SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
				SendFile_1024.SendFile.dwStatus = SFS_CANCEL;
				dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
				dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
				pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);

				SetWindowText(hSendFileWnd, String(IDS_NETWORK_SENDFILE));
				NetworkStatusText = IDS_NETWORK_TRANSFERCANCELLED;
				SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
				LeaveCriticalSection(&csNetwork);
				return 0;
			}

			ZeroMemory(&of, sizeof(of));
			of.lStructSize = sizeof(of);
			of.hwndOwner = hWnd;
			/*LoadString(IDS_OPEN_GBROM, szBuffer2, sizeof(szBuffer2));
			dwPos = strlen(szBuffer2) + 1;
			strcpy(szBuffer2 + dwPos, "*.GB;*.GBC");
			dwPos += strlen(szBuffer2 + dwPos) + 1;*/
			LoadString(IDS_OPEN_ALLFILES, szBuffer2, sizeof(szBuffer2));
			dwPos = strlen(szBuffer2) + 1;
			strcpy(szBuffer2 + dwPos, "*.*");
			dwPos += strlen(szBuffer2 + dwPos) + 1;
			*(szBuffer2 + dwPos) = '\0';
			of.lpstrFilter = szBuffer2;
			of.nFilterIndex = 1;
			of.lpstrFile = szSendFile;
			of.nMaxFile = sizeof(szSendFile);
			of.lpstrTitle = String(IDS_OPEN);
			of.lpstrInitialDir = Settings.szSendFileDir;
			of.Flags = /*OFN_ALLOWMULTISELECT |*/ OFN_HIDEREADONLY | OFN_FILEMUSTEXIST /*| OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING*/;
			//of.lpfnHook = OFNHookProc;
			szSendFile[0] = 0;

			if (!GetOpenFileName(&of))
			{
				LeaveCriticalSection(&csNetwork);
				return 0;
			}
			CopyString(Settings.szSendFileDir, szSendFile, of.nFileOffset);

			dwSize = strlen(&szSendFile[of.nFileOffset]);
			if (!(pSendFile = (SENDFILE *)new BYTE[sizeof(SENDFILE) + dwSize]))
			{
				LeaveCriticalSection(&csNetwork);
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return 0;
			}

			if ((hSendFile = CreateFile(szSendFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
			{
				LeaveCriticalSection(&csNetwork);
				DisplayErrorMessage();
				return 0;
			}
			SendingFile = true;

			ZeroMemory(pSendFile, sizeof(SENDFILE) + dwSize);
			pSendFile->MsgHeader.dwType = MSGTYPE_SENDFILE;
			pSendFile->MsgHeader.dwSize = sizeof(SENDFILE) + dwSize;
			pSendFile->dwOffset = dwSendFileSize = GetFileSize(hSendFile, NULL);
			dwSendFilePos = 0;
			pSendFile->dwStatus = SFS_REQUESTSENDFILE;
			strcpy(&pSendFile->szFile, &szSendFile[of.nFileOffset]);
			dpBufferDesc.dwBufferSize = pSendFile->MsgHeader.dwSize;
			dpBufferDesc.pBufferData = (BYTE *)pSendFile;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, pSendFile, &hMsgAsyncOp, DPNSEND_GUARANTEED);

			SetWindowText(hSendFileWnd, String(IDS_CANCEL));

			NetworkStatusText = IDS_NETWORK_QUERYSENDFILE;
			SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);

			LeaveCriticalSection(&csNetwork);
			return 0;

		case ID_NETWORKLINKCABLE:
			Settings.NetworkLinkCable = SendMessage(hLinkCable, BM_GETCHECK, 0, 0) == BST_CHECKED;
			return 0;
		}
		break;

	case WM_APP_SAVEINCOMINGFILE:
		ZeroMemory(&SendFile_1024.SendFile, sizeof(SendFile_1024.SendFile));
		SendFile_1024.SendFile.MsgHeader.dwType = MSGTYPE_SENDFILE;
		SendFile_1024.SendFile.MsgHeader.dwSize = sizeof(SendFile_1024.SendFile) - sizeof(SendFile_1024.SendFile.bData);
		dpBufferDesc.dwBufferSize = SendFile_1024.SendFile.MsgHeader.dwSize;
		dpBufferDesc.pBufferData = (BYTE *)&SendFile_1024.SendFile;
		if (hSendFile)
		{
			SendFile_1024.SendFile.dwStatus = SFS_ERROR;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			return 1;
		}

		ZeroMemory(&of, sizeof(of));
		of.lStructSize = sizeof(of);
		of.hwndOwner = hWnd;
		/*LoadString(IDS_OPEN_GBROM, szBuffer2, sizeof(szBuffer2));
		dwPos = strlen(szBuffer2) + 1;
		strcpy(szBuffer2 + dwPos, "*.GB;*.GBC");
		dwPos += strlen(szBuffer2 + dwPos) + 1;*/
		LoadString(IDS_OPEN_ALLFILES, szBuffer2, sizeof(szBuffer2));
		dwPos = strlen(szBuffer2) + 1;
		strcpy(szBuffer2 + dwPos, "*.*");
		dwPos += strlen(szBuffer2 + dwPos) + 1;
		*(szBuffer2 + dwPos) = '\0';
		of.lpstrFilter = szBuffer2;
		of.nFilterIndex = 1;
		of.lpstrFile = szSendFile;
		of.nMaxFile = sizeof(szSendFile);
		of.lpstrTitle = String(IDS_SAVEAS);
		of.lpstrInitialDir = Settings.szSendFileDir;
		of.Flags = OFN_HIDEREADONLY /*| OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING*/;
		//of.lpfnHook = OFNHookProc;

		if (!GetSaveFileName(&of))
		{
			SendFile_1024.SendFile.dwStatus = SFS_CANCEL;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			NetworkStatusText = IDS_NETWORK_TRANSFERCANCELLED;
			SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
			return 1;
		}

		EnterCriticalSection(&csNetwork);

		CopyString(Settings.szSendFileDir, szSendFile, of.nFileOffset);
		if ((hSendFile = CreateFile(szSendFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
		{
			dwErrCode = GetLastError();
			LeaveCriticalSection(&csNetwork);
			SendFile_1024.SendFile.dwStatus = SFS_ERROR;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			NetworkStatusText = NULL;
			SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
			DisplayErrorMessage(dwErrCode);
			return 1;
		}
		SendingFile = false;

		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			switch (MessageBox(hWin, String(IDS_NETWORK_OVERWRITEPROMPT), "Game Lad", MB_OK | MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2))
			{
			case IDYES:
				SetEndOfFile(hSendFile);
				break;

			case IDCANCEL:
				CloseHandle(hSendFile);
				hSendFile = NULL;
				LeaveCriticalSection(&csNetwork);
				SendFile_1024.SendFile.dwStatus = SFS_CANCEL;
				pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
				NetworkStatusText = IDS_NETWORK_TRANSFERCANCELLED;
				SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
				return 0;
			}
		}

		if ((SendFile_1024.SendFile.dwOffset = dwSendFilePos = GetFileSize(hSendFile, NULL)) == 0xFFFFFFFF)
		{
			CloseHandle(hSendFile);
			hSendFile = NULL;
			LeaveCriticalSection(&csNetwork);
			SendFile_1024.SendFile.dwStatus = SFS_CANCEL;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
			SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
			return 0;
		}
		if (SetFilePointer(hSendFile, dwSendFilePos, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			CloseHandle(hSendFile);
			hSendFile = NULL;
			LeaveCriticalSection(&csNetwork);
			SendFile_1024.SendFile.dwStatus = SFS_CANCEL;
			pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
			NetworkStatusText = IDS_NETWORK_TRANSFERERROR;
			SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
			return 0;
		}

		SendFile_1024.SendFile.dwStatus = SFS_OK;
		pdpp->SendTo(dpnidRemotePlayer, &dpBufferDesc, 1, 0, NULL, &hMsgAsyncOp, DPNSEND_GUARANTEED);
		NetworkStatusText = IDS_NETWORK_TRANSFERPROGRESS;
		LeaveCriticalSection(&csNetwork);
		SendMessage(hWin, WM_APP_UPDATESTATUS, 0, 0);
		return 0;

	case WM_APP_PREPARENETWORKWINDOW:
		EnterCriticalSection(&csNetwork);
		PrepareNetworkWindow(NULL);
		LeaveCriticalSection(&csNetwork);
		return 0;

	case WM_APP_CHANGELANGUAGE:
		SetWindowText(hWin, String(IDS_NETWORK_TITLE));
		SetWindowText(hPrevious, String(GameLadInfo.dwStatus == NS_QUERYCONNECT ? IDS_CANCEL : IDS_PREVIOUS));
		SetWindowText(hNext, String(dwPage == 1 ? IDS_NETWORK_JOIN : IDS_NEXT));
		switch (dwPage)
		{
		case 1:
			SetWindowText(hTextBox1_Static, String(IDS_NETWORK_PASSWORD));
			SetWindowText(hTextBox2_Static, String(IDS_NETWORK_COMMENT));
			SetWindowText(hTextBox3_Static, String(IDS_NETWORK_NAME));
			SetWindowText(hHost, String(IDS_NETWORK_HOST));
			break;

		case 2:
			SetWindowText(hTextBox2_Static, String(IDS_NETWORK_MESSAGE));
			SetWindowText(hSend, String(IDS_NETWORK_SEND));
			SetWindowText(hLinkCable, String(IDS_NETWORK_LINKCABLE));
			SetWindowText(hSendFileWnd, String(hSendFile ? IDS_CANCEL : IDS_NETWORK_SENDFILE));
			break;
		}
	case WM_APP_UPDATESTATUS:
		if (NetworkStatusText == IDS_NETWORK_TRANSFERPROGRESS)
		{
			LoadString(IDS_NETWORK_TRANSFERPROGRESS, szBuffer, sizeof(szBuffer), strchr(szSendFile, '\\') ? strrchr(szSendFile, '\\') + 1 : szSendFile);
			strcat(szBuffer, " ");
			strcat(szBuffer, itoa(dwSendFileSize != 0 ? (100 * dwSendFilePos) / dwSendFileSize : 0, NumBuffer, 10));
			strcat(szBuffer, "% (");
			strcat(szBuffer, itoa(dwSendFilePos, NumBuffer, 10));
			strcat(szBuffer, " / ");
			strcat(szBuffer, itoa(dwSendFileSize, NumBuffer, 10));
			strcat(szBuffer, ")");
			SetWindowText(hNetworkStatus, szBuffer);
		}
		else
		{
			switch (NetworkStatusText)
			{
			case IDS_NETWORK_QUERYSENDFILE:
			case IDS_NETWORK_TRANSFERCOMPLETE:
				SetWindowText(hNetworkStatus, LoadString(NetworkStatusText, szBuffer, sizeof(szBuffer), strchr(szSendFile, '\\') ? strrchr(szSendFile, '\\') + 1 : szSendFile));
				break;

			default:
				SetWindowText(hNetworkStatus, String(NetworkStatusText));
				break;
			}
		}
		return 0;

	case WM_SIZE:
		GetClientRect(hWin, &rct);

		MoveWindow(hNetworkList, 5, 5, rct.right - 10, rct.bottom - (hTextBox3 ? 130 : hTextBox2 ? 105 : hTextBox1 ? 80 : 55), true);
		if (hTextBox1)
		{
			MoveWindow(hTextBox1_Static, 5, rct.bottom - 70, 80, 20, true);
			MoveWindow(hTextBox1, 90, rct.bottom - 70, rct.right - 95, 20, true);
		}
		if (hTextBox2)
		{
			MoveWindow(hTextBox2_Static, 5, rct.bottom - 95, 80, 20, true);
			if (!hSend)
			{
				MoveWindow(hTextBox2, 90, rct.bottom - 95, rct.right - 95, 20, true);
			}
			else
			{
				MoveWindow(hTextBox2, 90, rct.bottom - 95, rct.right - 180, 20, true);
			}
		}
		if (hTextBox3)
		{
			MoveWindow(hTextBox3_Static, 5, rct.bottom - 120, 80, 20, true);
			MoveWindow(hTextBox3, 90, rct.bottom - 120, rct.right - 95, 20, true);
		}
		if (hSend)
		{
			MoveWindow(hSend, rct.right - 85, rct.bottom - 95, 80, 20, true);
		}
		if (hLinkCable)
		{
			MoveWindow(hLinkCable, 5, rct.bottom - 70, rct.right - 95, 20, true);
		}
		if (hSendFileWnd)
		{
			MoveWindow(hSendFileWnd, rct.right - 85, rct.bottom - 70, 80, 20, true);
		}
		MoveWindow(hNetworkStatus, 5, rct.bottom - 45, rct.right - 10, 15, true);
		MoveWindow(hPrevious, 5, rct.bottom - 25, 80, 20, true);
		MoveWindow(hNext, 90, rct.bottom - 25, 80, 20, true);
		if (hHost)
		{
			MoveWindow(hHost, 175, rct.bottom - 25, 80, 20, true);
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
		CloseNetwork();
		return 0;

	case WM_CREATE:
		dwPage = 0;

		CopyMemory(GameLadInfo.Id, "Game Lad", 8);
		GameLadInfo.dwStatus = NS_DISCONNECTED;
		RemoteGameLadInfo.dwSize = 0;

		hNetworkList = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | LBS_HASSTRINGS | LBS_SORT | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hNetworkList, WM_SETFONT, (WPARAM)hFont, false);

		hTextBox1_Static = NULL;
		hTextBox1 = NULL;
		hTextBox2_Static = NULL;
		hTextBox2 = NULL;
		hTextBox3_Static = NULL;
		hTextBox3 = NULL;
		hHost = NULL;
		hSend = NULL;
		hLinkCable = NULL;
		hSendFileWnd = NULL;

		hNetworkStatus = CreateWindowEx(WS_EX_CLIENTEDGE, "STATIC", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hNetworkStatus, WM_SETFONT, (WPARAM)hFont, false);
		hPrevious = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hPrevious, WM_SETFONT, (WPARAM)hFont, false);
		SetWindowLong(hPrevious, GWL_ID, ID_PREVIOUS);
		hNext = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWin, NULL, hInstance, NULL);
		SendMessage(hNext, WM_SETFONT, (WPARAM)hFont, false);
		SetWindowLong(hNext, GWL_ID, ID_NEXT);

		NetworkStatusText = NULL;//IDS_NETWORK_STATUS_SELECTSERVICEPROVIDER;

		InitNetwork();

		PrepareNetworkWindow(hWin);

		return 0;
	}

	return DefMDIChildProc(hWin, uMsg, wParam, lParam);
}

