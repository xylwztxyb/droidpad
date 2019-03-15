#ifndef mime_h__
#define mime_h__

#include "common.h"
#include "win32/immdev.h"
#include "remote_client.h"

#ifdef MIME_EXPORTS
#define MIME_API __declspec(dllexport)
#else
#define MIME_API __declspec(dllimport)
#endif

using namespace DroidPad;

#define MIME_UI_CLASS_NAME TEXT("mime_ui_class")
#define MAX_COMPOSITION_SIZE 256

extern CRemoteClient *gRemoteClient;

LRESULT WINAPI UIWndProc(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp);
int registerUIClass(HINSTANCE instance);
void unregisterUIClass(HINSTANCE instance);

#endif // mime_h__