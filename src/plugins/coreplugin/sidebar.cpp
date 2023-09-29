// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sidebar.h"
#include "sidebarwidget.h"

#include "actionmanager/command.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/utilsicons.h>

#include <QPointer>
#include <QToolButton>

using namespace Utils;

namespace Core {

SideBarItem::SideBarItem(QWidget *widget, const QString &id) :
    m_id(id), m_widget(widget)
{
}

SideBarItem::~SideBarItem()
{
    delete m_widget;
}

QWidget *SideBarItem::widget() const
{
    return m_widget;
}

QString SideBarItem::id() const
{
    return m_id;
}

QString SideBarItem::title() const
{
    return m_widget->windowTitle();
}

QList<QToolButton *> SideBarItem::createToolBarWidgets()
{
    return {};
}

struct SideBarPrivate {
    SideBarPrivate() = default;

    QList<Internal::SideBarWidget*> m_widgets;
    QMap<QString, QPointer<SideBarItem> > m_itemMap;
    QStringList m_availableItemIds;
    QStringList m_availableItemTitles;
    QStringList m_unavailableItemIds;
    QStringList m_defaultVisible;
    QMap<QString, Core::Command*> m_shortcutMap;
    bool m_closeWhenEmpty = false;
};

SideBar::SideBar(QList<SideBarItem*> itemList,
                 QList<SideBarItem*> defaultVisible) :
    d(new SideBarPrivate)
{
    setOrientation(Qt::Vertical);
    for (SideBarItem *item : std::as_const(itemList)) {
        d->m_itemMap.insert(item->id(), item);
        d->m_availableItemIds.append(item->id());
        d->m_availableItemTitles.append(item->title());
    }

    for (SideBarItem *item : std::as_const(defaultVisible)) {
        if (!itemList.contains(item))
            continue;
        d->m_defaultVisible.append(item->id());
    }
}

SideBar::~SideBar()
{
    for (const QPointer<SideBarItem> &i : std::as_const(d->m_itemMap))
        if (!i.isNull())
            delete i.data();
    delete d;
}

QString SideBar::idForTitle(const QString &title) const
{
    for (auto iter = d->m_itemMap.cbegin(), end = d->m_itemMap.cend(); iter != end; ++iter) {
        if (iter.value().data()->title() == title)
            return iter.key();
    }
    return QString();
}

QStringList SideBar::availableItemIds() const
{
    return d->m_availableItemIds;
}

QStringList SideBar::availableItemTitles() const
{
    return d->m_availableItemTitles;
}

QStringList SideBar::unavailableItemIds() const
{
    return d->m_unavailableItemIds;
}

bool SideBar::closeWhenEmpty() const
{
    return d->m_closeWhenEmpty;
}
void SideBar::setCloseWhenEmpty(bool value)
{
    d->m_closeWhenEmpty = value;
}

void SideBar::makeItemAvailable(SideBarItem *item)
{
    auto cend = d->m_itemMap.constEnd();
    for (auto it = d->m_itemMap.constBegin(); it != cend ; ++it) {
        if (it.value().data() == item) {
            d->m_availableItemIds.append(it.key());
            d->m_availableItemTitles.append(it.value().data()->title());
            d->m_unavailableItemIds.removeAll(it.key());
            Utils::sort(d->m_availableItemTitles);
            emit availableItemsChanged();
            //updateWidgets();
            break;
        }
    }
}

// sets a list of externally used, unavailable items. For example,
// another sidebar could set
void SideBar::setUnavailableItemIds(const QStringList &itemIds)
{
    // re-enable previous items
    for (const QString &id : std::as_const(d->m_unavailableItemIds)) {
        d->m_availableItemIds.append(id);
        d->m_availableItemTitles.append(d->m_itemMap.value(id).data()->title());
    }

    d->m_unavailableItemIds.clear();

    for (const QString &id : itemIds) {
        if (!d->m_unavailableItemIds.contains(id))
            d->m_unavailableItemIds.append(id);
        d->m_availableItemIds.removeAll(id);
        d->m_availableItemTitles.removeAll(d->m_itemMap.value(id).data()->title());
    }
    Utils::sort(d->m_availableItemTitles);
    updateWidgets();
}

SideBarItem *SideBar::item(const QString &id)
{
    if (d->m_itemMap.contains(id)) {
        d->m_availableItemIds.removeAll(id);
        d->m_availableItemTitles.removeAll(d->m_itemMap.value(id).data()->title());

        if (!d->m_unavailableItemIds.contains(id))
            d->m_unavailableItemIds.append(id);

        emit availableItemsChanged();
        return d->m_itemMap.value(id).data();
    }
    return nullptr;
}

Internal::SideBarWidget *SideBar::insertSideBarWidget(int position, const QString &id)
{
    if (!d->m_widgets.isEmpty())
        d->m_widgets.at(0)->setCloseIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());

