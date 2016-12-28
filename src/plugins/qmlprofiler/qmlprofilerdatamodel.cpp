/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerdatamodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilerdetailsrewriter.h"
#include "qmlprofilereventtypes.h"
#include "qmltypedevent.h"

#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QUrl>
#include <QDebug>
#include <QStack>
#include <algorithm>

namespace QmlProfiler {

class QmlProfilerDataModel::QmlProfilerDataModelPrivate
{
public:
    QmlProfilerDataModelPrivate() : file("qmlprofiler-data") { }
    void rewriteType(int typeIndex);
    int resolveStackTop();

    QVector<QmlEventType> eventTypes;
    Internal::QmlProfilerDetailsRewriter *detailsRewriter;

    Utils::TemporaryFile file;
    QDataStream eventStream;
};

QString getDisplayName(const QmlEventType &event)
{
    if (event.location().filename().isEmpty()) {
        return QmlProfilerDataModel::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(event.location().filename()).path();
        return filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(event.location().line());
    }
}

QString getInitialDetails(const QmlEventType &event)
{
    QString details = event.data();
    // generate details string
    if (!details.isEmpty()) {
        details = details.replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            if (event.rangeType() == Javascript)
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
    } else if (event.rangeType() == Painting) {
        // QtQuick1 animations always run in GUI thread.
        details = QmlProfilerDataModel::tr("GUI Thread");
    }

    return details;
}

QmlProfilerDataModel::QmlProfilerDataModel(QObject *parent) :
    QObject(parent), d_ptr(new QmlProfilerDataModelPrivate)
{
    Q_D(QmlProfilerDataModel);
    Q_ASSERT(parent);
    d->detailsRewriter = new QmlProfilerDetailsRewriter(this);
    connect(d->detailsRewriter, &QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, &QmlProfilerDataModel::detailsChanged);
    connect(d->detailsRewriter, &QmlProfilerDetailsRewriter::eventDetailsChanged,
            this, &QmlProfilerDataModel::allTypesLoaded);
    d->file.open();
    d->eventStream.setDevice(&d->file);
}

QmlProfilerDataModel::~QmlProfilerDataModel()
{
    Q_D(QmlProfilerDataModel);
    delete d;
}

const QmlEventType &QmlProfilerDataModel::eventType(int typeId) const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventTypes.at(typeId);
}

const QVector<QmlEventType> &QmlProfilerDataModel::eventTypes() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventTypes;
}

void QmlProfilerDataModel::addEventTypes(const QVector<QmlEventType> &types)
{
    Q_D(QmlProfilerDataModel);
    int typeIndex = d->eventTypes.length();
    d->eventTypes.append(types);
    for (const int end = d->eventTypes.length(); typeIndex < end; ++typeIndex)
        d->rewriteType(typeIndex);
}

void QmlProfilerDataModel::addEventType(const QmlEventType &type)
{
    Q_D(QmlProfilerDataModel);
    int typeIndex = d->eventTypes.length();
    d->eventTypes.append(type);
    d->rewriteType(typeIndex);
}

void QmlProfilerDataModel::populateFileFinder(
        const ProjectExplorer::RunConfiguration *runConfiguration)
{
    Q_D(QmlProfilerDataModel);
    d->detailsRewriter->populateFileFinder(runConfiguration);
}

QString QmlProfilerDataModel::findLocalFile(const QString &remoteFile)
{
    Q_D(QmlProfilerDataModel);
    return d->detailsRewriter->getLocalFile(remoteFile);
}

void QmlProfilerDataModel::addEvent(const QmlEvent &event)
{
    Q_D(QmlProfilerDataModel);
    d->eventStream << event;
}

void QmlProfilerDataModel::addEvents(const QVector<QmlEvent> &events)
{
    Q_D(QmlProfilerDataModel);
    for (const QmlEvent &event : events)
        d->eventStream << event;
}

