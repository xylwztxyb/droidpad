
// mime.cpp : 定义 DLL 应用程序的导出函数。
//
#include "common.h"
#include "mime.h"

CRemoteClient *gRemoteClient = NULL;

struct CompositionInfo
{
	COMPOSITIONSTRING cs;
	WCHAR szCompStr[MAX_COMPOSITION_SIZE];
	WCHAR szResultStr[MAX_COMPOSITION_SIZE];
	void reset()
	{
		memset(this, 0, sizeof(*this));
		cs.dwSize = sizeof(*this);
		cs.dwCompStrOffset = (DWORD)((ptrdiff_t)szCompStr - (ptrdiff_t)this);
		cs.dwResultStrOffset = (DWORD)((ptrdiff_t)szResultStr - (ptrdiff_t)this);
	}
};

static HRESULT _FillIMEMsg(LPINPUTCONTEXT lpIMC, UINT msg, WPARAM wp, LPARAM lp)
{
	HIMCC hBuf = ImmReSizeIMCC(lpIMC->hMsgBuf,
							   sizeof(TRANSMSG) * (lpIMC->dwNumMsgBuf + 1));
	if (!hBuf)
		return E_FAIL;

	lpIMC->hMsgBuf = hBuf;
	LPTRANSMSG pBuf = (LPTRANSMSG)ImmLockIMCC(hBuf);
	if (!pBuf)
		return E_FAIL;

	DWORD last = lpIMC->dwNumMsgBuf;
	pBuf[last].message = msg;
	pBuf[last].wParam = wp;
	pBuf[last].lParam = lp;
	lpIMC->dwNumMsgBuf++;

	ImmUnlockIMCC(hBuf);
	return S_OK;
}

static HRESULT _SendComposition(HIMC hIMC, const wchar_t *wc, size_t len)
{
	LPINPUTCONTEXT lpIMC;
	LPCOMPOSITIONSTRING lpCompStr;

	lpIMC = ImmLockIMC(hIMC);
	if (!lpIMC)
		return E_FAIL;

	lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
	if (!lpCompStr)
	{
		ImmUnlockIMC(hIMC);
		return E_FAIL;
	}

	CompositionInfo *pInfo = (CompositionInfo *)lpCompStr;
	wcsncpy_s(pInfo->szResultStr, wc, len);
	lpCompStr->dwResultStrLen = wcslen(pInfo->szResultStr);

	ImmUnlockIMCC(lpIMC->hCompStr);

	if (_FillIMEMsg(lpIMC, WM_IME_STARTCOMPOSITION, 0, 0) == S_OK)
		ImmGenerateMessage(hIMC);

	if (_FillIMEMsg(lpIMC, WM_IME_COMPOSITION, 0, GCS_COMP | GCS_RESULTSTR) == S_OK)
		ImmGenerateMessage(hIMC);

	if (_FillIMEMsg(lpIMC, WM_IME_ENDCOMPOSITION, 0, 0) == S_OK)
		ImmGenerateMessage(hIMC);

	if (_FillIMEMsg(lpIMC, WM_IME_NOTIFY, IMN_CLOSECANDIDATE, 0) == S_OK)
		ImmGenerateMessage(hIMC);

	ImmUnlockIMC(hIMC);
	return S_OK;
}

static HRESULT _Initialize(HIMC hIMC)
{
	LPINPUTCONTEXT lpIMC = ImmLockIMC(hIMC);
	if (!lpIMC)
		return E_FAIL;

	lpIMC->fOpen = TRUE;

	HIMCC &hIMCC = lpIMC->hCompStr;
	if (!hIMCC)
		hIMCC = ImmCreateIMCC(sizeof(CompositionInfo));
	else
		hIMCC = ImmReSizeIMCC(hIMCC, sizeof(CompositionInfo));
	if (!hIMCC)
	{
		ImmUnlockIMC(hIMC);
		return E_FAIL;
	}

	CompositionInfo *pInfo = (CompositionInfo *)ImmLockIMCC(hIMCC);
	if (!pInfo)
	{
		ImmUnlockIMC(hIMC);
		return E_FAIL;
	}

	pInfo->reset();
	ImmUnlockIMCC(hIMCC);
	ImmUnlockIMC(hIMC);

	return S_OK;
}

DWORD WINAPI ImeConversionList(HIMC hIMC, LPCTSTR lpSrc, LPCANDIDATELIST lpDst, DWORD dwBufLen, UINT uFlag)
{
	return 0;
}

BOOL WINAPI ImeConfigure(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
	return FALSE;
}

BOOL WINAPI ImeDestroy(UINT uReserved)
{
	return TRUE;
}

LRESULT WINAPI ImeEscape(HIMC hIMC, UINT uEscape, LPVOID lpData)
{
	return 0;
}

BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT uVirKey, LPARAM lParam, CONST LPBYTE lpbKeyState)
{
	if (uVirKey == VK_PROCESSKEY)
	{
		std::wstring wstr = gRemoteClient->getWordSet();
		if (!wstr.empty())
			_SendComposition(hIMC, wstr.c_str(), wstr.length());
	}
	return FALSE;
}

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL selected)
{
	if (selected)
	{
		_Initialize(hIMC);
		if (gRemoteClient->resumeThread())
			gRemoteClient->IMEOnClientSelect(true);
		else
			gRemoteClient->showMessage(L"Fail to start remote proxy client.");
	}
	else
	{
		gRemoteClient->IMEOnClientSelect(false);
		gRemoteClient->suspendThread();
	}
	return TRUE;
}

BOOL WINAPI ImeSetActiveContext(HIMC hIMC, BOOL fFlag)
{
	if (gRemoteClient)
		gRemoteClient->IMEOnClientActive(fFlag);
	return TRUE;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen)
{
	return FALSE;
}

UINT WINAPI ImeToAsciiEx(UINT virtual_key, UINT scan_code, CONST LPBYTE key_state, LPTRANSMSGLIST message_list, UINT fuState, HIMC hIMC)
{
	return 0;
}

BOOL WINAPI ImeInquire(LPIMEINFO lpImeInfo, LPTSTR lpszUIClass, DWORD dwSystemInfoFlags)
{
	lpImeInfo->dwPrivateDataSize = 0;
	lpImeInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST |
							 IME_PROP_SPECIAL_UI |
							 IME_PROP_CANDLIST_START_FROM_1 |
							 IME_PROP_UNICODE |
							 IME_PROP_END_UNLOAD;
	lpImeInfo->fdwConversionCaps = IME_CMODE_SYMBOL |
								   IME_CMODE_FULLSHAPE |
								   IME_CMODE_SOFTKBD;
	lpImeInfo->fdwSentenceCaps = IME_SMODE_NONE;
	lpImeInfo->fdwUICaps = UI_CAP_SOFTKBD |
						   UI_CAP_2700;
	lpImeInfo->fdwSCSCaps = SCS_CAP_COMPSTR;
	lpImeInfo->fdwSelectCaps = 0;

	wcscpy(lpszUIClass, MIME_UI_CLASS_NAME);

	gRemoteClient->initialize();
	return TRUE;
}

BOOL WINAPI ImeRegisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
	return TRUE;
}

BOOL WINAPI ImeUnregisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
	return TRUE;
}

UINT WINAPI ImeGetRegisterWordStyle(UINT nItem, LPSTYLEBUF lpStyleBuf)
{
	return 0;
}

UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfnEnumProc, LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString, LPVOID lpData)
{
	return 0;
}

BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
	return TRUE;
}

LRESULT WINAPI UIWndProc(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	return S_OK;
}
