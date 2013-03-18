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

#include "wizard.h"

#include <QMap>
#include <QHash>

#include <QLabel>
#include <QVBoxLayout>
#include <QStyle>

/*! \class Utils::Wizard

  \brief A wizard with a progress bar on the left.

  Informs the user about the progress.
*/

namespace Utils {

class ProgressItemWidget : public QWidget
{
    Q_OBJECT
public:
    ProgressItemWidget(const QPixmap &indicatorPixmap, const QString &title, QWidget *parent = 0)
        : QWidget(parent),
        m_indicatorVisible(false),
        m_indicatorPixmap(indicatorPixmap)
    {
        m_indicatorLabel = new QLabel(this);
        m_indicatorLabel->setFixedSize(m_indicatorPixmap.size());
        m_titleLabel = new QLabel(title, this);
        QHBoxLayout *l = new QHBoxLayout(this);
        l->setMargin(0);
        l->addWidget(m_indicatorLabel);
        l->addWidget(m_titleLabel);
    }
    void setIndicatorVisible(bool visible) {
        if (m_indicatorVisible == visible)
            return;
        m_indicatorVisible = visible;
        if (m_indicatorVisible)
            m_indicatorLabel->setPixmap(m_indicatorPixmap);
        else
            m_indicatorLabel->setPixmap(QPixmap());
    }
    void setTitle(const QString &title) {
        m_titleLabel->setText(title);
    }
    void setWordWrap(bool wrap) {
        m_titleLabel->setWordWrap(wrap);
    }

private:
    bool m_indicatorVisible;
    QPixmap m_indicatorPixmap;
    QLabel *m_indicatorLabel;
    QLabel *m_titleLabel;
};

class LinearProgressWidget : public QWidget
{
    Q_OBJECT
public:
    LinearProgressWidget(WizardProgress *progress, QWidget *parent = 0);

private slots:
    void slotItemAdded(WizardProgressItem *item);
    void slotItemRemoved(WizardProgressItem *item);
    void slotItemChanged(WizardProgressItem *item);
    void slotNextItemsChanged(WizardProgressItem *item, const QList<WizardProgressItem *> &nextItems);
    void slotNextShownItemChanged(WizardProgressItem *item, WizardProgressItem *nextItem);
    void slotStartItemChanged(WizardProgressItem *item);
    void slotCurrentItemChanged(WizardProgressItem *item);

private:
    void recreateLayout();
    void updateProgress();
    void disableUpdates();
    void enableUpdates();

    QVBoxLayout *m_mainLayout;
    QVBoxLayout *m_itemWidgetLayout;
    WizardProgress *m_wizardProgress;
    QMap<WizardProgressItem *, ProgressItemWidget *> m_itemToItemWidget;
    QMap<ProgressItemWidget *, WizardProgressItem *> m_itemWidgetToItem;
    QList<WizardProgressItem *> m_visibleItems;
    ProgressItemWidget *m_dotsItemWidget;
    int m_disableUpdatesCount;
    QPixmap m_indicatorPixmap;
};

LinearProgressWidget::LinearProgressWidget(WizardProgress *progress, QWidget *parent)
    :
    QWidget(parent),
    m_dotsItemWidget(0),
    m_disableUpdatesCount(0)
{
    m_indicatorPixmap = QIcon::fromTheme(QLatin1String("go-next"), QIcon(QLatin1String(":/utils/images/arrow.png"))).pixmap(16);
    m_wizardProgress = progress;
    m_mainLayout = new QVBoxLayout(this);
    m_itemWidgetLayout = new QVBoxLayout();
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding);
    m_mainLayout->addLayout(m_itemWidgetLayout);
    m_mainLayout->addSpacerItem(spacer);

    m_dotsItemWidget = new ProgressItemWidget(m_indicatorPixmap, tr("..."), this);
    m_dotsItemWidget->setVisible(false);
    m_dotsItemWidget->setEnabled(false);

