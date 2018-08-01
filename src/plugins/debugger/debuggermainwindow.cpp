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
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorericons.h>

#include <utils/algorithm.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/proxyaction.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTimer>
#include <QToolButton>

using namespace Debugger;
using namespace Core;

namespace Utils {

const char LAST_PERSPECTIVE_KEY[]   = "LastPerspective";

DebuggerMainWindow::DebuggerMainWindow()
{
    m_centralWidgetStack = new QStackedWidget;
    m_statusLabel = new Utils::StatusLabel;
    m_editorPlaceHolder = new EditorManagerPlaceHolder;

    m_perspectiveChooser = new QComboBox;
    m_perspectiveChooser->setObjectName(QLatin1String("PerspectiveChooser"));
    connect(m_perspectiveChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int item) {
        restorePerspective(findPerspective(m_perspectiveChooser->itemData(item).toByteArray()));
    });

    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);

    connect(this, &FancyMainWindow::resetLayout,
            this, &DebuggerMainWindow::resetCurrentPerspective);
}

DebuggerMainWindow::~DebuggerMainWindow()
{
    savePerspectiveHelper(m_currentPerspective);

    delete m_editorPlaceHolder;
    m_editorPlaceHolder = nullptr;
    // As we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach (QDockWidget *dock, m_dockForDockId) {
        if (dock && !dock->parentWidget())
            delete dock;
    }

    foreach (const Perspective *perspective, m_perspectives)
        delete perspective;
}

void DebuggerMainWindow::registerPerspective(const QByteArray &perspectiveId, Perspective *perspective)
{
    m_perspectives.append(perspective);
    perspective->m_id = perspectiveId;
    QByteArray parentPerspective = perspective->parentPerspective();
    // Add "main" perspectives to the chooser.
    if (parentPerspective.isEmpty()) {
        m_perspectiveChooser->addItem(perspective->name(), perspective->m_id);
        increaseChooserWidthIfNecessary(perspective->name());
    }
}

void DebuggerMainWindow::increaseChooserWidthIfNecessary(const QString &visibleName)
{
    const int oldWidth = m_perspectiveChooser->width();
    const int contentWidth = m_perspectiveChooser->fontMetrics().width(visibleName);
    QStyleOptionComboBox option;
    option.initFrom(m_perspectiveChooser);
    const QSize sz(contentWidth, 1);
    const int width = m_perspectiveChooser->style()->sizeFromContents(
                QStyle::CT_ComboBox, &option, sz).width();
    if (width > oldWidth)
        m_perspectiveChooser->setFixedWidth(width);
}

void DebuggerMainWindow::destroyDynamicPerspective(Perspective *perspective)
{
    QTC_ASSERT(perspective, return);
    savePerspectiveHelper(perspective);

    m_perspectives.removeAll(perspective);
    // Dynamic perspectives are currently not visible in the chooser.
    // This might change in the future, make sure we notice.
    const int idx = indexInChooser(perspective);
    QTC_ASSERT(idx == -1, m_perspectiveChooser->removeItem(idx));
    QByteArray parentPerspective = perspective->parentPerspective();
    delete perspective;
    // All dynamic perspectives currently have a static parent perspective.
    // This might change in the future, make sure we notice.
    QTC_CHECK(!parentPerspective.isEmpty());
    restorePerspective(findPerspective(parentPerspective));
}

void DebuggerMainWindow::showStatusMessage(const QString &message, int timeoutMS)
{
    m_statusLabel->showStatusMessage(message, timeoutMS);
}

void DebuggerMainWindow::raiseDock(const QByteArray &dockId)
{
    QDockWidget *dock = m_dockForDockId.value(dockId);
    QTC_ASSERT(dock, return);
    QAction *act = dock->toggleViewAction();
    if (!act->isChecked())
        QTimer::singleShot(1, act, [act] { act->trigger(); });
    dock->raise();
}

QByteArray DebuggerMainWindow::currentPerspective() const
{
    return m_currentPerspective ? m_currentPerspective->m_id : QByteArray();
}

