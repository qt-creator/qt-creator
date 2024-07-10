// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include "expected.h"
#include "utils_global.h"

namespace Utils {

class LuaState
{
public:
    virtual ~LuaState() = default;
};

class QTCREATOR_UTILS_EXPORT LuaInterface
{
public:
    virtual ~LuaInterface() = default;
    virtual expected_str<std::unique_ptr<LuaState>> runScript(
        const QString &script, const QString &name)
        = 0;
};

QTCREATOR_UTILS_EXPORT void setLuaInterface(LuaInterface *luaInterface);
QTCREATOR_UTILS_EXPORT LuaInterface *luaInterface();

QTCREATOR_UTILS_EXPORT expected_str<std::unique_ptr<LuaState>> runScript(
    const QString &script, const QString &name);

} // namespace Utils
