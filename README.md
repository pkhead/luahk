# LuaHK
A Windows application that allows user input simulation and monitoring with the Lua 5.4 programming language.

Run "luahk.exe" with a Lua file. Afterwards, you can interact with the process via the taskbar icon.
"luahkc.exe" is the command-line alternative and does not create the taskbar icon.

# Compilation

Visual Studio 2019 or above is required.

Only 64-bit builds are configured properly. There are four configurations:

- Debug
- Debug Console
- Release
- Release with Console

Debug and Release correspond to "luahk.exe", and Debug Console and Release with Console correspond to "luahkc.exe"

# Documentation
`void os.tick()`

Returns the time that has passed since the application started.

`void os.setcursor(number x, number y)`

Moves the cursor to the specified `x` and `y` coordinates on screen.

`number, number os.getcursor()`

Returns the x and y position of the cursor.

`void os.sleep(number ms)`

Sleeps for the specified milliseconds.

`void os.hotkey(string hotkey, function callback)`

Registers a hotkey for a callback.

The hotkey is formatted such that it starts with a key combination, with each key separated by a "+", and keywords specified after.

The supported keywords are as follows:
|Keyword|Function|
|-------|--------|
|`up`|Fires the callback when the hotkey is released|
|`once`|Does not repeat the callback when it is held down|

`number os.send(string inputs...)`

Sends keyboard/mouse inputs. Each input should be have the key name be first, and "down" or "up" after a space.

Returns the number of inputs that were successfully sent.

`void os.sendtext(string text)`

Sends inputs that writes `text`

`void os.update(function callback)`

Begins running the `callback` function whenever the message queue is dequeued.

`void os.message(string text)`

Shows a message box.

`string|nil os.prompt(string text)`

Shows a message box with a text input. Returns the text the user entered when they press "Ok" or Enter, and `nil` when the user presses "Cancel".

`void os.setclipboard(string contents)`

Sets the clipboard to the string contents.

`string os.getclipboard()`

Returns the user's text clipboard

`boolean os.isdown(string key)`

Returns true if the key is down, and false if it isn't.

## Task Library

The runtime comes with a library that allows non-blocking sleep.

`void task.spawn(function func, arguments...)`

Automatically creates and resumes a coroutine with `func` and passes the `arguments` given into the coroutine.

`number task.wait(number ms)`

Yields the coroutine, and resumes it after `ms` milliseconds. Returns the amount of time that has passed in milliseconds.