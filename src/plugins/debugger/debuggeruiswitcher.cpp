/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerrunner.h"
#include "debuggerplugin.h"
#include "savedaction.h"

#include <utils/savedaction.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfiguration.h>

#include <QtGui/QActionGroup>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QDockWidget>
#include <QtGui/QResizeEvent>
#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSettings>

namespace Debugger {
namespace Internal {

DockWidgetEventFilter::DockWidgetEventFilter(QObject *parent)
    : QObject(parent)
{}

bool DockWidgetEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize || event->type() == QEvent::ZOrderChange) {
        emit widgetResized();
    }

    return QObject::eventFilter(obj, event);
}

// first: language id, second: menu item
typedef QPair<DebuggerLanguage, QAction *> ViewsMenuItems;

struct DebuggerUISwitcherPrivate
{
    explicit DebuggerUISwitcherPrivate(DebuggerUISwitcher *q);

    QList<ViewsMenuItems> m_viewsMenuItems;
    QList<QDockWidget *> m_dockWidgets;

    QHash<QString, QVariant> m_dockWidgetActiveStateCpp;
    QHash<QString, QVariant> m_dockWidgetActiveStateQmlCpp;
    Internal::DockWidgetEventFilter *m_resizeEventFilter;

    QMap<DebuggerLanguage, QWidget *> m_toolBars;

    DebuggerLanguages m_supportedLanguages;
    int m_languageCount;

    QStackedWidget *m_toolbarStack;
    Internal::DebuggerMainWindow *m_mainWindow;

    QHash<DebuggerLanguage, Core::Context> m_contextsForLanguage;

    QActionGroup *m_languageActionGroup;
    bool m_inDebugMode;
    bool m_changingUI;

    Core::ActionContainer *m_debuggerLanguageMenu;
    DebuggerLanguages m_previousDebugLanguages;
    DebuggerLanguages m_activeDebugLanguages;
    QAction *m_activateCppAction;
    QAction *m_activateQmlAction;
    QAction *m_openMemoryEditorAction;
    bool m_qmlEnabled;

    Core::ActionContainer *m_viewsMenu;
    Core::ActionContainer *m_debugMenu;

    QMultiHash<DebuggerLanguage, Core::Command *> m_menuCommands;

    QWeakPointer<ProjectExplorer::Project> m_previousProject;
    QWeakPointer<ProjectExplorer::Target> m_previousTarget;
    QWeakPointer<ProjectExplorer::RunConfiguration> m_previousRunConfiguration;

    bool m_initialized;

    static DebuggerUISwitcher *m_instance;
};

DebuggerUISwitcherPrivate::DebuggerUISwitcherPrivate(DebuggerUISwitcher *q)
    : m_resizeEventFilter(new Internal::DockWidgetEventFilter(q))
    , m_supportedLanguages(AnyLanguage)
    , m_languageCount(0)
    , m_toolbarStack(new QStackedWidget)
    , m_languageActionGroup(new QActionGroup(q))
    , m_inDebugMode(false)
    , m_changingUI(false)
    , m_debuggerLanguageMenu(0)
    , m_previousDebugLanguages(AnyLanguage)
    , m_activeDebugLanguages(AnyLanguage)
    , m_activateCppAction(0)
    , m_activateQmlAction(0)
    , m_openMemoryEditorAction(0)
    , m_qmlEnabled(false)
    , m_viewsMenu(0)
    , m_debugMenu(0)
    , m_initialized(false)
{
    m_languageActionGroup->setExclusive(false);
}

DebuggerUISwitcher *DebuggerUISwitcherPrivate::m_instance = 0;

} // namespace Internal

using namespace Internal;

