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
#include "savedaction.h"

#include <utils/savedaction.h>
#include <utils/styledbar.h>
#include <coreplugin/actionmanager/command.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggeractions.h>

#include <coreplugin/modemanager.h>
#include <coreplugin/basemode.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QActionGroup>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QDockWidget>

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSettings>

namespace Debugger {
namespace Internal {

} // namespace Internal
using namespace Debugger::Internal;

// first: language id, second: menu item
typedef QPair<int, QAction* > ViewsMenuItems;

struct DebuggerUISwitcherPrivate {
    explicit DebuggerUISwitcherPrivate(DebuggerUISwitcher *q);

    QList< ViewsMenuItems > m_viewsMenuItems;
    QList< Internal::DebugToolWindow* > m_dockWidgets;

    QMap<QString, QWidget *> m_toolBars;
    QStringList m_languages;

    QStackedWidget *m_toolbarStack;
    Internal::DebuggerMainWindow *m_mainWindow;

    // global context
    QList<int> m_globalContext;

    QHash<int, QList<int> > m_contextsForLanguage;

    QActionGroup *m_languageActionGroup;

    int m_activeLanguage;
    bool m_isActiveMode;
    bool m_changingUI;

    QAction *m_toggleLockedAction;

    const static int StackIndexRole = Qt::UserRole + 11;

    Core::ActionContainer *m_languageMenu;
    Core::ActionContainer *m_viewsMenu;
    Core::ActionContainer *m_debugMenu;

    QMultiHash< int, Core::Command *> m_menuCommands;

    static DebuggerUISwitcher *m_instance;
};

DebuggerUISwitcherPrivate::DebuggerUISwitcherPrivate(DebuggerUISwitcher *q) :
    m_toolbarStack(new QStackedWidget),
    m_languageActionGroup(new QActionGroup(q)),
    m_activeLanguage(-1),
    m_isActiveMode(false),
    m_changingUI(false),
    m_toggleLockedAction(0),
    m_viewsMenu(0),
    m_debugMenu(0)
{
}

DebuggerUISwitcher *DebuggerUISwitcherPrivate::m_instance = 0;

DebuggerUISwitcher::DebuggerUISwitcher(Core::BaseMode *mode, QObject* parent) :
    QObject(parent), d(new DebuggerUISwitcherPrivate(this))
{
    mode->setWidget(createContents(mode));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            SLOT(modeChanged(Core::IMode*)));


    d->m_debugMenu = am->actionContainer(ProjectExplorer::Constants::M_DEBUG);

    d->m_viewsMenu = am->createMenu(Debugger::Constants::M_DEBUG_VIEWS);
    d->m_languageMenu = am->createMenu(Debugger::Constants::M_DEBUG_LANGUAGES);

    d->m_languageActionGroup->setExclusive(true);

    d->m_globalContext << Core::Constants::C_GLOBAL_ID;

    DebuggerUISwitcherPrivate::m_instance = this;
}

DebuggerUISwitcher::~DebuggerUISwitcher()
{
    qDeleteAll(d->m_dockWidgets);
    d->m_dockWidgets.clear();
    DebuggerUISwitcherPrivate::m_instance = 0;
    delete d;
}

void DebuggerUISwitcher::addMenuAction(Core::Command *command, const QString &langName,
                                       const QString &group)
{
    d->m_debugMenu->addAction(command, group);
    d->m_menuCommands.insert(d->m_languages.indexOf(langName), command);
}

void DebuggerUISwitcher::setActiveLanguage(const QString &langName)
{
    if (theDebuggerAction(SwitchLanguageAutomatically)->isChecked()
        && d->m_languages.contains(langName))
    {
        changeDebuggerUI(langName);
    }
}

int DebuggerUISwitcher::activeLanguageId() const
{
    return d->m_activeLanguage;
}
void DebuggerUISwitcher::modeChanged(Core::IMode *mode)
{
    d->m_isActiveMode = (mode->id() == Debugger::Constants::MODE_DEBUG);
    hideInactiveWidgets();
}

