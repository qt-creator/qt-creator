// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalpane.h"

#include "shellmodel.h"
#include "terminalconstants.h"
#include "terminalicons.h"
#include "terminalsettings.h"
#include "terminaltr.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/terminalhooks.h>
#include <utils/utilsicons.h>

#include <QFileIconProvider>
#include <QGuiApplication>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QToolButton>

namespace Terminal {

using namespace Utils;
using namespace Utils::Terminal;
using namespace Core;

static QAbstractItemModel *macroModel(QObject *parent);

namespace {
enum MacroModelRoles { IsPrefixRole = Qt::UserRole + 1, ValueRole = Qt::UserRole + 2 };
}

TerminalPane::TerminalPane(QObject *parent)
    : IOutputPane(parent)
    , m_closeCurrentTabAction(new QAction(Tr::tr("Close Tab"), this))
    , m_closeAllTabsAction(new QAction(Tr::tr("Close All Tabs"), this))
    , m_closeOtherTabsAction(new QAction(Tr::tr("Close Other Tabs"), this))
    , m_selfContext("Terminal.Pane")
{
    setId("Terminal");
    setDisplayName(Tr::tr("Terminal"));
    setPriorityInStatusBar(20);

    setupContext(m_selfContext, &m_tabWidget);
    setZoomButtonsEnabled(true);

    connect(this, &IOutputPane::zoomInRequested, this, [this] {
        if (currentTerminal())
            currentTerminal()->zoomIn();
    });
    connect(this, &IOutputPane::zoomOutRequested, this, [this] {
        if (currentTerminal())
            currentTerminal()->zoomOut();
    });

    m_tabWidget.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
        &m_tabWidget,
        &QWidget::customContextMenuRequested,
        this,
        &TerminalPane::contextMenuRequested);

    createShellMenu();
    initActions();

    m_newTerminalButton = new QToolButton();
    m_newTerminalButton->setDefaultAction(m_newTerminalAction);
    m_newTerminalButton->setMenu(&m_shellMenu);
    m_newTerminalButton->setPopupMode(QToolButton::MenuButtonPopup);

    m_closeTerminalButton = new QToolButton();
    m_closeTerminalButton->setDefaultAction(m_closeTerminalAction);

    m_openSettingsButton = new QToolButton();
    m_openSettingsButton->setToolTip(Tr::tr("Configure..."));
    m_openSettingsButton->setIcon(Icons::SETTINGS_TOOLBAR.icon());

    connect(m_openSettingsButton, &QToolButton::clicked, m_openSettingsButton, []() {
        ICore::showSettings("Terminal.General");
    });

    m_escSettingButton = new QToolButton();
    m_escSettingButton->setDefaultAction(settings().sendEscapeToTerminal.action());

    m_lockKeyboardButton = new QToolButton();
    m_lockKeyboardButton->setDefaultAction(m_toggleKeyboardLockAction);

    m_variablesButton = new QToolButton();
    m_variablesButton->setText("%{...}");
    m_variablesButton->setToolTip(Tr::tr("Insert Macro Variable"));
    m_variablesButton->setPopupMode(QToolButton::InstantPopup);
    QMenu *variableMenu = new QMenu();
    variableMenu->setToolTipsVisible(true);

    m_variablesButton->setMenu(variableMenu);

    connect(variableMenu, &QMenu::aboutToShow, variableMenu, [this, variableMenu] {
        variableMenu->clear();
        const auto model = macroModel(variableMenu);
        for (int i = 0; i < model->rowCount(); ++i) {
            const QModelIndex index = model->index(i, 0);
            if (index.data(Qt::AccessibleDescriptionRole).toString() == "separator") {
                variableMenu->addSeparator();
                continue;
            }
            QString display = index.data(Qt::DisplayRole).toString();
            QAction *action = variableMenu->addAction(display);
            action->setData(index.data(MacroModelRoles::ValueRole));
            action->setToolTip(index.data(Qt::ToolTipRole).toString());
            action->setEnabled(index.flags() & Qt::ItemIsEnabled);
            connect(action, &QAction::triggered, action, [this, action] {
                if (auto t = currentTerminal()) {
                    QString txt = action->data().toString();
                    if (txt.contains(' '))
                        txt.prepend('"').append('"');
                    t->paste(txt);
                    t->setFocus();
                }
            });
        }
    });
}

