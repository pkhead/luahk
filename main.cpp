#include <lua.hpp>
#include <list>
#include <vector>
#include <sstream>
#include <string>
#include <Windows.h>

#include "config.h"

struct hotkey_up_config {
	UINT fsModifiers;
	UINT vk;
	int callback;
};

struct key_map {
	const char* key_name;
	int code;
};

static HWND _hwnd;
static lua_State* g_L;
static std::vector<hotkey_up_config> HotkeyUpListeners;

static int nextHotkeyId = 1;

const key_map KEY_MAP[] = {
	// mouse buttons
	{ "mouseleft", VK_LBUTTON },
	{ "mouseright", VK_RBUTTON },
	{ "mousemiddle", VK_MBUTTON },
	{ "mousex1", VK_XBUTTON1 },
	{ "mousex2", VK_XBUTTON2 },

	// letter buttons
	{ "0", 0x30 },
	{ "1", 0x31 },
	{ "2", 0x32 },
	{ "3", 0x33 },
	{ "4", 0x34 },
	{ "5", 0x35 },
	{ "6", 0x36 },
	{ "7", 0x37 },
	{ "8", 0x38 },
	{ "9", 0x39 },

	{ "a", 0x41 },
	{ "b", 0x42 },
	{ "c", 0x43 },
	{ "d", 0x44 },
	{ "e", 0x45 },
	{ "f", 0x46 },
	{ "g", 0x47 },
	{ "h", 0x48 },
	{ "i", 0x49 },
	{ "j", 0x4a },
	{ "k", 0x4b },
	{ "l", 0x4c },
	{ "m", 0x4d },
	{ "n", 0x4e },
	{ "o", 0x4f },
	{ "p", 0x50 },
	{ "q", 0x51 },
	{ "r", 0x52 },
	{ "s", 0x53 },
	{ "t", 0x54 },
	{ "u", 0x55 },
	{ "v", 0x56 },
	{ "w", 0x57 },
	{ "x", 0x58 },
	{ "y", 0x59 },
	{ "z", 0x5a },

	{ "plus", VK_OEM_PLUS },
	{ "comma", VK_OEM_COMMA },
	{ "minus", VK_OEM_MINUS },
	{ "period", VK_OEM_PERIOD },
	{ "lbracket", VK_OEM_4 }, { "lbrace", VK_OEM_4 },
	{ "rbracket", VK_OEM_6 }, { "rbrace", VK_OEM_6 },
	{ "backslash", VK_OEM_5 },
	{ "slash", VK_OEM_2 }, { "question", VK_OEM_2 },
	{ "semicolon", VK_OEM_1 }, { "colon", VK_OEM_1 },
	{ "quote", VK_OEM_7 },
	{ "backtick", VK_OEM_3 },

	{ "left", VK_LEFT },
	{ "right", VK_RIGHT },
	{ "up", VK_UP },
	{ "down", VK_DOWN },
	{ "enter", VK_RETURN }, { "return", VK_RETURN },

	{ "space", 32 },

	{ "caps", VK_CAPITAL },
	{ "shift", VK_SHIFT },
	{ "lshift", VK_LSHIFT },
	{ "rshift", VK_RSHIFT },
	{ "ctrl", VK_CONTROL },
	{ "lctrl", VK_LCONTROL },
	{ "rctrl", VK_RCONTROL },
	{ "win", VK_LWIN },
	{ "lwin", VK_LWIN },
	{ "rwin", VK_RWIN },

	{ "numpad0", VK_NUMPAD0 },
	{ "numpad1", VK_NUMPAD1 },
	{ "numpad2", VK_NUMPAD2 },
	{ "numpad3", VK_NUMPAD3 },
	{ "numpad4", VK_NUMPAD4 },
	{ "numpad5", VK_NUMPAD5 },
	{ "numpad6", VK_NUMPAD6 },
	{ "numpad7", VK_NUMPAD7 },
	{ "numpad8", VK_NUMPAD8 },
	{ "numpad9", VK_NUMPAD9 },

	{ "f1", VK_F1 },
	{ "f2", VK_F2 },
	{ "f3", VK_F3 },
	{ "f4", VK_F4 },
	{ "f5", VK_F5 },
	{ "f6", VK_F6 },
	{ "f7", VK_F7 },
	{ "f8", VK_F8 },
	{ "f9", VK_F9 },
	{ "f10", VK_F10 },
	{ "f11", VK_F11 },
	{ "f12", VK_F12 },
};

