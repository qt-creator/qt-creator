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

#include <utils/fancymainwindow.h>
#include <utils/utilsicons.h>

#ifdef WITH_TESTS
#include <utils/temporarydirectory.h>

#include <QTest>
#endif

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStandardItemModel>

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

const char modesGroup[] = "Modes";

static void addActivationInfo(Id activatedId, const ActivationInfo &activationInfo)
{
    s_activationsMap.insert(activatedId, activationInfo);
}

static NavigationWidget *instance(Side side)
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

struct NavigationLayout
{
    QStringList viewIds;
    QByteArray splitterState;
    bool visible = true;
    int width = 240;
};

struct NavigationWidgetPrivate
{
    explicit NavigationWidgetPrivate(QAction *toggleSideBarAction, Side side);
    ~NavigationWidgetPrivate() { delete m_factoryModel; }

    QList<Internal::NavigationSubWidget *> m_subWidgets;
    QHash<QAction *, Id> m_actionMap;
    QHash<Id, Command *> m_commandMap;
    QStandardItemModel *m_factoryModel;
    FancyMainWindow *m_mainWindow = nullptr;

    // What a key missing from a layout's group means: a mode's own group is
    // read against the shared layout, the shared one against the built-in
    // defaults. The writer leaves out exactly what this supplies, so both
    // sides have to take it from here.
    NavigationLayout defaultsFor(Id mode) const;

    QHash<Id, NavigationLayout> m_modeLayouts;
    NavigationLayout m_defaultLayout;
    // Invalid until a mode with a place holder has been entered, and for
    // every mode that shares the layout.
    Id m_layoutMode;

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
    connect(ModeManager::instance(),
            &ModeManager::currentModeChanged,
            this,
            &NavigationWidget::switchModeLayout);
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

