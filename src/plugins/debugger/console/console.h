/**************************************************************************
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

#ifndef DEBUGGER_CONSOLE_H
#define DEBUGGER_CONSOLE_H

#include "consoleitem.h"

#include <coreplugin/ioutputpane.h>

#include <functional>

#include <QObject>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class SavedAction; }

namespace Debugger {
namespace Internal {

typedef std::function<bool(QString)> ScriptEvaluator;

class ConsoleItemModel;
class ConsoleView;

class Console : public Core::IOutputPane
{
    Q_OBJECT

public:
    Console();
    ~Console();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const { return tr("Debugger Console"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void readSettings();
    void setContext(const QString &context);

    void setScriptEvaluator(const ScriptEvaluator &evaluator);

    void evaluate(const QString &expression);
    void printItem(ConsoleItem *item);
    void printItem(ConsoleItem::ItemType itemType, const QString &text);

    void writeSettings() const;

private:
    QToolButton *m_showDebugButton;
    QToolButton *m_showWarningButton;
    QToolButton *m_showErrorButton;
    Utils::SavedAction *m_showDebugButtonAction;
    Utils::SavedAction *m_showWarningButtonAction;
    Utils::SavedAction *m_showErrorButtonAction;
    QWidget *m_spacer;
    QLabel *m_statusLabel;
    ConsoleItemModel *m_consoleItemModel;
    ConsoleView *m_consoleView;
    QWidget *m_consoleWidget;
    ScriptEvaluator m_scriptEvaluator;
};

Console *debuggerConsole();

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CONSOLE_H