DebuggerUISwitcher::DebuggerUISwitcher(Core::BaseMode *mode, QObject* parent)
  : QObject(parent), d(new DebuggerUISwitcherPrivate(this))
{
    mode->setWidget(createContents(mode));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    ProjectExplorer::ProjectExplorerPlugin *pe =
        ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(pe->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(updateUiForProject(ProjectExplorer::Project*)));
    connect(d->m_resizeEventFilter, SIGNAL(widgetResized()),
        SLOT(updateDockWidgetSettings()));

    d->m_debugMenu = am->actionContainer(ProjectExplorer::Constants::M_DEBUG);
    d->m_viewsMenu = am->actionContainer(QLatin1String(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(d->m_viewsMenu, return)
    d->m_debuggerLanguageMenu = am->createMenu(Constants::M_DEBUG_DEBUGGING_LANGUAGES);

    DebuggerUISwitcherPrivate::m_instance = this;
}

DebuggerUISwitcher::~DebuggerUISwitcher()
{
    DebuggerUISwitcherPrivate::m_instance = 0;
    delete d;
}

void DebuggerUISwitcher::updateUiOnFileListChange()
{
    if (d->m_previousProject) {
        updateUiForTarget(d->m_previousProject.data()->activeTarget());
    }
}

void DebuggerUISwitcher::updateUiForProject(ProjectExplorer::Project *project)
{
    if (project) {
        if (d->m_previousProject) {
            disconnect(d->m_previousProject.data(), SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                       this, SLOT(updateUiForTarget(ProjectExplorer::Target*)));
        }
        d->m_previousProject = project;
        connect(project, SIGNAL(fileListChanged()),
            SLOT(updateUiOnFileListChange()));
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            SLOT(updateUiForTarget(ProjectExplorer::Target*)));
        updateUiForTarget(project->activeTarget());
    }
}

void DebuggerUISwitcher::updateUiForTarget(ProjectExplorer::Target *target)
{
    if (!target)
        return;

    if (d->m_previousTarget) {
            disconnect(d->m_previousTarget.data(),
                       SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    }
    d->m_previousTarget = target;
    connect(target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    updateUiForRunConfiguration(target->activeRunConfiguration());
}

// updates default debug language settings per run config.
void DebuggerUISwitcher::updateUiForRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    if (rc) {
        if (d->m_previousRunConfiguration) {
            disconnect(d->m_previousRunConfiguration.data(),
                       SIGNAL(debuggersChanged()),
                       this, SLOT(updateUiForCurrentRunConfiguration()));
        }
        d->m_previousRunConfiguration = rc;
        connect(d->m_previousRunConfiguration.data(),
                   SIGNAL(debuggersChanged()),
                   this, SLOT(updateUiForCurrentRunConfiguration()));

        updateUiForCurrentRunConfiguration();
    }
}

void DebuggerUISwitcher::updateUiForCurrentRunConfiguration()
{
    if (d->m_previousRunConfiguration) {
        ProjectExplorer::RunConfiguration *rc = d->m_previousRunConfiguration.data();

        if (d->m_activateCppAction)
            d->m_activateCppAction->setChecked(rc->useCppDebugger());
        if (d->m_activateQmlAction)
            d->m_activateQmlAction->setChecked(rc->useQmlDebugger());
    }

    updateActiveLanguages();
}

void DebuggerUISwitcher::updateActiveLanguages()
{
    DebuggerLanguages prevLanguages = d->m_activeDebugLanguages;

    d->m_activeDebugLanguages = AnyLanguage;

    if (d->m_activateCppAction->isChecked())
        d->m_activeDebugLanguages = CppLanguage;

    if (d->m_qmlEnabled && d->m_activateQmlAction->isChecked())
        d->m_activeDebugLanguages |= QmlLanguage;

    if (d->m_activeDebugLanguages == AnyLanguage) {
        // do mutual exclusive selection if qml is enabled. Otherwise, just keep
        // cpp language selected.
        if (prevLanguages & CppLanguage && d->m_qmlEnabled) {
            d->m_activeDebugLanguages = QmlLanguage;
            d->m_activateQmlAction->setChecked(true);
        } else {
            d->m_activateCppAction->setChecked(true);
            d->m_activeDebugLanguages = CppLanguage;
        }
    }

    emit activeLanguagesChanged(d->m_activeDebugLanguages);

    updateUi();
}

DebuggerLanguages DebuggerUISwitcher::supportedLanguages() const
{
    return d->m_supportedLanguages;
}

void DebuggerUISwitcher::addMenuAction(Core::Command *command,
    const DebuggerLanguage &language, const QString &group)
{
    d->m_debugMenu->addAction(command, group);
    d->m_menuCommands.insert(language, command);
}

DebuggerLanguages DebuggerUISwitcher::activeDebugLanguages() const
{
    return d->m_activeDebugLanguages;
}

void DebuggerUISwitcher::onModeChanged(Core::IMode *mode)
{
    d->m_inDebugMode = (mode->id() == Constants::MODE_DEBUG);
    d->m_mainWindow->setDockActionsVisible(d->m_inDebugMode);
    hideInactiveWidgets();

    if (mode->id() != Constants::MODE_DEBUG)
        //|| DebuggerPlugin::instance()->hasSnapsnots())
       return;

    updateActiveLanguages();
}

void DebuggerUISwitcher::hideInactiveWidgets()
{
    // Hide all the debugger windows if mode is different.
    if (d->m_inDebugMode)
        return;
    // Hide dock widgets manually in case they are floating.
    foreach (QDockWidget *dockWidget, d->m_dockWidgets) {
        if (dockWidget->isFloating())
            dockWidget->hide();
    }
}

void DebuggerUISwitcher::createViewsMenuItems()
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    d->m_debugMenu->addMenu(d->m_debuggerLanguageMenu, Core::Constants::G_DEFAULT_THREE);
    d->m_debuggerLanguageMenu->menu()->setTitle(tr("&Debug Languages"));

    d->m_openMemoryEditorAction = new QAction(this);
    d->m_openMemoryEditorAction->setText(tr("Memory..."));
    connect(d->m_openMemoryEditorAction, SIGNAL(triggered()),
       SIGNAL(memoryEditorRequested()));

    // Add menu items
    Core::Command *cmd = 0;
    cmd = am->registerAction(d->m_openMemoryEditorAction,
        QLatin1String("Debugger.Views.OpenMemoryEditor"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(d->m_mainWindow->menuSeparator1(),
        QLatin1String("Debugger.Views.Separator1"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(d->m_mainWindow->toggleLockedAction(),
        QLatin1String("Debugger.Views.ToggleLocked"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(d->m_mainWindow->menuSeparator2(),
        QLatin1String("Debugger.Views.Separator2"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(d->m_mainWindow->resetLayoutAction(),
        QLatin1String("Debugger.Views.ResetSimple"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
}

DebuggerUISwitcher *DebuggerUISwitcher::instance()
{
    return DebuggerUISwitcherPrivate::m_instance;
}

void DebuggerUISwitcher::addLanguage(const DebuggerLanguage &languageId,
                                     const QString &languageName, const Core::Context &context)
{
    bool activate = (d->m_supportedLanguages == AnyLanguage);
    d->m_supportedLanguages = d->m_supportedLanguages | languageId;
    d->m_languageCount++;

    d->m_toolBars.insert(languageId, 0);
    d->m_contextsForLanguage.insert(languageId, context);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();

    QAction *debuggableLang = new QAction(languageName, this);
    debuggableLang->setCheckable(true);
    debuggableLang->setText(languageName);
    d->m_languageActionGroup->addAction(debuggableLang);
    Core::Command *activeDebugLanguageCmd = am->registerAction(debuggableLang,
                         "Debugger.DebugLanguage." + languageName,
                          Core::Context(Core::Constants::C_GLOBAL));
    d->m_debuggerLanguageMenu->addAction(activeDebugLanguageCmd);

    QString shortcutPrefix = tr("Alt+L");
    QString shortcutIndex = QString::number(d->m_languageCount);
    activeDebugLanguageCmd->setDefaultKeySequence(QKeySequence(
                            QString("%1,%2").arg(shortcutPrefix).arg(shortcutIndex)));

    if (languageId == QmlLanguage) {
        d->m_qmlEnabled = true;
        d->m_activateQmlAction = debuggableLang;
    } else if (!d->m_activateCppAction) {
        d->m_activateCppAction = debuggableLang;
    }
    connect(debuggableLang, SIGNAL(triggered()), SLOT(updateActiveLanguages()));

    updateUiForRunConfiguration(0);

    if (activate)
        updateUi();
}

void DebuggerUISwitcher::updateUi()
{
    if (d->m_changingUI || !d->m_initialized || !d->m_inDebugMode)
        return;

    d->m_changingUI = true;

    if (isQmlActive()) {
        activateQmlCppLayout();
    } else {
        activateCppLayout();
    }

    d->m_previousDebugLanguages = d->m_activeDebugLanguages;

    d->m_changingUI = false;
}

void DebuggerUISwitcher::activateQmlCppLayout()
{
    Core::ICore *core = Core::ICore::instance();
    Core::Context qmlCppContext = d->m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(d->m_contextsForLanguage.value(CppLanguage));

    // always use cpp toolbar
    d->m_toolbarStack->setCurrentWidget(d->m_toolBars.value(CppLanguage));

    if (d->m_previousDebugLanguages & QmlLanguage) {
        d->m_dockWidgetActiveStateQmlCpp = d->m_mainWindow->saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Core::Context());
    } else if (d->m_previousDebugLanguages & CppLanguage) {
        d->m_dockWidgetActiveStateCpp = d->m_mainWindow->saveSettings();
        core->updateAdditionalContexts(d->m_contextsForLanguage.value(CppLanguage), Core::Context());
    }

    d->m_mainWindow->restoreSettings(d->m_dockWidgetActiveStateQmlCpp);
    core->updateAdditionalContexts(Core::Context(), qmlCppContext);
}

void DebuggerUISwitcher::activateCppLayout()
{
    Core::ICore *core = Core::ICore::instance();
    Core::Context qmlCppContext = d->m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(d->m_contextsForLanguage.value(CppLanguage));
    d->m_toolbarStack->setCurrentWidget(d->m_toolBars.value(CppLanguage));

    if (d->m_previousDebugLanguages & QmlLanguage) {
        d->m_dockWidgetActiveStateQmlCpp = d->m_mainWindow->saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Core::Context());
    } else if (d->m_previousDebugLanguages & CppLanguage) {
        d->m_dockWidgetActiveStateCpp = d->m_mainWindow->saveSettings();
        core->updateAdditionalContexts(d->m_contextsForLanguage.value(CppLanguage), Core::Context());
    }

    d->m_mainWindow->restoreSettings(d->m_dockWidgetActiveStateCpp);

    const Core::Context &cppContext = d->m_contextsForLanguage.value(CppLanguage);
    core->updateAdditionalContexts(Core::Context(), cppContext);
}

void DebuggerUISwitcher::setToolbar(const DebuggerLanguage &language, QWidget *widget)
{
    Q_ASSERT(d->m_toolBars.contains(language));
    d->m_toolBars[language] = widget;
    d->m_toolbarStack->addWidget(widget);
}

Utils::FancyMainWindow *DebuggerUISwitcher::mainWindow() const
{
    return d->m_mainWindow;
}

QWidget *DebuggerUISwitcher::createMainWindow(Core::BaseMode *mode)
{
    d->m_mainWindow = new DebuggerMainWindow(this);
    d->m_mainWindow->setDocumentMode(true);
    d->m_mainWindow->setDockNestingEnabled(true);
    connect(d->m_mainWindow, SIGNAL(resetLayout()),
        SLOT(resetDebuggerLayout()));
    connect(d->m_mainWindow->toggleLockedAction(), SIGNAL(triggered()),
        SLOT(updateDockWidgetSettings()));

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(new Core::EditorManagerPlaceHolder(mode));
    editorHolderLayout->addWidget(new Core::FindToolBarPlaceHolder(editorAndFindWidget));

    Core::MiniSplitter *documentAndRightPane = new Core::MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new Core::RightPanePlaceHolder(mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    Utils::StyledBar *debugToolBar = new Utils::StyledBar;
    debugToolBar->setProperty("topBorder", true);
    QHBoxLayout *debugToolBarLayout = new QHBoxLayout(debugToolBar);
    debugToolBarLayout->setMargin(0);
    debugToolBarLayout->setSpacing(0);
    debugToolBarLayout->addWidget(d->m_toolbarStack);
    debugToolBarLayout->addStretch();
    debugToolBarLayout->addWidget(new Utils::StyledSeparator);

    QDockWidget *dock = new QDockWidget(tr("Debugger Toolbar"));
    dock->setObjectName(QLatin1String("Debugger Toolbar"));
    dock->setWidget(debugToolBar);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock));
    d->m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dock);
    d->m_mainWindow->setToolBarDockWidget(dock);

    QWidget *centralWidget = new QWidget;
    d->m_mainWindow->setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    return d->m_mainWindow;
}

QDockWidget *DebuggerUISwitcher::breakWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_BREAK);
}

QDockWidget *DebuggerUISwitcher::stackWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_STACK);
}

QDockWidget *DebuggerUISwitcher::watchWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_WATCHERS);
}

QDockWidget *DebuggerUISwitcher::outputWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_OUTPUT);
}

