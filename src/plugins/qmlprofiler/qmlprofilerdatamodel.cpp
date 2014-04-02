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

#include "qmlprofilerdatamodel.h"
#include "qmlprofilerbasemodel_p.h"
#include "qmlprofilermodelmanager.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <utils/qtcassert.h>
#include <QUrl>
#include <QDebug>
#include <algorithm>

namespace QmlProfiler {

class QmlProfilerDataModel::QmlProfilerDataModelPrivate :
        public QmlProfilerBaseModel::QmlProfilerBaseModelPrivate
{
public:
    QmlProfilerDataModelPrivate(QmlProfilerDataModel *qq) : QmlProfilerBaseModelPrivate(qq) {}
    QVector<QmlEventData> eventList;
private:
    Q_DECLARE_PUBLIC(QmlProfilerDataModel)
};

QmlDebug::QmlEventLocation getLocation(const QmlProfilerDataModel::QmlEventData &event);
QString getDisplayName(const QmlProfilerDataModel::QmlEventData &event);
QString getInitialDetails(const QmlProfilerDataModel::QmlEventData &event);

QmlDebug::QmlEventLocation getLocation(const QmlProfilerDataModel::QmlEventData &event)
{
    QmlDebug::QmlEventLocation eventLocation = event.location;
    if ((event.eventType == QmlDebug::Creating || event.eventType == QmlDebug::Compiling)
            && eventLocation.filename.isEmpty()) {
        eventLocation.filename = getInitialDetails(event);
        eventLocation.line = 1;
        eventLocation.column = 1;
    }
    return eventLocation;
}

QString getDisplayName(const QmlProfilerDataModel::QmlEventData &event)
{
    const QmlDebug::QmlEventLocation eventLocation = getLocation(event);
    QString displayName;

    // generate hash
    if (eventLocation.filename.isEmpty()) {
        displayName = QmlProfilerDataModel::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(eventLocation.filename).path();
        displayName = filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(eventLocation.line);
    }

    return displayName;
}

QString getInitialDetails(const QmlProfilerDataModel::QmlEventData &event)
{
    QString details;
    // generate details string
    if (event.data.isEmpty())
        details = QmlProfilerDataModel::tr("Source code not available.");
    else {
        details = event.data.join(QLatin1String(" ")).replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            details = QmlProfilerDataModel::tr("anonymous function");
        } else {
            QRegExp rewrite(QLatin1String("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)"));
            bool match = rewrite.exactMatch(details);
            if (match)
                details = rewrite.cap(1) + QLatin1String(": ") + rewrite.cap(3);
            if (details.startsWith(QLatin1String("file://")) ||
                    details.startsWith(QLatin1String("qrc:/")))
                details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
        }
    }

    return details;
}


//////////////////////////////////////////////////////////////////////////////

QmlProfilerDataModel::QmlProfilerDataModel(Utils::FileInProjectFinder *fileFinder,
                                                     QmlProfilerModelManager *parent)
    : QmlProfilerBaseModel(fileFinder, parent, new QmlProfilerDataModelPrivate(this))
{
    Q_D(QmlProfilerDataModel);
    // The document loading is very expensive.
    d->modelManager->setProxyCountWeight(d->modelId, 4);
}

const QVector<QmlProfilerDataModel::QmlEventData> &QmlProfilerDataModel::getEvents() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList;
}

int QmlProfilerDataModel::count() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList.count();
}

void QmlProfilerDataModel::clear()
{
    Q_D(QmlProfilerDataModel);
    d->eventList.clear();
    // This call emits changed(). Don't emit it again here.
    QmlProfilerBaseModel::clear();
}

bool QmlProfilerDataModel::isEmpty() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList.isEmpty();
}

inline static bool operator<(const QmlProfilerDataModel::QmlEventData &t1,
                             const QmlProfilerDataModel::QmlEventData &t2)
{
    return t1.startTime < t2.startTime;
}

void QmlProfilerDataModel::complete()
{
    Q_D(QmlProfilerDataModel);
    // post-processing

    // sort events by start time, using above operator<
    std::sort(d->eventList.begin(), d->eventList.end());

    // rewrite strings
    int n = d->eventList.count();
    for (int i = 0; i < n; i++) {
        QmlEventData *event = &d->eventList[i];
        event->location = getLocation(*event);
        event->displayName = getDisplayName(*event);
        event->data = QStringList() << getInitialDetails(*event);

        //
        // request further details from files
        //

        if (event->eventType != QmlDebug::Binding && event->eventType != QmlDebug::HandlingSignal)
            continue;

        // This skips anonymous bindings in Qt4.8 (we don't have valid location data for them)
        if (event->location.filename.isEmpty())
            continue;

        // Skip non-anonymous bindings from Qt4.8 (we already have correct details for them)
        if (event->location.column == -1)
            continue;

        d->detailsRewriter->requestDetailsForLocation(i, event->location);
        d->modelManager->modelProxyCountUpdated(d->modelId, i + n, n * 2);
    }

    // Allow changed() event only after documents have been reloaded to avoid
    // unnecessary updates of child models.
    QmlProfilerBaseModel::complete();
}

void QmlProfilerDataModel::addQmlEvent(int type, int bindingType, qint64 startTime,
                                            qint64 duration, const QStringList &data,
                                            const QmlDebug::QmlEventLocation &location,
                                            qint64 ndata1, qint64 ndata2, qint64 ndata3,
                                            qint64 ndata4, qint64 ndata5)
{
    Q_D(QmlProfilerDataModel);
    QString displayName;
    if (type == QmlDebug::Painting && bindingType == QmlDebug::AnimationFrame) {
        displayName = tr("Animations");
    } else {
        displayName = QString::fromLatin1("%1:%2").arg(
                location.filename,
                QString::number(location.line));
    }

    QmlEventData eventData = {displayName, type, bindingType, startTime, duration, data, location,
                              ndata1, ndata2, ndata3, ndata4, ndata5};
    d->eventList.append(eventData);

    d->modelManager->modelProxyCountUpdated(d->modelId, startTime,
                                            d->modelManager->estimatedProfilingTime() * 2);
}

QString QmlProfilerDataModel::getHashString(const QmlProfilerDataModel::QmlEventData &event)
{
    return QString::fromLatin1("%1:%2:%3:%4:%5").arg(
                event.location.filename,
                QString::number(event.location.line),
                QString::number(event.location.column),
                QString::number(event.eventType),
                QString::number(event.bindingType));
}

qint64 QmlProfilerDataModel::lastTimeMark() const
{
    Q_D(const QmlProfilerDataModel);
    if (d->eventList.isEmpty())
        return 0;

    return d->eventList.last().startTime + d->eventList.last().duration;
}

void QmlProfilerDataModel::detailsChanged(int requestId, const QString &newString)
{
    Q_D(QmlProfilerDataModel);
    QTC_ASSERT(requestId < d->eventList.count(), return);

    QmlEventData *event = &d->eventList[requestId];
    event->data = QStringList(newString);
}

}
