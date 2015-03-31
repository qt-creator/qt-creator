/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsexpander.h"

#include "corejsextensions.h"

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QJSEngine>

namespace Core {

namespace Internal {

class JsExpanderPrivate {
public:
    QJSEngine m_engine;
};

} // namespace Internal

static Internal::JsExpanderPrivate *d;

void JsExpander::registerQObjectForJs(const QString &name, QObject *obj)
{
    QJSValue jsObj = d->m_engine.newQObject(obj);
    d->m_engine.globalObject().setProperty(name, jsObj);
}

QString JsExpander::evaluate(const QString &expression, QString *errorMessage)
{
    QJSValue value = d->m_engine.evaluate(expression);
    if (value.isError()) {
        const QString msg = QCoreApplication::translate("Core::JsExpander", "Error in \"%1\": %2")
                .arg(expression, value.toString());
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
    QString msg = QCoreApplication::translate("Core::JsExpander",
                                              "Cannot convert result of \"%1\" to string.").arg(expression);
    if (errorMessage)
        *errorMessage = msg;
    return QString();
}

JsExpander::JsExpander()
{
    d = new Internal::JsExpanderPrivate;
    Utils::globalMacroExpander()->registerPrefix("JS",
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