    if (const auto it = s_activationsMap.constFind(factoryId); it != s_activationsMap.constEnd()) {
        navigationWidget = instance(it->side);
        preferredPosition = it->position;
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
    const QString toolTip = d->m_side == Side::Left
                                ? (d->m_toggleSideBarAction->isChecked() ? msgHideLeftSideBar()
                                                                         : msgShowLeftSideBar())
                                : (d->m_toggleSideBarAction->isChecked() ? msgHideRightSideBar()
                                                                         : msgShowRightSideBar());

    d->m_toggleSideBarAction->setToolTip(toolTip);
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
    if (index >= 0 && !d->m_subWidgets.isEmpty()) {
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

static NavigationLayout defaultLayout(Side side)
{
    NavigationLayout layout;
    layout.viewIds = QStringList(defaultFirstView(side));
    layout.visible = defaultVisible(side);
    return layout;
}

static QStringList shownViewIds(const QList<Internal::NavigationSubWidget *> &subWidgets)
{
    QStringList result;
    result.reserve(subWidgets.size());
    for (Internal::NavigationSubWidget *subWidget : subWidgets)
        result.append(subWidget->factory()->id().toString());
    return result;
}

static NavigationLayout readLayout(QtcSettings *settings, const Key &prefix,
                                   const NavigationLayout &defaults)
{
    NavigationLayout layout;
    layout.viewIds = settings->value(prefix + "Views", defaults.viewIds).toStringList();
    if (layout.viewIds.isEmpty())
        layout.viewIds = defaults.viewIds;
    layout.splitterState
        = settings->value(prefix + "VerticalPosition", defaults.splitterState).toByteArray();
    layout.visible = settings->value(prefix + "Visible", defaults.visible).toBool();
    layout.width = qMax(40, settings->value(prefix + "Width", defaults.width).toInt());
    return layout;
}

// The invalid Id is the layout the modes that did not ask for one share.
static Id layoutKey(Id mode)
{
    return ModeManager::modeKeepsOwnLayout(mode) ? mode : Id();
}

// The counterpart of readLayout(): a key is left out when the value the reader
// would fall back to is the same one.
static void writeLayout(QtcSettings *settings, const Key &prefix, const NavigationLayout &layout,
                        const NavigationLayout &defaults)
{
    settings->setValueWithDefault(prefix + "Views", layout.viewIds, defaults.viewIds);
    settings->setValueWithDefault(prefix + "Visible", layout.visible, defaults.visible);
    settings->setValueWithDefault(prefix + "Width", layout.width, defaults.width);
    settings->setValue(prefix + "VerticalPosition", layout.splitterState);
}

NavigationLayout NavigationWidgetPrivate::defaultsFor(Id mode) const
{
    return mode.isValid() ? m_defaultLayout : defaultLayout(m_side);
}

void NavigationWidget::switchModeLayout(Id mode)
{
    const Id key = layoutKey(mode);
    if (key == d->m_layoutMode || !d->m_factoryModel->rowCount()
        || !NavigationWidgetPlaceHolder::current(d->m_side)) {
        return;
    }
    storeLayout();
    d->m_layoutMode = key;
    applyLayout();
}

void NavigationWidget::storeLayout()
{
    NavigationLayout layout;
    layout.viewIds = shownViewIds(d->m_subWidgets);
    layout.splitterState = saveState();
    layout.visible = d->m_shown;
    layout.width = d->m_width;
    if (d->m_layoutMode.isValid())
        d->m_modeLayouts.insert(d->m_layoutMode, layout);
    else
        d->m_defaultLayout = layout;
}

void NavigationWidget::applyLayout()
{
    const NavigationLayout layout = d->m_modeLayouts.value(d->m_layoutMode, d->m_defaultLayout);

    if (shownViewIds(d->m_subWidgets) != layout.viewIds) {
        closeSubWidgets();
        bool allViewsFound = true;
        for (const QString &id : layout.viewIds) {
            const int index = factoryIndex(Id::fromString(id));
            if (index < 0) {
                allViewsFound = false;
                continue;
            }
            insertSubItem(d->m_subWidgets.size(), index, /*updateActivationsMap=*/false);
        }
        if (d->m_subWidgets.isEmpty()) {
            // Make sure we have at least the projects widget or outline widget
            insertSubItem(0,
                          qMax(0, factoryIndex(Id::fromString(defaultFirstView(d->m_side)))),
                          /*updateActivationsMap=*/false);
            allViewsFound = false;
        }

        if (allViewsFound && !layout.splitterState.isEmpty()) {
            restoreState(layout.splitterState);
        } else {
            QList<int> sizes;
            sizes += 256;
            for (int i = d->m_subWidgets.size() - 1; i > 0; --i)
                sizes.prepend(512);
            setSizes(sizes);
        }
    }

    d->m_width = layout.width;
    setShown(layout.visible);

    if (NavigationWidgetPlaceHolder *placeHolder = NavigationWidgetPlaceHolder::current(d->m_side))
        placeHolder->applyStoredSize();
}

void NavigationWidget::saveSettings(QtcSettings *settings)
{
    for (Internal::NavigationSubWidget *subWidget : std::as_const(d->m_subWidgets))
        subWidget->saveSettings();

    storeLayout();

    writeLayout(settings, layoutSettingsPrefix({}), d->m_defaultLayout, d->defaultsFor({}));

    // The group of a mode that gave its layout up would be read back forever.
    settings->beginGroup(settingsKey(modesGroup));
    const QStringList staleModes = settings->childGroups();
    settings->endGroup();
    for (const QString &modeId : staleModes) {
        if (!d->m_modeLayouts.contains(Id::fromString(modeId)))
            settings->remove(settingsKey(modesGroup) + '/' + keyFromString(modeId));
    }

    for (auto it = d->m_modeLayouts.cbegin(), end = d->m_modeLayouts.cend(); it != end; ++it)
        writeLayout(settings, layoutSettingsPrefix(it.key()), *it, d->defaultsFor(it.key()));

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

    d->m_defaultLayout = readLayout(settings, layoutSettingsPrefix({}), d->defaultsFor({}));

    const int version = settings->value(settingsKey("Version"), 1).toInt();
    if (version == 1) {
        const QString defaultSecondView = d->m_side == Side::Left ? QString("Open Documents")
                                                                  : QString("Bookmarks");
        if (!d->m_defaultLayout.viewIds.contains(defaultSecondView)) {
            d->m_defaultLayout.viewIds += defaultSecondView;
            d->m_defaultLayout.splitterState.clear();
        }
        settings->setValue(settingsKey("Version"), 2);
    }

    settings->beginGroup(settingsKey(modesGroup));
    const QStringList modeIds = settings->childGroups();
    settings->endGroup();
    for (const QString &modeId : modeIds) {
        const Id mode = Id::fromString(modeId);
        if (!ModeManager::modeKeepsOwnLayout(mode))
            continue;
        d->m_modeLayouts.insert(
            mode, readLayout(settings, layoutSettingsPrefix(mode), d->defaultsFor(mode)));
    }

    if (NavigationWidgetPlaceHolder::current(d->m_side))
        d->m_layoutMode = layoutKey(ModeManager::currentModeId());
    applyLayout();

    // Restore last activation positions
    settings->beginGroup(settingsGroup());
    const QString activationKey = QStringLiteral("ActivationPosition.");
    const auto keys = settings->allKeys();
    for (const QString &key : keys) {
        if (!key.startsWith(activationKey))
            continue;

        int position = settings->value(keyFromString(key)).toInt();
        Id factoryId = Id::fromString(key.mid(activationKey.size()));
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

// An invalid mode addresses the keys that predate the per-mode layouts.
Key NavigationWidget::layoutSettingsPrefix(Id mode) const
{
    if (!mode.isValid())
        return settingsGroup() + '/';
    return settingsKey(modesGroup) + '/' + mode.toKey() + '/';
}

QHash<Id, Command *> NavigationWidget::commandMap() const
{
    return d->m_commandMap;
}

#ifdef WITH_TESTS

} // namespace Core

Q_DECLARE_METATYPE(Core::NavigationLayout)

namespace Core {

// writeLayout() leaves a key out when the value matches what the reader would
// fall back to, and readLayout() supplies that fallback. This pins that the
// two agree field by field; that both are given the same fallback is what
// NavigationWidgetPrivate::defaultsFor() is for.
class NavigationSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRoundTripsALayout_data()
    {
        QTest::addColumn<NavigationLayout>("layout");
        QTest::addColumn<NavigationLayout>("defaults");

        const NavigationLayout shown{{"Projects"}, {}, true, 240};
        const NavigationLayout hidden{{"Projects"}, {}, false, 240};
        const NavigationLayout other{{"Open Documents", "Projects"}, "state", true, 400};

        QTest::newRow("everything at the default") << shown << shown;
        QTest::newRow("nothing at the default") << other << hidden;
        QTest::newRow("shown against a hidden default") << shown << hidden;
        QTest::newRow("hidden against a shown default") << hidden << shown;
        QTest::newRow("a width of its own") << other << shown;
    }

    void testRoundTripsALayout()
    {
        QFETCH(NavigationLayout, layout);
        QFETCH(NavigationLayout, defaults);

        Utils::TemporaryDirectory directory("navigation-settings");
        QVERIFY(directory.isValid());
        const QString file = directory.filePath("settings.ini").toUrlishString();

        {
            QtcSettings settings(file, QSettings::IniFormat);
            writeLayout(&settings, "Test/", layout, defaults);
        }

        QtcSettings settings(file, QSettings::IniFormat);
        const NavigationLayout read = readLayout(&settings, "Test/", defaults);

        QCOMPARE(read.viewIds, layout.viewIds);
        QCOMPARE(read.visible, layout.visible);
        QCOMPARE(read.width, layout.width);
        QCOMPARE(read.splitterState, layout.splitterState);
    }
};

QObject *createNavigationSettingsTest()
{
    return new NavigationSettingsTest;
}

#endif // WITH_TESTS

} // namespace Core

#ifdef WITH_TESTS
#include "navigationwidget.moc"
#endif
