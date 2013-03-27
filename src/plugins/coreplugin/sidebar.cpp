/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sidebar.h"
#include "sidebarwidget.h"

#include "actionmanager/command.h"

#include <QSettings>
#include <QPointer>
#include <QToolButton>

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
    return QList<QToolButton *>();
}

struct SideBarPrivate {
    SideBarPrivate() :m_closeWhenEmpty(false) {}

    QList<Internal::SideBarWidget*> m_widgets;
    QMap<QString, QPointer<SideBarItem> > m_itemMap;
    QStringList m_availableItemIds;
    QStringList m_availableItemTitles;
    QStringList m_unavailableItemIds;
    QStringList m_defaultVisible;
    QMap<QString, Core::Command*> m_shortcutMap;
    bool m_closeWhenEmpty;
};

SideBar::SideBar(QList<SideBarItem*> itemList,
                 QList<SideBarItem*> defaultVisible) :
    d(new SideBarPrivate)
{
    setOrientation(Qt::Vertical);
    foreach (SideBarItem *item, itemList) {
        d->m_itemMap.insert(item->id(), item);
        d->m_availableItemIds.append(item->id());
        d->m_availableItemTitles.append(item->title());
    }

    foreach (SideBarItem *item, defaultVisible) {
        if (!itemList.contains(item))
            continue;
        d->m_defaultVisible.append(item->id());
    }
}

SideBar::~SideBar()
{
    foreach (const QPointer<SideBarItem> &i, d->m_itemMap)
        if (!i.isNull())
            delete i.data();
    delete d;
}

QString SideBar::idForTitle(const QString &title) const
{
    QMapIterator<QString, QPointer<SideBarItem> > iter(d->m_itemMap);
    while (iter.hasNext()) {
        iter.next();
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
    typedef QMap<QString, QPointer<SideBarItem> >::const_iterator Iterator;

    const Iterator cend = d->m_itemMap.constEnd();
    for (Iterator it = d->m_itemMap.constBegin(); it != cend ; ++it) {
        if (it.value().data() == item) {
            d->m_availableItemIds.append(it.key());
            d->m_availableItemTitles.append(it.value().data()->title());
            d->m_unavailableItemIds.removeAll(it.key());
            qSort(d->m_availableItemTitles);
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
    foreach (const QString &id, d->m_unavailableItemIds) {
        d->m_availableItemIds.append(id);
        d->m_availableItemTitles.append(d->m_itemMap.value(id).data()->title());
    }

    d->m_unavailableItemIds.clear();

    foreach (const QString &id, itemIds) {
        if (!d->m_unavailableItemIds.contains(id))
            d->m_unavailableItemIds.append(id);
        d->m_availableItemIds.removeAll(id);
        d->m_availableItemTitles.removeAll(d->m_itemMap.value(id).data()->title());
    }
    qSort(d->m_availableItemTitles);
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
    return 0;
}

Internal::SideBarWidget *SideBar::insertSideBarWidget(int position, const QString &id)
{
    Internal::SideBarWidget *item = new Internal::SideBarWidget(this, id);
    connect(item, SIGNAL(splitMe()), this, SLOT(splitSubWidget()));
    connect(item, SIGNAL(closeMe()), this, SLOT(closeSubWidget()));
    connect(item, SIGNAL(currentWidgetChanged()), this, SLOT(updateWidgets()));
    insertWidget(position, item);
    d->m_widgets.insert(position, item);
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

void SideBar::splitSubWidget()
{
    Internal::SideBarWidget *original = qobject_cast<Internal::SideBarWidget*>(sender());
    int pos = indexOf(original) + 1;
    insertSideBarWidget(pos);
    updateWidgets();
}

void SideBar::closeSubWidget()
{
    if (d->m_widgets.count() != 1) {
        Internal::SideBarWidget *widget = qobject_cast<Internal::SideBarWidget*>(sender());
        if (!widget)
            return;
        removeSideBarWidget(widget);
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
    foreach (Internal::SideBarWidget *i, d->m_widgets)
        i->updateAvailableItems();
}

void SideBar::saveSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    QStringList views;
    for (int i = 0; i < d->m_widgets.count(); ++i) {
        QString currentItemId = d->m_widgets.at(i)->currentItemId();
        if (!currentItemId.isEmpty())
            views.append(currentItemId);
    }
    if (views.isEmpty() && d->m_itemMap.size()) {
        QMapIterator<QString, QPointer<SideBarItem> > iter(d->m_itemMap);
        iter.next();
        views.append(iter.key());
    }

    settings->setValue(prefix + QLatin1String("Views"), views);
    settings->setValue(prefix + QLatin1String("Visible"), true);
    settings->setValue(prefix + QLatin1String("VerticalPosition"), saveState());
    settings->setValue(prefix + QLatin1String("Width"), width());
}

void SideBar::closeAllWidgets()
{
    foreach (Internal::SideBarWidget *widget, d->m_widgets)
        removeSideBarWidget(widget);
}

void SideBar::readSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    closeAllWidgets();

    const QString viewsKey = prefix + QLatin1String("Views");
    if (settings->contains(viewsKey)) {
        QStringList views = settings->value(viewsKey).toStringList();
        if (views.count()) {
            foreach (const QString &id, views)
                insertSideBarWidget(d->m_widgets.count(), id);

        } else {
            insertSideBarWidget(0);
        }
    } else {
        foreach (const QString &id, d->m_defaultVisible)
            insertSideBarWidget(d->m_widgets.count(), id);
    }

    const QString visibleKey = prefix + QLatin1String("Visible");
    if (settings->contains(visibleKey))
        setVisible(settings->value(visibleKey).toBool());

    const QString positionKey = prefix + QLatin1String("VerticalPosition");
    if (settings->contains(positionKey))
        restoreState(settings->value(positionKey).toByteArray());

    const QString widthKey = prefix + QLatin1String("Width");
    if (settings->contains(widthKey)) {
        QSize s = size();
        s.setWidth(settings->value(widthKey).toInt());
        resize(s);
    }
}

void SideBar::activateItem(SideBarItem *item)
{
    typedef QMap<QString, QPointer<SideBarItem> >::const_iterator Iterator;

    QString id;
    const Iterator cend = d->m_itemMap.constEnd();
    for (Iterator it = d->m_itemMap.constBegin(); it != cend ; ++it) {
        if (it.value().data() == item) {
            id = it.key();
            break;
        }
    }

    if (id.isEmpty())
        return;

    for (int i = 0; i < d->m_widgets.count(); ++i) {
        if (d->m_widgets.at(i)->currentItemId() == id) {
            item->widget()->setFocus();
            return;
        }
    }

    Internal::SideBarWidget *widget = d->m_widgets.first();
    widget->setCurrentItem(id);
    updateWidgets();
    item->widget()->setFocus();
}

void SideBar::setShortcutMap(const QMap<QString, Core::Command*> &shortcutMap)
{
    d->m_shortcutMap = shortcutMap;
}

QMap<QString, Core::Command*> SideBar::shortcutMap() const
{
    return d->m_shortcutMap;
}
} // namespace Core