ULONGLONG start_tick;

#define _tick (GetTickCount64() - start_tick)

static int stringToVk(const char* str) {
	for (auto& map : KEY_MAP) {
		if (strcmp(str, map.key_name) == 0) {
			return map.code;
		}
	}

	return -1;
}

static const char* vkToCString(int vk) {
	for (auto& map : KEY_MAP) {
		if (map.code == vk) return map.key_name;
	}

	return nullptr;
}

static int stringToVk(const std::string& str) {
	for (auto& map : KEY_MAP) {
		if (str == map.key_name) {
			return map.code;
		}
	}

	return -1;
}

static void error(lua_State* L) {
	const char* msg = lua_tostring(L, -1);
	lua_pop(L, 1);

#ifdef LUAHK_CONSOLE
	// print error message in bright red
	fprintf(stderr, "\x1b[91m%s\n\x1b[0m", msg);
#else
	printf("error: %s\n", msg);
	//MessageBoxA(_hwnd, msg, "Luahk Error", MB_ICONERROR);
#endif
}

static void stackDump(lua_State* L) {
	int i;
	int top = lua_gettop(L);
	for (i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {

		case LUA_TSTRING:  /* strings */
			printf("`%s'", lua_tostring(L, i));
			break;

		case LUA_TBOOLEAN:  /* booleans */
			printf(lua_toboolean(L, i) ? "true" : "false");
			break;

		case LUA_TNUMBER:  /* numbers */
			printf("%g", lua_tonumber(L, i));
			break;

		default:  /* other values */
			printf("%s", lua_typename(L, t));
			break;

		}
		printf("  ");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}

#define LUAFUNC(name, L) static int name(lua_State* L)

LUAFUNC(_luaWarn, L) {
	int nargs = lua_gettop(L);

	// write orange color
	fprintf(stderr, "\x1b[33m");

	for (int i = 1; i <= nargs; i++) {
		const char* msg = lua_tostring(L, i);
		fprintf(stderr, "%s\t", msg);
	}

	fprintf(stderr, "\x1b[0m\n");

	return 0;
}

LUAFUNC(_luaTick, L) {
	lua_pushinteger(L, _tick);
	return 1;
}

LUAFUNC(_luaSetCursorPos, L) {
	lua_Number x = luaL_checknumber(L, 1);
	lua_Number y = luaL_checknumber(L, 2);

	if (!SetCursorPos((int)x, (int)y)) {
		luaL_error(L, "could not set cursor position");
	}

	return 0;
}

LUAFUNC(_luaGetCursorPos, L) {
	POINT pos;
	if (!GetCursorPos(&pos)) {
		luaL_error(L, "could not get cursor position");
		return 0;
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	return 2;
}

LUAFUNC(_luaMessageBox, L) {
	const char* text = lua_tostring(L, 1);

	MessageBoxA(_hwnd, text, "Luahk", MB_ICONINFORMATION);

	return 0;
}

char PromptLabel[256];
char PromptResult[256];

BOOL CALLBACK DeleteItemProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		SetDlgItemTextA(hwndDlg, 1002, PromptLabel);
		HWND input = GetDlgItem(hwndDlg, 1001);
		SetFocus(input);
		break;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (!GetDlgItemTextA(hwndDlg, 1001, PromptResult, 256)) {
				PromptResult[0] = '\0';
			}

			// fall through

		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}

	return FALSE;
}

extern std::string utf8_encode(const std::wstring& wstr);

