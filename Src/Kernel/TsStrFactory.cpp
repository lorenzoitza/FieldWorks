/*-----------------------------------------------------------------------*//*:Ignore in Surveyor
Copyright (c) 1999-2013 SIL International
This software is licensed under the LGPL, version 2.1 or later
(http://www.gnu.org/licenses/lgpl-2.1.html)

File: TsStringFactory.cpp
Responsibility: Jeff Gayle
Last reviewed:

	Implementation of ITsStringFactory.

	This is a thread-safe, "agile" component.
----------------------------------------------------------------------------------------------*/
#include "Main.h"
#pragma hdrstop

#include "Vector_i.cpp"

#undef THIS_FILE
DEFINE_THIS_FILE


// The class factory for TsStrFact.
static GenericFactory g_factStrFact(
	_T("FieldWorks.TsStrFactory"),
	&CLSID_TsStrFactory,
	_T("FieldWorks String Factory"),
	_T("Both"),
	&TsStrFact::CreateCom);

//:End Ignore

/*----------------------------------------------------------------------------------------------
	Called by the GenericFactory to "create" an ITsStringFactory. It just returns the global
	one.
----------------------------------------------------------------------------------------------*/
void TsStrFact::CreateCom(IUnknown * punkOuter, REFIID iid, void ** ppv)
{
	AssertPtr(ppv);
	Assert(!*ppv);

	if (punkOuter)
		ThrowHr(WarnHr(CLASS_E_NOAGGREGATION));

	CheckHr(KernelGlobals::g_strf->QueryInterface(iid, ppv));
}


