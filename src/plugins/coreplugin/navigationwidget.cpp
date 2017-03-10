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

#include "navigationwidget.h"
#include "navigationsubwidget.h"
#include "icontext.h"
#include "icore.h"
#include "inavigationwidgetfactory.h"
#include "modemanager.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "id.h"
#include "imode.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

#include <QAction>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStandardItemModel>

Q_DECLARE_METATYPE(Core::INavigationWidgetFactory *)

namespace Core {

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
    layout()->setMargin(0);
    connect(ModeManager::instance(), &ModeManager::currentModeAboutToChange,
            this, &NavigationWidgetPlaceHolder::currentModeAboutToChange);
}

NavigationWidgetPlaceHolder::~NavigationWidgetPlaceHolder()
{
    if (NavigationWidgetPlaceHolder::current(m_side) == this) {
        if (NavigationWidget *nw = NavigationWidget::instance(m_side)) {
            nw->setParent(nullptr);
            nw->hide();
        }
    }
}

void NavigationWidgetPlaceHolder::applyStoredSize(int width)
{
    if (width) {
        QSplitter *splitter = qobject_cast<QSplitter *>(parentWidget());
        if (splitter) {
            // A splitter we need to resize the splitter sizes
            QList<int> sizes = splitter->sizes();
            int index = splitter->indexOf(this);
            int diff = width - sizes.at(index);

            int count = sizes.count();
            for (int i = 0; i < sizes.count(); ++i) {
                if (qobject_cast<NavigationWidgetPlaceHolder *>(splitter->widget(i)))
                    --count;
            }

            int adjust = count > 1 ? (diff / (count - 1)) : 0;
            for (int i = 0; i < sizes.count(); ++i) {
                if (!qobject_cast<NavigationWidgetPlaceHolder *>(splitter->widget(i)))
                    sizes[i] += adjust;
            }

            sizes[index] = width;
            splitter->setSizes(sizes);
        } else {
            QSize s = size();
            s.setWidth(width);
            resize(s);
        }
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
    NavigationWidget *navigationWidget = NavigationWidget::instance(m_side);
    NavigationWidgetPlaceHolder *current = NavigationWidgetPlaceHolder::current(m_side);

    if (current == this) {
        setCurrent(m_side, nullptr);
        navigationWidget->setParent(nullptr);
        navigationWidget->hide();
        navigationWidget->placeHolderChanged(nullptr);
    }

    if (m_mode == mode) {
        setCurrent(m_side, this);

        int width = navigationWidget->storedWidth();

        layout()->addWidget(navigationWidget);
        navigationWidget->show();

        applyStoredSize(width);
        setVisible(navigationWidget->isShown());
        navigationWidget->placeHolderChanged(this);
    }
}

struct ActivationInfo {
    Side side;
    int position;
};
using ActivationsMap = QHash<Id, ActivationInfo>;

struct NavigationWidgetPrivate
{
    explicit NavigationWidgetPrivate(QAction *toggleSideBarAction, Side side);
    ~NavigationWidgetPrivate() { delete m_factoryModel; }

    QList<Internal::NavigationSubWidget *> m_subWidgets;
    QHash<QAction *, Id> m_actionMap;
    QHash<Id, Command *> m_commandMap;
    QStandardItemModel *m_factoryModel;

    bool m_shown;
    bool m_suppressed;
    int m_width;
    QAction *m_toggleSideBarAction; // does not take ownership
    Side m_side;

    static NavigationWidget *s_instanceLeft;
    static NavigationWidget *s_instanceRight;

    static ActivationsMap s_activationsMap;

    static void updateActivationsMap(Id activatedId, const ActivationInfo &activationInfo);
    static void removeFromActivationsMap(const ActivationInfo &activationInfo);
};

NavigationWidgetPrivate::NavigationWidgetPrivate(QAction *toggleSideBarAction, Side side) :
    m_factoryModel(new QStandardItemModel),
    m_shown(true),
    m_suppressed(false),
    m_width(0),
    m_toggleSideBarAction(toggleSideBarAction),
    m_side(side)
{
}

void NavigationWidgetPrivate::updateActivationsMap(Id activatedId, const ActivationInfo &activationInfo)
{
    s_activationsMap.insert(activatedId, activationInfo);
}

NavigationWidget *NavigationWidgetPrivate::s_instanceLeft = nullptr;
NavigationWidget *NavigationWidgetPrivate::s_instanceRight = nullptr;
ActivationsMap NavigationWidgetPrivate::s_activationsMap;

NavigationWidget::NavigationWidget(QAction *toggleSideBarAction, Side side) :
    d(new NavigationWidgetPrivate(toggleSideBarAction, side))
{
    d->m_factoryModel->setSortRole(FactoryPriorityRole);
    setOrientation(Qt::Vertical);

    if (side == Side::Left)
        d->s_instanceLeft = this;
    else
        d->s_instanceRight = this;
}

NavigationWidget::~NavigationWidget()
{
    if (d->m_side == Side::Left)
        NavigationWidgetPrivate::s_instanceLeft = nullptr;
    else
        NavigationWidgetPrivate::s_instanceRight = nullptr;

    delete d;
}

NavigationWidget *NavigationWidget::instance(Side side)
{
    return side == Side::Left ? NavigationWidgetPrivate::s_instanceLeft
                              : NavigationWidgetPrivate::s_instanceRight;
}

QWidget *NavigationWidget::activateSubWidget(Id factoryId, Side fallbackSide)
{
    NavigationWidget *navigationWidget = NavigationWidget::instance(fallbackSide);
    int preferredPosition = -1;

    if (NavigationWidgetPrivate::s_activationsMap.contains(factoryId)) {
        const ActivationInfo info = NavigationWidgetPrivate::s_activationsMap.value(factoryId);
        navigationWidget = NavigationWidget::instance(info.side);
        preferredPosition = info.position;
    }

    navigationWidget->activateSubWidget(factoryId, preferredPosition);
    return navigationWidget;
}

void NavigationWidget::setFactories(const QList<INavigationWidgetFactory *> &factories)
{
    Context navicontext(Constants::C_NAVIGATION_PANE);
    foreach (INavigationWidgetFactory *factory, factories) {
        const Id id = factory->id();
        const Id actionId = id.withPrefix("QtCreator.Sidebar.");

        if (!ActionManager::command(actionId)) {
            QAction *action = new QAction(tr("Activate %1 View").arg(factory->displayName()), this);
            d->m_actionMap.insert(action, id);
            connect(action, &QAction::triggered, this, [this, action]() {
                NavigationWidget::activateSubWidget(d->m_actionMap[action], Side::Left);
            });
            Command *cmd = ActionManager::registerAction(action, actionId, navicontext);
            cmd->setDefaultKeySequence(factory->activationSequence());
            d->m_commandMap.insert(id, cmd);
        }

        QStandardItem *newRow = new QStandardItem(factory->displayName());
        newRow->setData(qVariantFromValue(factory), FactoryObjectRole);
        newRow->setData(QVariant::fromValue(factory->id()), FactoryIdRole);
        newRow->setData(factory->priority(), FactoryPriorityRole);
        d->m_factoryModel->appendRow(newRow);
    }
    d->m_factoryModel->sort(0);
    updateToggleText();
}

QString NavigationWidget::settingsGroup() const
{
    const QString side(d->m_side == Side::Left ? QStringLiteral("Left")
                                               : QStringLiteral("Right"));
    return QStringLiteral("Navigation%1").arg(side);
}

int NavigationWidget::storedWidth()
{
    return d->m_width;
}

QAbstractItemModel *NavigationWidget::factoryModel() const
{
    return d->m_factoryModel;
}

void NavigationWidget::updateToggleText()
{
    bool haveData = d->m_factoryModel->rowCount();
    d->m_toggleSideBarAction->setVisible(haveData);
    d->m_toggleSideBarAction->setEnabled(haveData && NavigationWidgetPlaceHolder::current(d->m_side));

    const char *trToolTip = d->m_side == Side::Left
                                ? (isShown() ? Constants::TR_HIDE_LEFT_SIDEBAR : Constants::TR_SHOW_LEFT_SIDEBAR)
                                : (isShown() ? Constants::TR_HIDE_RIGHT_SIDEBAR : Constants::TR_SHOW_RIGHT_SIDEBAR);

    d->m_toggleSideBarAction->setToolTip(QCoreApplication::translate("Core", trToolTip));
}

void NavigationWidget::placeHolderChanged(NavigationWidgetPlaceHolder *holder)
{
    d->m_toggleSideBarAction->setChecked(holder && isShown());
    updateToggleText();
}

void NavigationWidget::resizeEvent(QResizeEvent *re)
{
    if (d->m_width && re->size().width())
        d->m_width = re->size().width();
    MiniSplitter::resizeEvent(re);
}

Internal::NavigationSubWidget *NavigationWidget::insertSubItem(int position, int factoryIndex)
{
    for (int pos = position + 1; pos < d->m_subWidgets.size(); ++pos) {
        Internal::NavigationSubWidget *nsw = d->m_subWidgets.at(pos);
        nsw->setPosition(pos + 1);
        NavigationWidgetPrivate::updateActivationsMap(nsw->factory()->id(), {d->m_side, pos + 1});
    }

    if (!d->m_subWidgets.isEmpty()) // Make all icons the bottom icon
        d->m_subWidgets.at(0)->setCloseIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());

    Internal::NavigationSubWidget *nsw = new Internal::NavigationSubWidget(this, position, factoryIndex);
    connect(nsw, &Internal::NavigationSubWidget::splitMe,  this, &NavigationWidget::splitSubWidget);
    connect(nsw, &Internal::NavigationSubWidget::closeMe, this, &NavigationWidget::closeSubWidget);
    connect(nsw, &Internal::NavigationSubWidget::factoryIndexChanged,
            this, &NavigationWidget::onSubWidgetFactoryIndexChanged);
    insertWidget(position, nsw);

    d->m_subWidgets.insert(position, nsw);
    d->m_subWidgets.at(0)->setCloseIcon(d->m_subWidgets.size() == 1
                                        ? Utils::Icons::CLOSE_SPLIT_LEFT.icon()
                                        : Utils::Icons::CLOSE_SPLIT_TOP.icon());
    NavigationWidgetPrivate::updateActivationsMap(nsw->factory()->id(), {d->m_side, position});
    return nsw;
}

