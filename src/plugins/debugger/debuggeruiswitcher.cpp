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
#include "debuggerplugin.h"

#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSettings>

#include <QtGui/QDockWidget>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>

using namespace Core;

namespace Debugger {
namespace Internal {

class DockWidgetEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit DockWidgetEventFilter(QObject *parent = 0) : QObject(parent) {}

signals:
    void widgetResized();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
};

bool DockWidgetEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
    case QEvent::ZOrderChange:
        emit widgetResized();
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

// first: language id, second: menu item
typedef QPair<DebuggerLanguage, QAction *> ViewsMenuItems;

class DebuggerMainWindowPrivate
{
public:
    explicit DebuggerMainWindowPrivate(DebuggerMainWindow *mainWindow);

public:
    DebuggerMainWindow *q;
    QList<ViewsMenuItems> m_viewsMenuItems;
    QList<QDockWidget *> m_dockWidgets;

    QHash<QString, QVariant> m_dockWidgetActiveStateCpp;
    QHash<QString, QVariant> m_dockWidgetActiveStateQmlCpp;
    Internal::DockWidgetEventFilter *m_resizeEventFilter;

    QMap<DebuggerLanguage, QWidget *> m_toolBars;

    DebuggerLanguages m_supportedLanguages;
    int m_languageCount;

    QStackedWidget *m_toolbarStack;

    QHash<DebuggerLanguage, Context> m_contextsForLanguage;

    bool m_inDebugMode;
    bool m_changingUI;

    DebuggerLanguages m_previousDebugLanguages;
    DebuggerLanguages m_activeDebugLanguages;
    QAction *m_openMemoryEditorAction;

    ActionContainer *m_viewsMenu;
    ActionContainer *m_debugMenu;

    QMultiHash<DebuggerLanguage, Command *> m_menuCommands;

    QWeakPointer<ProjectExplorer::Project> m_previousProject;
    QWeakPointer<ProjectExplorer::Target> m_previousTarget;
    QWeakPointer<ProjectExplorer::RunConfiguration> m_previousRunConfiguration;

    bool m_initialized;
    QSettings *m_settings;
};

DebuggerMainWindowPrivate::DebuggerMainWindowPrivate(DebuggerMainWindow *mw)
    : q(mw)
    , m_resizeEventFilter(new Internal::DockWidgetEventFilter(mw))
    , m_supportedLanguages(AnyLanguage)
    , m_languageCount(0)
    , m_toolbarStack(new QStackedWidget)
    , m_inDebugMode(false)
    , m_changingUI(false)
    , m_previousDebugLanguages(AnyLanguage)
    , m_activeDebugLanguages(AnyLanguage)
    , m_openMemoryEditorAction(0)
    , m_viewsMenu(0)
    , m_debugMenu(0)
    , m_initialized(false)
    , m_settings(0)
{
}

} // namespace Internal

using namespace Internal;

DebuggerMainWindow::DebuggerMainWindow()
{
  d = new DebuggerMainWindowPrivate(this);
}

DebuggerMainWindow::~DebuggerMainWindow()
{
    delete d;
}

void DebuggerMainWindow::updateUiOnFileListChange()
{
    if (d->m_previousProject)
        updateUiForTarget(d->m_previousProject.data()->activeTarget());
}

void DebuggerMainWindow::updateUiForProject(ProjectExplorer::Project *project)
{
    if (!project)
        return;
    if (d->m_previousProject) {
        disconnect(d->m_previousProject.data(),
            SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(updateUiForTarget(ProjectExplorer::Target*)));
    }
    d->m_previousProject = project;
    connect(project, SIGNAL(fileListChanged()),
        SLOT(updateUiOnFileListChange()));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        SLOT(updateUiForTarget(ProjectExplorer::Target*)));
    updateUiForTarget(project->activeTarget());
}

