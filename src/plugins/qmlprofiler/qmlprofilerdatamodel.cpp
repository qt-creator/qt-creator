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
#include <QUrl>
#include <QDebug>
#include <QStack>
#include <QTemporaryFile>
#include <algorithm>

namespace QmlProfiler {

class QmlProfilerDataModel::QmlProfilerDataModelPrivate
{
public:
    void rewriteType(int typeIndex);
    int resolveType(const QmlEventType &type);
    int resolveStackTop();

    QVector<QmlEventType> eventTypes;
    QHash<QmlEventType, int> eventTypeIds;

    QStack<QmlTypedEvent> rangesInProgress;

    QmlProfilerModelManager *modelManager;
    int modelId;
    Internal::QmlProfilerDetailsRewriter *detailsRewriter;

    QTemporaryFile file;
    QDataStream eventStream;
};

QString getDisplayName(const QmlEventType &event)
{
    if (event.location.filename.isEmpty()) {
        return QmlProfilerDataModel::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(event.location.filename).path();
        return filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(event.location.line);
    }
}

QString getInitialDetails(const QmlEventType &event)
{
    QString details;
    // generate details string
    if (!event.data.isEmpty()) {
        details = event.data;
        details = details.replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            if (event.rangeType == Javascript)
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
    } else if (event.rangeType == Painting) {
        // QtQuick1 animations always run in GUI thread.
        details = QmlProfilerDataModel::tr("GUI Thread");
    }

    return details;
}

QString QmlProfilerDataModel::formatTime(qint64 timestamp)
{
    if (timestamp < 1e6)
        return QString::number(timestamp/1e3f,'f',3) + trUtf8(" \xc2\xb5s");
    if (timestamp < 1e9)
        return QString::number(timestamp/1e6f,'f',3) + tr(" ms");

    return QString::number(timestamp/1e9f,'f',3) + tr(" s");
}

QmlProfilerDataModel::QmlProfilerDataModel(Utils::FileInProjectFinder *fileFinder,
                                           QmlProfilerModelManager *parent) :
    QObject(parent), d_ptr(new QmlProfilerDataModelPrivate)
{
    Q_D(QmlProfilerDataModel);
    Q_ASSERT(parent);
    d->modelManager = parent;
    d->detailsRewriter = new QmlProfilerDetailsRewriter(this, fileFinder);
    d->modelId = d->modelManager->registerModelProxy();
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
    delete d->detailsRewriter;
    delete d;
}

const QVector<QmlEventType> &QmlProfilerDataModel::eventTypes() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventTypes;
}

void QmlProfilerDataModel::setData(qint64 traceStart, qint64 traceEnd,
                                   const QVector<QmlEventType> &types,
                                   const QVector<QmlEvent> &events)
{
    Q_D(QmlProfilerDataModel);
    d->modelManager->traceTime()->setTime(traceStart, traceEnd);
    d->eventTypes = types;
    for (int id = 0; id < types.count(); ++id)
        d->eventTypeIds[types[id]] = id;

    foreach (const QmlEvent &event, events) {
        d->modelManager->dispatch(event, d->eventTypes[event.typeIndex()]);
        d->eventStream << event;
    }
}

void QmlProfilerDataModel::clear()
{
    Q_D(QmlProfilerDataModel);
    d->file.remove();
    d->file.open();
    d->eventStream.setDevice(&d->file);
    d->eventTypes.clear();
    d->eventTypeIds.clear();
    d->rangesInProgress.clear();
    d->detailsRewriter->clearRequests();
}

bool QmlProfilerDataModel::isEmpty() const
{
    Q_D(const QmlProfilerDataModel);
    return d->file.pos() == 0;
}

inline static uint qHash(const QmlEventType &type)
{
    return qHash(type.location.filename) ^
            ((type.location.line & 0xfff) |             // 12 bits of line number
            ((type.message << 12) & 0xf000) |           // 4 bits of message
            ((type.location.column << 16) & 0xff0000) | // 8 bits of column
            ((type.rangeType << 24) & 0xf000000) |      // 4 bits of rangeType
            ((type.detailType << 28) & 0xf0000000));    // 4 bits of detailType
}

