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

#include "abstractsyncwatchmodel.h"

#include <utils/qtcassert.h>
#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

enum  { debug = 0 };

AbstractSyncWatchModel::AbstractSyncWatchModel(WatchHandler *handler, WatchType type, QObject *parent) :
    WatchModel(handler, type, parent)
{
}

void AbstractSyncWatchModel::fetchMore(const QModelIndex &parent)
{
    WatchItem *item = watchItem(parent);
    if (!item || item == root())
        return;
    if (debug)
        qDebug() << ">fetchMore" << item->toString();
    // Figure out children...
    if (item->isHasChildrenNeeded()) {
        m_errorMessage.clear();
        if (!complete(item, &m_errorMessage)) {
            item->setHasChildren(false);
            emit error(m_errorMessage);
        }
    }
    if (item->isChildrenNeeded()) {
        m_errorMessage.clear();
        if (fetchChildren(item, &m_errorMessage)) {
            item->setChildrenUnneeded();
        } else {
            item->setHasChildren(false);
            emit error(m_errorMessage);
        }
    }

    if (debug)
        qDebug() << "<fetchMore" << item->iname << item->children.size();
}

bool AbstractSyncWatchModel::canFetchMore(const QModelIndex &parent) const
{
    WatchItem *item = watchItem(parent);
    if (!item || item == root())
        return false;
    const bool rc = item->isChildrenNeeded() || item->isHasChildrenNeeded();
    if (debug)
        qDebug() << "canFetchMore" << rc << item->iname;
    return rc;
}

QVariant AbstractSyncWatchModel::data(const QModelIndex &idx, int role) const
{
    if (WatchItem *wdata = watchItem(idx)) {
        const int column = idx.column();
        // Is any display data (except children) needed?
        const bool incomplete = (wdata->state & ~WatchData::ChildrenNeeded);
        if ((debug > 1) && incomplete && column == 0 && role == Qt::DisplayRole)
            qDebug() << "data()" << "incomplete=" << incomplete << wdata->toString();
        if (incomplete) {
            AbstractSyncWatchModel *nonConstThis = const_cast<AbstractSyncWatchModel *>(this);
            m_errorMessage.clear();
            if (!nonConstThis->complete(wdata, &m_errorMessage)) {
                wdata->setAllUnneeded();
                nonConstThis->emit error(m_errorMessage);
            }
        }
        return WatchModel::data(*wdata, column, role);
    }
    return QVariant();
}

} // namespace Internal
} // namespace Debugger
