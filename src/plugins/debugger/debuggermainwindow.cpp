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
#include <QDebug>
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
const char OWNED_BY_PERSPECTIVE[]   = "OwnedByPerspective";

static DebuggerMainWindow *theMainWindow = nullptr;

class ToolbarOperation
{
public:
    void attachToToolbar(QWidget *parent)
    {
        QLayout *layout = parent->layout();
        if (toolBarWidget) {
            toolBarWidget->setParent(parent);
            layout->addWidget(toolBarWidget);
        }
        if (toolBarAction) {
            toolBarAction->m_toolButton = new QToolButton(parent);
            toolBarAction->m_toolButton->setToolButtonStyle(toolBarAction->m_toolButtonStyle);
            toolBarAction->m_toolButton->setProperty("panelwidget", true);
            toolBarAction->m_toolButton->setDefaultAction(toolBarAction);
            toolBarAction->m_toolButton->setVisible(toolBarAction->isVisible());
            layout->addWidget(toolBarAction->m_toolButton);
        }
        if (toolBarPlainAction) {
            toolBarPlainActionButton = new QToolButton(parent);
            toolBarPlainActionButton->setProperty("panelwidget", true);
            toolBarPlainActionButton->setDefaultAction(toolBarPlainAction);
            layout->addWidget(toolBarPlainActionButton);
        }
        if (separator) {
            separator->setParent(parent);
            layout->addWidget(separator);
        }
    }

    void detachFromToolbar()
    {
        if (toolBarWidget) {
            toolBarWidget->setParent(nullptr);
        }
        if (toolBarAction) {
            delete toolBarAction->m_toolButton;
            toolBarAction->m_toolButton = nullptr;
        }
        if (toolBarPlainAction) {
            delete toolBarPlainActionButton;
            toolBarPlainActionButton = nullptr;
        }
        if (separator) {
            separator->setParent(nullptr);
        }
    }

    QPointer<QAction> toolBarPlainAction; // Owned by plugin if present.
    QPointer<OptionalAction> toolBarAction; // Owned by plugin if present.
    QPointer<QWidget> toolBarWidget; // Owned by plugin if present.

    // Helper/wrapper widget owned by us if present.
    QPointer<QToolButton> toolBarPlainActionButton;
    QPointer<QWidget> toolBarWidgetParent;
    QPointer<QWidget> separator;
};

class DockOperation
{
public:
    QPointer<QWidget> widget;
    QString anchorDockId;
    Perspective::OperationType operationType = Perspective::Raise;
    bool visibleByDefault = true;
    Qt::DockWidgetArea area = Qt::BottomDockWidgetArea;
};

class PerspectivePrivate
{
public:
    ~PerspectivePrivate() { destroyToolBar(); }

    void showToolBar();
    void hideToolBar();
    void destroyToolBar();

    QString m_id;
    QString m_name;
    QString m_parentPerspectiveId;
    QVector<DockOperation> m_dockOperations;
    QVector<ToolbarOperation> m_toolBarOperations;
    QPointer<QWidget> m_centralWidget;
    Perspective::Callback m_aboutToActivateCallback;
    QPointer<QWidget> m_toolButtonBox;
    QString m_lastActiveSubPerspectiveId;
};

class DebuggerMainWindowPrivate : public QObject
{
public:
    DebuggerMainWindowPrivate(DebuggerMainWindow *parent);

    void ensureToolBarDockExists();

    void selectPerspective(Perspective *perspective);
    void depopulateCurrentPerspective();
    void populateCurrentPerspective();
    void savePerspectiveHelper(const Perspective *perspective);
    void destroyPerspective(Perspective *perspective);
    void registerPerspective(Perspective *perspective);
    void increaseChooserWidthIfNecessary(const QString &visibleName);
    void resetCurrentPerspective();
    int indexInChooser(Perspective *perspective) const;

    DebuggerMainWindow *q = nullptr;
    Perspective *m_currentPerspective = nullptr;
    QComboBox *m_perspectiveChooser = nullptr;
    QStackedWidget *m_centralWidgetStack = nullptr;
    QHBoxLayout *m_toolBarLayout = nullptr;
    QWidget *m_editorPlaceHolder = nullptr;
    Utils::StatusLabel *m_statusLabel = nullptr;
    QDockWidget *m_toolBarDock = nullptr;

    QList<Perspective *> m_perspectives;
};

