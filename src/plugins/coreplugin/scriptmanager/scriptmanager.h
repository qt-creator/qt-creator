/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <coreplugin/core_global.h>

#include <QObject>
#include <QString>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QScriptEngine;
QT_END_NAMESPACE

namespace Core {

/* Script Manager.
 * Provides a script engine that is initialized with
 * Qt Creator's interfaces and allows for running scripts.
 * @{todo} Should it actually manage script files, too? */

class CORE_EXPORT ScriptManager : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<QScriptEngine> QScriptEnginePtr;

    // A stack frame as returned by a failed invocation (exception)
    // fileName may be empty. lineNumber can be 0 for the top frame (goof-up?).
    struct StackFrame {
        QString function;
        QString fileName;
        int lineNumber;
    };
    typedef QList<StackFrame> Stack;

    ScriptManager(QObject *parent = 0) : QObject(parent) {}
    virtual ~ScriptManager() { }

    // Run a script
    virtual bool runScript(const QString &script, QString *errorMessage, Stack *errorStack) = 0;
    virtual bool runScript(const QString &script, QString *errorMessage) = 0;

    virtual QScriptEnginePtr scriptEngine() = 0;
};

} // namespace Core

#endif // SCRIPTMANAGER_H