void DebuggerMainWindow::onModeChanged(Core::Id mode)
{
    if (mode == Debugger::Constants::MODE_DEBUG) {
        setDockActionsVisible(true);
        restorePerspective(nullptr);
    } else {
        setDockActionsVisible(false);

        // Hide dock widgets manually in case they are floating.
        foreach (QDockWidget *dockWidget, dockWidgets()) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

void DebuggerMainWindow::setPerspectiveEnabled(const QByteArray &perspectiveId, bool enabled)
{
    Perspective *perspective = findPerspective(perspectiveId);
    const int index = indexInChooser(perspective);
    QTC_ASSERT(index != -1, return);
    auto model = qobject_cast<QStandardItemModel*>(m_perspectiveChooser->model());
    QTC_ASSERT(model, return);
    QStandardItem *item = model->item(index, 0);
    item->setFlags(enabled ? item->flags() | Qt::ItemIsEnabled : item->flags() & ~Qt::ItemIsEnabled );
}

Perspective *DebuggerMainWindow::findPerspective(const QByteArray &perspectiveId) const
{
    return Utils::findOr(m_perspectives, nullptr, [&](Perspective *perspective) {
        return perspective->m_id == perspectiveId;
    });
}

void DebuggerMainWindow::resetCurrentPerspective()
{
    loadPerspectiveHelper(m_currentPerspective, false);
}

int DebuggerMainWindow::indexInChooser(Perspective *perspective) const
{
     return perspective ? m_perspectiveChooser->findData(perspective->m_id) : -1;
}

void DebuggerMainWindow::restorePerspective(Perspective *perspective)
{
    loadPerspectiveHelper(perspective, true);

    const int index = indexInChooser(m_currentPerspective);
    if (index != -1)
        m_perspectiveChooser->setCurrentIndex(index);
}

void DebuggerMainWindow::finalizeSetup()
{
    auto viewButton = new QToolButton;
    viewButton->setText(tr("&Views"));

    auto closeButton = new QToolButton();
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    closeButton->setToolTip(tr("Leave Debug Mode"));

    auto toolbuttonBox = new QWidget;
    m_toolbuttonBoxLayout = new QHBoxLayout(toolbuttonBox);
    m_toolbuttonBoxLayout->setMargin(0);
    m_toolbuttonBoxLayout->setSpacing(0);

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty("topBorder", true);
    auto hbox = new QHBoxLayout(toolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_perspectiveChooser);
    hbox->addWidget(toolbuttonBox);
    hbox->addStretch(3);
    hbox->addWidget(m_statusLabel);
    hbox->addStretch(1);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(viewButton);
    hbox->addWidget(closeButton);

    connect(viewButton, &QAbstractButton::clicked, [this, viewButton] {
        QMenu menu;
        addDockActionsToMenu(&menu);
        menu.exec(viewButton->mapToGlobal(QPoint()));
    });

    connect(closeButton, &QAbstractButton::clicked, [] {
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
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
    splitter->addWidget(new NavigationWidgetPlaceHolder(mode, Side::Left));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setObjectName(QLatin1String("DebugModeWidget"));
    mainWindow->setCentralWidget(centralEditorWidget);

    return splitter;
}

void DebuggerMainWindow::loadPerspectiveHelper(Perspective *perspective, bool fromStoredSettings)
{
    // Clean up old perspective.
    if (m_currentPerspective) {
        savePerspectiveHelper(m_currentPerspective);
        for (QDockWidget *dock : m_dockForDockId) {
            QTC_ASSERT(dock, continue);
            dock->setParent(nullptr);
            dock->widget()->setParent(nullptr);
            ActionManager::unregisterAction(dock->toggleViewAction(),
                Id("Dock.").withSuffix(dock->objectName()));
            delete dock;
        }
        m_dockForDockId.clear();

        ICore::removeAdditionalContext(Context(Id::fromName(m_currentPerspective->m_id)));
        QWidget *central = m_currentPerspective->centralWidget();
        m_centralWidgetStack->removeWidget(central ? central : m_editorPlaceHolder);

        // Detach potentially re-used widgets to prevent deletion.
        for (const Perspective::ToolbarOperation &op : m_currentPerspective->m_toolbarOperations) {
            if (op.widget)
                op.widget->setParent(nullptr);
            if (op.toolbutton)
                op.toolbutton->setParent(nullptr);
            if (op.separator)
                op.separator->setParent(nullptr);
        }

        while (QLayoutItem *item = m_toolbuttonBoxLayout->takeAt(0))
            delete item;
    }

    if (perspective) {
        m_currentPerspective = perspective;
    } else {
        const QSettings *settings = ICore::settings();
        m_currentPerspective = findPerspective(settings->value(QLatin1String(LAST_PERSPECTIVE_KEY)).toByteArray());
        if (!m_currentPerspective)
            m_currentPerspective = findPerspective(Debugger::Constants::CppPerspectiveId);
        if (!m_currentPerspective && !m_perspectives.isEmpty())
            m_currentPerspective = m_perspectives.first();
    }
    QTC_ASSERT(m_currentPerspective, return);

    ICore::addAdditionalContext(Context(Id::fromName(m_currentPerspective->m_id)));

    m_currentPerspective->aboutToActivate();

    for (const Perspective::ToolbarOperation &op : m_currentPerspective->m_toolbarOperations) {
        if (op.widget)
            m_toolbuttonBoxLayout->addWidget(op.widget);
        if (op.toolbutton)
            m_toolbuttonBoxLayout->addWidget(op.toolbutton);
        if (op.separator)
            m_toolbuttonBoxLayout->addWidget(op.separator);
    }

    for (const Perspective::Operation &op : m_currentPerspective->m_operations) {
        QTC_ASSERT(op.widget, continue);
        const QByteArray dockId = op.widget->objectName().toUtf8();
        QDockWidget *dock = m_dockForDockId.value(dockId);
        if (!dock) {
            QTC_CHECK(!dockId.isEmpty());
            dock = addDockForWidget(op.widget);
            m_dockForDockId[dockId] = dock;

            QAction *toggleViewAction = dock->toggleViewAction();
            toggleViewAction->setText(dock->windowTitle());

            Command *cmd = ActionManager::registerAction(toggleViewAction,
                Id("Dock.").withSuffix(dock->objectName()),
                Context(Id::fromName(m_currentPerspective->m_id)));
            cmd->setAttribute(Command::CA_Hide);

            ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS)->addAction(cmd);
        }
        // Restore parent/child relation, so that the widget hierarchy is clear.
        dock->setParent(this);
        if (op.operationType == Perspective::Raise) {
            dock->raise();
            continue;
        }
        addDockWidget(op.area, dock);
        QDockWidget *anchor = m_dockForDockId.value(op.anchorDockId);
        if (!anchor && op.area == Qt::BottomDockWidgetArea)
            anchor = m_toolbarDock;
        if (anchor) {
            switch (op.operationType) {
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
        if (!op.visibleByDefault)
            dock->hide();
        else
            dock->show();
    }

    if (fromStoredSettings) {
        QSettings *settings = ICore::settings();
        settings->beginGroup(QString::fromLatin1(m_currentPerspective->m_id));
        if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
            restoreSettings(settings);
        settings->endGroup();
    } else {
        // By default, show the central widget
        showCentralWidgetAction()->setChecked(true);
    }

    QWidget *central = m_currentPerspective->centralWidget();
    m_centralWidgetStack->addWidget(central ? central : m_editorPlaceHolder);
    showCentralWidgetAction()->setText(central ? central->windowTitle() : tr("Editor"));

    m_statusLabel->clear();
}

void DebuggerMainWindow::savePerspectiveHelper(const Perspective *perspective)
{
    if (!perspective)
        return;
    QSettings *settings = ICore::settings();
    settings->beginGroup(QString::fromLatin1(perspective->m_id));
    saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_PERSPECTIVE_KEY), perspective->m_id);
}

Perspective::~Perspective()
{
    for (const ToolbarOperation &op : m_toolbarOperations) {
        // op.widget and op.actions are owned by the plugins
        delete op.toolbutton;
        delete op.separator;
    }
}

void Perspective::setCentralWidget(QWidget *centralWidget)
{
    QTC_ASSERT(m_centralWidget == nullptr, return);
    m_centralWidget = centralWidget;
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

QByteArray Perspective::parentPerspective() const
{
    return m_parentPerspective;
}

void Perspective::setParentPerspective(const QByteArray &parentPerspective)
{
    m_parentPerspective = parentPerspective;
}

QToolButton *Perspective::addToolbarAction(QAction *action, const QIcon &toolbarIcon)
{
    ToolbarOperation op;
    op.action = action;
    op.icon = toolbarIcon;
    op.toolbutton = new QToolButton;
    op.toolbutton->setDefaultAction(toolbarIcon.isNull()
         ? action : ProxyAction::proxyActionWithIcon(action, toolbarIcon));
    m_toolbarOperations.append(op);

    return op.toolbutton;
}

void Perspective::addToolbarWidget(QWidget *widget)
{
    ToolbarOperation op;
    op.widget = widget;
    m_toolbarOperations.append(op);
}

void Perspective::addToolbarSeparator()
{
    ToolbarOperation op;
    op.separator = new StyledSeparator;
    m_toolbarOperations.append(op);
}

QWidget *Perspective::centralWidget() const
{
    return m_centralWidget;
}

Perspective::Perspective(const QString &name)
    : m_name(name)
{
}

void Perspective::addWindow(QWidget *widget, OperationType type, QWidget *anchorWidget,
                            bool visibleByDefault, Qt::DockWidgetArea area)
{
    Operation op;
    op.widget = widget;
    if (anchorWidget)
        op.anchorDockId = anchorWidget->objectName().toUtf8();
    op.operationType = type;
    op.visibleByDefault = visibleByDefault;
    op.area = area;
    m_operations.append(op);
}

} // Utils
