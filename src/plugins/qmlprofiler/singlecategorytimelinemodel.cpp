/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "singlecategorytimelinemodel.h"
#include "singlecategorytimelinemodel_p.h"

namespace QmlProfiler {

SingleCategoryTimelineModel::SingleCategoryTimelineModel(SingleCategoryTimelineModelPrivate *dd,
        const QString &name, const QString &label, QmlDebug::QmlEventType eventType,
        QObject *parent) :
    AbstractTimelineModel(dd, name, parent)
{
    Q_D(SingleCategoryTimelineModel);
    d->expanded = false;
    d->label = label;
    d->eventType = eventType;
}

/////////////////// QML interface

bool SingleCategoryTimelineModel::eventAccepted(const QmlProfilerDataModel::QmlEventData &event) const
{
    Q_D(const SingleCategoryTimelineModel);
    return (event.eventType == d->eventType);
}

bool SingleCategoryTimelineModel::expanded(int categoryIndex) const
{
    Q_D(const SingleCategoryTimelineModel);
    Q_UNUSED(categoryIndex);
    return d->expanded;
}

void SingleCategoryTimelineModel::setExpanded(int categoryIndex, bool expanded)
{
    Q_D(SingleCategoryTimelineModel);
    Q_UNUSED(categoryIndex);
    if (expanded != d->expanded) {
        d->expanded = expanded;
        emit expandedChanged();
    }
}

int SingleCategoryTimelineModel::categoryCount() const
{
    return 1;
}

const QString SingleCategoryTimelineModel::categoryLabel(int categoryIndex) const
{
    Q_D(const SingleCategoryTimelineModel);
    Q_UNUSED(categoryIndex);
    return d->label;
}

int SingleCategoryTimelineModel::getEventType(int index) const
{
    Q_D(const SingleCategoryTimelineModel);
    Q_UNUSED(index);
    return (int)d->eventType;
}

int SingleCategoryTimelineModel::getEventCategory(int index) const
{
    Q_UNUSED(index);
    return 0;
}

}