    connect(m_wizardProgress, SIGNAL(itemAdded(WizardProgressItem*)),
            this, SLOT(slotItemAdded(WizardProgressItem*)));
    connect(m_wizardProgress, SIGNAL(itemRemoved(WizardProgressItem*)),
            this, SLOT(slotItemRemoved(WizardProgressItem*)));
    connect(m_wizardProgress, SIGNAL(itemChanged(WizardProgressItem*)),
            this, SLOT(slotItemChanged(WizardProgressItem*)));
    connect(m_wizardProgress, SIGNAL(nextItemsChanged(WizardProgressItem*,QList<WizardProgressItem*>)),
            this, SLOT(slotNextItemsChanged(WizardProgressItem*,QList<WizardProgressItem*>)));
    connect(m_wizardProgress, SIGNAL(nextShownItemChanged(WizardProgressItem*,WizardProgressItem*)),
            this, SLOT(slotNextShownItemChanged(WizardProgressItem*,WizardProgressItem*)));
    connect(m_wizardProgress, SIGNAL(startItemChanged(WizardProgressItem*)),
            this, SLOT(slotStartItemChanged(WizardProgressItem*)));
    connect(m_wizardProgress, SIGNAL(currentItemChanged(WizardProgressItem*)),
            this, SLOT(slotCurrentItemChanged(WizardProgressItem*)));

    QList<WizardProgressItem *> items = m_wizardProgress->items();
    for (int i = 0; i < items.count(); i++)
        slotItemAdded(items.at(i));
    recreateLayout();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void LinearProgressWidget::slotItemAdded(WizardProgressItem *item)
{
    ProgressItemWidget *itemWidget = new ProgressItemWidget(m_indicatorPixmap, item->title(), this);
    itemWidget->setVisible(false);
    itemWidget->setWordWrap(item->titleWordWrap());
    m_itemToItemWidget.insert(item, itemWidget);
    m_itemWidgetToItem.insert(itemWidget, item);
}

void LinearProgressWidget::slotItemRemoved(WizardProgressItem *item)
{
    ProgressItemWidget *itemWidget = m_itemToItemWidget.value(item);
    if (!itemWidget)
        return;

    m_itemWidgetToItem.remove(itemWidget);
    m_itemToItemWidget.remove(item);

    recreateLayout();

    delete itemWidget;
}

void LinearProgressWidget::slotItemChanged(WizardProgressItem *item)
{
    ProgressItemWidget *itemWidget = m_itemToItemWidget.value(item);
    if (!itemWidget)
        return;

    itemWidget->setTitle(item->title());
    itemWidget->setWordWrap(item->titleWordWrap());
}

void LinearProgressWidget::slotNextItemsChanged(WizardProgressItem *item, const QList<WizardProgressItem *> &nextItems)
{
    Q_UNUSED(nextItems)
    if (m_visibleItems.contains(item))
        recreateLayout();
}

void LinearProgressWidget::slotNextShownItemChanged(WizardProgressItem *item, WizardProgressItem *nextItem)
{
    Q_UNUSED(nextItem)
    if (m_visibleItems.contains(item))
        recreateLayout();
}

void LinearProgressWidget::slotStartItemChanged(WizardProgressItem *item)
{
    Q_UNUSED(item)
    recreateLayout();
}

void LinearProgressWidget::slotCurrentItemChanged(WizardProgressItem *item)
{
    Q_UNUSED(item)
    if (m_wizardProgress->directlyReachableItems() == m_visibleItems)
        updateProgress();
    else
        recreateLayout();
}

void LinearProgressWidget::recreateLayout()
{
    disableUpdates();

    QMap<WizardProgressItem *, ProgressItemWidget *>::ConstIterator it = m_itemToItemWidget.constBegin();
    QMap<WizardProgressItem *, ProgressItemWidget *>::ConstIterator itEnd = m_itemToItemWidget.constEnd();
    while (it != itEnd) {
        it.value()->setVisible(false);
        ++it;
    }
    m_dotsItemWidget->setVisible(false);

    for (int i = m_itemWidgetLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = m_itemWidgetLayout->takeAt(i);
        delete item;
    }

    m_visibleItems = m_wizardProgress->directlyReachableItems();
    for (int i = 0; i < m_visibleItems.count(); i++) {
        ProgressItemWidget *itemWidget = m_itemToItemWidget.value(m_visibleItems.at(i));
        m_itemWidgetLayout->addWidget(itemWidget);
        itemWidget->setVisible(true);
    }

    if (!m_wizardProgress->isFinalItemDirectlyReachable()) {
        m_itemWidgetLayout->addWidget(m_dotsItemWidget);
        m_dotsItemWidget->setVisible(true);
    }

    enableUpdates();
    updateProgress();
}

void LinearProgressWidget::updateProgress()
{
    disableUpdates();

    QList<WizardProgressItem *> visitedItems = m_wizardProgress->visitedItems();

    QMap<WizardProgressItem *, ProgressItemWidget *>::ConstIterator it = m_itemToItemWidget.constBegin();
    QMap<WizardProgressItem *, ProgressItemWidget *>::ConstIterator itEnd = m_itemToItemWidget.constEnd();
    while (it != itEnd) {
        WizardProgressItem *item = it.key();
        ProgressItemWidget *itemWidget = it.value();
        itemWidget->setEnabled(visitedItems.contains(item));
        itemWidget->setIndicatorVisible(false);
        ++it;
    }

    WizardProgressItem *currentItem = m_wizardProgress->currentItem();
    ProgressItemWidget *currentItemWidget = m_itemToItemWidget.value(currentItem);
    if (currentItemWidget)
        currentItemWidget->setIndicatorVisible(true);

    enableUpdates();
}

void LinearProgressWidget::disableUpdates()
{
    if (m_disableUpdatesCount++ == 0) {
        setUpdatesEnabled(false);
        hide();
    }
}

void LinearProgressWidget::enableUpdates()
{
    if (--m_disableUpdatesCount == 0) {
        show();
        setUpdatesEnabled(true);
    }
}

class WizardPrivate
{
    Wizard *q_ptr;
    Q_DECLARE_PUBLIC(Wizard)

public:
    WizardPrivate()
        :
        m_automaticProgressCreation(true),
        m_wizardProgress(0)
    {
    }
    bool m_automaticProgressCreation;
    WizardProgress *m_wizardProgress;
};

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags) :
    QWizard(parent, flags), d_ptr(new WizardPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->m_wizardProgress = new WizardProgress(this);
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(_q_currentPageChanged(int)));
    connect(this, SIGNAL(pageAdded(int)), this, SLOT(_q_pageAdded(int)));
    connect(this, SIGNAL(pageRemoved(int)), this, SLOT(_q_pageRemoved(int)));
    setSideWidget(new LinearProgressWidget(d_ptr->m_wizardProgress, this));
}