void DebuggerUISwitcher::hideInactiveWidgets()
{
    if (!d->m_isActiveMode) {
        // hide all the debugger windows if mode is different
        foreach(Internal::DebugToolWindow *window, d->m_dockWidgets) {
            if (window->m_languageId == d->m_activeLanguage &&
                window->m_dockWidget->isVisible())
            {
                window->m_dockWidget->hide();
            }
        }
    } else {
        // bring them back
        foreach(Internal::DebugToolWindow *window, d->m_dockWidgets) {
            if (window->m_languageId == d->m_activeLanguage &&
                window->m_visible &&
                !window->m_dockWidget->isVisible())
            {
                window->m_dockWidget->show();
            }
        }
    }
}

void DebuggerUISwitcher::createViewsMenuItems()
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    Core::Command *cmd = 0;

    d->m_toggleLockedAction = new QAction(tr("Locked"), this);
    d->m_toggleLockedAction->setCheckable(true);
    d->m_toggleLockedAction->setChecked(true);
    connect(d->m_toggleLockedAction, SIGNAL(toggled(bool)),
            d->m_mainWindow, SLOT(setLocked(bool)));

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Views"), globalcontext);
    d->m_debugMenu->addAction(cmd);

    QMenu *mLang = d->m_languageMenu->menu();
    mLang->setTitle(tr("&Languages"));
    d->m_debugMenu->addMenu(d->m_languageMenu, Core::Constants::G_DEFAULT_THREE);

    QMenu *m = d->m_viewsMenu->menu();
    m->setTitle(tr("&Views"));
    d->m_debugMenu->addMenu(d->m_viewsMenu, Core::Constants::G_DEFAULT_THREE);

    m->addSeparator();
    m->addAction(d->m_toggleLockedAction);
    m->addSeparator();

    QAction *resetToSimpleAction = d->m_viewsMenu->menu()->addAction(tr("Reset to default layout"));
    connect(resetToSimpleAction, SIGNAL(triggered()),
            SLOT(resetDebuggerLayout()));

}

DebuggerUISwitcher *DebuggerUISwitcher::instance()
{
    return DebuggerUISwitcherPrivate::m_instance;
}

void DebuggerUISwitcher::addLanguage(const QString &langName, const QList<int> &context)
{
    d->m_toolBars.insert(langName, 0);
    d->m_contextsForLanguage.insert(d->m_languages.count(), context);
    d->m_languages.append(langName);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QAction *langChange = new QAction(langName, this);
    langChange->setCheckable(true);
    langChange->setChecked(false);

    d->m_languageActionGroup->addAction(langChange);

    connect(langChange, SIGNAL(triggered()), SLOT(langChangeTriggered()));
    Core::Command *cmd = am->registerAction(langChange,
                         "Debugger.Language." + langName, d->m_globalContext);
    d->m_languageMenu->addAction(cmd);
}

void DebuggerUISwitcher::langChangeTriggered()
{
    QObject *sdr = sender();
    QAction *act = qobject_cast<QAction*>(sdr);
    changeDebuggerUI(act->text());
}

void DebuggerUISwitcher::changeDebuggerUI(const QString &langName)
{
    if (d->m_changingUI)
        return;
    d->m_changingUI = true;

    int langId = d->m_languages.indexOf(langName);
    if (langId != d->m_activeLanguage) {
        d->m_languageActionGroup->actions()[langId]->setChecked(true);
        d->m_toolbarStack->setCurrentWidget(d->m_toolBars.value(langName));

        foreach (DebugToolWindow *window, d->m_dockWidgets) {
            if (window->m_languageId != langId) {
                // visibleTo must be used because during init, debugger is not visible,
                // although visibility is explicitly set through both default layout and
                // QSettings.
                window->m_visible = window->m_dockWidget->isVisibleTo(d->m_mainWindow);
                window->m_dockWidget->hide();
            } else {
                if (window->m_visible) {
                    window->m_dockWidget->show();
                }
            }
        }

        foreach (ViewsMenuItems menuitem, d->m_viewsMenuItems) {
            if (menuitem.first == langId) {
                menuitem.second->setVisible(true);
            } else {
                menuitem.second->setVisible(false);
            }
        }

        d->m_languageMenu->menu()->setTitle(tr("Language") + " (" + langName + ")");
        QHashIterator<int, Core::Command *> iter(d->m_menuCommands);

        Core::ICore *core = Core::ICore::instance();
        const QList<int> &oldContexts = d->m_contextsForLanguage.value(d->m_activeLanguage);
        const QList<int> &newContexts = d->m_contextsForLanguage.value(langId);
        core->updateAdditionalContexts(oldContexts, newContexts);

        d->m_activeLanguage = langId;

        emit languageChanged(langName);
    }

    d->m_changingUI = false;
}

