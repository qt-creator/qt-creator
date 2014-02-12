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

#include "qmlprofilerprocessedmodel.h"
#include "qmlprofilermodelmanager.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <utils/qtcassert.h>
#include <QUrl>
#include <QDebug>

namespace QmlProfiler {
namespace Internal {

QmlDebug::QmlEventLocation getLocation(const QmlProfilerSimpleModel::QmlEventData &event);
QString getDisplayName(const QmlProfilerSimpleModel::QmlEventData &event);
QString getInitialDetails(const QmlProfilerSimpleModel::QmlEventData &event);

QmlDebug::QmlEventLocation getLocation(const QmlProfilerSimpleModel::QmlEventData &event)
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

QString getDisplayName(const QmlProfilerSimpleModel::QmlEventData &event)
{
    const QmlDebug::QmlEventLocation eventLocation = getLocation(event);
    QString displayName;

    // generate hash
    if (eventLocation.filename.isEmpty()) {
        displayName = QmlProfilerProcessedModel::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(eventLocation.filename).path();
        displayName = filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(eventLocation.line);
    }

    return displayName;
}

QString getInitialDetails(const QmlProfilerSimpleModel::QmlEventData &event)
{
    QString details;
    // generate details string
    if (event.data.isEmpty())
        details = QmlProfilerProcessedModel::tr("Source code not available.");
    else {
        details = event.data.join(QLatin1String(" ")).replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        QRegExp rewrite(QLatin1String("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)"));
        bool match = rewrite.exactMatch(details);
        if (match)
            details = rewrite.cap(1) + QLatin1String(": ") + rewrite.cap(3);
        if (details.startsWith(QLatin1String("file://")))
            details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
    }

    return details;
}


bool compareStartTimes(const QmlProfilerSimpleModel::QmlEventData &t1, const QmlProfilerSimpleModel::QmlEventData &t2)
{
    return t1.startTime < t2.startTime;
}

//////////////////////////////////////////////////////////////////////////////

QmlProfilerProcessedModel::QmlProfilerProcessedModel(Utils::FileInProjectFinder *fileFinder, QmlProfilerModelManager *parent)
    : QmlProfilerSimpleModel(parent)
    , m_detailsRewriter(new QmlProfilerDetailsRewriter(this, fileFinder))
{
    m_processedModelId = m_modelManager->registerModelProxy();
    // The document loading is very expensive.
    m_modelManager->setProxyCountWeight(m_processedModelId, 3);

    connect(m_detailsRewriter, SIGNAL(rewriteDetailsString(int,QString)),
            this, SLOT(detailsChanged(int,QString)));
    connect(m_detailsRewriter, SIGNAL(eventDetailsChanged()),
            this, SLOT(detailsDone()));
}

QmlProfilerProcessedModel::~QmlProfilerProcessedModel()
{
}

void QmlProfilerProcessedModel::clear()
{
    m_detailsRewriter->clearRequests();
    m_modelManager->modelProxyCountUpdated(m_processedModelId, 0, 1);
    // This call emits changed(). Don't emit it again here.
    QmlProfilerSimpleModel::clear();
}

void QmlProfilerProcessedModel::complete()
{
    m_modelManager->modelProxyCountUpdated(m_processedModelId, 0, 1);
    // post-processing

    // sort events by start time
    qSort(eventList.begin(), eventList.end(), compareStartTimes);

    // rewrite strings
    int n = eventList.count();
    for (int i = 0; i < n; i++) {
        QmlEventData *event = &eventList[i];
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

        m_detailsRewriter->requestDetailsForLocation(i, event->location);
        m_modelManager->modelProxyCountUpdated(m_processedModelId, i, n);
    }

    // Allow QmlProfilerBaseModel::complete() only after documents have been reloaded to avoid
    // unnecessary updates of child models.
    m_detailsRewriter->reloadDocuments();
}

void QmlProfilerProcessedModel::detailsChanged(int requestId, const QString &newString)
{
    QTC_ASSERT(requestId < eventList.count(), return);

    QmlEventData *event = &eventList[requestId];
    event->data = QStringList(newString);
}

void QmlProfilerProcessedModel::detailsDone()
{
    m_modelManager->modelProxyCountUpdated(m_processedModelId, 1, 1);
    // The child models are supposed to synchronously update on changed(), triggered by
    // QmlProfilerBaseModel::complete().
    QmlProfilerSimpleModel::complete();
}

}
}