/*----------------------------------------------------------------------------------------------
	QueryInterface.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::QueryInterface(REFIID iid, void ** ppv)
{
	AssertPtr(ppv);
	if (!ppv)
		return WarnHr(E_POINTER);
	*ppv = NULL;

	if (iid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (iid == IID_ITsStrFactory)
		*ppv = static_cast<ITsStrFactory *>(this);
	else if (iid == IID_ISupportErrorInfo)
	{
		*ppv = NewObj CSupportErrorInfo(this, IID_ITsStrFactory);
		return S_OK;
	}
#if WIN32
	else if (iid == IID_IMarshal)
		return m_qunkMarshaler->QueryInterface(iid, ppv);
#endif
	else
		return E_NOINTERFACE;

	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	Since this is a singleton, just addref the module.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP_(UCOMINT32) TsStrFact::AddRef(void)
{
	return ModuleEntry::ModuleAddRef();
}


/*----------------------------------------------------------------------------------------------
	Since this is a singleton, just release the module.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP_(UCOMINT32) TsStrFact::Release(void)
{
	return ModuleEntry::ModuleRelease();
}


/*----------------------------------------------------------------------------------------------
	Reads both the text characters and formatting information from streams and returns an
	ITsString.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::DeserializeStringStreams(IStream * pstrmTxt, IStream * pstrmFmt,
	ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComArgPtr(pstrmTxt);
	ChkComArgPtr(pstrmFmt);
	ChkComOutPtr(pptss);

	STATSTG stat;
	CheckHr(pstrmTxt->Stat(&stat, STATFLAG_NONAME));
	DataReaderStrm drsTxt(pstrmTxt);
	DataReaderStrm drsFmt(pstrmFmt);

	DeserializeStringCore(&drsTxt, &drsFmt, pptss);
	if (drsTxt.IbCur() != (int)stat.cbSize.LowPart)
	{
		ReleaseObj(*pptss);
		ThrowHr(WarnHr(E_UNEXPECTED));
	}

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}

/*----------------------------------------------------------------------------------------------
	Reads formatting information from the stream and returns an ITsString.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::DeserializeString(BSTR bstrTxt, IStream * pstrm, ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComBstrArgN(bstrTxt);
	ChkComArgPtr(pstrm);
	ChkComOutPtr(pptss);

	DataReaderStrm drs(pstrm);
	int cch = BstrLen(bstrTxt);
	DataReaderRgb drr(bstrTxt, cch * isizeof(OLECHAR));
	DeserializeStringCore(&drr, &drs, pptss);
	if (drr.IbCur() != cch * isizeof(OLECHAR))
	{
		ReleaseObj(*pptss);
		ThrowHr(WarnHr(E_UNEXPECTED));
	}

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Constructs a TsString from the given input.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::DeserializeStringRgb(BSTR bstrTxt, const byte * prgbFmt, int cbFmt,
											 ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComBstrArgN(bstrTxt);
	ChkComArrayArg(prgbFmt, cbFmt);
	ChkComOutPtr(pptss);

	int cchTxt = BstrLen(bstrTxt);
	DataReaderRgb drrTxt(bstrTxt, cchTxt * isizeof(OLECHAR));
	DataReaderRgb drrFmt(prgbFmt, cbFmt);
	DeserializeStringCore(&drrTxt, &drrFmt, pptss);
	if (drrTxt.IbCur() != cchTxt * isizeof(OLECHAR) || cbFmt != drrFmt.IbCur())
	{
		ReleaseObj(*pptss);
		ThrowHr(WarnHr(E_UNEXPECTED));
	}

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Reads formatting information from the byte array and text from the character array. Returns
	an ITsString. If this doesn't consume all of the text or bytes, it returns S_FALSE.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::DeserializeStringRgch(const OLECHAR * prgchTxt, int * pcchTxt,
											  const byte * prgbFmt, int * pcbFmt,
											  ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComArgPtr(pcchTxt);
	ChkComArgPtr(pcbFmt);
	ChkComOutPtr(pptss);
	ChkComArrayArg(prgchTxt, *pcchTxt);
	ChkComArrayArg(prgbFmt, *pcbFmt);

	int cchTxt = *pcchTxt;
	int cbFmt = *pcbFmt;
	DataReaderRgb drrTxt(prgchTxt, cchTxt * isizeof(OLECHAR));
	DataReaderRgb drrFmt(prgbFmt, cbFmt);

	DeserializeStringCore(&drrTxt, &drrFmt, pptss);

	*pcchTxt = drrTxt.IbCur() / isizeof(OLECHAR);
	*pcbFmt = drrFmt.IbCur();

	Assert(*pcchTxt <= cchTxt);
	if (*pcchTxt > cchTxt)
		ThrowHr(E_UNEXPECTED);
	Assert(*pcbFmt <= cbFmt);
	if (*pcbFmt > cbFmt)
		ThrowHr(E_UNEXPECTED);

	return *pcbFmt < cbFmt || *pcchTxt < cchTxt ? S_FALSE : S_OK;

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Reads formatting information from a DataReader and returns an ITsString.
	ENHANCE JohnT: is it acceptable to get the total number of characters from pdrdrTxt->Size()?
	Potential problem: the interface DeserializeStringRgch indicates that it may not consume
	all the characters; but currently, the only way to know the end of the last run is that
	it is when all the characters have been used. This is fine with current clients, but
	may be a problem sometime.
	Note the discrepancy: the file stores a min offset for each run, the TsString stores
	a lim for each run. Thus, the file always (unless the first run has no formatting, which
	is not yet supported here) starts with zero offset, and has no info about the length
	of the last run; the TsString internal representation has info about each run, and assumes
	the first run starts at zero.
----------------------------------------------------------------------------------------------*/
void TsStrFact::DeserializeStringCore(DataReader * pdrdrTxt,
	DataReader * pdrdrFmt, ITsString ** pptss)
{
	AssertPtr(pdrdrTxt);
	AssertPtr(pdrdrFmt);
	AssertPtr(pptss);
	Assert(!*pptss);

	int crun = 0;
	if (pdrdrFmt->Size() > 0)
	{
		// Read the number of runs.
		pdrdrFmt->ReadInt(&crun);
	}
	Assert((pdrdrTxt->Size() % isizeof(OLECHAR)) == 0);
	int cch = pdrdrTxt->Size() / isizeof(OLECHAR);
	if (crun < 1)
	{
		TsStrSingle::Create(pdrdrTxt, cch, NULL, pptss);
		return;
	}

	// Calculate the number of bytes used to store properties.
	int cbruns = isizeof(int) + crun * 2 * isizeof(int);
	int cbttp;
	cbttp = pdrdrFmt->Size() - cbruns;
	if (cbttp < 0)
		ThrowHr(WarnHr(E_UNEXPECTED));
	// Read the runs.
	const int kcrunStat = 10;
	TxtRun rgrun[kcrunStat];
	Vector<TxtRun> vrun;
	TxtRun * prgrun;
	int rgoffset[kcrunStat];
	Vector<int> voffset;
	int * prgoffset;

	if (crun <= kcrunStat)
	{
		prgrun = rgrun;
		prgoffset = rgoffset;
	}
	else
	{
	   vrun.Resize(crun);
	   prgrun = vrun.Begin();
	   voffset.Resize(crun);
	   prgoffset = voffset.Begin();
	}
	int ichMin;
	TxtRun * prun = prgrun;
	TxtRun * prunLim = prgrun + crun;
	int * poffset = prgoffset;
	for ( ; prun < prunLim; prun++, poffset++)
	{
		pdrdrFmt->ReadInt(&ichMin);
		if (prun == prgrun)
		{
			// first run: offset had better be zero
			if (ichMin != 0)
				ThrowHr(WarnHr(E_FAIL), L"String format does not start at offset 0");
		}
		else
		{
			(prun - 1)->m_ichLim = ichMin;		// lim of previous run is min of this
#if 0
			// This is a mangled test I can no longer make sense of -- JohnT
			if (ibMin <= ibLim && (crun > 1 || prun->m_ibLim != 0))
				ThrowHr(WarnHr(E_UNEXPECTED));
#endif
			// Check run limits are in ascending order
			if (prun > prgrun + 1 && (prun - 2)->m_ichLim >= ichMin)
				ThrowHr(WarnHr(E_UNEXPECTED));
		}
		pdrdrFmt->ReadInt(poffset);
		// An offset of -1 means that the run has no properties.  This shouldn't happen, but if
		// it does, handle it properly at this level.
		Assert((unsigned)*poffset < (unsigned)cbttp || *poffset == -1);
	}
	// Last run ends at end of string.
	(prunLim - 1)->m_ichLim = cch;
	// Check limits in order for last run
	if (crun > 1 && (prunLim - 2)->m_ichLim >= cch)
		ThrowHr(WarnHr(E_UNEXPECTED));
	Assert(cbruns == pdrdrFmt->IbCur());
	int irun;
	int i;
	bool fAlready;
	for (irun = 0; irun < crun; ++irun)
	{
		for (fAlready = false, i = 0; i < irun; ++i)
		{
			if (prgoffset[i] == prgoffset[irun])
			{
				prgrun[irun].m_qttp = prgrun[i].m_qttp;
				fAlready = true;
				break;
			}
		}
		if (!fAlready)
		{
			if (prgoffset[irun] == -1)
			{
				// An offset of -1 means that the run has no properties.  This shouldn't happen,
				// but if it does, handle it properly at this level.
				TsTextProps::Create(NULL, 0, NULL, 0, &prgrun[irun].m_qttp);
			}
			else
			{
				pdrdrFmt->SeekAbs(cbruns + prgoffset[irun]);
				TsTextProps::DeserializeDataReader(pdrdrFmt, &prgrun[irun].m_qttp);
			}
		}
	}

	// Create the string object.
	if (crun == 1)
		TsStrSingle::Create(pdrdrTxt, cch, prgrun[0].m_qttp, pptss);
	else
		TsStrMulti::Create(pdrdrTxt, prgrun, crun, pptss);
}


