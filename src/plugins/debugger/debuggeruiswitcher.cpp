#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"

#include <debugger/debuggerconstants.h>
#include <utils/styledbar.h>
#include <coreplugin/modemanager.h>
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
#include <coreplugin/actionmanager/actioncontainer_p.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QSettings>

#include <QtGui/QActionGroup>
#include <QtGui/QStackedWidget>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QDockWidget>

#include <QDebug>

namespace Debugger {

using namespace Debugger::Internal;

DebuggerUISwitcher *DebuggerUISwitcher::m_instance = 0;

DebuggerUISwitcher::DebuggerUISwitcher(Core::BaseMode *mode, QObject* parent) : QObject(parent),
    m_model(new QStandardItemModel(this)),
    m_toolbarStack(new QStackedWidget),
    m_activeLanguage(-1),
    m_isActiveMode(false),
    m_changingUI(false),
    m_toggleLockedAction(0),
    m_languageActionGroup(new QActionGroup(this)),
    m_viewsMenu(0),
    m_debugMenu(0)
{
    mode->setWidget(createContents(mode));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            SLOT(modeChanged(Core::IMode*)));

    m_debugMenu = am->actionContainer(ProjectExplorer::Constants::M_DEBUG);
    m_languageMenu = am->createMenu(Debugger::Constants::M_DEBUG_LANGUAGES);
    m_languageActionGroup->setExclusive(true);
    m_viewsMenu = am->createMenu(Debugger::Constants::M_DEBUG_VIEWS);

    m_debuggercontext << Core::ICore::instance()->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER);

    m_instance = this;
}

DebuggerUISwitcher::~DebuggerUISwitcher()
{
    qDeleteAll(m_dockWidgets);
    m_dockWidgets.clear();
}

void DebuggerUISwitcher::addMenuAction(Core::Command *command, const QString &group)
{
    m_debugMenu->addAction(command, group);
}

void DebuggerUISwitcher::setActiveLanguage(const QString &langName)
{
    QModelIndex idx = modelIndexForLanguage(langName);
    if (!idx.isValid())
        return;

    changeDebuggerUI(idx.row());
}


void DebuggerUISwitcher::modeChanged(Core::IMode *mode)
{
    m_isActiveMode = (mode->id() == Debugger::Constants::MODE_DEBUG);
    hideInactiveWidgets();
}

