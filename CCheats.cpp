#include	<windows.h>
#include	<commctrl.h>

#define		CCHEATS_CPP
#include	"Game Lad.h"
#include	"Game Boy.h"
#include	"CGameBoys.h"
#include	"CCheats.h"
#include	"resource.h"



void CALLBACK DeleteGame(void *p, DWORD dw)
{
	delete ((GAMEINFO *)p)->pCheats;
}



void CALLBACK DeleteCheat(void *p, DWORD dw)
{
	delete ((CHEAT *)p)->pCodes;
}



CCheats::CCheats()
{
	m_pGames = NULL;
	m_pGameBoy = NULL;
}



CCheats::~CCheats()
{
	if (m_pGames)
	{
		delete m_pGames;
	}
}



void CCheats::SetGameBoy()
{
	if (m_pGameBoy)
	{
		m_pGameBoy->Release();
	}

	m_pGameBoy = GameBoys.GetActive(true);
}



void CCheats::ReleaseGameBoy()
{
	if (m_pGameBoy)
	{
		m_pGameBoy->Release();
		m_pGameBoy = NULL;
	}
}



BOOL ReadUntil(HANDLE hFile, char cEnd)
{
	char		c;
	DWORD		nBytes;


	do
	{
		if (!ReadFile(hFile, &c, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			return true;
		}
		if (nBytes != 1)
		{
			CloseHandle(hFile);
			return true;
		}
	}
	while (c != cEnd);

	return false;
}



BOOL ReadString(HANDLE hFile, char *pszBuffer, DWORD dwSize, char cEnd)
{
	char		c;
	DWORD		nBytes, dwPos;


	pszBuffer[0] = '\0';
	dwPos = 0;

	do
	{
		if (!ReadFile(hFile, &c, 1, &nBytes, NULL))
		{
			CloseHandle(hFile);
			return true;
		}
		if (nBytes != 1)
		{
			CloseHandle(hFile);
			return true;
		}
		if (c == cEnd)
		{
			return false;
		}
		pszBuffer[dwPos] = c;
		pszBuffer[++dwPos] = '\0';
	}
	while (dwPos + 1 < dwSize);

	switch (cEnd)
	{
	case '<':
		ReadUntil(hFile, '>');
		break;
	case '>':
		ReadUntil(hFile, '<');
		break;
	}
	return false;
}



BOOL CCheats::MergeFile(char *pszFilename)
{
	GAMEINFO		GameInfo, *pGameInfo;
	CHEAT			Cheat, *pCheat1, *pCheat2;
	HANDLE			hFile;
	char			szBuffer[0x100], *pszCode1, *pszCode2;
	BOOL			AlreadyExists;


	if (!m_pGames)
	{
		if (!(m_pGames = new CList(NULL, DeleteGame)))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
	}

	if ((hFile = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
		return true;
	}

	do
	{
		if (ReadUntil(hFile, '<'))
		{
			Save();
			return true;
		}
		if (ReadString(hFile, szBuffer, sizeof(szBuffer), '>'))
		{
			Save();
			return true;
		}
	}
	while (strcmp(szBuffer, "cheatlist"));

	do
	{
		if (ReadUntil(hFile, '<'))
		{
			Save();
			return true;
		}
		if (ReadString(hFile, szBuffer, sizeof(szBuffer), '>'))
		{
			Save();
			return true;
		}
		if (!strcmp(szBuffer, "game"))
		{
			GameInfo.szTitle[0] = '\0';
			GameInfo.szName[0] = '\0';
			GameInfo.Flags = 0;
			GameInfo.Checksum = 0;
			if (!(GameInfo.pCheats = new CList(NULL, DeleteCheat)))
			{
				CloseHandle(hFile);
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				Save();
				return true;
			}

			do
			{
				if (ReadUntil(hFile, '<'))
				{
					Save();
					SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
					return true;
				}
				if (ReadString(hFile, szBuffer, sizeof(szBuffer), '>'))
				{
					Save();
					SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
					return true;
				}
				if (!strcmp(szBuffer, "title"))
				{
					ReadString(hFile, GameInfo.szTitle, sizeof(GameInfo.szTitle), '<');
				}
				else if (!strcmp(szBuffer, "name"))
				{
					ReadString(hFile, GameInfo.szName, sizeof(GameInfo.szName), '<');
				}
				else if (!strcmp(szBuffer, "color"))
				{
					ReadString(hFile, szBuffer, sizeof(szBuffer), '<');
					if (szBuffer[0] == '1')
					{
						GameInfo.Flags |= GIF_COLOR;
					}
				}
				else if (!strcmp(szBuffer, "japan"))
				{
					ReadString(hFile, szBuffer, sizeof(szBuffer), '<');
					if (szBuffer[0] == '1')
					{
						GameInfo.Flags |= GIF_JAPAN;
					}
				}
				else if (!strcmp(szBuffer, "checksum"))
				{
					ReadString(hFile, szBuffer, sizeof(szBuffer), '<');
					if (HexToNum(&szBuffer[0]))
					{
						szBuffer[0] = '\0';
					}
					if (HexToNum(&szBuffer[1]))
					{
						szBuffer[1] = '\0';
					}
					if (HexToNum(&szBuffer[2]))
					{
						szBuffer[2] = '\0';
					}
					if (HexToNum(&szBuffer[3]))
					{
						szBuffer[3] = '\0';
					}
					GameInfo.Checksum = (szBuffer[0] << 12) | (szBuffer[1] << 8) | (szBuffer[2] << 4) | szBuffer[3];
				}
				else if (!strcmp(szBuffer, "cheat"))
				{
					if (!(Cheat.pCodes = new CList()))
					{
						CloseHandle(hFile);
						delete GameInfo.pCheats;
						DisplayErrorMessage(ERROR_OUTOFMEMORY);
						Save();
						SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
						return true;
					}
					do
					{
						if (ReadUntil(hFile, '<'))
						{
							delete GameInfo.pCheats;
							delete Cheat.pCodes;
							Save();
							SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
							return true;
						}
						if (ReadString(hFile, szBuffer, sizeof(szBuffer), '>'))
						{
							delete GameInfo.pCheats;
							delete Cheat.pCodes;
							Save();
							SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
							return true;
						}
						if (!strcmp(szBuffer, "code"))
						{
							ReadString(hFile, szBuffer, sizeof(szBuffer), '<');
							if (!Cheat.pCodes->NewItem(strlen(szBuffer) + 1, szBuffer))
							{
								CloseHandle(hFile);
								delete GameInfo.pCheats;
								delete Cheat.pCodes;
								Save();
								SetStatus(LoadString(IDS_STATUS_READERROR, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);
								return true;
							}
						}
						else if (!strcmp(szBuffer, "comment"))
						{
							ReadString(hFile, Cheat.szComment, sizeof(Cheat.szComment), '<');
						}
					}
					while (strcmp(szBuffer, "/cheat"));
					if (!Cheat.pCodes->GetMaxItemNo())
					{
						delete Cheat.pCodes;
						Cheat.pCodes = NULL;
					}
					if (!Cheat.szComment[0])
					{
						strcpy(Cheat.szComment, "No comment");
					}
					if (!GameInfo.pCheats->NewItem(sizeof(Cheat), &Cheat))
					{
						CloseHandle(hFile);
						delete GameInfo.pCheats;
						delete Cheat.pCodes;
						Save();
						return true;
					}
				}
			}
			while (strcmp(szBuffer, "/game"));

			if (!GameInfo.pCheats->GetMaxItemNo())
			{
				delete GameInfo.pCheats;
				GameInfo.pCheats = NULL;
			}

			if (!GameInfo.szName[0])
			{
				if (GameInfo.szTitle[0])
				{
					strcpy(GameInfo.szName, GameInfo.szTitle);
				}
				else
				{
					strcpy(GameInfo.szName, "Untitled");
				}
			}

			AlreadyExists = false;
			m_pGames->ResetSearch();
			while (pGameInfo = (GAMEINFO *)m_pGames->GetNextItem())
			{
				if (pGameInfo->Checksum == GameInfo.Checksum && pGameInfo->Flags == GameInfo.Flags && !strcmp(pGameInfo->szTitle, GameInfo.szTitle))
				{
					AlreadyExists = true;
					break;
				}
			}

			if (!AlreadyExists)
			{
				if (!m_pGames->NewItem(sizeof(GameInfo), &GameInfo))
				{
					CloseHandle(hFile);
					delete GameInfo.pCheats;
					Save();
					return true;
				}
			}
			else
			{
				if (!strcmp(pGameInfo->szTitle, pGameInfo->szName) || (!pGameInfo->szTitle[0] && !strcmp(pGameInfo->szName, "Untitled")))
				{
					strcpy(pGameInfo->szName, GameInfo.szName);
				}
				if (GameInfo.pCheats)
				{
					if (!pGameInfo->pCheats)
					{
						pGameInfo->pCheats = GameInfo.pCheats;
					}
					else
					{
						GameInfo.pCheats->ResetSearch();
						while (pCheat1 = (CHEAT *)GameInfo.pCheats->GetNextItem())
						{
							AlreadyExists = false;
							pGameInfo->pCheats->ResetSearch();
							while (pCheat2 = (CHEAT *)pGameInfo->pCheats->GetNextItem())
							{
								if (!stricmp(pCheat1->szComment, pCheat2->szComment))
								{
									AlreadyExists = true;
									break;
								}
							}
							if (!AlreadyExists)
							{
								if (!pGameInfo->pCheats->NewItem(sizeof(*pCheat1), pCheat1))
								{
									CloseHandle(hFile);
									delete GameInfo.pCheats;
									Save();
									return true;
								}
								pCheat1->pCodes = NULL;
							}
							else
							{
								if (pCheat1->pCodes)
								{
									pCheat1->pCodes->ResetSearch();
									while (pszCode1 = (char *)pCheat1->pCodes->GetNextItem())
									{
										AlreadyExists = false;
										pCheat2->pCodes->ResetSearch();
										while (pszCode2 = (char *)pCheat2->pCodes->GetNextItem())
										{
											if (CompareCodes(pszCode1, pszCode2))
											{
												AlreadyExists = true;
												break;
											}
										}
										if (!AlreadyExists)
										{
											if (!pCheat2->pCodes->NewItem(strlen(pszCode1) + 1, pszCode1))
											{
												CloseHandle(hFile);
												delete GameInfo.pCheats;
												Save();
												return true;
											}
										}
									}
								}
							}
						}
						delete GameInfo.pCheats;
					}
				}
			}
		}
	}
	while (strcmp(szBuffer, "/cheatlist"));

	CloseHandle(hFile);

	Save();
	SetStatus(LoadString(IDS_STATUS_LOADED, szBuffer, sizeof(szBuffer), pszFilename), SF_MESSAGE);

	return false;
}



#define		WriteToFile(source)										\
	if (!WriteFile(hFile, source, strlen(source), &nBytes, NULL))	\
	{																\
		CloseHandle(hFile);											\
		return false;												\
	}																\
	if (nBytes != strlen(source))									\
	{																\
		CloseHandle(hFile);											\
		return false;												\
	}

BOOL CCheats::Save()
{
	char		szFilename[MAX_PATH];
	HANDLE		hFile;
	DWORD		nBytes;
	GAMEINFO	*pGameInfo;
	CHEAT		*pCheat;
	char		*pszCode;


	GetModuleFileName(hInstance, szFilename, sizeof(szFilename));
	if (strchr(szFilename, '\\'))
	{
		*strrchr(szFilename, '\\') = '\0';
		strcat(szFilename, "\\Cheats.xml");
	}

	if ((hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	WriteToFile("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
	WriteToFile("<cheatlist>\r\n");

	if (!m_pGames)
	{
		WriteToFile("</cheatlist>\r\n");
		CloseHandle(hFile);
		return true;
	}

	m_pGames->ResetSearch();
	while (pGameInfo = (GAMEINFO *)m_pGames->GetNextItem())
	{
		WriteToFile("\t<game>\r\n\t\t<title>");
		WriteToFile(pGameInfo->szTitle);
		WriteToFile("</title>\r\n\t\t<name>");
		WriteToFile(pGameInfo->szName);
		WriteToFile("</name>\r\n");
		if (pGameInfo->Flags & GIF_COLOR)
		{
			WriteToFile("\t\t<color>1</color>\r\n");
		}
		else
		{
			WriteToFile("\t\t<color>0</color>\r\n");
		}
		if (pGameInfo->Flags & GIF_JAPAN)
		{
			WriteToFile("\t\t<japan>1</japan>\r\n");
		}
		else
		{
			WriteToFile("\t\t<japan>0</japan>\r\n");
		}
		WriteToFile("\t\t<checksum>");
		itoa(pGameInfo->Checksum, NumBuffer, 16);
		NumBuffer[0] = toupper(NumBuffer[0]);
		NumBuffer[1] = toupper(NumBuffer[1]);
		NumBuffer[2] = toupper(NumBuffer[2]);
		NumBuffer[3] = toupper(NumBuffer[3]);
		WriteToFile(NumBuffer);
		WriteToFile("</checksum>\r\n");
		if (pGameInfo->pCheats)
		{
			pGameInfo->pCheats->ResetSearch();
			while (pCheat = (CHEAT *)pGameInfo->pCheats->GetNextItem())
			{
				WriteToFile("\t\t<cheat>\r\n\t\t\t<comment>");
				WriteToFile(pCheat->szComment);
				WriteToFile("</comment>\r\n");
				if (pCheat->pCodes)
				{
					pCheat->pCodes->ResetSearch();
					while (pszCode = (char *)pCheat->pCodes->GetNextItem())
					{
						WriteToFile("\t\t\t<code>");
						WriteToFile(pszCode);
						WriteToFile("</code>\r\n");
					}
				}
				WriteToFile("\t\t</cheat>\r\n");
			}
		}
		WriteToFile("\t</game>\r\n");
	}
	WriteToFile("</cheatlist>\r\n");

	CloseHandle(hFile);

	return true;
}



BOOL CCheats::AddCheat(CGameBoy *pGameBoy, char *pszCheat, char *pszCode)
{
	GAMEINFO		GameInfo, *pGameInfo;
	CHEAT			*pCheat;
	char			*psz;
	DWORD			dwPos;


	if (pszCheat)
	{
		m_pGameBoy = NULL;
		m_pCodes = NULL;

		if (!m_pGames)
		{
			if (!(m_pGames = new CList(NULL, DeleteGame)))
			{
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return true;
			}
		}

		m_pGames->ResetSearch();
		while (pGameInfo = (GAMEINFO *)m_pGames->GetNextItem())
		{
			if (pGameInfo->Checksum == (pGameBoy->MEM_ROM[0x14E] << 8 | pGameBoy->MEM_ROM[0x14F]) && !memcmp(pGameInfo->szTitle, &pGameBoy->MEM_ROM[0x134], strlen(pGameInfo->szTitle))
				&& (((pGameInfo->Flags & GIF_COLOR) && (pGameBoy->MEM_ROM[0x143] & 0x80)) || (!(pGameInfo->Flags & GIF_COLOR) && !(pGameBoy->MEM_ROM[0x143] & 0x80)))
				&& (((pGameInfo->Flags & GIF_JAPAN) && !pGameBoy->MEM_ROM[0x14A]) || (!(pGameInfo->Flags & GIF_JAPAN) && pGameBoy->MEM_ROM[0x14A])))
			{
				m_pGameBoy = pGameBoy;
				break;
			}
		}
		if (!m_pGameBoy)
		{
			m_pGameBoy = pGameBoy;
			CopyMemory(GameInfo.szTitle, &pGameBoy->MEM_ROM[0x134], 16);
			if (pGameBoy->MEM_ROM[0x143] & 0x80)
			{
				GameInfo.szTitle[15] = '\0';
				GameInfo.Flags = GIF_COLOR;
			}
			else
			{
				GameInfo.szTitle[16] = '\0';
				GameInfo.Flags = 0;
			}
			for (dwPos = strlen(GameInfo.szTitle) - 1; GameInfo.szTitle[dwPos] == ' '; dwPos--)
			{
				GameInfo.szTitle[dwPos] = '\0';
				if (dwPos == 0)
				{
					break;
				}
			}
			if (GameInfo.szTitle[0])
			{
				strcpy(GameInfo.szName, GameInfo.szTitle);
			}
			else
			{
				strcpy(GameInfo.szName, "Untitled");
			}
			if (!pGameBoy->MEM_ROM[0x14A])
			{
				GameInfo.Flags |= GIF_JAPAN;
				strcat(GameInfo.szName, " (J)");
			}
			GameInfo.Checksum = (pGameBoy->MEM_ROM[0x14E] << 8) | pGameBoy->MEM_ROM[0x14F];
			GameInfo.pCheats = NULL;
			if (!(pGameInfo = (GAMEINFO *)m_pGames->NewItem(sizeof(GameInfo), &GameInfo)))
			{
				return true;
			}
		}

		if (!pGameInfo->pCheats)
		{
			if (!(pGameInfo->pCheats = new CList(NULL, DeleteCheat)))
			{
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return true;
			}
		}
		pGameInfo->pCheats->ResetSearch();
		while (pCheat = (CHEAT *)pGameInfo->pCheats->GetNextItem())
		{
			if (!stricmp(pCheat->szComment, pszCheat))
			{
				m_pCodes = pCheat->pCodes;
			}
		}

		if (!m_pCodes)
		{
			if (!(pCheat = (CHEAT *)pGameInfo->pCheats->NewItem(sizeof(CHEAT))))
			{
				return true;
			}
			strcpy(pCheat->szComment, pszCheat);
			if (!(m_pCodes = pCheat->pCodes = new CList()))
			{
				return true;
			}
		}
	}

	if (pszCode)
	{
		if (!m_pCodes)
		{
			return true;
		}

		m_pCodes->ResetSearch();
		while (psz = (char *)m_pCodes->GetNextItem())
		{
			if (!stricmp(psz, pszCode))
			{
				return false;
			}
		}

		return m_pCodes->NewItem(strlen(pszCode) + 1, pszCode) ? false : true;
	}

	return false;
}



void CCheats::DisableDuplicateCodes(HTREEITEM hti)
{
	TVITEM			tvi;
	HTREEITEM		hti2;
	char			szBuffer[0x100], szBuffer2[0x100];


	tvi.mask = TVIF_HANDLE | TVIF_TEXT;
	tvi.hItem = hti;
	tvi.pszText = szBuffer;
	tvi.cchTextMax = sizeof(szBuffer);
	TreeView_GetItem(m_hTreeView, &tvi);

	tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.pszText = szBuffer2;

	hti2 = hti;
	while (hti2 = TreeView_GetParent(m_hTreeView, hti2))
	{
		tvi.hItem = hti2;
	}

	tvi.hItem = TreeView_GetChild(m_hTreeView, tvi.hItem);
	do
	{
		hti2 = tvi.hItem;
		if (tvi.hItem = TreeView_GetChild(m_hTreeView, tvi.hItem))
		{
			do
			{
				if (tvi.hItem != hti)
				{
					TreeView_GetItem(m_hTreeView, &tvi);
					if (CompareCodes(szBuffer, szBuffer2))
					{
						tvi.state = INDEXTOSTATEIMAGEMASK(1);
						TreeView_SetItem(m_hTreeView, &tvi);
						UpdateCheckMarks(TreeView_GetParent(m_hTreeView, tvi.hItem));
					}
				}
			}
			while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, tvi.hItem));
		}
	}
	while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, hti2));
}



void CCheats::UpdateCheckMarks(HTREEITEM hti)
{
	TVITEM				tvi;
	BOOL				HasCheckedChildren, HasUncheckedChildren;


	HasCheckedChildren = false;
	HasUncheckedChildren = false;

	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.hItem = TreeView_GetChild(m_hTreeView, hti);
	do
	{
		TreeView_GetItem(m_hTreeView, &tvi);
		if ((tvi.state >> 12) == 3)
		{
			HasCheckedChildren = true;
			if (HasUncheckedChildren)
			{
				break;
			}
		}
		else if ((tvi.state >> 12) == 5)
		{
			HasCheckedChildren = true;
			HasUncheckedChildren = true;
			break;
		}
		else
		{
			HasUncheckedChildren = true;
			if (HasCheckedChildren)
			{
				break;
			}
		}
	}
	while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, tvi.hItem));

	tvi.hItem = hti;
	if (HasCheckedChildren && HasUncheckedChildren)
	{
		tvi.state = INDEXTOSTATEIMAGEMASK(5);
	}
	else if (HasCheckedChildren)
	{
		tvi.state = INDEXTOSTATEIMAGEMASK(3);
	}
	else
	{
		tvi.state = INDEXTOSTATEIMAGEMASK(1);
	}
	TreeView_SetItem(m_hTreeView, &tvi);
}



