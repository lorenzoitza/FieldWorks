/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (c) 1999-2013 SIL International
This software is licensed under the LGPL, version 2.1 or later
(http://www.gnu.org/licenses/lgpl-2.1.html)

File: LgKeymanHandler.cpp
Responsibility: John Thomson
Last reviewed: Not yet.

Description:  A wrapper for the keyman program.
-------------------------------------------------------------------------------*//*:End Ignore*/

//:>********************************************************************************************
//:>	   Include files
//:>********************************************************************************************
#include "Main.h"

#pragma hdrstop
#include "limits.h"
#undef THIS_FILE
DEFINE_THIS_FILE

//:>********************************************************************************************
//:>A bit of special header information needed only in this file, for interacting with Keyman.
//:>********************************************************************************************

#ifndef _KEYMANAPI_H
#define _KEYMANAPI_H

typedef struct tagKEYBOARDINFO
{
	DWORD      KeymanID;
	DWORD      HotKey;
	DWORD      KeyboardID;
	char       Name[256];
	// The original type for pKeyboard was LPKEYBOARD Keyboard, but we don't have defn of
	// LPKEYBOARD
	int * pKeyboard;
	DWORD      nIMDLLs;
	DWORD /*LPIMDLL*/ IMDLLs;
	// JohnT: The definition Marc gave me didn't have this, but the objects version 6 returns
	// are four bytes bigger than Marc's typedef indicated. This aligns things properly and
	// makes sure we allocate enough memory.
	//DWORD Dummy;
} KEYBOARDINFO, *LPKEYBOARDINFO;

typedef BOOL (WINAPI *PFNKeyman_BuildKeyboardList)(LPKEYBOARDINFO kbd, int*n);
//extern "C" HWND WINAPI Keyman_GetLastActiveWindow();
//extern "C" HWND WINAPI Keyman_GetLastFocusWindow();

// Marc Durdin gave me a header with this, but not LPKEYBOARD; hopefully we don't need it.
//extern "C" PWSTR WINAPI GetSystemStore(LPKEYBOARD kb, DWORD SystemID);
// I'm making the first argument int * to correspond to what I made pKeyboard in KEYBOARDINFO,
// since that's the only place we get an argument from.
typedef PWSTR (WINAPI *PFNGetSystemStore)(int * pKeyboard, DWORD SystemID);
#define TSS_NAME 7 // from Tavultesoft FAQ4 sample

typedef DWORD (WINAPI *PFNGetActiveKeymanID)();

#define KEYMAN_API_V6		 0x0600

//extern "C" DWORD WINAPI Keyman_GetAPIVersion();

#endif

// statics required for Keyman
//:>********************************************************************************************
//:>	   Local Constants and static variables
//:>********************************************************************************************
struct KbdInfo
{
	StrUni m_stuName;
	int m_id;
};
typedef Vector<KbdInfo> VecKbdInfo;
static int s_wm_kmselectlang = 0; // Windows message Keyman uses to change language
static int s_wm_kmkbchange = 0;
static bool s_fKeymanInitialized = false; // flag set when keyman has initialized
static bool s_fKeymanFailed = false; // flag set when keyman has failed to initialized
static VecKbdInfo s_vkiKeyboards; // Info about them.
const int knKeymanID = 1;  // skKeymanID in Marc's code; param to PostMessage.
#define KEYMANID_NONKEYMAN 0xFFFFFFFF       //Keyboard default (off) ID

typedef DWORD (WINAPI *PGETACTIVEKEYMANIDFUNC)();

static PFNKeyman_BuildKeyboardList pKeyman_BuildKeyboardList = 0;
static PFNGetActiveKeymanID pGetActiveKeymanID = 0;
static PFNGetSystemStore pGetSystemStore = 0;
//:>********************************************************************************************
//:>	   Local methods.
//:>********************************************************************************************

// Keyman is up and running if its window exists (even if not visible).
// Marc's sample code indicates this works for version 5 and 6 (and presumably will keep
// working :-).
bool KeymanOn()
{
	// Finding a window called Keyman50 is the recommended technique for detecting Keyman 5
	// AND ALSO Keyman 6.
#if WIN32
	if (::FindWindowA(NULL, "Keyman50") != 0)
		return true;
	// Keyman 7 can typically be found by this code:
		//::FindWindowA(NULL, "Keyman") != 0);
	// But Marc Durdin recommends this:
	HANDLE hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, false, L"KeymanEXE70");
	if (hMutex != 0)
	{
	  // keyman is running
		::CloseHandle(hMutex);
		return true;
	}
#endif
	return false; // Keyman is not running.
}