QDockWidget *DebuggerUISwitcher::snapshotsWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_SNAPSHOTS);
}

QDockWidget *DebuggerUISwitcher::threadsWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_THREADS);
}

QDockWidget *DebuggerUISwitcher::qmlInspectorWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_QML_INSPECTOR);
}

QDockWidget *DebuggerUISwitcher::dockWidget(const QString &objectName) const
{
    foreach(QDockWidget *dockWidget, d->m_dockWidgets) {
        if (dockWidget->objectName() == objectName)
            return dockWidget;
    }
    return 0;
}

/*!
    Keep track of dock widgets so they can be shown/hidden for different languages
*/
QDockWidget *DebuggerUISwitcher::createDockWidget(const DebuggerLanguage &language,
    QWidget *widget, Qt::DockWidgetArea area)
{
    //qDebug() << "CREATE DOCK" << widget->objectName() << langName
    //    << d->m_activeLanguage << "VISIBLE BY DEFAULT: " << visibleByDefault;
    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    d->m_mainWindow->addDockWidget(area, dockWidget);
    d->m_dockWidgets.append(dockWidget);

    if (!(d->m_activeDebugLanguages & language))
        dockWidget->hide();

    Core::Context globalContext(Core::Constants::C_GLOBAL);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    Core::Command *cmd = am->registerAction(toggleViewAction,
                         "Debugger." + dockWidget->objectName(), globalContext);
    cmd->setAttribute(Core::Command::CA_Hide);
    d->m_viewsMenu->addAction(cmd);

    d->m_viewsMenuItems.append(qMakePair(language, toggleViewAction));

    dockWidget->installEventFilter(d->m_resizeEventFilter);

    connect(dockWidget->toggleViewAction(), SIGNAL(triggered(bool)),
        SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
        SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        SLOT(updateDockWidgetSettings()));

    return dockWidget;
}

