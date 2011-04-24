extern void				*OpCodes[], *DebugOpCodes[];
extern void __fastcall	LD_mem8(CGameBoy *pGameBoy, DWORD Addr);
extern void __fastcall	Debug_LD_mem8(CGameBoy *pGameBoy, DWORD Addr);
extern DWORD __fastcall	ReadMem(CGameBoy *pGameBoy, DWORD Addr);
extern BYTE __fastcall	RetrieveAccess(CGameBoy *pGameBoy, DWORD Addr);
extern void __fastcall	SetAccess(CGameBoy *pGameBoy, DWORD Addr, BYTE Access);
extern BOOL __fastcall	CheckWriteAccessWord(CGameBoy *pGameBoy, DWORD Addr);
extern void __fastcall	SetRTCReg(CGameBoy *pGameBoy);

