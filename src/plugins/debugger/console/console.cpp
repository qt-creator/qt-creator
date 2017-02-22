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

#include "console.h"
#include "consoleview.h"
#include "consoleproxymodel.h"
#include "consoleitemdelegate.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <utils/savedaction.h>
#include <utils/utilsicons.h>
#include <aggregation/aggregate.h>
#include <coreplugin/find/itemviewfind.h>

#include <QCoreApplication>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>

const char CONSOLE[] = "Console";
const char SHOW_LOG[] = "showLog";
const char SHOW_WARNING[] = "showWarning";
const char SHOW_ERROR[] = "showError";

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// Console
//
/////////////////////////////////////////////////////////////////////

Console::Console()
{
    m_consoleItemModel = new ConsoleItemModel(this);

    m_consoleWidget = new QWidget;
    m_consoleWidget->setWindowTitle(displayName());
    m_consoleWidget->setEnabled(true);

    QVBoxLayout *vbox = new QVBoxLayout(m_consoleWidget);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    m_consoleView = new ConsoleView(m_consoleItemModel, m_consoleWidget);
    auto proxyModel = new ConsoleProxyModel(this);
    proxyModel->setSourceModel(m_consoleItemModel);
    connect(m_consoleItemModel,
            &ConsoleItemModel::selectEditableRow,
            proxyModel,
            &ConsoleProxyModel::selectEditableRow);

    //Scroll to bottom when rows matching current filter settings are inserted
    //Not connecting rowsRemoved as the only way to remove rows is to clear the
    //model which will automatically reset the view.
    connect(m_consoleItemModel, &QAbstractItemModel::rowsInserted,
            proxyModel, &ConsoleProxyModel::onRowsInserted);
    m_consoleView->setModel(proxyModel);

    connect(proxyModel, &ConsoleProxyModel::setCurrentIndex,
            m_consoleView->selectionModel(), &QItemSelectionModel::setCurrentIndex);
    connect(proxyModel, &ConsoleProxyModel::scrollToBottom,
            m_consoleView, &ConsoleView::onScrollToBottom);

    auto itemDelegate = new ConsoleItemDelegate(m_consoleItemModel, this);
    connect(m_consoleView->selectionModel(), &QItemSelectionModel::currentChanged,
            itemDelegate, &ConsoleItemDelegate::currentChanged);
    m_consoleView->setItemDelegate(itemDelegate);

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate();
    aggregate->add(m_consoleView);
    aggregate->add(new Core::ItemViewFind(m_consoleView));

    vbox->addWidget(m_consoleView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(m_consoleWidget));

    m_showDebugButton = new QToolButton(m_consoleWidget);
    m_showDebugButton->setAutoRaise(true);

    m_showDebugButtonAction = new Utils::SavedAction(this);
    m_showDebugButtonAction->setDefaultValue(true);
    m_showDebugButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_LOG));
    m_showDebugButtonAction->setToolTip(tr("Show debug, log, and info messages."));
    m_showDebugButtonAction->setCheckable(true);
    m_showDebugButtonAction->setChecked(true);
    m_showDebugButtonAction->setIcon(Utils::Icons::INFO_TOOLBAR.icon());
    connect(m_showDebugButtonAction, &Utils::SavedAction::toggled,
            proxyModel, &ConsoleProxyModel::setShowLogs);
    m_showDebugButton->setDefaultAction(m_showDebugButtonAction);

    m_showWarningButton = new QToolButton(m_consoleWidget);
    m_showWarningButton->setAutoRaise(true);

    m_showWarningButtonAction = new Utils::SavedAction(this);
    m_showWarningButtonAction->setDefaultValue(true);
    m_showWarningButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_WARNING));
    m_showWarningButtonAction->setToolTip(tr("Show warning messages."));
    m_showWarningButtonAction->setCheckable(true);
    m_showWarningButtonAction->setChecked(true);
    m_showWarningButtonAction->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
    connect(m_showWarningButtonAction, &Utils::SavedAction::toggled,
            proxyModel, &ConsoleProxyModel::setShowWarnings);
    m_showWarningButton->setDefaultAction(m_showWarningButtonAction);

    m_showErrorButton = new QToolButton(m_consoleWidget);
    m_showErrorButton->setAutoRaise(true);

    m_showErrorButtonAction = new Utils::SavedAction(this);
    m_showErrorButtonAction->setDefaultValue(true);
    m_showErrorButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_ERROR));
    m_showErrorButtonAction->setToolTip(tr("Show error messages."));
    m_showErrorButtonAction->setCheckable(true);
    m_showErrorButtonAction->setChecked(true);
    m_showErrorButtonAction->setIcon(Utils::Icons::CRITICAL_TOOLBAR.icon());
    connect(m_showErrorButtonAction, &Utils::SavedAction::toggled,
            proxyModel, &ConsoleProxyModel::setShowErrors);
    m_showErrorButton->setDefaultAction(m_showErrorButtonAction);

    m_spacer = new QWidget(m_consoleWidget);
    m_spacer->setMinimumWidth(30);

    m_statusLabel = new QLabel(m_consoleWidget);

    readSettings();
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &Console::writeSettings);
}