TerminalPane::~TerminalPane() {}

static std::optional<FilePath> startupProjectDirectory()
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return std::nullopt;

    return project->projectDirectory();
}

void TerminalPane::openTerminal(const OpenTerminalParameters &parameters)
{
    OpenTerminalParameters parametersCopy{parameters};

    if (!parametersCopy.workingDirectory) {
        const std::optional<FilePath> projectDir = startupProjectDirectory();
        if (projectDir) {
            if (!parametersCopy.shellCommand) {
                parametersCopy.workingDirectory = *projectDir;
            } else if (parametersCopy.shellCommand->executable().ensureReachable(*projectDir)) {
                parametersCopy.workingDirectory
                    = parametersCopy.shellCommand->executable().withNewMappedPath(*projectDir);
            }
        }
    }

    if (parametersCopy.workingDirectory && !parametersCopy.workingDirectory->isLocal()
        && !parametersCopy.shellCommand) {
        const FilePath shell = parametersCopy.workingDirectory->withNewPath(
            parametersCopy.environment
                .value_or(parametersCopy.workingDirectory->deviceEnvironment())
                .value_or("SHELL", "/bin/sh"));
        if (!shell.isExecutableFile())
            parametersCopy.workingDirectory.reset();
        else
            parametersCopy.shellCommand = CommandLine{shell};
    }

    const auto terminalWidget = new TerminalWidget(&m_tabWidget, parametersCopy);

    using namespace Constants;
    terminalWidget->unlockGlobalAction("Coreplugin.OutputPane.minmax");
    terminalWidget->unlockGlobalAction(NEWTERMINAL);
    terminalWidget->unlockGlobalAction(NEXTTERMINAL);
    terminalWidget->unlockGlobalAction(PREVTERMINAL);

    QIcon icon;
    if (HostOsInfo::isWindowsHost()) {
        icon = parametersCopy.icon;
        if (icon.isNull()) {
            QFileIconProvider iconProvider;
            const FilePath command = parametersCopy.shellCommand
                                         ? parametersCopy.shellCommand->executable()
                                         : settings().shell();
            icon = iconProvider.icon(command.toFileInfo());
        }
    }
    m_tabWidget.setCurrentIndex(m_tabWidget.addTab(terminalWidget, icon, Tr::tr("Terminal")));
    setupTerminalWidget(terminalWidget);

    if (!m_isVisible)
        emit showPage(IOutputPane::ModeSwitch);

    m_tabWidget.currentWidget()->setFocus();

    emit navigateStateUpdate();
}

void TerminalPane::addTerminal(TerminalWidget *terminal, const QString &title)
{
    m_tabWidget.setCurrentIndex(m_tabWidget.addTab(terminal, title));
    setupTerminalWidget(terminal);

    if (!m_isVisible)
        emit showPage(IOutputPane::ModeSwitch);

    terminal->setFocus();

    emit navigateStateUpdate();
}

void TerminalPane::ensureVisible(TerminalWidget *terminal)
{
    if (!m_isVisible)
        emit showPage(IOutputPane::ModeSwitch);
    m_tabWidget.setCurrentWidget(terminal);
    terminal->setFocus();
}

TerminalWidget *TerminalPane::stoppedTerminalWithId(Id identifier) const
{
    for (int i = 0; i < m_tabWidget.count(); ++i) {
        const auto terminal = qobject_cast<TerminalWidget *>(m_tabWidget.widget(i));
        if (terminal && terminal->processState() == QProcess::NotRunning
            && terminal->identifier() == identifier)
            return terminal;
    }

    return nullptr;
}

