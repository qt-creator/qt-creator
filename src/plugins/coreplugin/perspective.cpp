// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perspective.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "editormanager/editormanager.h"
#include "icore.h"
#include "modemanager.h"
#include "rightpane.h"

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/storekey.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDataStream>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLoggingCategory>
#include <QMenu>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>

static Q_LOGGING_CATEGORY(perspectivesLog, "qtc.utils.perspectives", QtWarningMsg)

namespace Core {

using namespace Utils;

const char LAST_PERSPECTIVE_KEY[]   = "LastPerspective";
const char SHOW_CENTRALWIDGET_KEY[] = "ShowCentralWidget";
const char STATE_KEY2[]             = "State2";
const char CHANGED_DOCK_KEY[]       = "ChangedDocks";
const char SAVES_HEADER_KEY[]       = "SavesHeader";

const char MAINWINDOW_STATE_KEY[]   = "MainWindow";
const char HEADERVIEW_STATES_KEY[]  = "HeaderViewStates";

// Settings group of the default (Debug) view. Kept for backward compatibility.
const char DEFAULT_SETTINGS_GROUP[] = "Debugger.MainWindow";

// The default (Debug) view, plus all views (default and additional ones).
static PerspectivesView *theDefaultView = nullptr;
static QList<PerspectivesView *> theViews;

class PerspectiveState
{
public:
    bool hasWindowState() const { return !mainWindowState.isEmpty(); }

    bool restoreWindowState(FancyMainWindow *mainWindow)
    {
        if (!mainWindowState.isEmpty())
            return mainWindow->restoreSettings(mainWindowState);
        return false;
    }

    Store toSettings() const
    {
        Store result;
        result.insert(MAINWINDOW_STATE_KEY, QVariant::fromValue(mainWindowState));
        result.insert(HEADERVIEW_STATES_KEY, QVariant::fromValue(headerViewStates));
        return result;
    }

    static PerspectiveState fromSettings(const Store &settings)
    {
        PerspectiveState state;
        state.mainWindowState = settings.value(MAINWINDOW_STATE_KEY).value<Store>();
        state.headerViewStates = settings.value(HEADERVIEW_STATES_KEY).value<QVariantHash>();
        return state;
    }

    friend QDataStream &operator>>(QDataStream &ds, PerspectiveState &state)
    {
        QByteArray mainWindowStateLegacy;
        ds >> mainWindowStateLegacy >> state.headerViewStates;
        state.mainWindowState.clear();
        state.mainWindowState.insert("State", mainWindowStateLegacy);
        return ds;
    }
    friend QDataStream &operator<<(QDataStream &ds, const PerspectiveState &state)
    {
        return ds << state.mainWindowState.value("State") << state.headerViewStates;
    }

    Store mainWindowState;
    QVariantHash headerViewStates;
};

class InnerMainWindow final : public FancyMainWindow
{
protected:
    void contextMenuEvent(QContextMenuEvent *ev) final
    {
        ActionContainer *viewsMenu = ActionManager::actionContainer(Core::Constants::M_VIEW_VIEWS);
        QTC_ASSERT(viewsMenu, return);
        viewsMenu->menu()->exec(ev->globalPos());
    }
};

class DockOperation
{
public:
    void setupLayout();
    void ensureDockExists();

    QString name() const { QTC_ASSERT(widget, return QString()); return widget->objectName(); }
    bool changedByUser() const;
    void recordVisibility();

    PerspectivesView *view = nullptr; // The view owning the perspective this dock belongs to.
    Utils::Id commandId;
    QPointer<QWidget> widget;
    QPointer<QDockWidget> dock;
    QPointer<QWidget> anchorWidget;
    QPointer<Utils::ProxyAction> toggleViewAction;
    Perspective::OperationType operationType = Perspective::Raise;
    bool visibleByDefault = true;
    Qt::DockWidgetArea area = Qt::BottomDockWidgetArea;
};

class PerspectivePrivate
{
public:
    PerspectivePrivate() = default;

    void showInnerToolBar();
    void hideInnerToolBar();
    void restoreLayout();
    void saveLayout();
    void resetPerspective();
    void populatePerspective();
    void depopulatePerspective();
    void saveAsLastUsedPerspective();
    Context context() const;

    QString settingsId() const;
    void setupToolButton(QAction *action, Qt::ToolButtonStyle style);

    PerspectivesView *m_view = nullptr; // The view this perspective is registered with.
    QString m_id;
    QString m_name;
    QString m_parentPerspectiveId;
    QString m_settingsId;
    QList<DockOperation> m_dockOperations;
    Perspective::Callback m_aboutToActivateCallback;
    QWidget *m_innerToolBar = nullptr;
    QHBoxLayout *m_innerToolBarLayout = nullptr;
    QList<QPointer<QWidget>> m_toolBarWidgets;
    QPointer<QWidget> m_switcher;
    QString m_lastActiveSubPerspectiveId;
};

// ensures that all items of the popup are readable
class TweakedCombo final : public QComboBox
{
public:
    TweakedCombo() = default;