void DebuggerMainWindow::updateUiForTarget(ProjectExplorer::Target *target)
{
    if (!target)
        return;

    if (d->m_previousTarget) {
         disconnect(d->m_previousTarget.data(),
            SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    }
    d->m_previousTarget = target;
    connect(target,
        SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    updateUiForRunConfiguration(target->activeRunConfiguration());
}

// updates default debug language settings per run config.
void DebuggerMainWindow::updateUiForRunConfiguration(ProjectExplorer::RunConfiguration *rc)
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

void DebuggerMainWindow::updateUiForCurrentRunConfiguration()
{
    updateActiveLanguages();
}

void DebuggerMainWindow::updateActiveLanguages()
{
    DebuggerLanguages newLanguages = AnyLanguage;

    if (d->m_previousRunConfiguration) {
        if (d->m_previousRunConfiguration.data()->useCppDebugger())
            newLanguages = CppLanguage;
        if (d->m_previousRunConfiguration.data()->useQmlDebugger())
            newLanguages |= QmlLanguage;
    }

    if (newLanguages != d->m_activeDebugLanguages) {
        d->m_activeDebugLanguages = newLanguages;
        emit activeLanguagesChanged(d->m_activeDebugLanguages);
    }

    updateUi();
}

DebuggerLanguages DebuggerMainWindow::supportedLanguages() const
{
    return d->m_supportedLanguages;
}

void DebuggerMainWindow::addMenuAction(Command *command,
    const DebuggerLanguage &language, const QString &group)
{
    d->m_debugMenu->addAction(command, group);
    d->m_menuCommands.insert(language, command);
}

DebuggerLanguages DebuggerMainWindow::activeDebugLanguages() const
{
    return d->m_activeDebugLanguages;
}

void DebuggerMainWindow::onModeChanged(IMode *mode)
{
    d->m_inDebugMode = (mode->id() == Constants::MODE_DEBUG);
    setDockActionsVisible(d->m_inDebugMode);
    hideInactiveWidgets();

    if (mode->id() != Constants::MODE_DEBUG)
        //|| DebuggerPlugin::instance()->hasSnapshots())
       return;

    updateActiveLanguages();
}

void DebuggerMainWindow::hideInactiveWidgets()
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

void DebuggerMainWindow::createViewsMenuItems()
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    Context globalcontext(Core::Constants::C_GLOBAL);

    d->m_openMemoryEditorAction = new QAction(this);
    d->m_openMemoryEditorAction->setText(tr("Memory..."));
    connect(d->m_openMemoryEditorAction, SIGNAL(triggered()),
       SIGNAL(memoryEditorRequested()));

    // Add menu items
    Command *cmd = 0;
    cmd = am->registerAction(d->m_openMemoryEditorAction,
        Core::Id("Debugger.Views.OpenMemoryEditor"),
        Core::Context(Constants::C_DEBUGMODE));
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(menuSeparator1(),
        Core::Id("Debugger.Views.Separator1"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(toggleLockedAction(),
        Core::Id("Debugger.Views.ToggleLocked"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(menuSeparator2(),
        Core::Id("Debugger.Views.Separator2"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(resetLayoutAction(),
        Core::Id("Debugger.Views.ResetSimple"), globalcontext);
    d->m_viewsMenu->addAction(cmd);
}

void DebuggerMainWindow::addLanguage(const DebuggerLanguage &languageId, const Context &context)
{
    bool activate = (d->m_supportedLanguages == AnyLanguage);
    d->m_supportedLanguages = d->m_supportedLanguages | languageId;
    d->m_languageCount++;

    d->m_toolBars.insert(languageId, 0);
    d->m_contextsForLanguage.insert(languageId, context);

    updateUiForRunConfiguration(0);

    if (activate)
        updateUi();
}

void DebuggerMainWindow::updateUi()
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

void DebuggerMainWindow::activateQmlCppLayout()
{
    ICore *core = ICore::instance();
    Context qmlCppContext = d->m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(d->m_contextsForLanguage.value(CppLanguage));

    // always use cpp toolbar
    d->m_toolbarStack->setCurrentWidget(d->m_toolBars.value(CppLanguage));

    if (d->m_previousDebugLanguages & QmlLanguage) {
        d->m_dockWidgetActiveStateQmlCpp = saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Context());
    } else if (d->m_previousDebugLanguages & CppLanguage) {
        d->m_dockWidgetActiveStateCpp = saveSettings();
        core->updateAdditionalContexts(d->m_contextsForLanguage.value(CppLanguage), Context());
    }

    restoreSettings(d->m_dockWidgetActiveStateQmlCpp);
    core->updateAdditionalContexts(Context(), qmlCppContext);
}

void DebuggerMainWindow::activateCppLayout()
{
    ICore *core = ICore::instance();
    Context qmlCppContext = d->m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(d->m_contextsForLanguage.value(CppLanguage));
    d->m_toolbarStack->setCurrentWidget(d->m_toolBars.value(CppLanguage));

    if (d->m_previousDebugLanguages & QmlLanguage) {
        d->m_dockWidgetActiveStateQmlCpp = saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Context());
    } else if (d->m_previousDebugLanguages & CppLanguage) {
        d->m_dockWidgetActiveStateCpp = saveSettings();
        core->updateAdditionalContexts(d->m_contextsForLanguage.value(CppLanguage), Context());
    }

    restoreSettings(d->m_dockWidgetActiveStateCpp);

    const Context &cppContext = d->m_contextsForLanguage.value(CppLanguage);
    core->updateAdditionalContexts(Context(), cppContext);
}

void DebuggerMainWindow::setToolbar(const DebuggerLanguage &language, QWidget *widget)
{
    Q_ASSERT(d->m_toolBars.contains(language));
    d->m_toolBars[language] = widget;
    d->m_toolbarStack->addWidget(widget);
}

QDockWidget *DebuggerMainWindow::breakWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_BREAK);
}

QDockWidget *DebuggerMainWindow::stackWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_STACK);
}

QDockWidget *DebuggerMainWindow::watchWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_WATCHERS);
}

