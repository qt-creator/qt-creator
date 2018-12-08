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
#include "enginemanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
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
    void showToolBar();
    void hideToolBar();
    void restoreLayout();
    void saveLayout();
    QString settingsId() const;
    QToolButton *setupToolButton(QAction *action);

    QString m_id;
    QString m_name;
    QString m_parentPerspectiveId;
    QString m_subPerspectiveType;
    QVector<DockOperation> m_dockOperations;
    QPointer<QWidget> m_centralWidget;
    Perspective::Callback m_aboutToActivateCallback;
    QPointer<QWidget> m_innerToolBar;
    QHBoxLayout *m_innerToolBarLayout = nullptr;
    QPointer<QWidget> m_switcher;
    QString m_lastActiveSubPerspectiveId;
    QHash<QString, QVariant> m_nonPersistenSettings;
    Perspective::ShouldPersistChecker m_shouldPersistChecker;
};

class DebuggerMainWindowPrivate : public QObject
{
public:
    DebuggerMainWindowPrivate(DebuggerMainWindow *parent);

    void createToolBar();

    void selectPerspective(Perspective *perspective);
    void depopulateCurrentPerspective();
    void populateCurrentPerspective();
    void destroyPerspective(Perspective *perspective);
    void registerPerspective(Perspective *perspective);
    void resetCurrentPerspective();
    int indexInChooser(Perspective *perspective) const;
    void fixupLayoutIfNeeded();

    DebuggerMainWindow *q = nullptr;
    Perspective *m_currentPerspective = nullptr;
    QComboBox *m_perspectiveChooser = nullptr;
    QStackedWidget *m_centralWidgetStack = nullptr;
    QHBoxLayout *m_subPerspectiveSwitcherLayout = nullptr;
    QHBoxLayout *m_innerToolsLayout = nullptr;
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
    m_statusLabel->setIndent(2 * QFontMetrics(q->font()).width(QChar('x')));
    m_editorPlaceHolder = new EditorManagerPlaceHolder;

    m_perspectiveChooser = new QComboBox;
    m_perspectiveChooser->setObjectName("PerspectiveChooser");
    m_perspectiveChooser->setProperty("panelwidget", true);
    m_perspectiveChooser->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_perspectiveChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int item) {
        Perspective *perspective = Perspective::findPerspective(m_perspectiveChooser->itemData(item).toString());
        QTC_ASSERT(perspective, return);
        if (auto subPerspective = Perspective::findPerspective(perspective->d->m_lastActiveSubPerspectiveId))
            subPerspective->select();
        else
            perspective->select();
    });

    auto viewButton = new QToolButton;
    viewButton->setText(DebuggerMainWindow::tr("&Views"));

    auto closeButton = new QToolButton();
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    closeButton->setToolTip(DebuggerMainWindow::tr("Leave Debug Mode"));

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty("topBorder", true);

    // "Engine switcher" style comboboxes
    auto subPerspectiveSwitcher = new QWidget;
    m_subPerspectiveSwitcherLayout = new QHBoxLayout(subPerspectiveSwitcher);
    m_subPerspectiveSwitcherLayout->setMargin(0);
    m_subPerspectiveSwitcherLayout->setSpacing(0);

    // All perspective toolbars will get inserted here, but only
    // the current perspective's toolbar is set visible.
    auto innerTools = new QWidget;
    m_innerToolsLayout = new QHBoxLayout(innerTools);
    m_innerToolsLayout->setMargin(0);
    m_innerToolsLayout->setSpacing(0);

    auto hbox = new QHBoxLayout(toolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_perspectiveChooser);
    hbox->addWidget(subPerspectiveSwitcher);
    hbox->addWidget(innerTools);
    hbox->addWidget(m_statusLabel);
    hbox->addStretch(1);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(viewButton);
    hbox->addWidget(closeButton);

    auto dock = new QDockWidget(DebuggerMainWindow::tr("Toolbar"), q);
    dock->setObjectName(QLatin1String("Toolbar"));
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock)); // hide title bar
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    toolbar->setParent(dock);
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
    QString parentPerspective = perspective->d->m_parentPerspectiveId;
    // Add only "main" perspectives to the chooser.
    if (parentPerspective.isEmpty())
        m_perspectiveChooser->addItem(perspective->d->m_name, perspective->d->m_id);
    m_perspectives.append(perspective);
}

void DebuggerMainWindowPrivate::destroyPerspective(Perspective *perspective)
{
    if (perspective == m_currentPerspective) {
        depopulateCurrentPerspective();
        m_currentPerspective = nullptr;
    }

    m_perspectives.removeAll(perspective);

    // Dynamic perspectives are currently not visible in the chooser.
    // This might change in the future, make sure we notice.
    const int idx = indexInChooser(perspective);
    if (idx != -1)
        m_perspectiveChooser->removeItem(idx);
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
        if (Perspective *perspective = theMainWindow->d->m_currentPerspective)
            perspective->d->saveLayout();

        theMainWindow->setDockActionsVisible(false);

        // Hide dock widgets manually in case they are floating.
        for (QDockWidget *dockWidget : theMainWindow->dockWidgets()) {
            if (dockWidget->isFloating())
                dockWidget->hide();
        }
    }
}

