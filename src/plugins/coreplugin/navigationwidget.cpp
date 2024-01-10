// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigationwidget.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreplugintr.h"
#include "icontext.h"
#include "icore.h"
#include "imode.h"
#include "inavigationwidgetfactory.h"
#include "modemanager.h"
#include "navigationsubwidget.h"

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <aggregation/aggregate.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStandardItemModel>

Q_DECLARE_METATYPE(Core::INavigationWidgetFactory *)

using namespace Utils;

namespace Core {

struct ActivationInfo {
    Side side;
    int position;
};
using ActivationsMap = QHash<Id, ActivationInfo>;

static NavigationWidget *s_instanceLeft = nullptr;
static NavigationWidget *s_instanceRight = nullptr;
static ActivationsMap s_activationsMap = {};

static void addActivationInfo(Id activatedId, const ActivationInfo &activationInfo)
{
    s_activationsMap.insert(activatedId, activationInfo);
}

NavigationWidget *instance(Side side)
{
    return side == Side::Left ? s_instanceLeft : s_instanceRight;
}

NavigationWidgetPlaceHolder *NavigationWidgetPlaceHolder::s_currentLeft = nullptr;
NavigationWidgetPlaceHolder *NavigationWidgetPlaceHolder::s_currentRight = nullptr;

NavigationWidgetPlaceHolder *NavigationWidgetPlaceHolder::current(Side side)
{
    return side == Side::Left ? s_currentLeft : s_currentRight;
}

void NavigationWidgetPlaceHolder::setCurrent(Side side, NavigationWidgetPlaceHolder *navWidget)
{
    if (side == Side::Left)
        s_currentLeft = navWidget;
    else
        s_currentRight = navWidget;
}

NavigationWidgetPlaceHolder::NavigationWidgetPlaceHolder(Id mode, Side side, QWidget *parent)
    :QWidget(parent), m_mode(mode), m_side(side)
{
    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    connect(ModeManager::instance(), &ModeManager::currentModeAboutToChange,
            this, &NavigationWidgetPlaceHolder::currentModeAboutToChange);
}

NavigationWidgetPlaceHolder::~NavigationWidgetPlaceHolder()
{
    if (NavigationWidgetPlaceHolder::current(m_side) == this) {
        if (NavigationWidget *nw = instance(m_side)) {
            nw->setParent(nullptr);
            nw->hide();
        }
    }
}

void NavigationWidgetPlaceHolder::applyStoredSize()
{
    auto splitter = qobject_cast<QSplitter *>(parentWidget());
    if (splitter) {
        // A splitter we need to resize the splitter sizes
        QList<int> sizes = splitter->sizes();
        int diff = 0;
        int count = sizes.count();
        for (int i = 0; i < sizes.count(); ++i) {
            if (auto ph = qobject_cast<NavigationWidgetPlaceHolder *>(splitter->widget(i))) {
                --count;
                int width = ph->storedWidth();
                diff += width - sizes.at(i);
                sizes[i] = width;
            }
        }
        int adjust = count > 1 ? (diff / (count - 1)) : 0;
        for (int i = 0; i < sizes.count(); ++i) {
            if (!qobject_cast<NavigationWidgetPlaceHolder *>(splitter->widget(i)))
                sizes[i] += adjust;
        }

        splitter->setSizes(sizes);
    } else {
        QSize s = size();
        s.setWidth(storedWidth());
        resize(s);
    }
}

// This function does work even though the order in which
// the placeHolder get the signal is undefined.
// It does ensure that after all PlaceHolders got the signal
// m_current points to the current PlaceHolder, or zero if there
// is no PlaceHolder in this mode
// And that the parent of the NavigationWidget gets the correct parent
void NavigationWidgetPlaceHolder::currentModeAboutToChange(Id mode)
{
    NavigationWidget *navigationWidget = instance(m_side);
    NavigationWidgetPlaceHolder *current = NavigationWidgetPlaceHolder::current(m_side);

    if (current == this) {
        setCurrent(m_side, nullptr);
        navigationWidget->setParent(nullptr);
        navigationWidget->hide();
        navigationWidget->placeHolderChanged();
    }

    if (m_mode == mode) {
        setCurrent(m_side, this);

        layout()->addWidget(navigationWidget);
        navigationWidget->show();

        applyStoredSize();
        setVisible(navigationWidget->isShown());
        navigationWidget->placeHolderChanged();
    }
}

int NavigationWidgetPlaceHolder::storedWidth() const
{
    return instance(m_side)->storedWidth();
}

struct NavigationWidgetPrivate
{
    explicit NavigationWidgetPrivate(QAction *toggleSideBarAction, Side side);
    ~NavigationWidgetPrivate() { delete m_factoryModel; }

