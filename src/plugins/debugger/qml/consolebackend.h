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

#ifndef CONSOLEBACKEND_H
#define CONSOLEBACKEND_H

#include "consoleitemmodel.h"
#include "interactiveinterpreter.h"

#include <qmljsdebugclient/qdeclarativeenginedebug.h>

#include <QtCore/QObject>

namespace Debugger {
namespace Internal {

class ConsoleBackend : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleBackend(QObject *parent = 0);

    virtual void emitErrorMessage() = 0;

    virtual void evaluate(const QString &, bool *returnKeyConsumed) = 0;

signals:
    void message(ConsoleItemModel::ItemType itemType, const QString &msg);
};

class QmlEngine;
class QmlJSConsoleBackend : public ConsoleBackend
{
    Q_OBJECT
public:
    explicit QmlJSConsoleBackend(QObject *parent = 0);

    void setInferiorStopped(bool inferiorStopped);
    bool inferiorStopped() const;

    void setEngine(QmlEngine *engine);
    QmlEngine *engine() const;

    void setIsValidContext(bool isValidContext);
    bool isValidContext() const;

    virtual void evaluate(const QString &, bool *returnKeyConsumed);
    void emitErrorMessage();

private slots:
    void onDebugQueryStateChanged(
            QmlJsDebugClient::QDeclarativeDebugQuery::State state);

signals:
    void evaluateExpression(const QString &);

private:
     bool canEvaluateScript(const QString &script);

private:
    QmlEngine *m_engine;
    InteractiveInterpreter m_interpreter;
    bool m_inferiorStopped;
    bool m_isValidContext;
    bool m_Error;
};

} //Internal
} //Debugger

#endif // CONSOLEBACKEND_H