BOOL CCheats::FillWindow(HWND hTreeView)
{
	GAMEINFO			GameInfo, *pGameInfo;
	CHEAT				*pCheat;
	TVINSERTSTRUCT		tvis;
	HTREEITEM			htiGame, htiCheat;
	DWORD				Pos;
	BOOL				HasCheckedChildren, HasUncheckedChildren;


	m_hTreeView = hTreeView;
	m_hGameItem = NULL;


	if (!m_pGames)
	{
		if (!m_pGameBoy)
		{
			return false;
		}
		if (!(m_pGames = new CList(NULL, DeleteGame)))
		{
			DisplayErrorMessage(ERROR_OUTOFMEMORY);
			return true;
		}
	}

	m_pGames->ResetSearch();
	while (pGameInfo = (GAMEINFO *)m_pGames->GetNextItem())
	{
		tvis.hParent = NULL;
		tvis.hInsertAfter = TVI_SORT;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK(7);
		tvis.item.lParam = (LPARAM)pGameInfo;
		if (pGameInfo->szName[0])
		{
			tvis.item.pszText = pGameInfo->szName;
		}
		else
		{
			tvis.item.pszText = pGameInfo->szTitle;
		}
		htiGame = TreeView_InsertItem(m_hTreeView, &tvis);

		if (m_pGameBoy)
		{
			if (pGameInfo->Checksum == (m_pGameBoy->MEM_ROM[0x14E] << 8 | m_pGameBoy->MEM_ROM[0x14F]) && !memcmp(pGameInfo->szTitle, &m_pGameBoy->MEM_ROM[0x134], strlen(pGameInfo->szTitle))
				&& (((pGameInfo->Flags & GIF_COLOR) && (m_pGameBoy->MEM_ROM[0x143] & 0x80)) || (!(pGameInfo->Flags & GIF_COLOR) && !(m_pGameBoy->MEM_ROM[0x143] & 0x80)))
				&& (((pGameInfo->Flags & GIF_JAPAN) && !m_pGameBoy->MEM_ROM[0x14A]) || (!(pGameInfo->Flags & GIF_JAPAN) && m_pGameBoy->MEM_ROM[0x14A])))
			{
				tvis.item.mask = TVIF_HANDLE | TVIF_STATE;
				tvis.item.hItem = m_hGameItem = htiGame;
				tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
				TreeView_SetItem(m_hTreeView, &tvis.item);
				tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			}
		}

		if (pGameInfo->pCheats)
		{
			pGameInfo->pCheats->ResetSearch();
			while (pCheat = (CHEAT *)pGameInfo->pCheats->GetNextItem())
			{
				tvis.hParent = htiGame;
				tvis.item.pszText = pCheat->szComment;
				tvis.item.lParam = NULL;
				if (htiGame == m_hGameItem)
				{
					tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
				}
				else
				{
					tvis.item.state = INDEXTOSTATEIMAGEMASK(7);
				}
				htiCheat = TreeView_InsertItem(m_hTreeView, &tvis);
				if (pCheat->pCodes)
				{
					HasCheckedChildren = false;
					HasUncheckedChildren = false;
					pCheat->pCodes->ResetSearch();
					while (tvis.item.pszText = (char *)pCheat->pCodes->GetNextItem())
					{
						tvis.hParent = htiCheat;
						if (m_hGameItem == htiGame)
						{
							if (m_pGameBoy->IsApplied(tvis.item.pszText))
							{
								HasCheckedChildren = true;
								tvis.item.state = INDEXTOSTATEIMAGEMASK(3);
							}
							else
							{
								HasUncheckedChildren = true;
								tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
							}
						}
						TreeView_InsertItem(m_hTreeView, &tvis);
					}
					if (HasCheckedChildren)
					{
						tvis.item.mask = TVIF_HANDLE | TVIF_STATE;
						tvis.item.hItem = htiCheat;
						tvis.item.state = INDEXTOSTATEIMAGEMASK(HasUncheckedChildren ? 5 : 3);
						TreeView_SetItem(m_hTreeView, &tvis.item);
						tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
					}
					UpdateCheckMarks(TreeView_GetParent(m_hTreeView, tvis.item.hItem));
				}
			}
		}
	}

	if (m_pGameBoy && !m_hGameItem)
	{
		CopyMemory(GameInfo.szTitle, &m_pGameBoy->MEM_ROM[0x134], 16);
		if (m_pGameBoy->MEM_ROM[0x143] & 0x80)
		{
			GameInfo.szTitle[15] = '\0';
			GameInfo.Flags = GIF_COLOR;
		}
		else
		{
			GameInfo.szTitle[16] = '\0';
			GameInfo.Flags = 0;
		}
		for (Pos = strlen(GameInfo.szTitle) - 1; GameInfo.szTitle[Pos] == ' '; Pos--)
		{
			GameInfo.szTitle[Pos] = '\0';
			if (Pos == 0)
			{
				break;
			}
		}
		if (GameInfo.szTitle[0])
		{
			strcpy(GameInfo.szName, GameInfo.szTitle);
		}
		else
		{
			strcpy(GameInfo.szName, "Untitled");
		}
		if (!m_pGameBoy->MEM_ROM[0x14A])
		{
			GameInfo.Flags |= GIF_JAPAN;
			strcat(GameInfo.szName, " (J)");
		}
		GameInfo.Checksum = (m_pGameBoy->MEM_ROM[0x14E] << 8) | m_pGameBoy->MEM_ROM[0x14F];
		GameInfo.pCheats = NULL;
		if (!(pGameInfo = (GAMEINFO *)m_pGames->NewItem(sizeof(GameInfo), &GameInfo)))
		{
			return true;
		}

		tvis.hParent = NULL;
		tvis.hInsertAfter = TVI_SORT;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
		tvis.item.lParam = (LPARAM)pGameInfo;
		tvis.item.pszText = pGameInfo->szName;
		m_hGameItem = TreeView_InsertItem(m_hTreeView, &tvis);
	}

	if (m_hGameItem)
	{
		TreeView_Select(m_hTreeView, m_hGameItem, TVGN_CARET);
		TreeView_Expand(m_hTreeView, m_hGameItem, TVE_EXPAND);
		TreeView_SelectSetFirstVisible(m_hTreeView, m_hGameItem);
	}
	else
	{
		TreeView_Select(m_hTreeView, TreeView_GetRoot(m_hTreeView), TVGN_CARET);
	}

	return false;
}