    QList<Internal::NavigationSubWidget *> m_subWidgets;
    QHash<QAction *, Id> m_actionMap;
    QHash<Id, Command *> m_commandMap;
    QStandardItemModel *m_factoryModel;
    FancyMainWindow *m_mainWindow = nullptr;

    bool m_shown;
    int m_width;
    QAction *m_toggleSideBarAction; // does not take ownership
    Side m_side;
};

NavigationWidgetPrivate::NavigationWidgetPrivate(QAction *toggleSideBarAction, Side side) :
    m_factoryModel(new QStandardItemModel),
    m_shown(true),
    m_width(0),
    m_toggleSideBarAction(toggleSideBarAction),
    m_side(side)
{
}

NavigationWidget::NavigationWidget(QAction *toggleSideBarAction, Side side) :
    d(new NavigationWidgetPrivate(toggleSideBarAction, side))
{
    d->m_factoryModel->setSortRole(FactoryPriorityRole);
    setOrientation(Qt::Vertical);

    if (side == Side::Left)
        s_instanceLeft = this;
    else
        s_instanceRight = this;

    connect(ModeManager::instance(),
            &ModeManager::currentMainWindowChanged,
            this,
            &NavigationWidget::updateMode);
}

NavigationWidget::~NavigationWidget()
{
    if (d->m_side == Side::Left)
        s_instanceLeft = nullptr;
    else
        s_instanceRight = nullptr;
    delete d;
}

QWidget *NavigationWidget::activateSubWidget(Id factoryId, Side fallbackSide)
{
    NavigationWidget *navigationWidget = instance(fallbackSide);
    int preferredPosition = -1;

    // TODO: Use find
    if (s_activationsMap.contains(factoryId)) {
        const ActivationInfo info = s_activationsMap.value(factoryId);
        navigationWidget = instance(info.side);
        preferredPosition = info.position;
    }

    return navigationWidget->activateSubWidget(factoryId, preferredPosition);
}

void NavigationWidget::setFactories(const QList<INavigationWidgetFactory *> &factories)
{
    Context navicontext(Constants::C_NAVIGATION_PANE);
    for (INavigationWidgetFactory *factory : std::as_const(factories)) {
        const Id id = factory->id();
        const Id actionId = id.withPrefix("QtCreator.Sidebar.");

        if (!ActionManager::command(actionId)) {
            QAction *action = new QAction(Tr::tr("Activate %1 View").arg(factory->displayName()), this);
            d->m_actionMap.insert(action, id);
            connect(action, &QAction::triggered, this, [this, action] {
                NavigationWidget::activateSubWidget(d->m_actionMap[action], Side::Left);
            });
            Command *cmd = ActionManager::registerAction(action, actionId, navicontext);
            cmd->setDefaultKeySequence(factory->activationSequence());
            d->m_commandMap.insert(id, cmd);
        }

        QStandardItem *newRow = new QStandardItem(factory->displayName());
        newRow->setData(QVariant::fromValue(factory), FactoryObjectRole);
        newRow->setData(QVariant::fromValue(factory->id()), FactoryIdRole);
        newRow->setData(QVariant::fromValue(actionId), FactoryActionIdRole);
        newRow->setData(factory->priority(), FactoryPriorityRole);
        d->m_factoryModel->appendRow(newRow);
    }
    d->m_factoryModel->sort(0);
    updateToggleAction();
}

Key NavigationWidget::settingsGroup() const
{
    return d->m_side == Side::Left ? Key("NavigationLeft") : Key("NavigationRight");
}

int NavigationWidget::storedWidth()
{
    return d->m_width;
}

QAbstractItemModel *NavigationWidget::factoryModel() const
{
    return d->m_factoryModel;
}

void NavigationWidget::updateMode()
{
    IMode *currentMode = ModeManager::currentMode();
    FancyMainWindow *mainWindow = currentMode ? currentMode->mainWindow() : nullptr;
    if (d->m_mainWindow == mainWindow)
        return;
    if (d->m_mainWindow)
        disconnect(d->m_mainWindow, nullptr, this, nullptr);
    d->m_mainWindow = mainWindow;
    if (d->m_mainWindow)
        connect(d->m_mainWindow,
                &FancyMainWindow::dockWidgetsChanged,
                this,
                &NavigationWidget::updateToggleAction);
    updateToggleAction();
}

void NavigationWidget::updateToggleAction()
{
    d->m_toggleSideBarAction->setVisible(toggleActionVisible());
    d->m_toggleSideBarAction->setEnabled(toggleActionEnabled());
    d->m_toggleSideBarAction->setChecked(toggleActionChecked());
    const char *trToolTip = d->m_side == Side::Left ? (d->m_toggleSideBarAction->isChecked()
                                                           ? Constants::TR_HIDE_LEFT_SIDEBAR
                                                           : Constants::TR_SHOW_LEFT_SIDEBAR)
                                                    : (d->m_toggleSideBarAction->isChecked()
                                                           ? Constants::TR_HIDE_RIGHT_SIDEBAR
                                                           : Constants::TR_SHOW_RIGHT_SIDEBAR);

    d->m_toggleSideBarAction->setToolTip(Tr::tr(trToolTip));
}

void NavigationWidget::placeHolderChanged()
{
    updateToggleAction();
}

void NavigationWidget::resizeEvent(QResizeEvent *re)
{
    if (d->m_width && re->size().width())
        d->m_width = re->size().width();
    MiniSplitter::resizeEvent(re);
}

static QIcon closeIconForSide(Side side, int itemCount)
{
    if (itemCount > 1)
        return Utils::Icons::CLOSE_SPLIT_TOP.icon();
    return side == Side::Left
            ? Utils::Icons::CLOSE_SPLIT_LEFT.icon()
            : Utils::Icons::CLOSE_SPLIT_RIGHT.icon();
}

Internal::NavigationSubWidget *NavigationWidget::insertSubItem(int position,
                                                               int factoryIndex,
                                                               bool updateActivationsMap)
{
    for (int pos = position + 1; pos < d->m_subWidgets.size(); ++pos) {
        Internal::NavigationSubWidget *nsw = d->m_subWidgets.at(pos);
        nsw->setPosition(pos + 1);
        addActivationInfo(nsw->factory()->id(), {d->m_side, pos + 1});
    }

    if (!d->m_subWidgets.isEmpty()) // Make all icons the bottom icon
        d->m_subWidgets.at(0)->setCloseIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());

