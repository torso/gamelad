#include	<windows.h>

#define		CLIST_CPP

#include	"..\Error.h"
#include	"CList.h"



CList::CList()
{
	CList(NULL, NULL);
}



CList::CList(CLIST_CREATECALLBACK _CreateCallback, CLIST_DELETECALLBACK _DeleteCallback)
{
	CreateCallback = _CreateCallback;
	DeleteCallback = _DeleteCallback;

	pList = NULL;
	pFirstList = NULL;
	pCurrentItem = NULL;
	MaxItemNo = 0;
	dwUserData = 0;
}



CList::~CList()
{
	DWORD	CurrentItem;


	while (pFirstList)
	{
		for (CurrentItem = 0; CurrentItem < CLIST_LISTSIZE; CurrentItem++)
		{
			if (pFirstList->le[CurrentItem].pv)
			{
				if (DeleteCallback)
				{
					DeleteCallback(pFirstList->le[CurrentItem].pv);
				}
				else
				{
					delete pFirstList->le[CurrentItem].pv;
				}
			}
		}
		pFirstList = pFirstList->pNext;
	}
}



void *CList::NewItem(DWORD dwSize)
{
	LIST	*CurrentList = pFirstList, **NewList = &pFirstList;
	DWORD	CurrentItem;


	while (CurrentList)
	{
		for (CurrentItem = 0; CurrentItem < CLIST_LISTSIZE; CurrentItem++)
		{
			if (CurrentList->le[CurrentItem].Size == 0)
			{
				if (CreateCallback)
				{
					if (CreateCallback(&CurrentList->le[CurrentItem].pv))
					{
						return NULL;
					}
				}
				else
				{
					if (!(CurrentList->le[CurrentItem].pv = new BYTE[dwSize]))
					{
						FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
						return NULL;
					}
					ZeroMemory(CurrentList->le[CurrentItem].pv, dwSize);
				}
				CurrentList->le[CurrentItem].Size = dwSize;
				CurrentList->le[CurrentItem].Number = ++MaxItemNo;
				CurrentList->le[CurrentItem].pvPrevious = pCurrentItem;
				pCurrentItem = &CurrentList->le[CurrentItem];
				return pCurrentItem->pv;
			}
		}
		if (!CurrentList->pNext)
		{
			NewList = &CurrentList->pNext;
		}
		CurrentList = CurrentList->pNext;
	}

	if (!(*NewList = new LIST))
	{
		FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
		return NULL;
	}
	ZeroMemory(*NewList, sizeof(**NewList));
	if (CreateCallback)
	{
		if (CreateCallback(&(*NewList)->le[0].pv))
		{
			delete *NewList;
			*NewList = NULL;
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}
	}
	else
	{
		if (!((*NewList)->le[0].pv = new BYTE[dwSize]))
		{
			delete *NewList;
			*NewList = NULL;
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}
		ZeroMemory((*NewList)->le[0].pv, dwSize);
	}
	(*NewList)->le[0].Size = dwSize;
	(*NewList)->le[0].Number = ++MaxItemNo;
	(*NewList)->le[0].pvPrevious = pCurrentItem;
	pCurrentItem = &(*NewList)->le[0];

	return pCurrentItem->pv;
}



void *CList::NewItem(DWORD dwSize, void *pv)
{
	LIST	*CurrentList = pFirstList, **NewList = &pFirstList;
	DWORD	CurrentItem;


	if (!pv)
	{
		return NewItem(dwSize);
	}

	while (CurrentList)
	{
		for (CurrentItem = 0; CurrentItem < CLIST_LISTSIZE; CurrentItem++)
		{
			if (CurrentList->le[CurrentItem].Size == 0)
			{
				if (CreateCallback)
				{
					if (CreateCallback(&CurrentList->le[CurrentItem].pv))
					{
						return NULL;
					}
				}
				else
				{
					if (!(CurrentList->le[CurrentItem].pv = new BYTE[dwSize]))
					{
						FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
						return NULL;
					}
				}
				CopyMemory(CurrentList->le[CurrentItem].pv, pv, dwSize);
				CurrentList->le[CurrentItem].Size = dwSize;
				CurrentList->le[CurrentItem].Number = ++MaxItemNo;
				CurrentList->le[CurrentItem].pvPrevious = pCurrentItem;
				pCurrentItem = &CurrentList->le[CurrentItem];
				return pCurrentItem->pv;
			}
		}
		if (!CurrentList->pNext)
		{
			NewList = &CurrentList->pNext;
		}
		CurrentList = CurrentList->pNext;
	}

	if (!(*NewList = new LIST))
	{
		FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
		return NULL;
	}
	ZeroMemory(*NewList, sizeof(**NewList));
	if (CreateCallback)
	{
		if (CreateCallback(&(*NewList)->le[0].pv))
		{
			delete *NewList;
			*NewList = NULL;
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}
	}
	else
	{
		if (!((*NewList)->le[0].pv = new BYTE[dwSize]))
		{
			delete *NewList;
			*NewList = NULL;
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}
	}
	CopyMemory((*NewList)->le[0].pv, pv, dwSize);
	(*NewList)->le[0].Size = dwSize;
	(*NewList)->le[0].Number = ++MaxItemNo;
	(*NewList)->le[0].pvPrevious = pCurrentItem;
	pCurrentItem = &(*NewList)->le[0];

	return pCurrentItem->pv;
}



void CList::PreviousItemToCurrent()
{
	if (pCurrentItem)
	{
		pCurrentItem = GetListNo(pCurrentItem->Number);
	}
}



LISTELEMENT *CList::GetListNo(DWORD dwNumber)
{
	LIST	*CurrentList = pFirstList;


	if (dwNumber-- == 0)
	{
		return NULL;
	}

	while (dwNumber > 99)
	{
		CurrentList = CurrentList->pNext;
		if (!CurrentList)
		{
			return NULL;
		}
		dwNumber -= 100;
	}

	return &CurrentList->le[dwNumber];
}



void *CList::GetCurrentItem()
{
	if (!pCurrentItem)
	{
		return NULL;
	}

	return pCurrentItem->pv;
}



DWORD CList::GetCurrentItemNo()
{
	if (!pCurrentItem)
	{
		return 0;
	}

	return pCurrentItem->Number;
}



DWORD CList::GetMaxItemNo()
{
	return MaxItemNo;
}



void *CList::GetItem(DWORD dwNumber)
{
	LISTELEMENT		*pListElement;


	pListElement = GetListNo(dwNumber);
	if (pListElement)
	{
		return pListElement->pv;
	}

	return NULL;
}



DWORD CList::GetSizeofItem(DWORD dwNumber)
{
	LISTELEMENT		*pListElement;


	pListElement = GetListNo(dwNumber);
	if (pListElement)
	{
		return pListElement->Size;
	}

	return 0;
}

