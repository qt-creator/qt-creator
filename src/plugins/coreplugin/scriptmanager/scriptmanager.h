/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtScript/QScriptEngine>

namespace Core {

/* Script Manager.
 * Provides a script engine that is initialized with
 * Qt Creator's interfaces and allows for running scripts.
 * @{todo} Should it actually manage script files, too? */

class CORE_EXPORT ScriptManager : public QObject
{
    Q_OBJECT
public:
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

    // Access the engine (for plugins to wrap additional interfaces).
    virtual QScriptEngine &scriptEngine() = 0;

    // Run a script
    virtual bool runScript(const QString &script, QString *errorMessage, Stack *errorStack) = 0;
    virtual bool runScript(const QString &script, QString *errorMessage) = 0;
};

} // namespace Core

#endif // SCRIPTMANAGER_H
