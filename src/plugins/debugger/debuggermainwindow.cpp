/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggermainwindow.h"
#include "debuggercore.h"
#include "debuggerrunconfigurationaspect.h"

#include <utils/algorithm.h>
#include <utils/appmainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <QDebug>

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

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
    void addLanguage(DebuggerLanguage language, const Context &context);

    QDockWidget *dockWidget(const QString &objectName) const
        { return q->findChild<QDockWidget *>(objectName); }

public slots:
    void resetDebuggerLayout();
    void updateUiForProject(Project *project);
    void updateUiForTarget(Target *target);
    void updateUiForRunConfiguration(RunConfiguration *rc);
    void updateUiForCurrentRunConfiguration();
    void updateActiveLanguages();
    void updateDockWidgetSettings();
    void openMemoryEditor() { Internal::openMemoryEditor(); }

public:
    DebuggerMainWindow *q;

    QHash<QString, QVariant> m_dockWidgetActiveStateCpp;
    QHash<QString, QVariant> m_dockWidgetActiveStateQmlCpp;
    DockWidgetEventFilter m_resizeEventFilter;

    QMap<DebuggerLanguage, QWidget *> m_toolBars;

    DebuggerLanguages m_supportedLanguages;

    QWidget *m_debugToolBar;
    QToolButton *m_viewButton;
    QHBoxLayout *m_debugToolBarLayout;

    QHash<DebuggerLanguage, Context> m_contextsForLanguage;

    bool m_inDebugMode;
    bool m_changingUI;

    DebuggerLanguages m_previousDebugLanguages;
    DebuggerLanguages m_activeDebugLanguages;
    DebuggerLanguages m_engineDebugLanguages;

    ActionContainer *m_viewsMenu;

    Project *m_previousProject;
    Target *m_previousTarget;
    RunConfiguration *m_previousRunConfiguration;
};

DebuggerMainWindowPrivate::DebuggerMainWindowPrivate(DebuggerMainWindow *mw)
    : q(mw)
    , m_resizeEventFilter(this)
    , m_supportedLanguages(AnyLanguage)
    , m_debugToolBar(new QWidget)
    , m_debugToolBarLayout(new QHBoxLayout(m_debugToolBar))
    , m_inDebugMode(false)
    , m_changingUI(false)
    , m_previousDebugLanguages(AnyLanguage)
    , m_activeDebugLanguages(AnyLanguage)
    , m_engineDebugLanguages(AnyLanguage)
    , m_viewsMenu(0)
    , m_previousProject(0)
    , m_previousTarget(0)
    , m_previousRunConfiguration(0)
{
    m_debugToolBarLayout->setMargin(0);
    m_debugToolBarLayout->setSpacing(0);
    createViewsMenuItems();
    addLanguage(AnyLanguage, Context());
    addLanguage(CppLanguage, Context(C_CPPDEBUGGER));
    addLanguage(QmlLanguage, Context(C_QMLDEBUGGER));
}

void DebuggerMainWindowPrivate::updateUiForProject(Project *project)
{
    if (m_previousProject) {
        disconnect(m_previousProject, &Project::activeTargetChanged,
            this, &DebuggerMainWindowPrivate::updateUiForTarget);
    }
    m_previousProject = project;
    if (!project) {
        updateUiForTarget(0);
        return;
    }
    connect(project, &Project::activeTargetChanged,
            this, &DebuggerMainWindowPrivate::updateUiForTarget);
    updateUiForTarget(project->activeTarget());
}

void DebuggerMainWindowPrivate::updateUiForTarget(Target *target)
{
    if (m_previousTarget) {
         disconnect(m_previousTarget, &Target::activeRunConfigurationChanged,
                    this, &DebuggerMainWindowPrivate::updateUiForRunConfiguration);
    }

    m_previousTarget = target;

    if (!target) {
        updateUiForRunConfiguration(0);
        return;
    }

    connect(target, &Target::activeRunConfigurationChanged,
            this, &DebuggerMainWindowPrivate::updateUiForRunConfiguration);
    updateUiForRunConfiguration(target->activeRunConfiguration());
}

// updates default debug language settings per run config.
void DebuggerMainWindowPrivate::updateUiForRunConfiguration(RunConfiguration *rc)
{
    if (m_previousRunConfiguration)
        disconnect(m_previousRunConfiguration, &RunConfiguration::requestRunActionsUpdate,
                   this, &DebuggerMainWindowPrivate::updateUiForCurrentRunConfiguration);
    m_previousRunConfiguration = rc;
    updateUiForCurrentRunConfiguration();
    if (!rc)
        return;

    connect(m_previousRunConfiguration, &RunConfiguration::requestRunActionsUpdate,
            this, &DebuggerMainWindowPrivate::updateUiForCurrentRunConfiguration);
}