    void showPopup() final
    {
        QTC_ASSERT(view(), return);
        view()->setMinimumWidth(view()->sizeHintForColumn(0));
        QComboBox::showPopup();
    }
};

class PerspectivesViewPrivate : public QObject
{
public:
    PerspectivesViewPrivate(PerspectivesView *parent,
                            const QString &settingsGroup,
                            const QString &statusObjectName);

    void depopulateCurrentPerspective();
    void populateCurrentPerspective();
    void destroyPerspective(Perspective *perspective);
    void registerPerspective(Perspective *perspective);
    void resetCurrentPerspective();
    int indexInChooser(Perspective *perspective) const;
    void updatePerspectiveChooserWidth();

    void cleanDocks();
    void setCentralWidget(QWidget *widget);

    QDockWidget *dockForWidget(QWidget *widget) const;

    void setCurrentPerspective(Perspective *perspective)
    {
        const Core::Context oldContext = m_currentPerspective
                ? Context(Id::fromString(m_currentPerspective->id())) : Context();
        m_currentPerspective = perspective;
        const Core::Context newContext = m_currentPerspective
                ? Context(Id::fromString(m_currentPerspective->id())) : Context();
        ICore::updateAdditionalContexts(oldContext, newContext);
    }

    InnerMainWindow m_mainWindow;
    QStackedWidget m_centralWidgetStack;
    QDockWidget m_toolBarDock;
    TweakedCombo m_perspectiveChooser;
    StatusLabel m_statusLabel;
    EditorManagerPlaceHolder m_editorPlaceHolder;
    QMenu m_perspectiveMenu;

    PerspectivesView *q = nullptr;
    Key m_settingsGroup;

    QPointer<Perspective> m_currentPerspective = nullptr;
    QHBoxLayout *m_subPerspectiveSwitcherLayout = nullptr;
    QHBoxLayout *m_innerToolsLayout = nullptr;
    bool needRestoreOnModeEnter = false;

    QList<QPointer<Perspective>> m_perspectives;
    QSet<QString> m_persistentChangedDocks; // Dock Ids of docks with non-default visibility.

    QHash<QString, PerspectiveState> m_lastPerspectiveStates;      // Perspective::id() -> MainWindow::state()
    QHash<QString, PerspectiveState> m_lastTypePerspectiveStates;  // Perspective::settingsId() -> MainWindow::state()
};

PerspectivesViewPrivate::PerspectivesViewPrivate(PerspectivesView *parent,
                                                 const QString &settingsGroup,
                                                 const QString &statusObjectName)
    : q(parent)
    , m_settingsGroup(keyFromString(settingsGroup))
{
    m_statusLabel.setObjectName(statusObjectName); // "DebuggerStatusLabel" is used by Squish
    StyleHelper::setPanelWidget(&m_statusLabel);
    m_statusLabel.setIndent(2 * QFontMetrics(m_mainWindow.font()).horizontalAdvance(QChar('x')));

    m_perspectiveChooser.setObjectName("PerspectiveChooser");
    StyleHelper::setPanelWidget(&m_perspectiveChooser);
    m_perspectiveChooser.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(&m_perspectiveChooser, &QComboBox::activated, this, [this](int item) {
        Perspective *perspective = Perspective::findPerspective(m_perspectiveChooser.itemData(item).toString());
        QTC_ASSERT(perspective, return);
        if (auto subPerspective = Perspective::findPerspective(perspective->d->m_lastActiveSubPerspectiveId))
            subPerspective->select();
        else
            perspective->select();
    });

    connect(&m_perspectiveMenu, &QMenu::aboutToShow, this, [this] {
        m_perspectiveMenu.clear();
        q->addPerspectiveMenu(&m_perspectiveMenu);
    });

    auto viewButton = new QToolButton;
    viewButton->setText(Tr::tr("&Views"));

    auto closeButton = new QToolButton();
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    closeButton->setToolTip(Tr::tr("Leave Debug Mode"));

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty(StyleHelper::C_TOP_BORDER, true);

    // "Engine switcher" style comboboxes
    auto subPerspectiveSwitcher = new QWidget;
    m_subPerspectiveSwitcherLayout = new QHBoxLayout(subPerspectiveSwitcher);
    m_subPerspectiveSwitcherLayout->setContentsMargins(0, 0, 0, 0);
    m_subPerspectiveSwitcherLayout->setSpacing(0);

    // All perspective toolbars will get inserted here, but only
    // the current perspective's toolbar is set visible.
    auto innerTools = new QWidget;
    m_innerToolsLayout = new QHBoxLayout(innerTools);
    m_innerToolsLayout->setContentsMargins(0, 0, 0, 0);
    m_innerToolsLayout->setSpacing(0);

    auto hbox = new QHBoxLayout(toolbar);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);
    hbox->addWidget(&m_perspectiveChooser);
    hbox->addWidget(subPerspectiveSwitcher);
    hbox->addWidget(innerTools);
    hbox->addWidget(&m_statusLabel);
    hbox->addStretch(1);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(viewButton);
    hbox->addWidget(closeButton);

