// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../luaengine.h"

#include <utils/filepath.h>
#include <utils/icon.h>

#include <QMetaEnum>

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

using IconFilePathOrString = std::variant<std::shared_ptr<Utils::Icon>, Utils::FilePath, QString>;

inline std::shared_ptr<Utils::Icon> toIcon(const IconFilePathOrString &v)
{
    return std::visit(
        [](auto &&arg) -> std::shared_ptr<Utils::Icon> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::shared_ptr<Utils::Icon>>)
                return arg;
            else
                return std::make_shared<Utils::Icon>(toFilePath(arg));
        },
        v);
}

inline void mirrorEnum(sol::table &target, QMetaEnum metaEnum, const QString &name = {})
{
    sol::table widgetAttributes = target.create(
        name.isEmpty() ? QString::fromUtf8(metaEnum.name()) : name, metaEnum.keyCount());
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        widgetAttributes.set(metaEnum.key(i), metaEnum.value(i));
};

template <typename E>
inline QFlags<E> tableToFlags(const sol::table &table) noexcept {
    static_assert(std::is_enum<E>::value, "Enum type required");

    QFlags<E> flags;
    for (const auto& kv : table)
        flags.setFlag(static_cast<E>(kv.second.as<int>()));

    return flags;
}

} // namespace Lua::Internal
