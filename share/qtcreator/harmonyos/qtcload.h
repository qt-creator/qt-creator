// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <elf.h>

namespace QtcLoad {

class Image
{
public:
    char *base = nullptr;
    size_t size = 0;
    Elf64_Addr lowest = 0;
    const Elf64_Sym *symbols = nullptr;
    size_t symbolCount = 0;
    const char *strings = nullptr;
    std::vector<void *> dependencies;    // handles kept for the image's lifetime
    std::vector<std::string> missing;    // DT_NEEDED entries the platform would not load
    bool framesRegistered = false;
    size_t frameCount = 0;
};

bool load(const char *path, Image *image, std::string *error);
void *lookup(const Image &image, const char *name);

} // namespace QtcLoad