LUAFUNC(_luaPrompt, L) {
	strcpy_s(PromptLabel, 256, "enter input");

	if (!lua_isnoneornil(L, 1)) {
		const char* str = lua_tostring(L, 1);

		if (str) {
			strcpy_s(PromptLabel, 256, str);
		}
	}

	SetForegroundWindow(_hwnd);

	if (DialogBoxA(NULL, (LPCSTR)MAKEINTRESOURCE(102), _hwnd, (DLGPROC)DeleteItemProc) == IDOK) {
		lua_pushstring(L, PromptResult);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

LUAFUNC(_luaMoveMouse, L) {
	lua_Integer dx = luaL_checkinteger(L, 1);
	lua_Integer dy = luaL_checkinteger(L, 2);

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = {
		(long)dx, (long)dy,
		0,
		MOUSEEVENTF_MOVE,
		0,
		NULL
	};

	SendInput(1, &input, sizeof(input));

	return 0;
}

LUAFUNC(_luaSleep, L) {
	int waitTime = luaL_checkinteger(L, 1);
	Sleep(waitTime);
	return 0;
}

LUAFUNC(_luaExit, L) {
	int code = 0;

	if (lua_isinteger(L, 1)) {
		code = lua_tointeger(L, 1);
	}

	PostQuitMessage(0);
	return 0;
}

LUAFUNC(_luaHotkey, L) {
	const char* input_str = luaL_checkstring(L, 1);
	int id = nextHotkeyId++;

	std::stringstream input(input_str);

	UINT modifiers = 0;
	std::string token;
	char ch = '\0';
	int vk = -1;
	bool keyup = false;
	bool parseKey = true;

	// parse key/modifier
	// valid hotkeys:
	// - "w"
	// - "ctrl+w"
	// - "w up"
	// - "ctrl+w up"
	// "up" must be the last character

	while (!input.eof()) {
		token.clear();

		while (true) {
			input.get(ch);
			if (input.eof() || isspace(ch) || ch == '+') break;
			token.push_back(ch);
		}

		// ch is the delimiter

		if (token == "up") {
			// if vk isn't defined, throw error (keywords must come after the keys)
			if (parseKey) {
				luaL_error(L, "expected key, got \"up\"");
				return 0;
			}

			keyup = true;

		} else if (token == "once") {
			// if vk isn't defined, throw error (keywords must come after the keys)
			if (parseKey) {
				luaL_error(L, "expected key, got \"once\"");
				return 0;
			}

			modifiers |= MOD_NOREPEAT;

		} else {
			if (token.empty()) {
				luaL_error(L, input.eof() ? "unexpected eof" : "unexpected +");
				return 0;
			}

			if (!parseKey) {
				luaL_error(L, "expected keyword or eof, got key");
			}

			if (token == "ctrl") {
				modifiers |= MOD_CONTROL;
			}
			else if (token == "alt") {
				modifiers |= MOD_ALT;
			}
			else if (token == "shift") {
				modifiers |= MOD_SHIFT;
			}
			else if (token == "win") {
				modifiers |= MOD_WIN;
			}
			else {
				bool success = false;
				int new_vk = stringToVk(token);

				// if key map doesn't exist
				if (new_vk < 0) {
					luaL_error(L, "unknown key %s", token.c_str());
					return 0;
				}

				// if vk is already defined, throw error
				if (vk >= 0) {
					luaL_error(L, "cannot have more than one key");
					return 0;
				}

				vk = new_vk;
			}
		}

		parseKey = !input.eof() && ch == '+';
	}

	if (vk < 0) {
		luaL_error(L, "key not specified");
	}

	if (keyup) {
		// reference the callback function
		lua_pushvalue(L, 2);
		int callback = luaL_ref(L, LUA_REGISTRYINDEX);

		hotkey_up_config config{
			modifiers,
			vk,
			callback
		};

		HotkeyUpListeners.push_back(config);
	}
	else {
		if (!RegisterHotKey(_hwnd, id, modifiers, vk)) {
			luaL_error(L, "could not register hot key");
			return 0;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, "OSHOTKEYS");
		lua_pushvalue(L, 2);
		lua_seti(L, -2, id);
	}

	return 0;
}

LUAFUNC(_luaBindUpdate, L) {
	if (!lua_isfunction(L, 1)) {
		luaL_typeerror(L, 1, "function");
		return 0;
	}

	lua_getfield(L, LUA_REGISTRYINDEX, "OSUPDATEBINDS");
	int tableLen = lua_rawlen(L, -1);
	lua_pushvalue(L, 1);
	lua_seti(L, -2, (lua_Integer)tableLen + 1);
	lua_pop(L, 2);

	return 0;
}

LUAFUNC(_luaSendInput, L) {
	int nargs = lua_gettop(L);
	if (nargs == 0) return 0;

	int res = 0;

	for (int i = 1; i <= nargs; i++) {
		std::stringstream skey(luaL_checkstring(L, i));

		std::string key;
		std::string status;

		// get key (left of space)
		if (!std::getline(skey, key, ' ')) {
			luaL_error(L, "expected a key name");
			return 0;
		}

		// get status (right of space)
		if (!std::getline(skey, status, ' ')) {
			luaL_error(L, "expected \"up\" or \"down\" after key name");
			return 0;
		}

		bool down;

		if (status == "up") {
			down = false;
		}
		else if (status == "down") {
			down = true;
		}
		else {
			luaL_error(L, "expected \"up\" or \"down\" after key name");
			return 0;
		}

		bool lbutton = key == "mouseleft";
		bool rbutton = key == "mouseright";
		bool mbutton = key == "mousemiddle";
		bool xbutton1 = key == "mousex1";
		bool xbutton2 = key == "mousex2";

		// mouse event?
		if (lbutton || rbutton || mbutton || xbutton1 || xbutton2) {
			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi = {
				0, // dx
				0, // dy

				// mouseData
				(DWORD)(xbutton1 ? XBUTTON1 : xbutton2 ? XBUTTON2 : NULL),

				// dwFlags
				(DWORD)(lbutton ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP) :
				(rbutton ? (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP) :
				(mbutton ? (down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP) :
				(xbutton1 || xbutton2 ? (down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP) : 0)
				))),

				0, // time
				NULL, // dwExtraInfo
			};

			res += SendInput(1, &input, sizeof(input));
		}

		// keyboard event
		else {
			int vk = stringToVk(key);

			if (vk == -1) {
				luaL_error(L, "unknown key %s", key);
				return 0;
			}

			INPUT input;
			input.type = INPUT_KEYBOARD;
			input.ki = {
				(WORD)vk, // wVk
				NULL, // wScan
				(DWORD)(down ? NULL : KEYEVENTF_KEYUP), // dwFlags
				0, // time
				NULL, // dwExtraInfo
			};

			res += SendInput(1, &input, sizeof(input));
		}
	}

	lua_pushinteger(L, res);
	return 1;
}

LUAFUNC(_luaSendText, L) {
	size_t strlen;
	const char* str = lua_tolstring(L, 1, &strlen);

	std::vector<INPUT> inputs;

	for (int i = 0; i < strlen; i++) {
		char ch = str[i];

		bool doShift = false;
		int vk;

#define CASE(upper,lower,v) case upper: doShift = true; case lower: vk = v; break;
		
		switch (ch) {
			CASE('~', '`', VK_OEM_3)
			CASE('!', '1', 0x31)
			CASE('@', '2', 0x32)
			CASE('#', '3', 0x33)
			CASE('$', '4', 0x34)
			CASE('%', '5', 0x35)
			CASE('^', '6', 0x36)
			CASE('&', '7', 0x37)
			CASE('*', '8', 0x38)
			CASE('(', '9', 0x39)
			CASE(')', '0', 0x30)
			CASE('_', '-', VK_OEM_MINUS)
			CASE('+', '=', VK_OEM_PLUS)
			CASE('{', '[', VK_OEM_4)
			CASE('}', ']', VK_OEM_6)
			CASE('|', '\\', VK_OEM_5)
			CASE(':', ';', VK_OEM_1)
			CASE('"', '\'', VK_OEM_7)
			CASE('<', ',', VK_OEM_COMMA)
			CASE('>', '.', VK_OEM_PERIOD)
			CASE('?', '/', VK_OEM_2)

		case ' ':
			vk = VK_SPACE;
			break;

		case '\n':
			vk = VK_RETURN;
			break;

		case '\t':
			vk = VK_TAB;
			break;

		default:
			// if uppercase
			if (ch > 64 && ch < 91) {
				doShift = true;
				vk = ch - 65 + 0x41;
			}

			// if lowercase
			else if (ch > 96 && ch < 123) {
				doShift = false;
				vk = ch - 97 + 0x41;
			}

			else {
				doShift = true;
				vk = VK_OEM_2;
			}

			break;
		}

#undef CASE

		INPUT input = {};
		input.type = INPUT_KEYBOARD;
		input.ki.time = 0;
		input.ki.wScan = 0;
		input.ki.dwExtraInfo = NULL;

		// send shift
		if (doShift) {
			input.ki.wVk = VK_LSHIFT;
			inputs.push_back(input);
		}

		// send key
		input.ki.wVk = vk;
		inputs.push_back(input);

		// release keys
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		inputs.push_back(input);

		if (doShift) {
			input.ki.wVk = VK_LSHIFT;
			inputs.push_back(input);
		}
	}

	if (!inputs.empty()) {
		SendInput(inputs.size(), &inputs[0], sizeof(INPUT));
	}

	return 0;
}

LUAFUNC(_luaSetClipboard, L) {
	size_t strlen;
	const char* content = lua_tolstring(L, 1, &strlen);

	if (OpenClipboard(NULL) == 0) {
		luaL_error(L, "could not open clipboard");
		return 0;
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen+1);
	if (hMem == NULL) {
		luaL_error(L, "could not allocate");
		return 0;
	}

	memcpy(GlobalLock(hMem), content, strlen + 1);
	GlobalUnlock(hMem);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();

	return 0;
}

LUAFUNC(_luaGetClipboard, L) {
	if (OpenClipboard(NULL) == 0) {
		luaL_error(L, "could not open clipboard");
		return 0;
	}

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == NULL) {
		luaL_error(L, "could not get clipboard data");
		return 0;
	}

	char* pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr) {
		luaL_error(L, "could not get clipboard data");
		return 0;
	}

	lua_pushstring(L, pszText);
	GlobalUnlock(hData);
	CloseClipboard();

	return 1;
}