/*----------------------------------------------------------------------------------------------
	Make a string with the given characters and writing system and no style.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::MakeString(BSTR bstr, int ws, ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComBstrArgN(bstr);
	ChkComOutPtr(pptss);
	if ((uint)ws > kwsLim)
		ThrowInternalError(E_INVALIDARG, "Magic writing system invalid in string");
	Assert(ws);
	if (!ws)
		ThrowInternalError(E_INVALIDARG, "writing system zero invalid in string");

	// Deliberately duplicated for performance from MakeStringRgch
	ComSmartPtr<TsTextProps> qttp;
	TsIntProp tip;
	tip.m_tpt = ktptWs;
	tip.m_nVar = 0;
	tip.m_nVal = ws;		// Writing system
	TsTextProps::CreateCanonical(&tip, 1, NULL, 0, &qttp);
	TsStrSingle::Create(bstr, BstrLen(bstr), qttp, pptss);

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Make a string with the given characters and writing system and no style.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::MakeStringRgch(const OLECHAR * prgch, int cch, int ws,
	ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComArrayArg(prgch, cch);
	ChkComOutPtr(pptss);
	if ((uint)ws > kwsLim)
		ThrowInternalError(E_INVALIDARG, "Magic writing system invalid in string");
	Assert(ws);
	if (!ws)
		ThrowInternalError(E_INVALIDARG, "writing system zero invalid in string");

	// This code is duplicated (for performance) in MakeString.
	ComSmartPtr<TsTextProps> qttp;
	TsIntProp tip;
	tip.m_tpt = ktptWs;
	tip.m_nVar = 0;
	tip.m_nVal = ws;		// Writing system
	TsTextProps::CreateCanonical(&tip, 1, NULL, 0, &qttp);
	TsStrSingle::Create(prgch, cch, qttp, pptss);

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}

/*----------------------------------------------------------------------------------------------
	Make a string with the given characters and and properties.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::MakeStringWithPropsRgch(const OLECHAR * prgch, int cch,
	ITsTextProps * pttp, ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComArrayArg(prgch, cch);
	ChkComArgPtr(pttp);
	ChkComOutPtr(pptss);

	DataReaderRgb drr(prgch, cch * isizeof(OLECHAR));
	TsStrSingle::Create(&drr, cch, pttp, pptss);

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Get an ITsStrBldr initially with empty state.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::GetBldr(ITsStrBldr ** pptsb)
{
	BEGIN_COM_METHOD;
	ChkComOutPtr(pptsb);

	TsStrBldr::Create(NULL, 0, NULL, 0,  pptsb);

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Get an ITsIncStrBldr with initially empty state.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::GetIncBldr(ITsIncStrBldr ** pptisb)
{
	BEGIN_COM_METHOD;
	ChkComOutPtr(pptisb);

	TsIncStrBldr::Create(NULL, 0, NULL, 0, pptisb);

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Get the run count given the format.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::get_RunCount(const byte * prgbFmt, int cbFmt, int * pcrun)
{
	BEGIN_COM_METHOD;
	ChkComArrayArg(prgbFmt, cbFmt);
	ChkComOutPtr(pcrun);

	if (!cbFmt)
		*pcrun = 0;
	else
		*pcrun = reinterpret_cast<const int *>(prgbFmt)[0] & 0x7FFFFFFF;

	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}

/*----------------------------------------------------------------------------------------------
	Fill in the TsRunInfo given irun.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::FetchRunInfo(const byte * prgbFmt, int cbFmt,  int irun,
									 TsRunInfo * ptri, ITsTextProps ** ppttp)
{
	BEGIN_COM_METHOD;
	if (cbFmt <= 0)
		ThrowHr(WarnHr(E_INVALIDARG));
	ChkComArrayArg(prgbFmt, cbFmt);
	ChkComArgPtr(prgbFmt);
	ChkComOutPtr(ppttp);
	ChkComArgPtr(ptri);

#ifdef TODO2
	ClearItems(ptri, 1);

	// Get the first Int and determine if the objects bit is set.
	const int * prgnFmt = reinterpret_cast<const int *>(prgbFmt);
	int crun = prgnFmt[0];

	// Extract the number of runs.
	crun &= 0x7FFFFFFF;
	if (crun < 1)
		return WarnHr(E_INVALIDARG);
	Assert((uint)irun < (uint)crun);

	// Index past the count of runs in the buffer.
	prgnFmt++;

	// Get the min and lim of the run.
	int ibLim;
	int ibMin;

	// Get the ibMin info from the previous run if there is one, otherwise the ibMin is 0.
	// Determine how many ints are stored for each run. 3 if there are objects, 2 otherwise.
	int cbFmtRun = 2;

	if (irun)
		ibMin = prgnFmt[(irun - 1) * cbFmtRun];
	else
		ibMin = 0;

	 // Set the pointer to the ibLim of irun;
	prgnFmt += irun * cbFmtRun;

	// Get the ibLim from the run specified. Index the pointer after use to skip past the ibLim
	// to next value.
	ibLim = *prgnFmt++;
	if ((ibLim % isizeof(OLECHAR)) != 0 || ibMin >= ibLim)
		return WarnHr(E_UNEXPECTED);

	HRESULT hr;
	IgnoreHr(hr = pool->GetTextPropsFromCookie(*prgnFmt, ppttp));
	if (FAILED(hr))
		return hr;
	AddRefObj(*ppttp);

	// Fill in the TsRunInfo structure.
	ptri->irun = irun;
	ptri->ichMin = ibMin / isizeof(OLECHAR);
	ptri->ichLim = ibLim / isizeof(OLECHAR);

	return S_OK;
#else
	ThrowHr(WarnHr(E_NOTIMPL));
#endif
	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}


/*----------------------------------------------------------------------------------------------
	Fill in the TsRunInfo given irun.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::FetchRunInfoAt(const byte * prgbFmt, int cbFmt, int ich,
									   TsRunInfo * ptri, ITsTextProps ** ppttp)
{
	// TODO JeffG(ShonK): Make this use FetchRunInfo.
	BEGIN_COM_METHOD;
	if (cbFmt <= 0)
		ThrowHr(WarnHr(E_INVALIDARG));
	ChkComArrayArg(prgbFmt, cbFmt);
	ChkComArgPtr(prgbFmt);
	ChkComOutPtr(ppttp);
	ChkComArgPtr(ptri);

#ifdef TODO2
	ClearItems(ptri, 1);

	// Get the first Int and determine if the objects bit is set.
	const int * prgnFmt = reinterpret_cast<const int *>(prgbFmt);
	int crun = prgnFmt[0];

	// Extract the number of runs.
	if (crun < 1)
		return WarnHr(E_UNEXPECTED);

	// Index past the count of runs in the buffer.
	prgnFmt++;

	// Get the run from the char position.
	int irun;
	int ibMin;

	// Determine how many ints are stored for each run. 3 if there are objects, 2 otherwise.
	int cbFmtRun = 2;

	// Loop through the runs to find the run containing ich.
	for (irun = 0, ibMin = 0; irun < crun; irun++)
	{
		int ibLimT = prgnFmt[irun * cbFmtRun];
		if (ibLimT / isizeof(OLECHAR) > ich)
			break;

		// Set the min to the lim of this run if it does not contain ich.
		ibMin = ibLimT;
	}

	if (irun == crun)
		return WarnHr(E_INVALIDARG);

	 // Set the pointer to the ibLim of irun;
	prgnFmt += irun * cbFmtRun;

	// Index the pointer after use to skip past the ibLim to next value.
	int ibLim = *prgnFmt++;
	if ((ibLim % isizeof(OLECHAR)) != 0 || ibMin >= ibLim)
		return WarnHr(E_UNEXPECTED);

	// At this point we have the ibMin and ibLim for the run.

	HRESULT hr;
	IgnoreHr(hr = pool->GetTextPropsFromCookie(*prgnFmt, ppttp));
	if (FAILED(hr))
		return hr;
	AddRefObj(*ppttp);

	// Fill in the TsRunInfo structure.
	ptri->irun = irun;
	ptri->ichMin = ibMin / isizeof(OLECHAR);
	ptri->ichLim = ibLim / isizeof(OLECHAR);

	return S_OK;
#else
	ThrowHr(WarnHr(E_NOTIMPL));
#endif
	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}

/*----------------------------------------------------------------------------------------------
	Return an empty string in the specified writing system.
	These are cached so a new object does not have to be created every time.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP TsStrFact::EmptyString(int ws, ITsString ** pptss)
{
	BEGIN_COM_METHOD;
	ChkComOutPtr(pptss);
	ITsStringPtr qtss;
	LOCK (m_mutex)
	{
		if (m_hmwsqtssEmptyStrings.Retrieve(ws, qtss))
		{
			*pptss = qtss.Detach();
		}
		else
		{
			CheckHr(MakeString(NULL, ws, pptss));
			m_hmwsqtssEmptyStrings.Insert(ws, *pptss);
		}
	}
	END_COM_METHOD(g_factStrFact, IID_ITsStrFactory);
}

/*----------------------------------------------------------------------------------------------
	Skip over whitespace characters in the string starting at ichMin, returning the index of the
	first non-whitespace character found.
----------------------------------------------------------------------------------------------*/
int TsStrFact::SkipWhitespace(StrUni stu, int ichMin)
{
	int ich = ichMin;
	int ichLim = stu.Length();
	for ( ; ich < ichLim; ++ich)
	{
		if (!iswspace(stu.GetAt(ich)))
			break;
	}
	return ich;
}


