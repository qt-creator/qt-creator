/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "asyncwatchmodel.h"

#include <QtCore/QCoreApplication>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

int AsyncWatchModel::generationCounter = 0;

static const QString strNotInScope =
        QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");

////////////////////////////////////////////////////////////////////
//
// WatchItem
//
////////////////////////////////////////////////////////////////////

AsyncWatchModel::AsyncWatchModel(WatchHandler *handler, WatchType type, QObject *parent) :
    WatchModel(handler, type, parent)
{
    dummyRoot()->fetchTriggered = true;
}

void AsyncWatchModel::removeOutdated()
{
    WatchItem *item = dummyRoot();
    QTC_ASSERT(item, return);
    foreach (WatchItem *child, item->children)
        removeOutdatedHelper(child);
#if DEBUG_MODEL
#if USE_MODEL_TEST
    //(void) new ModelTest(this, this);
#endif
#endif
}

void AsyncWatchModel::removeOutdatedHelper(WatchItem *item)
{
    if (item->generation < generationCounter)
        removeItem(item);
    else {
        foreach (WatchItem *child, item->children)
            removeOutdatedHelper(child);
        item->fetchTriggered = false;
    }
}

bool AsyncWatchModel::canFetchMore(const QModelIndex &index) const
{
    if (index.isValid())
        if (const WatchItem *item = watchItem(index))
            return !item->fetchTriggered;
    return false;
}

void AsyncWatchModel::fetchMore(const QModelIndex &index)
{
    QTC_ASSERT(index.isValid(), return);
    WatchItem *item = watchItem(index);
    QTC_ASSERT(item && !item->fetchTriggered, return);
    item->fetchTriggered = true;
    WatchData data = *item;
    data.setChildrenNeeded();
    emit watchDataUpdateNeeded(data);
}

static bool iNameSorter(const WatchItem *item1, const WatchItem *item2)
{
    QString name1 = item1->iname.section('.', -1);
    QString name2 = item2->iname.section('.', -1);
    if (!name1.isEmpty() && !name2.isEmpty()) {
        if (name1.at(0).isDigit() && name2.at(0).isDigit())
            return name1.toInt() < name2.toInt();
    }
    return name1 < name2;
}

static int findInsertPosition(const QList<WatchItem *> &list, const WatchItem *item)
{
    QList<WatchItem *>::const_iterator it =
        qLowerBound(list.begin(), list.end(), item, iNameSorter);
    return it - list.begin();
}

void AsyncWatchModel::insertData(const WatchData &data)
{
    // qDebug() << "WMI:" << data.toString();
    QTC_ASSERT(!data.iname.isEmpty(), return);
    WatchItem *parent = findItemByIName(parentName(data.iname), root());
    if (!parent) {
        WatchData parent;
        parent.iname = parentName(data.iname);
        insertData(parent);
        //MODEL_DEBUG("\nFIXING MISSING PARENT FOR\n" << data.iname);
        return;
    }
    QModelIndex index = watchIndex(parent);
    if (WatchItem *oldItem = findItemByIName(data.iname, parent)) {
        // overwrite old entry
        //MODEL_DEBUG("OVERWRITE : " << data.iname << data.value);
        bool changed = !data.value.isEmpty()
            && data.value != oldItem->value
            && data.value != strNotInScope;
        oldItem->setData(data);
        oldItem->changed = changed;
        oldItem->generation = generationCounter;
        QModelIndex idx = watchIndex(oldItem);
        emit dataChanged(idx, idx.sibling(idx.row(), 2));
    } else {
        // add new entry
        //MODEL_DEBUG("INSERT : " << data.iname << data.value);
        WatchItem *item = new WatchItem(data);
        item->parent = parent;
        item->generation = generationCounter;
        item->changed = true;
        int n = findInsertPosition(parent->children, item);
        beginInsertRows(index, n, n);
        parent->children.insert(n, item);
        endInsertRows();
    }
}

// ----------------- AsyncWatchModelMixin
AsyncWatchModelMixin::AsyncWatchModelMixin(WatchHandler *wh,
                                           QObject *parent) :
    QObject(parent),
    m_wh(wh)
{
}

AsyncWatchModelMixin::~AsyncWatchModelMixin()
{
}

AsyncWatchModel *AsyncWatchModelMixin::model(int wt) const
{
    if (wt < 0 ||  wt >= WatchModelCount)
        return 0;
    if (!m_models[wt]) {
        m_models[wt] = new AsyncWatchModel(m_wh, static_cast<WatchType>(wt), const_cast<AsyncWatchModelMixin *>(this));
        connect(m_models[wt], SIGNAL(watchDataUpdateNeeded(WatchData)), this, SIGNAL(watchDataUpdateNeeded(WatchData)));
        m_models[wt]->setObjectName(QLatin1String("model") + QString::number(wt));
    }
    return m_models[wt];
}

AsyncWatchModel *AsyncWatchModelMixin::modelForIName(const QString &iname) const
{
    const WatchType wt = WatchHandler::watchTypeOfIName(iname);
    QTC_ASSERT(wt != WatchModelCount, return 0);
    return model(wt);
}

void AsyncWatchModelMixin::beginCycle()
{
    ++AsyncWatchModel::generationCounter;
}

void AsyncWatchModelMixin::updateWatchers()
{
    //qDebug() << "UPDATE WATCHERS";
    // copy over all watchers and mark all watchers as incomplete
    foreach (const QString &exp, m_wh->watcherExpressions()) {
        WatchData data;
        data.iname = m_wh->watcherName(exp);
        data.setAllNeeded();
        data.name = exp;
        data.exp = exp;
        insertData(data);
    }
}

void AsyncWatchModelMixin::endCycle()
{
    for (int m = 0; m < WatchModelCount; m++)
        if (m_models[m])
            m_models[m]->removeOutdated();
}

void AsyncWatchModelMixin::insertWatcher(const WatchData &data)
{
    // Is the engine active?
    if (m_models[WatchersWatch] && m_wh->model(WatchersWatch) == m_models[WatchersWatch])
        insertData(data);
}

void AsyncWatchModelMixin::insertData(const WatchData &data)
{
    QTC_ASSERT(data.isValid(), return);
    if (data.isSomethingNeeded()) {
        emit watchDataUpdateNeeded(data);
    } else {
        AsyncWatchModel *model = modelForIName(data.iname);
        QTC_ASSERT(model, return);
        model->insertData(data);
    }
}

} // namespace Internal
} // namespace Debugger
