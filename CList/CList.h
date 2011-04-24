#ifndef		CLIST_H
#define		CLIST_H



typedef		BOOL (CALLBACK *CLIST_CREATECALLBACK)(void **);
typedef		void (CALLBACK *CLIST_DELETECALLBACK)(void *);

#define		CLIST_LISTSIZE		0x100



struct LISTELEMENT
{
	void		*pv;
	DWORD		Size;
	DWORD		Number;
	void		*pvPrevious;
};



struct LIST
{
	LISTELEMENT	le[CLIST_LISTSIZE];
	LIST		*pNext;
};



class CList
{
private:
	LIST					*pList, *pFirstList;
	LISTELEMENT				*pCurrentItem;
	DWORD					MaxItemNo;
	CLIST_CREATECALLBACK	CreateCallback;
	CLIST_DELETECALLBACK	DeleteCallback;

	void					Init(CLIST_CREATECALLBACK _CreateCallback, CLIST_DELETECALLBACK _DeleteCallback);
	LISTELEMENT				*GetListNo(DWORD dwNumber);

public:
	DWORD					dwUserData;

	CList(CLIST_CREATECALLBACK _CreateCallback, CLIST_DELETECALLBACK _DeleteCallback);
	CList();
	~CList();

	void					*NewItem(DWORD dwSize);
	void					*NewItem(DWORD dwSize, void *pv);
	void					PreviousItemToCurrent();
	void					*GetCurrentItem();
	DWORD					GetCurrentItemNo();
	DWORD					GetMaxItemNo();
	void					*GetItem(DWORD dwNumber);
	DWORD					GetSizeofItem(DWORD dwNumber);
};



#endif		//CLIST_H