    auto scrolledToolbar = new QScrollArea;
    scrolledToolbar->setWidget(toolbar);
    scrolledToolbar->setFrameStyle(QFrame::NoFrame);
    scrolledToolbar->setWidgetResizable(true);
    scrolledToolbar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrolledToolbar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    StyleHelper::setPanelWidgetSingleRow(scrolledToolbar);

    m_toolBarDock.setWindowTitle(Tr::tr("Toolbar"));
    m_toolBarDock.setObjectName("Toolbar");
    m_toolBarDock.setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_toolBarDock.setAllowedAreas(Qt::BottomDockWidgetArea);
    m_toolBarDock.setTitleBarWidget(new QWidget(&m_toolBarDock)); // hide title bar
    m_toolBarDock.setProperty("managed_dockwidget", "true");
    m_toolBarDock.setWidget(scrolledToolbar);

    m_mainWindow.setDockNestingEnabled(true);
    m_mainWindow.setDockActionsVisible(false);
    m_mainWindow.setDocumentMode(true);
    m_mainWindow.addDockWidget(Qt::BottomDockWidgetArea, &m_toolBarDock);

    connect(viewButton, &QAbstractButton::clicked, this, [viewButton] {
        ActionContainer *viewsMenu = ActionManager::actionContainer(Core::Constants::M_VIEW_VIEWS);
        viewsMenu->menu()->exec(viewButton->mapToGlobal(QPoint()));
    });

    connect(closeButton, &QAbstractButton::clicked, [] {
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
    });

    connect(&m_mainWindow, &FancyMainWindow::resetLayout,
            this, &PerspectivesViewPrivate::resetCurrentPerspective);
}