LUAFUNC(_luaIsDown, L) {
	const char* vkStr = lua_tostring(L, 1);
	if (vkStr == nullptr) {
		luaL_typeerror(L, 1, "string");
		return 0;
	}

	int vk = stringToVk(vkStr);

	if (vk >= 0) {
		SHORT state = GetKeyState(vk);

		if (vk == VK_CAPITAL) {
			lua_pushboolean(L, (GetKeyState(vk) & 1) == 1);
		}
		else {
			lua_pushboolean(L, GetKeyState(vk) < 0);
		}

		return 1;
	}
}

/*
static int resumeThread(lua_State* L, int nargs) {
	int nres;
	int res;
	if ((res = lua_resume(L, g_L, nargs, &nres)) > LUA_YIELD) {
		error(L);
	}
	return res;
}
*/

static const luaL_Reg osLib[] = {
	{"tick", _luaTick},
	{"setcursor", _luaSetCursorPos},
	{"getcursor", _luaGetCursorPos},
	{"movecursor", _luaMoveMouse},
	{"sleep", _luaSleep},
	{"hotkey", _luaHotkey},
	{"send", _luaSendInput},
	{"sendtext", _luaSendText},
	{"exit", _luaExit},
	{"update", _luaBindUpdate},
	{"message", _luaMessageBox},
	{"prompt", _luaPrompt},
	{"setclipboard", _luaSetClipboard},
	{"getclipboard", _luaGetClipboard},
	{"isdown", _luaIsDown},
	{NULL, NULL},
};

