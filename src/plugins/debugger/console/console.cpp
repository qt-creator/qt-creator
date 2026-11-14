// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "console.h"

#include "consoleview.h"
#include "consoleproxymodel.h"
#include "consoleitemdelegate.h"
#include "../debuggerconstants.h"
#include "../debuggerengine.h"
#include "../debuggertr.h"
#include "../enginemanager.h"

#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QStringView>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

const char CONSOLE[] = "Console";
const char SHOW_LOG[] = "showLog";
const char SHOW_WARNING[] = "showWarning";
const char SHOW_ERROR[] = "showError";

using namespace Utils;

namespace Debugger::Internal {

/////////////////////////////////////////////////////////////////////
//
// Console
//
/////////////////////////////////////////////////////////////////////

Console::Console()
{
    setId("QMLDebuggerConsole"); // Keep the pane id for existing layouts.
    setDisplayName(Tr::tr("Debugger Console"));
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
    m_consoleView->setSearchRole(Qt::DisplayRole);

    vbox->addWidget(m_consoleView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(m_consoleWidget));

    m_showDebugButton = new QToolButton(m_consoleWidget);

    m_showDebug.setDefaultValue(true);
    m_showDebug.setSettingsKey(CONSOLE, SHOW_LOG);
    m_showDebug.setLabelText(Tr::tr("Show debug, log, and info messages."));
    m_showDebug.setToolTip(Tr::tr("Show debug, log, and info messages."));
    m_showDebug.setValue(true);
    m_showDebug.setIcon(Icons::INFO_TOOLBAR.icon());
    connect(&m_showDebug, &BoolAspect::changed,
            proxyModel, [this, proxyModel] { proxyModel->setShowLogs(m_showDebug()); });
    m_showDebugButton->setDefaultAction(m_showDebug.action());

    m_showWarningButton = new QToolButton(m_consoleWidget);

    m_showWarning.setDefaultValue(true);
    m_showWarning.setSettingsKey(CONSOLE, SHOW_WARNING);
    m_showWarning.setLabelText(Tr::tr("Show warning messages."));
    m_showWarning.setToolTip(Tr::tr("Show warning messages."));
    m_showWarning.setValue(true);
    m_showWarning.setIcon(Icons::WARNING_TOOLBAR.icon());
    connect(&m_showWarning, &BoolAspect::changed,
            proxyModel, [this, proxyModel] { proxyModel->setShowWarnings(m_showWarning()); });
    m_showWarningButton->setDefaultAction(m_showWarning.action());

    m_showErrorButton = new QToolButton(m_consoleWidget);

    m_showError.setDefaultValue(true);
    m_showError.setSettingsKey(CONSOLE, SHOW_ERROR);
    m_showError.setLabelText(Tr::tr("Show error messages."));
    m_showError.setToolTip(Tr::tr("Show error messages."));
    m_showError.setValue(true);
    m_showError.setIcon(Icons::CRITICAL_TOOLBAR.icon());
    connect(&m_showError, &BoolAspect::changed,
            proxyModel, [this, proxyModel] { proxyModel->setShowErrors(m_showError()); });
    m_showErrorButton->setDefaultAction(m_showError.action());

    m_spacer = new QWidget(m_consoleWidget);
    m_spacer->setMinimumWidth(30);

    m_statusLabel = new QLabel(m_consoleWidget);
    StyleHelper::setPanelWidget(m_statusLabel);
    m_modeSelector = new QComboBox(m_consoleWidget);
    m_modeSelector->addItem(Tr::tr("QML / JavaScript"), int(ConsoleMode::Qml));
    m_modeSelector->addItem(Tr::tr("Debugger commands"), int(ConsoleMode::Backend));
    connect(m_modeSelector, &QComboBox::currentIndexChanged, this, [this] {
        m_consoleItemModel->setHistoryContext(int(mode()));
    });
    connect(EngineManager::instance(), &EngineManager::currentEngineChanged, this, [this] {
        updateModes();
        const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
        // Switching to Debug mode can announce its preset before the new engine.
        if (!engine || engine->state() == DebuggerFinished)
            return;
        const bool keepFocus = std::exchange(m_keepFocusForStartupDebugging, false);
        if (!keepFocus && !m_showConsoleInDebugMode)
            return;
        DebuggerConsoleCommandAdapter *adapter = engine->consoleCommandAdapter();
        if (!adapter) {
            clearStartupRequest();
            return;
        }
        m_focusOnFirstLocation = keepFocus;
        if (Core::ModeManager::currentModeId() == Constants::MODE_DEBUG) {
            m_showConsoleInDebugMode = false;
            scheduleStartupFocus(m_startupRequestGeneration);
        }
    });
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
            this, [this](Id mode, Id) {
        if (!m_showConsoleInDebugMode || mode != Constants::MODE_DEBUG)
            return;
        m_showConsoleInDebugMode = false;
        scheduleStartupFocus(m_startupRequestGeneration);
    });
    updateModes();

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
     return {m_modeSelector, m_showDebugButton, m_showWarningButton, m_showErrorButton,
             m_spacer, m_statusLabel};
}

