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

#include "qmlprofilersimplemodel.h"

#include "qmlprofilermodelmanager.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <QStringList>
#include <QVector>
#include <QDebug>

namespace QmlProfiler {

QmlProfilerSimpleModel::QmlProfilerSimpleModel(QmlProfilerModelManager *parent)
    : QmlProfilerBaseModel(parent)
{
    m_modelManager->setProxyCountWeight(m_modelId, 2);
}

QmlProfilerSimpleModel::~QmlProfilerSimpleModel()
{
}

void QmlProfilerSimpleModel::clear()
{
    eventList.clear();
    QmlProfilerBaseModel::clear();
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

void QmlProfilerSimpleModel::addQmlEvent(int type, int bindingType, qint64 startTime, qint64 duration, const QStringList &data, const QmlDebug::QmlEventLocation &location, qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5)
{
    QString displayName;
    if (type == QmlDebug::Painting && bindingType == QmlDebug::AnimationFrame) {
        displayName = tr("Animations");
    } else {
        displayName = QString::fromLatin1("%1:%2").arg(
                location.filename,
                QString::number(location.line));
    }

    QmlEventData eventData = {displayName, type, bindingType, startTime, duration, data, location, ndata1, ndata2, ndata3, ndata4, ndata5};
    eventList.append(eventData);

    m_modelManager->modelProxyCountUpdated(m_modelId, startTime, m_modelManager->estimatedProfilingTime());
}

qint64 QmlProfilerSimpleModel::lastTimeMark() const
{
    if (eventList.isEmpty())
        return 0;

    return eventList.last().startTime + eventList.last().duration;
}

QString QmlProfilerSimpleModel::getHashString(const QmlProfilerSimpleModel::QmlEventData &event)
{
    return QString::fromLatin1("%1:%2:%3:%4:%5").arg(
                event.location.filename,
                QString::number(event.location.line),
                QString::number(event.location.column),
                QString::number(event.eventType),
                QString::number(event.bindingType));
}

QString QmlProfilerSimpleModel::formatTime(qint64 timestamp)
{
    if (timestamp < 1e6)
        return QString::number(timestamp/1e3f,'f',3) + trUtf8(" \xc2\xb5s");
    if (timestamp < 1e9)
        return QString::number(timestamp/1e6f,'f',3) + tr(" ms");

    return QString::number(timestamp/1e9f,'f',3) + tr(" s");
}

}