QWidget *DebuggerMainWindow::centralWidgetStack()
{
    return theMainWindow ? theMainWindow->d->m_centralWidgetStack : nullptr;
}

void DebuggerMainWindow::addSubPerspectiveSwitcher(QWidget *widget)
{
    widget->setVisible(false);
    widget->setProperty("panelwidget", true);
    d->m_subPerspectiveSwitcherLayout->addWidget(widget);
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

void DebuggerMainWindowPrivate::resetCurrentPerspective()
{
    if (m_currentPerspective) {
        m_currentPerspective->d->m_nonPersistenSettings.clear();
        m_currentPerspective->d->hideToolBar();
    }
    depopulateCurrentPerspective();
    populateCurrentPerspective();
    if (m_currentPerspective)
        m_currentPerspective->d->saveLayout();
}

int DebuggerMainWindowPrivate::indexInChooser(Perspective *perspective) const
{
    return perspective ? m_perspectiveChooser->findData(perspective->d->m_id) : -1;
}

void DebuggerMainWindowPrivate::fixupLayoutIfNeeded()
{
    // Evil workaround for QTCREATORBUG-21455: In some so far unknown situation
    // the saveLayout/restoreLayout process leads to a situation where some docks
    // does not end up below the perspective toolbar even though they were there
    // initially, leading to an awkward dock layout.
    // This here tries to detect the situation (no other dock directly below the
    // toolbar) and "corrects" that by restoring the default layout.
    const QRect toolbarRect = m_toolBarDock->geometry();
    const int targetX = toolbarRect.left();
    const int targetY = toolbarRect.bottom();

    const QList<QDockWidget *> docks = q->dockWidgets();
    for (QDockWidget *dock : docks) {
        const QRect dockRect = dock->geometry();
        // 10 for some decoration wiggle room. Found something below? Good.
        if (targetX == dockRect.left() && qAbs(targetY - dockRect.top()) < 10)
            return;
    }

    qDebug() << "Scrambled dock layout found. Resetting it.";
    resetCurrentPerspective();
}

void DebuggerMainWindowPrivate::selectPerspective(Perspective *perspective)
{
    QTC_ASSERT(perspective, return);

    if (m_currentPerspective) {
        m_currentPerspective->d->saveLayout();
        m_currentPerspective->d->hideToolBar();
    }

    depopulateCurrentPerspective();

    m_currentPerspective = perspective;

    perspective->aboutToActivate();

    populateCurrentPerspective();

    if (m_currentPerspective) {
        m_currentPerspective->d->restoreLayout();
        fixupLayoutIfNeeded();
    }

    int index = indexInChooser(m_currentPerspective);
    if (index == -1) {
        if (Perspective *parent = Perspective::findPerspective(m_currentPerspective->d->m_parentPerspectiveId))
            index = indexInChooser(parent);
    }

    if (index != -1) {
        m_perspectiveChooser->setCurrentIndex(index);

        const int contentWidth = m_perspectiveChooser->fontMetrics().width(perspective->d->m_name);
        QStyleOptionComboBox option;
        option.initFrom(m_perspectiveChooser);
        const QSize sz(contentWidth, 1);
        const int width = m_perspectiveChooser->style()->sizeFromContents(
                    QStyle::CT_ComboBox, &option, sz).width();
        m_perspectiveChooser->setFixedWidth(width);
    }
}

void DebuggerMainWindowPrivate::depopulateCurrentPerspective()
{
    // Clean up old perspective.
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
    }
}