DebuggerMainWindowPrivate::DebuggerMainWindowPrivate(DebuggerMainWindow *parent)
    : q(parent)
{
    m_centralWidgetStack = new QStackedWidget;
    m_statusLabel = new Utils::StatusLabel;
    m_statusLabel->setProperty("panelwidget", true);
    m_editorPlaceHolder = new EditorManagerPlaceHolder;

    m_perspectiveChooser = new QComboBox;
    m_perspectiveChooser->setObjectName(QLatin1String("PerspectiveChooser"));
    m_perspectiveChooser->setProperty("panelwidget", true);
    connect(m_perspectiveChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int item) {
        Perspective *perspective = Perspective::findPerspective(m_perspectiveChooser->itemData(item).toString());
        QTC_ASSERT(perspective, return);
        if (auto subPerspective = Perspective::findPerspective(perspective->d->m_lastActiveSubPerspectiveId))
            subPerspective->select();
        else
            perspective->select();
    });
}

DebuggerMainWindow::DebuggerMainWindow()
    : d(new DebuggerMainWindowPrivate(this))
{
    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);

    connect(this, &FancyMainWindow::resetLayout,
            d, &DebuggerMainWindowPrivate::resetCurrentPerspective);

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
}

DebuggerMainWindow::~DebuggerMainWindow()
{
    d->savePerspectiveHelper(d->m_currentPerspective);
    delete d;
}

void DebuggerMainWindow::ensureMainWindowExists()
{
    if (!theMainWindow)
        theMainWindow = new DebuggerMainWindow;
}

void DebuggerMainWindow::doShutdown()
{
    delete theMainWindow;
    theMainWindow = nullptr;
}

void DebuggerMainWindowPrivate::registerPerspective(Perspective *perspective)
{
    m_perspectives.append(perspective);
    QString parentPerspective = perspective->d->m_parentPerspectiveId;
    // Add only "main" perspectives to the chooser.
    if (parentPerspective.isEmpty()) {
        m_perspectiveChooser->addItem(perspective->name(), perspective->id());
        increaseChooserWidthIfNecessary(perspective->name());
    }
}

void DebuggerMainWindowPrivate::increaseChooserWidthIfNecessary(const QString &visibleName)
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

void DebuggerMainWindowPrivate::destroyPerspective(Perspective *perspective)
{
    savePerspectiveHelper(perspective);
    m_perspectives.removeAll(perspective);

    // Dynamic perspectives are currently not visible in the chooser.
    // This might change in the future, make sure we notice.
    const int idx = indexInChooser(perspective);
    if (idx != -1)
        m_perspectiveChooser->removeItem(idx);

    if (perspective == m_currentPerspective) {
        m_currentPerspective = nullptr;
        if (!perspective->d->m_parentPerspectiveId.isEmpty()) {
            if (Perspective *parent = Perspective::findPerspective(perspective->d->m_parentPerspectiveId))
                parent->select();
        }
    }
}

void DebuggerMainWindow::showStatusMessage(const QString &message, int timeoutMS)
{
    if (theMainWindow)
        theMainWindow->d->m_statusLabel->showStatusMessage(message, timeoutMS);
}

