
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luaqttypes.h"

#include <QMetaProperty>

#include <QDialog>

namespace Lua::Internal {

template<class T = QObject>
sol::object qobject_index_get(const sol::this_state &s, QObject *obj, const char *key)
{
    auto &metaObject = T::staticMetaObject;
    int iProp = metaObject.indexOfProperty(key);
    if (iProp != -1) {
        QMetaProperty p = metaObject.property(iProp);

        if (p.isEnumType()) {
            int v = p.read(obj).toInt();
            return sol::make_object(s.lua_state(), p.enumerator().valueToKey(v));
        }

#define LUA_VALUE_FROM_PROPERTY(VARIANT_TYPE, TYPE) \
    case VARIANT_TYPE: { \
        TYPE r = qvariant_cast<TYPE>(p.read(obj)); \
        return sol::make_object(s.lua_state(), r); \
    }

        switch (p.metaType().id()) {
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QRect, QRect)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QSize, QSize)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QPoint, QPoint)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QRectF, QRectF)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QSizeF, QSizeF)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QPointF, QPointF)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QColor, QColor)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::Bool, bool)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::Int, int)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::Double, double)
            LUA_VALUE_FROM_PROPERTY(QMetaType::Type::QString, QString)
        default:
            break;
        }
    }

    for (int i = 0; i < metaObject.methodCount(); i++) {
        QMetaMethod method = metaObject.method(i);
        if (method.methodType() != QMetaMethod::Signal && method.name() == key) {
            if (method.parameterCount() == 0) {
                return sol::make_object(s.lua_state(),
                                        [obj, i]() { obj->metaObject()->method(i).invoke(obj); });
            }
        }
    }

    return sol::lua_nil;
}

template<class T>
void qobject_index_set(QObject *obj, const char *key, sol::stack_object value)
{
    auto &metaObject = T::staticMetaObject;
    int iProp = metaObject.indexOfProperty(key);
    if (iProp == -1)
        return;

    QMetaProperty p = metaObject.property(iProp);

    if (p.isEnumType()) {
        int v = p.enumerator().keyToValue(value.as<const char *>());
        p.write(obj, v);
    } else {
#define SET_PROPERTY_FROM_LUA(VARIANT_TYPE, TYPE) \
    case VARIANT_TYPE: { \
        TYPE r = value.as<TYPE>(); \
        p.write(obj, QVariant::fromValue(r)); \
        break; \
    }

        switch (p.metaType().id()) {
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QRect, QRect)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QSize, QSize)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QPoint, QPoint)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QRectF, QRectF)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QSizeF, QSizeF)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QPointF, QPointF)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QColor, QColor)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::Bool, bool)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::Int, int)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::Double, double)
            SET_PROPERTY_FROM_LUA(QMetaType::Type::QString, QString)
        default:
            break;
        }
    }
}

template<class T>
size_t qobject_index_size(QObject *)
{
    return 0;
}

template<class T, class... Bases>
sol::usertype<T> runtimeObject(sol::state_view lua)
{
    auto &metaObject = T::staticMetaObject;
    auto className = metaObject.className();

    return lua.new_usertype<T>(
        className,
        sol::call_constructor,
        sol::constructors<T>(),
        sol::meta_function::index,
        [](sol::this_state s, T *obj, const char *key) { return qobject_index_get<T>(s, obj, key); },
        sol::meta_function::new_index,
        [](T *obj, const char *key, sol::stack_object value) {
            qobject_index_set<T>(obj, key, value);
        },
        sol::meta_function::length,
        [](T *obj) { return qobject_index_size<T>(obj); },
        sol::base_classes,
        sol::bases<Bases...>());
}

void registerUiBindings()
{
    registerProvider("Qt.Gui", [](sol::state_view lua) {
        runtimeObject<QObject>(lua);
        runtimeObject<QWidget, QObject>(lua);
        runtimeObject<QDialog, QWidget, QObject>(lua);

        return sol::object{};
    });
}

} // namespace Lua::Internal
