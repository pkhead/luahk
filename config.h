#ifndef LUAHK_CONFIG
#define LUAHK_CONFIG

#ifndef LUAHK_CONSOLE
#define TRAY_ICON
#endif

// the length in ms that the programs sleeps after a message has been dequeued
// used to prevent program from hogging cpu time
// is enabled by default, and can be turned off by os.throttle(false) in the script
constexpr int REFRESH_RATE = 8;

#endif