// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "utils.h"

#include <QApplication>
#include <QClipboard>
#include <QCompleter>
#include <QDir>
#include <QFileDevice>
#include <QStandardPaths>

namespace Lua::Internal {

void setupQtModule()
{
    registerProvider("Qt", [](sol::state_view lua) {
        sol::table qt(lua, sol::create);
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");

        qt.new_usertype<QCompleter>(
            "QCompleter",
            "create",
            [](const QStringList &list) -> std::unique_ptr<QCompleter> {
                return std::make_unique<QCompleter>(list);
            },
            "currentCompletion",
            &QCompleter::currentCompletion,
            "completionMode",
            sol::property(
                &QCompleter::completionMode,
                [](QCompleter *c, QCompleter::CompletionMode mode) { c->setCompletionMode(mode); }),
            "onActivated",
            sol::property([guard = pluginSpec](QCompleter &obj, sol::function callback) {
                QObject::connect(
                    &obj,
                    QOverload<const QString &>::of(&QCompleter::activated),
                    guard->connectionGuard.get(),
                    [callback](const QString &arg) { void_safe_call(callback, arg); });
            }));

        qt.new_usertype<QClipboard>(
            "QClipboard",
            sol::no_constructor,
            "text",
            sol::property(
                [](QClipboard &self) { return self.text(); },
                [](QClipboard &self, const QString &value) { self.setText(value); }));

        qt["clipboard"] = &QApplication::clipboard;

        mirrorEnum(qt, QMetaEnum::fromType<QCompleter::CompletionMode>(), "QCompleterCompletionMode");

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

        qt["QFileDevice"] = lua.create_table_with(
            "Permission", lua.create_table_with(
                "ReadOwner", QFileDevice::ReadOwner,
                "ReadUser", QFileDevice::ReadUser,
                "ReadGroup", QFileDevice::ReadGroup,
                "ReadOther", QFileDevice::ReadOther,
                "WriteOwner", QFileDevice::WriteOwner,
                "WriteUser", QFileDevice::WriteUser,
                "WriteGroup", QFileDevice::WriteGroup,
                "WriteOther", QFileDevice::WriteOther,
                "ExeOwner", QFileDevice::ExeOwner,
                "ExeUser", QFileDevice::ExeUser,
                "ExeGroup", QFileDevice::ExeGroup,
                "ExeOther", QFileDevice::ExeOther
            )
        );

        qt["QStandardPaths"] = lua.create_table_with(
            "StandardLocation", lua.create_table_with(
                "DesktopLocation", QStandardPaths::DesktopLocation,
                "DocumentsLocation", QStandardPaths::DocumentsLocation,
                "FontsLocation", QStandardPaths::FontsLocation,
                "ApplicationsLocation", QStandardPaths::ApplicationsLocation,
                "MusicLocation", QStandardPaths::MusicLocation,
                "MoviesLocation", QStandardPaths::MoviesLocation,
                "PicturesLocation", QStandardPaths::PicturesLocation,
                "TempLocation", QStandardPaths::TempLocation,
                "HomeLocation", QStandardPaths::HomeLocation,
                "AppLocalDataLocation", QStandardPaths::AppLocalDataLocation,
                "CacheLocation", QStandardPaths::CacheLocation,
                "GenericDataLocation", QStandardPaths::GenericDataLocation,
                "RuntimeLocation", QStandardPaths::RuntimeLocation,
                "ConfigLocation", QStandardPaths::ConfigLocation,
                "DownloadLocation", QStandardPaths::DownloadLocation,
                "GenericCacheLocation", QStandardPaths::GenericCacheLocation,
                "GenericConfigLocation", QStandardPaths::GenericConfigLocation,
                "AppDataLocation", QStandardPaths::AppDataLocation,
                "AppConfigLocation", QStandardPaths::AppConfigLocation,
                "PublicShareLocation", QStandardPaths::PublicShareLocation,
                "TemplatesLocation", QStandardPaths::TemplatesLocation
        ));
        // clang-format on

        return qt;
    });
}

} // namespace Lua::Internal
