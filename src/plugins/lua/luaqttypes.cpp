// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

// This defines the conversion from QString to lua_string and vice versa
bool sol_lua_check(sol::types<QString>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    // use sol's method for checking specifically for a string
    return sol::stack::check<const char *>(L, index, handler, tracking);
}

QString sol_lua_get(sol::types<QString>, lua_State *L, int index, sol::stack::record &tracking)
{
    const char *str = sol::stack::get<const char *>(L, index, tracking);
    return QString::fromLocal8Bit(str);
}

int sol_lua_push(sol::types<QString>, lua_State *L, const QString &qStr)
{
    // create table
    sol::state_view lua(L);
    // use base sol method to push the string
    int amount = sol::stack::push(L, qStr.toLocal8Bit().data());
    // return # of things pushed onto stack
    return amount;
}

// QRect
bool sol_lua_check(sol::types<QRect>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QRect sol_lua_get(sol::types<QRect>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2 && table.size() != 4)
        throw sol::error("Expected table to have 'x', 'y', 'width' and 'height' or 2 (pos and "
                         "size) or 4 elements");
    if (table.size() == 0)
        return QRect(
            table.get<int>("x"),
            table.get<int>("y"),
            table.get<int>("width"),
            table.get<int>("height"));
    if (table.size() == 2)
        return QRect(table.get<QPoint>(1), table.get<QSize>(2));

    if (table.size() == 4)
        return QRect(table.get<int>(1), table.get<int>(2), table.get<int>(3), table.get<int>(4));
}

int sol_lua_push(sol::types<QRect>, lua_State *L, const QRect &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("x", value.x(), "y", value.y(), "width", value.width(), "height", value.height());
    return sol::stack::push(L, table);
}

// QSize
bool sol_lua_check(sol::types<QSize>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::lua_table>(L, index, handler, tracking);
}

QSize sol_lua_get(sol::types<QSize>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2)
        throw sol::error("Expected table to have 'width' and 'height' or 2 elements");
    if (table.size() == 0)
        return QSize(table.get<int>("width"), table.get<int>("height"));

    return QSize(table.get<int>(1), table.get<int>(2));
}
int sol_lua_push(sol::types<QSize>, lua_State *L, const QSize &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("width", value.width(), "height", value.height());
    return sol::stack::push(L, table);
}

// QPoint
bool sol_lua_check(sol::types<QPoint>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QPoint sol_lua_get(sol::types<QPoint>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2)
        throw sol::error("Expected table to have 'x' and 'y' or 2 elements");
    if (table.size() == 0)
        return QPoint(table.get<int>("x"), table.get<int>("y"));
    return QPoint(table.get<int>(1), table.get<int>(2));
}
int sol_lua_push(sol::types<QPoint>, lua_State *L, const QPoint &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("x", value.x(), "y", value.y());
    return sol::stack::push(L, table);
}

// QRectF
bool sol_lua_check(sol::types<QRectF>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QRectF sol_lua_get(sol::types<QRectF>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2 && table.size() != 4)
        throw sol::error("Expected table to have 'x', 'y', 'width' and 'height' or 2 (pos and "
                         "size) or 4 elements");
    if (table.size() == 0) {
        return QRectF(
            table.get<qreal>("x"),
            table.get<qreal>("y"),
            table.get<qreal>("width"),
            table.get<qreal>("height"));
    }
    if (table.size() == 2)
        return QRectF(table.get<QPointF>(1), table.get<QSizeF>(2));

    return QRectF(table.get<qreal>(1), table.get<qreal>(2), table.get<qreal>(3), table.get<qreal>(4));
}
int sol_lua_push(sol::types<QRectF>, lua_State *L, const QRectF &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("x", value.x(), "y", value.y(), "width", value.width(), "height", value.height());
    return sol::stack::push(L, table);
}

// QSizeF
bool sol_lua_check(sol::types<QSizeF>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QSizeF sol_lua_get(sol::types<QSizeF>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2)
        throw sol::error("Expected table to have 'width' and 'height' or 2 elements");
    if (table.size() == 0)
        return QSizeF(table.get<qreal>("width"), table.get<qreal>("height"));
    return QSizeF(table.get<qreal>(1), table.get<qreal>(2));
}
int sol_lua_push(sol::types<QSizeF>, lua_State *L, const QSizeF &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("width", value.width(), "height", value.height());
    return sol::stack::push(L, table);
}

// QPointF
bool sol_lua_check(sol::types<QPointF>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QPointF sol_lua_get(sol::types<QPointF>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 2)
        throw sol::error("Expected table to have 'x' and 'y' or 2 elements");
    if (table.size() == 0)
        return QPointF(table.get<qreal>("x"), table.get<qreal>("y"));
    return QPointF(table.get<qreal>(1), table.get<qreal>(2));
}
int sol_lua_push(sol::types<QPointF>, lua_State *L, const QPointF &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("x", value.x(), "y", value.y());
    return sol::stack::push(L, table);
}

// QColor
bool sol_lua_check(sol::types<QColor>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QColor sol_lua_get(sol::types<QColor>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    if (table.size() != 0 && table.size() != 4)
        throw sol::error("Expected table to have 0 or 4 elements");
    if (table.size() == 0) {
        return QColor(
            table.get<int>("red"),
            table.get<int>("green"),
            table.get<int>("blue"),
            table.get<int>("alpha"));
    }
    return QColor(table.get<int>(1), table.get<int>(2), table.get<int>(3), table.get<int>(4));
}
int sol_lua_push(sol::types<QColor>, lua_State *L, const QColor &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    table.set("red",
              value.red(),
              "green",
              value.green(),
              "blue",
              value.blue(),
              "alpha",
              value.alpha());
    return sol::stack::push(L, table);
}

// QStringList
bool sol_lua_check(sol::types<QStringList>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QStringList sol_lua_get(sol::types<QStringList>,
                        lua_State *L,
                        int index,
                        sol::stack::record &tracking)
{
    QStringList result;
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    for (size_t i = 1; i < table.size() + 1; i++) {
        result.append(table.get<QString>(i));
    }
    return result;
}
int sol_lua_push(sol::types<QStringList>, lua_State *L, const QStringList &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    for (const QString &str : value)
        table.add(str);
    return sol::stack::push(L, table);
}