void DebuggerUISwitcher::hideInactiveWidgets()
{
    if (!m_isActiveMode) {
        // hide all the debugger windows if mode is different
        foreach(Internal::DebugToolWindow *window, m_dockWidgets) {
            if (window->m_languageId == m_activeLanguage &&
                window->m_dockWidget->isVisible())
            {
                window->m_dockWidget->hide();
            }
        }
    } else {
        // bring them back
        foreach(Internal::DebugToolWindow *window, m_dockWidgets) {
            if (window->m_languageId == m_activeLanguage &&
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

    m_toggleLockedAction = new QAction(tr("Locked"), this);
    m_toggleLockedAction->setCheckable(true);
    m_toggleLockedAction->setChecked(true);
    connect(m_toggleLockedAction, SIGNAL(toggled(bool)),
            m_mainWindow, SLOT(setLocked(bool)));

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Views"), globalcontext);
    m_debugMenu->addAction(cmd);

    QMenu *mLang = m_languageMenu->menu();
    mLang->setEnabled(true);
    mLang->setTitle(tr("&Languages"));
    m_debugMenu->addMenu(m_languageMenu, Core::Constants::G_DEFAULT_THREE);

    QMenu *m = m_viewsMenu->menu();
    m->setEnabled(true);
    m->setTitle(tr("&Views"));
    m_debugMenu->addMenu(m_viewsMenu, Core::Constants::G_DEFAULT_THREE);

    m->addSeparator();
    m->addAction(m_toggleLockedAction);
    m->addSeparator();

    QAction *resetToSimpleAction = m_viewsMenu->menu()->addAction(tr("Reset to default layout"));
    connect(resetToSimpleAction, SIGNAL(triggered()),
            SLOT(resetDebuggerLayout()));

}

DebuggerUISwitcher *DebuggerUISwitcher::instance()
{
    return m_instance;
}

void DebuggerUISwitcher::addLanguage(const QString &langName)
{
    QStandardItem* item = new QStandardItem(langName);

    m_model->appendRow(item);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QAction *langChange = new QAction(langName, this);
    langChange->setCheckable(true);
    langChange->setChecked(false);

    m_languageActionGroup->addAction(langChange);

    connect(langChange, SIGNAL(triggered()), SLOT(langChangeTriggered()));
    Core::Command *cmd = am->registerAction(langChange,
                         "Debugger.Language" + langName, m_debuggercontext);
    m_languageMenu->addAction(cmd);

}

void DebuggerUISwitcher::langChangeTriggered()
{
    QObject *sdr = sender();
    QAction *act = qobject_cast<QAction*>(sdr);
    changeDebuggerUI(modelIndexForLanguage(act->text()).row());

}

void DebuggerUISwitcher::changeDebuggerUI(int langId)
{
    if (m_changingUI)
        return;
    m_changingUI = true;

    // id
    QModelIndex idx = m_model->index(langId, 0);
    if (langId != m_activeLanguage) {
        m_languageActionGroup->actions()[langId]->setChecked(true);
        m_activeLanguage = langId;

        m_toolbarStack->setCurrentIndex(m_model->data(idx, StackIndexRole).toInt());

        foreach (DebugToolWindow *window, m_dockWidgets) {
            if (window->m_languageId != langId) {
                // visibleTo must be used because during init, debugger is not visible,
                // although visibility is explicitly set through both default layout and
                // QSettings.
                window->m_visible = window->m_dockWidget->isVisibleTo(m_mainWindow);
                window->m_dockWidget->hide();
            } else {
                if (window->m_visible) {
                    window->m_dockWidget->show();
                }
            }
        }

        foreach (ViewsMenuItems menuitem, m_viewsMenuItems) {
            if (menuitem.first == langId) {
                menuitem.second->setVisible(true);
            } else {
                menuitem.second->setVisible(false);
            }
            qDebug() << menuitem.second->isVisible() << menuitem.first << menuitem.second->text();
        }
        m_languageMenu->menu()->setTitle(tr("Language") + " (" + idx.data().toString() + ")");
        emit languageChanged(idx.data().toString());
    }

    m_changingUI = false;
}

QModelIndex DebuggerUISwitcher::modelIndexForLanguage(const QString& languageName)
{
    QList<QStandardItem*> items = m_model->findItems(languageName);
    if (items.length() != 1)
        return QModelIndex();

    return m_model->indexFromItem(items.at(0));
}

void DebuggerUISwitcher::setToolbar(const QString &langName, QWidget *widget)
{
    QModelIndex modelIdx = modelIndexForLanguage(langName);
    if (!modelIdx.isValid())
        return;

    int stackIdx = m_toolbarStack->addWidget(widget);
    m_model->setData(modelIdx, stackIdx, StackIndexRole);

}

DebuggerMainWindow *DebuggerUISwitcher::mainWindow() const
{
    return m_mainWindow;
}

QWidget *DebuggerUISwitcher::createMainWindow(Core::BaseMode *mode)
{
    m_mainWindow = new DebuggerMainWindow(this);
    m_mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    m_mainWindow->setDocumentMode(true);

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
    debugToolBarLayout->addWidget(m_toolbarStack);
    debugToolBarLayout->addStretch();
    debugToolBarLayout->addWidget(new Utils::StyledSeparator);

    QWidget *centralWidget = new QWidget;
    m_mainWindow->setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->addWidget(debugToolBar);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    return m_mainWindow;
}


/*!
    Keep track of dock widgets so they can be shown/hidden for different languages
*/
QDockWidget *DebuggerUISwitcher::createDockWidget(const QString &langName, QWidget *widget,
                                               Qt::DockWidgetArea area, bool visibleByDefault)
{
    QDockWidget *dockWidget = m_mainWindow->addDockForWidget(widget);
    m_mainWindow->addDockWidget(area, dockWidget);
    DebugToolWindow *window = new DebugToolWindow;
    window->m_languageId = modelIndexForLanguage(langName).row();
    window->m_dockWidget = dockWidget;

    window->m_visible = visibleByDefault;
    m_dockWidgets.append(window);

    if (modelIndexForLanguage(langName).row() != m_activeLanguage)
        dockWidget->hide();

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::Command *cmd = am->registerAction(dockWidget->toggleViewAction(),
                         "Debugger." + dockWidget->objectName(), m_debuggercontext);
    m_viewsMenu->addAction(cmd);

    m_viewsMenuItems.append(qMakePair(modelIndexForLanguage(langName).row(), dockWidget->toggleViewAction()));

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

    foreach(Internal::DebugToolWindow *toolWindow, m_dockWidgets) {
        bool visible = toolWindow->m_visible;
        if (toolWindow->m_languageId == m_activeLanguage) {
            visible = toolWindow->m_dockWidget->isVisibleTo(m_mainWindow);
        }
        toolWindow->m_dockWidget->setVisible(visible);
    }

    m_mainWindow->saveSettings(s);
    s->endGroup();
}

void DebuggerUISwitcher::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String("DebugMode"));
    m_mainWindow->restoreSettings(s);
    m_toggleLockedAction->setChecked(m_mainWindow->isLocked());
    s->endGroup();

    foreach(Internal::DebugToolWindow *toolWindow, m_dockWidgets) {
        toolWindow->m_visible = toolWindow->m_dockWidget->isVisibleTo(m_mainWindow);
    }
}

void DebuggerUISwitcher::initialize()
{
    createViewsMenuItems();

    emit dockArranged(QString());
    readSettings();

    if (m_activeLanguage == -1) {
        changeDebuggerUI(0);
    }
    hideInactiveWidgets();
}

void DebuggerUISwitcher::resetDebuggerLayout()
{
    emit dockArranged(m_model->index(m_activeLanguage, 0).data().toString());
}

}