/*----------------------------------------------------------------------------------------------
	If the next and only attribute is ws="...", get its value and return true.  Otherwise return
	false.  Start the scan at ichMin, and don't go past ichLim (or ichLim - 1 to be precise).
----------------------------------------------------------------------------------------------*/
bool TsStrFact::GetOnlyWsAttribute(StrUni stuXml, int ichMin, int ichLim, StrUni & stuValue)
{
	int ichName = SkipWhitespace(stuXml, ichMin);
	if (ichName > ichMin && ichName < ichLim - 6 &&
		stuXml.GetAt(ichName) == L'w' &&
		stuXml.GetAt(ichName+1) == L's' &&
		stuXml.GetAt(ichName+2) == L'=')
	{
		int ichBeginQuote = ichName + 3;
		wchar chQuote = stuXml.GetAt(ichBeginQuote);
		if (chQuote == L'"' || chQuote == L'\'')
		{
			int ichEndQuote = stuXml.FindCh(chQuote, ichBeginQuote + 1);
			if (ichEndQuote > ichBeginQuote && ichEndQuote < ichLim)
			{
				int ichValue = ichBeginQuote + 1;
				stuValue.Assign(stuXml.Chars() + ichValue, ichEndQuote - ichValue);
				return SkipWhitespace(stuXml, ichEndQuote + 1) == ichLim;
			}
		}
	}
	stuValue.Clear();
	return false;
}

