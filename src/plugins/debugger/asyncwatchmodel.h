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

#ifndef ASYNCWATCHMODEL_H
#define ASYNCWATCHMODEL_H

#include "watchhandler.h"
#include <QtCore/QPointer>

namespace Debugger {
namespace Internal {

/* AsyncWatchModel: incrementally populated by asynchronous debuggers via
 * fetch. */

class AsyncWatchModel : public WatchModel
{
    Q_OBJECT
public:
    explicit AsyncWatchModel(WatchHandler *handler, WatchType type, QObject *parent = 0);

    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    friend class WatchHandler;
    friend class GdbEngine;

    void insertData(const WatchData &data);

    void removeOutdated();
    void removeOutdatedHelper(WatchItem *item);

    void setActiveData(const QString &data) { m_activeData = data; }

    static int generationCounter;

signals:
    void watchDataUpdateNeeded(const WatchData &data);

private:
    QString m_activeData;
    WatchItem *m_root;
};

/* A Mixin to manage asynchronous models. */

class AsyncWatchModelMixin :  public QObject {
    Q_OBJECT
public:
    explicit AsyncWatchModelMixin(WatchHandler *wh, QObject *parent = 0);
    virtual ~AsyncWatchModelMixin();

    AsyncWatchModel *model(int t) const;
    AsyncWatchModel *modelForIName(const QString &iname) const;

    void beginCycle(); // called at begin of updateLocals() cycle
    void updateWatchers(); // called after locals are fetched
    void endCycle(); // called after all results have been received

public slots:
    void insertData(const WatchData &data);
    // For watchers, ensures that engine is active
    void insertWatcher(const WatchData &data);

signals:
    void watchDataUpdateNeeded(const WatchData &data);

private:
    WatchHandler *m_wh;
    mutable QPointer<AsyncWatchModel> m_models[WatchModelCount];
};

} // namespace Internal
} // namespace Debugger

#endif // ASYNCWATCHMODEL_H
