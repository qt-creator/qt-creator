// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalpane.h"

#include "terminaltr.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icontext.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/utilsicons.h>

#include <QFileIconProvider>
#include <QMenu>
#include <QStandardPaths>

namespace Terminal {

using namespace Utils;

FilePaths availableShells()
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        FilePaths shells;

        FilePath comspec = FilePath::fromUserInput(qtcEnvironmentVariable("COMSPEC"));
        shells << comspec;

        if (comspec.fileName() != "cmd.exe") {
            FilePath cmd = FilePath::fromUserInput(QStandardPaths::findExecutable("cmd.exe"));
            shells << cmd;
        }

        FilePath powershell = FilePath::fromUserInput(
            QStandardPaths::findExecutable("powershell.exe"));
        if (powershell.exists())
            shells << powershell;

        FilePath bash = FilePath::fromUserInput(QStandardPaths::findExecutable("bash.exe"));
        if (bash.exists())
            shells << bash;

        FilePath git_bash = FilePath::fromUserInput(QStandardPaths::findExecutable("git.exe"));
        if (git_bash.exists())
            shells << git_bash.parentDir().parentDir().pathAppended("usr/bin/bash.exe");

        FilePath msys2_bash = FilePath::fromUserInput(QStandardPaths::findExecutable("msys2.exe"));
        if (msys2_bash.exists())
            shells << msys2_bash.parentDir().pathAppended("usr/bin/bash.exe");

        return shells;
    } else {
        FilePath shellsFile = FilePath::fromString("/etc/shells");
        const auto shellFileContent = shellsFile.fileContents();
        QTC_ASSERT_EXPECTED(shellFileContent, return {});

        QString shellFileContentString = QString::fromUtf8(*shellFileContent);

        // Filter out comments ...
        const QStringList lines
            = Utils::filtered(shellFileContentString.split('\n', Qt::SkipEmptyParts),
                              [](const QString &line) { return !line.trimmed().startsWith('#'); });

        // Convert lines to file paths ...
        const FilePaths shells = Utils::transform(lines, [](const QString &line) {
            return FilePath::fromUserInput(line.trimmed());
        });

        // ... and filter out non-existing shells.
        return Utils::filtered(shells, [](const FilePath &shell) { return shell.exists(); });
    }
}

TerminalPane::TerminalPane(QObject *parent)
    : Core::IOutputPane(parent)
{
    Core::Context context("Terminal.Window");

    m_newTerminal.setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_newTerminal.setToolTip(Tr::tr("Create a new Terminal."));

    connect(&m_newTerminal, &QAction::triggered, this, [this] { openTerminal({}); });

    m_closeTerminal.setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    m_closeTerminal.setToolTip(Tr::tr("Close the current Terminal."));
    m_closeTerminal.setEnabled(false);

    connect(&m_closeTerminal, &QAction::triggered, this, [this] {
        removeTab(m_tabWidget->currentIndex());
    });

    //Core::Command *cmd = Core::ActionManager::registerAction(m_newTerminal, Constants::STOP);
    //cmd->setDescription(m_newTerminal->toolTip());

    m_newTerminalButton = new QToolButton();

    QMenu *shellMenu = new QMenu(m_newTerminalButton);

    const FilePaths shells = availableShells();

    QFileIconProvider iconProvider;

    // Create an action for each available shell ...
    for (const FilePath &shell : shells) {
        const QIcon icon = iconProvider.icon(shell.toFileInfo());

        QAction *action = new QAction(icon, shell.toUserOutput(), shellMenu);
        action->setData(shell.toVariant());
        shellMenu->addAction(action);
    }
    connect(shellMenu, &QMenu::triggered, this, [this](QAction *action) {
        openTerminal(
                    Utils::Terminal::OpenTerminalParameters{CommandLine{FilePath::fromVariant(action->data()), {}},
                                                            std::nullopt,
                                                            std::nullopt,
                                                            Utils::Terminal::ExitBehavior::Close});
    });
    m_newTerminal.setMenu(shellMenu);

    m_newTerminalButton->setDefaultAction(&m_newTerminal);

    m_closeTerminalButton = new QToolButton();
    m_closeTerminalButton->setDefaultAction(&m_closeTerminal);
}

void TerminalPane::openTerminal(const Utils::Terminal::OpenTerminalParameters &parameters)
{
    showPage(0);
    m_tabWidget->setCurrentIndex(
        m_tabWidget->addTab(new TerminalWidget(m_tabWidget, parameters), Tr::tr("Terminal")));

    m_closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
}

void TerminalPane::addTerminal(TerminalWidget *terminal, const QString &title)
{
    showPage(0);
    m_tabWidget->setCurrentIndex(m_tabWidget->addTab(terminal, title));

    m_closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
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

        m_tabWidget->addTab(new TerminalWidget(parent), Tr::tr("Terminal"));
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
        t->hasFocus();

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
    return m_tabWidget->count() > 1 && m_tabWidget->currentIndex() < m_tabWidget->count() - 1;
}

bool TerminalPane::canPrevious() const
{
    return m_tabWidget->count() > 1 && m_tabWidget->currentIndex() > 0;
}

void TerminalPane::goToNext()
{
    m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() + 1);
    emit navigateStateUpdate();
}

void TerminalPane::goToPrev()
{
    m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() - 1);
    emit navigateStateUpdate();
}

} // namespace Terminal