inline static bool operator==(const QmlEventType &type1,
                              const QmlEventType &type2)
{
    return type1.message == type2.message && type1.rangeType == type2.rangeType &&
            type1.detailType == type2.detailType && type1.location.line == type2.location.line &&
            type1.location.column == type2.location.column &&
            // compare filename last as it's expensive.
            type1.location.filename == type2.location.filename;
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::rewriteType(int typeIndex)
{
    QmlEventType &type = eventTypes[typeIndex];
    type.displayName = getDisplayName(type);
    type.data = getInitialDetails(type);

    // Only bindings and signal handlers need rewriting
    if (type.rangeType != Binding && type.rangeType != HandlingSignal)
        return;

    // There is no point in looking for invalid locations
    if (type.location.filename.isEmpty() || type.location.line < 0 || type.location.column < 0)
        return;

    detailsRewriter->requestDetailsForLocation(typeIndex, type.location);
}

int QmlProfilerDataModel::QmlProfilerDataModelPrivate::resolveType(const QmlEventType &type)
{
    QHash<QmlEventType, int>::ConstIterator it = eventTypeIds.constFind(type);

    int typeIndex = -1;
    if (it != eventTypeIds.constEnd()) {
        typeIndex = it.value();
    } else {
        typeIndex = eventTypes.size();
        eventTypeIds[type] = typeIndex;
        eventTypes.append(type);
        rewriteType(typeIndex);
    }
    return typeIndex;
}

int QmlProfilerDataModel::QmlProfilerDataModelPrivate::resolveStackTop()
{
    if (rangesInProgress.isEmpty())
        return -1;

    QmlTypedEvent &typedEvent = rangesInProgress.top();
    int typeIndex = typedEvent.event.typeIndex();
    if (typeIndex >= 0)
        return typeIndex;

    typeIndex = resolveType(typedEvent.type);
    typedEvent.event.setTypeIndex(typeIndex);
    eventStream << typedEvent.event;
    modelManager->dispatch(typedEvent.event, eventTypes[typeIndex]);
    return typeIndex;
}

void QmlProfilerDataModel::addEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_D(QmlProfilerDataModel);

    // RangeData and RangeLocation always apply to the range on the top of the stack. Furthermore,
    // all ranges are perfectly nested. This is why we can defer the type resolution until either
    // the range ends or a child range starts. With only the information in RangeStart we wouldn't
    // be able to uniquely identify the event type.
    Message rangeStage = type.rangeType == MaximumRangeType ? type.message : event.rangeStage();
    switch (rangeStage) {
    case RangeStart:
        d->resolveStackTop();
        d->rangesInProgress.push(QmlTypedEvent({event, type}));
        break;
    case RangeEnd: {
        int typeIndex = d->resolveStackTop();
        QTC_ASSERT(typeIndex != -1, break);
        QmlEvent appended = event;
        appended.setTypeIndex(typeIndex);
        d->eventStream << appended;
        d->modelManager->dispatch(appended, d->eventTypes[typeIndex]);
        d->rangesInProgress.pop();
        break;
    }
    case RangeData:
        d->rangesInProgress.top().type.data = type.data;
        break;
    case RangeLocation:
        d->rangesInProgress.top().type.location = type.location;
        break;
    default: {
        QmlEvent appended = event;
        int typeIndex = d->resolveType(type);
        appended.setTypeIndex(typeIndex);
        d->eventStream << appended;
        d->modelManager->dispatch(appended, d->eventTypes[typeIndex]);
        break;
    }
    }
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
    while (!stream.atEnd()) {
        stream >> event;
        if (stream.status() == QDataStream::ReadPastEnd)
            break;

        const QmlEventType &type = d->eventTypes[event.typeIndex()];
        if (rangeStart != -1 && rangeEnd != -1) {
            if (event.timestamp() < rangeStart) {
                if (type.rangeType != MaximumRangeType) {
                    if (event.rangeStage() == RangeStart)
                        stack.push(event);
                    else if (event.rangeStage() == RangeEnd)
                        stack.pop();
                }
                continue;
            } else if (event.timestamp() > rangeEnd) {
                if (type.rangeType != MaximumRangeType) {
                    if (event.rangeStage() == RangeEnd) {
                        if (stack.isEmpty()) {
                            QmlEvent endEvent(event);
                            endEvent.setTimestamp(rangeEnd);
                            loader(event, d->eventTypes[event.typeIndex()]);
                        } else {
                            stack.pop();
                        }
                    } else if (event.rangeStage() == RangeStart) {
                        stack.push(event);
                    }
                }
                continue;
            } else if (!stack.isEmpty()) {
                foreach (QmlEvent stashed, stack) {
                    stashed.setTimestamp(rangeStart);
                    loader(stashed, d->eventTypes[stashed.typeIndex()]);
                }
                stack.clear();
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

void QmlProfilerDataModel::detailsChanged(int requestId, const QString &newString)
{
    Q_D(QmlProfilerDataModel);
    QTC_ASSERT(requestId < d->eventTypes.count(), return);
    d->eventTypes[requestId].data = newString;
}

} // namespace QmlProfiler