Wizard::~Wizard()
{
    delete d_ptr;
}

bool Wizard::isAutomaticProgressCreationEnabled() const
{
    Q_D(const Wizard);

    return d->m_automaticProgressCreation;
}

void Wizard::setAutomaticProgressCreationEnabled(bool enabled)
{
    Q_D(Wizard);

    d->m_automaticProgressCreation = enabled;
}

void Wizard::setStartId(int pageId)
{
    Q_D(Wizard);

    QWizard::setStartId(pageId);
    d->m_wizardProgress->setStartPage(startId());
}

WizardProgress *Wizard::wizardProgress() const
{
    Q_D(const Wizard);

    return d->m_wizardProgress;
}

bool Wizard::validateCurrentPage()
{
    emit nextClicked();
    return QWizard::validateCurrentPage();
}

void Wizard::_q_currentPageChanged(int pageId)
{
    Q_D(Wizard);

    d->m_wizardProgress->setCurrentPage(pageId);
}

void Wizard::_q_pageAdded(int pageId)
{
    Q_D(Wizard);

    if (!d->m_automaticProgressCreation)
        return;

    WizardProgressItem *item = d->m_wizardProgress->addItem(page(pageId)->title());
    item->addPage(pageId);
    d->m_wizardProgress->setStartPage(startId());
    if (!d->m_wizardProgress->startItem())
        return;

    QList<int> pages = pageIds();
    int index = pages.indexOf(pageId);
    int prevId = -1;
    int nextId = -1;
    if (index > 0)
        prevId = pages.at(index - 1);
    if (index < pages.count() - 1)
        nextId = pages.at(index + 1);

    WizardProgressItem *prevItem = 0;
    WizardProgressItem *nextItem = 0;

    if (prevId >= 0)
        prevItem = d->m_wizardProgress->item(prevId);
    if (nextId >= 0)
        nextItem = d->m_wizardProgress->item(nextId);

    if (prevItem)
        prevItem->setNextItems(QList<WizardProgressItem *>() << item);
    if (nextItem)
        item->setNextItems(QList<WizardProgressItem *>() << nextItem);
}