void DebuggerMainWindow::onModeChanged(Core::Id mode)
{
    if (mode == Debugger::Constants::MODE_DEBUG) {
        theMainWindow->setDockActionsVisible(true);
        Perspective *perspective = theMainWindow->d->m_currentPerspective;
        if (!perspective) {
            const QSettings *settings = ICore::settings();
            const QString lastPerspectiveId = settings->value(QLatin1String(LAST_PERSPECTIVE_KEY)).toString();
            perspective = Perspective::findPerspective(lastPerspectiveId);
            // If we don't find a perspective with the stored name, pick any.
            // This can happen e.g. when a plugin was disabled that provided
            // the stored perspective, or when the save file was modified externally.
            if (!perspective && !theMainWindow->d->m_perspectives.isEmpty())
                perspective = theMainWindow->d->m_perspectives.first();
        }
        // There's at least the debugger preset perspective that should be found above.
        QTC_ASSERT(perspective, return);
        perspective->select();
    } else {
        theMainWindow->setDockActionsVisible(false);

        // Hide dock widgets manually in case they are floating.
        foreach (QDockWidget *dockWidget, theMainWindow->dockWidgets()) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

QWidget *DebuggerMainWindow::centralWidgetStack()
{
    return theMainWindow ? theMainWindow->d->m_centralWidgetStack : nullptr;
}

DebuggerMainWindow *DebuggerMainWindow::instance()
{
    return theMainWindow;
}

Perspective *Perspective::findPerspective(const QString &perspectiveId)
{
    return Utils::findOr(theMainWindow->d->m_perspectives, nullptr, [&](Perspective *perspective) {
        return perspective->d->m_id == perspectiveId;
    });
}

void DebuggerMainWindow::closeEvent(QCloseEvent *)
{
    d->savePerspectiveHelper(d->m_currentPerspective);
}

void DebuggerMainWindowPrivate::resetCurrentPerspective()
{
    depopulateCurrentPerspective();
    populateCurrentPerspective();
}

int DebuggerMainWindowPrivate::indexInChooser(Perspective *perspective) const
{
    return perspective ? m_perspectiveChooser->findData(perspective->d->m_id) : -1;
}

void DebuggerMainWindowPrivate::selectPerspective(Perspective *perspective)
{
    QTC_ASSERT(perspective, return);

    depopulateCurrentPerspective();

    m_currentPerspective = perspective;

    perspective->aboutToActivate();

    populateCurrentPerspective();

    QSettings *settings = ICore::settings();
    settings->beginGroup(perspective->d->m_id);
    if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
        q->restoreSettings(settings);
    settings->endGroup();

    const int index = indexInChooser(m_currentPerspective);
    if (index != -1)
        m_perspectiveChooser->setCurrentIndex(index);
}

void DebuggerMainWindowPrivate::ensureToolBarDockExists()
{
    if (m_toolBarDock)
        return;

    auto viewButton = new QToolButton;
    viewButton->setText(tr("&Views"));

    auto closeButton = new QToolButton();
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    closeButton->setToolTip(tr("Leave Debug Mode"));

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty("topBorder", true);
    auto hbox = new QHBoxLayout(toolbar);
    m_toolBarLayout = hbox;
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_perspectiveChooser);
    // <- All perspective toolbars will get inserted here, but only
    // the current perspective's toolbar is set visible.
    hbox->addStretch(3);
    hbox->addWidget(m_statusLabel);
    hbox->addStretch(1);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(viewButton);
    hbox->addWidget(closeButton);

    auto dock = new QDockWidget(tr("Toolbar"));
    dock->setObjectName(QLatin1String("Toolbar"));
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock)); // hide title bar
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    dock->setWidget(toolbar);
    m_toolBarDock = dock;
    q->addDockWidget(Qt::BottomDockWidgetArea, m_toolBarDock);

    connect(viewButton, &QAbstractButton::clicked, [this, viewButton] {
        QMenu menu;
        q->addDockActionsToMenu(&menu);
        menu.exec(viewButton->mapToGlobal(QPoint()));
    });

    connect(closeButton, &QAbstractButton::clicked, [] {
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
    });
}

QWidget *createModeWindow(const Core::Id &mode)
{
    auto editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    auto editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(theMainWindow->centralWidgetStack());
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
    mainWindowSplitter->addWidget(theMainWindow);
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(mode, mainWindowSplitter));
    auto outputPane = new OutputPanePlaceHolder(mode, mainWindowSplitter);
    outputPane->setObjectName(QLatin1String("DebuggerOutputPanePlaceHolder"));
    mainWindowSplitter->addWidget(outputPane);
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation and right-side window.
    auto splitter = new MiniSplitter;
    splitter->setFocusProxy(theMainWindow->centralWidgetStack());
    splitter->addWidget(new NavigationWidgetPlaceHolder(mode, Side::Left));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setObjectName(QLatin1String("DebugModeWidget"));
    theMainWindow->setCentralWidget(centralEditorWidget);

    return splitter;
}

void DebuggerMainWindowPrivate::depopulateCurrentPerspective()
{
    // Clean up old perspective.
    savePerspectiveHelper(m_currentPerspective);
    for (QDockWidget *dock : q->dockWidgets()) {
        if (dock->property(OWNED_BY_PERSPECTIVE).toBool()) {
            // Prevent deletion of plugin-owned widgets.
            if (dock->widget())
                dock->widget()->setParent(nullptr);
            ActionManager::unregisterAction(dock->toggleViewAction(),
                                            Id("Dock.").withSuffix(dock->objectName()));
            delete dock;
        }
    }

    if (m_currentPerspective) {
        ICore::removeAdditionalContext(m_currentPerspective->context());
        QWidget *central = m_currentPerspective->centralWidget();
        m_centralWidgetStack->removeWidget(central ? central : m_editorPlaceHolder);

        m_currentPerspective->d->destroyToolBar();
    }
}

