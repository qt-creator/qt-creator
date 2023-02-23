#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "TestUtil.cc"

static void usage() {
    printf("usage: SetBufInfo [-set] [-buf W H] [-win W H] [-pos X Y]\n");
}

int main(int argc, char *argv[]) {
    const HANDLE conout = CreateFileW(L"CONOUT$",
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING, 0, NULL);
    ASSERT(conout != INVALID_HANDLE_VALUE);

    bool change = false;
    BOOL success;
    CONSOLE_SCREEN_BUFFER_INFOEX info = {};
    info.cbSize = sizeof(info);

    success = GetConsoleScreenBufferInfoEx(conout, &info);
    ASSERT(success && "GetConsoleScreenBufferInfoEx failed");

    for (int i = 1; i < argc; ) {
        std::string arg = argv[i];
        if (arg == "-buf" && (i + 2) < argc) {
            info.dwSize.X = atoi(argv[i + 1]);
            info.dwSize.Y = atoi(argv[i + 2]);
            i += 3;
            change = true;
        } else if (arg == "-pos" && (i + 2) < argc) {
            int dx = info.srWindow.Right - info.srWindow.Left;
            int dy = info.srWindow.Bottom - info.srWindow.Top;
            info.srWindow.Left = atoi(argv[i + 1]);
            info.srWindow.Top = atoi(argv[i + 2]);
            i += 3;
            info.srWindow.Right = info.srWindow.Left + dx;
            info.srWindow.Bottom = info.srWindow.Top + dy;
            change = true;
        } else if (arg == "-win" && (i + 2) < argc) {
            info.srWindow.Right = info.srWindow.Left + atoi(argv[i + 1]) - 1;
            info.srWindow.Bottom = info.srWindow.Top + atoi(argv[i + 2]) - 1;
            i += 3;
            change = true;
        } else if (arg == "-set") {
            change = true;
            ++i;
        } else if (arg == "--help" || arg == "-help") {
            usage();
            exit(0);
        } else {
            fprintf(stderr, "error: unrecognized argument: %s\n", arg.c_str());
            usage();
            exit(1);
        }
    }

    if (change) {
        success = SetConsoleScreenBufferInfoEx(conout, &info);
        if (success) {
            printf("success\n");
        } else {
            printf("SetConsoleScreenBufferInfoEx call failed\n");
        }
        success = GetConsoleScreenBufferInfoEx(conout, &info);
        ASSERT(success && "GetConsoleScreenBufferInfoEx failed");
    }

    auto dump = [](const char *fmt, ...) {
        char msg[256];
        va_list ap;
        va_start(ap, fmt);
        vsprintf(msg, fmt, ap);
        va_end(ap);
        trace("%s", msg);
        printf("%s\n", msg);
    };

    dump("buffer-size: %d x %d", info.dwSize.X, info.dwSize.Y);
    dump("window-size: %d x %d", 
        info.srWindow.Right - info.srWindow.Left + 1,
        info.srWindow.Bottom - info.srWindow.Top + 1);
    dump("window-pos:  %d, %d", info.srWindow.Left, info.srWindow.Top);

    return 0;
}
