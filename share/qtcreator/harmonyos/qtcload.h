// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <elf.h>

namespace QtcLoad {

class Image
{
public:
    std::string name;                    // the file name the image was mapped under
    std::string path;                    // the file it was read from, for dladdr()
    char *base = nullptr;
    size_t size = 0;
    Elf64_Addr lowest = 0;
    const Elf64_Sym *symbols = nullptr;
    size_t symbolCount = 0;
    const char *strings = nullptr;
    std::unordered_map<std::string, void *> exports; // what .dynsym defines, by name
    std::vector<Elf64_Sym> localSymbols; // .symtab, kept for what .dynsym does not export
    std::string localStrings;
    std::vector<void *> dependencies;    // handles kept for the image's lifetime
    std::vector<std::string> missing;    // DT_NEEDED entries the platform would not load
    bool framesRegistered = false;
    size_t frameCount = 0;
};

// The images the loader mapped itself, and the directories that make it do so. A library
// the application wrote is one the platform refuses to load, so a Qt that arrived over the
// channel has to go through this loader as much as the application does: name the directory
// it was written to, and every DT_NEEDED entry found there is mapped here rather than
// dlopened, with the whole set resolving against itself.
class World
{
public:
    std::vector<std::string> directories;
    std::vector<std::unique_ptr<Image>> images; // in the order they were mapped

    Image *find(const std::string &name) const;
    // The file a DT_NEEDED entry names, in the directories above, or empty.
    std::string locate(const char *name) const;
};

bool load(const char *path, Image *image, std::string *error, World *world = nullptr);
void *lookup(const Image &image, const char *name);

// Whether the platform has a library of this name to offer: one that is loaded already, or
// one installed beside it. What it says no to is what a caller has to bring itself.
bool installed(const char *name);

} // namespace QtcLoad