void DebuggerMainWindowPrivate::populateCurrentPerspective()
{
    // Create dock widgets wrapping ther perspective's widgets.
    QHash<QString, QDockWidget *> dockForDockId;
    for (const DockOperation &op : m_currentPerspective->d->m_dockOperations) {
        QTC_ASSERT(op.widget, continue);
        const QString dockId = op.widget->objectName();
        QTC_CHECK(!dockId.isEmpty());
        QTC_ASSERT(!dockForDockId.contains(dockId), continue);
        QDockWidget *dock = q->addDockForWidget(op.widget);
        dock->setProperty(OWNED_BY_PERSPECTIVE, true);
        dockForDockId[dockId] = dock;
        q->addDockWidget(op.area, dock);

        QAction *toggleViewAction = dock->toggleViewAction();
        toggleViewAction->setText(dock->windowTitle());

        Command *cmd = ActionManager::registerAction(toggleViewAction,
                                                     Id("Dock.").withSuffix(dock->objectName()),
                                                     m_currentPerspective->context());
        cmd->setAttribute(Command::CA_Hide);
        ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS)->addAction(cmd);
    }

    // Pre-arrange dock widgets.
    for (const DockOperation &op : m_currentPerspective->d->m_dockOperations) {
        QTC_ASSERT(op.widget, continue);
        const QString dockId = op.widget->objectName();
        QDockWidget *dock = dockForDockId.value(dockId);
        QTC_ASSERT(dock, continue);
        if (op.operationType == Perspective::Raise) {
            dock->raise();
            continue;
        }
        QDockWidget *anchor = dockForDockId.value(op.anchorDockId);
        if (!anchor && op.area == Qt::BottomDockWidgetArea) {
            ensureToolBarDockExists();
            anchor = m_toolBarDock;
        }

        if (anchor) {
            switch (op.operationType) {
            case Perspective::AddToTab:
                q->tabifyDockWidget(anchor, dock);
                break;
            case Perspective::SplitHorizontal:
                q->splitDockWidget(anchor, dock, Qt::Horizontal);
                break;
            case Perspective::SplitVertical:
                q->splitDockWidget(anchor, dock, Qt::Vertical);
                break;
            default:
                break;
            }
        }
        dock->setVisible(op.visibleByDefault);
    }

    m_currentPerspective->d->showToolBar();

    QWidget *central = m_currentPerspective->centralWidget();
    m_centralWidgetStack->addWidget(central ? central : m_editorPlaceHolder);
    q->showCentralWidgetAction()->setText(central ? central->windowTitle() : tr("Editor"));

    m_statusLabel->clear();

    ICore::addAdditionalContext(m_currentPerspective->context());
}

void DebuggerMainWindowPrivate::savePerspectiveHelper(const Perspective *perspective)
{
    if (!perspective)
        return;
    QSettings *settings = ICore::settings();
    settings->beginGroup(perspective->d->m_id);
    q->saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_PERSPECTIVE_KEY), perspective->d->m_id);
}

// Perspective

Perspective::Perspective(const QString &id, const QString &name, const QString &parentPerspectiveId)
    : d(new PerspectivePrivate)
{
    d->m_id = id;
    d->m_name = name;
    d->m_parentPerspectiveId = parentPerspectiveId;

    DebuggerMainWindow::ensureMainWindowExists();
    theMainWindow->d->registerPerspective(this);
}

Perspective::~Perspective()
{
    if (theMainWindow) {
        theMainWindow->d->destroyPerspective(this);
    }
    delete d;
}

void Perspective::setCentralWidget(QWidget *centralWidget)
{
    QTC_ASSERT(d->m_centralWidget == nullptr, return);
    d->m_centralWidget = centralWidget;
}

QString Perspective::name() const
{
    return d->m_name;
}

QString Perspective::id() const
{
    return d->m_id;
}

void Perspective::setAboutToActivateCallback(const Perspective::Callback &cb)
{
    d->m_aboutToActivateCallback = cb;
}

void Perspective::aboutToActivate() const
{
    if (d->m_aboutToActivateCallback)
        d->m_aboutToActivateCallback();
}