//:>********************************************************************************************
//:>	   Constructor/Destructor
//:>********************************************************************************************

LgKeymanHandler::LgKeymanHandler()
{
	m_cref = 1;
	ModuleEntry::ModuleAddRef();
}

LgKeymanHandler::~LgKeymanHandler()
{
	Close();
	ModuleEntry::ModuleRelease();
}

//:>********************************************************************************************
//:>	   Generic factory stuff to allow creating an instance with CoCreateInstance.
//:>********************************************************************************************
static GenericFactory g_fact(
	L"SIL.Language1.LgKeymanHandler",
	&CLSID_LgKeymanHandler,
	_T("SIL Keyman Handler"),
	_T("Apartment"),
	&LgKeymanHandler::CreateCom);


void LgKeymanHandler::CreateCom(IUnknown *punkCtl, REFIID riid, void ** ppv)
{
	AssertPtr(ppv);
	Assert(!*ppv);
	if (punkCtl)
		ThrowHr(WarnHr(CLASS_E_NOAGGREGATION));

	ComSmartPtr<LgKeymanHandler> qlkh;
	qlkh.Attach(NewObj LgKeymanHandler());		// ref count initialy 1
	CheckHr(qlkh->QueryInterface(riid, ppv));
}

//:>********************************************************************************************
//:>	   IUnknown Methods
//:>********************************************************************************************
STDMETHODIMP LgKeymanHandler::QueryInterface(REFIID riid, void **ppv)
{
	AssertPtr(ppv);
	if (!ppv)
		return WarnHr(E_POINTER);
	*ppv = NULL;

	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (riid == IID_ILgKeymanHandler)
		*ppv = static_cast<ILgKeymanHandler *>(this);
	else if (riid == IID_ISupportErrorInfo)
	{
		*ppv = NewObj CSupportErrorInfo(static_cast<ILgKeymanHandler *>(this),
			IID_ILgKeymanHandler);
		return S_OK;
	}
	else
		return E_NOINTERFACE;

	AddRef();
	return NOERROR;
}

//:>********************************************************************************************
//:>	   Error Handling Methods
//:>********************************************************************************************
/*----------------------------------------------------------------------------------------------
	Creates an error object and then sets a description from a resource id. Also sets a full
	help URL as required by HtmlHelp. Uses ierr as an index for both resource id and help URL.
	@param ierr Index to a set of htm help files (second part of full help URL) and matching
	resource strings for Message Box text.
	@param pei [out] Error info object
----------------------------------------------------------------------------------------------*/
void LgKeymanHandler::ThrowErrorWithInfo(HRESULT hrErr, int stidDescription)
{
	IErrorInfoPtr qei;
	ICreateErrorInfoPtr qcei;

	// Create error info object.
	CheckHr(CreateErrorInfo(&qcei));

	StrUni stu(stidDescription);
	CheckHr(qcei->SetDescription((wchar *)stu.Chars()));

	// Now get the IErrorInfo interface of the error object and set it for the current thread.
	CheckHr(qcei->QueryInterface(IID_IErrorInfo, (void **)&qei));
	SetErrorInfo(0, qei);

	ThrowHr(hrErr, stu.Chars(), -1, qei);	// An error object exists.
}

//:>********************************************************************************************
//:>	   Interface Methods
//:>********************************************************************************************