void Console::clearContents()
{
    m_pendingCommandOutput.clear();
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
    scheduleInputFocus();
}

void Console::scheduleInputFocus()
{
    QTimer::singleShot(0, m_consoleWidget, [this] {
        if (m_consoleWidget->isHidden())
            return;
        m_consoleView->setFocus();
        m_consoleView->focusInput();
    });
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

void Console::setScriptEvaluator(QObject *owner, const ScriptEvaluator &evaluator)
{
    if (evaluator) {
        const quint64 generation = ++m_scriptEvaluatorGeneration;
        m_scriptEvaluatorOwner = owner;
        m_scriptEvaluator = evaluator;
        connect(owner, &QObject::destroyed, this, [this, generation] {
            if (generation != m_scriptEvaluatorGeneration)
                return;
            m_scriptEvaluatorOwner.clear();
            m_scriptEvaluator = {};
            m_consoleItemModel->setCanFetchMore(false);
            setContext(QString());
            updateModes();
        });
    } else if (m_scriptEvaluatorOwner == owner) {
        ++m_scriptEvaluatorGeneration;
        m_scriptEvaluatorOwner.clear();
        m_scriptEvaluator = {};
    }
    const bool canUseQml = m_scriptEvaluatorOwner && m_scriptEvaluator;
    m_consoleItemModel->setCanFetchMore(canUseQml);
    updateModes();
    if (!canUseQml)
        setContext(QString());
}

void Console::updateModes()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    disconnect(m_engineConsoleOutputConnection);
    if (engine) {
        m_engineConsoleOutputConnection = connect(
            engine, &DebuggerEngine::consoleOutputRequested, this,
            [this](const QString &message) {
                printCommandOutput(message.endsWith('\n') ? message : message + '\n',
                                   ConsoleItem::DefaultType);
            });
    }
    if (engine != m_navigationEngine) {
        disconnect(m_navigationStartedConnection);
        disconnect(m_navigationFinishedConnection);
        m_navigationEngine = engine;
        m_navigationDepth = 0;
        m_restoreNavigationFocus = false;
        m_focusOnFirstLocation = false;
        if (engine) {
            m_navigationStartedConnection = connect(
                engine, &DebuggerEngine::locationNavigationStarted, this, [this] {
                    if (m_navigationDepth++ == 0)
                        m_restoreNavigationFocus = hasFocus() || m_focusOnFirstLocation;
                });
            m_navigationFinishedConnection = connect(
                engine, &DebuggerEngine::locationNavigationFinished, this, [this] {
                    if (m_navigationDepth == 0 || --m_navigationDepth != 0)
                        return;
                    m_focusOnFirstLocation = false;
                    if (std::exchange(m_restoreNavigationFocus, false))
                        scheduleInputFocus();
                });
        }
    }
    const bool canUseQml = m_scriptEvaluatorOwner && m_scriptEvaluator;
    if (engine || canUseQml)
        m_consoleView->populateFileFinder();

    DebuggerConsoleCommandAdapter *adapter = engine ? engine->consoleCommandAdapter() : nullptr;
    const bool canUseBackend = adapter;
    if (auto *model = qobject_cast<QStandardItemModel *>(m_modeSelector->model())) {
        if (QStandardItem *qmlMode = model->item(int(ConsoleMode::Qml)))
            qmlMode->setEnabled(canUseQml);
        if (QStandardItem *backendMode = model->item(int(ConsoleMode::Backend))) {
            backendMode->setEnabled(canUseBackend);
            backendMode->setText(adapter ? adapter->commandModeName() : Tr::tr("Debugger commands"));
        }
    }
    m_modeSelector->setEnabled(canUseQml || canUseBackend);

    if ((mode() == ConsoleMode::Qml && !canUseQml)
        || (mode() == ConsoleMode::Backend && !canUseBackend)) {
        if (canUseQml)
            m_modeSelector->setCurrentIndex(int(ConsoleMode::Qml));
        else
            m_modeSelector->setCurrentIndex(int(ConsoleMode::Backend));
    }
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
    // Only draw attention to a message type the user has not filtered out:
    // popping the pane up for errors that are hidden anyway is just clutter.
    if (item->itemType() == ConsoleItem::ErrorType) {
        if (m_showError())
            popup(Core::IOutputPane::ModeSwitch);
    } else if (item->itemType() == ConsoleItem::WarningType) {
        if (m_showWarning())
            flash();
    }
}