QWidget *TerminalPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    if (!m_widgetInitialized) {
        m_widgetInitialized = true;
        m_tabWidget.setTabBarAutoHide(false);
        m_tabWidget.setDocumentMode(true);
        m_tabWidget.setTabsClosable(true);
        m_tabWidget.setMovable(true);

        connect(&m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
            removeTab(index);
        });

        connect(&m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
            if (auto widget = m_tabWidget.widget(index))
                widget->setFocus();
            else
                emit hidePage();
        });
    }

    return &m_tabWidget;
}

TerminalWidget *TerminalPane::currentTerminal() const
{
    return static_cast<TerminalWidget *>(m_tabWidget.currentWidget());
}

void TerminalPane::removeTab(int index)
{
    delete m_tabWidget.widget(index);
    emit navigateStateUpdate();
}

void TerminalPane::setupTerminalWidget(TerminalWidget *terminal)
{
    if (!terminal)
        return;

    const auto setTabText = [this, terminal] {
        const int index = m_tabWidget.indexOf(terminal);
        m_tabWidget.setTabText(index, terminal->title());
    };

    connect(terminal, &TerminalWidget::started, this, setTabText);
    connect(terminal, &TerminalWidget::cwdChanged, this, setTabText);
    connect(terminal, &TerminalWidget::commandChanged, this, setTabText);
    connect(terminal, &TerminalWidget::titleChanged, this, setTabText);

    if (!terminal->shellName().isEmpty())
        setTabText();
}

void TerminalPane::initActions()
{
    using namespace Constants;

    ActionBuilder(this, NEWTERMINAL)
        .setText(Tr::tr("New Terminal"))
        .bindContextAction(&m_newTerminalAction)
        .setIcon(NEW_TERMINAL_ICON.icon())
        .setToolTip(Tr::tr("Create a new Terminal."))
        .setContext(m_selfContext)
        .setDefaultKeySequence("Ctrl+T", "Ctrl+Shift+T")
        .addOnTriggered(this, [this] { openTerminal({}); });

    ActionBuilder(this, CLOSETERMINAL)
        .setText(Tr::tr("Close Terminal"))
        .bindContextAction(&m_closeTerminalAction)
        .setIcon(CLOSE_TERMINAL_ICON.icon())
        .setToolTip(Tr::tr("Close the current Terminal."))
        .setContext(m_selfContext)
        .addOnTriggered(this, [this] { removeTab(m_tabWidget.currentIndex()); });

    ActionBuilder(this, NEXTTERMINAL)
        .setText(Tr::tr("Next Terminal"))
        .setContext(m_selfContext)
        .setDefaultKeySequences(
            {QKeySequence("Alt+Tab"),
             QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+[")
                                                  : QLatin1String("Ctrl+PgUp"))})
        .addOnTriggered(this, [this] {
            if (canNavigate())
                goToNext();
        });

    ActionBuilder(this, PREVTERMINAL)
        .setText(Tr::tr("Previous Terminal"))
        .setContext(m_selfContext)
        .setDefaultKeySequences(
            {QKeySequence("Alt+Shift+Tab"),
             QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+]")
                                                  : QLatin1String("Ctrl+PgDown"))})
        .addOnTriggered(this, [this] {
            if (canPrevious())
                goToPrev();
        });

    Command *cmd = ActionManager::registerAction(settings().lockKeyboard.action(),
                                                 TOGGLE_KEYBOARD_LOCK);
    m_toggleKeyboardLockAction = cmd->action();
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
}

static Internal::ShellModel *shellModel()
{
    static Internal::ShellModel model;
    return &model;
}