void Wizard::_q_pageRemoved(int pageId)
{
    Q_D(Wizard);

    if (!d->m_automaticProgressCreation)
        return;

    WizardProgressItem *item = d->m_wizardProgress->item(pageId);
    d->m_wizardProgress->removePage(pageId);
    d->m_wizardProgress->setStartPage(startId());

    if (!item->pages().isEmpty())
        return;

    QList<int> pages = pageIds();
    int index = pages.indexOf(pageId);
    int prevId = -1;
    int nextId = -1;
    if (index > 0)
        prevId = pages.at(index - 1);
    if (index < pages.count() - 1)
        nextId = pages.at(index + 1);

    WizardProgressItem *prevItem = 0;
    WizardProgressItem *nextItem = 0;

    if (prevId >= 0)
        prevItem = d->m_wizardProgress->item(prevId);
    if (nextId >= 0)
        nextItem = d->m_wizardProgress->item(nextId);

    if (prevItem && nextItem) {
        QList<WizardProgressItem *> nextItems = prevItem->nextItems();
        nextItems.removeOne(item);
        if (!nextItems.contains(nextItem))
            nextItems.append(nextItem);
        prevItem->setNextItems(nextItems);
    }
    d->m_wizardProgress->removeItem(item);
}




class WizardProgressPrivate
{
    WizardProgress *q_ptr;
    Q_DECLARE_PUBLIC(WizardProgress)

public:
    WizardProgressPrivate()
        :
        m_currentItem(0),
        m_startItem(0)
    {
    }

    bool isNextItem(WizardProgressItem *item, WizardProgressItem *nextItem) const;
    // if multiple paths are possible the empty list is returned
    QList<WizardProgressItem *> singlePathBetween(WizardProgressItem *fromItem, WizardProgressItem *toItem) const;
    void updateReachableItems();

    QMap<int, WizardProgressItem *> m_pageToItem;
    QMap<WizardProgressItem *, WizardProgressItem *> m_itemToItem;

    QList<WizardProgressItem *> m_items;

    QList<WizardProgressItem *> m_visitedItems;
    QList<WizardProgressItem *> m_reachableItems;

    WizardProgressItem *m_currentItem;
    WizardProgressItem *m_startItem;
};

class WizardProgressItemPrivate
{
    WizardProgressItem *q_ptr;
    Q_DECLARE_PUBLIC(WizardProgressItem)
public:
    QString m_title;
    bool m_titleWordWrap;
    WizardProgress *m_wizardProgress;
    QList<int> m_pages;
    QList<WizardProgressItem *> m_nextItems;
    QList<WizardProgressItem *> m_prevItems;
    WizardProgressItem *m_nextShownItem;
};

bool WizardProgressPrivate::isNextItem(WizardProgressItem *item, WizardProgressItem *nextItem) const
{
    QHash<WizardProgressItem *, bool> visitedItems;
    QList<WizardProgressItem *> workingItems = item->nextItems();
    while (!workingItems.isEmpty()) {
        WizardProgressItem *workingItem = workingItems.takeFirst();
        if (workingItem == nextItem)
            return true;
        if (visitedItems.contains(workingItem))
            continue;
        visitedItems.insert(workingItem, true);
        workingItems += workingItem->nextItems();
    }
    return false;
}