QWidget *NavigationWidget::activateSubWidget(Id factoryId, int preferredPosition)
{
    setShown(true);
    foreach (Internal::NavigationSubWidget *subWidget, d->m_subWidgets) {
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

void NavigationWidget::splitSubWidget(int factoryIndex)
{
    Internal::NavigationSubWidget *original = qobject_cast<Internal::NavigationSubWidget *>(sender());
    int pos = indexOf(original) + 1;
    insertSubItem(pos, factoryIndex);
}

void NavigationWidget::closeSubWidget()
{
    if (d->m_subWidgets.count() != 1) {
        Internal::NavigationSubWidget *subWidget = qobject_cast<Internal::NavigationSubWidget *>(sender());
        subWidget->saveSettings();

        int position = d->m_subWidgets.indexOf(subWidget);
        for (int pos = position + 1; pos < d->m_subWidgets.size(); ++pos) {
            Internal::NavigationSubWidget *nsw = d->m_subWidgets.at(pos);
            nsw->setPosition(pos - 1);
            NavigationWidgetPrivate::updateActivationsMap(nsw->factory()->id(), {d->m_side, pos - 1});
        }

        d->m_subWidgets.removeOne(subWidget);
        subWidget->hide();
        subWidget->deleteLater();
        // update close button of top item
        if (d->m_subWidgets.size() == 1)
            d->m_subWidgets.at(0)->setCloseIcon(d->m_subWidgets.size() == 1
                                                ? Utils::Icons::CLOSE_SPLIT_LEFT.icon()
                                                : Utils::Icons::CLOSE_SPLIT_TOP.icon());
    } else {
        setShown(false);
    }
}

void NavigationWidget::saveSettings(QSettings *settings)
{
    QStringList viewIds;
    for (int i=0; i<d->m_subWidgets.count(); ++i) {
        d->m_subWidgets.at(i)->saveSettings();
        viewIds.append(d->m_subWidgets.at(i)->factory()->id().toString());
    }
    settings->setValue(settingsKey("Views"), viewIds);
    settings->setValue(settingsKey("Visible"), isShown());
    settings->setValue(settingsKey("VerticalPosition"), saveState());
    settings->setValue(settingsKey("Width"), d->m_width);

    const QString activationKey = QStringLiteral("ActivationPosition.");
    const auto keys = NavigationWidgetPrivate::s_activationsMap.keys();
    for (const auto &factoryId : keys) {
        const auto &info = NavigationWidgetPrivate::s_activationsMap[factoryId];
        if (info.side == d->m_side)
            settings->setValue(settingsKey(activationKey + factoryId.toString()), info.position);
    }
}

void NavigationWidget::restoreSettings(QSettings *settings)
{
    if (!d->m_factoryModel->rowCount()) {
        // We have no widgets to show!
        setShown(false);
        return;
    }

    const bool isLeftSide = d->m_side == Side::Left;
    QLatin1String defaultFirstView = isLeftSide ? QLatin1String("Projects") : QLatin1String("Outline");
    QStringList viewIds = settings->value(settingsKey("Views"), QStringList(defaultFirstView)).toStringList();

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
    foreach (const QString &id, viewIds) {
        int index = factoryIndex(Id::fromString(id));
        if (index >= 0) {
            // Only add if the id was actually found!
            insertSubItem(position, index);
            ++position;
        } else {
            restoreSplitterState = false;
        }
    }

    if (d->m_subWidgets.isEmpty())
        // Make sure we have at least the projects widget or outline widget
        insertSubItem(0, qMax(0, factoryIndex(defaultFirstView.data())));

    setShown(settings->value(settingsKey("Visible"), isLeftSide).toBool());

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
        NavigationWidgetPlaceHolder::current(d->m_side)->applyStoredSize(d->m_width);

    // Restore last activation positions
    settings->beginGroup(settingsGroup());
    const QString activationKey = QStringLiteral("ActivationPosition.");
    const auto keys = settings->allKeys();
    for (const QString &key : keys) {
        if (!key.startsWith(activationKey))
            continue;

        int position = settings->value(key).toInt();
        Id factoryId = Id::fromString(key.mid(activationKey.length()));
        NavigationWidgetPrivate::updateActivationsMap(factoryId, {d->m_side, position});
    }
    settings->endGroup();
}

void NavigationWidget::closeSubWidgets()
{
    foreach (Internal::NavigationSubWidget *subWidget, d->m_subWidgets) {
        subWidget->saveSettings();
        delete subWidget;
    }
    d->m_subWidgets.clear();
}

void NavigationWidget::setShown(bool b)
{
    if (d->m_shown == b)
        return;
    bool haveData = d->m_factoryModel->rowCount();
    d->m_shown = b;
    NavigationWidgetPlaceHolder *current = NavigationWidgetPlaceHolder::current(d->m_side);
    if (current) {
        bool visible = d->m_shown && !d->m_suppressed && haveData;
        current->setVisible(visible);
        d->m_toggleSideBarAction->setChecked(visible);
    } else {
        d->m_toggleSideBarAction->setChecked(false);
    }
    updateToggleText();
}

bool NavigationWidget::isShown() const
{
    return d->m_shown;
}

bool NavigationWidget::isSuppressed() const
{
    return d->m_suppressed;
}

void NavigationWidget::setSuppressed(bool b)
{
    if (d->m_suppressed == b)
        return;
    d->m_suppressed = b;
    if (NavigationWidgetPlaceHolder::current(d->m_side))
        NavigationWidgetPlaceHolder::current(d->m_side)->setVisible(d->m_shown && !d->m_suppressed);
}

int NavigationWidget::factoryIndex(Id id)
{
    for (int row = 0; row < d->m_factoryModel->rowCount(); ++row) {
        if (d->m_factoryModel->data(d->m_factoryModel->index(row, 0), FactoryIdRole).value<Id>() == id)
            return row;
    }
    return -1;
}

QString NavigationWidget::settingsKey(const QString &key) const
{
    return QStringLiteral("%1/%2").arg(settingsGroup(), key);
}

void NavigationWidget::onSubWidgetFactoryIndexChanged(int factoryIndex)
{
    Q_UNUSED(factoryIndex);
    Internal::NavigationSubWidget *subWidget = qobject_cast<Internal::NavigationSubWidget *>(sender());
    QTC_ASSERT(subWidget, return);
    Id factoryId = subWidget->factory()->id();
    NavigationWidgetPrivate::updateActivationsMap(factoryId, {d->m_side, subWidget->position()});
}

QHash<Id, Command *> NavigationWidget::commandMap() const
{
    return d->m_commandMap;
}

} // namespace Core
