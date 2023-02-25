// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalpane.h"

#include "shellmodel.h"
#include "terminaltr.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icontext.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/utilsicons.h>

#include <QMenu>
#include <QStandardPaths>

namespace Terminal {

using namespace Utils;

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

    m_newTerminalButton->setDefaultAction(&m_newTerminal);

    m_closeTerminalButton = new QToolButton();
    m_closeTerminalButton->setDefaultAction(&m_closeTerminal);
}

void TerminalPane::openTerminal(const Utils::Terminal::OpenTerminalParameters &parameters)
{
    showPage(0);
    m_tabWidget->setCurrentIndex(
        m_tabWidget->addTab(new TerminalWidget(m_tabWidget, parameters), Tr::tr("Terminal")));

    m_tabWidget->currentWidget()->setFocus();

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
