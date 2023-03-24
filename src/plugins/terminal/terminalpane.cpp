// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalpane.h"

#include "shellmodel.h"
#include "terminalcommands.h"
#include "terminalsettings.h"
#include "terminaltr.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/terminalhooks.h>
#include <utils/utilsicons.h>

#include <QMenu>
#include <QStandardPaths>
#include <QToolButton>

namespace Terminal {

using namespace Utils;
using namespace Utils::Terminal;

TerminalPane::TerminalPane(QObject *parent)
    : Core::IOutputPane(parent)
    , m_tabWidget(new QTabWidget)
{
    setupContext("Terminal.Pane", m_tabWidget);
    setZoomButtonsEnabled(true);

    TerminalCommands::instance().init(Core::Context("Terminal.Pane"));

    connect(this, &IOutputPane::zoomInRequested, this, [this] {
        if (currentTerminal())
            currentTerminal()->zoomIn();
    });
    connect(this, &IOutputPane::zoomOutRequested, this, [this] {
        if (currentTerminal())
            currentTerminal()->zoomOut();
    });

    QAction &newTerminal = TerminalCommands::instance().paneActions().newTerminal;
    QAction &closeTerminal = TerminalCommands::instance().paneActions().closeTerminal;

    newTerminal.setIcon(
        Icon({{":/terminal/images/terminal.png", Theme::IconsBaseColor},
              {":/utils/images/iconoverlay_add_small.png", Theme::IconsRunToolBarColor}})
            .icon());
    newTerminal.setToolTip(Tr::tr("Create a new Terminal."));

    connect(&newTerminal, &QAction::triggered, this, [this] { openTerminal({}); });

    closeTerminal.setIcon(
        Icon({{":/terminal/images/terminal.png", Theme::IconsBaseColor},
              {":/utils/images/iconoverlay_close_small.png", Theme::IconsStopToolBarColor}})
            .icon());
    closeTerminal.setToolTip(Tr::tr("Close the current Terminal."));
    closeTerminal.setEnabled(false);

    connect(&closeTerminal, &QAction::triggered, this, [this] {
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

    newTerminal.setMenu(shellMenu);

    m_newTerminalButton->setDefaultAction(&newTerminal);

    m_closeTerminalButton = new QToolButton();
    m_closeTerminalButton->setDefaultAction(&closeTerminal);

    connect(&TerminalCommands::instance().paneActions().nextTerminal,
            &QAction::triggered,
            this,
            [this] {
                if (canNavigate())
                    goToNext();
            });
    connect(&TerminalCommands::instance().paneActions().prevTerminal,
            &QAction::triggered,
            this,
            [this] {
                if (canPrevious())
                    goToPrev();
            });

    connect(&TerminalCommands::instance().paneActions().minMax, &QAction::triggered, this, []() {
        Core::Command *minMaxCommand = Core::ActionManager::command("Coreplugin.OutputPane.minmax");
        if (minMaxCommand)
            emit minMaxCommand->action()->triggered();
    });

    m_openSettingsButton = new QToolButton();
    m_openSettingsButton->setToolTip(Tr::tr("Open Terminal Settings"));
    m_openSettingsButton->setIcon(Icons::SETTINGS_TOOLBAR.icon());

    connect(m_openSettingsButton, &QToolButton::clicked, m_openSettingsButton, []() {
        TerminalCommands::openSettingsAction()->trigger();
    });

    auto updateEscButton = [this] {
        m_escSettingButton->setChecked(TerminalSettings::instance().sendEscapeToTerminal.value());
        if (TerminalSettings::instance().sendEscapeToTerminal.value()) {
            m_escSettingButton->setText("⎋");
            m_escSettingButton->setToolTip(Tr::tr("Sending ESC to terminal instead of Qt Creator"));
        } else {
            m_escSettingButton->setText("⇧+⎋");
            m_escSettingButton->setToolTip(Tr::tr("Press ⇧+⎋ to send ESC to terminal"));
        }
    };

    m_escSettingButton = new QToolButton();
    m_escSettingButton->setCheckable(true);

    updateEscButton();

    connect(m_escSettingButton, &QToolButton::toggled, this, [this] {
        TerminalSettings::instance().sendEscapeToTerminal.setValue(m_escSettingButton->isChecked());
        TerminalSettings::instance().apply();
        TerminalSettings::instance().writeSettings(Core::ICore::settings());
    });

    connect(&TerminalSettings::instance(), &TerminalSettings::applied, this, updateEscButton);
}

TerminalPane::~TerminalPane()
{
    delete m_tabWidget;
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

    TerminalCommands::instance().paneActions().closeTerminal.setEnabled(m_tabWidget->count() > 1);
    emit navigateStateUpdate();
}

void TerminalPane::addTerminal(TerminalWidget *terminal, const QString &title)
{
    showPage(0);
    m_tabWidget->setCurrentIndex(m_tabWidget->addTab(terminal, title));
    setupTerminalWidget(terminal);

    TerminalCommands::instance().paneActions().closeTerminal.setEnabled(m_tabWidget->count() > 1);
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
    if (!m_widgetInitialized) {
        m_widgetInitialized = true;
        m_tabWidget->setTabBarAutoHide(false);
        m_tabWidget->setDocumentMode(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setMovable(true);

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
            removeTab(index);
        });

        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
            if (auto widget = m_tabWidget->widget(index))
                widget->setFocus();
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

    TerminalCommands::instance().paneActions().closeTerminal.setEnabled(m_tabWidget->count() > 1);

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

    connect(terminal, &TerminalWidget::started, this, [setTabText, terminal](qint64 /*pid*/) {
        setTabText(terminal);
    });

    connect(terminal, &TerminalWidget::cwdChanged, this, [setTabText, terminal]() {
        setTabText(terminal);
    });

    connect(terminal, &TerminalWidget::commandChanged, this, [setTabText, terminal]() {
        setTabText(terminal);
    });

    if (!terminal->shellName().isEmpty())
        setTabText(terminal);
}

QList<QWidget *> TerminalPane::toolBarWidgets() const
{
    QList<QWidget *> widgets = IOutputPane::toolBarWidgets();
    widgets.prepend(m_newTerminalButton);
    widgets.prepend(m_closeTerminalButton);

    return widgets << m_openSettingsButton << m_escSettingButton;
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
