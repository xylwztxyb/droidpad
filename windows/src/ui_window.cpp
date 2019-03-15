#include "common.h"
#include "mime.h"
#include "resource.h"

int registerUIClass(HINSTANCE instance) {
	WNDCLASSEX ui_class;

	ui_class.cbSize			= sizeof(WNDCLASSEX);
	ui_class.style			= CS_IME | CS_VREDRAW | CS_HREDRAW;
	ui_class.cbClsExtra		= 0;
	ui_class.cbWndExtra		= sizeof(LONG_PTR) * 2;
	ui_class.hIcon			= 0;
	ui_class.hIconSm		= 0;
	ui_class.hInstance		= instance;
	ui_class.hCursor		= NULL;
	ui_class.hbrBackground	= NULL;
	ui_class.lpszMenuName	= NULL;
	ui_class.lpfnWndProc	= UIWndProc;
	ui_class.lpszClassName	= MIME_UI_CLASS_NAME;

	if (!RegisterClassEx(&ui_class))
		return 0;

	return 1;
}

void unregisterUIClass(HINSTANCE instance) {
	UnregisterClass(MIME_UI_CLASS_NAME, instance);
}