BOOL CCheats::UpdateCheats()
{
	GAMEINFO			GameInfo;
	CHEAT				Cheat;
	CList				*pGames;
	TVITEM				tvi;
	char				szBuffer[0x100], *pszCode;
	HTREEITEM			hGameItem, hCheatItem;


	if (m_pGameBoy)
	{
		m_pGameBoy->RemoveCheats();
	}

	if (!TreeView_GetCount(m_hTreeView))
	{
		if (m_pGames)
		{
			delete m_pGames;
			m_pGames = NULL;
			return false;
		}
	}

	if (!(pGames = new CList(NULL, DeleteGame)))
	{
		DisplayErrorMessage(ERROR_OUTOFMEMORY);
		return true;
	}

	tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.pszText = szBuffer;
	tvi.cchTextMax = sizeof(szBuffer);
	tvi.hItem = hGameItem = TreeView_GetRoot(m_hTreeView);
	do
	{
		TreeView_GetItem(m_hTreeView, &tvi);
		strcpy(GameInfo.szTitle, ((GAMEINFO *)tvi.lParam)->szTitle);
		strcpy(GameInfo.szName, szBuffer);
		GameInfo.Flags = ((GAMEINFO *)tvi.lParam)->Flags;
		GameInfo.Checksum = ((GAMEINFO *)tvi.lParam)->Checksum;

		if (tvi.cChildren)
		{
			if (!(GameInfo.pCheats = new CList(NULL, DeleteCheat)))
			{
				delete pGames;
				DisplayErrorMessage(ERROR_OUTOFMEMORY);
				return true;
			}

			tvi.hItem = hCheatItem = TreeView_GetChild(m_hTreeView, hGameItem);
			do
			{
				TreeView_GetItem(m_hTreeView, &tvi);
				strcpy(Cheat.szComment, szBuffer);
				if (tvi.cChildren)
				{
					if (!(Cheat.pCodes = new CList()))
					{
						delete pGames;
						DisplayErrorMessage(ERROR_OUTOFMEMORY);
						return true;
					}
					tvi.hItem = TreeView_GetChild(m_hTreeView, hCheatItem);
					do
					{
						TreeView_GetItem(m_hTreeView, &tvi);
						if (!(pszCode = (char *)Cheat.pCodes->NewItem(strlen(szBuffer) + 1, szBuffer)))
						{
							delete pGames;
							return true;
						}
						if (hGameItem == m_hGameItem)
						{
							if ((tvi.state >> 12) == 3)
							{
								m_pGameBoy->AddCheat(pszCode);
							}
						}
					}
					while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, tvi.hItem));
				}
				else
				{
					Cheat.pCodes = NULL;
				}

				if (!GameInfo.pCheats->NewItem(sizeof(Cheat), &Cheat))
				{
					delete pGames;
					return true;
				}
			}
			while (tvi.hItem = hCheatItem = TreeView_GetNextSibling(m_hTreeView, hCheatItem));
		}
		else
		{
			GameInfo.pCheats = NULL;
		}
		if (!pGames->NewItem(sizeof(GameInfo), &GameInfo))
		{
			delete pGames;
			return true;
		}
	}
	while (tvi.hItem = hGameItem = TreeView_GetNextSibling(m_hTreeView, hGameItem));

	delete m_pGames;
	m_pGames = pGames;


	while (true)
	{
		if (Save())
		{
			//File saved OK
			break;
		}
		else
		{
			switch (MessageBox(hMsgParent, "Error saving Cheat list. Changes will not be saved. Retry?", "Game Lad", MB_OK | MB_YESNOCANCEL))
			{
			case IDNO:
				//Discard changes
				return false;

			case IDCANCEL:
				//Return to cheat dialog
				return true;
			}
		}
	}

	return false;
}