void TerminalPane::createShellMenu()
{
    connect(&m_shellMenu, &QMenu::aboutToShow, &m_shellMenu, [this] {
        m_shellMenu.clear();

        const auto addItems = [this](const QList<Internal::ShellModelItem> &items) {
            for (const Internal::ShellModelItem &item : items) {
                QAction *action = new QAction(item.openParameters.icon, item.name, &m_shellMenu);

                connect(action, &QAction::triggered, action, [item, this]() {
                    openTerminal(item.openParameters);
                });

                m_shellMenu.addAction(action);
            }
        };

        addItems(shellModel()->local());
        m_shellMenu.addSection(Tr::tr("Devices"));
        addItems(shellModel()->remote());
    });
}

QList<QWidget *> TerminalPane::toolBarWidgets() const
{
    QList<QWidget *> widgets = IOutputPane::toolBarWidgets();

    widgets.prepend(m_newTerminalButton);
    widgets.prepend(m_closeTerminalButton);

    return widgets << m_openSettingsButton << m_lockKeyboardButton << m_escSettingButton
                   << m_variablesButton;
}

void TerminalPane::clearContents()
{
    if (const auto t = currentTerminal())
        t->clearContents();
}

void TerminalPane::visibilityChanged(bool visible)
{
    if (m_isVisible == visible)
        return;

    m_isVisible = visible;

    if (visible && m_tabWidget.count() == 0)
        openTerminal({});

    IOutputPane::visibilityChanged(visible);
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
    return m_tabWidget.count() > 1;
}

bool TerminalPane::canPrevious() const
{
    return m_tabWidget.count() > 1;
}

void TerminalPane::goToNext()
{
    int nextIndex = m_tabWidget.currentIndex() + 1;
    if (nextIndex >= m_tabWidget.count())
        nextIndex = 0;

    m_tabWidget.setCurrentIndex(nextIndex);
    emit navigateStateUpdate();
}

void TerminalPane::goToPrev()
{
    int prevIndex = m_tabWidget.currentIndex() - 1;
    if (prevIndex < 0)
        prevIndex = m_tabWidget.count() - 1;

    m_tabWidget.setCurrentIndex(prevIndex);
    emit navigateStateUpdate();
}

void TerminalPane::contextMenuRequested(const QPoint &pos)
{
    if (!m_tabWidget.tabBar()->geometry().contains(pos))
        return;

    const int index = m_tabWidget.tabBar()->tabAt(pos);
    const QList<QAction *> actions
        = {m_closeCurrentTabAction, m_closeAllTabsAction, m_closeOtherTabsAction};
    QAction *action = QMenu::exec(actions, m_tabWidget.mapToGlobal(pos), nullptr, &m_tabWidget);

    if (action == m_closeAllTabsAction) {
        while (m_tabWidget.count() > 0)
            removeTab(0);
        return;
    }

    const int currentIdx = index != -1 ? index : m_tabWidget.currentIndex();
    if (action == m_closeCurrentTabAction) {
        if (currentIdx >= 0)
            removeTab(currentIdx);
    } else if (action == m_closeOtherTabsAction) {
        for (int t = m_tabWidget.count() - 1; t >= 0; t--)
            if (t != currentIdx)
                removeTab(t);
    }
}

TabWidget::TabWidget()
{
    setTabBar(new DocumentTabBar);
}

