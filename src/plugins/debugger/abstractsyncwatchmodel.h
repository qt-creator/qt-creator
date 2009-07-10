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

#ifndef ABSTRACTSYNCWATCHMODEL_H
#define ABSTRACTSYNCWATCHMODEL_H

#include "watchhandler.h"
#include <QtCore/QPointer>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

/* AbstractSyncWatchModel: To be used by synchonous debuggers.
 * Implements all of WatchModel and provides new virtuals for
 * the debugger to complete items. */

class AbstractSyncWatchModel : public WatchModel
{
    Q_OBJECT
public:
    explicit AbstractSyncWatchModel(WatchHandler *handler, WatchType type, QObject *parent = 0);

    virtual QVariant data(const QModelIndex &index, int role) const;

    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &parent) const;

signals:
    // Emitted if one of fetchChildren/complete fails.
    void error(const QString &);

protected:
    // Overwrite these virtuals to fetch children of an item and to complete it
    virtual bool fetchChildren(WatchItem *wd, QString *errorMessage) = 0;
    virtual bool complete(WatchItem *wd, QString *errorMessage) = 0;

private:
    mutable QString m_errorMessage;
};

} // namespace Internal
} // namespace Debugger

#endif // ABSTRACTSYNCWATCHMODEL_H
