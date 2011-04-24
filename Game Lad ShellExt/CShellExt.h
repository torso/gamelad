#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))



#define		RHF_BADCHECKSUM					0x01
#define		RHF_BADCOMPLEMENTCHECK			0x02
#define		RHF_BADNINTENDOCHARACTERAREA	0x04



class CShellExt : public	IShellExtInit,
							IPersistFile,
							IShellPropSheetExt
#ifdef CONTEXTMENUHANDLER
							, IContextMenu
#endif //CONTEXTMENUHANDLER
#ifdef ICONHANDLER
							, IExtractIcon
#endif //ICONHANDLER
#ifdef COPYHOOK
							, ICopyHook
#endif //COPYHOOK
{
private:
	ULONG					m_cRef;
	LPDATAOBJECT			m_pDataObj;
	//char					m_szPropSheetFilename[MAX_PATH];

	//STDMETHODIMP			DoGAKMenu1(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd);
	//STDMETHODIMP			DoGAKMenu2(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd);
	//STDMETHODIMP			DoGAKMenu3(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd);
	//STDMETHODIMP			DoGAKMenu4(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd);

public:
	char					m_szFilename[MAX_PATH];
	BYTE					m_RomHeader[0x50];
	DWORD					m_RomHeaderFlags;
	BYTE					m_ComplementCheck;
	WORD					m_CheckSum;

public:
	CShellExt();
	~CShellExt();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID iid, void **ppvObject);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IShellExtInit methods
	STDMETHODIMP			Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hKeyProgID);

	//IPersistFile methods
	STDMETHODIMP			GetClassID(CLSID *lpClassID);
	STDMETHODIMP			IsDirty();
	STDMETHODIMP			Load(LPCOLESTR lpszFileName, DWORD dwMode);
	STDMETHODIMP			Save(LPCOLESTR lpszFileName, BOOL fRemember);
	STDMETHODIMP			SaveCompleted(LPCOLESTR lpszFileName);
	STDMETHODIMP			GetCurFile(LPOLESTR *ppszFileName);

	//IShellPropSheetExt methods
	STDMETHODIMP			AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
	STDMETHODIMP			ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam);

#ifdef CONTEXTMENUHANDLER
	//IContextMenu members
	STDMETHODIMP			QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP			InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
	STDMETHODIMP			GetCommandString(UINT idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);
#endif //CONTEXTMENUHANDLER

#ifdef ICONHANDLER
	//IExtractIcon methods
	STDMETHODIMP			GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, LPINT piIndex, UINT *pwFlags);
	STDMETHODIMP			Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
#endif //ICONHANDLER

#ifdef COPYHOOKHANDLER
	//ICopyHook method
	STDMETHODIMP_(UINT)		CopyCallback(HWND hWin, UINT wFunc, UINT wFlags, LPCSTR pszSrcFile, DWORD dwSrcAttribs, LPCSTR pszDestFile, DWORD dwDestAttribs);
#endif //COPYHOOKHANDLER
};

