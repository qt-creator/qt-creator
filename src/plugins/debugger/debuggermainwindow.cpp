/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "debuggermainwindow.h"
#include "debuggerconstants.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorericons.h>

#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/proxyaction.h>

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

using namespace Debugger;
using namespace Core;

namespace Utils {

const char LAST_PERSPECTIVE_KEY[]   = "LastPerspective";

DebuggerMainWindow::DebuggerMainWindow()
{
    m_controlsStackWidget = new QStackedWidget;
    m_centralWidgetStack = new QStackedWidget;
    m_statusLabel = new Utils::StatusLabel;
    m_editorPlaceHolder = new EditorManagerPlaceHolder;

    m_perspectiveChooser = new QComboBox;
    m_perspectiveChooser->setObjectName(QLatin1String("PerspectiveChooser"));
    connect(m_perspectiveChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int item) { restorePerspective(m_perspectiveChooser->itemData(item).toByteArray()); });

    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);

    connect(this, &FancyMainWindow::resetLayout,
            this, &DebuggerMainWindow::resetCurrentPerspective);
}

DebuggerMainWindow::~DebuggerMainWindow()
{
    delete m_editorPlaceHolder;
    m_editorPlaceHolder = 0;
    // As we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach (QDockWidget *dock, m_dockForDockId) {
        if (dock && !dock->parentWidget())
            delete dock;
    }

    foreach (const Perspective *perspective, m_perspectiveForPerspectiveId)
        delete perspective;
}

void DebuggerMainWindow::registerPerspective(const QByteArray &perspectiveId, const Perspective *perspective)
{
    m_perspectiveForPerspectiveId.insert(perspectiveId, perspective);
    m_perspectiveChooser->addItem(perspective->name(), perspectiveId);
    // adjust width if necessary
    const int oldWidth = m_perspectiveChooser->width();
    const int contentWidth = m_perspectiveChooser->fontMetrics().width(perspective->name());
    QStyleOptionComboBox option;
    option.initFrom(m_perspectiveChooser);
    const QSize sz(contentWidth, 1);
    const int width = m_perspectiveChooser->style()->sizeFromContents(
                QStyle::CT_ComboBox, &option, sz).width();
    if (width > oldWidth)
        m_perspectiveChooser->setFixedWidth(width);
}

void DebuggerMainWindow::registerToolbar(const QByteArray &perspectiveId, QWidget *widget)
{
    m_toolbarForPerspectiveId.insert(perspectiveId, widget);
    m_controlsStackWidget->addWidget(widget);
}

void DebuggerMainWindow::showStatusMessage(const QString &message, int timeoutMS)
{
    m_statusLabel->showStatusMessage(message, timeoutMS);
}

QDockWidget *DebuggerMainWindow::dockWidget(const QByteArray &dockId) const
{
    return m_dockForDockId.value(dockId);
}

