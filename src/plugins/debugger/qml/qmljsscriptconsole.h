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

#ifndef QMLJSSCRIPTCONSOLE_H
#define QMLJSSCRIPTCONSOLE_H

#include "consoleitemmodel.h"
#include <debugger/debuggerconstants.h>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class StatusLabel;
class SavedAction;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class ConsoleTreeView;
class ConsoleItemDelegate;
class QmlJSConsoleBackend;
class QmlJSScriptConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    QmlJSScriptConsoleWidget(QWidget *parent = 0);
    ~QmlJSScriptConsoleWidget();

    void setEngine(DebuggerEngine *engine);
    void readSettings();

public slots:
    void writeSettings() const;
    void appendResult(const QString &result);
    void appendOutput(ConsoleItemModel::ItemType, const QString &message);
    void appendMessage(QtMsgType type, const QString &message);

signals:
    void evaluateExpression(const QString &expr);

private slots:
    void onEngineStateChanged(Debugger::DebuggerState state);
    void onSelectionChanged();

private:
    ConsoleTreeView *m_consoleView;
    ConsoleItemModel *m_model;
    ConsoleItemDelegate *m_itemDelegate;
    QmlJSConsoleBackend *m_consoleBackend;
    Utils::StatusLabel *m_statusLabel;
    QToolButton *m_showLog;
    QToolButton *m_showWarning;
    QToolButton *m_showError;
    Utils::SavedAction *m_showLogAction;
    Utils::SavedAction *m_showWarningAction;
    Utils::SavedAction *m_showErrorAction;
};

} //Internal
} //Debugger

#endif