QDockWidget *DebuggerMainWindow::outputWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_OUTPUT);
}

QDockWidget *DebuggerMainWindow::snapshotsWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_SNAPSHOTS);
}

QDockWidget *DebuggerMainWindow::threadsWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_THREADS);
}

QDockWidget *DebuggerMainWindow::qmlInspectorWindow() const
{
    return dockWidget(Constants::DOCKWIDGET_QML_INSPECTOR);
}

QDockWidget *DebuggerMainWindow::dockWidget(const QString &objectName) const
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
QDockWidget *DebuggerMainWindow::createDockWidget(const DebuggerLanguage &language,
    QWidget *widget, Qt::DockWidgetArea area)
{
//    qDebug() << "CREATE DOCK" << widget->objectName() << "LANGUAGE ID" << language
//             << "VISIBLE BY DEFAULT" << ((d->m_activeDebugLanguages & language) ? "true" : "false");
    QDockWidget *dockWidget = addDockForWidget(widget);
    addDockWidget(area, dockWidget);
    d->m_dockWidgets.append(dockWidget);

    if (!(d->m_activeDebugLanguages & language))
        dockWidget->hide();

    Context globalContext(Core::Constants::C_GLOBAL);

    ActionManager *am = ICore::instance()->actionManager();
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    Command *cmd = am->registerAction(toggleViewAction,
                         QString("Debugger." + dockWidget->objectName()), globalContext);
    cmd->setAttribute(Command::CA_Hide);
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

QWidget *DebuggerMainWindow::createContents(IMode *mode)
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();

    ProjectExplorer::ProjectExplorerPlugin *pe =
        ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(pe->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(updateUiForProject(ProjectExplorer::Project*)));
    connect(d->m_resizeEventFilter, SIGNAL(widgetResized()),
        SLOT(updateDockWidgetSettings()));

    d->m_debugMenu = am->actionContainer(ProjectExplorer::Constants::M_DEBUG);
    d->m_viewsMenu = am->actionContainer(Core::Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(d->m_viewsMenu, return 0)

    //d->m_mainWindow = new Internal::DebuggerMainWindow(this);
    setDocumentMode(true);
    setDockNestingEnabled(true);
    connect(this, SIGNAL(resetLayout()),
        SLOT(resetDebuggerLayout()));
    connect(toggleLockedAction(), SIGNAL(triggered()),
        SLOT(updateDockWidgetSettings()));

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(mode));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    MiniSplitter *documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(mode));
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
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    setToolBarDockWidget(dock);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    // Right-side window with editor, output etc.
    MiniSplitter *mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(this);
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(mode, mainWindowSplitter));
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation and right-side window.
    MiniSplitter *splitter = new MiniSplitter;
    splitter->addWidget(new NavigationWidgetPlaceHolder(mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

void DebuggerMainWindow::writeSettings() const
{
    d->m_settings->beginGroup(QLatin1String("DebugMode.CppMode"));
    QHashIterator<QString, QVariant> it(d->m_dockWidgetActiveStateCpp);
    while (it.hasNext()) {
        it.next();
        d->m_settings->setValue(it.key(), it.value());
    }
    d->m_settings->endGroup();

    d->m_settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
    it = QHashIterator<QString, QVariant>(d->m_dockWidgetActiveStateQmlCpp);
    while (it.hasNext()) {
        it.next();
        d->m_settings->setValue(it.key(), it.value());
    }
    d->m_settings->endGroup();
}

void DebuggerMainWindow::readSettings()
{
    d->m_dockWidgetActiveStateCpp.clear();
    d->m_dockWidgetActiveStateQmlCpp.clear();

    d->m_settings->beginGroup(QLatin1String("DebugMode.CppMode"));
    foreach (const QString &key, d->m_settings->childKeys())
        d->m_dockWidgetActiveStateCpp.insert(key, d->m_settings->value(key));
    d->m_settings->endGroup();

    d->m_settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
    foreach (const QString &key, d->m_settings->childKeys())
        d->m_dockWidgetActiveStateQmlCpp.insert(key, d->m_settings->value(key));
    d->m_settings->endGroup();

    // reset initial settings when there are none yet
    DebuggerLanguages langs = d->m_activeDebugLanguages;
    if (d->m_dockWidgetActiveStateCpp.isEmpty()) {
        d->m_activeDebugLanguages = CppLanguage;
        resetDebuggerLayout();
    }
    if (d->m_dockWidgetActiveStateQmlCpp.isEmpty()) {
        d->m_activeDebugLanguages = QmlLanguage;
        resetDebuggerLayout();
    }
    d->m_activeDebugLanguages = langs;
}

void DebuggerMainWindow::initialize(QSettings *settings)
{
    d->m_settings = settings;
    createViewsMenuItems();

    emit dockResetRequested(AnyLanguage);
    readSettings();

    updateUi();

    hideInactiveWidgets();
    setDockActionsVisible(false);
    d->m_initialized = true;
}

void DebuggerMainWindow::resetDebuggerLayout()
{
    emit dockResetRequested(d->m_activeDebugLanguages);

    if (isQmlActive())
        d->m_dockWidgetActiveStateQmlCpp = saveSettings();
    else
        d->m_dockWidgetActiveStateCpp = saveSettings();

    updateActiveLanguages();
}

void DebuggerMainWindow::updateDockWidgetSettings()
{
    if (!d->m_inDebugMode || d->m_changingUI)
        return;

    if (isQmlActive())
        d->m_dockWidgetActiveStateQmlCpp = saveSettings();
    else
        d->m_dockWidgetActiveStateCpp = saveSettings();
}

bool DebuggerMainWindow::isQmlCppActive() const
{
    return (d->m_activeDebugLanguages & CppLanguage)
        && (d->m_activeDebugLanguages & QmlLanguage);
}

bool DebuggerMainWindow::isQmlActive() const
{
    return (d->m_activeDebugLanguages & QmlLanguage);
}

QMenu *DebuggerMainWindow::createPopupMenu()
{
    QMenu *menu = 0;

    const QList<QDockWidget* > dockwidgets = d->m_dockWidgets;

    if (!dockwidgets.isEmpty()) {
        menu = FancyMainWindow::createPopupMenu();

        foreach (QDockWidget *dockWidget, dockwidgets) {
            if (dockWidget->parentWidget() == this)
                menu->addAction(dockWidget->toggleViewAction());
        }
        menu->addSeparator();
    }
    return menu;
}

} // namespace Debugger

#include "debuggeruiswitcher.moc"
