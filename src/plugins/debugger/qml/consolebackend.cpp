/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "consolebackend.h"
#include "qmlengine.h"
#include "qmladapter.h"

#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// ConsoleBackend
//
///////////////////////////////////////////////////////////////////////

ConsoleBackend::ConsoleBackend(QObject *parent) :
    QObject(parent)
{
}

///////////////////////////////////////////////////////////////////////
//
// QmlJSConsoleBackend
//
///////////////////////////////////////////////////////////////////////

QmlJSConsoleBackend::QmlJSConsoleBackend(QObject *parent) :
    ConsoleBackend(parent),
    m_engine(0),
    m_inferiorStopped(false),
    m_isValidContext(false),
    m_Error(false)
{
}

void QmlJSConsoleBackend::setInferiorStopped(bool isInferiorStopped)
{
    m_inferiorStopped = isInferiorStopped;
}

bool QmlJSConsoleBackend::inferiorStopped() const
{
    return m_inferiorStopped;
}

void QmlJSConsoleBackend::setEngine(QmlEngine *engine)
{
    m_engine = engine;
}

QmlEngine *QmlJSConsoleBackend::engine() const
{
    return m_engine;
}

void QmlJSConsoleBackend::setIsValidContext(bool isValidContext)
{
    m_isValidContext = isValidContext;
}

bool QmlJSConsoleBackend::isValidContext() const
{
    return m_isValidContext;
}

void QmlJSConsoleBackend::onDebugQueryStateChanged(
        QmlJsDebugClient::QDeclarativeDebugQuery::State state)
{
    QmlJsDebugClient::QDeclarativeDebugExpressionQuery *query =
            qobject_cast<QmlJsDebugClient::QDeclarativeDebugExpressionQuery *>(
                sender());
    if (query && state != QmlJsDebugClient::QDeclarativeDebugQuery::Error)
        emit message(ConsoleItemModel::UndefinedType,
                     query->result().toString());
    else
        emit message(ConsoleItemModel::ErrorType,
                     _("Error evaluating expression."));
    delete query;
}

void QmlJSConsoleBackend::evaluate(const QString &script,
                                   bool *returnKeyConsumed)
{
    *returnKeyConsumed = true;
    m_Error = false;
    //Check if string is only white spaces
    if (!script.trimmed().isEmpty()) {
        //Check for a valid context
        if (m_isValidContext) {
            //check if it can be evaluated
            if (canEvaluateScript(script)) {
                //Evaluate expression based on engine state
                //When engine->state() == InferiorStopOk, the expression
                //is sent to V8DebugService. In all other cases, the
                //expression is evaluated by QDeclarativeEngine.
                if (!m_inferiorStopped) {
                    QmlAdapter *adapter = m_engine->adapter();
                    QTC_ASSERT(adapter, return);
                    QDeclarativeEngineDebug *engineDebug =
                            adapter->engineDebugClient();

                    int id = adapter->currentSelectedDebugId();
                    if (engineDebug && id != -1) {
                        QDeclarativeDebugExpressionQuery *query =
                                engineDebug->queryExpressionResult(id, script);
                        connect(query,
                                SIGNAL(stateChanged(
                                           QmlJsDebugClient::QDeclarativeDebugQuery
                                           ::State)),
                                this,
                                SLOT(onDebugQueryStateChanged(
                                         QmlJsDebugClient::QDeclarativeDebugQuery
                                         ::State)));
                    }
                } else {
                    emit evaluateExpression(script);
                }
            } else {
                *returnKeyConsumed = false;
            }
        } else {
            //Incase of invalid context, append the expression to history
            //and show Error message
            m_Error = true;
        }
    }
}

void QmlJSConsoleBackend::emitErrorMessage()
{
    if (m_Error)
        emit message(
                    ConsoleItemModel::ErrorType,
                    _("Cannot evaluate without a valid QML/JS Context"));
}

bool QmlJSConsoleBackend::canEvaluateScript(const QString &script)
{
    m_interpreter.clearText();
    m_interpreter.appendText(script);
    return m_interpreter.canEvaluate();
}

}
}
