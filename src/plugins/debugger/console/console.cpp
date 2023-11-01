// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "console.h"

#include "consoleview.h"
#include "consoleproxymodel.h"
#include "consoleitemdelegate.h"
#include "../debuggertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>

#include <utils/utilsicons.h>

#include <aggregation/aggregate.h>
#include <coreplugin/find/itemviewfind.h>

#include <QAction>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>

const char CONSOLE[] = "Console";
const char SHOW_LOG[] = "showLog";
const char SHOW_WARNING[] = "showWarning";
const char SHOW_ERROR[] = "showError";

namespace Debugger::Internal {

/////////////////////////////////////////////////////////////////////
//
// Console
//
/////////////////////////////////////////////////////////////////////

Console::Console()
{
    setId("QMLDebuggerConsole");
    setDisplayName(Tr::tr("QML Debugger Console"));
    setPriorityInStatusBar(-40);

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
    m_showDebug.setLabelText(Tr::tr("Show debug, log, and info messages."));
    m_showDebug.setToolTip(Tr::tr("Show debug, log, and info messages."));
    m_showDebug.setValue(true);
    m_showDebug.action()->setIcon(Utils::Icons::INFO_TOOLBAR.icon());
    connect(&m_showDebug, &Utils::BoolAspect::changed,
            proxyModel, [this, proxyModel] { proxyModel->setShowLogs(m_showDebug()); });
    m_showDebugButton->setDefaultAction(m_showDebug.action());

    m_showWarningButton = new QToolButton(m_consoleWidget);

    m_showWarning.setDefaultValue(true);
    m_showWarning.setSettingsKey(CONSOLE, SHOW_WARNING);
    m_showWarning.setLabelText(Tr::tr("Show warning messages."));
    m_showWarning.setToolTip(Tr::tr("Show warning messages."));
    m_showWarning.setValue(true);
    m_showWarning.action()->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
    connect(m_showWarning.action(), &QAction::toggled,
            proxyModel, [this, proxyModel] { proxyModel->setShowWarnings(m_showWarning()); });
    m_showWarningButton->setDefaultAction(m_showWarning.action());

    m_showErrorButton = new QToolButton(m_consoleWidget);

    m_showError.setDefaultValue(true);
    m_showError.setSettingsKey(CONSOLE, SHOW_ERROR);
    m_showError.setLabelText(Tr::tr("Show error messages."));
    m_showError.setToolTip(Tr::tr("Show error messages."));
    m_showError.setValue(true);
    m_showError.action()->setIcon(Utils::Icons::CRITICAL_TOOLBAR.icon());
    connect(m_showError.action(), &QAction::toggled,
            proxyModel, [this, proxyModel] { proxyModel->setShowErrors(m_showError()); });
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
    m_showDebug.readSettings();
    m_showWarning.readSettings();
    m_showError.readSettings();
}

void Console::setContext(const QString &context)
{
    m_statusLabel->setText(context);
}

void Console::writeSettings() const
{
    m_showDebug.writeSettings();
    m_showWarning.writeSettings();
    m_showError.writeSettings();
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
        auto item = new ConsoleItem(
            ConsoleItem::ErrorType, Tr::tr("Can only evaluate during a debug session."));
        m_consoleItemModel->shiftEditableRow();
        printItem(item);
    }
}

} // Debugger::Internal