QList<WizardProgressItem *> WizardProgressPrivate::singlePathBetween(WizardProgressItem *fromItem, WizardProgressItem *toItem) const
{
    WizardProgressItem *item = fromItem;
    if (!item)
        item = m_startItem;
    if (!item)
        return QList<WizardProgressItem *>();

    // Optimization. It is workaround for case A->B, B->C, A->C where "from" is A and "to" is C.
    // When we had X->A in addition and "from" was X and "to" was C, this would not work
    // (it should return the shortest path which would be X->A->C).
    if (item->nextItems().contains(toItem))
        return QList<WizardProgressItem *>() << toItem;

    QHash<WizardProgressItem *, QHash<WizardProgressItem *, bool> > visitedItemsToParents;
    QList<QPair<WizardProgressItem *, WizardProgressItem *> > workingItems; // next to prev item

    QList<WizardProgressItem *> items = item->nextItems();
    for (int i = 0; i < items.count(); i++)
        workingItems.append(qMakePair(items.at(i), item));

    while (!workingItems.isEmpty()) {
        QPair<WizardProgressItem *, WizardProgressItem *> workingItem = workingItems.takeFirst();

        QHash<WizardProgressItem *, bool> &parents = visitedItemsToParents[workingItem.first];
        parents.insert(workingItem.second, true);
        if (parents.count() > 1)
            continue;

        QList<WizardProgressItem *> items = workingItem.first->nextItems();
        for (int i = 0; i < items.count(); i++)
            workingItems.append(qMakePair(items.at(i), workingItem.first));
    }

    QList<WizardProgressItem *> path;

    WizardProgressItem *it = toItem;
    QHash<WizardProgressItem *, QHash<WizardProgressItem *, bool> >::ConstIterator itItem = visitedItemsToParents.constFind(it);
    QHash<WizardProgressItem *, QHash<WizardProgressItem *, bool> >::ConstIterator itEnd = visitedItemsToParents.constEnd();
    while (itItem != itEnd) {
        path.prepend(itItem.key());
        if (itItem.value().count() != 1)
            return QList<WizardProgressItem *>();
        it = itItem.value().constBegin().key();
        if (it == item)
            return path;
        itItem = visitedItemsToParents.constFind(it);
    }
    return QList<WizardProgressItem *>();
}

void WizardProgressPrivate::updateReachableItems()
{
    m_reachableItems = m_visitedItems;
    WizardProgressItem *item = 0;
    if (m_visitedItems.count() > 0)
        item = m_visitedItems.last();
    if (!item) {
        item = m_startItem;
        m_reachableItems.append(item);
    }
    if (!item)
        return;
    while (item->nextShownItem()) {
        item = item->nextShownItem();
        m_reachableItems.append(item);
    }
}


WizardProgress::WizardProgress(QObject *parent)
    : QObject(parent), d_ptr(new WizardProgressPrivate)
{
    d_ptr->q_ptr = this;
}

WizardProgress::~WizardProgress()
{
    Q_D(WizardProgress);

    QMap<WizardProgressItem *, WizardProgressItem *>::ConstIterator it = d->m_itemToItem.constBegin();
    QMap<WizardProgressItem *, WizardProgressItem *>::ConstIterator itEnd = d->m_itemToItem.constEnd();
    while (it != itEnd) {
        delete it.key();
        ++it;
    }
    delete d_ptr;
}

WizardProgressItem *WizardProgress::addItem(const QString &title)
{
    Q_D(WizardProgress);

    WizardProgressItem *item = new WizardProgressItem(this, title);
    d->m_itemToItem.insert(item, item);
    emit itemAdded(item);
    return item;
}

void WizardProgress::removeItem(WizardProgressItem *item)
{
    Q_D(WizardProgress);

    QMap<WizardProgressItem *, WizardProgressItem *>::iterator it = d->m_itemToItem.find(item);
    if (it == d->m_itemToItem.end()) {
        qWarning("WizardProgress::removePage: Item is not a part of the wizard");
        return;
    }

    // remove item from prev items
    QList<WizardProgressItem *> prevItems = item->d_ptr->m_prevItems;
    for (int i = 0; i < prevItems.count(); i++) {
        WizardProgressItem *prevItem = prevItems.at(i);
        prevItem->d_ptr->m_nextItems.removeOne(item);
    }

    // remove item from next items
    QList<WizardProgressItem *> nextItems = item->d_ptr->m_nextItems;
    for (int i = 0; i < nextItems.count(); i++) {
        WizardProgressItem *nextItem = nextItems.at(i);
        nextItem->d_ptr->m_prevItems.removeOne(item);
    }

    // update history
    int idx = d->m_visitedItems.indexOf(item);
    if (idx >= 0)
        d->m_visitedItems.removeAt(idx);

    // update reachable items
    d->updateReachableItems();

    emit itemRemoved(item);

    QList<int> pages = item->pages();
    for (int i = 0; i < pages.count(); i++)
        d->m_pageToItem.remove(pages.at(i));
    d->m_itemToItem.erase(it);
    delete item;
}

