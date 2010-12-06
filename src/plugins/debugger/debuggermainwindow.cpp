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

#include "debuggermainwindow.h"
#include "debuggercore.h"

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

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

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
using namespace ProjectExplorer;
using namespace Debugger::Constants;

namespace Debugger {
namespace Internal {

class DockWidgetEventFilter : public QObject
{
public:
    DockWidgetEventFilter(DebuggerMainWindowPrivate *mw) : m_mw(mw) {}
private:
    virtual bool eventFilter(QObject *obj, QEvent *event);
    DebuggerMainWindowPrivate *m_mw;
};

class DebuggerMainWindowPrivate : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerMainWindowPrivate(DebuggerMainWindow *mainWindow);

    void activateQmlCppLayout();
    void activateCppLayout();
    void createViewsMenuItems();
    bool isQmlCppActive() const;
    bool isQmlActive() const;
    void setSimpleDockWidgetArrangement();
    // Debuggable languages are registered with this function.
    void addLanguage(const DebuggerLanguage &language, const Core::Context &context);


public slots:
    void resetDebuggerLayout();
    void updateUiForProject(ProjectExplorer::Project *project);
    void updateUiForTarget(ProjectExplorer::Target *target);
    void updateUiForRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void updateUiForCurrentRunConfiguration();
    void updateActiveLanguages();
    void updateUiOnFileListChange();
    void updateDockWidgetSettings();
    void openMemoryEditor() { debuggerCore()->openMemoryEditor(); }

public:
    DebuggerMainWindow *q;
    QList<QDockWidget *> m_dockWidgets;

    QHash<QString, QVariant> m_dockWidgetActiveStateCpp;
    QHash<QString, QVariant> m_dockWidgetActiveStateQmlCpp;
    DockWidgetEventFilter m_resizeEventFilter;

    QMap<DebuggerLanguage, QWidget *> m_toolBars;

    DebuggerLanguages m_supportedLanguages;

    QStackedWidget *m_toolbarStack;

    QHash<DebuggerLanguage, Context> m_contextsForLanguage;

    bool m_inDebugMode;
    bool m_changingUI;

    DebuggerLanguages m_previousDebugLanguages;
    DebuggerLanguages m_activeDebugLanguages;

    ActionContainer *m_viewsMenu;

    QWeakPointer<Project> m_previousProject;
    QWeakPointer<Target> m_previousTarget;
    QWeakPointer<RunConfiguration> m_previousRunConfiguration;
};

DebuggerMainWindowPrivate::DebuggerMainWindowPrivate(DebuggerMainWindow *mw)
    : q(mw)
    , m_resizeEventFilter(this)
    , m_supportedLanguages(AnyLanguage)
    , m_toolbarStack(new QStackedWidget)
    , m_inDebugMode(false)
    , m_changingUI(false)
    , m_previousDebugLanguages(AnyLanguage)
    , m_activeDebugLanguages(AnyLanguage)
    , m_viewsMenu(0)
{
    createViewsMenuItems();
    addLanguage(CppLanguage, Context(C_CPPDEBUGGER));
    addLanguage(QmlLanguage, Context(C_QMLDEBUGGER));
}

void DebuggerMainWindowPrivate::updateUiOnFileListChange()
{
    if (m_previousProject)
        updateUiForTarget(m_previousProject.data()->activeTarget());
}

void DebuggerMainWindowPrivate::updateUiForProject(Project *project)
{
    if (!project)
        return;
    if (m_previousProject) {
        disconnect(m_previousProject.data(),
            SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(updateUiForTarget(ProjectExplorer::Target*)));
    }
    m_previousProject = project;
    connect(project, SIGNAL(fileListChanged()),
        SLOT(updateUiOnFileListChange()));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        SLOT(updateUiForTarget(ProjectExplorer::Target*)));
    updateUiForTarget(project->activeTarget());
}