    auto nsw = new Internal::NavigationSubWidget(this, position, factoryIndex);
    connect(nsw, &Internal::NavigationSubWidget::splitMe, this, [this, nsw](int factoryIndex) {
        insertSubItem(indexOf(nsw) + 1, factoryIndex);
    });
    connect(nsw, &Internal::NavigationSubWidget::closeMe, this, [this, nsw] {
        closeSubWidget(nsw);
    });
    connect(nsw, &Internal::NavigationSubWidget::factoryIndexChanged, this, [this, nsw] {
        const Id factoryId = nsw->factory()->id();
        addActivationInfo(factoryId, {d->m_side, nsw->position()});
    });
    insertWidget(position, nsw);

    d->m_subWidgets.insert(position, nsw);
    d->m_subWidgets.at(0)->setCloseIcon(closeIconForSide(d->m_side, d->m_subWidgets.size()));
    if (updateActivationsMap)
        addActivationInfo(nsw->factory()->id(), {d->m_side, position});
    return nsw;
}

QWidget *NavigationWidget::activateSubWidget(Id factoryId, int preferredPosition)
{
    setShown(true);
    for (Internal::NavigationSubWidget *subWidget : std::as_const(d->m_subWidgets)) {
        if (subWidget->factory()->id() == factoryId) {
            subWidget->setFocusWidget();
            ICore::raiseWindow(this);
            return subWidget->widget();
        }
    }

    int index = factoryIndex(factoryId);
    if (index >= 0) {
        bool preferredIndexValid = 0 <= preferredPosition && preferredPosition < d->m_subWidgets.count();
        const int activationIndex = preferredIndexValid ? preferredPosition : 0;
        Internal::NavigationSubWidget *subWidget = d->m_subWidgets.at(activationIndex);
        subWidget->setFactoryIndex(index);
        subWidget->setFocusWidget();
        ICore::raiseWindow(this);
        return subWidget->widget();
    }
    return nullptr;
}