/*----------------------------------------------------------------------------------------------
	Get the next attribute name and value (if any) from inside the XML start tag that ends at
	ichLim.  Start the scan at ichMin.
----------------------------------------------------------------------------------------------*/
bool TsStrFact::TryParseSimpleXml(BSTR bstrXml, ILgWritingSystemFactory * pwsf,
	ITsString ** pptss)
{
	StrUni stuXml;
	StrUtil::TrimWhiteSpace(bstrXml, stuXml);
	// Check for the outer <Str> or <AStr> element.
	int ichStr = stuXml.FindStr("<Str>");
	int ichNext;
	int ichClose;
	if (ichStr == 0)
	{
		int ichEndStr = stuXml.FindStr("</Str>");
		if (ichEndStr != stuXml.Length() - 6)
			return false;
		ichNext = 5;
		ichClose = ichEndStr;
	}
	else
	{
		int ichAStr = stuXml.FindStr("<AStr");
		if (ichAStr == 0)
		{
			int ichEndAStr = stuXml.FindStr("</AStr>");
			if (ichEndAStr != stuXml.Length() - 7)
				return false;
			int ichAStrLim = stuXml.FindCh(L'>');
			if (ichAStrLim > ichEndAStr)
				return false;
			StrUni stuWs;
			if (!GetOnlyWsAttribute(stuXml, 5, ichAStrLim, stuWs))
				return false;
			ichNext = ichAStrLim + 1;
			ichClose = ichEndAStr;
		}
		else
		{
			return false;
		}
	}
	// Check for a single inner <Run> element.
	ichNext = SkipWhitespace(stuXml, ichNext);
	int ichRun = stuXml.FindStr(L"<Run");
	if (ichRun != ichNext)
		return false;
	int ichEndRun = stuXml.FindStr(L"</Run>");
	if (ichEndRun < ichRun)
		return false;
	ichNext = SkipWhitespace(stuXml, ichEndRun + 6);
	if (ichNext != ichClose)
		return false;
	int ichRunLim = stuXml.FindCh(L'>', ichRun + 4);
	if (ichRunLim < 0 || ichRunLim > ichEndRun)
		return false;
	int ich3rdGt = stuXml.FindCh(L'<', ichRunLim + 1);
	if (ich3rdGt != ichEndRun)
		return false;
	// Check that the <Run> element has a single attribute named 'ws'.
	StrUni stuWs;
	if (GetOnlyWsAttribute(stuXml, ichRun + 4, ichRunLim, stuWs))
	{
		int ws;
		CheckHr(pwsf->GetWsFromStr(stuWs.Bstr(), &ws));
		if (ws == 0)
			return false;
		int ichText = ichRunLim + 1;
		StrUni stuText(stuXml.Chars() + ichText, ichEndRun - ichText);
		// Create the simple string and return.
		bool fOk = DecodeCharacterEntities(stuText);
		if (fOk)
		{
			CheckHr(MakeString(stuText.Bstr(), ws, pptss));
			return true;
		}
	}
	return false;
}

#include "ComHashMap_i.cpp"
template class ComHashMap<int, ITsString>;