void DebuggerMainWindowPrivate::updateUiForTarget(Target *target)
{
    if (!target)
        return;

    if (m_previousTarget) {
         disconnect(m_previousTarget.data(),
            SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    }
    m_previousTarget = target;
    connect(target,
        SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        SLOT(updateUiForRunConfiguration(ProjectExplorer::RunConfiguration*)));
    updateUiForRunConfiguration(target->activeRunConfiguration());
}

// updates default debug language settings per run config.
void DebuggerMainWindowPrivate::updateUiForRunConfiguration(RunConfiguration *rc)
{
    if (!rc)
        return;
    if (m_previousRunConfiguration)
        disconnect(m_previousRunConfiguration.data(), SIGNAL(debuggersChanged()),
                   this, SLOT(updateUiForCurrentRunConfiguration()));
    m_previousRunConfiguration = rc;
    connect(m_previousRunConfiguration.data(),
            SIGNAL(debuggersChanged()),
            SLOT(updateUiForCurrentRunConfiguration()));
    updateUiForCurrentRunConfiguration();
}

void DebuggerMainWindowPrivate::updateUiForCurrentRunConfiguration()
{
    updateActiveLanguages();
}

void DebuggerMainWindowPrivate::updateActiveLanguages()
{
    DebuggerLanguages newLanguages = AnyLanguage;

    if (m_previousRunConfiguration) {
        if (m_previousRunConfiguration.data()->useCppDebugger())
            newLanguages = CppLanguage;
        if (m_previousRunConfiguration.data()->useQmlDebugger())
            newLanguages |= QmlLanguage;
    }

    if (newLanguages != m_activeDebugLanguages) {
        m_activeDebugLanguages = newLanguages;
        debuggerCore()->languagesChanged();
    }

    if (m_changingUI || !m_inDebugMode)
        return;

    m_changingUI = true;

    if (isQmlActive())
        activateQmlCppLayout();
    else
        activateCppLayout();

    m_previousDebugLanguages = m_activeDebugLanguages;

    m_changingUI = false;
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

DebuggerLanguages DebuggerMainWindow::activeDebugLanguages() const
{
    return d->m_activeDebugLanguages;
}

void DebuggerMainWindow::onModeChanged(IMode *mode)
{
    d->m_inDebugMode = (mode->id() == Constants::MODE_DEBUG);
    setDockActionsVisible(d->m_inDebugMode);

    // Hide all the debugger windows if mode is different.
    if (mode->id() == Constants::MODE_DEBUG) {
        readSettings();
        d->updateActiveLanguages();
    } else {
        // Hide dock widgets manually in case they are floating.
        foreach (QDockWidget *dockWidget, d->m_dockWidgets) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

void DebuggerMainWindowPrivate::createViewsMenuItems()
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    Context globalcontext(Core::Constants::C_GLOBAL);
    m_viewsMenu = am->actionContainer(Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(m_viewsMenu, return)

    QAction *openMemoryEditorAction = new QAction(this);
    openMemoryEditorAction->setText(tr("Memory..."));
    connect(openMemoryEditorAction, SIGNAL(triggered()),
       SLOT(openMemoryEditor()));

    // Add menu items
    Command *cmd = 0;
    cmd = am->registerAction(openMemoryEditorAction,
        Core::Id("Debugger.Views.OpenMemoryEditor"),
        Core::Context(Constants::C_DEBUGMODE));
    m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(q->menuSeparator1(),
        Core::Id("Debugger.Views.Separator1"), globalcontext);
    m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(q->toggleLockedAction(),
        Core::Id("Debugger.Views.ToggleLocked"), globalcontext);
    m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(q->menuSeparator2(),
        Core::Id("Debugger.Views.Separator2"), globalcontext);
    m_viewsMenu->addAction(cmd);
    cmd = am->registerAction(q->resetLayoutAction(),
        Core::Id("Debugger.Views.ResetSimple"), globalcontext);
    m_viewsMenu->addAction(cmd);
}

void DebuggerMainWindowPrivate::addLanguage(const DebuggerLanguage &languageId,
    const Context &context)
{
    m_supportedLanguages = m_supportedLanguages | languageId;
    m_toolBars.insert(languageId, 0);
    m_contextsForLanguage.insert(languageId, context);
}

void DebuggerMainWindowPrivate::activateQmlCppLayout()
{
    ICore *core = ICore::instance();
    Context qmlCppContext = m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(m_contextsForLanguage.value(CppLanguage));

    // always use cpp toolbar
    m_toolbarStack->setCurrentWidget(m_toolBars.value(CppLanguage));

    if (m_previousDebugLanguages & QmlLanguage) {
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Context());
    } else if (m_previousDebugLanguages & CppLanguage) {
        m_dockWidgetActiveStateCpp = q->saveSettings();
        core->updateAdditionalContexts(m_contextsForLanguage.value(CppLanguage),
            Context());
    }

    q->restoreSettings(m_dockWidgetActiveStateQmlCpp);
    core->updateAdditionalContexts(Context(), qmlCppContext);
}

void DebuggerMainWindowPrivate::activateCppLayout()
{
    ICore *core = ICore::instance();
    Context qmlCppContext = m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(m_contextsForLanguage.value(CppLanguage));
    m_toolbarStack->setCurrentWidget(m_toolBars.value(CppLanguage));

    if (m_previousDebugLanguages & QmlLanguage) {
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
        core->updateAdditionalContexts(qmlCppContext, Context());
    } else if (m_previousDebugLanguages & CppLanguage) {
        m_dockWidgetActiveStateCpp = q->saveSettings();
        core->updateAdditionalContexts(m_contextsForLanguage.value(CppLanguage),
            Context());
    }

    q->restoreSettings(m_dockWidgetActiveStateCpp);

    const Context &cppContext = m_contextsForLanguage.value(CppLanguage);
    core->updateAdditionalContexts(Context(), cppContext);
}

void DebuggerMainWindow::setToolbar(const DebuggerLanguage &language, QWidget *widget)
{
    Q_ASSERT(d->m_toolBars.contains(language));
    d->m_toolBars[language] = widget;
    d->m_toolbarStack->addWidget(widget);
}

QDockWidget *DebuggerMainWindow::dockWidget(const QString &objectName) const
{
    foreach (QDockWidget *dockWidget, d->m_dockWidgets) {
        if (dockWidget->objectName() == objectName)
            return dockWidget;
    }
    return 0;
}

bool DebuggerMainWindow::isDockVisible(const QString &objectName) const
{
    QDockWidget *dock = dockWidget(objectName);
    return dock && dock->toggleViewAction()->isChecked();
}

/*!
    Keep track of dock widgets so they can be shown/hidden for different languages
*/
QDockWidget *DebuggerMainWindow::createDockWidget(const DebuggerLanguage &language,
    QWidget *widget)
{
//    qDebug() << "CREATE DOCK" << widget->objectName() << "LANGUAGE ID" << language
//             << "VISIBLE BY DEFAULT" << ((d->m_activeDebugLanguages & language) ? "true" : "false");
    QDockWidget *dockWidget = addDockForWidget(widget);
    dockWidget->setObjectName(widget->objectName());
    addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
    d->m_dockWidgets.append(dockWidget);

    if (!(d->m_activeDebugLanguages & language))
        dockWidget->hide();

    Context globalContext(Core::Constants::C_GLOBAL);

    ActionManager *am = ICore::instance()->actionManager();
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    Command *cmd = am->registerAction(toggleViewAction,
             QString("Debugger." + widget->objectName()), globalContext);
    cmd->setAttribute(Command::CA_Hide);
    d->m_viewsMenu->addAction(cmd);

    dockWidget->installEventFilter(&d->m_resizeEventFilter);

    connect(dockWidget->toggleViewAction(), SIGNAL(triggered(bool)),
        d, SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
        d, SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        d, SLOT(updateDockWidgetSettings()));

    return dockWidget;
}

QWidget *DebuggerMainWindow::createContents(IMode *mode)
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    connect(pe->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        d, SLOT(updateUiForProject(ProjectExplorer::Project*)));

    d->m_viewsMenu = am->actionContainer(Core::Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(d->m_viewsMenu, return 0)

    //d->m_mainWindow = new Internal::DebuggerMainWindow(this);
    setDocumentMode(true);
    setDockNestingEnabled(true);
    connect(this, SIGNAL(resetLayout()),
        d, SLOT(resetDebuggerLayout()));
    connect(toggleLockedAction(), SIGNAL(triggered()),
        d, SLOT(updateDockWidgetSettings()));

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
    QWidget *outputPane = new OutputPanePlaceHolder(mode, mainWindowSplitter);
    outputPane->setObjectName(QLatin1String("DebuggerOutputPanePlaceHolder"));
    mainWindowSplitter->addWidget(outputPane);
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
    ICore *core = ICore::instance();
    QTC_ASSERT(core, return);
    QSettings *settings = core->settings();
    QTC_ASSERT(settings, return);

    settings->beginGroup(QLatin1String("DebugMode.CppMode"));
    QHashIterator<QString, QVariant> it(d->m_dockWidgetActiveStateCpp);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
    settings->endGroup();

    settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
    it = QHashIterator<QString, QVariant>(d->m_dockWidgetActiveStateQmlCpp);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
    settings->endGroup();
}

void DebuggerMainWindow::readSettings()
{
    ICore *core = ICore::instance();
    QTC_ASSERT(core, return);
    QSettings *settings = core->settings();
    QTC_ASSERT(settings, return);

    d->m_dockWidgetActiveStateCpp.clear();
    d->m_dockWidgetActiveStateQmlCpp.clear();

    settings->beginGroup(QLatin1String("DebugMode.CppMode"));
    foreach (const QString &key, settings->childKeys())
        d->m_dockWidgetActiveStateCpp.insert(key, settings->value(key));
    settings->endGroup();

    settings->beginGroup(QLatin1String("DebugMode.CppQmlMode"));
    foreach (const QString &key, settings->childKeys())
        d->m_dockWidgetActiveStateQmlCpp.insert(key, settings->value(key));
    settings->endGroup();

    // Reset initial settings when there are none yet.
    if (d->isQmlActive()) {
        if (d->m_dockWidgetActiveStateQmlCpp.isEmpty()) {
            d->m_activeDebugLanguages = DebuggerLanguage(QmlLanguage|CppLanguage);
            d->setSimpleDockWidgetArrangement();
            d->m_dockWidgetActiveStateCpp = saveSettings();
        }
    } else {
        if (d->m_dockWidgetActiveStateCpp.isEmpty()) {
            d->m_activeDebugLanguages = CppLanguage;
            d->setSimpleDockWidgetArrangement();
            d->m_dockWidgetActiveStateCpp = saveSettings();
        }
    }
    writeSettings();
}

void DebuggerMainWindowPrivate::resetDebuggerLayout()
{
    setSimpleDockWidgetArrangement();

    if (isQmlActive())
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
    else
        m_dockWidgetActiveStateCpp = q->saveSettings();

    updateActiveLanguages();
}

void DebuggerMainWindowPrivate::updateDockWidgetSettings()
{
    if (!m_inDebugMode || m_changingUI)
        return;

    if (isQmlActive())
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
    else
        m_dockWidgetActiveStateCpp = q->saveSettings();
}

bool DebuggerMainWindowPrivate::isQmlCppActive() const
{
    return (m_activeDebugLanguages & CppLanguage)
        && (m_activeDebugLanguages & QmlLanguage);
}

bool DebuggerMainWindowPrivate::isQmlActive() const
{
    return (m_activeDebugLanguages & QmlLanguage);
}

QMenu *DebuggerMainWindow::createPopupMenu()
{
    QMenu *menu = 0;
    if (!d->m_dockWidgets.isEmpty()) {
        menu = FancyMainWindow::createPopupMenu();
        foreach (QDockWidget *dockWidget, d->m_dockWidgets)
            if (dockWidget->parentWidget() == this)
                menu->addAction(dockWidget->toggleViewAction());
        menu->addSeparator();
    }
    return menu;
}

void DebuggerMainWindowPrivate::setSimpleDockWidgetArrangement()
{
    using namespace Constants;
    QTC_ASSERT(q, return);
    q->setTrackingEnabled(false);

    foreach (QDockWidget *dockWidget, m_dockWidgets) {
        dockWidget->setFloating(false);
        q->removeDockWidget(dockWidget);
    }

    foreach (QDockWidget *dockWidget, m_dockWidgets) {
        int area = Qt::BottomDockWidgetArea;
        QVariant p = dockWidget->property(DOCKWIDGET_DEFAULT_AREA);
        if (p.isValid())
            area = Qt::DockWidgetArea(p.toInt());
        q->addDockWidget(Qt::DockWidgetArea(area), dockWidget);
        dockWidget->hide();
    }

    QDockWidget *breakDock = q->dockWidget(DOCKWIDGET_BREAK);
    QDockWidget *stackDock = q->dockWidget(DOCKWIDGET_STACK);
    QDockWidget *watchDock = q->dockWidget(DOCKWIDGET_WATCHERS);
    QDockWidget *snapshotsDock = q->dockWidget(DOCKWIDGET_SNAPSHOTS);
    QDockWidget *threadsDock = q->dockWidget(DOCKWIDGET_THREADS);
    QDockWidget *outputDock = q->dockWidget(DOCKWIDGET_OUTPUT);
    QDockWidget *qmlInspectorDock = q->dockWidget(DOCKWIDGET_QML_INSPECTOR);
    QDockWidget *scriptConsoleDock = q->dockWidget(DOCKWIDGET_QML_SCRIPTCONSOLE);
    QDockWidget *modulesDock = q->dockWidget(DOCKWIDGET_MODULES);
    QDockWidget *registerDock = q->dockWidget(DOCKWIDGET_REGISTER);
    QDockWidget *sourceFilesDock = q->dockWidget(DOCKWIDGET_SOURCE_FILES);

    QTC_ASSERT(breakDock, return);
    QTC_ASSERT(stackDock, return);
    QTC_ASSERT(watchDock, return);
    QTC_ASSERT(snapshotsDock, return);
    QTC_ASSERT(threadsDock, return);
    QTC_ASSERT(outputDock, return);
    //QTC_ASSERT(qmlInspectorDock, return); // This is really optional.
    QTC_ASSERT(scriptConsoleDock, return);
    QTC_ASSERT(modulesDock, return);
    QTC_ASSERT(registerDock, return);
    QTC_ASSERT(sourceFilesDock, return);

    if (m_activeDebugLanguages.testFlag(Debugger::CppLanguage)
            && m_activeDebugLanguages.testFlag(Debugger::QmlLanguage)) {

        // cpp + qml
        stackDock->show();
        watchDock->show();
        breakDock->show();
        if (qmlInspectorDock)
            qmlInspectorDock->show();

        q->splitDockWidget(q->toolBarDockWidget(), stackDock, Qt::Vertical);
        q->splitDockWidget(stackDock, breakDock, Qt::Horizontal);
        q->tabifyDockWidget(stackDock, snapshotsDock);
        q->tabifyDockWidget(stackDock, threadsDock);
        if (qmlInspectorDock)
            q->splitDockWidget(stackDock, qmlInspectorDock, Qt::Horizontal);

    } else {

        stackDock->show();
        breakDock->show();
        watchDock->show();
        threadsDock->show();
        snapshotsDock->show();

        if ((m_activeDebugLanguages.testFlag(CppLanguage)
                && !m_activeDebugLanguages.testFlag(QmlLanguage))
            || m_activeDebugLanguages == AnyLanguage) {
            threadsDock->show();
            snapshotsDock->show();
        } else {
            scriptConsoleDock->show();
            //if (qmlInspectorDock)
            //    qmlInspectorDock->show();
        }
        q->splitDockWidget(q->toolBarDockWidget(), stackDock, Qt::Vertical);
        q->splitDockWidget(stackDock, breakDock, Qt::Horizontal);
        q->tabifyDockWidget(breakDock, modulesDock);
        q->tabifyDockWidget(breakDock, registerDock);
        q->tabifyDockWidget(breakDock, threadsDock);
        q->tabifyDockWidget(breakDock, sourceFilesDock);
        q->tabifyDockWidget(breakDock, snapshotsDock);
        q->tabifyDockWidget(breakDock, scriptConsoleDock);
        //if (qmlInspectorDock)
        //    q->splitDockWidget(breakDock, qmlInspectorDock, Qt::Horizontal);
    }

    q->setTrackingEnabled(true);
    q->update();
}

bool DockWidgetEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
    case QEvent::ZOrderChange:
        m_mw->updateDockWidgetSettings();
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

} // namespace Debugger

#include "debuggermainwindow.moc"
