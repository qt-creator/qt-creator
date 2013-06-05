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

#include "qmlprofileroverviewmodelproxy.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersimplemodel.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <QVector>
#include <QHash>
#include <QUrl>
#include <QStack>
#include <QString>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerOverviewModelProxy::QmlProfilerOverviewModelProxyPrivate
{
public:
    QmlProfilerOverviewModelProxyPrivate(QmlProfilerOverviewModelProxy *qq) : q(qq) {}
    ~QmlProfilerOverviewModelProxyPrivate() {}

    QVector <QmlProfilerOverviewModelProxy::QmlOverviewEvent> data;
    int rowCount;

    QmlProfilerModelManager *modelManager;
    QmlProfilerOverviewModelProxy *q;
};

QmlProfilerOverviewModelProxy::QmlProfilerOverviewModelProxy(QmlProfilerModelManager *modelManager, QObject *parent)
    : QObject(parent), d(new QmlProfilerOverviewModelProxyPrivate(this))
{
    d->modelManager = modelManager;
    connect(modelManager->simpleModel(),SIGNAL(changed()),this,SLOT(dataChanged()));
    connect(modelManager,SIGNAL(countChanged()),this,SIGNAL(countChanged()));
}

QmlProfilerOverviewModelProxy::~QmlProfilerOverviewModelProxy()
{
    delete d;
}

const QVector<QmlProfilerOverviewModelProxy::QmlOverviewEvent> QmlProfilerOverviewModelProxy::getData() const
{
    return d->data;
}
void QmlProfilerOverviewModelProxy::clear()
{
    d->data.clear();
    d->rowCount = 0;
}

void QmlProfilerOverviewModelProxy::dataChanged()
{
    loadData();

    emit dataAvailable();
    emit emptyChanged();
}

void QmlProfilerOverviewModelProxy::detectBindingLoops()
{
    QStack<int> callStack;

    static QVector<int> acceptedTypes =
            QVector<int>() << QmlDebug::Compiling << QmlDebug::Creating
                           << QmlDebug::Binding << QmlDebug::HandlingSignal;


    for (int i = 0; i < d->data.size(); ++i) {
        QmlOverviewEvent *event = &d->data[i];

        if (!acceptedTypes.contains(event->eventType))
            continue;

        QmlOverviewEvent *potentialParent = callStack.isEmpty() ? 0 : &d->data[callStack.top()];

        while (potentialParent
               && !(potentialParent->startTime + potentialParent->duration > event->startTime)) {
            callStack.pop();
            potentialParent = callStack.isEmpty() ? 0 : &d->data[callStack.top()];
        }

        // check whether event is already in stack
        for (int ii = 0; ii < callStack.size(); ++ii) {
            if (d->data[callStack.at(ii)].eventId == event->eventId) {
                event->bindingLoopHead = callStack.at(ii);
                break;
            }
        }

        callStack.push(i);
    }
}

bool compareEvents(const QmlProfilerOverviewModelProxy::QmlOverviewEvent &t1, const QmlProfilerOverviewModelProxy::QmlOverviewEvent &t2)
{
    return t1.startTime < t2.startTime;
}


void QmlProfilerOverviewModelProxy::loadData()
{
    clear();
    QmlProfilerSimpleModel *simpleModel = d->modelManager->simpleModel();
    if (simpleModel->isEmpty())
        return;

    QHash <QString, int> eventIdDict;

    const QVector<QmlProfilerSimpleModel::QmlEventData> eventList = simpleModel->getEvents();
    foreach (const QmlProfilerSimpleModel::QmlEventData &event, eventList) {

        // find event id
        QString eventHash = QmlProfilerSimpleModel::getHashString(event);
        int eventId;
        if (eventIdDict.contains(eventHash)) {
            eventId = eventIdDict[eventHash];
        } else {
            eventId = eventIdDict.count();
            eventIdDict.insert(eventHash, eventId);
        }

        QmlOverviewEvent newEvent = {
            event.eventType,
            event.startTime,
            event.duration,
            1.0, // height
            eventId, // eventId
            event.eventType, // eventType
            -1 // bindingLoopHead
        };
        d->data.append(newEvent);

        if (event.eventType > d->rowCount)
            d->rowCount = event.eventType;
    }

    qSort(d->data.begin(), d->data.end(), compareEvents);

    detectBindingLoops();
}

/////////////////// QML interface

bool QmlProfilerOverviewModelProxy::empty() const
{
    return count() == 0;
}

int QmlProfilerOverviewModelProxy::count() const
{
    return d->data.count();
}

qint64 QmlProfilerOverviewModelProxy::traceStartTime() const
{
    return d->modelManager->traceTime()->startTime();
}

qint64 QmlProfilerOverviewModelProxy::traceDuration() const
{
    return d->modelManager->traceTime()->duration();
}

qint64 QmlProfilerOverviewModelProxy::traceEndTime() const
{
    return d->modelManager->traceTime()->endTime();
}

qint64 QmlProfilerOverviewModelProxy::getStartTime(int index) const
{
    return d->data[index].startTime;
}

qint64 QmlProfilerOverviewModelProxy::getDuration(int index) const
{
    return d->data[index].duration;
}

int QmlProfilerOverviewModelProxy::getType(int index) const
{
    return d->data[index].row;
}

int QmlProfilerOverviewModelProxy::getEventId(int index) const
{
    return d->data[index].eventId;
}

int QmlProfilerOverviewModelProxy::getBindingLoopDest(int index) const
{
    return d->data[index].bindingLoopHead;
}

}
}