void DebuggerMainWindowPrivate::updateUiForCurrentRunConfiguration()
{
    updateActiveLanguages();
}

void DebuggerMainWindowPrivate::updateActiveLanguages()
{
    DebuggerLanguages newLanguages = AnyLanguage;

    if (m_engineDebugLanguages != AnyLanguage) {
        newLanguages = m_engineDebugLanguages;
    } else if (m_previousRunConfiguration) {
        if (m_previousRunConfiguration->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger())
            newLanguages |= CppLanguage;
        if (m_previousRunConfiguration->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger())
            newLanguages |= QmlLanguage;
    }

    if (newLanguages != m_activeDebugLanguages)
        m_activeDebugLanguages = newLanguages;

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

void DebuggerMainWindow::setEngineDebugLanguages(DebuggerLanguages languages)
{
    if (d->m_engineDebugLanguages == languages)
        return;

    d->m_engineDebugLanguages = languages;
    d->updateActiveLanguages();
}

void DebuggerMainWindow::onModeChanged(IMode *mode)
{
    d->m_inDebugMode = (mode && mode->id() == Constants::MODE_DEBUG);
    setDockActionsVisible(d->m_inDebugMode);

    // Hide all the debugger windows if mode is different.
    if (d->m_inDebugMode) {
        readSettings();
        d->updateActiveLanguages();
    } else {
        // Hide dock widgets manually in case they are floating.
        foreach (QDockWidget *dockWidget, dockWidgets()) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

void DebuggerMainWindowPrivate::createViewsMenuItems()
{
    Context debugcontext(Constants::C_DEBUGMODE);
    m_viewsMenu = ActionManager::actionContainer(Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(m_viewsMenu, return);

    QAction *openMemoryEditorAction = new QAction(this);
    openMemoryEditorAction->setText(tr("Memory..."));
    connect(openMemoryEditorAction, &QAction::triggered,
            this, &DebuggerMainWindowPrivate::openMemoryEditor);

    // Add menu items
    Command *cmd = 0;
    cmd = ActionManager::registerAction(openMemoryEditorAction,
        "Debugger.Views.OpenMemoryEditor", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    m_viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    cmd = ActionManager::registerAction(q->menuSeparator1(),
        "Debugger.Views.Separator1", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    m_viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(q->autoHideTitleBarsAction(),
        "Debugger.Views.AutoHideTitleBars", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    m_viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(q->menuSeparator2(),
        "Debugger.Views.Separator2", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    m_viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
}

void DebuggerMainWindowPrivate::addLanguage(DebuggerLanguage languageId,
                                            const Context &context)
{
    m_supportedLanguages = m_supportedLanguages | languageId;
    m_toolBars.insert(languageId, 0);
    m_contextsForLanguage.insert(languageId, context);
}

void DebuggerMainWindowPrivate::activateQmlCppLayout()
{
    Context qmlCppContext = m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(m_contextsForLanguage.value(CppLanguage));
    if (m_toolBars.value(QmlLanguage)) {
        m_debugToolBarLayout->insertWidget(1, m_toolBars.value(QmlLanguage));
        m_toolBars.value(QmlLanguage)->show();
    }

    if (m_previousDebugLanguages & QmlLanguage) {
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
        ICore::removeAdditionalContext(qmlCppContext);
    } else if (m_previousDebugLanguages & CppLanguage) {
        m_dockWidgetActiveStateCpp = q->saveSettings();
        ICore::removeAdditionalContext(m_contextsForLanguage.value(CppLanguage));
    }

    q->restoreSettings(m_dockWidgetActiveStateQmlCpp);
    ICore::addAdditionalContext(qmlCppContext);
}

void DebuggerMainWindowPrivate::activateCppLayout()
{
    Context qmlCppContext = m_contextsForLanguage.value(QmlLanguage);
    qmlCppContext.add(m_contextsForLanguage.value(CppLanguage));
    if (m_toolBars.value(QmlLanguage)) {
        m_toolBars.value(QmlLanguage)->hide();
        m_debugToolBarLayout->removeWidget(m_toolBars.value(QmlLanguage));
    }

    if (m_previousDebugLanguages & QmlLanguage) {
        m_dockWidgetActiveStateQmlCpp = q->saveSettings();
        ICore::removeAdditionalContext(qmlCppContext);
    } else if (m_previousDebugLanguages & CppLanguage) {
        m_dockWidgetActiveStateCpp = q->saveSettings();
        ICore::removeAdditionalContext(m_contextsForLanguage.value(CppLanguage));
    }

    q->restoreSettings(m_dockWidgetActiveStateCpp);

    const Context &cppContext = m_contextsForLanguage.value(CppLanguage);
    ICore::addAdditionalContext(cppContext);
}

void DebuggerMainWindow::setToolBar(DebuggerLanguage language, QWidget *widget)
{
    Q_ASSERT(d->m_toolBars.contains(language));
    d->m_toolBars[language] = widget;
    if (language == CppLanguage)
        d->m_debugToolBarLayout->addWidget(widget);

    //Add widget at the end
    if (language == AnyLanguage)
        d->m_debugToolBarLayout->insertWidget(-1, widget, 10);
}

/*!
    Keep track of dock widgets so they can be shown/hidden for different languages
*/
QDockWidget *DebuggerMainWindow::createDockWidget(const DebuggerLanguage &language,
    QWidget *widget)
{
    QDockWidget *dockWidget = addDockForWidget(widget);
    dockWidget->setObjectName(widget->objectName());
    addDockWidget(Qt::BottomDockWidgetArea, dockWidget);

    if (!(d->m_activeDebugLanguages & language))
        dockWidget->hide();

    QAction *toggleViewAction = dockWidget->toggleViewAction();
    Command *cmd = ActionManager::registerAction(toggleViewAction,
             Id("Debugger.").withSuffix(widget->objectName()));
    cmd->setAttribute(Command::CA_Hide);

    dockWidget->installEventFilter(&d->m_resizeEventFilter);

    connect(dockWidget->toggleViewAction(), SIGNAL(triggered(bool)),
        d, SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
        d, SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        d, SLOT(updateDockWidgetSettings()));

    return dockWidget;
}

void DebuggerMainWindow::addStagedMenuEntries()
{
    addDockActionsToMenu(d->m_viewsMenu->menu());
}

QWidget *DebuggerMainWindow::createContents(IMode *mode)
{
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
        d, &DebuggerMainWindowPrivate::updateUiForProject);

    d->m_viewsMenu = ActionManager::actionContainer(Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(d->m_viewsMenu, return 0);

    //d->m_mainWindow = new Internal::DebuggerMainWindow(this);
    setDocumentMode(true);
    setDockNestingEnabled(true);
    connect(this, SIGNAL(resetLayout()),
        d, SLOT(resetDebuggerLayout()));
    connect(autoHideTitleBarsAction(), SIGNAL(triggered()),
        d, SLOT(updateDockWidgetSettings()));

    auto editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    auto editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    auto editorManagerPlaceHolder = new EditorManagerPlaceHolder(mode);
    editorHolderLayout->addWidget(editorManagerPlaceHolder);
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    auto documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    d->m_viewButton = new QToolButton();
    // FIXME: Use real thing after string freeze.
    QString hackyName = QCoreApplication::translate("Core::Internal::MainWindow", "&Views");
    hackyName.replace(QLatin1Char('&'), QString());
    d->m_viewButton->setText(hackyName);

    auto debugToolBar = new Utils::StyledBar;
    debugToolBar->setProperty("topBorder", true);
    auto debugToolBarLayout = new QHBoxLayout(debugToolBar);
    debugToolBarLayout->setMargin(0);
    debugToolBarLayout->setSpacing(0);
    debugToolBarLayout->addWidget(d->m_debugToolBar);
    debugToolBarLayout->addWidget(new Utils::StyledSeparator);
    debugToolBarLayout->addWidget(d->m_viewButton);

    connect(d->m_viewButton, &QAbstractButton::clicked, [this] {
        QMenu menu;
        addDockActionsToMenu(&menu);
        menu.exec(d->m_viewButton->mapToGlobal(QPoint()));
    });

    auto dock = new QDockWidget(DebuggerMainWindowPrivate::tr("Debugger Toolbar"));
    dock->setObjectName(QLatin1String("Debugger Toolbar"));
    dock->setWidget(debugToolBar);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock));
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    setToolBarDockWidget(dock);

    auto centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    auto centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    // Right-side window with editor, output etc.
    auto mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(this);
    auto outputPane = new OutputPanePlaceHolder(mode, mainWindowSplitter);
    outputPane->setObjectName(QLatin1String("DebuggerOutputPanePlaceHolder"));
    mainWindowSplitter->addWidget(outputPane);
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation and right-side window.
    auto splitter = new MiniSplitter;
    splitter->setFocusProxy(editorManagerPlaceHolder);
    splitter->addWidget(new NavigationWidgetPlaceHolder(mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setObjectName(QLatin1String("DebugModeWidget"));
    return splitter;
}

void DebuggerMainWindow::writeSettings() const
{
    QSettings *settings = ICore::settings();
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
    QSettings *settings = ICore::settings();
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
    if (d->m_dockWidgetActiveStateQmlCpp.isEmpty()) {
        d->m_activeDebugLanguages = DebuggerLanguage(QmlLanguage|CppLanguage);
        d->setSimpleDockWidgetArrangement();
        d->m_dockWidgetActiveStateCpp = saveSettings();
    }
    if (d->m_dockWidgetActiveStateCpp.isEmpty()) {
        d->m_activeDebugLanguages = CppLanguage;
        d->setSimpleDockWidgetArrangement();
        d->m_dockWidgetActiveStateCpp = saveSettings();
    }
    writeSettings();
}

void DebuggerMainWindowPrivate::resetDebuggerLayout()
{
    m_activeDebugLanguages = DebuggerLanguage(QmlLanguage | CppLanguage);
    setSimpleDockWidgetArrangement();
    m_dockWidgetActiveStateQmlCpp = q->saveSettings();

    m_activeDebugLanguages = CppLanguage;
    m_previousDebugLanguages = CppLanguage;
    setSimpleDockWidgetArrangement();
    // will save state in m_dockWidgetActiveStateCpp
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

void DebuggerMainWindowPrivate::setSimpleDockWidgetArrangement()
{
    using namespace Constants;
    QTC_ASSERT(q, return);
    q->setTrackingEnabled(false);

    QList<QDockWidget *> dockWidgets = q->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        dockWidget->setFloating(false);
        q->removeDockWidget(dockWidget);
    }

    foreach (QDockWidget *dockWidget, dockWidgets) {
        int area = Qt::BottomDockWidgetArea;
        QVariant p = dockWidget->property(DOCKWIDGET_DEFAULT_AREA);
        if (p.isValid())
            area = Qt::DockWidgetArea(p.toInt());
        q->addDockWidget(Qt::DockWidgetArea(area), dockWidget);
        dockWidget->hide();
    }

    QDockWidget *toolBarDock = q->toolBarDockWidget();
    QDockWidget *breakDock = dockWidget(QLatin1String(DOCKWIDGET_BREAK));
    QDockWidget *stackDock = dockWidget(QLatin1String(DOCKWIDGET_STACK));
    QDockWidget *watchDock = dockWidget(QLatin1String(DOCKWIDGET_WATCHERS));
    QDockWidget *snapshotsDock = dockWidget(QLatin1String(DOCKWIDGET_SNAPSHOTS));
    QDockWidget *threadsDock = dockWidget(QLatin1String(DOCKWIDGET_THREADS));
    QDockWidget *outputDock = dockWidget(QLatin1String(DOCKWIDGET_OUTPUT));
    QDockWidget *qmlInspectorDock = dockWidget(QLatin1String(DOCKWIDGET_QML_INSPECTOR));
    QDockWidget *modulesDock = dockWidget(QLatin1String(DOCKWIDGET_MODULES));
    QDockWidget *registerDock = dockWidget(QLatin1String(DOCKWIDGET_REGISTER));
    QDockWidget *sourceFilesDock = dockWidget(QLatin1String(DOCKWIDGET_SOURCE_FILES));

    QTC_ASSERT(breakDock, return);
    QTC_ASSERT(stackDock, return);
    QTC_ASSERT(watchDock, return);
    QTC_ASSERT(snapshotsDock, return);
    QTC_ASSERT(threadsDock, return);
    QTC_ASSERT(outputDock, return);
    QTC_ASSERT(modulesDock, return);
    QTC_ASSERT(registerDock, return);
    QTC_ASSERT(sourceFilesDock, return);

    // make sure main docks are visible so that split equally divides the space
    toolBarDock->show();
    stackDock->show();
    breakDock->show();
    watchDock->show();

    // toolBar
    // --------------------------------------------------------------------------------
    // stack,qmlinspector | breakpoints,modules,register,threads,sourceFiles,snapshots
    //
    q->splitDockWidget(toolBarDock, stackDock, Qt::Vertical);
    q->splitDockWidget(stackDock, breakDock, Qt::Horizontal);

    if (qmlInspectorDock)
        q->tabifyDockWidget(stackDock, qmlInspectorDock);

    q->tabifyDockWidget(breakDock, modulesDock);
    q->tabifyDockWidget(breakDock, registerDock);
    q->tabifyDockWidget(breakDock, threadsDock);
    q->tabifyDockWidget(breakDock, sourceFilesDock);
    q->tabifyDockWidget(breakDock, snapshotsDock);

    if (m_activeDebugLanguages.testFlag(Debugger::QmlLanguage)) {
        if (qmlInspectorDock)
            qmlInspectorDock->show();
    } else {
        // CPP only
        threadsDock->show();
        snapshotsDock->show();
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
