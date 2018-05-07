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

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertracefile.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilerdetailsrewriter.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <tracing/tracestashfile.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QRegExp>
#include <QStack>

#include <functional>

namespace QmlProfiler {

static const char *ProfileFeatureNames[] = {
    QT_TRANSLATE_NOOP("MainView", "JavaScript"),
    QT_TRANSLATE_NOOP("MainView", "Memory Usage"),
    QT_TRANSLATE_NOOP("MainView", "Pixmap Cache"),
    QT_TRANSLATE_NOOP("MainView", "Scene Graph"),
    QT_TRANSLATE_NOOP("MainView", "Animations"),
    QT_TRANSLATE_NOOP("MainView", "Painting"),
    QT_TRANSLATE_NOOP("MainView", "Compiling"),
    QT_TRANSLATE_NOOP("MainView", "Creating"),
    QT_TRANSLATE_NOOP("MainView", "Binding"),
    QT_TRANSLATE_NOOP("MainView", "Handling Signal"),
    QT_TRANSLATE_NOOP("MainView", "Input Events"),
    QT_TRANSLATE_NOOP("MainView", "Debug Messages")
};

Q_STATIC_ASSERT(sizeof(ProfileFeatureNames) == sizeof(char *) * MaximumProfileFeature);

class QmlProfilerModelManager::QmlProfilerModelManagerPrivate
{
public:
    QmlProfilerModelManagerPrivate() : file("qmlprofiler-data") {}

    Internal::QmlProfilerTextMarkModel *textMarkModel = nullptr;

    QVector<QmlEventType> eventTypes;
    Internal::QmlProfilerDetailsRewriter *detailsRewriter = nullptr;

    Timeline::TraceStashFile<QmlEvent> file;

    bool isRestrictedToRange = false;

    void writeToStream(const QmlEvent &event);
    void addEventType(const QmlEventType &eventType);
    void handleError(const QString &message);

