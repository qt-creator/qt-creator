// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lua.h"

#include "utilstr.h"

namespace Utils {

static LuaInterface *s_luaInterface = nullptr;

void setLuaInterface(LuaInterface *luaInterface)
{
    s_luaInterface = luaInterface;
}

LuaInterface *luaInterface()
{
    return s_luaInterface;
}

expected_str<std::unique_ptr<LuaState>> runScript(const QString &script, const QString &name)
{
    if (!s_luaInterface)
        return make_unexpected(Tr::tr("No Lua interface set"));

    return s_luaInterface->runScript(script, name);
}

} // namespace Utils
