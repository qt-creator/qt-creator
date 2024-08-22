// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// DO NOT INCLUDE THIS YOURSELF!!1!

#include "lua_global.h"

#include <QColor>
#include <QList>
#include <QPointer>
#include <QRect>
#include <QString>

#define SOL_CONVERSION_FUNCTIONS(TYPE) \
    bool LUA_EXPORT sol_lua_check(sol::types<TYPE>, \
                                  lua_State *L, \
                                  int index, \
                                  std::function<sol::check_handler_type> handler, \
                                  sol::stack::record &tracking); \
    TYPE LUA_EXPORT sol_lua_get(sol::types<TYPE>, \
                                lua_State *L, \
                                int index, \
                                sol::stack::record &tracking); \
    int LUA_EXPORT sol_lua_push(sol::types<TYPE>, lua_State *L, const TYPE &rect);

SOL_CONVERSION_FUNCTIONS(QString)

SOL_CONVERSION_FUNCTIONS(QRect)
SOL_CONVERSION_FUNCTIONS(QSize)
SOL_CONVERSION_FUNCTIONS(QPoint)
SOL_CONVERSION_FUNCTIONS(QRectF)
SOL_CONVERSION_FUNCTIONS(QSizeF)
SOL_CONVERSION_FUNCTIONS(QPointF)
SOL_CONVERSION_FUNCTIONS(QColor)

SOL_CONVERSION_FUNCTIONS(QStringList)

#undef SOL_CONVERSION_FUNCTIONS

namespace sol {
template<typename T>
struct unique_usertype_traits<QPointer<T>>
{
    typedef T type;
    typedef QPointer<T> actual_type;
    static const bool value = true;

    static bool is_null(const actual_type &ptr) { return ptr == nullptr; }

    static type *get(const actual_type &ptr) { return ptr.get(); }
};
} // namespace sol