BOOL CCheats::AddCheat(char *pszCheat)
{
	TVINSERTSTRUCT		tvis;
	HTREEITEM			hti;
	char				szBuffer[0x100];


	tvis.hParent = TreeView_GetSelection(m_hTreeView);
	while (hti = TreeView_GetParent(m_hTreeView, tvis.hParent))
	{
		tvis.hParent = hti;
	}

	tvis.hInsertAfter = TVI_SORT;
	tvis.item.mask = TVIF_HANDLE | TVIF_TEXT;

	if (tvis.item.hItem = TreeView_GetChild(m_hTreeView, tvis.hParent))
	{
		tvis.item.pszText = szBuffer;
		tvis.item.cchTextMax = sizeof(szBuffer);
		do
		{
			TreeView_GetItem(m_hTreeView, &tvis.item);
			if (!stricmp(szBuffer, pszCheat))
			{
				return false;
			}
		}
		while (tvis.item.hItem = TreeView_GetNextSibling(m_hTreeView, tvis.item.hItem));
	}

	tvis.item.mask = TVIF_TEXT | TVIF_STATE;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	if (tvis.hParent == m_hGameItem)
	{
		tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
	}
	else
	{
		tvis.item.state = INDEXTOSTATEIMAGEMASK(7);
	}
	tvis.item.pszText = pszCheat;
	hti = TreeView_InsertItem(m_hTreeView, &tvis);
	TreeView_Select(m_hTreeView, hti, TVGN_CARET);

	return false;
}



