// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Starts qtprofiler-cli on Windows, where nothing like an rpath tells the
// loader that the Profiler plugin and the plugins it needs are in the plugin
// directory. The tool is a library next to this program, loaded once that
// directory is on the search path.

#include <windows.h>

#include <cstdio>
#include <string>

int main(int argc, char *argv[])
{
    std::wstring directory(MAX_PATH, L'\0');
    DWORD length = 0;
    while ((length = GetModuleFileNameW(nullptr, directory.data(), DWORD(directory.size())))
           == directory.size()) {
        directory.resize(directory.size() * 2);
    }
    directory.resize(length);
    directory.resize(directory.find_last_of(L"\\/"));

    // Only while the tool is loaded: what it launches gets the default search path.
    SetDllDirectoryW((directory + L"\\" RELATIVE_PLUGIN_PATH).c_str());
    const HMODULE tool = LoadLibraryW((directory + L"\\" QTPROFILERCLI_DLL).c_str());
    const DWORD loadError = GetLastError();
    SetDllDirectoryW(nullptr);
    if (!tool) {
        std::fprintf(stderr, "qtprofiler-cli: cannot load %s (error %lu)\n", QTPROFILERCLI_DLL,
                     loadError);
        return 1;
    }

    using Main = int (*)(int, char *[]);
    const auto run = reinterpret_cast<Main>(GetProcAddress(tool, "qtProfilerCliMain"));
    if (!run) {
        std::fprintf(stderr, "qtprofiler-cli: %s has no entry point\n", QTPROFILERCLI_DLL);
        return 1;
    }
    return run(argc, argv);
}
