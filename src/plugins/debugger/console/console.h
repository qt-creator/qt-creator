/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

using ScriptEvaluator = std::function<void (QString)>;

class ConsoleItemModel;
class ConsoleView;

class Console : public Core::IOutputPane
{
    Q_OBJECT

public:
    Console();
    ~Console() override;

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;
    QString displayName() const override { return tr("QML Debugger Console"); }
    int priorityInStatusBar() const override;
    void clearContents() override;
    void visibilityChanged(bool visible) override;
    bool canFocus() const override;
    bool hasFocus() const override;
    void setFocus() override;

    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    bool canNavigate() const override;

    void readSettings();
    void setContext(const QString &context);

    void setScriptEvaluator(const ScriptEvaluator &evaluator);
    void populateFileFinder();

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
