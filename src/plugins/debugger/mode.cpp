/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "mode.h"

#include "debuggerconstants.h"
#include "debuggermanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

using namespace Core;
using namespace ExtensionSystem;
using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;


DebugMode::DebugMode(DebuggerManager *manager, QObject *parent)
  : BaseMode(tr("Debug"), Constants::MODE_DEBUG,
             QIcon(":/fancyactionbar/images/mode_Debug.png"),
             Constants::P_MODE_DEBUG, 0, parent),
    m_manager(manager)
{
    IDebuggerManagerAccessForDebugMode *managerAccess =
        m_manager->debugModeInterface();
    UniqueIDManager *uidm =
        PluginManager::instance()->getObject<ICore>()->uniqueIDManager();
    QList<int> context;
    context.append(uidm->uniqueIdentifier(Core::Constants::C_EDITORMANAGER));
    context.append(uidm->uniqueIdentifier(Constants::C_GDBDEBUGGER));
    context.append(uidm->uniqueIdentifier(Core::Constants::C_NAVIGATION_PANE));
    setContext(context);

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(this));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(this));

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);

    MiniSplitter *rightPaneSplitter = new MiniSplitter;
    rightPaneSplitter->addWidget(editorAndFindWidget);
    rightPaneSplitter->addWidget(new RightPanePlaceHolder(this));
    rightPaneSplitter->setStretchFactor(0, 1);
    rightPaneSplitter->setStretchFactor(1, 0);

    QWidget *centralWidget = new QWidget;
    QBoxLayout *toolBarAddingLayout = new QVBoxLayout(centralWidget);
    toolBarAddingLayout->setMargin(0);
    toolBarAddingLayout->setSpacing(0);
    toolBarAddingLayout->addWidget(rightPaneSplitter);

    m_manager->mainWindow()->setCentralWidget(centralWidget);

    MiniSplitter *splitter = new MiniSplitter;
    splitter->addWidget(m_manager->mainWindow());
    splitter->addWidget(new OutputPanePlaceHolder(this));
    splitter->setStretchFactor(0, 10);
    splitter->setStretchFactor(1, 0);
    splitter->setOrientation(Qt::Vertical);

    MiniSplitter *splitter2 = new MiniSplitter;
    splitter2 = new MiniSplitter;
    splitter2->addWidget(new NavigationWidgetPlaceHolder(this));
    splitter2->addWidget(splitter);
    splitter2->setStretchFactor(0, 0);
    splitter2->setStretchFactor(1, 1);

    setWidget(splitter2);

    QToolBar *toolBar = createToolBar();
    toolBarAddingLayout->addWidget(toolBar);

    managerAccess->createDockWidgets();
    m_manager->setSimpleDockWidgetArrangement();
    readSettings();

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(focusCurrentEditor(Core::IMode*)));
    widget()->setFocusProxy(EditorManager::instance());
}

DebugMode::~DebugMode()
{
    // Make sure the editor manager does not get deleted
    EditorManager::instance()->setParent(0);
}

void DebugMode::shutdown()
{
    writeSettings();
}

QToolBar *DebugMode::createToolBar()
{
    IDebuggerManagerAccessForDebugMode *managerAccess =
        m_manager->debugModeInterface();

    Core::ActionManager *am =
            ExtensionSystem::PluginManager::instance()
            ->getObject<Core::ICore>()->actionManager();
    QToolBar *debugToolBar = new QToolBar;
    debugToolBar->addAction(am->command(ProjectExplorer::Constants::DEBUG)->action());
    debugToolBar->addAction(am->command(Constants::INTERRUPT)->action());
    debugToolBar->addAction(am->command(Constants::NEXT)->action());
    debugToolBar->addAction(am->command(Constants::STEP)->action());
    debugToolBar->addAction(am->command(Constants::STEPOUT)->action());
    debugToolBar->addSeparator();
    debugToolBar->addAction(am->command(Constants::STEPI)->action());
    debugToolBar->addAction(am->command(Constants::NEXTI)->action());
    debugToolBar->addSeparator();
    debugToolBar->addWidget(new QLabel(tr("Threads:")));

    QComboBox *threadBox = new QComboBox;
    threadBox->setModel(m_manager->threadsModel());
    connect(threadBox, SIGNAL(activated(int)),
        managerAccess->threadsWindow(), SIGNAL(threadSelected(int)));
    debugToolBar->addWidget(threadBox);

    debugToolBar->addWidget(managerAccess->statusLabel());

    QWidget *stretch = new QWidget;
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    debugToolBar->addWidget(stretch);

    return debugToolBar;
}

void DebugMode::focusCurrentEditor(IMode *mode)
{
    if (mode != this)
        return;

    EditorManager *editorManager = EditorManager::instance();

    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus();
}

void DebugMode::writeSettings() const
{
    QSettings *s = settings();
    QTC_ASSERT(m_manager, return);
    QTC_ASSERT(m_manager->mainWindow(), return);
    s->beginGroup(QLatin1String("DebugMode"));
    s->setValue(QLatin1String("State"), m_manager->mainWindow()->saveState());
    //s->setValue(QLatin1String("Locked"), m_toggleLockedAction->isChecked());
    s->endGroup();
}

void DebugMode::readSettings()
{
    QSettings *s = settings();
    s->beginGroup(QLatin1String("DebugMode"));
    m_manager->mainWindow()->restoreState(s->value(QLatin1String("State"), QByteArray()).toByteArray());
    //m_toggleLockedAction->setChecked(s->value(QLatin1String("Locked"), true).toBool());
    s->endGroup();
}

QSettings *DebugMode::settings()
{
    return PluginManager::instance()->getObject<ICore>()->settings();
}
