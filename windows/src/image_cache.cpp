#include "common.h"
#include "image_cache.h"
#include "util.h"
#include "mime.h"

#define ZLIB_WINAPI
#include "zlib/unzip.h"

#ifdef _WIN32
#define USEWIN32IOAPI
#include "zlib\iowin32.h"
#endif

using namespace DroidPad;

////////////////////////////////////////////////////////////
static LPCWCHAR IME_STATUS_BAR_IMG_SOURCE[STATE_MAX_SIZE] =
    {
        L"status_bar_not_connected.png",
        L"status_bar_connected_by_usb.png",
        L"status_bar_connected_by_wifi.png",
        L"status_bar_connecting.png",
        L"status_bar_connecting_fail.png"};

////////////////////////////////////////////////////////////

static IStream *_createStream(LPCWCHAR _file)
{
    WCHAR real_path[MAX_PATH] = {'\0'};
    WCHAR root_path[MAX_PATH] = {'\0'};
    if (getRootPath(root_path, MAX_PATH - 1) != STATUS_NO_ERROR)
        return NULL;
    _snwprintf_s(real_path, MAX_PATH, L"%s\\%s", root_path, IME_STATUS_BAR_SKIN_PATH);

    // open and read the skin file
    unzFile zFile;
    char *buff = NULL;
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
#ifdef UNICODE
    fill_win32_filefunc64W(&ffunc);
#else
    fill_win32_filefunc64A(&ffunc);
#endif
#endif
    do
    {
        if (!(zFile = unzOpen2_64(real_path, &ffunc)))
            break;
        if (unzLocateFile(zFile, wideToANSI(_file).c_str(), 0) != UNZ_OK)
            break;
        if (unzOpenCurrentFile(zFile) != UNZ_OK)
            break;
        unz_file_info64 info;
        if (unzGetCurrentFileInfo64(zFile, &info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
            break;

        buff = new char[info.uncompressed_size];
        memset(buff, 0, info.uncompressed_size);
        int bytes;
        if ((bytes = unzReadCurrentFile(zFile, buff, info.uncompressed_size)) <= 0)
            break;
        unzCloseCurrentFile(zFile);
        unzClose(zFile);

        //write the data in buff to IStream
        IStream *pStream;
        if (CreateStreamOnHGlobal(NULL, true, &pStream) != S_OK)
            break;
        if (pStream->Write(buff, bytes, NULL) != S_OK)
        {
            pStream->Release();
            break;
        }
        delete[] buff;
        return pStream;

    } while (0);

    if (zFile)
    {
        unzCloseCurrentFile(zFile);
        unzClose(zFile);
    }
    if (buff)
        delete[] buff;
    return NULL;
}

////////////////////////////////////////////////////////////
namespace DroidPad
{

HDC CImageCache::m_hCachedImageDC[STATE_MAX_SIZE] = {NULL};

bool CImageCache::cacheDC(HDC hWinDC, int iWidth, int iHeight)
{
    for (int i = 0; i < STATE_MAX_SIZE; i++)
    {
        if (!(m_hCachedImageDC[i] = CreateCompatibleDC(hWinDC)))
            return false;

        HBITMAP bm;
        if (!(bm = CreateCompatibleBitmap(hWinDC, iWidth, iHeight)))
            return false;
        SelectObject(m_hCachedImageDC[i], bm);
        IStream *stream = _createStream(IME_STATUS_BAR_IMG_SOURCE[i]);
        if (!stream)
            return false;
        Gdiplus::Image img(stream);
        Gdiplus::Graphics g(m_hCachedImageDC[i]);
        g.DrawImage(&img, 0, 0, iWidth, iHeight);
        DeleteObject(bm);
        stream->Release();
    }
    return true;
}

void CImageCache::clean()
{
    for (int i = 0; i < STATE_MAX_SIZE; i++)
    {
        if (m_hCachedImageDC[i])
        {
            DeleteDC(m_hCachedImageDC[i]);
            m_hCachedImageDC[i] = NULL;
        }
    }
}

HDC CImageCache::getCachedDC(eBarState eState)
{
    return m_hCachedImageDC[eState];
}

} // namespace DroidPad