void WizardProgress::removePage(int pageId)
{
    Q_D(WizardProgress);

    QMap<int, WizardProgressItem *>::iterator it = d->m_pageToItem.find(pageId);
    if (it == d->m_pageToItem.end()) {
        qWarning("WizardProgress::removePage: page is not a part of the wizard");
        return;
    }
    WizardProgressItem *item = it.value();
    d->m_pageToItem.erase(it);
    item->d_ptr->m_pages.removeOne(pageId);
}

QList<int> WizardProgress::pages(WizardProgressItem *item) const
{
    return item->pages();
}

WizardProgressItem *WizardProgress::item(int pageId) const
{
    Q_D(const WizardProgress);

    return d->m_pageToItem.value(pageId);
}

WizardProgressItem *WizardProgress::currentItem() const
{
    Q_D(const WizardProgress);

    return d->m_currentItem;
}

QList<WizardProgressItem *> WizardProgress::items() const
{
    Q_D(const WizardProgress);

    return d->m_itemToItem.keys();
}

WizardProgressItem *WizardProgress::startItem() const
{
    Q_D(const WizardProgress);

    return d->m_startItem;
}

QList<WizardProgressItem *> WizardProgress::visitedItems() const
{
    Q_D(const WizardProgress);

    return d->m_visitedItems;
}

QList<WizardProgressItem *> WizardProgress::directlyReachableItems() const
{
    Q_D(const WizardProgress);

    return d->m_reachableItems;
}

bool WizardProgress::isFinalItemDirectlyReachable() const
{
    Q_D(const WizardProgress);

    if (d->m_reachableItems.isEmpty())
        return false;

    return d->m_reachableItems.last()->isFinalItem();
}

void WizardProgress::setCurrentPage(int pageId)
{
    Q_D(WizardProgress);

    if (pageId < 0) { // reset history
        d->m_currentItem = 0;
        d->m_visitedItems.clear();
        d->m_reachableItems.clear();
        d->updateReachableItems();
        return;
    }

    WizardProgressItem *item = d->m_pageToItem.value(pageId);
    if (!item) {
        qWarning("WizardProgress::setCurrentPage: page is not mapped to any wizard progress item");
        return;
    }

    if (d->m_currentItem == item) // nothing changes
        return;

    const bool currentStartItem = !d->m_currentItem && d->m_startItem && d->m_startItem == item;

    // Check if item is reachable with the provided history or with the next items.
    const QList<WizardProgressItem *> singleItemPath = d->singlePathBetween(d->m_currentItem, item);
    const int prevItemIndex = d->m_visitedItems.indexOf(item);

    if (singleItemPath.isEmpty() && prevItemIndex < 0 && !currentStartItem) {
        qWarning("WizardProgress::setCurrentPage: new current item is not directly reachable from the old current item");
        return;
    }

    // Update the history accordingly.
    if (prevItemIndex >= 0) {
        while (prevItemIndex + 1 < d->m_visitedItems.count())
            d->m_visitedItems.removeLast();
    } else {
        if ((!d->m_currentItem && d->m_startItem && !singleItemPath.isEmpty()) || currentStartItem)
            d->m_visitedItems += d->m_startItem;
        d->m_visitedItems += singleItemPath;
    }

    d->m_currentItem = item;

    // Update reachable items accordingly.
    d->updateReachableItems();

    emit currentItemChanged(item);
}

