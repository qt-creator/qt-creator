// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QStringList>

#include <memory>

namespace Lua {
class LuaPluginLoaderPrivate;
class LuaPluginLoader : public QObject
{
public:
    LuaPluginLoader();
    ~LuaPluginLoader();

    static LuaPluginLoader &instance();

    void scan(const QStringList &paths);

signals:
    void pluginLoaded(const QString &name);

private:
    std::unique_ptr<LuaPluginLoaderPrivate> d;
};

} // namespace Lua