void DebuggerUISwitcher::setToolbar(const QString &langName, QWidget *widget)
{
    Q_ASSERT(d->m_toolBars.contains(langName));
    d->m_toolBars[langName] = widget;
    d->m_toolbarStack->addWidget(widget);
}

Utils::FancyMainWindow *DebuggerUISwitcher::mainWindow() const
{
    return d->m_mainWindow;
}

QWidget *DebuggerUISwitcher::createMainWindow(Core::BaseMode *mode)
{
    d->m_mainWindow = new DebuggerMainWindow(this);
    d->m_mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    d->m_mainWindow->setDocumentMode(true);

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

    QWidget *centralWidget = new QWidget;
    d->m_mainWindow->setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->addWidget(debugToolBar);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    return d->m_mainWindow;
}


/*!
    Keep track of dock widgets so they can be shown/hidden for different languages
*/
QDockWidget *DebuggerUISwitcher::createDockWidget(const QString &langName, QWidget *widget,
                                               Qt::DockWidgetArea area, bool visibleByDefault)
{
    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    d->m_mainWindow->addDockWidget(area, dockWidget);
    DebugToolWindow *window = new DebugToolWindow;
    window->m_languageId = d->m_languages.indexOf(langName);
    window->m_dockWidget = dockWidget;

    window->m_visible = visibleByDefault;
    d->m_dockWidgets.append(window);

    if (d->m_languages.indexOf(langName) != d->m_activeLanguage)
        dockWidget->hide();

    QList<int> langContext = d->m_contextsForLanguage.value(d->m_languages.indexOf(langName));

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::Command *cmd = am->registerAction(dockWidget->toggleViewAction(),
                         "Debugger." + dockWidget->objectName(), langContext);
    cmd->setAttribute(Core::Command::CA_Hide);
    d->m_viewsMenu->addAction(cmd);

    d->m_viewsMenuItems.append(qMakePair(d->m_languages.indexOf(langName), dockWidget->toggleViewAction()));

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

void DebuggerUISwitcher::shutdown()
{
    writeSettings();
}

void DebuggerUISwitcher::writeSettings() const
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String("DebugMode"));

    foreach(Internal::DebugToolWindow *toolWindow, d->m_dockWidgets) {
        bool visible = toolWindow->m_visible;
        if (toolWindow->m_languageId == d->m_activeLanguage) {
            visible = toolWindow->m_dockWidget->isVisibleTo(d->m_mainWindow);
        }
        toolWindow->m_dockWidget->setVisible(visible);
    }

    d->m_mainWindow->saveSettings(s);
    s->endGroup();
}

void DebuggerUISwitcher::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String("DebugMode"));
    d->m_mainWindow->restoreSettings(s);
    d->m_toggleLockedAction->setChecked(d->m_mainWindow->isLocked());
    s->endGroup();

    foreach(Internal::DebugToolWindow *toolWindow, d->m_dockWidgets) {
        toolWindow->m_visible = toolWindow->m_dockWidget->isVisibleTo(d->m_mainWindow);
    }
}

void DebuggerUISwitcher::initialize()
{
    createViewsMenuItems();

    emit dockArranged(QString());
    readSettings();

    if (d->m_activeLanguage == -1) {
        changeDebuggerUI(d->m_languages.first());
    }
    hideInactiveWidgets();
}

void DebuggerUISwitcher::resetDebuggerLayout()
{
    emit dockArranged(d->m_languages.at(d->m_activeLanguage));
}

QList<Internal::DebugToolWindow* > DebuggerUISwitcher::i_mw_debugToolWindows() const
{
    return d->m_dockWidgets;
}

}
