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

#include "qmlprofilersimplemodel.h"
#include <QStringList>
#include <QVector>
#include <QDebug>
#include "qmldebug/qmlprofilereventtypes.h"

namespace QmlProfiler {
namespace Internal {

QmlProfilerSimpleModel::QmlProfilerSimpleModel(QObject *parent)
    : QObject(parent)
{
}

QmlProfilerSimpleModel::~QmlProfilerSimpleModel()
{
}

void QmlProfilerSimpleModel::clear()
{
    eventList.clear();
    emit changed();
}

bool QmlProfilerSimpleModel::isEmpty() const
{
    return eventList.isEmpty();
}

const QVector<QmlProfilerSimpleModel::QmlEventData> &QmlProfilerSimpleModel::getEvents() const
{
    return eventList;
}

int QmlProfilerSimpleModel::count() const
{
    return eventList.count();
}

void QmlProfilerSimpleModel::addRangedEvent(int type, int bindingType, qint64 startTime, qint64 duration, const QStringList &data, const QmlDebug::QmlEventLocation &location)
{
    QString displayName = QString::fromLatin1("%1:%2").arg(
                location.filename,
                QString::number(location.line));
    QmlEventData eventData = {displayName, type, bindingType, startTime, duration, data, location, 0, 0, 0, 0, 0};
    eventList.append(eventData);
}

void QmlProfilerSimpleModel::addFrameEvent(qint64 time, int framerate, int animationcount)
{
    qint64 duration = 1e9 / framerate;
    QmlEventData eventData = {tr("Animations"), QmlDebug::Painting, QmlDebug::AnimationFrame, time, duration, QStringList(), QmlDebug::QmlEventLocation(), framerate, animationcount, 0, 0, 0};
    eventList.append(eventData);
}

void QmlProfilerSimpleModel::addSceneGraphEvent(int eventType, int SGEtype, qint64 startTime, qint64 timing1, qint64 timing2, qint64 timing3, qint64 timing4, qint64 timing5)
{
    QmlEventData eventData = {QString(), eventType, SGEtype, startTime, 0, QStringList(), QmlDebug::QmlEventLocation(), timing1, timing2, timing3, timing4, timing5};
    eventList.append(eventData);
}

void QmlProfilerSimpleModel::addPixmapCacheEvent(qint64 time, int cacheEventType, const QString &url, int width, int height, int refCount)
{
    QmlDebug::QmlEventLocation location(url, 0, 0);
    QmlEventData eventData = {QString(), QmlDebug::PixmapCacheEvent, cacheEventType, time, 0, QStringList(), location, width, height, refCount, -1, -1};
    eventList.append(eventData);
}

qint64 QmlProfilerSimpleModel::lastTimeMark() const
{
    if (eventList.isEmpty())
        return 0;

    return eventList.last().startTime + eventList.last().duration;
}

void QmlProfilerSimpleModel::complete()
{
    emit changed();
}

QString QmlProfilerSimpleModel::getHashString(const QmlProfilerSimpleModel::QmlEventData &event)
{
    return QString::fromLatin1("%1:%2:%3:%4").arg(
                event.location.filename,
                QString::number(event.location.line),
                QString::number(event.location.column),
                QString::number(event.eventType));
}


}
}
