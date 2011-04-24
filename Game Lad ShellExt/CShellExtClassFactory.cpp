#include	<windows.h>
#include	<shlobj.h>

#include	"Game Lad ShellExt.h"
#include	"CShellExt.h"
#include	"CShellExtClassFactory.h"



CShellExtClassFactory::CShellExtClassFactory()
{
	m_cRef = 0;

	DllRefCount++;
}



CShellExtClassFactory::~CShellExtClassFactory()
{
	DllRefCount--;
}



STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
	{
		*ppvObject = this;

		AddRef();

		return NOERROR;
	}

	return E_NOINTERFACE;
}



STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
	return ++m_cRef;
}



STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
	if (--m_cRef)
	{
		return m_cRef;
	}

	delete this;

	return 0;
}



STDMETHODIMP CShellExtClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	CShellExt	*pShellExt;


	*ppvObject = NULL;

	if (pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	if (!(pShellExt = new CShellExt()))
	{
		return E_OUTOFMEMORY;
	}

	return pShellExt->QueryInterface(riid, ppvObject);
}



STDMETHODIMP CShellExtClassFactory::LockServer(BOOL fLock)
{
	return NOERROR;
}