/*----------------------------------------------------------------------------------------------
	Initialize the Keyboard handler. Set fForce to re-initialize even if already
	initialized, for example, if a keyboard has been added or removed.

	Load the Keyman library, initialize address pointers and messages, and load keyboards into
	memory.

	If Keyman isn't installed or isn't registered or isn't running, we just return S_OK.
	FieldWorks does not require Keyman except where a writing system indicates it is used.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP LgKeymanHandler::Init(ComBool fForce)
{
	BEGIN_COM_METHOD

	if (fForce)
		s_fKeymanInitialized = false;
	// If not forcing, and already initialized, done.
	if (s_fKeymanInitialized)
		return S_OK;

	// TE-5637, 27 JUL 2007:  Moved this line up from below to prevent crash
	// Get rid of any info from previous Init calls.
	s_vkiKeyboards.Clear();


#if WIN32
	// Keyman must be running - but don't start it here to avoid possible conflicts
	if (!KeymanOn())
		return S_OK;

	if (!s_wm_kmselectlang)
		s_wm_kmselectlang = ::RegisterWindowMessageW(L"WM_KMSELECTLANG");

	// This really only needs to be done once, even if fForce is true;
	if (!pGetActiveKeymanID)
	{
		// Load the Keyman library and initialize needed function pointers.
		// This correctly handles keyman 5-8. Perhaps 9-10?
		// Review: for 7, might be good enough to test as far as keyman engine, leave off 7.0. That should
		// verify AT LEAST keyman 7.
		// Note: try 7 first. This works better if more than one is installed, or if registry keys
		// got left behind.
		RegKey rk;
		rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman engine\\10.0")); // Anticipate 9/10 even though it may be years away.
		if (rk == 0)
			rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman engine\\9.0"));
		if (rk == 0)
			rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman engine\\8.0"));
		if (rk == 0)
			rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman engine\\7.0"));
		if (rk == 0)
			rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman\\6.0"));
		if (rk == 0)
			rk.InitLm(_T("SOFTWARE\\Tavultesoft\\keyman\\5.0"));
		if (rk == 0)
			ThrowErrorWithInfo(E_UNEXPECTED, kstidKeymanNotRegisteredMsg);

		wchar rgch[MAX_PATH];
		DWORD cb = isizeof(rgch);
		DWORD dwT;
		LONG lRet = ::RegQueryValueExW(rk, L"root path", NULL, &dwT, (BYTE *)rgch, &cb);
		if (lRet != ERROR_SUCCESS)
			ThrowErrorWithInfo(WarnHr(E_UNEXPECTED), kstidKeymanRootNotRegisteredMsg);
		Assert(dwT == REG_SZ);
		StrUni stuKeymanDllPath;
		stuKeymanDllPath.Assign(rgch);
		if (stuKeymanDllPath.Chars()[stuKeymanDllPath.Length() - 1] != '\\')
			stuKeymanDllPath.Append(L"\\");
		stuKeymanDllPath.Append(L"Keyman32.dll");
		HMODULE hm = ::LoadLibraryW(stuKeymanDllPath.Chars());
		if (hm == NULL)
			ThrowErrorWithInfo(WarnHr(E_UNEXPECTED), kstidKeymanDllLoadFailureMsg);
		/*
		  From Marc Durdin:

		  Keyman doesn't initialize for a given process until after the first
		  POSTED message is received.  This is due to the way that Windows handles
		  hookprocs...  the DLL does not get loaded for the process until it is
		  needed.  It is possible to force Keyman to initialize early by posting a
		  thread message and then handling it immediately with GetMessage, something
		  like this (you may need to add a dose of error checking of course...):
		 */
		MSG msg;
		while (!::PostThreadMessage(::GetCurrentThreadId(), WM_NULL, 0, 0) &&
			::GetLastError() == ERROR_INVALID_THREAD_ID)
		{
			::Sleep(0);
		}
		::GetMessage(&msg, NULL, WM_NULL, WM_NULL);

		pKeyman_BuildKeyboardList = (PFNKeyman_BuildKeyboardList)
			::GetProcAddress(hm, "Keyman_BuildKeyboardList");
		pGetActiveKeymanID = (PFNGetActiveKeymanID)
			::GetProcAddress(hm, "GetActiveKeymanID");
		pGetSystemStore = (PFNGetSystemStore)
			::GetProcAddress(hm, "GetSystemStore");
	}

//-	// Register Keyman message. May as well use A version to save memory on constant.
//-	s_wm_kmselectlang = ::RegisterWindowMessageA("WM_KMSELECTLANG");

	Vector<KEYBOARDINFO> vkbi;
	int nKeyboards = 0;
	(*pKeyman_BuildKeyboardList)(NULL, &nKeyboards);
	if (nKeyboards == 0)
		return S_OK; // no keyboards installed.
	vkbi.Resize(nKeyboards);
	(*pKeyman_BuildKeyboardList)(vkbi.Begin(), &nKeyboards);

	// TE-5637, 27 JUL 2007:  Removed this line and moved it up to prevent crash
	// Get rid of any info from previous Init calls.
	//s_vkiKeyboards.Clear();

	// Get actual keyboard name where possible
	for (int ikbd = 0; ikbd < nKeyboards; ++ikbd)
	{
		KEYBOARDINFO & kbi = vkbi[ikbd];
		// From comments in Marc's code, it seems if this is 0 it isn't a valid,
		// useable keyboard. Ignore it.
		if (kbi.pKeyboard == 0)
			continue;

		KbdInfo ki;
		ki.m_stuName = (*pGetSystemStore)(kbi.pKeyboard, TSS_NAME);
		ki.m_id = kbi.KeymanID;
		s_vkiKeyboards.Push(ki);
	}

	s_fKeymanInitialized = true;
