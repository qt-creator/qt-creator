// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalpane.h"

#include "shellmodel.h"
#include "terminaltr.h"
#include "terminalwidget.h"
#include "utils/terminalhooks.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icontext.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/utilsicons.h>

#include <QMenu>
#include <QStandardPaths>

namespace Terminal {

using namespace Utils;
using namespace Utils::Terminal;

TerminalPane::TerminalPane(QObject *parent)
    : Core::IOutputPane(parent)
{
    Core::Context context("Terminal.Window");

    m_newTerminal.setIcon(Icons::PLUS_TOOLBAR.icon());
    m_newTerminal.setToolTip(Tr::tr("Create a new Terminal."));

    connect(&m_newTerminal, &QAction::triggered, this, [this] { openTerminal({}); });

    m_closeTerminal.setIcon(Icons::CLOSE_TOOLBAR.icon());
    m_closeTerminal.setToolTip(Tr::tr("Close the current Terminal."));
    m_closeTerminal.setEnabled(false);

    connect(&m_closeTerminal, &QAction::triggered, this, [this] {
        removeTab(m_tabWidget->currentIndex());
    });

    m_newTerminalButton = new QToolButton();

    QMenu *shellMenu = new QMenu(m_newTerminalButton);
    Internal::ShellModel *shellModel = new Internal::ShellModel(shellMenu);
    connect(shellMenu, &QMenu::aboutToShow, shellMenu, [shellMenu, shellModel, pane = this] {
        shellMenu->clear();

        const auto addItems = [shellMenu, pane](const QList<Internal::ShellModelItem> &items) {
            for (const Internal::ShellModelItem &item : items) {
                QAction *action = new QAction(item.icon, item.name, shellMenu);

                connect(action, &QAction::triggered, action, [item, pane]() {
                    pane->openTerminal(item.openParameters);
                });

                shellMenu->addAction(action);
            }
        };

        addItems(shellModel->local());
        shellMenu->addSection(Tr::tr("Devices"));
        addItems(shellModel->remote());
    });

    m_newTerminal.setMenu(shellMenu);
    m_newTerminal.setShortcut(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+T") : QLatin1String("Ctrl+Shift+T")));

    m_newTerminalButton->setDefaultAction(&m_newTerminal);

    m_closeTerminalButton = new QToolButton();
    m_closeTerminalButton->setDefaultAction(&m_closeTerminal);

    m_nextTerminal.setShortcut(QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+[")
                                                                    : QLatin1String("Ctrl+PgUp")));
    m_prevTerminal.setShortcut(QKeySequence(HostOsInfo::isMacHost()
                                                ? QLatin1String("Ctrl+Shift+]")
                                                : QLatin1String("Ctrl+PgDown")));

    connect(&m_nextTerminal, &QAction::triggered, this, [this] {
        if (canNavigate())
            goToNext();
    });
    connect(&m_prevTerminal, &QAction::triggered, this, [this] {
        if (canPrevious())
            goToPrev();
    });

    m_minMaxAction.setShortcut(QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Return")
                                                                    : QLatin1String("Alt+Return")));

    connect(&m_minMaxAction, &QAction::triggered, this, []() {
        Core::Command *minMaxCommand = Core::ActionManager::command("Coreplugin.OutputPane.minmax");
        if (minMaxCommand)
            emit minMaxCommand->action()->triggered();
    });
}

static std::optional<FilePath> startupProjectDirectory()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return std::nullopt;

    return project->projectDirectory();
}

void TerminalPane::openTerminal(const OpenTerminalParameters &parameters)
{
    OpenTerminalParameters parametersCopy{parameters};
    showPage(0);

    if (!parametersCopy.workingDirectory) {
        const std::optional<FilePath> projectDir = startupProjectDirectory();
        if (projectDir) {
            if (!parametersCopy.shellCommand
                || parametersCopy.shellCommand->executable().ensureReachable(*projectDir)) {
                parametersCopy.workingDirectory = *projectDir;
            }
        }
    }

    auto terminalWidget = new TerminalWidget(m_tabWidget, parametersCopy);
    m_tabWidget->setCurrentIndex(m_tabWidget->addTab(terminalWidget, Tr::tr("Terminal")));
    setupTerminalWidget(terminalWidget);

    m_tabWidget->currentWidget()->setFocus();

    m_closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
}