    void rewriteType(int typeIndex);
    int resolveStackTop();
};

QmlProfilerModelManager::QmlProfilerModelManager(QObject *parent) :
    Timeline::TimelineTraceManager(parent), d(new QmlProfilerModelManagerPrivate)
{
    setNotesModel(new QmlProfilerNotesModel(this));
    d->textMarkModel = new Internal::QmlProfilerTextMarkModel(this);

    d->detailsRewriter = new Internal::QmlProfilerDetailsRewriter(this);
    connect(d->detailsRewriter, &Internal::QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, &QmlProfilerModelManager::typeDetailsChanged);
    connect(d->detailsRewriter, &Internal::QmlProfilerDetailsRewriter::eventDetailsChanged,
            this, &QmlProfilerModelManager::typeDetailsFinished);

    if (!d->file.open())
        emit error(tr("Cannot open temporary trace file to store events."));

    quint64 allFeatures = 0;
    for (quint8 i = 0; i <= MaximumProfileFeature; ++i)
        allFeatures |= (1ull << i);
}

QmlProfilerModelManager::~QmlProfilerModelManager()
{
    delete d;
}

Internal::QmlProfilerTextMarkModel *QmlProfilerModelManager::textMarkModel() const
{
    return d->textMarkModel;
}

void QmlProfilerModelManager::registerFeatures(quint64 features, QmlEventLoader eventLoader,
                                               Initializer initializer, Finalizer finalizer,
                                               Clearer clearer)
{
    const TraceEventLoader traceEventLoader = eventLoader ? [eventLoader](
            const Timeline::TraceEvent &event, const Timeline::TraceEventType &type) {
        return eventLoader(static_cast<const QmlEvent &>(event),
                           static_cast<const QmlEventType &>(type));
    } : TraceEventLoader();

    Timeline::TimelineTraceManager::registerFeatures(features, traceEventLoader, initializer,
                                                     finalizer, clearer);
}

void QmlProfilerModelManager::addEvents(const QVector<QmlEvent> &events)
{
    for (const QmlEvent &event : events)
        addEvent(event);
}

void QmlProfilerModelManager::addEventTypes(const QVector<QmlEventType> &types)
{
    for (const QmlEventType &type : types) {
        d->addEventType(type);
        TimelineTraceManager::addEventType(type);
    }
}

const QmlEventType &QmlProfilerModelManager::eventType(int typeId) const
{
    return d->eventTypes.at(typeId);
}

void QmlProfilerModelManager::replayEvents(qint64 rangeStart, qint64 rangeEnd,
                                           TraceEventLoader loader, Initializer initializer,
                                           Finalizer finalizer, ErrorHandler errorHandler,
                                           QFutureInterface<void> &future) const
{
    replayQmlEvents(rangeStart, rangeEnd, static_cast<QmlEventLoader>(loader), initializer,
                    finalizer, errorHandler, future);
}

static bool isStateful(const QmlEventType &type)
{
    // Events of these types carry state that has to be taken into account when adding later events:
    // PixmapCacheEvent: Total size of the cache and size of pixmap currently being loaded
    // MemoryAllocation: Total size of the JS heap and the amount of it currently in use
    const Message message = type.message();
    return message == PixmapCacheEvent || message == MemoryAllocation;
}

void QmlProfilerModelManager::replayQmlEvents(qint64 rangeStart, qint64 rangeEnd,
                                              QmlEventLoader loader, Initializer initializer,
                                              Finalizer finalizer, ErrorHandler errorHandler,
                                              QFutureInterface<void> &future) const
{
    if (initializer)
        initializer();

    QStack<QmlEvent> stack;
    bool crossedRangeStart = false;

    const auto result = d->file.replay([&](const QmlEvent &event) {
        if (future.isCanceled())
            return false;

        const QmlEventType &type = d->eventTypes[event.typeIndex()];

        // No restrictions: load all events
        if (rangeStart == -1 || rangeEnd == -1) {
            loader(event, type);
            return true;
        }

        // Double-check if rangeStart has been crossed. Some versions of Qt send dirty data.
        qint64 adjustedTimestamp = event.timestamp();
        if (event.timestamp() < rangeStart && !crossedRangeStart) {
            if (type.rangeType() != MaximumRangeType) {
                if (event.rangeStage() == RangeStart)
                    stack.push(event);
                else if (event.rangeStage() == RangeEnd)
                    stack.pop();
                return true;
            } else if (isStateful(type)) {
                adjustedTimestamp = rangeStart;
            } else {
                return true;
            }
        } else {
            if (!crossedRangeStart) {
                for (QmlEvent stashed : stack) {
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
                    return true;
                } else if (isStateful(type)) {
                    adjustedTimestamp = rangeEnd;
                } else {
                    return true;
                }
            }
        }

        if (adjustedTimestamp != event.timestamp()) {
            QmlEvent adjusted(event);
            adjusted.setTimestamp(adjustedTimestamp);
            loader(adjusted, type);
        } else {
            loader(event, type);
        }
        return true;
    });

    switch (result) {
    case Timeline::TraceStashFile<QmlEvent>::ReplaySuccess:
        if (finalizer)
            finalizer();
        break;
    case Timeline::TraceStashFile<QmlEvent>::ReplayOpenFailed:
        if (errorHandler)
            errorHandler(tr("Could not re-open temporary trace file"));
        break;
    case Timeline::TraceStashFile<QmlEvent>::ReplayLoadFailed:
        if (errorHandler)
            errorHandler(tr("Could not load events from temporary trace file"));
        break;
    case Timeline::TraceStashFile<QmlEvent>::ReplayReadPastEnd:
        if (errorHandler)
            errorHandler(tr("Read past end in temporary trace file"));
        break;
    }
}

static QString getDisplayName(const QmlEventType &event)
{
    if (event.location().filename().isEmpty()) {
        return QmlProfilerModelManager::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(event.location().filename()).path();
        return filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(event.location().line());
    }
}

static QString getInitialDetails(const QmlEventType &event)
{
    QString details = event.data();
    // generate details string
    if (!details.isEmpty()) {
        details = details.replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            if (event.rangeType() == Javascript)
                details = QmlProfilerModelManager::tr("anonymous function");
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

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::writeToStream(const QmlEvent &event)
{
    file.append(event);
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::addEventType(const QmlEventType &type)
{
    const int typeId = eventTypes.count();
    eventTypes.append(type);
    rewriteType(typeId);
    const QmlEventLocation &location = type.location();
    if (location.isValid()) {
        textMarkModel->addTextMarkId(typeId, QmlEventLocation(
                                         detailsRewriter->getLocalFile(location.filename()),
                                         location.line(), location.column()));
    }
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::handleError(const QString &message)
{
    // What to do here?
    qWarning() << message;
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::rewriteType(int typeIndex)
{
    QmlEventType &type = eventTypes[typeIndex];
    type.setDisplayName(getDisplayName(type));
    type.setData(getInitialDetails(type));

    const QmlEventLocation &location = type.location();
    // There is no point in looking for invalid locations
    if (!location.isValid())
        return;

    // Only bindings and signal handlers need rewriting
    if (type.rangeType() == Binding || type.rangeType() == HandlingSignal)
        detailsRewriter->requestDetailsForLocation(typeIndex, location);
}

const char *QmlProfilerModelManager::featureName(ProfileFeature feature)
{
    return ProfileFeatureNames[feature];
}

void QmlProfilerModelManager::finalize()
{
    if (!d->file.flush())
        emit error(tr("Failed to flush temporary trace file"));
    d->detailsRewriter->reloadDocuments();

    // Load notes after the timeline models have been initialized ...
    // which happens on stateChanged(Done).

    TimelineTraceManager::finalize();
}

void QmlProfilerModelManager::populateFileFinder(const ProjectExplorer::Target *target)
{
    d->detailsRewriter->populateFileFinder(target);
}

QString QmlProfilerModelManager::findLocalFile(const QString &remoteFile)
{
    return d->detailsRewriter->getLocalFile(remoteFile);
}

void QmlProfilerModelManager::detailsChanged(int typeId, const QString &newString)
{
    QTC_ASSERT(typeId < d->eventTypes.count(), return);
    d->eventTypes[typeId].setData(newString);
    emit typeDetailsChanged(typeId);
}

const Timeline::TraceEventType &QmlProfilerModelManager::lookupType(int typeIndex) const
{
    return eventType(typeIndex);
}

void QmlProfilerModelManager::clearEventStorage()
{
    TimelineTraceManager::clearEventStorage();
    d->file.clear();
    if (!d->file.open())
        emit error(tr("Failed to reset temporary trace file"));
}

void QmlProfilerModelManager::clearTypeStorage()
{
    TimelineTraceManager::clearTypeStorage();
    d->eventTypes.clear();
}

void QmlProfilerModelManager::addEventType(const QmlEventType &type)
{
    d->addEventType(type);
    TimelineTraceManager::addEventType(type);
}

void QmlProfilerModelManager::addEvent(const QmlEvent &event)
{
    d->writeToStream(event);
    TimelineTraceManager::addEvent(event);
}

void QmlProfilerModelManager::restrictToRange(qint64 start, qint64 end)
{
    d->isRestrictedToRange = (start != -1 || end != -1);
    TimelineTraceManager::restrictToRange(start, end);
}

bool QmlProfilerModelManager::isRestrictedToRange() const
{
    return d->isRestrictedToRange;
}

Timeline::TimelineTraceFile *QmlProfilerModelManager::createTraceFile()
{
    return new Internal::QmlProfilerTraceFile(this);
}

} // namespace QmlProfiler
