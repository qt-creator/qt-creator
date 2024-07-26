// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/generatedfile.h>

using namespace Core;

namespace Lua::Internal {

void setupCoreModule()
{
    registerProvider("Core", [](sol::state_view lua) -> sol::object {
        sol::table core = lua.create_table();

        auto generatedFileType = core.new_usertype<GeneratedFile>(
            "GeneratedFile",
            "filePath",
            sol::property(&GeneratedFile::filePath, &GeneratedFile::setFilePath),
            "contents",
            sol::property(&GeneratedFile::contents, &GeneratedFile::setContents),
            "attributes",
            sol::property([](GeneratedFile *f) -> int { return f->attributes().toInt(); },
                          [](GeneratedFile *f, int flags) {
                              f->setAttributes(GeneratedFile::Attributes::fromInt(flags));
                          }),
            "isBinary",
            sol::property(&GeneratedFile::isBinary, &GeneratedFile::setBinary));

        // clang-format off
        generatedFileType["Attribute"] = lua.create_table_with(
            "OpenEditorAttribute", GeneratedFile::OpenEditorAttribute,
            "OpenProjectAttribute", GeneratedFile::OpenProjectAttribute,
            "CustomGeneratorAttribute", GeneratedFile::CustomGeneratorAttribute,
            "KeepExistingFileAttribute", GeneratedFile::KeepExistingFileAttribute,
            "ForceOverwrite", GeneratedFile::ForceOverwrite,
            "TemporaryFile", GeneratedFile::TemporaryFile
        );
        // clang-format on

        return core;
    });
}

} // namespace Lua::Internal
