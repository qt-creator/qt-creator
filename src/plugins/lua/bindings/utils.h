// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Lua::Internal {

using FilePathOrString = std::variant<Utils::FilePath, QString>;

inline Utils::FilePath toFilePath(const FilePathOrString &v)
{
    return std::visit(
        [](auto &&arg) -> Utils::FilePath {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, QString>)
                return Utils::FilePath::fromUserInput(arg);
            else
                return arg;
        },
        v);
}

} // namespace Lua::Internal