extern const char* TASK_LIB_SRC;
extern const char* HOTKEY_SRC;

LUAFUNC(luaErrHandler, L) {
	printf("error: %s\n", lua_tostring(L, 1));

	lua_getglobal(L, "debug");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "traceback");

		if (lua_isfunction(L, -1)) {
			if (!lua_pcall(L, 0, 1, NULL)) {
				const char* traceback = lua_tostring(L, -1);

				printf("%s\n", traceback);
			}
		}
	}

	return 0;
}

int luacall(lua_State* L, int nargs, int nres) {
	int hpos = lua_gettop(L) - nargs;
	int ret = 0;
	lua_pushcfunction(L, luaErrHandler);
	lua_insert(L, hpos);
	ret = lua_pcall(L, nargs, nres, hpos);
	lua_remove(L, hpos);
	return ret;
}

LUAFUNC(_luaLoadTask, L) {
	if (luaL_loadstring(L, TASK_LIB_SRC)) {
		lua_error(L);
		return 0;
	}

	luacall(L, 0, 1);

	return 1;
}

void APP_SETUP(HWND hwnd, int argc, const char* argv[]) {
	start_tick = GetTickCount64();

	lua_State* L = luaL_newstate();
	g_L = L;
	luaL_openlibs(L);

	_hwnd = hwnd;

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "OSHOTKEYS");

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "OSHOTKEYUP");

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "OSUPDATEBINDS");

	lua_pushcfunction(L, _luaWarn);
	lua_setglobal(L, "warn");

	// preload task library
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, _luaLoadTask);
	lua_setfield(L, -2, "task");
	lua_pop(L, 2);
	
	// define os library
	lua_getglobal(L, "os");
	for (int i = 0; osLib[i].func != NULL; i++) {
		lua_pushstring(L, osLib[i].name);
		lua_pushcfunction(L, osLib[i].func);
		lua_settable(L, -3);
	}
	lua_pop(L, 1);

	if (argc > 1) {
		if (luaL_loadfile(L, argv[1])) {
			error(L);
			return;
		}

		lua_newtable(L);

		// load arguments
		for (int i = 2; i < argc; i++) {
			lua_pushstring(L, argv[i]);
			lua_seti(L, -2, (lua_Integer)i - 1);
		}

		lua_setglobal(L, "args");

		/*
		// load hotkey function
		if (luaL_loadbuffer(L, HOTKEY_SRC, strlen(HOTKEY_SRC), "hotkey") || lua_pcall(L, 0, 0, NULL)) {
			error(L);
			PostQuitMessage(-1);
			return;
		}
		*/

		luacall(L, 0, 0);
	}
	else {
#ifdef LUAHK_CONSOLE
		puts("No input files");
#else
		MessageBoxW(_hwnd, L"No input files", L"Luahk", MB_ICONWARNING);
#endif
		PostQuitMessage(-1);
	}
}

