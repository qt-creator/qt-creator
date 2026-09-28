// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtcchannel.h"
#include "qtcload.h"

// What the shim hands the runner. The shim takes the channel as far as the Qt, because it
// has to map one before ArkTS goes on, and the runner reads the rest. The runner cannot
// reach the shim by dlopen: the platform loads napi modules into a linker namespace of their
// own, which hides them from it. So the shim puts the address of this into the environment
// under ShimVariable, and the runner takes it only if the version matches its own.
static const int ShimVersion = 1;
static const char *ShimVariable = "QTC_SHIM";

struct QtcShim
{
    int version = ShimVersion;
    QtcLoad::World *world = nullptr;
    bool (*load)(const char *, QtcLoad::Image *, std::string *, QtcLoad::World *, bool) = nullptr;
    bool (*registerPlugin)(const char *, QtcLoad::World *, std::string *) = nullptr;
    void *(*lookup)(const QtcLoad::Image &, const char *) = nullptr;
    int channel = -1;
    // A header the shim read that did not announce a Qt.
    bool headerPending = false;
    size_t header = 0;
    Launch *launch = nullptr;
};
