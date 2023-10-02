// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <functional>

QT_BEGIN_NAMESPACE
class QJSEngine;
class QObject;
class QString;
QT_END_NAMESPACE

namespace Utils { class MacroExpander; }

namespace Core {

namespace Internal {
class ICorePrivate;
class JsExpanderPrivate;
} // namespace Internal

class CORE_EXPORT JsExpander
{
public:
    using ObjectFactory = std::function<QObject *()>;

    JsExpander();
    ~JsExpander();

    template <class T>
    static void registerGlobalObject(const QString &name)
    {
        registerGlobalObject(name, []{ return new T; });
    }

    static void registerGlobalObject(const QString &name, const ObjectFactory &factory);

    void registerObject(const QString &name, QObject *obj);
    QString evaluate(const QString &expression, QString *errorMessage = nullptr);

    QJSEngine &engine();
    void registerForExpander(Utils::MacroExpander *macroExpander);

private:
    static JsExpander *createGlobalJsExpander();

    Internal::JsExpanderPrivate *d;
    friend class Internal::ICorePrivate;
};

} // namespace Core