PerspectivesView::PerspectivesView(const QString &settingsGroup,
                                   const Utils::Id &modeContext,
                                   const QString &statusObjectName)
    : d(new PerspectivesViewPrivate(this, settingsGroup, statusObjectName))
{
    // Action ids must be unique per view, so suffix them with the mode context.
    Context modecontext(modeContext);
    const QString suffix = '.' + modeContext.toString();
    ActionContainer *viewsMenu = ActionManager::actionContainer(Core::Constants::M_VIEW_VIEWS);
    Command *cmd = ActionManager::registerAction(d->m_mainWindow.showCentralWidgetAction(),
        Id("Core.Views.ShowCentralWidget").withSuffix(suffix), modecontext);
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(d->m_mainWindow.menuSeparator1(),
        Id("Core.Views.Separator1").withSuffix(suffix), modecontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(d->m_mainWindow.resetLayoutAction(),
        Id("Core.Views.ResetSimple").withSuffix(suffix), modecontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    // HACK: See QTCREATORBUG-23755. This ensures the showCentralWidget()
    // call in restorePersistentSettings() below has something to operate on,
    // and a plain QWidget is what we'll use anyway as central widget.
    d->m_mainWindow.setCentralWidget(new QWidget);

    theViews.append(this);

    restorePersistentSettings();
}

PerspectivesView::~PerspectivesView()
{
    savePersistentSettings();

    // Delete inner toolbars before the main window so that tracked toolbar
    // widgets (which may be value members in plugin-owned objects) are
    // unparented before Qt's parent-child deletion runs.
    for (const QPointer<Perspective> &p : d->m_perspectives) {
        if (!p)
            continue;
        for (const QPointer<QWidget> &w : p->d->m_toolBarWidgets) {
            if (w)
                w->setParent(nullptr);
        }
        delete p->d->m_innerToolBar;
        p->d->m_innerToolBar = nullptr;
    }

    theViews.removeAll(this);
    delete d;
}

FancyMainWindow *PerspectivesView::mainWindow()
{
    return &d->m_mainWindow;
}

void PerspectivesView::ensureMainWindowExists()
{
    if (!theDefaultView) {
        theDefaultView = new PerspectivesView(QString::fromUtf8(DEFAULT_SETTINGS_GROUP),
                                              Core::Constants::C_DEBUG_MODE,
                                              "DebuggerStatusLabel");
    }
}

PerspectivesView *PerspectivesView::createView(const QString &settingsGroup,
                                               const Utils::Id &modeContext,
                                               const QString &statusObjectName)
{
    return new PerspectivesView(settingsGroup, modeContext, statusObjectName);
}

void PerspectivesView::destroyView(PerspectivesView *view)
{
    delete view;
}

void PerspectivesView::doShutdown()
{
    // Additional views (e.g. the Profiler mode's) are torn down by their
    // owning plugins, in "view before mode" order. Here we only handle the
    // default (Debug) view.
    delete theDefaultView;
    theDefaultView = nullptr;
}

void PerspectivesViewPrivate::registerPerspective(Perspective *perspective)
{
    QString parentPerspective = perspective->d->m_parentPerspectiveId;
    // Add only "main" perspectives to the chooser.
    if (parentPerspective.isEmpty())
        m_perspectiveChooser.addItem(perspective->d->m_name, perspective->d->m_id);
    m_perspectives.append(perspective);
}

void PerspectivesViewPrivate::destroyPerspective(Perspective *perspective)
{
    qCDebug(perspectivesLog) << "ABOUT TO DESTROY PERSPECTIVE" << perspective->id();

    const bool wasCurrent = perspective == m_currentPerspective;
    if (wasCurrent) {
        qCDebug(perspectivesLog) << "RAMPING IT DOWN FIRST AS IT WAS CURRENT" << perspective->id();
        perspective->rampDownAsCurrent();
    }

    m_perspectives.removeAll(perspective);

    // Dynamic perspectives are currently not visible in the chooser.
    // This might change in the future, make sure we notice.
    const int idx = indexInChooser(perspective);
    if (idx != -1)
        m_perspectiveChooser.removeItem(idx);

    for (DockOperation &op : perspective->d->m_dockOperations) {
        if (op.commandId.isValid())
            ActionManager::unregisterAction(op.toggleViewAction, op.commandId);
        if (op.dock) {
            m_mainWindow.removeDockWidget(op.dock);
            if (op.widget)
                op.widget->setParent(nullptr); // Prevent deletion
            op.dock->setParent(nullptr);
            delete op.dock;
            op.dock = nullptr;
        }
    }

    if (wasCurrent) {
        if (Perspective *parent = Perspective::findPerspective(perspective->d->m_parentPerspectiveId)) {
            qCDebug(perspectivesLog) << "RAMPING UP PARENT PERSPECTIVE" << parent->id();
            parent->rampUpAsCurrent();
        } else {
            qCDebug(perspectivesLog) << "RAMPED DOWN WAS ACTION, BUT NO PARENT AVAILABLE. TAKE FIRST BEST";
            if (QTC_GUARD(!m_perspectives.isEmpty()))
                m_perspectives.first()->rampUpAsCurrent();
        }
    }

    qCDebug(perspectivesLog) << "DESTROYED PERSPECTIVE" << perspective->id();
}

void PerspectivesView::showStatusMessage(const QString &message, int timeoutMS)
{
    d->m_statusLabel.showStatusMessage(message, timeoutMS);
}

void PerspectivesView::enableMainWindow(bool on)
{
    d->m_mainWindow.setEnabled(on);
}

void PerspectivesView::showPermanentStatusMessage(const QString &message)
{
    showStatusMessage(message, -1);
}

void PerspectivesView::enter()
{
    d->m_mainWindow.setDockActionsVisible(true);
    QTC_CHECK(d->m_currentPerspective == nullptr);
    if (d->needRestoreOnModeEnter)
        restorePersistentSettings();

    QtcSettings *settings = ICore::settings();
    settings->beginGroup(d->m_settingsGroup);
    const QString lastPerspectiveId = settings->value(LAST_PERSPECTIVE_KEY).toString();
    settings->endGroup();
    Perspective *perspective = Perspective::findPerspective(lastPerspectiveId);
    // Only accept a perspective that actually belongs to this view.
    if (perspective && perspective->d->m_view != this)
        perspective = nullptr;
    // If we don't find a perspective with the stored name, pick any.
    // This can happen e.g. when a plugin was disabled that provided
    // the stored perspective, or when the save file was modified externally.
    if (!perspective && !d->m_perspectives.isEmpty())
        perspective = d->m_perspectives.first();

    // There's at least the debugger preset perspective that should be found above.
    QTC_ASSERT(perspective, return);

    if (auto sub = Perspective::findPerspective(perspective->d->m_lastActiveSubPerspectiveId)) {
        qCDebug(perspectivesLog) << "SWITCHING TO SUBPERSPECTIVE" << sub->d->m_id;
        perspective = sub;
    }

    perspective->rampUpAsCurrent();
}

void PerspectivesView::leave()
{
    d->needRestoreOnModeEnter = true;
    savePersistentSettings();

    if (d->m_currentPerspective)
        d->m_currentPerspective->rampDownAsCurrent();
    QTC_CHECK(d->m_currentPerspective == nullptr);

    d->m_mainWindow.setDockActionsVisible(false);

    // Hide dock widgets manually in case they are floating.
    for (QDockWidget *dockWidget : d->m_mainWindow.dockWidgets()) {
        if (dockWidget->isFloating())
            dockWidget->setVisible(false);
    }
}

void PerspectivesView::restorePersistentSettings()
{
    qCDebug(perspectivesLog) << "RESTORE ALL PERSPECTIVES";
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(d->m_settingsGroup);

    const QHash<QString, QVariant> states2 = settings->value(STATE_KEY2).toHash();
    d->m_lastTypePerspectiveStates.clear();
    for (auto it = states2.begin(); it != states2.end(); ++it) {
        const PerspectiveState state = PerspectiveState::fromSettings(storeFromMap(it->toMap()));
        QTC_ASSERT(state.hasWindowState(), continue);
        d->m_lastTypePerspectiveStates.insert(it.key(), state);
    }

    d->m_mainWindow.showCentralWidget(settings->value(SHOW_CENTRALWIDGET_KEY, true).toBool());
    d->m_persistentChangedDocks = Utils::toSet(settings->value(CHANGED_DOCK_KEY).toStringList());
    settings->endGroup();

    qCDebug(perspectivesLog) << "LOADED CHANGED DOCKS:" << d->m_persistentChangedDocks;
}

Perspective *PerspectivesView::currentPerspective()
{
    return d->m_currentPerspective;
}

void PerspectivesView::savePersistentSettings() const
{
    // The current one might have active, non saved changes.
    if (Perspective *perspective = d->m_currentPerspective)
        perspective->d->saveLayout();

    QVariantHash states;
    qCDebug(perspectivesLog) << "PERSPECTIVE TYPES: " << d->m_lastTypePerspectiveStates.keys();
    for (auto it = d->m_lastTypePerspectiveStates.cbegin();
         it != d->m_lastTypePerspectiveStates.cend(); ++it) {
        const QString &type = it.key();
        const PerspectiveState &state = it.value();
        qCDebug(perspectivesLog) << "PERSPECTIVE TYPE " << type
                                 << " HAS STATE: " << !state.mainWindowState.isEmpty();
        QTC_ASSERT(!state.mainWindowState.isEmpty(), continue);
        states.insert(type, mapFromStore(state.toSettings()));
    }

    QtcSettings *settings = ICore::settings();
    settings->beginGroup(d->m_settingsGroup);
    settings->setValue(CHANGED_DOCK_KEY, QStringList(Utils::toList(d->m_persistentChangedDocks)));
    settings->setValue(STATE_KEY2, states);
    settings->setValue(SHOW_CENTRALWIDGET_KEY, d->m_mainWindow.isCentralWidgetShown());
    settings->endGroup();

    qCDebug(perspectivesLog) << "SAVED CHANGED DOCKS:" << d->m_persistentChangedDocks;
}

QWidget *PerspectivesView::centralWidgetStack()
{
    return &d->m_centralWidgetStack;
}

void PerspectivesView::addSubPerspectiveSwitcher(QWidget *widget)
{
    widget->setVisible(false);
    StyleHelper::setPanelWidget(widget);
    d->m_subPerspectiveSwitcherLayout->addWidget(widget);
}

void PerspectivesView::addPerspectiveMenu(QMenu *menu)
{
    for (Perspective *perspective : std::as_const(d->m_perspectives)) {
        menu->addAction(perspective->d->m_name, perspective, [perspective] {
            if (auto subPerspective = Perspective::findPerspective(
                    perspective->d->m_lastActiveSubPerspectiveId))
                subPerspective->select();
            else
                perspective->select();
        });
    }
}

PerspectivesView *PerspectivesView::instance()
{
    return theDefaultView;
}

Perspective *Perspective::findPerspective(const QString &perspectiveId)
{
    for (PerspectivesView *view : std::as_const(theViews)) {
        if (Perspective *p = Utils::findOr(view->d->m_perspectives, nullptr,
                [perspectiveId](Perspective *perspective) {
                    return perspective && perspective->d->m_id == perspectiveId;
                })) {
            return p;
        }
    }
    return nullptr;
}

bool Perspective::isCurrent() const
{
    return d->m_view->d->m_currentPerspective == this;
}

QDockWidget *PerspectivesViewPrivate::dockForWidget(QWidget *widget) const
{
    QTC_ASSERT(widget, return nullptr);

    for (QDockWidget *dock : m_mainWindow.dockWidgets()) {
        if (dock->widget() == widget)
            return dock;
    }

    return nullptr;
}

void PerspectivesViewPrivate::resetCurrentPerspective()
{
    QTC_ASSERT(m_currentPerspective, return);
    cleanDocks();
    setCentralWidget(&m_editorPlaceHolder);
    m_mainWindow.showCentralWidget(true);
    m_currentPerspective->d->resetPerspective();
}

void PerspectivesViewPrivate::setCentralWidget(QWidget *widget)
{
    m_centralWidgetStack.addWidget(widget);
    m_mainWindow.showCentralWidgetAction()->setText(widget->windowTitle());
}

void PerspectivePrivate::resetPerspective()
{
    showInnerToolBar();

    for (DockOperation &op : m_dockOperations) {
        if (!op.dock) {
            qCDebug(perspectivesLog) << "RESET UNUSED " << op.name();
        } else if (op.operationType == Perspective::Raise) {
            QTC_ASSERT(op.dock, qCDebug(perspectivesLog) << op.name(); continue);
            op.dock->raise();
        } else {
            op.setupLayout();
            op.dock->setVisible(op.visibleByDefault);
            m_view->d->m_persistentChangedDocks.remove(op.name());
            qCDebug(perspectivesLog) << "SETTING " << op.name()
                                     << " TO ACTIVE: " << op.visibleByDefault;
        }
    }
}

void DockOperation::setupLayout()
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(operationType != Perspective::Raise, return);
    QTC_ASSERT(dock, return);

    QDockWidget *anchor = nullptr;
    if (anchorWidget)
        anchor = view->d->dockForWidget(anchorWidget);
    else if (area == Qt::BottomDockWidgetArea)
        anchor = &view->d->m_toolBarDock;

    if (anchor) {
        switch (operationType) {
        case Perspective::AddToTab:
            view->d->m_mainWindow.tabifyDockWidget(anchor, dock);
            break;
        case Perspective::SplitHorizontal:
            view->d->m_mainWindow.splitDockWidget(anchor, dock, Qt::Horizontal);
            break;
        case Perspective::SplitVertical:
            view->d->m_mainWindow.splitDockWidget(anchor, dock, Qt::Vertical);
            break;
        default:
            break;
        }
    } else {
        view->d->m_mainWindow.addDockWidget(area, dock);
    }
}

void DockOperation::ensureDockExists()
{
    if (dock)
        return;

    QTC_ASSERT(widget, return);

    dock = view->d->m_mainWindow.addDockForWidget(widget);

    if (view->d->m_mainWindow.restoreDockWidget(dock)) {
        qCDebug(perspectivesLog) << "RESTORED SUCCESSFULLY" << commandId;
    } else {
        qCDebug(perspectivesLog) << "COULD NOT RESTORE" << commandId;
        setupLayout();
    }

    toggleViewAction->setAction(dock->toggleViewAction());

    QObject::connect(dock->toggleViewAction(), &QAction::triggered,
                     dock->toggleViewAction(), [this] { recordVisibility(); });
}

bool DockOperation::changedByUser() const
{
    return view->d->m_persistentChangedDocks.contains(name());
}

void DockOperation::recordVisibility()
{
    if (operationType != Perspective::Raise) {
        if (toggleViewAction->isChecked() == visibleByDefault)
            view->d->m_persistentChangedDocks.remove(name());
        else
            view->d->m_persistentChangedDocks.insert(name());
    }
    qCDebug(perspectivesLog) << "RECORDING DOCK VISIBILITY " << name()
                             << toggleViewAction->isChecked()
                             << view->d->m_persistentChangedDocks;
}

int PerspectivesViewPrivate::indexInChooser(Perspective *perspective) const
{
    return perspective ? m_perspectiveChooser.findData(perspective->d->m_id) : -1;
}

void PerspectivesViewPrivate::updatePerspectiveChooserWidth()
{
    Perspective *perspective = m_currentPerspective;
    int index = indexInChooser(m_currentPerspective);
    if (index == -1) {
        perspective = Perspective::findPerspective(m_currentPerspective->d->m_parentPerspectiveId);
        if (perspective)
            index = indexInChooser(perspective);
    }

    if (index != -1) {
        m_perspectiveChooser.setCurrentIndex(index);

        const int contentWidth =
            m_perspectiveChooser.fontMetrics().horizontalAdvance(perspective->d->m_name);
        QStyleOptionComboBox option;
        option.initFrom(&m_perspectiveChooser);
        const QSize sz(contentWidth, 1);
        const int width = m_perspectiveChooser.style()->sizeFromContents(
                    QStyle::CT_ComboBox, &option, sz).width();
        m_perspectiveChooser.setFixedWidth(width);
    }
}

void PerspectivesViewPrivate::cleanDocks()
{
    m_statusLabel.clear();

    for (QDockWidget *dock : m_mainWindow.dockWidgets()) {
        dock->setFloating(false);
        if (dock != &m_toolBarDock)
            dock->setVisible(false);
    }
}

void PerspectivePrivate::depopulatePerspective()
{
    ICore::removeAdditionalContext(context());
    m_view->d->m_centralWidgetStack.removeWidget(&m_view->d->m_editorPlaceHolder);

    m_view->d->m_statusLabel.clear();

    qCDebug(perspectivesLog) << "DEPOPULATE PERSPECTIVE" << m_id;
    for (QDockWidget *dock : m_view->d->m_mainWindow.dockWidgets()) {
        if (dock != &m_view->d->m_toolBarDock)
            dock->setVisible(false);
    }

    hideInnerToolBar();
}

void PerspectivePrivate::saveAsLastUsedPerspective()
{
    if (Perspective *parent = Perspective::findPerspective(m_parentPerspectiveId))
        parent->d->m_lastActiveSubPerspectiveId = m_id;
    else
        m_lastActiveSubPerspectiveId.clear();

    const QString &lastKey = m_parentPerspectiveId.isEmpty() ? m_id : m_parentPerspectiveId;
    qCDebug(perspectivesLog) << "SAVE AS LAST USED PERSPECTIVE" << lastKey;
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(m_view->d->m_settingsGroup);
    settings->setValue(LAST_PERSPECTIVE_KEY, lastKey);
    settings->endGroup();
}

void PerspectivePrivate::populatePerspective()
{
    showInnerToolBar();

    m_view->d->m_centralWidgetStack.addWidget(&m_view->d->m_editorPlaceHolder);
    m_view->d->m_mainWindow.showCentralWidgetAction()->setText(Tr::tr("Editor"));

    ICore::addAdditionalContext(context());

    restoreLayout();

    if (IEditor *editor = EditorManager::currentEditor())
        editor->widget()->setFocus();
}

// Perspective

Perspective::Perspective(const QString &id, const QString &name,
                         const QString &parentPerspectiveId,
                         const QString &settingsId,
                         PerspectivesView *view)
    : d(new PerspectivePrivate)
{
    d->m_id = id;
    d->m_name = name;
    d->m_parentPerspectiveId = parentPerspectiveId;
    d->m_settingsId = settingsId;

    if (!view) {
        PerspectivesView::ensureMainWindowExists();
        view = theDefaultView;
    }
    d->m_view = view;
    view->d->registerPerspective(this);

    d->m_innerToolBar = new QWidget;
    d->m_innerToolBar->setVisible(false);
    view->d->m_innerToolsLayout->addWidget(d->m_innerToolBar);

    d->m_innerToolBarLayout = new QHBoxLayout(d->m_innerToolBar);
    d->m_innerToolBarLayout->setContentsMargins(0, 0, 0, 0);
    d->m_innerToolBarLayout->setSpacing(0);
}

Perspective::~Perspective()
{
    // The owning view may already have been torn down in doShutdown().
    if (theViews.contains(d->m_view)) {
        for (const QPointer<QWidget> &w : d->m_toolBarWidgets)
            if (w) w->setParent(nullptr);
        delete d->m_innerToolBar;
        d->m_innerToolBar = nullptr;
    }
    delete d;
}

QString Perspective::id() const
{
    return d->m_id;
}

const char *Perspective::savesHeaderKey()
{
    return SAVES_HEADER_KEY;
}

QString Perspective::parentPerspectiveId() const
{
    return d->m_parentPerspectiveId;
}

QString Perspective::name() const
{
    return d->m_name;
}

void Perspective::setAboutToActivateCallback(const Perspective::Callback &cb)
{
    d->m_aboutToActivateCallback = cb;
}

void Perspective::setEnabled(bool enabled)
{
    const int index = d->m_view->d->indexInChooser(this);
    QTC_ASSERT(index != -1, return);
    auto model = qobject_cast<QStandardItemModel*>(d->m_view->d->m_perspectiveChooser.model());
    QTC_ASSERT(model, return);
    QStandardItem *item = model->item(index, 0);
    item->setFlags(enabled ? item->flags() | Qt::ItemIsEnabled : item->flags() & ~Qt::ItemIsEnabled );
}

void PerspectivePrivate::setupToolButton(QAction *action, Qt::ToolButtonStyle style)
{
    QTC_ASSERT(action, return);
    auto toolButton = new QToolButton(m_innerToolBar);
    StyleHelper::setPanelWidget(toolButton);
    toolButton->setDefaultAction(action);
    toolButton->setToolTip(action->toolTip());
    toolButton->setToolButtonStyle(style);
    toolButton->setVisible(action->isVisible());
    QObject::connect(action, &QAction::changed, toolButton, [action, toolButton] {
        toolButton->setVisible(action->isVisible());
    });
    m_innerToolBarLayout->addWidget(toolButton);
}

void Perspective::addToolBarAction(QAction *action, Qt::ToolButtonStyle style)
{
    QTC_ASSERT(action, return);
    d->setupToolButton(action, style);
}

void Perspective::addToolBarWidget(QWidget *widget)
{
    QTC_ASSERT(widget, return);
    StyleHelper::setPanelWidget(widget);
    d->m_toolBarWidgets.append(widget);
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

void Perspective::registerNextPrevShortcuts(QAction *next, QAction *prev)
{
    static const char nextId[] = "Analyzer.nextitem";
    static const char prevId[] = "Analyzer.previtem";

    next->setText(Tr::tr("Next Item"));
    Command * const nextCmd = ActionManager::registerAction(next, nextId,
                                                            Context(Id::fromString(id())));
    nextCmd->augmentActionWithShortcutToolTip(next);
    prev->setText(Tr::tr("Previous Item"));
    Command * const prevCmd = ActionManager::registerAction(prev, prevId,
                                                            Context(Id::fromString(id())));
    prevCmd->augmentActionWithShortcutToolTip(prev);
}

Context PerspectivePrivate::context() const
{
    return Context(Id::fromName(m_id.toUtf8()));
}

void PerspectivePrivate::showInnerToolBar()
{
    m_innerToolBar->setVisible(true);
    if (m_switcher)
        m_switcher->setVisible(true);
}

void PerspectivePrivate::hideInnerToolBar()
{
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
    op.view = d->m_view;
    op.widget = widget;
    op.operationType = type;
    op.anchorWidget = anchorWidget;
    op.visibleByDefault = visibleByDefault;
    op.area = area;

    if (op.operationType != Perspective::Raise) {
        qCDebug(perspectivesLog) << "CREATING DOCK " << op.name()
                                 << "DEFAULT: " << op.visibleByDefault;
        op.commandId = Id("Dock.").withSuffix(op.name());

        op.toggleViewAction = new ProxyAction(this);
        op.toggleViewAction->setText(widget->windowTitle());

        Command *cmd = ActionManager::registerAction(op.toggleViewAction, op.commandId, d->context());
        cmd->setAttribute(Command::CA_Hide);
        ActionManager::actionContainer(Core::Constants::M_VIEW_VIEWS)->addAction(cmd);
    }

    d->m_dockOperations.append(op);
}

void Perspective::destroy()
{
    d->m_view->d->destroyPerspective(this);
}

void Perspective::rampDownAsCurrent()
{
    PerspectivesView *view = d->m_view;
    QTC_ASSERT(this == view->d->m_currentPerspective, return);
    d->saveLayout();
    d->depopulatePerspective();
    view->d->setCurrentPerspective(nullptr);

    emit view->perspectivesChanged();
}

void Perspective::rampUpAsCurrent()
{
    if (d->m_aboutToActivateCallback)
        d->m_aboutToActivateCallback();

    PerspectivesView *view = d->m_view;
    QTC_ASSERT(view->d->m_currentPerspective == nullptr, return);
    view->d->setCurrentPerspective(this);
    QTC_ASSERT(view->d->m_currentPerspective == this, return);

    view->d->m_mainWindow.showCentralWidget(true);

    d->populatePerspective();

    view->d->updatePerspectiveChooserWidth();

    d->saveAsLastUsedPerspective();

    emit view->perspectivesChanged();
}

void Perspective::select()
{
    PerspectivesView *view = d->m_view;
    emit view->modeActivationRequested();

    if (view->d->m_currentPerspective == this)
        return;

    if (view->d->m_currentPerspective)
        view->d->m_currentPerspective->rampDownAsCurrent();
    QTC_CHECK(view->d->m_currentPerspective == nullptr);

    rampUpAsCurrent();
}

void PerspectivePrivate::restoreLayout()
{
    qCDebug(perspectivesLog) << "RESTORE LAYOUT FOR " << m_id << settingsId();
    PerspectiveState state = m_view->d->m_lastPerspectiveStates.value(m_id);
    if (!state.hasWindowState()) {
        qCDebug(perspectivesLog) << "PERSPECTIVE STATE NOT AVAILABLE BY FULL ID.";
        state = m_view->d->m_lastTypePerspectiveStates.value(settingsId());
        if (state.hasWindowState()) {
            qCDebug(perspectivesLog) << "PERSPECTIVE STATE AVAILABLE BY PERSPECTIVE TYPE.";
        } else {
            qCDebug(perspectivesLog) << "PERSPECTIVE STATE NOT AVAILABLE BY PERSPECTIVE TYPE";
        }
    } else {
        qCDebug(perspectivesLog) << "PERSPECTIVE STATE AVAILABLE BY FULL ID.";
    }

    // The order is important here: While QMainWindow can restore layouts with
    // not-existing docks (some placeholders are used internally), later
    // replacements with restoreDockWidget(dock) trigger a re-layout, resulting
    // in different sizes. So make sure all docks exist first before restoring state.

    qCDebug(perspectivesLog) << "PERSPECTIVE" << m_id << "RESTORING LAYOUT FROM " << settingsId();
    for (DockOperation &op : m_dockOperations) {
        if (op.operationType != Perspective::Raise) {
            op.ensureDockExists();
            QTC_ASSERT(op.dock, continue);
            const bool active = op.visibleByDefault != op.changedByUser();
            op.dock->setVisible(active);
            qCDebug(perspectivesLog) << "RESTORE DOCK " << op.name() << "ACTIVE: " << active
                                     << (active == op.visibleByDefault ? "DEFAULT USER" : "*** NON-DEFAULT USER");
        }
    }

    if (!state.hasWindowState()) {
        qCDebug(perspectivesLog) << "PERSPECTIVE " << m_id << "RESTORE NOT POSSIBLE, NO STORED STATE";
    } else {
        const bool result = state.restoreWindowState(&m_view->d->m_mainWindow);
        qCDebug(perspectivesLog) << "PERSPECTIVE " << m_id << "RESTORED, SUCCESS: " << result;
    }

    for (DockOperation &op : m_dockOperations) {
        if (op.operationType != Perspective::Raise) {
            QTC_ASSERT(op.dock, continue);
            for (QTreeView *tv : op.dock->findChildren<QTreeView *>()) {
                if (tv->property(SAVES_HEADER_KEY).toBool()) {
                    const QByteArray s = state.headerViewStates.value(op.name()).toByteArray();
                    if (!s.isEmpty())
                        tv->header()->restoreState(s);
                }
            }
        }
    }
}

void PerspectivePrivate::saveLayout()
{
    qCDebug(perspectivesLog) << "PERSPECTIVE" << m_id << "SAVE LAYOUT TO " << settingsId();
    PerspectiveState state;
    state.mainWindowState = m_view->d->m_mainWindow.saveSettings();
    for (DockOperation &op : m_dockOperations) {
        if (op.operationType != Perspective::Raise) {
            QTC_ASSERT(op.dock, continue);
            for (QTreeView *tv : op.dock->findChildren<QTreeView *>()) {
                if (tv->property(SAVES_HEADER_KEY).toBool()) {
                    if (QHeaderView *hv = tv->header())
                        state.headerViewStates.insert(op.name(), hv->saveState());
                }
            }
        }
    }
    m_view->d->m_lastPerspectiveStates.insert(m_id, state);
    m_view->d->m_lastTypePerspectiveStates.insert(settingsId(), state);
}

QString PerspectivePrivate::settingsId() const
{
    return m_settingsId.isEmpty() ? m_id : m_settingsId;
}

} // namespace Core