#endif
	END_COM_METHOD(g_fact, IID_ILgKeymanHandler);
}

/*----------------------------------------------------------------------------------------------
	Gracefully shut down the keyboard handler. We need this for calling from .NET so that we
	don't rely on garbage collection.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP LgKeymanHandler::Close()
{
	BEGIN_COM_METHOD

	END_COM_METHOD(g_fact, IID_ILgKeymanHandler);
}

/*----------------------------------------------------------------------------------------------
	Initialize the Keyboard handler if not already initialized.
	If Keyman couldn't be initialized, we just return false.
	FieldWorks does not require Keyman except where a writing system indicates it is used.
----------------------------------------------------------------------------------------------*/
bool LgKeymanHandler::InitInternal()
{
	if (s_fKeymanInitialized)
		return true;
	if (s_fKeymanFailed)
	{
		Warn("Skipping Keyman initialization due to previous failure.");
		return false;
	}
	StrUni stuCaption(kstidKeymanInitFailedCaption);
	StrUni stuMsg;
	try
	{
		if (!s_fKeymanInitialized)
		{
			HRESULT hr = Init(false);
			if (SUCCEEDED(hr))
				return true;
			IErrorInfo * pIErrorInfo = NULL;
			::GetErrorInfo(0, &pIErrorInfo);
			BSTR bstrDescription;
			pIErrorInfo->GetDescription(&bstrDescription);
			stuMsg.Assign(bstrDescription);
		}
	}
	catch (Throwable error)
	{
		stuMsg.Load(kstidKeymanInitUnexpectedFailMsg);
	}
	s_fKeymanFailed = true;
#if WIN32
	::MessageBox(NULL, stuMsg.Chars(), stuCaption.Chars(), MB_OK | MB_ICONINFORMATION);
#endif // !WIN32
	return s_fKeymanInitialized;
}

/*----------------------------------------------------------------------------------------------
	Get the active keyboard. Returns NULL if Keyman is not running or otherwise can't be
	initialized; "(None)" if no keyman keyboard active.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP LgKeymanHandler::get_ActiveKeyboardName(BSTR * pbstrName)
{
	BEGIN_COM_METHOD
	ChkComOutPtr(pbstrName);
	if (!InitInternal())
		return S_OK;
#if WIN32
	//int nActiveKeymanId = (*pGetActiveKeymanID)();
	HMODULE hKeyman = GetModuleHandleA("keyman32.dll");
	if(hKeyman == 0)
		return S_OK; // leave name null

	PGETACTIVEKEYMANIDFUNC pGetActiveKeymanID =
		(PGETACTIVEKEYMANIDFUNC) GetProcAddress(hKeyman, "GetActiveKeymanID");
	if(!pGetActiveKeymanID)
		return S_OK; // leave name null

	int nActiveKeymanId = (*pGetActiveKeymanID)();

	if (nActiveKeymanId == -1)
	{
		*pbstrName = ::SysAllocString(L"(None)"); // Todo JohnT: localize?
#ifdef TRACING_KEYMAN
		StrAnsi staMsg;
		staMsg.Format("Keyman active ID is %d name is %S\n", nActiveKeymanId, *pbstrName);
		OutputDebugStringA(staMsg.Chars());
#endif
		return S_OK;
	}
	for (int iki = 0; iki < s_vkiKeyboards.Size(); ++iki)
	{
		if (s_vkiKeyboards[iki].m_id == nActiveKeymanId)
		{
			s_vkiKeyboards[iki].m_stuName.GetBstr(pbstrName);
#ifdef TRACING_KEYMAN
			StrAnsi staMsg;
			staMsg.Format("Keyman active ID is %d name is %S\n", nActiveKeymanId, *pbstrName);
			OutputDebugStringA(staMsg.Chars());
#endif
			return S_OK;
		}
	}
#endif
	Assert(false); // Keyman gave us an ID, but it didn't match!
	return E_UNEXPECTED;
	END_COM_METHOD(g_fact, IID_ILgKeymanHandler);
}

// Explicit instantiation.
#include "Vector_i.cpp"
template class Vector<KEYBOARDINFO>;
template class Vector<KbdInfo>; // VecKbdInfo;