void APP_UPDATE() {
	lua_State* L = g_L;

	lua_getfield(L, LUA_REGISTRYINDEX, "OSUPDATEBINDS");
	lua_pushnil(L);

	while (lua_next(L, -2)) {
		if (luacall(L, 0, 0)) break;
	}

	lua_pop(L, 1);

	/*
	lua_getglobal(L, "os");
	lua_getfield(L, -1, "update");

	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, NULL)) error(L);
	}
	else {
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	*/
}

void APP_HOTKEY(int id) {
	lua_State* L = g_L;

	lua_getfield(L, LUA_REGISTRYINDEX, "OSHOTKEYS");
	lua_geti(L, -1, id);

	luacall(L, 0, 0);
}

void APP_KBHOOK(int state, int vkCode, bool isRepeat) {
	lua_State* L = g_L;

	// if up
	if (state == 1) {
		for (auto& config : HotkeyUpListeners) {
			if (vkCode == config.vk) {
				// if all the specified modifier keys are down
				// ctrl, alt, shift, win
				if (
					((config.fsModifiers & MOD_CONTROL) != MOD_CONTROL || GetKeyState(VK_CONTROL) < 0) &&
					((config.fsModifiers & MOD_ALT) != MOD_ALT || GetKeyState(VK_MENU) < 0) &&
					((config.fsModifiers & MOD_SHIFT) != MOD_SHIFT || GetKeyState(VK_SHIFT) < 0) &&
					((config.fsModifiers & MOD_WIN) != MOD_WIN || GetKeyState(VK_LWIN) < 0)
					) {
					// get and call callback function
					lua_rawgeti(L, LUA_REGISTRYINDEX, config.callback);
					luacall(L, 0, 0);

					break;
				}
			}
		}
	}

	/*
	// run os.kbhook(state, vkCode, isRepeat), if "os" is a table and "kbhook" is a function
	lua_getglobal(L, "os");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "kbhook");

		if (lua_isfunction(L, -1)) {
			// first argument is false (up) or down (true)
			lua_pushboolean(L, state == 0);

			// second argument is the keycode as a string
			const char* vkStr = vkToCString(vkCode);
			if (vkStr == nullptr) {
				lua_pushstring(L, "");
			}
			else {
				lua_pushstring(L, vkStr);
			}

			// call function
			if (lua_pcall(L, 2, 0, NULL)) error(L);
		}
	}
	*/
}

void APP_END() {
	lua_State* L = g_L;

	lua_getfield(L, LUA_REGISTRYINDEX, "OSHOTKEYS");
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		// discard value
		lua_pop(L, 1);
		UnregisterHotKey(_hwnd, lua_tointeger(L, -1));
	}

	lua_close(g_L);
}