BOOL CCheats::CanReplaceCheat(HTREEITEM hti, char *pszCheat)
{
	TVITEM			tvi;
	char			szBuffer[0x100];


	tvi.mask = TVIF_HANDLE | TVIF_TEXT;
	tvi.pszText = szBuffer;
	tvi.cchTextMax = sizeof(szBuffer);
	tvi.hItem = TreeView_GetChild(m_hTreeView, TreeView_GetParent(m_hTreeView, hti));
	do
	{
		if (tvi.hItem != hti)
		{
			TreeView_GetItem(m_hTreeView, &tvi);
			if (!stricmp(szBuffer, pszCheat))
			{
				return false;
			}
		}
	}
	while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, tvi.hItem));

	return true;
}



BOOL CCheats::CompareCodes(char *pszCode1, char *pszCode2)
{
	switch (strlen(pszCode1))
	{
	case 7:
	case 11:
		if (strlen(pszCode2) != 7 && strlen(pszCode2) != 11)
		{
			break;
		}
		if (pszCode1[2] == pszCode2[2] && pszCode1[4] == pszCode2[4] && pszCode1[5] == pszCode2[5] && pszCode1[6] == pszCode2[6])
		{
			if (strlen(pszCode1) == 11 && strlen(pszCode2) == 11)
			{
				if ((~((pszCode1[8] << 2) | (pszCode1[10] >> 2) | (pszCode1[10] << 6)) ^ 0x45) != (~((pszCode2[8] << 2) | (pszCode2[10] >> 2) | (pszCode2[10] << 6)) ^ 0x45))
				{
					break;
				}
			}
			return true;
		}
		break;

	case 9:
		if (strlen(pszCode2) != 9)
		{
			break;
		}
		if (pszCode1[0] == pszCode2[0] && pszCode1[1] == pszCode2[1] && pszCode1[5] == pszCode2[5] && pszCode1[6] == pszCode2[6] && pszCode1[7] == pszCode2[7] && pszCode1[8] == pszCode2[8])
		{
			return true;
		}
		break;
	}

	return false;
}



BOOL CCheats::AddCode(char *pszCode)
{
	TVINSERTSTRUCT		tvis;
	HTREEITEM			hti;
	char				szBuffer[0x100];
	char				szCode[10];
	int					Pos, Pos2;


	switch (VerifyCode(pszCode))
	{
	case CODE_GENIE:
	case CODE_INEFFECTIVE:
		for (Pos = Pos2 = 0; pszCode[Pos]; Pos++)
		{
			if (pszCode[Pos] != '-')
			{
				if (Pos2 == 3 || Pos2 == 7)
				{
					szCode[Pos2++] = '-';
				}
				szCode[Pos2++] = toupper(pszCode[Pos]);
			}
		}
		szCode[Pos2] = '\0';
		break;

	case CODE_SHARK:
		for (Pos = Pos2 = 0; pszCode[Pos]; Pos++)
		{
			if (pszCode[Pos] != '-')
			{
				if (Pos2 == 4)
				{
					szCode[Pos2++] = '-';
				}
				szCode[Pos2++] = toupper(pszCode[Pos]);
			}
		}
		szCode[Pos2] = '\0';
		break;

	case CODE_INVALID:
		return true;
	}

	tvis.hParent = TreeView_GetSelection(m_hTreeView);
	if (!(hti = TreeView_GetParent(m_hTreeView, tvis.hParent)))
	{
		return true;
	}
	if (TreeView_GetParent(m_hTreeView, hti))
	{
		tvis.hParent = hti;
	}

	if (tvis.item.hItem = TreeView_GetChild(m_hTreeView, tvis.hParent))
	{
		tvis.item.mask = TVIF_HANDLE | TVIF_TEXT;
		tvis.item.pszText = szBuffer;
		tvis.item.cchTextMax = sizeof(szBuffer);
		do
		{
			TreeView_GetItem(m_hTreeView, &tvis.item);
			if (CompareCodes(tvis.item.pszText, szCode))
			{
				return false;
			}
		}
		while (tvis.item.hItem = TreeView_GetNextSibling(m_hTreeView, tvis.item.hItem));
	}

	tvis.hInsertAfter = TVI_SORT;
	tvis.item.mask = TVIF_TEXT | TVIF_STATE;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	tvis.item.state = INDEXTOSTATEIMAGEMASK(TreeView_GetParent(m_hTreeView, tvis.hParent) == m_hGameItem ? 1 : 7);
	tvis.item.pszText = szCode;
	TreeView_Select(m_hTreeView, TreeView_InsertItem(m_hTreeView, &tvis), TVGN_CARET);

	if (TreeView_GetParent(m_hTreeView, tvis.hParent) == m_hGameItem)
	{
		UpdateCheckMarks(tvis.hParent);
	}

	return false;
}