static QAbstractItemModel *macroModel(QObject *parent)
{
    class MacroModel : public QAbstractItemModel
    {
        struct VarAndProvider
        {
            QByteArray variable;
            std::optional<MacroExpanderProvider> provider;
        };

        QList<VarAndProvider> m_variables;

    public:
        MacroModel(QObject *parent = nullptr)
            : QAbstractItemModel(parent)
        {
            const auto setupSubProviders = [this]() {
                beginResetModel();
                m_expander.clearSubProviders();
                m_expander.registerSubProvider({this, globalMacroExpander()});
                if (auto startupProject = ProjectExplorer::ProjectManager::startupProject()) {
                    m_expander.registerSubProvider(
                        {startupProject, startupProject->macroExpander()});

                    if (auto bc = startupProject->activeBuildConfiguration()) {
                        //   m_expander.registerSubProvider({bc, bc->macroExpander()});
                        if (bc->kit())
                            m_expander.registerSubProvider({bc, bc->kit()->macroExpander()});
                    }
                }

                m_variables.clear();
                for (const MacroExpanderProvider &provider : m_expander.subProviders()) {
                    if (MacroExpander *exp = provider()) {
                        for (const QByteArray &var : exp->visibleVariables())
                            m_variables.append({var, provider});

                        m_variables.append({"---", std::nullopt});
                    }
                }
                if (!m_variables.isEmpty())
                    m_variables.removeLast(); // remove last separator
                endResetModel();
            };

            connect(
                ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::startupProjectChanged,
                this,
                setupSubProviders);

            connect(
                ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::activeBuildConfigurationChanged,
                this,
                setupSubProviders);

            setupSubProviders();
        }

        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
            return parent.isValid() ? 0 : m_variables.size();
        }
        int columnCount(const QModelIndex &parent = QModelIndex()) const override
        {
            return parent.isValid() ? 0 : 1;
        }
        QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override
        {
            if (parent.isValid() || row < 0 || column < 0 || column >= 1
                || row >= m_variables.size())
                return QModelIndex();
            return createIndex(row, column);
        }
        QModelIndex parent(const QModelIndex &index) const override
        {
            Q_UNUSED(index)
            return QModelIndex();
        }

        Qt::ItemFlags flags(const QModelIndex &index) const override
        {
            if (!index.isValid())
                return Qt::NoItemFlags;

            if (index.data(MacroModelRoles::ValueRole).toString().isEmpty())
                return Qt::NoItemFlags;

            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
        {
            if (!index.isValid() || index.row() < 0 || index.column() < 0 || index.column() >= 1
                || index.row() >= m_variables.size()) {
                return QVariant();
            }
            const QByteArray varName = m_variables.at(index.row()).variable;
            auto provider = m_variables.at(index.row()).provider;
            if (!provider) {
                if (role == Qt::AccessibleDescriptionRole && varName == "---")
                    return "separator";
                return QVariant();
            }
            MacroExpander *expander = (*provider)();
            if (!expander)
                return QVariant();

            if (role == Qt::DisplayRole)
                return QString::fromUtf8(varName);
            if (role == Qt::ToolTipRole) {
                if (!expander)
                    return QVariant();

                QString description = expander->variableDescription(varName);
                const QByteArray exampleUsage = expander->variableExampleUsage(varName);
                const QString value = expander->value(exampleUsage).toHtmlEscaped();
                if (!value.isEmpty())
                    description += QLatin1String("<p>")
                                   + Tr::tr("Current Value of %{%1}: %2")
                                         .arg(QString::fromUtf8(exampleUsage), value);
                return description;
            }
            if (role == MacroModelRoles::IsPrefixRole) {
                if (varName.endsWith("<value>"))
                    return true;

                return expander->isPrefixVariable(varName);
            }
            if (role == MacroModelRoles::ValueRole)
                return expander->value(varName);

            return QVariant();
        }

    private:
        MacroExpander m_expander;
    };

    class FilterModel : public QSortFilterProxyModel
    {
    public:
        FilterModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent)
        {}

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
        {
            QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
            if (index.data(Qt::AccessibleDescriptionRole).toString() == "separator")
                return true;

            QVariant v = index.data(MacroModelRoles::IsPrefixRole);
            const bool accept = v.isValid() && v.typeId() == QMetaType::Bool && !v.toBool();
            return accept;
        }
    };
    auto model = new MacroModel(parent);
    auto filterModel = new FilterModel(model);
    filterModel->setSourceModel(model);
    return filterModel;
}

} // namespace Terminal