void Perspective::setEnabled(bool enabled)
{
    QTC_ASSERT(theMainWindow, return);
    const int index = theMainWindow->d->indexInChooser(this);
    QTC_ASSERT(index != -1, return);
    auto model = qobject_cast<QStandardItemModel*>(theMainWindow->d->m_perspectiveChooser->model());
    QTC_ASSERT(model, return);
    QStandardItem *item = model->item(index, 0);
    item->setFlags(enabled ? item->flags() | Qt::ItemIsEnabled : item->flags() & ~Qt::ItemIsEnabled );
}

void Perspective::addToolBarAction(QAction *action)
{
    auto toolButton = new QToolButton;
    ToolbarOperation op;
    op.toolBarPlainAction = action;
    op.toolBarPlainActionButton = toolButton;
    d->m_toolBarOperations.append(op);
}

void Perspective::addToolBarAction(OptionalAction *action)
{
    ToolbarOperation op;
    op.toolBarAction = action;
    d->m_toolBarOperations.append(op);
}

void Perspective::addToolBarWidget(QWidget *widget)
{
    ToolbarOperation op;
    op.toolBarWidget = widget;
    // QStyle::polish is called before it is added to the toolbar, explicitly make it a panel widget
    op.toolBarWidget->setProperty("panelwidget", true);
    d->m_toolBarOperations.append(op);
}

void Perspective::addToolbarSeparator()
{
    ToolbarOperation op;
    op.separator = new StyledSeparator;
    d->m_toolBarOperations.append(op);
}

QWidget *Perspective::centralWidget() const
{
    return d->m_centralWidget;
}

Perspective *Perspective::currentPerspective()
{
    return theMainWindow ? theMainWindow->d->m_currentPerspective : nullptr;
}

Context Perspective::context() const
{
    return Context(Id::fromName(d->m_id.toUtf8()));
}

void PerspectivePrivate::showToolBar()
{
    if (!m_toolButtonBox) {
        m_toolButtonBox = new QWidget;
        theMainWindow->d->m_toolBarLayout->insertWidget(1, m_toolButtonBox);

        auto hbox = new QHBoxLayout(m_toolButtonBox);
        hbox->setMargin(0);
        hbox->setSpacing(0);
        for (ToolbarOperation &op : m_toolBarOperations)
            op.attachToToolbar(m_toolButtonBox);
    }

    for (ToolbarOperation &op : m_toolBarOperations) {
        if (op.toolBarAction)
            op.toolBarAction->m_toolButton->setVisible(op.toolBarAction->isVisible());
    }

    m_toolButtonBox->setVisible(true);
}

void PerspectivePrivate::hideToolBar()
{
    if (m_toolButtonBox)
        m_toolButtonBox->setVisible(false);

}
void PerspectivePrivate::destroyToolBar()
{
    // Detach potentially re-used widgets to prevent deletion.
    for (ToolbarOperation &op : m_toolBarOperations)
        op.detachFromToolbar();

    delete m_toolButtonBox;
    m_toolButtonBox = nullptr;
}

void Perspective::addWindow(QWidget *widget,
                            Perspective::OperationType type,
                            QWidget *anchorWidget,
                            bool visibleByDefault,
                            Qt::DockWidgetArea area)
{
    DockOperation op;
    op.widget = widget;
    if (anchorWidget)
        op.anchorDockId = anchorWidget->objectName();
    op.operationType = type;
    op.visibleByDefault = visibleByDefault;
    op.area = area;
    d->m_dockOperations.append(op);
}

void Perspective::select()
{
    ModeManager::activateMode(Debugger::Constants::MODE_DEBUG);
    theMainWindow->d->selectPerspective(this);
    if (Perspective *parent = Perspective::findPerspective(d->m_parentPerspectiveId))
        parent->d->m_lastActiveSubPerspectiveId = d->m_id;
    else
        d->m_lastActiveSubPerspectiveId.clear();
}

// ToolbarAction

OptionalAction::OptionalAction(const QString &text)
    : QAction(text)
{
}

OptionalAction::~OptionalAction()
{
    delete m_toolButton;
}

void OptionalAction::setVisible(bool on)
{
    QAction::setVisible(on);
    if (m_toolButton)
        m_toolButton->setVisible(on);
}

void OptionalAction::setToolButtonStyle(Qt::ToolButtonStyle style)
{
    m_toolButtonStyle = style;
}

} // Utils