BOOL CCheats::CanReplaceCode(HTREEITEM hti, char *pszCode, char *pszNewCode)
{
	TVITEM				tvi;
	HTREEITEM			hti2;
	char				szBuffer[0x100];
	int					Pos, Pos2;


	switch (VerifyCode(pszCode))
	{
	case CODE_GENIE:
	case CODE_INEFFECTIVE:
		for (Pos = Pos2 = 0; pszCode[Pos]; Pos++)
		{
			if (pszCode[Pos] != '-')
			{
				if (Pos2 == 3 || Pos2 == 7)
				{
					pszNewCode[Pos2++] = '-';
				}
				pszNewCode[Pos2++] = toupper(pszCode[Pos]);
			}
		}
		pszNewCode[Pos2] = '\0';
		break;

	case CODE_SHARK:
		for (Pos = Pos2 = 0; pszCode[Pos]; Pos++)
		{
			if (pszCode[Pos] != '-')
			{
				if (Pos2 == 4)
				{
					pszNewCode[Pos2++] = '-';
				}
				pszNewCode[Pos2++] = toupper(pszCode[Pos]);
			}
		}
		pszNewCode[Pos2] = '\0';
		break;

	case CODE_INVALID:
		return false;
	}

	hti2 = TreeView_GetParent(m_hTreeView, hti);
	tvi.hItem = TreeView_GetChild(m_hTreeView, hti2);
	do
	{
		if (tvi.hItem != hti)
		{
			tvi.mask = TVIF_HANDLE | TVIF_TEXT;
			tvi.pszText = szBuffer;
			tvi.cchTextMax = sizeof(szBuffer);
			TreeView_GetItem(m_hTreeView, &tvi);
			if (CompareCodes(tvi.pszText, pszNewCode))
			{
				return false;
			}
		}
	}
	while (tvi.hItem = TreeView_GetNextSibling(m_hTreeView, tvi.hItem));

	return true;
}



int CCheats::VerifyCode(char *pszCode)
{
	char		szCode[10];
	int			Pos, Pos2;
	WORD		Offset;


	strcpy(szCode, pszCode);
	for (Pos = Pos2 = 0; szCode[Pos]; Pos++)
	{
		if (szCode[Pos] != '-')
		{
			if (HexToNum(&szCode[Pos]))
			{
				return CODE_INVALID;
			}
			szCode[Pos2++] = szCode[Pos];
		}
	}

	switch (Pos2)
	{
	case 6:
	case 9:
		//CGameBoy::VerifyCode(...) will handle m_pGameBoy being NULL
		return m_pGameBoy->VerifyCode(szCode, Pos2 == 9 ? true : false);

	case 8:
		Offset = (szCode[6] << 12) | (szCode[7] << 8) | (szCode[4] << 4) | szCode[5];
		if (Offset < 0xA000 || Offset >= 0xE000)
		{
			return CODE_INVALID;
		}
		return CODE_SHARK;
	}

	return CODE_INVALID;
}



void CCheats::DeleteItem(HTREEITEM hti)
{
	HTREEITEM			hti2;
	TVITEM				tvi;


	hti2 = TreeView_GetParent(m_hTreeView, hti);
	if (hti != m_hGameItem)
	{
		tvi.mask = TVIF_HANDLE | TVIF_STATE;
		tvi.hItem = hti;
		tvi.stateMask = TVIS_STATEIMAGEMASK;
		TreeView_GetItem(m_hTreeView, &tvi);
		TreeView_DeleteItem(m_hTreeView, hti);
		if ((tvi.state >> 12) == 7)
		{
			return;
		}
	}
	if (hti2)
	{
		UpdateCheckMarks(hti2);
		if (hti2 = TreeView_GetParent(m_hTreeView, hti2))
		{
			UpdateCheckMarks(hti2);
		}
	}
}



WNDPROC		OldTreeViewProc, OldEditBoxProc, OldButtonProc;

LPARAM CALLBACK TreeViewProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HTREEITEM		hti;
	POINT			Point;


	switch (uMsg)
	{
	case WM_COMMAND:
		break;

	case WM_RBUTTONUP:
		//GetCursorPos(&Point);
		Point.x = LOWORD(lParam);
		Point.y = HIWORD(lParam);
		ClientToScreen(hWin, &Point);
		TrackPopupMenu(GetSubMenu(hPopupMenu, 2), TPM_LEFTBUTTON, Point.x, Point.y, 0, hWin, NULL);
		return 0;

	case WM_RBUTTONDOWN:
		return 0;

	case WM_SETFOCUS:
		SetWindowLong(GetDlgItem(GetParent(hWin), IDOK), GWL_USERDATA, true);
		InvalidateRect(GetDlgItem(GetParent(hWin), IDOK), NULL, true);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_DELETE:
			if (hti = TreeView_GetSelection(hWin))
			{
				Cheats.DeleteItem(hti);
				return true;
			}
			break;

		case VK_F2:
			if (hti = TreeView_GetSelection(hWin))
			{
				TreeView_EditLabel(hWin, hti);
				return true;
			}
			break;

		case VK_APPS:
			return 0;
		}
		break;
	}

	return CallWindowProc(OldTreeViewProc, hWin, uMsg, wParam, lParam);
}



LPARAM CALLBACK ButtonProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		SetWindowLong(hWin, GWL_USERDATA, true);
		InvalidateRect(hWin, NULL, true);
		break;

	case WM_KILLFOCUS:
		SetWindowLong(hWin, GWL_USERDATA, false);
		InvalidateRect(hWin, NULL, true);
		break;

	case WM_PAINT:
		if (GetWindowLong(hWin, GWL_USERDATA))
		{
			if (!(GetWindowLong(hWin, GWL_STYLE) & BS_DEFPUSHBUTTON))
			{
				SetWindowLong(hWin, GWL_STYLE, GetWindowLong(hWin, GWL_STYLE) | BS_DEFPUSHBUTTON);
				InvalidateRect(hWin, NULL, true);
			}
		}
		else
		{
			if (GetWindowLong(hWin, GWL_STYLE) & BS_DEFPUSHBUTTON)
			{
				SetWindowLong(hWin, GWL_STYLE, GetWindowLong(hWin, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
				InvalidateRect(hWin, NULL, true);
			}
		}
		break;
	}

	return CallWindowProc(OldButtonProc, hWin, uMsg, wParam, lParam);
}



LPARAM CALLBACK EditBoxProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		switch (GetWindowLong(hWin, GWL_ID))
		{
		case IDC_CHEAT:
			SetWindowLong(GetDlgItem(GetParent(hWin), ID_ADDCHEAT), GWL_USERDATA, true);
			InvalidateRect(GetDlgItem(GetParent(hWin), ID_ADDCHEAT), NULL, true);
			SetWindowLong(GetDlgItem(GetParent(hWin), IDOK), GWL_USERDATA, false);
			InvalidateRect(GetDlgItem(GetParent(hWin), IDOK), NULL, true);
			break;

		case IDC_CODE:
			SetWindowLong(GetDlgItem(GetParent(hWin), ID_ADDCODE), GWL_USERDATA, true);
			InvalidateRect(GetDlgItem(GetParent(hWin), ID_ADDCODE), NULL, true);
			SetWindowLong(GetDlgItem(GetParent(hWin), IDOK), GWL_USERDATA, false);
			InvalidateRect(GetDlgItem(GetParent(hWin), IDOK), NULL, true);
			break;
		}
		break;

	case WM_KILLFOCUS:
		switch (GetWindowLong(hWin, GWL_ID))
		{
		case IDC_CHEAT:
			SetWindowLong(GetDlgItem(GetParent(hWin), ID_ADDCHEAT), GWL_USERDATA, false);
			InvalidateRect(GetDlgItem(GetParent(hWin), ID_ADDCHEAT), NULL, true);
			break;

		case IDC_CODE:
			SetWindowLong(GetDlgItem(GetParent(hWin), ID_ADDCODE), GWL_USERDATA, false);
			InvalidateRect(GetDlgItem(GetParent(hWin), ID_ADDCODE), NULL, true);
			break;
		}
		break;

	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			switch (GetWindowLong(hWin, GWL_ID))
			{
			case IDC_CHEAT:
				SendMessage(GetParent(hWin), WM_COMMAND, ID_ADDCHEAT, NULL);
				SetFocus(GetDlgItem(GetParent(hWin), IDC_CODE));
				return 0;

			case IDC_CODE:
				SendMessage(GetParent(hWin), WM_COMMAND, ID_ADDCODE, NULL);
				return 0;
			}
		}
		break;

	case WM_GETDLGCODE:
		if (lParam)
		{
			if (((MSG *)lParam)->message == WM_KEYDOWN && ((MSG *)lParam)->wParam == VK_TAB)
			{
				break;
			}
		}
		return DLGC_WANTALLKEYS;
	}

	return CallWindowProc(OldEditBoxProc, hWin, uMsg, wParam, lParam);
}