void DebuggerMainWindowPrivate::populateCurrentPerspective()
{
    // Create dock widgets wrapping ther perspective's widgets.
    QHash<QString, QDockWidget *> dockForDockId;
    for (const DockOperation &op : m_currentPerspective->d->m_dockOperations) {
        if (op.operationType == Perspective::Raise)
            continue;
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

    m_currentPerspective->d->showToolBar();

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

    QWidget *central = m_currentPerspective->centralWidget();
    m_centralWidgetStack->addWidget(central ? central : m_editorPlaceHolder);
    q->showCentralWidgetAction()->setText(central ? central->windowTitle()
                                                  : DebuggerMainWindow::tr("Editor"));

    m_statusLabel->clear();

    ICore::addAdditionalContext(m_currentPerspective->context());
}

// Perspective

Perspective::Perspective(const QString &id, const QString &name,
                         const QString &parentPerspectiveId,
                         const QString &subPerspectiveType)
    : d(new PerspectivePrivate)
{
    const bool shouldPersist = parentPerspectiveId.isEmpty();
    d->m_id = id;
    d->m_name = name;
    d->m_parentPerspectiveId = parentPerspectiveId;
    d->m_subPerspectiveType = subPerspectiveType;
    d->m_shouldPersistChecker = [shouldPersist] { return shouldPersist; };

    DebuggerMainWindow::ensureMainWindowExists();
    theMainWindow->d->registerPerspective(this);

    d->m_innerToolBar = new QWidget;
    d->m_innerToolBar->setVisible(false);
    theMainWindow->d->m_innerToolsLayout->addWidget(d->m_innerToolBar);

    d->m_innerToolBarLayout = new QHBoxLayout(d->m_innerToolBar);
    d->m_innerToolBarLayout->setMargin(0);
    d->m_innerToolBarLayout->setSpacing(0);
}

Perspective::~Perspective()
{
    if (theMainWindow) {
        d->saveLayout();

        delete d->m_innerToolBar;
        d->m_innerToolBar = nullptr;

        theMainWindow->d->destroyPerspective(this);
    }
    delete d;
}

void Perspective::setCentralWidget(QWidget *centralWidget)
{
    QTC_ASSERT(d->m_centralWidget == nullptr, return);
    d->m_centralWidget = centralWidget;
}

QString Perspective::id() const
{
    return d->m_id;
}

QString Perspective::name() const
{
    return d->m_name;
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

QToolButton *PerspectivePrivate::setupToolButton(QAction *action)
{
    QTC_ASSERT(action, return nullptr);
    auto toolButton = new QToolButton(m_innerToolBar);
    toolButton->setProperty("panelwidget", true);
    toolButton->setDefaultAction(action);
    m_innerToolBarLayout->addWidget(toolButton);
    return toolButton;
}

void Perspective::addToolBarAction(QAction *action)
{
    QTC_ASSERT(action, return);
    d->setupToolButton(action);
}

void Perspective::addToolBarAction(OptionalAction *action)
{
    QTC_ASSERT(action, return);
    action->m_toolButton = d->setupToolButton(action);
}

void Perspective::addToolBarWidget(QWidget *widget)
{
    QTC_ASSERT(widget, return);
    // QStyle::polish is called before it is added to the toolbar, explicitly make it a panel widget
    widget->setProperty("panelwidget", true);
    widget->setParent(d->m_innerToolBar);
    d->m_innerToolBarLayout->addWidget(widget);
}

void Perspective::useSubPerspectiveSwitcher(QWidget *widget)
{
    d->m_switcher = widget;
}

void Perspective::addToolbarSeparator()
{
    d->m_innerToolBarLayout->addWidget(new StyledSeparator(d->m_innerToolBar));
}

void Perspective::setShouldPersistChecker(const ShouldPersistChecker &checker)
{
    d->m_shouldPersistChecker = checker;
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
    m_innerToolBar->setVisible(true);
    if (m_switcher)
        m_switcher->setVisible(true);
}

void PerspectivePrivate::hideToolBar()
{
    QTC_ASSERT(m_innerToolBar, return);
    m_innerToolBar->setVisible(false);
    if (m_switcher)
        m_switcher->setVisible(false);
}

void Perspective::addWindow(QWidget *widget,
                            Perspective::OperationType type,
                            QWidget *anchorWidget,
                            bool visibleByDefault,
                            Qt::DockWidgetArea area)
{
    QTC_ASSERT(widget, return);
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
    Debugger::Internal::EngineManager::activateDebugMode();
    if (Perspective::currentPerspective() == this)
        return;
    theMainWindow->d->selectPerspective(this);
    if (Perspective *parent = Perspective::findPerspective(d->m_parentPerspectiveId))
        parent->d->m_lastActiveSubPerspectiveId = d->m_id;
    else
        d->m_lastActiveSubPerspectiveId.clear();

    const QString &lastKey = d->m_parentPerspectiveId.isEmpty() ? d->m_id : d->m_parentPerspectiveId;
    ICore::settings()->setValue(QLatin1String(LAST_PERSPECTIVE_KEY), lastKey);
}

void PerspectivePrivate::restoreLayout()
{
    if (m_nonPersistenSettings.isEmpty()) {
        //qDebug() << "PERSPECTIVE" << m_id << "RESTORE PERSISTENT FROM " << settingsId();
        QSettings *settings = ICore::settings();
        settings->beginGroup(settingsId());
        theMainWindow->restoreSettings(settings);
        settings->endGroup();
        m_nonPersistenSettings = theMainWindow->saveSettings();
    } else {
        //qDebug() << "PERSPECTIVE" << m_id << "RESTORE FROM LOCAL TEMP";
        theMainWindow->restoreSettings(m_nonPersistenSettings);
    }
}

void PerspectivePrivate::saveLayout()
{
    //qDebug() << "PERSPECTIVE" << m_id << "SAVE LOCAL TEMP";
    m_nonPersistenSettings = theMainWindow->saveSettings();

    if (m_shouldPersistChecker()) {
        //qDebug() << "PERSPECTIVE" << m_id << "SAVE PERSISTENT TO " << settingsId();
        QSettings *settings = ICore::settings();
        settings->beginGroup(settingsId());
        theMainWindow->saveSettings(settings);
        settings->endGroup();
    } else {
        //qDebug() << "PERSPECTIVE" << m_id << "NOT PERSISTENT";
    }
}

QString PerspectivePrivate::settingsId() const
{
    return m_parentPerspectiveId.isEmpty() ? m_id : (m_parentPerspectiveId + '.' + m_subPerspectiveType);
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
    QTC_ASSERT(m_toolButton, return);
    m_toolButton->setToolButtonStyle(style);
}

} // Utils