void Console::printCommandOutput(const QString &output, ConsoleItem::ItemType itemType)
{
    if (output.isEmpty())
        return;

    m_pendingCommandOutput.enqueue({output, itemType});
    if (m_commandOutputScheduled)
        return;

    m_commandOutputScheduled = true;
    QTimer::singleShot(0, this, &Console::appendCommandOutputChunk);
}

void Console::appendCommandOutputChunk()
{
    static const QRegularExpression locationAfterAt(
        R"(\bat\s+(?<file>.+?):(?<line>\d+)(?::\d+)?\s*$)");
    static const QRegularExpression fileAndLine(
        R"(\bfile\s+(?<file>.+),\s+line\s+(?<line>\d+)\.?\s*$)");
    static const QRegularExpression absoluteLocation(
        R"((?<file>(?:[A-Za-z]:)?[/\\][^\n:]+?):(?<line>\d+)(?::\d+)?(?=$|\s|[:,)]))");
    static const QRegularExpression relativeLocation(
        R"((?<file>[^\s:()]+):(?<line>\d+)(?::\d+)?(?=$|\s|[:,)]))");

    constexpr qsizetype maxItemsPerChunk = 50;
    constexpr qsizetype maxCharactersPerItem = 8192;
    qsizetype itemsAdded = 0;
    while (!m_pendingCommandOutput.isEmpty() && itemsAdded < maxItemsPerChunk) {
        PendingCommandOutput &pending = m_pendingCommandOutput.head();
        const qsizetype remaining = pending.text.size() - pending.position;
        const qsizetype searchLength = qMin(remaining, maxCharactersPerItem + 1);
        const qsizetype newlineOffset = QStringView(pending.text)
                                            .sliced(pending.position, searchLength)
                                            .indexOf('\n');
        const bool hasNewline = newlineOffset >= 0 && newlineOffset <= maxCharactersPerItem;
        const qsizetype segmentLength = hasNewline ? newlineOffset
                                                   : qMin(remaining, maxCharactersPerItem);
        const bool completeLine = hasNewline || segmentLength == remaining;
        qsizetype segmentEnd = pending.position + segmentLength;
        if (!hasNewline && segmentLength < remaining && segmentLength > 0
            && pending.text.at(segmentEnd - 1).isHighSurrogate()) {
            --segmentEnd;
        }
        QString line = pending.text.mid(pending.position, segmentEnd - pending.position);
        pending.position = segmentEnd + (hasNewline ? 1 : 0);
        const bool isFinished = pending.position >= pending.text.size();
        const ConsoleItem::ItemType itemType = pending.itemType;
        if (isFinished)
            m_pendingCommandOutput.dequeue();

        if (line.endsWith('\r'))
            line.chop(1);

        QRegularExpressionMatch match;
        if (completeLine) {
            match = locationAfterAt.match(line);
            if (!match.hasMatch())
                match = fileAndLine.match(line);
            if (!match.hasMatch())
                match = absoluteLocation.match(line);
            if (!match.hasMatch())
                match = relativeLocation.match(line);
        }

        if (match.hasMatch()) {
            const QString file = match.captured("file").trimmed();
            const int lineNumber = match.captured("line").toInt();
            printItem(new ConsoleItem(itemType, line, file, qMax(0, lineNumber)));
        } else {
            printItem(itemType, line);
        }
        ++itemsAdded;
    }

    if (m_pendingCommandOutput.isEmpty()) {
        m_commandOutputScheduled = false;
        return;
    }
    QTimer::singleShot(0, this, &Console::appendCommandOutputChunk);
}

