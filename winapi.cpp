#ifndef _FILE_DEFINED
// this is set in order to redirect stdout properly
struct __iobuf {
	char* _ptr;
	int _cnt;
	char* _base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char* _tmpfname;
};
typedef struct __iobuf FILE_COMPLETE;
//#define _FILE_DEFINED
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <windowsx.h>
#include <locale>
#include <codecvt>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <strsafe.h>
#include <string>
#include "config.h"

#define MENU_QUIT 1000
#define MENU_SHOWCONSOLE 1001

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT keyboardProc(int code, WPARAM wParam, LPARAM lParam);

// these are defined in main.cpp
extern bool APP_THROTTLING;
extern void APP_SETUP(HWND hwnd, int argc, const char* argv[]);
extern void APP_END();
extern void APP_UPDATE();
extern void APP_HOTKEY(int id);
extern void APP_KBHOOK(int state, int keycode, bool repeat);

std::string utf8_encode(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

static HINSTANCE _hInstance;
static HANDLE Stdout_r = NULL;
static HANDLE Stdout_w = NULL;
static int _stdoutfd = -1;
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UnicodeConverter;

const wchar_t WINDOW_CLASS_NAME[] = L"LuaHK window";

#ifdef LUAHK_CONSOLE
// if building as a console application
int main(int argc, const char* argv[]) {
	HINSTANCE hInstance = GetModuleHandle(0);
#else
// if building as a Windows application
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd) {
	_hInstance = hInstance;

	// parse arguments into argc and argv
	int argc;
	LPWSTR* _wArgv = CommandLineToArgvW(lpCmdLine, &argc);
	const char** argv = NULL;

	if (argc > 0) {
		argv = new const char* [argc+1];

		argv[0] = "luahk";

		for (int i = 0; i < argc; i++) {
			std::string* str = new std::string(utf8_encode(_wArgv[i]));
			argv[i + 1] = str->c_str();
		}

		argc += 1;
	}

	{
		// redirect stdout to a pipe
		SECURITY_ATTRIBUTES saAttr = { };
		saAttr.nLength = sizeof(saAttr);
		saAttr.lpSecurityDescriptor = NULL;
		saAttr.bInheritHandle = TRUE;

		if (!CreatePipe(&Stdout_r, &Stdout_w, &saAttr, 0)) {
			MessageBox(NULL, L"Couldn't create pipe", L"Error", MB_ICONERROR);
			return -1;
		}

		SetStdHandle(STD_OUTPUT_HANDLE, Stdout_w);
		int fd = _open_osfhandle((intptr_t)Stdout_w, O_WRONLY | O_TEXT);
		if (_dup2(fd, 1) != 0) {
			MessageBox(NULL, L"Couldn't _dup2", L"Error", MB_ICONERROR);
			return -1;
		}

		*(FILE_COMPLETE*)stdout = *(FILE_COMPLETE*)_fdopen(fd, "w");
		setvbuf(stdout, NULL, _IONBF, 0);

		_stdoutfd = fd;
	}
#endif
	// register window classes
	{
		WNDCLASS wc = { };

		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = WINDOW_CLASS_NAME;
		RegisterClass(&wc);
	}

	if (argc < 2) {
#ifdef LUAHK_CONSOLE
		printf("error: no input files specified\n");
#else
		MessageBoxA(NULL, "No input files specified", "Error", MB_ICONERROR);
#endif
		return -1;
	}

	// create window
	std::wstring windowName = L"LuaHK - ";
	windowName += UnicodeConverter.from_bytes(argv[1]);

	HWND hwnd = CreateWindowEx(
		0,
		WINDOW_CLASS_NAME,
		windowName.c_str(),
		WS_OVERLAPPEDWINDOW,

		CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,

		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) {
		return -1;
	}
	
	ShowWindow(hwnd, SW_HIDE);

	APP_SETUP(hwnd, argc, argv);

	#ifdef TRAY_ICON
	// register tray icon
	NOTIFYICONDATA nid;

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
	nid.uVersion = NOTIFYICON_VERSION_4;
	nid.uCallbackMessage = WM_APP + 1;
	nid.hIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(101));
	StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"LuaHK");

	Shell_NotifyIcon(NIM_ADD, &nid);
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
	#endif

	HHOOK hhook = SetWindowsHookExA(WH_KEYBOARD_LL, keyboardProc, NULL, 0);
	
	if (hhook == NULL) {
		printf("could not create hook\n");
	}

	// run message loop

	MSG msg = { };

	char console_buf[64];
	DWORD size, read;

	while (true) {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) goto exit_program;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		APP_UPDATE();

