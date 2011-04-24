#include	<windows.h>

#define		CLIST_CPP

#include	"..\Error.h"
#include	"CList.h"



CList::CList()
{
	Init(NULL, NULL, NULL, NULL);
}



CList::CList(CLIST_CREATECALLBACK CreateCallback, CLIST_DELETECALLBACK DeleteCallback)
{
	Init(CreateCallback, NULL, DeleteCallback, NULL);
}



CList::CList(CLIST_CREATECALLBACK CreateCallback, DWORD dwCreateData, CLIST_DELETECALLBACK DeleteCallback, DWORD dwDeleteData)
{
	Init(CreateCallback, dwCreateData, DeleteCallback, dwDeleteData);
}



void CList::Init(CLIST_CREATECALLBACK CreateCallback, DWORD dwCreateData, CLIST_DELETECALLBACK DeleteCallback, DWORD dwDeleteData)
{
	m_CreateCallback = CreateCallback;
	m_DeleteCallback = DeleteCallback;
	m_dwCreateData = dwCreateData;
	m_dwDeleteData = dwDeleteData;

	m_pFirstItem = NULL;
	m_pCurrentItem = NULL;
	m_dwItems = 0;
	m_dwUserData = 0;
}



CList::~CList()
{
	ITEM		*pNextItem;


	while (m_pFirstItem)
	{
		if (m_DeleteCallback)
		{
			m_DeleteCallback(&m_pFirstItem->Data, m_dwDeleteData);
		}
		pNextItem = m_pFirstItem->Header.pNext;
		delete m_pFirstItem;
		m_pFirstItem = pNextItem;
	}
}



void *CList::NewItem(DWORD dwSize)
{
	return NewItem(dwSize, NULL);
}



void *CList::NewItem(DWORD dwSize, void *pv)
{
	if (m_pFirstItem)
	{
		if (!(m_pFirstItem->Header.pPrevious = (ITEM *)new BYTE[sizeof(ITEM::ITEMHEADER) + dwSize]))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}

		m_pFirstItem->Header.pPrevious->Header.pNext = m_pFirstItem;
		m_pFirstItem = m_pFirstItem->Header.pPrevious;
	}
	else
	{
		if (!(m_pFirstItem = (ITEM *)new BYTE[sizeof(ITEM::ITEMHEADER) + dwSize]))
		{
			FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
			return NULL;
		}

		m_pFirstItem->Header.pNext = NULL;
	}

	m_dwItems++;

	m_pFirstItem->Header.pPrevious = NULL;
	m_pFirstItem->Header.dwSize = dwSize;

	if (pv)
	{
		CopyMemory(&m_pFirstItem->Data, pv, dwSize);
	}
	else
	{
		ZeroMemory(&m_pFirstItem->Data, dwSize);
	}

	if (m_CreateCallback)
	{
		if (m_CreateCallback(&m_pFirstItem->Data, m_dwCreateData))
		{
			DeleteItem(m_pFirstItem->Data);
			return NULL;
		}
	}

	return &m_pFirstItem->Data;
}



DWORD CList::GetMaxItemNo()
{
	return m_dwItems;
}



BOOL CList::DeleteItem(void *pvItem)
{
	ITEM		*pItem;


	pItem = m_pFirstItem;

	do
	{
		if (&pItem->Data == pvItem)
		{
			if (m_DeleteCallback)
			{
				m_DeleteCallback(pvItem, m_dwDeleteData);
			}
			if (pItem->Header.pPrevious)
			{
				pItem->Header.pPrevious->Header.pNext = pItem->Header.pNext;
			}
			if (pItem->Header.pNext)
			{
				pItem->Header.pNext->Header.pPrevious = pItem->Header.pPrevious;
			}
			m_dwItems--;
			if (pItem == m_pCurrentItem)
			{
				m_pCurrentItem = pItem->Header.pNext;
			}
			delete pItem;
			return false;
		}
	}
	while (pItem = pItem->Header.pNext);

	return true;
}



void CList::ResetSearch()
{
	m_pCurrentItem = m_pFirstItem;
}



void *CList::GetNextItem()
{
	void		*pv;


	if (!m_pCurrentItem)
	{
		return NULL;
	}

	pv = &m_pCurrentItem->Data;

	m_pCurrentItem = m_pCurrentItem->Header.pNext;

	return pv;
}