void DebuggerMainWindow::onModeChanged(Core::Id mode)
{
    if (mode == Debugger::Constants::MODE_DEBUG) {
        setDockActionsVisible(true);
        restorePerspective({});
    } else {
        setDockActionsVisible(false);

        // Hide dock widgets manually in case they are floating.
        foreach (QDockWidget *dockWidget, dockWidgets()) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

void DebuggerMainWindow::resetCurrentPerspective()
{
    loadPerspectiveHelper(m_currentPerspectiveId, false);
}

void DebuggerMainWindow::restorePerspective(const QByteArray &perspectiveId)
{
    loadPerspectiveHelper(perspectiveId, true);

    int index = m_perspectiveChooser->findData(perspectiveId);
    if (index == -1)
        index = m_perspectiveChooser->findData(m_currentPerspectiveId);
    if (index != -1)
        m_perspectiveChooser->setCurrentIndex(index);
}

void DebuggerMainWindow::finalizeSetup()
{
    auto viewButton = new QToolButton;
    viewButton->setText(tr("Views"));

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty("topBorder", true);
    auto hbox = new QHBoxLayout(toolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_perspectiveChooser);
    hbox->addWidget(m_controlsStackWidget);
    hbox->addWidget(m_statusLabel);
    hbox->addStretch(1);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(viewButton);

    connect(viewButton, &QAbstractButton::clicked, [this, viewButton] {
        QMenu menu;
        addDockActionsToMenu(&menu);
        menu.exec(viewButton->mapToGlobal(QPoint()));
    });

    Context debugcontext(Debugger::Constants::C_DEBUGMODE);

    ActionContainer *viewsMenu = ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS);
    Command *cmd = ActionManager::registerAction(showCentralWidgetAction(),
        "Debugger.Views.ShowCentralWidget", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(menuSeparator1(),
        "Debugger.Views.Separator1", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(autoHideTitleBarsAction(),
        "Debugger.Views.AutoHideTitleBars", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(menuSeparator2(),
        "Debugger.Views.Separator2", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(resetLayoutAction(),
        "Debugger.Views.ResetSimple", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    auto dock = new QDockWidget(tr("Toolbar"));
    dock->setObjectName(QLatin1String("Toolbar"));
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock)); // hide title bar
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    dock->setWidget(toolbar);
    m_toolbarDock = dock;

    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

QWidget *createModeWindow(const Core::Id &mode, DebuggerMainWindow *mainWindow)
{
    auto editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    auto editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(mainWindow->centralWidgetStack());
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    auto documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    auto centralEditorWidget = new QWidget;
    auto centralLayout = new QVBoxLayout(centralEditorWidget);
    centralEditorWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    // Right-side window with editor, output etc.
    auto mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(mainWindow);
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(mode, mainWindowSplitter));
    auto outputPane = new OutputPanePlaceHolder(mode, mainWindowSplitter);
    outputPane->setObjectName(QLatin1String("DebuggerOutputPanePlaceHolder"));
    mainWindowSplitter->addWidget(outputPane);
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation and right-side window.
    auto splitter = new MiniSplitter;
    splitter->setFocusProxy(mainWindow->centralWidgetStack());
    splitter->addWidget(new NavigationWidgetPlaceHolder(mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setObjectName(QLatin1String("DebugModeWidget"));
    mainWindow->setCentralWidget(centralEditorWidget);

    return splitter;
}

void DebuggerMainWindow::loadPerspectiveHelper(const QByteArray &perspectiveId, bool fromStoredSettings)
{
    // Clean up old perspective.
    if (!m_currentPerspectiveId.isEmpty()) {
        saveCurrentPerspective();
        foreach (QDockWidget *dockWidget, m_dockForDockId) {
            QTC_ASSERT(dockWidget, continue);
            dockWidget->setFloating(false);
            removeDockWidget(dockWidget);
            dockWidget->hide();
            // Prevent saveState storing the data of the wrong children.
            dockWidget->setParent(0);
        }

        ICore::removeAdditionalContext(Context(Id::fromName(m_currentPerspectiveId)));
        const Perspective *perspective = m_perspectiveForPerspectiveId.value(m_currentPerspectiveId);
        QWidget *central = perspective ? perspective->centralWidget() : nullptr;
        m_centralWidgetStack->removeWidget(central ? central : m_editorPlaceHolder);
    }

    m_currentPerspectiveId = perspectiveId;

    if (m_currentPerspectiveId.isEmpty()) {
        const QSettings *settings = ICore::settings();
        m_currentPerspectiveId = settings->value(QLatin1String(LAST_PERSPECTIVE_KEY)).toByteArray();
        if (m_currentPerspectiveId.isEmpty())
            m_currentPerspectiveId = Debugger::Constants::CppPerspectiveId;
    }

    ICore::addAdditionalContext(Context(Id::fromName(m_currentPerspectiveId)));

    QTC_ASSERT(m_perspectiveForPerspectiveId.contains(m_currentPerspectiveId), return);
    const Perspective *perspective = m_perspectiveForPerspectiveId.value(m_currentPerspectiveId);
    perspective->aboutToActivate();
    for (const Perspective::Operation &operation : perspective->operations()) {
        QDockWidget *dock = m_dockForDockId.value(operation.dockId);
        if (!dock) {
            QTC_CHECK(!operation.widget->objectName().isEmpty());
            dock = registerDockWidget(operation.dockId, operation.widget);

            QAction *toggleViewAction = dock->toggleViewAction();
            toggleViewAction->setText(dock->windowTitle());

            Command *cmd = ActionManager::registerAction(toggleViewAction,
                Id("Dock.").withSuffix(dock->objectName()),
                Context(Id::fromName(m_currentPerspectiveId)));
            cmd->setAttribute(Command::CA_Hide);

            ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS)->addAction(cmd);
        }
        // Restore parent/child relation, so that the widget hierarchy is clear.
        dock->setParent(this);
        if (operation.operationType == Perspective::Raise) {
            dock->raise();
            continue;
        }
        addDockWidget(operation.area, dock);
        QDockWidget *anchor = m_dockForDockId.value(operation.anchorDockId);
        if (!anchor && operation.area == Qt::BottomDockWidgetArea)
            anchor = m_toolbarDock;
        if (anchor) {
            switch (operation.operationType) {
            case Perspective::AddToTab:
                tabifyDockWidget(anchor, dock);
                break;
            case Perspective::SplitHorizontal:
                splitDockWidget(anchor, dock, Qt::Horizontal);
                break;
            case Perspective::SplitVertical:
                splitDockWidget(anchor, dock, Qt::Vertical);
                break;
            default:
                break;
            }
        }
        if (!operation.visibleByDefault)
            dock->hide();
        else
            dock->show();
    }

    if (fromStoredSettings) {
        QSettings *settings = ICore::settings();
        settings->beginGroup(QString::fromLatin1(m_currentPerspectiveId));
        if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
            restoreSettings(settings);
        settings->endGroup();
    } else {
        // By default, show the central widget
        showCentralWidgetAction()->setChecked(true);
    }

    QWidget *central = perspective->centralWidget();
    m_centralWidgetStack->addWidget(central ? central : m_editorPlaceHolder);
    showCentralWidgetAction()->setText(central ? central->windowTitle() : tr("Editor"));

    QTC_CHECK(m_toolbarForPerspectiveId.contains(m_currentPerspectiveId));
    m_controlsStackWidget->setCurrentWidget(m_toolbarForPerspectiveId.value(m_currentPerspectiveId));
    m_statusLabel->clear();
}

void DebuggerMainWindow::saveCurrentPerspective()
{
    if (m_currentPerspectiveId.isEmpty())
        return;
    QSettings *settings = ICore::settings();
    settings->beginGroup(QString::fromLatin1(m_currentPerspectiveId));
    saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_PERSPECTIVE_KEY), m_currentPerspectiveId);
}

QDockWidget *DebuggerMainWindow::registerDockWidget(const QByteArray &dockId, QWidget *widget)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = addDockForWidget(widget);
    m_dockForDockId[dockId] = dockWidget;
    return dockWidget;
}

Perspective::~Perspective()
{
    foreach (const Operation &operation, m_operations)
        delete operation.widget;
    delete m_centralWidget;
}

QString Perspective::name() const
{
    return m_name;
}

void Perspective::setName(const QString &name)
{
    m_name = name;
}

void Perspective::setAboutToActivateCallback(const Perspective::Callback &cb)
{
    m_aboutToActivateCallback = cb;
}

void Perspective::aboutToActivate() const
{
    if (m_aboutToActivateCallback)
        m_aboutToActivateCallback();
}

QList<QWidget *> ToolbarDescription::widgets() const
{
    return m_widgets;
}

void ToolbarDescription::addAction(QAction *action, const QIcon &toolbarIcon)
{
    auto button = new QToolButton;
    button->setDefaultAction(toolbarIcon.isNull()
                             ? action : ProxyAction::proxyActionWithIcon(action, toolbarIcon));
    m_widgets.append(button);
}

void ToolbarDescription::addWidget(QWidget *widget)
{
    m_widgets.append(widget);
}

Perspective::Operation::Operation(const QByteArray &dockId, QWidget *widget, const QByteArray &anchorDockId,
                                  Perspective::OperationType splitType, bool visibleByDefault,
                                  Qt::DockWidgetArea area)
    : dockId(dockId), widget(widget), anchorDockId(anchorDockId),
      operationType(splitType), visibleByDefault(visibleByDefault), area(area)
{}

Perspective::Perspective(const QString &name, const QVector<Operation> &splits,
                         QWidget *centralWidget)
    : m_name(name), m_operations(splits), m_centralWidget(centralWidget)
{
    for (const Operation &split : splits)
        m_docks.append(split.dockId);
}

void Perspective::addOperation(const Operation &operation)
{
    m_docks.append(operation.dockId);
    m_operations.append(operation);
}

} // Utils