void TerminalPane::addTerminal(TerminalWidget *terminal, const QString &title)
{
    showPage(0);
    m_tabWidget->setCurrentIndex(m_tabWidget->addTab(terminal, title));
    setupTerminalWidget(terminal);

    m_closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
}

TerminalWidget *TerminalPane::stoppedTerminalWithId(const Id &identifier) const
{
    QTC_ASSERT(m_tabWidget, return nullptr);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto terminal = qobject_cast<TerminalWidget *>(m_tabWidget->widget(i));
        if (terminal->processState() == QProcess::NotRunning && terminal->identifier() == identifier)
            return terminal;
    }

    return nullptr;
}

QWidget *TerminalPane::outputWidget(QWidget *parent)
{
    if (!m_tabWidget) {
        m_tabWidget = new QTabWidget(parent);

        m_tabWidget->setTabBarAutoHide(true);
        m_tabWidget->setDocumentMode(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setMovable(true);

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
            removeTab(index);
        });

        auto terminalWidget = new TerminalWidget(parent);
        m_tabWidget->addTab(terminalWidget, Tr::tr("Terminal"));
        setupTerminalWidget(terminalWidget);
    }

    return m_tabWidget;
}

TerminalWidget *TerminalPane::currentTerminal() const
{
    QWidget *activeWidget = m_tabWidget->currentWidget();
    return static_cast<TerminalWidget *>(activeWidget);
}

void TerminalPane::removeTab(int index)
{
    if (m_tabWidget->count() > 1)
        delete m_tabWidget->widget(index);

    m_closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
}

void TerminalPane::setupTerminalWidget(TerminalWidget *terminal)
{
    if (!terminal)
        return;

    auto setTabText = [this](TerminalWidget *terminal) {
        auto index = m_tabWidget->indexOf(terminal);
        const FilePath cwd = terminal->cwd();

        const QString exe = terminal->currentCommand().isEmpty()
                                ? terminal->shellName()
                                : terminal->currentCommand().executable().fileName();

        if (cwd.isEmpty())
            m_tabWidget->setTabText(index, exe);
        else
            m_tabWidget->setTabText(index, exe + " - " + cwd.fileName());
    };

    connect(terminal, &TerminalWidget::started, [setTabText, terminal](qint64 /*pid*/) {
        setTabText(terminal);
    });

    connect(terminal, &TerminalWidget::cwdChanged, [setTabText, terminal]() {
        setTabText(terminal);
    });

    connect(terminal, &TerminalWidget::commandChanged, [setTabText, terminal]() {
        setTabText(terminal);
    });

    if (!terminal->shellName().isEmpty())
        setTabText(terminal);

    terminal->addActions({&m_newTerminal, &m_nextTerminal, &m_prevTerminal, &m_minMaxAction});
}

QList<QWidget *> TerminalPane::toolBarWidgets() const
{
    return {m_newTerminalButton, m_closeTerminalButton};
}

QString TerminalPane::displayName() const
{
    return Tr::tr("Terminal");
}

int TerminalPane::priorityInStatusBar() const
{
    return 50;
}

void TerminalPane::clearContents()
{
    if (const auto t = currentTerminal())
        t->clearContents();
}

void TerminalPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible);
}

void TerminalPane::setFocus()
{
    if (const auto t = currentTerminal())
        t->setFocus();
}

bool TerminalPane::hasFocus() const
{
    if (const auto t = currentTerminal())
        return t->hasFocus();

    return false;
}

bool TerminalPane::canFocus() const
{
    return true;
}

bool TerminalPane::canNavigate() const
{
    return true;
}

bool TerminalPane::canNext() const
{
    return m_tabWidget->count() > 1;
}

bool TerminalPane::canPrevious() const
{
    return m_tabWidget->count() > 1;
}

void TerminalPane::goToNext()
{
    int nextIndex = m_tabWidget->currentIndex() + 1;
    if (nextIndex >= m_tabWidget->count())
        nextIndex = 0;

    m_tabWidget->setCurrentIndex(nextIndex);
    emit navigateStateUpdate();
}

void TerminalPane::goToPrev()
{
    int prevIndex = m_tabWidget->currentIndex() - 1;
    if (prevIndex < 0)
        prevIndex = m_tabWidget->count() - 1;

    m_tabWidget->setCurrentIndex(prevIndex);
    emit navigateStateUpdate();
}

} // namespace Terminal