#ifndef LUAHK_CONSOLE
		if (PeekNamedPipe(Stdout_r, 0, 0, 0, &size, 0) != FALSE && size > 0) {
			if (ReadFile(Stdout_r, console_buf, 64, &read, NULL)) {
				SendMessage(hwnd, WM_APP + 2, (WPARAM)console_buf, (LPARAM)read);
			}
		}
#endif

		if (APP_THROTTLING) {
			Sleep(REFRESH_RATE);
		}
	}

	exit_program:

	#ifdef TRAY_ICON
	Shell_NotifyIcon(NIM_DELETE, &nid);
	#endif

	if (hhook) {
		UnhookWindowsHookEx(hhook);
	}

	APP_END();

	return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND editBox;

	switch (uMsg) {

#ifndef LUAHK_CONSOLE
	case WM_CREATE:
	{
		// create EDIT control used for displaying stdout
		RECT rcClient;
		GetClientRect(hwnd, &rcClient);

		editBox = CreateWindowEx(
			0,
			L"EDIT",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
			0, 0, rcClient.right, rcClient.bottom,
			hwnd,
			NULL,
			GetModuleHandle(0),
			NULL
		);
		if (editBox == NULL) {
			printf("error!\n");
			return 0;
		}

		// create font
		HFONT hf;
		HDC hdc = GetDC(NULL);
		long lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);

		hf = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Consolas");
		SendMessage(editBox, WM_SETFONT, (WPARAM)hf, TRUE);

		return 0;
	}

	// tray icon interaction
	case WM_APP + 1:
		switch (lParam) {
		case WM_LBUTTONDOWN:
			SendMessage(hwnd, WM_COMMAND, (WPARAM)MENU_SHOWCONSOLE, NULL);
			break;

		case WM_RBUTTONDOWN:
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, MENU_SHOWCONSOLE, L"Logs");
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, MENU_QUIT, L"Quit");
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), 0, hwnd, NULL);
			break;
		}

		return 0;

	// message sent whenever there is a new message in stdout
	case WM_APP + 2:
	{
		std::wstring wide = UnicodeConverter.from_bytes((char*)wParam, (char*)wParam + (size_t)lParam);
		const wchar_t* wide_c = wide.c_str();

		// get the current selection
		DWORD StartPos, EndPos;
		SendMessage(editBox, EM_GETSEL, reinterpret_cast<WPARAM>(&StartPos), reinterpret_cast<WPARAM>(&EndPos));

		// move the caret to the end of the text
		int outLength = GetWindowTextLength(editBox);
		SendMessage(editBox, EM_SETSEL, outLength, outLength);

		// insert the text at the new caret position
		SendMessage(editBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(wide_c));

		// restore the previous selection
		SendMessage(editBox, EM_SETSEL, StartPos, EndPos);
	}

	case WM_COMMAND:
		switch (wParam) {
		case MENU_QUIT:
			DestroyWindow(hwnd);
			return 0;
		case MENU_SHOWCONSOLE:
		{
			ShowWindow(hwnd, SW_SHOW);
		} return 0;
		}
		break;

	case WM_SIZE:
		MoveWindow(editBox,
			0, 0,
			LOWORD(lParam),
			HIWORD(lParam),
			TRUE
		);
		return 0;

	case WM_CLOSE:
		ShowWindow(hwnd, SW_HIDE);
		return 0;
#endif
	
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_HOTKEY:
		APP_HOTKEY((int)wParam);

		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// keyboardProc for the hook
LRESULT keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if (code >= 0) {
		int state = 0; // 0 = down, 1 = up

		switch (wParam)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			state = 0;
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			state = 1;
			break;
		}

		LPKBDLLHOOKSTRUCT info = (LPKBDLLHOOKSTRUCT) lParam;

		APP_KBHOOK(state, info->vkCode, (info->flags & KF_REPEAT) == KF_REPEAT);
	}

	return CallNextHookEx(NULL, code, wParam, lParam);
}