    auto item = new Internal::SideBarWidget(this, id);
    connect(item, &Internal::SideBarWidget::splitMe, this, [this, item] { splitSubWidget(item); });
    connect(item, &Internal::SideBarWidget::closeMe, this, [this, item] { closeSubWidget(item); });
    connect(item, &Internal::SideBarWidget::currentWidgetChanged, this, &SideBar::updateWidgets);
    insertWidget(position, item);
    d->m_widgets.insert(position, item);
    if (d->m_widgets.size() == 1)
    d->m_widgets.at(0)->setCloseIcon(d->m_widgets.size() == 1
                                     ? Utils::Icons::CLOSE_SPLIT_LEFT.icon()
                                     : Utils::Icons::CLOSE_SPLIT_TOP.icon());
    updateWidgets();
    return item;
}

void SideBar::removeSideBarWidget(Internal::SideBarWidget *widget)
{
    widget->removeCurrentItem();
    d->m_widgets.removeOne(widget);
    widget->hide();
    widget->deleteLater();
}

void SideBar::splitSubWidget(Internal::SideBarWidget *widget)
{
    int pos = indexOf(widget) + 1;
    insertSideBarWidget(pos);
    updateWidgets();
}

void SideBar::closeSubWidget(Internal::SideBarWidget *widget)
{
    if (d->m_widgets.count() != 1) {
        removeSideBarWidget(widget);
        // update close button of top item
        if (d->m_widgets.size() == 1)
            d->m_widgets.at(0)->setCloseIcon(d->m_widgets.size() == 1
                                             ? Utils::Icons::CLOSE_SPLIT_LEFT.icon()
                                             : Utils::Icons::CLOSE_SPLIT_TOP.icon());
        updateWidgets();
    } else {
        if (d->m_closeWhenEmpty) {
            setVisible(false);
            emit sideBarClosed();
        }
    }
}

void SideBar::updateWidgets()
{
    for (Internal::SideBarWidget *i : std::as_const(d->m_widgets))
        i->updateAvailableItems();
}

void SideBar::saveSettings(QtcSettings *settings, const QString &name)
{
    const Key prefix = keyFromString(name.isEmpty() ? name : (name + QLatin1Char('/')));

    QStringList views;
    for (int i = 0; i < d->m_widgets.count(); ++i) {
        QString currentItemId = d->m_widgets.at(i)->currentItemId();
        if (!currentItemId.isEmpty())
            views.append(currentItemId);
    }
    if (views.isEmpty() && !d->m_itemMap.isEmpty())
        views.append(d->m_itemMap.cbegin().key());

    settings->setValue(prefix + "Views", views);
    settings->setValue(prefix + "Visible", parentWidget() ? isVisibleTo(parentWidget()) : true);
    settings->setValue(prefix + "VerticalPosition", saveState());
    settings->setValue(prefix + "Width", width());
}

void SideBar::closeAllWidgets()
{
    for (Internal::SideBarWidget *widget : std::as_const(d->m_widgets))
        removeSideBarWidget(widget);
}

void SideBar::readSettings(QtcSettings *settings, const QString &name)
{
    const Key prefix = keyFromString(name.isEmpty() ? name : (name + QLatin1Char('/')));

    closeAllWidgets();

    const Key viewsKey = prefix + "Views";
    if (settings->contains(viewsKey)) {
        const QStringList views = settings->value(viewsKey).toStringList();
        if (!views.isEmpty()) {
            for (const QString &id : views)
                if (availableItemIds().contains(id))
                    insertSideBarWidget(d->m_widgets.count(), id);

        } else {
            insertSideBarWidget(0);
        }
    }
    if (d->m_widgets.size() == 0) {
        for (const QString &id : std::as_const(d->m_defaultVisible))
            insertSideBarWidget(d->m_widgets.count(), id);
    }

    const Key visibleKey = prefix + "Visible";
    if (settings->contains(visibleKey))
        setVisible(settings->value(visibleKey).toBool());

    const Key positionKey = prefix + "VerticalPosition";
    if (settings->contains(positionKey))
        restoreState(settings->value(positionKey).toByteArray());

    const Key widthKey = prefix + "Width";
    if (settings->contains(widthKey)) {
        QSize s = size();
        s.setWidth(settings->value(widthKey).toInt());
        resize(s);
    }
}

void SideBar::activateItem(const QString &id)
{
    QTC_ASSERT(d->m_itemMap.contains(id), return);
    for (int i = 0; i < d->m_widgets.count(); ++i) {
        if (d->m_widgets.at(i)->currentItemId() == id) {
            d->m_itemMap.value(id)->widget()->setFocus();
            return;
        }
    }

    Internal::SideBarWidget *widget = d->m_widgets.first();
    widget->setCurrentItem(id);
    updateWidgets();
    d->m_itemMap.value(id)->widget()->setFocus();
}

void SideBar::setShortcutMap(const QMap<QString, Command*> &shortcutMap)
{
    d->m_shortcutMap = shortcutMap;
}

QMap<QString, Command*> SideBar::shortcutMap() const
{
    return d->m_shortcutMap;
}

} // namespace Core