QWidget *DebuggerUISwitcher::createContents(Core::BaseMode *mode)
{
    // right-side window with editor, output etc.
    Core::MiniSplitter *mainWindowSplitter = new Core::MiniSplitter;
    mainWindowSplitter->addWidget(createMainWindow(mode));
    mainWindowSplitter->addWidget(new Core::OutputPanePlaceHolder(mode, mainWindowSplitter));
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // navigation + right-side window
    Core::MiniSplitter *splitter = new Core::MiniSplitter;
    splitter->addWidget(new Core::NavigationWidgetPlaceHolder(mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

void DebuggerUISwitcher::aboutToStartDebugger()
{
    if (!DebuggerPlugin::instance()->hasSnapsnots())
        updateActiveLanguages();
}

void DebuggerUISwitcher::aboutToShutdown()
{
    writeSettings();
}

void DebuggerUISwitcher::writeSettings() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    {
        settings->beginGroup(QLatin1String("DebugMode.CppMode"));
        QHashIterator<QString, QVariant> it(d->m_dockWidgetActiveStateCpp);
        while (it.hasNext()) {
            it.next();
            settings->setValue(it.key(), it.value());
        }
        settings->endGroup();
    }
    if (d->m_qmlEnabled) {
        settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
        QHashIterator<QString, QVariant> it(d->m_dockWidgetActiveStateQmlCpp);
        while (it.hasNext()) {
            it.next();
            settings->setValue(it.key(), it.value());
        }
        settings->endGroup();
    }
}

void DebuggerUISwitcher::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    d->m_dockWidgetActiveStateCpp.clear();
    d->m_dockWidgetActiveStateQmlCpp.clear();

    settings->beginGroup(QLatin1String("DebugMode.CppMode"));
    foreach (const QString &key, settings->childKeys()) {
        d->m_dockWidgetActiveStateCpp.insert(key, settings->value(key));
    }
    settings->endGroup();

    if (d->m_qmlEnabled) {
        settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
        foreach (const QString &key, settings->childKeys()) {
            d->m_dockWidgetActiveStateQmlCpp.insert(key, settings->value(key));
        }
        settings->endGroup();
    }

    // reset initial settings when there are none yet
    DebuggerLanguages langs = d->m_activeDebugLanguages;
    if (d->m_dockWidgetActiveStateCpp.isEmpty()) {
        d->m_activeDebugLanguages = CppLanguage;
        resetDebuggerLayout();
    }
    if (d->m_qmlEnabled && d->m_dockWidgetActiveStateQmlCpp.isEmpty()) {
        d->m_activeDebugLanguages = QmlLanguage;
        resetDebuggerLayout();
    }
    d->m_activeDebugLanguages = langs;
}

void DebuggerUISwitcher::initialize()
{
    createViewsMenuItems();

    emit dockResetRequested(AnyLanguage);
    readSettings();

    updateUi();

    hideInactiveWidgets();
    d->m_mainWindow->setDockActionsVisible(false);
    d->m_initialized = true;
}

void DebuggerUISwitcher::resetDebuggerLayout()
{
    emit dockResetRequested(d->m_activeDebugLanguages);

    if (isQmlActive()) {
        d->m_dockWidgetActiveStateQmlCpp = d->m_mainWindow->saveSettings();
    } else {
        d->m_dockWidgetActiveStateCpp = d->m_mainWindow->saveSettings();
    }

    updateActiveLanguages();
}

void DebuggerUISwitcher::updateDockWidgetSettings()
{
    if (!d->m_inDebugMode || d->m_changingUI)
        return;

    if (isQmlActive()) {
        d->m_dockWidgetActiveStateQmlCpp = d->m_mainWindow->saveSettings();
    } else {
        d->m_dockWidgetActiveStateCpp = d->m_mainWindow->saveSettings();
    }
}

bool DebuggerUISwitcher::isQmlCppActive() const
{
    return (d->m_activeDebugLanguages & CppLanguage)
        && (d->m_activeDebugLanguages & QmlLanguage);
}

bool DebuggerUISwitcher::isQmlActive() const
{
    return (d->m_activeDebugLanguages & QmlLanguage);
}

QList<QDockWidget* > DebuggerUISwitcher::i_mw_dockWidgets() const
{
    return d->m_dockWidgets;
}

} // namespace Debugger