Console::~Console()
{
    writeSettings();
    delete m_consoleWidget;
}

QWidget *Console::outputWidget(QWidget *)
{
    return m_consoleWidget;
}

QList<QWidget *> Console::toolBarWidgets() const
{
     return {m_showDebugButton, m_showWarningButton, m_showErrorButton,
             m_spacer, m_statusLabel};
}

int Console::priorityInStatusBar() const
{
    return 20;
}

void Console::clearContents()
{
    m_consoleItemModel->clear();
}

void Console::visibilityChanged(bool /*visible*/)
{
}

bool Console::canFocus() const
{
    return true;
}

bool Console::hasFocus() const
{
    for (QWidget *widget = m_consoleWidget->window()->focusWidget(); widget != 0;
         widget = widget->parentWidget()) {
        if (widget == m_consoleWidget)
            return true;
    }
    return false;
}

void Console::setFocus()
{
    m_consoleView->setFocus();
}

bool Console::canNext() const
{
    return false;
}

bool Console::canPrevious() const
{
    return false;
}

void Console::goToNext()
{
}

void Console::goToPrev()
{
}

bool Console::canNavigate() const
{
    return false;
}

void Console::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    m_showDebugButtonAction->readSettings(settings);
    m_showWarningButtonAction->readSettings(settings);
    m_showErrorButtonAction->readSettings(settings);
}

void Console::setContext(const QString &context)
{
    m_statusLabel->setText(context);
}

void Console::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_showDebugButtonAction->writeSettings(settings);
    m_showWarningButtonAction->writeSettings(settings);
    m_showErrorButtonAction->writeSettings(settings);
}

void Console::setScriptEvaluator(const ScriptEvaluator &evaluator)
{
    m_scriptEvaluator = evaluator;
    m_consoleItemModel->setCanFetchMore(bool(m_scriptEvaluator));
    if (!m_scriptEvaluator)
        setContext(QString());
}

void Console::printItem(ConsoleItem::ItemType itemType, const QString &text)
{
    printItem(new ConsoleItem(itemType, text));
}

void Console::printItem(ConsoleItem *item)
{
    m_consoleItemModel->appendItem(item);
    if (item->itemType() == ConsoleItem::ErrorType)
        popup(Core::IOutputPane::ModeSwitch);
    else if (item->itemType() == ConsoleItem::WarningType)
        flash();
}

void Console::evaluate(const QString &expression)
{
    if (m_scriptEvaluator) {
        m_consoleItemModel->shiftEditableRow();
        m_scriptEvaluator(expression);
    } else {
        auto item = new ConsoleItem(ConsoleItem::ErrorType,
               QCoreApplication::translate(
                        "Debugger::Internal::Console",
                        "Can only evaluate during a debug session."));
        m_consoleItemModel->shiftEditableRow();
        printItem(item);
    }
}

Console *debuggerConsole()
{
    static Console *theConsole = new Console;
    return theConsole;
}

} // Internal
} // Debugger
