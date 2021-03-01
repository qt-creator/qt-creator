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
#include <utils/utilsicons.h>
#include <aggregation/aggregate.h>
#include <coreplugin/find/itemviewfind.h>

#include <QAction>
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

    auto vbox = new QVBoxLayout(m_consoleWidget);
    vbox->setContentsMargins(0, 0, 0, 0);
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

    auto aggregate = new Aggregation::Aggregate();
    aggregate->add(m_consoleView);
    aggregate->add(new Core::ItemViewFind(m_consoleView));

    vbox->addWidget(m_consoleView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(m_consoleWidget));

    m_showDebugButton = new QToolButton(m_consoleWidget);

    m_showDebug.setDefaultValue(true);
    m_showDebug.setSettingsKey(CONSOLE, SHOW_LOG);
    m_showDebug.setLabelText(tr("Show debug, log, and info messages."));
    m_showDebug.setToolTip(tr("Show debug, log, and info messages."));
    m_showDebug.setValue(true);
    m_showDebug.action()->setIcon(Utils::Icons::INFO_TOOLBAR.icon());
    connect(&m_showDebug, &Utils::BoolAspect::valueChanged,
            proxyModel, &ConsoleProxyModel::setShowLogs);
    m_showDebugButton->setDefaultAction(m_showDebug.action());

    m_showWarningButton = new QToolButton(m_consoleWidget);

    m_showWarning.setDefaultValue(true);
    m_showWarning.setSettingsKey(CONSOLE, SHOW_WARNING);
    m_showWarning.setLabelText(tr("Show warning messages."));
    m_showWarning.setToolTip(tr("Show warning messages."));
    m_showWarning.setValue(true);
    m_showWarning.action()->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
    connect(m_showWarning.action(), &QAction::toggled,
            proxyModel, &ConsoleProxyModel::setShowWarnings);
    m_showWarningButton->setDefaultAction(m_showWarning.action());

    m_showErrorButton = new QToolButton(m_consoleWidget);

    m_showError.setDefaultValue(true);
    m_showError.setSettingsKey(CONSOLE, SHOW_ERROR);
    m_showError.setLabelText(tr("Show error messages."));
    m_showError.setToolTip(tr("Show error messages."));
    m_showError.setValue(true);
    m_showError.action()->setIcon(Utils::Icons::CRITICAL_TOOLBAR.icon());
    connect(m_showError.action(), &QAction::toggled,
            proxyModel, &ConsoleProxyModel::setShowErrors);
    m_showErrorButton->setDefaultAction(m_showError.action());

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

bool Console::canFocus() const
{
    return true;
}

bool Console::hasFocus() const
{
    for (QWidget *widget = m_consoleWidget->window()->focusWidget(); widget != nullptr;
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
    m_showDebug.readSettings(settings);
    m_showWarning.readSettings(settings);
    m_showError.readSettings(settings);
}

void Console::setContext(const QString &context)
{
    m_statusLabel->setText(context);
}

void Console::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_showDebug.writeSettings(settings);
    m_showWarning.writeSettings(settings);
    m_showError.writeSettings(settings);
}

void Console::setScriptEvaluator(const ScriptEvaluator &evaluator)
{
    m_scriptEvaluator = evaluator;
    m_consoleItemModel->setCanFetchMore(bool(m_scriptEvaluator));
    if (!m_scriptEvaluator)
        setContext(QString());
}

void Console::populateFileFinder()
{
    m_consoleView->populateFileFinder();
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

} // Internal
} // Debugger
