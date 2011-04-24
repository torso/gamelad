#ifndef	NETWORK_H
#define	NETWORK_H

#ifdef	NETWORK_CPP
#define	EQUALNULL		= NULL
#else
#define	NETWORK_CPP		extern
#define	EQUALNULL
#endif


NETWORK_CPP	LRESULT CALLBACK	NetworkWndProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam);
NETWORK_CPP	DWORD				InitNetworkLinkCable();
NETWORK_CPP	void				CloseNetworkLinkCable(DWORD dwLinkCableNo);
NETWORK_CPP	BOOL				SyncNetworkLinkCable(DWORD dwLinkCableNo, DWORD dwTicks, BYTE SerialBit, BYTE FF01, BYTE FF02, DWORD *pdwTicks, BYTE *pSerialBit, BYTE *pFF01, BYTE *pFF02);



enum
{
	NLC_FALSE = 0,				//No link cable
	NLC_WAITING,				//Waiting for remote player to start emulation
	NLC_REMOTEWAITING,			//Remote player waiting for local player to start emulation
	NLC_INITIALIZING,			//Both players have started emulation, but it hasn't started yet
	NLC_TRUE					//Emulation in progress
};

NETWORK_CPP	DWORD				NetworkLinkCable EQUALNULL;

NETWORK_CPP	GUID				ServiceProviderGuid;
NETWORK_CPP	char				szPlayerName[0x20];



#endif	//NETWORK_H