void NavigationWidget::closeSubWidget(Internal::NavigationSubWidget *subWidget)
{
    if (d->m_subWidgets.count() != 1) {
        subWidget->saveSettings();

        int position = d->m_subWidgets.indexOf(subWidget);
        for (int pos = position + 1; pos < d->m_subWidgets.size(); ++pos) {
            Internal::NavigationSubWidget *nsw = d->m_subWidgets.at(pos);
            nsw->setPosition(pos - 1);
            addActivationInfo(nsw->factory()->id(), {d->m_side, pos - 1});
        }

        d->m_subWidgets.removeOne(subWidget);
        subWidget->hide();
        subWidget->deleteLater();
        // update close button of top item
        if (!d->m_subWidgets.isEmpty())
            d->m_subWidgets.at(0)->setCloseIcon(closeIconForSide(d->m_side, d->m_subWidgets.size()));
    } else {
        setShown(false);
    }
}

bool NavigationWidget::toggleActionVisible() const
{
    const bool haveData = d->m_factoryModel->rowCount();
    return haveData || d->m_mainWindow;
}

static Qt::DockWidgetArea dockAreaForSide(Side side)
{
    return side == Side::Left ? Qt::LeftDockWidgetArea : Qt::RightDockWidgetArea;
}

bool NavigationWidget::toggleActionEnabled() const
{
    const bool haveData = d->m_factoryModel->rowCount();
    if (haveData && NavigationWidgetPlaceHolder::current(d->m_side))
        return true;
    if (!d->m_mainWindow)
        return false;
    return d->m_mainWindow->isDockAreaAvailable(dockAreaForSide(d->m_side));
}

bool NavigationWidget::toggleActionChecked() const
{
    const bool haveData = d->m_factoryModel->rowCount();
    if (haveData && NavigationWidgetPlaceHolder::current(d->m_side))
        return d->m_shown;
    if (!d->m_mainWindow)
        return false;
    return d->m_mainWindow->isDockAreaVisible(dockAreaForSide(d->m_side));
}

static QString defaultFirstView(Side side)
{
    return side == Side::Left ? QString("Projects") : QString("Outline");
}

static bool defaultVisible(Side side)
{
    return side == Side::Left;
}

void NavigationWidget::saveSettings(QtcSettings *settings)
{
    QStringList viewIds;
    for (int i=0; i<d->m_subWidgets.count(); ++i) {
        d->m_subWidgets.at(i)->saveSettings();
        viewIds.append(d->m_subWidgets.at(i)->factory()->id().toString());
    }
    settings->setValueWithDefault(settingsKey("Views"), viewIds, {defaultFirstView(d->m_side)});
    settings->setValueWithDefault(settingsKey("Visible"), isShown(), defaultVisible(d->m_side));
    settings->setValue(settingsKey("VerticalPosition"), saveState());
    settings->setValue(settingsKey("Width"), d->m_width);

    const Key activationKey = "ActivationPosition.";
    for (auto it = s_activationsMap.cbegin(); it != s_activationsMap.cend(); ++it) {
        const auto &info = *it;
        const Utils::Key key = settingsKey(activationKey + it.key().name());
        if (info.side == d->m_side)
            settings->setValue(key, info.position);
        else
            settings->remove(key);
    }
}