void WizardProgress::setStartPage(int pageId)
{
    Q_D(WizardProgress);

    WizardProgressItem *item = d->m_pageToItem.value(pageId);
    if (!item) {
        qWarning("WizardProgress::setStartPage: page is not mapped to any wizard progress item");
        return;
    }

    d->m_startItem = item;
    d->updateReachableItems();

    emit startItemChanged(item);
}

WizardProgressItem::WizardProgressItem(WizardProgress *progress, const QString &title)
    : d_ptr(new WizardProgressItemPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->m_title = title;
    d_ptr->m_titleWordWrap = false;
    d_ptr->m_wizardProgress = progress;
    d_ptr->m_nextShownItem = 0;
}

WizardProgressItem::~WizardProgressItem()
{
    delete d_ptr;
}

void WizardProgressItem::addPage(int pageId)
{
    Q_D(WizardProgressItem);

    if (d->m_wizardProgress->d_ptr->m_pageToItem.contains(pageId)) {
        qWarning("WizardProgress::addPage: Page is already added to the item");
        return;
    }
    d->m_pages.append(pageId);
    d->m_wizardProgress->d_ptr->m_pageToItem.insert(pageId, this);
}

QList<int> WizardProgressItem::pages() const
{
    Q_D(const WizardProgressItem);

    return d->m_pages;
}

void WizardProgressItem::setNextItems(const QList<WizardProgressItem *> &items)
{
    Q_D(WizardProgressItem);

    // check if we create cycle
    for (int i = 0; i < items.count(); i++) {
        WizardProgressItem *nextItem = items.at(i);
        if (nextItem == this || d->m_wizardProgress->d_ptr->isNextItem(nextItem, this)) {
            qWarning("WizardProgress::setNextItems: Setting one of the next items would create a cycle");
            return;
        }
    }

    if (d->m_nextItems == items) // nothing changes
        return;

    if (!items.contains(d->m_nextShownItem))
        setNextShownItem(0);

    // update prev items (remove this item from the old next items)
    for (int i = 0; i < d->m_nextItems.count(); i++) {
        WizardProgressItem *nextItem = d->m_nextItems.at(i);
        nextItem->d_ptr->m_prevItems.removeOne(this);
    }

    d->m_nextItems = items;

    // update prev items (add this item to the new next items)
    for (int i = 0; i < d->m_nextItems.count(); i++) {
        WizardProgressItem *nextItem = d->m_nextItems.at(i);
        nextItem->d_ptr->m_prevItems.append(this);
    }

    d->m_wizardProgress->d_ptr->updateReachableItems();

    emit d->m_wizardProgress->nextItemsChanged(this, items);

    if (items.count() == 1)
        setNextShownItem(items.first());
}

QList<WizardProgressItem *> WizardProgressItem::nextItems() const
{
    Q_D(const WizardProgressItem);

    return d->m_nextItems;
}

void WizardProgressItem::setNextShownItem(WizardProgressItem *item)
{
    Q_D(WizardProgressItem);

    if (d->m_nextShownItem == item) // nothing changes
        return;

    if (item && !d->m_nextItems.contains(item)) // the "item" is not a one of next items
        return;

    d->m_nextShownItem = item;

    d->m_wizardProgress->d_ptr->updateReachableItems();

    emit d->m_wizardProgress->nextShownItemChanged(this, item);
}

WizardProgressItem *WizardProgressItem::nextShownItem() const
{
    Q_D(const WizardProgressItem);

    return d->m_nextShownItem;
}

bool WizardProgressItem::isFinalItem() const
{
    return nextItems().isEmpty();
}

void WizardProgressItem::setTitle(const QString &title)
{
    Q_D(WizardProgressItem);

    d->m_title = title;
    emit d->m_wizardProgress->itemChanged(this);
}

QString WizardProgressItem::title() const
{
    Q_D(const WizardProgressItem);

    return d->m_title;
}

void WizardProgressItem::setTitleWordWrap(bool wrap)
{
    Q_D(WizardProgressItem);

    d->m_titleWordWrap = wrap;
    emit d->m_wizardProgress->itemChanged(this);
}

bool WizardProgressItem::titleWordWrap() const
{
    Q_D(const WizardProgressItem);

    return d->m_titleWordWrap;
}

} // namespace Utils

#include "wizard.moc"
