#ifndef		CLIST_H
#define		CLIST_H



typedef		BOOL (CALLBACK *CLIST_CREATECALLBACK)(void *, DWORD);
typedef		void (CALLBACK *CLIST_DELETECALLBACK)(void *, DWORD);



struct ITEM
{
	struct ITEMHEADER
	{
		DWORD		dwSize;
		ITEM		*pNext, *pPrevious;
	} Header;
	BYTE		Data[1];
};



class CList
{
private:
	ITEM					*m_pFirstItem, *m_pCurrentItem;
	DWORD					m_dwItems;
	CLIST_CREATECALLBACK	m_CreateCallback;
	CLIST_DELETECALLBACK	m_DeleteCallback;
	DWORD					m_dwCreateData, m_dwDeleteData;

	void					Init(CLIST_CREATECALLBACK CreateCallback, DWORD dwCreateData, CLIST_DELETECALLBACK DeleteCallback, DWORD dwDeleteData);

public:
	DWORD					m_dwUserData;

	CList(CLIST_CREATECALLBACK CreateCallback, CLIST_DELETECALLBACK DeleteCallback);
	CList(CLIST_CREATECALLBACK CreateCallback, DWORD dwCreateData, CLIST_DELETECALLBACK DeleteCallback, DWORD dwDeleteData);
	CList();
	~CList();

	DWORD					GetMaxItemNo();
	BOOL					DeleteItem(void *pvItem);

	void					*NewItem(DWORD dwSize);
	void					*NewItem(DWORD dwSize, const void *pv);

	void					ResetSearch();
	void					*GetNextItem();
};



#endif		//CLIST_H

