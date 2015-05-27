/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerbasemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerbasemodel_p.h"

namespace QmlProfiler {

QmlProfilerBaseModel::QmlProfilerBaseModel(Utils::FileInProjectFinder *fileFinder,
                                           QmlProfilerModelManager *manager,
                                           QmlProfilerBaseModelPrivate *dd)
    : QObject(manager)
    , d_ptr(dd)
{
    Q_D(QmlProfilerBaseModel);
    d->modelManager = manager;
    d->processingDone = false;
    d->detailsRewriter = new QmlProfilerDetailsRewriter(this, fileFinder);
    Q_ASSERT(d->modelManager);
    d->modelId = d->modelManager->registerModelProxy();
    connect(d->detailsRewriter, SIGNAL(rewriteDetailsString(int,QString)),
            this, SLOT(detailsChanged(int,QString)));
    connect(d->detailsRewriter, SIGNAL(eventDetailsChanged()),
            this, SLOT(detailsDone()));
}

QmlProfilerBaseModel::~QmlProfilerBaseModel()
{
    Q_D(QmlProfilerBaseModel);
    delete d->detailsRewriter;
    delete d;
}

void QmlProfilerBaseModel::clear()
{
    Q_D(QmlProfilerBaseModel);
    d->detailsRewriter->clearRequests();
    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
    d->processingDone = false;
    emit changed();
}

bool QmlProfilerBaseModel::processingDone() const
{
    Q_D(const QmlProfilerBaseModel);
    return d->processingDone;
}

void QmlProfilerBaseModel::complete()
{
    Q_D(QmlProfilerBaseModel);
    d->detailsRewriter->reloadDocuments();
}

QString QmlProfilerBaseModel::formatTime(qint64 timestamp)
{
    if (timestamp < 1e6)
        return QString::number(timestamp/1e3f,'f',3) + trUtf8(" \xc2\xb5s");
    if (timestamp < 1e9)
        return QString::number(timestamp/1e6f,'f',3) + tr(" ms");

    return QString::number(timestamp/1e9f,'f',3) + tr(" s");
}

void QmlProfilerBaseModel::detailsDone()
{
    Q_D(QmlProfilerBaseModel);
    emit changed();
    d->processingDone = true;
    d->modelManager->modelProxyCountUpdated(d->modelId, isEmpty() ? 0 : 1, 1);
    d->modelManager->complete();
}

}