void Console::clearStartupRequest()
{
    ++m_startupRequestGeneration;
    m_keepFocusForStartupDebugging = false;
    m_showConsoleInDebugMode = false;
    m_focusOnFirstLocation = false;
}

void Console::scheduleStartupFocus(quint64 generation)
{
    QTimer::singleShot(0, this, [this, generation] {
        const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
        if (generation == m_startupRequestGeneration
            && Core::ModeManager::currentModeId() == Constants::MODE_DEBUG
            && engine && engine->state() != DebuggerFinished && engine->consoleCommandAdapter()) {
            emit showPage(WithFocus);
        }
    });
}

void Console::evaluate(const QString &expression)
{
    const bool keepConsoleFocus = hasFocus();
    const QString command = expression.trimmed();
    const bool startsAtMain = command.compare("start", Qt::CaseInsensitive) == 0;
    const QPointer<DebuggerEngine> currentEngine = EngineManager::currentEngine();
    const bool startsStartupProject
        = (startsAtMain || command.compare("run", Qt::CaseInsensitive) == 0)
          && (!currentEngine || currentEngine->state() == DebuggerFinished);
    m_consoleItemModel->shiftEditableRow(int(mode()));
    if (keepConsoleFocus)
        scheduleInputFocus();

    if (startsStartupProject) {
        const auto canRun = ProjectExplorer::ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::DEBUG_RUN_MODE);
        if (!canRun) {
            printItem(ConsoleItem::ErrorType, canRun.error());
            return;
        }
        const quint64 generation = ++m_startupRequestGeneration;
        const QPointer<Console> console(this);
        ProjectExplorer::ProjectExplorerPlugin::runStartupProjectWithCallback(
            ProjectExplorer::Constants::DEBUG_RUN_MODE,
            [console, generation, keepConsoleFocus, startsAtMain](ProjectExplorer::RunControl *runControl) {
                if (!console || generation != console->m_startupRequestGeneration)
                    return;
                if (!runControl) {
                    console->clearStartupRequest();
                    return;
                }
                QVariantHash extraData = runControl->extraData();
                extraData.insert(Constants::BREAK_ON_MAIN, startsAtMain);
                runControl->setExtraData(extraData);
                console->m_keepFocusForStartupDebugging = keepConsoleFocus;
                console->m_showConsoleInDebugMode = keepConsoleFocus;
                const auto clear = [console, generation] {
                    if (console && generation == console->m_startupRequestGeneration)
                        console->clearStartupRequest();
                };
                connect(runControl, &ProjectExplorer::RunControl::stopped, console.data(), clear);
                connect(runControl, &QObject::destroyed, console.data(), clear);
            });
        return;
    }

    switch (mode()) {
    case ConsoleMode::Qml:
        if (m_scriptEvaluatorOwner && m_scriptEvaluator)
            m_scriptEvaluator(expression);
        else
            printItem(ConsoleItem::ErrorType, Tr::tr("No active QML debugger session."));
        break;
    case ConsoleMode::Backend: {
        DebuggerConsoleCommandAdapter *adapter = currentEngine
                                                     ? currentEngine->consoleCommandAdapter()
                                                     : nullptr;
        if (adapter) {
            const QPointer<Console> console(this);
            adapter->executeCommand(expression,
                                    [console](const QString &output,
                                              DebuggerConsoleOutputKind outputKind) {
                if (!console)
                    return;
                QMetaObject::invokeMethod(
                    console.data(), [console, output, outputKind] {
                        if (!console)
                            return;
                        const ConsoleItem::ItemType itemType
                            = outputKind == DebuggerConsoleOutputKind::Error
                                  ? ConsoleItem::ErrorType
                                  : ConsoleItem::DefaultType;
                        console->printCommandOutput(output, itemType);
                    }, Qt::QueuedConnection);
            });
        } else {
            printItem(ConsoleItem::ErrorType,
                      Tr::tr("No active debugger command adapter."));
        }
        break;
    }
    }
}

ConsoleMode Console::mode() const
{
    return ConsoleMode(m_modeSelector->currentData().toInt());
}

} // Debugger::Internal
