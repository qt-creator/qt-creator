// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsexpander.h"

#include "corejsextensions.h"
#include "coreplugintr.h"

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QJSEngine>

#include <unordered_map>

using ExtensionMap = std::unordered_map<QString, Core::JsExpander::ObjectFactory>;
Q_GLOBAL_STATIC(ExtensionMap, globalJsExtensions);

static Core::JsExpander *globalExpander = nullptr;

namespace Core {
namespace Internal {

class JsExpanderPrivate {
public:
    QJSEngine m_engine;
};

} // namespace Internal

void JsExpander::registerGlobalObject(const QString &name, const ObjectFactory &factory)
{
    globalJsExtensions->insert({name, factory});
    if (globalExpander)
        globalExpander->registerObject(name, factory());
}

void JsExpander::registerObject(const QString &name, QObject *obj)
{
    QJSValue jsObj = d->m_engine.newQObject(obj);
    d->m_engine.globalObject().setProperty(name, jsObj);
}

QString JsExpander::evaluate(const QString &expression, QString *errorMessage)
{
    QJSValue value = d->m_engine.evaluate(expression);
    if (value.isError()) {
        const QString msg = Tr::tr("Error in \"%1\": %2").arg(expression, value.toString());
        if (errorMessage)
            *errorMessage = msg;
        return QString();
    }
    // Try to convert to bool, be that an int or whatever.
    if (value.isBool())
        return value.toString();
    if (value.isNumber())
        return QString::number(value.toNumber());
    if (value.isString())
        return value.toString();
    QString msg = Tr::tr("Cannot convert result of \"%1\" to string.").arg(expression);
    if (errorMessage)
        *errorMessage = msg;
    return QString();
}

QJSEngine &JsExpander::engine()
{
    return d->m_engine;
}

void JsExpander::registerForExpander(Utils::MacroExpander *macroExpander)
{
    macroExpander->registerPrefix(
        "JS",
        Tr::tr("Evaluate simple JavaScript statements.<br>"
               "Literal '}' characters must be escaped as \"\\}\", "
               "'\\' characters must be escaped as \"\\\\\", "
               "and \"%{\" must be escaped as \"%\\{\"."),
        [this](QString in) -> QString {
            QString errorMessage;
            QString result = evaluate(in, &errorMessage);
            if (!errorMessage.isEmpty()) {
                qWarning() << errorMessage;
                return errorMessage;
            } else {
                return result;
            }
        });
}

JsExpander *JsExpander::createGlobalJsExpander()
{
    globalExpander = new JsExpander();
    registerGlobalObject<Internal::UtilsJsExtension>("Util");
    globalExpander->registerForExpander(Utils::globalMacroExpander());
    return globalExpander;
}

JsExpander::JsExpander()
{
    d = new Internal::JsExpanderPrivate;
    for (const std::pair<const QString, ObjectFactory> &obj : *globalJsExtensions)
        registerObject(obj.first, obj.second());
}

JsExpander::~JsExpander()
{
    delete d;
    d = nullptr;
}

} // namespace Core