BOOL CALLBACK CheatDialogProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT			rct;
	long			width, height;
	char			szBuffer[0x100];
	HIMAGELIST		himl;
	HBITMAP			hbmp, hOldBmp;
	HDC				hdc, hdc2;
	TVITEM			tvi;
	HTREEITEM		hti;


	switch (uMsg)
	{
	case WM_COMMAND:
		if (lParam && (HWND)lParam == (HWND)SendMessage(GetDlgItem(hWin, IDC_TREE), TVM_GETEDITCONTROL, 0, 0))
		{
			break;
		}
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (Cheats.UpdateCheats())
			{
				break;
			}
			EndDialog(hWin, 0);
			return true;

		case IDCANCEL:
			EndDialog(hWin, 0);
			return true;

		case ID_ADDCHEAT:
			if (GetWindowText(GetDlgItem(hWin, IDC_CHEAT), szBuffer, sizeof(szBuffer)))
			{
				Cheats.AddCheat(szBuffer);
			}
			return true;

		case ID_ADDCODE:
			if (GetWindowText(GetDlgItem(hWin, IDC_CODE), szBuffer, sizeof(szBuffer)))
			{
				Cheats.AddCode(szBuffer);
			}
			return true;

		case IDC_CODE:
			if (HIWORD(wParam) == EN_UPDATE)
			{
				SendDlgItemMessage(hWin, IDC_CODE, WM_GETTEXT, sizeof(szBuffer), (LPARAM)&szBuffer);
				switch (Cheats.VerifyCode(szBuffer))
				{
				case CODE_INVALID:
					SetWindowText(GetDlgItem(hWin, IDC_MESSAGE), String(IDS_CHEAT_INVALID));
					return true;
				case CODE_GENIE:
					SetWindowText(GetDlgItem(hWin, IDC_MESSAGE), String(IDS_CHEAT_GAMEGENIE));
					//EnableWindow(GetDlgItem(hWin, ID_INSERTCODE), true);
					return true;
				case CODE_SHARK:
					SetWindowText(GetDlgItem(hWin, IDC_MESSAGE), String(IDS_CHEAT_GAMESHARK));
					return true;
				case CODE_INEFFECTIVE:
					SetWindowText(GetDlgItem(hWin, IDC_MESSAGE), String(IDS_CHEAT_INEFFECTIVE));
					//EnableWindow(GetDlgItem(hWin, ID_INSERTCODE), true);
					return true;
				}
				SendDlgItemMessage(hWin, IDC_MESSAGE, WM_SETTEXT, 0, NULL);
				//EnableWindow(GetDlgItem(hWin, ID_INSERTCODE), false);
				return true;
			}
			break;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->idFrom == IDC_TREE)
		{
			switch (((NMHDR *)lParam)->code)
			{
			case NM_CUSTOMDRAW:
				SetWindowLong(hWin, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				if (((NMTVCUSTOMDRAW *)lParam)->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
				{
					tvi.mask = TVIF_HANDLE | TVIF_STATE | TVIF_CHILDREN;
					tvi.stateMask = TVIS_STATEIMAGEMASK;
					tvi.hItem = (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec;
					TreeView_GetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
					tvi.mask = TVIF_HANDLE | TVIF_STATE;
					switch (tvi.state >> 12)
					{
					case 2:
						if (((NMTVCUSTOMDRAW *)lParam)->iLevel != 2 && !tvi.cChildren)
						{
							tvi.state = INDEXTOSTATEIMAGEMASK(1);
							TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
							return true;
						}
						tvi.state = INDEXTOSTATEIMAGEMASK(3);
						TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
						switch (((NMTVCUSTOMDRAW *)lParam)->iLevel)
						{
						case 0:
							tvi.hItem = TreeView_GetChild(((NMHDR *)lParam)->hwndFrom, tvi.hItem);
							do
							{
								tvi.mask = TVIF_HANDLE | TVIF_STATE | TVIF_CHILDREN;
								TreeView_GetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
								tvi.mask = TVIF_HANDLE | TVIF_STATE;
								if (tvi.cChildren)
								{
									tvi.state = INDEXTOSTATEIMAGEMASK(3);
								}
								else
								{
									tvi.state = INDEXTOSTATEIMAGEMASK(1);
								}
								TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
								hti = tvi.hItem;
								if (tvi.hItem = TreeView_GetChild(((NMHDR *)lParam)->hwndFrom, tvi.hItem))
								{
									do
									{
										Cheats.DisableDuplicateCodes(tvi.hItem);
										TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
									}
									while (tvi.hItem = TreeView_GetNextSibling(((NMHDR *)lParam)->hwndFrom, tvi.hItem));
								}
								Cheats.UpdateCheckMarks(hti);
							}
							while (tvi.hItem = TreeView_GetNextSibling(((NMHDR *)lParam)->hwndFrom, hti));
							Cheats.UpdateCheckMarks((HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec);
							break;

						case 1:
							tvi.hItem = TreeView_GetChild(((NMHDR *)lParam)->hwndFrom, tvi.hItem);
							do
							{
								Cheats.DisableDuplicateCodes(tvi.hItem);
								TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
							}
							while (tvi.hItem = TreeView_GetNextSibling(((NMHDR *)lParam)->hwndFrom, tvi.hItem));
							Cheats.UpdateCheckMarks((HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec);
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec));
							break;

						case 2:
							Cheats.DisableDuplicateCodes((HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec);
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec));
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec)));
							break;
						}
						return true;

					case 4:
					case 6:
						tvi.state = INDEXTOSTATEIMAGEMASK(1);
						TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
						if (tvi.hItem = TreeView_GetChild(((NMHDR *)lParam)->hwndFrom, tvi.hItem))
						{
							do
							{
								TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
								hti = tvi.hItem;
								if (tvi.hItem = TreeView_GetChild(((NMHDR *)lParam)->hwndFrom, tvi.hItem))
								{
									do
									{
										TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
									}
									while (tvi.hItem = TreeView_GetNextSibling(((NMHDR *)lParam)->hwndFrom, tvi.hItem));
								}
							}
							while (tvi.hItem = TreeView_GetNextSibling(((NMHDR *)lParam)->hwndFrom, hti));
						}
						if (((NMTVCUSTOMDRAW *)lParam)->iLevel == 2)
						{
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec));
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec)));
						}
						else if (((NMTVCUSTOMDRAW *)lParam)->iLevel == 1)
						{
							Cheats.UpdateCheckMarks(TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, (HTREEITEM)((NMTVCUSTOMDRAW *)lParam)->nmcd.dwItemSpec));
						}
						return true;

					case 8:
						tvi.state = INDEXTOSTATEIMAGEMASK(7);
						TreeView_SetItem(((NMHDR *)lParam)->hwndFrom, &tvi);
						return true;
					}
				}
				return true;

			case TVN_BEGINLABELEDIT:
				SendMessage((HWND)SendMessage(((NMHDR *)lParam)->hwndFrom, TVM_GETEDITCONTROL, 0, 0), EM_SETLIMITTEXT, 255, 0);
				SetWindowLong((HWND)SendMessage(((NMHDR *)lParam)->hwndFrom, TVM_GETEDITCONTROL, 0, 0), GWL_WNDPROC, (long)EditBoxProc);
				SetWindowLong(hWin, DWL_MSGRESULT, 0);
				return true;

			case TVN_ENDLABELEDIT:
				if (((NMTVDISPINFO *)lParam)->item.pszText)
				{
					if (hti = TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, ((NMTVDISPINFO *)lParam)->item.hItem))
					{
						if (TreeView_GetParent(((NMHDR *)lParam)->hwndFrom, hti))
						{
							if (Cheats.CanReplaceCode(((NMTVDISPINFO *)lParam)->item.hItem, ((NMTVDISPINFO *)lParam)->item.pszText, szBuffer))
							{
								((NMTVDISPINFO *)lParam)->item.pszText = szBuffer;
								SetWindowLong(hWin, DWL_MSGRESULT, true);
							}
							else
							{
								SetWindowLong(hWin, DWL_MSGRESULT, false);
							}
						}
						else
						{
							if (Cheats.CanReplaceCheat(((NMTVDISPINFO *)lParam)->item.hItem, ((NMTVDISPINFO *)lParam)->item.pszText))
							{
								SetWindowLong(hWin, DWL_MSGRESULT, true);
							}
							else
							{
								SetWindowLong(hWin, DWL_MSGRESULT, false);
							}
						}
					}
					else
					{
						SetWindowLong(hWin, DWL_MSGRESULT, true);
					}
				}
				return true;
			}
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWin, 0);
		return true;

	case WM_INITDIALOG:
		//Center dialog
		GetWindowRect(hWin, &rct);
		width = rct.right - rct.left;
		height = rct.bottom - rct.top;
		GetWindowRect(hWnd, &rct);
		rct.right -= rct.left; //width
		rct.bottom -= rct.top; //height
		MoveWindow(hWin, rct.left + ((rct.right - width) >> 1), rct.top + ((rct.bottom - height) >> 1), width, height, true);

		SetWindowText(hWin, String(IDS_CHEAT_TITLE));
		SetWindowText(GetDlgItem(hWin, ID_ADDCHEAT), String(IDS_CHEAT_ADDCHEAT));
		SetWindowText(GetDlgItem(hWin, ID_ADDCODE), String(IDS_CHEAT_ADDCODE));

		OldTreeViewProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDC_TREE), GWL_WNDPROC, (long)TreeViewProc);

		if (!(himl = ImageList_Create(13, 13, false, 9, 0)))
		{
			return -1;
		}
		if (!(hdc = CreateCompatibleDC(NULL)))
		{
			return -1;
		}
		hdc2 = GetDC(hWnd);
		if (!(hbmp = CreateCompatibleBitmap(hdc2, 13, 13)))
		{
			return -1;
		}
		ReleaseDC(hWnd, hdc2);
		hOldBmp = (HBITMAP)SelectObject(hdc, hbmp);
		rct.left = 0;
		rct.right = 13;
		rct.top = 0;
		rct.bottom = 13;
		DrawFrameControl(hdc, &rct, DFC_BUTTON, DFCS_BUTTONCHECK);
		SelectObject(hdc, hOldBmp);
		ImageList_Add(himl, hbmp, NULL);
		ImageList_Add(himl, hbmp, NULL);
		ImageList_Add(himl, hbmp, NULL);
		hOldBmp = (HBITMAP)SelectObject(hdc, hbmp);
		DrawFrameControl(hdc, &rct, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED);
		SelectObject(hdc, hOldBmp);
		ImageList_Add(himl, hbmp, NULL);
		ImageList_Add(himl, hbmp, NULL);
		hOldBmp = (HBITMAP)SelectObject(hdc, hbmp);
		DrawFrameControl(hdc, &rct, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE);
		SelectObject(hdc, hOldBmp);
		ImageList_Add(himl, hbmp, NULL);
		ImageList_Add(himl, hbmp, NULL);
		hOldBmp = (HBITMAP)SelectObject(hdc, hbmp);
		DrawFrameControl(hdc, &rct, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_INACTIVE);
		SelectObject(hdc, hOldBmp);
		ImageList_Add(himl, hbmp, NULL);
		ImageList_Add(himl, hbmp, NULL);
		DeleteDC(hdc);
		DeleteObject(hbmp);
		TreeView_SetImageList(GetDlgItem(hWin, IDC_TREE), himl, TVSIL_STATE);

		if (Cheats.FillWindow(GetDlgItem(hWin, IDC_TREE)))
		{
			PostMessage(hWin, WM_CLOSE, 0, 0);
			return true;
		}

		if (!TreeView_GetCount(GetDlgItem(hWin, IDC_TREE)))
		{
			EnableWindow(GetDlgItem(hWin, IDC_TREE), false);
			EnableWindow(GetDlgItem(hWin, IDC_CHEAT), false);
			EnableWindow(GetDlgItem(hWin, IDC_CODE), false);
			EnableWindow(GetDlgItem(hWin, ID_ADDCHEAT), false);
			EnableWindow(GetDlgItem(hWin, ID_ADDCODE), false);
			SetWindowLong(GetDlgItem(hWin, IDOK), GWL_USERDATA, true);
			SetWindowLong(GetDlgItem(hWin, ID_ADDCHEAT), GWL_USERDATA, false);
			SetWindowLong(GetDlgItem(hWin, ID_ADDCODE), GWL_USERDATA, false);
		}
		else
		{
			SetWindowLong(GetDlgItem(hWin, IDOK), GWL_USERDATA, false);
			SetWindowLong(GetDlgItem(hWin, ID_ADDCHEAT), GWL_USERDATA, true);
			SetWindowLong(GetDlgItem(hWin, ID_ADDCODE), GWL_USERDATA, false);
		}

		OldButtonProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDOK), GWL_WNDPROC, (long)ButtonProc);
		SetWindowLong(GetDlgItem(hWin, ID_ADDCHEAT), GWL_WNDPROC, (long)ButtonProc);
		SetWindowLong(GetDlgItem(hWin, ID_ADDCODE), GWL_WNDPROC, (long)ButtonProc);

		OldEditBoxProc = (WNDPROC)SetWindowLong(GetDlgItem(hWin, IDC_CHEAT), GWL_WNDPROC, (long)EditBoxProc);
		SetWindowLong(GetDlgItem(hWin, IDC_CODE), GWL_WNDPROC, (long)EditBoxProc);

		hMsgParent = hWin;

		return true;
	}

	return false;
}



BOOL CCheats::ShowCheatDialog()
{
	Cheats.SetGameBoy();

	if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_CHEAT), hWnd, CheatDialogProc) == -1)
	{
		hMsgParent = hWnd;
		Cheats.ReleaseGameBoy();
		DisplayErrorMessage();
		return true;
	}

	hMsgParent = hWnd;

	Cheats.ReleaseGameBoy();

	return false;
}

