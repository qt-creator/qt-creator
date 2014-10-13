/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsexpander.h"

#include "corejsextensions.h"
#include "variablemanager.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QScriptEngine>

namespace Core {

namespace Internal {

class JsExpanderPrivate {
public:
    ~JsExpanderPrivate() { qDeleteAll(m_registeredObjects); }

    QScriptEngine m_engine;
    QList<QObject *> m_registeredObjects;
};

} // namespace Internal

static Internal::JsExpanderPrivate *d;

void JsExpander::registerQObjectForJs(const QString &name, QObject *obj)
{
    obj->setParent(0); // take ownership!
    d->m_registeredObjects.append(obj);
    QScriptValue jsObj = d->m_engine.newQObject(obj, QScriptEngine::QtOwnership);
    d->m_engine.globalObject().setProperty(name, jsObj);
}

QString JsExpander::evaluate(const QString &expression, QString *errorMessage)
{
    d->m_engine.clearExceptions();
    QScriptValue value = d->m_engine.evaluate(expression);
    if (d->m_engine.hasUncaughtException()) {
        const QString msg = QCoreApplication::translate("Core::JsExpander", "Error in \"%1\": %2")
                .arg(expression, d->m_engine.uncaughtException().toString());
        if (errorMessage)
            *errorMessage = msg;
        return QString();
    }
    // Try to convert to bool, be that an int or whatever.
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isNumber())
        return QString::number(value.toNumber());
    if (value.isString())
        return value.toString();
    QString msg = QCoreApplication::translate("Core::JsExpander",
                                              "Cannot convert result of \"%1\" to string.").arg(expression);
    if (errorMessage)
        *errorMessage = msg;
    return QString();
}

JsExpander::JsExpander()
{
    d = new Internal::JsExpanderPrivate;
    globalMacroExpander()->registerPrefix("JS",
        QCoreApplication::translate("Core::JsExpander",
                                    "Evaluate simple Javascript statements.\n"
                                    "The statements may not contain '{' nor '}' characters."),
        [this](QString in) -> QString {
            QString errorMessage;
            QString result = JsExpander::evaluate(in, &errorMessage);
            if (!errorMessage.isEmpty()) {
                qWarning() << errorMessage;
                return errorMessage;
            } else {
                return result;
            }
        });

    registerQObjectForJs(QLatin1String("Util"), new Internal::UtilsJsExtension);
}

JsExpander::~JsExpander()
{
    delete d;
    d = 0;
}

} // namespace Core
