// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <windows.h>

#include <cstdio>

struct LibProbe;
struct SecondProbe;

struct AppProbe { int appValue; };

int probeStorage = 4711;
int secondStorage = 8822;
AppProbe appStorage = {12};

LibProbe *libProbe = (LibProbe *) &probeStorage;
SecondProbe *pointerToSecond = (SecondProbe *) &secondStorage;
AppProbe *appProbe = &appStorage;

int libraryLoaded = 0;

extern "C" void afterLoad()
{
    printf("loaded %d\n", libraryLoaded);
    fflush(stdout);
}

int main()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    *(wcsrchr(path, L'\\') + 1) = 0;
    lstrcatW(path, L"problib.dll");
    libraryLoaded = LoadLibraryW(path) != nullptr;
    afterLoad();
    return 0;
}
