// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <QDir>

namespace Lua::Internal {

void setupQtModule()
{
    registerProvider("Qt", [](sol::state_view lua) {
        sol::table qt(lua, sol::create);

        // clang-format off
        qt["TextElideMode"] = lua.create_table_with(
            "ElideLeft", Qt::ElideLeft,
            "ElideRight", Qt::ElideRight,
            "ElideMiddle", Qt::ElideMiddle,
            "ElideNone", Qt::ElideNone
        );

        qt["QDirIterator"] = lua.create_table_with(
            "IteratorFlag", lua.create_table_with(
                "NoIteratorFlags", QDirIterator::NoIteratorFlags,
                "FollowSymlinks", QDirIterator::FollowSymlinks,
                "Subdirectories", QDirIterator::Subdirectories
            )
        );

        qt["QDir"] = lua.create_table_with(
            // QDir::Filters
            "Filters", lua.create_table_with(
                "Dirs", QDir::Dirs,
                "Files", QDir::Files,
                "Drives", QDir::Drives,
                "NoSymLinks", QDir::NoSymLinks,
                "AllEntries", QDir::AllEntries,
                "TypeMask", QDir::TypeMask,
                "Readable", QDir::Readable,
                "Writable", QDir::Writable,
                "Executable", QDir::Executable,
                "PermissionMask", QDir::PermissionMask,
                "Modified", QDir::Modified,
                "Hidden", QDir::Hidden,
                "System", QDir::System,
                "AccessMask", QDir::AccessMask,
                "AllDirs", QDir::AllDirs,
                "CaseSensitive", QDir::CaseSensitive,
                "NoDot", QDir::NoDot,
                "NoDotDot", QDir::NoDotDot,
                "NoDotAndDotDot", QDir::NoDotAndDotDot,
                "NoFilter", QDir::NoFilter
            ),

            // QDir::SortFlag
            "SortFlags", lua.create_table_with(
                "Name", QDir::Name,
                "Time", QDir::Time,
                "Size", QDir::Size,
                "Unsorted", QDir::Unsorted,
                "SortByMask", QDir::SortByMask,
                "DirsFirst", QDir::DirsFirst,
                "Reversed", QDir::Reversed,
                "IgnoreCase", QDir::IgnoreCase,
                "DirsLast", QDir::DirsLast,
                "LocaleAware", QDir::LocaleAware,
                "Type", QDir::Type,
                "NoSort", QDir::NoSort
            )
        );
        // clang-format on

        return qt;
    });
}

} // namespace Lua::Internal