void NavigationWidget::restoreSettings(QtcSettings *settings)
{
    if (!d->m_factoryModel->rowCount()) {
        // We have no widgets to show!
        setShown(false);
        return;
    }

    const bool isLeftSide = d->m_side == Side::Left;
    QStringList viewIds = settings
                              ->value(settingsKey("Views"), QStringList(defaultFirstView(d->m_side)))
                              .toStringList();

    bool restoreSplitterState = true;
    int version = settings->value(settingsKey("Version"), 1).toInt();
    if (version == 1) {
        QLatin1String defaultSecondView = isLeftSide ? QLatin1String("Open Documents") : QLatin1String("Bookmarks");
        if (!viewIds.contains(defaultSecondView)) {
            viewIds += defaultSecondView;
            restoreSplitterState = false;
        }
        settings->setValue(settingsKey("Version"), 2);
    }

    int position = 0;
    for (const QString &id : std::as_const(viewIds)) {
        int index = factoryIndex(Id::fromString(id));
        if (index >= 0) {
            // Only add if the id was actually found!
            insertSubItem(position, index, /*updateActivationsMap=*/false);
            ++position;
        } else {
            restoreSplitterState = false;
        }
    }

    if (d->m_subWidgets.isEmpty())
        // Make sure we have at least the projects widget or outline widget
        insertSubItem(0,
                      qMax(0, factoryIndex(Id::fromString(defaultFirstView(d->m_side)))),
                      /*updateActivationsMap=*/false);

    setShown(settings->value(settingsKey("Visible"), defaultVisible(d->m_side)).toBool());

    if (restoreSplitterState && settings->contains(settingsKey("VerticalPosition"))) {
        restoreState(settings->value(settingsKey("VerticalPosition")).toByteArray());
    } else {
        QList<int> sizes;
        sizes += 256;
        for (int i = viewIds.size()-1; i > 0; --i)
            sizes.prepend(512);
        setSizes(sizes);
    }

    d->m_width = settings->value(settingsKey("Width"), 240).toInt();
    if (d->m_width < 40)
        d->m_width = 40;

    // Apply
    if (NavigationWidgetPlaceHolder::current(d->m_side))
        NavigationWidgetPlaceHolder::current(d->m_side)->applyStoredSize();

    // Restore last activation positions
    settings->beginGroup(settingsGroup());
    const QString activationKey = QStringLiteral("ActivationPosition.");
    const auto keys = settings->allKeys();
    for (const QString &key : keys) {
        if (!key.startsWith(activationKey))
            continue;

        int position = settings->value(keyFromString(key)).toInt();
        Id factoryId = Id::fromString(key.mid(activationKey.length()));
        addActivationInfo(factoryId, {d->m_side, position});
    }
    settings->endGroup();
}

void NavigationWidget::closeSubWidgets()
{
    for (Internal::NavigationSubWidget *subWidget : std::as_const(d->m_subWidgets)) {
        subWidget->saveSettings();
        delete subWidget;
    }
    d->m_subWidgets.clear();
}

void NavigationWidget::setShown(bool b)
{
    NavigationWidgetPlaceHolder *current = NavigationWidgetPlaceHolder::current(d->m_side);
    if (!current && d->m_mainWindow) {
        // mode without placeholder but with main window
        d->m_mainWindow->setDockAreaVisible(dockAreaForSide(d->m_side), b);
    } else {
        // mode with navigation widget placeholder or e.g. during startup/settings restore
        if (d->m_shown == b)
            return;
        const bool haveData = d->m_factoryModel->rowCount();
        d->m_shown = b;
        if (current) {
            const bool visible = d->m_shown && haveData;
            current->setVisible(visible);
        }
    }
    updateToggleAction();
}

bool NavigationWidget::isShown() const
{
    return d->m_shown;
}

int NavigationWidget::factoryIndex(Id id)
{
    for (int row = 0; row < d->m_factoryModel->rowCount(); ++row) {
        if (d->m_factoryModel->data(d->m_factoryModel->index(row, 0), FactoryIdRole).value<Id>() == id)
            return row;
    }
    return -1;
}

Key NavigationWidget::settingsKey(const Key &key) const
{
    return settingsGroup() + '/' + key;
}

QHash<Id, Command *> NavigationWidget::commandMap() const
{
    return d->m_commandMap;
}

} // namespace Core