void QmlProfilerDataModel::clear()
{
    Q_D(QmlProfilerDataModel);
    d->file.remove();
    d->file.open();
    d->eventStream.setDevice(&d->file);
    d->eventTypes.clear();
    d->detailsRewriter->clearRequests();
}

bool QmlProfilerDataModel::isEmpty() const
{
    Q_D(const QmlProfilerDataModel);
    return d->file.pos() == 0;
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::rewriteType(int typeIndex)
{
    QmlEventType &type = eventTypes[typeIndex];
    type.setDisplayName(getDisplayName(type));
    type.setData(getInitialDetails(type));

    // Only bindings and signal handlers need rewriting
    if (type.rangeType() != Binding && type.rangeType() != HandlingSignal)
        return;

    // There is no point in looking for invalid locations
    if (!type.location().isValid())
        return;

    detailsRewriter->requestDetailsForLocation(typeIndex, type.location());
}

static bool isStateful(const QmlEventType &type)
{
    // Events of these types carry state that has to be taken into account when adding later events:
    // PixmapCacheEvent: Total size of the cache and size of pixmap currently being loaded
    // MemoryAllocation: Total size of the JS heap and the amount of it currently in use
    const Message message = type.message();
    return message == PixmapCacheEvent || message == MemoryAllocation;
}

void QmlProfilerDataModel::replayEvents(qint64 rangeStart, qint64 rangeEnd,
                                        QmlProfilerModelManager::EventLoader loader) const
{
    Q_D(const QmlProfilerDataModel);
    QStack<QmlEvent> stack;
    QmlEvent event;
    QFile file(d->file.fileName());
    file.open(QIODevice::ReadOnly);
    QDataStream stream(&file);
    bool crossedRangeStart = false;
    while (!stream.atEnd()) {
        stream >> event;
        if (stream.status() == QDataStream::ReadPastEnd)
            break;

        const QmlEventType &type = d->eventTypes[event.typeIndex()];
        if (rangeStart != -1 && rangeEnd != -1) {
            // Double-check if rangeStart has been crossed. Some versions of Qt send dirty data.
            if (event.timestamp() < rangeStart && !crossedRangeStart) {
                if (type.rangeType() != MaximumRangeType) {
                    if (event.rangeStage() == RangeStart)
                        stack.push(event);
                    else if (event.rangeStage() == RangeEnd)
                        stack.pop();
                    continue;
                } else if (isStateful(type)) {
                    event.setTimestamp(rangeStart);
                } else {
                    continue;
                }
            } else {
                if (!crossedRangeStart) {
                    foreach (QmlEvent stashed, stack) {
                        stashed.setTimestamp(rangeStart);
                        loader(stashed, d->eventTypes[stashed.typeIndex()]);
                    }
                    stack.clear();
                    crossedRangeStart = true;
                }
                if (event.timestamp() > rangeEnd) {
                    if (type.rangeType() != MaximumRangeType) {
                        if (event.rangeStage() == RangeEnd) {
                            if (stack.isEmpty()) {
                                QmlEvent endEvent(event);
                                endEvent.setTimestamp(rangeEnd);
                                loader(endEvent, d->eventTypes[event.typeIndex()]);
                            } else {
                                stack.pop();
                            }
                        } else if (event.rangeStage() == RangeStart) {
                            stack.push(event);
                        }
                        continue;
                    } else if (isStateful(type)) {
                        event.setTimestamp(rangeEnd);
                    } else {
                        continue;
                    }
                }
            }
        }

        loader(event, type);
    }
}

void QmlProfilerDataModel::finalize()
{
    Q_D(QmlProfilerDataModel);
    d->file.flush();
    d->detailsRewriter->reloadDocuments();
}

void QmlProfilerDataModel::detailsChanged(int typeId, const QString &newString)
{
    Q_D(QmlProfilerDataModel);
    QTC_ASSERT(typeId < d->eventTypes.count(), return);
    d->eventTypes[typeId].setData(newString);
}

} // namespace QmlProfiler
