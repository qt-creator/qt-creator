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
    return QRect(table.get_or<int, const char *, int>("x", 0),
                 table.get_or<int, const char *, int>("y", 0),
                 table.get_or<int, const char *, int>("width", 0),
                 table.get_or<int, const char *, int>("height", 0));
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
    return sol::stack::check<sol::table>(L, index, handler, tracking);
}
QSize sol_lua_get(sol::types<QSize>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    return QSize(table.get_or<int, const char *, int>("width", 0),
                 table.get_or<int, const char *, int>("height", 0));
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
    return QPoint(table.get_or<int, const char *, int>("x", 0),
                  table.get_or<int, const char *, int>("y", 0));
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
    return QRectF(table.get_or<qreal, const char *, qreal>("x", 0.0),
                  table.get_or<qreal, const char *, qreal>("y", 0.0),
                  table.get_or<qreal, const char *, qreal>("width", 0.0),
                  table.get_or<qreal, const char *, qreal>("height", 0.0));
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
    return QSizeF(table.get_or<qreal, const char *, qreal>("width", 0.0),
                  table.get_or<qreal, const char *, qreal>("height", 0.0));
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
    return QPointF(table.get_or<qreal, const char *, qreal>("x", 0.0),
                   table.get_or<qreal, const char *, qreal>("y", 0.0));
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
    return QColor(table.get_or<int, const char *, int>("red", 0),
                  table.get_or<int, const char *, int>("green", 0),
                  table.get_or<int, const char *, int>("blue", 0),
                  table.get_or<int, const char *, int>("alpha", 255));
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
