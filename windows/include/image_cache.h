#ifndef __IMAGE_CACHE_H__
#define __IMAGE_CACHE_H__

#include "common.h"

#pragma comment(lib, "GdiPlus.lib")
#ifdef NDEBUG
#pragma comment(lib, "zlibwapi.lib")
#else
#pragma comment(lib, "zlibwapid.lib")
#endif


namespace DroidPad
{


#define IME_STATUS_BAR_SKIN_PATH L"res\\skin.gz"

enum eBarState
{
  STATE_NOT_CONNECTED,
  STATE_CONNECTED_BY_USB,
  STATE_CONNECTED_BY_WIFI,
  STATE_CONNECTING,
  STATE_CONNECTION_FAIL,
  STATE_MAX_SIZE,
  /////////////////
  STATE_UPDATE_POS
};

//extern LPCWCHAR IME_STATUS_BAR_IMG_SOURCE[STATE_MAX_SIZE];

class CImageCache
{
public:
  static bool cacheDC(HDC hWinDC, int iWidth, int iHeight);
  static void clean();
  static HDC getCachedDC(eBarState eState);

private:
  static HDC m_hCachedImageDC[STATE_MAX_SIZE];
};

}

#endif