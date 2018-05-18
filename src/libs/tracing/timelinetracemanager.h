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

#pragma once

#include "tracing_global.h"
#include "traceevent.h"
#include "traceeventtype.h"

#include <QObject>
#include <QFuture>

#include <functional>
#include <memory>

namespace Timeline {

class TRACING_EXPORT TraceEventTypeStorage
{
public:
    virtual ~TraceEventTypeStorage();
    virtual const TraceEventType &get(int typeId) const = 0;
    virtual void set(int typeId, TraceEventType &&type) = 0;
    virtual int append(TraceEventType &&type) = 0;
    virtual int size() const = 0;
    virtual void clear() = 0;
};

class TRACING_EXPORT TraceEventStorage
{
public:
    virtual ~TraceEventStorage();
    virtual int append(TraceEvent &&event) = 0;
    virtual int size() const = 0;
    virtual void clear() = 0;
    virtual void finalize() = 0;
    virtual bool replay(const std::function<bool(TraceEvent &&)> &receiver) const = 0;
};

class TimelineNotesModel;
class TimelineTraceFile;
class TRACING_EXPORT TimelineTraceManager : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void(const TraceEvent &, const TraceEventType &)> TraceEventLoader;
    typedef std::function<TraceEventLoader(TraceEventLoader)> TraceEventFilter;
    typedef std::function<void()> Initializer;
    typedef std::function<void()> Finalizer;
    typedef std::function<void()> Clearer;
    typedef std::function<void(const QString &)> ErrorHandler;

    explicit TimelineTraceManager(std::unique_ptr<TraceEventStorage> &&eventStorage,
                                  std::unique_ptr<TraceEventTypeStorage> &&typeStorage,
                                  QObject *parent = nullptr);
    ~TimelineTraceManager() override;

    qint64 traceStart() const;
    qint64 traceEnd() const;
    qint64 traceDuration() const;
    void decreaseTraceStart(qint64 start);
    void increaseTraceEnd(qint64 end);

    void setNotesModel(TimelineNotesModel *notesModel);
    TimelineNotesModel *notesModel() const;

    bool isEmpty() const;
    int numEvents() const;
    int numEventTypes() const;

    const TraceEventStorage *eventStorage() const;
    const TraceEventTypeStorage *typeStorage() const;

    void registerFeatures(quint64 features, TraceEventLoader eventLoader,
                          Initializer initializer = Initializer(),
                          Finalizer finalizer = Finalizer(),
                          Clearer clearer = Clearer());

    quint64 availableFeatures() const;
    quint64 visibleFeatures() const;
    void setVisibleFeatures(quint64 features);
    quint64 recordedFeatures() const;
    void setRecordedFeatures(quint64 features);
    bool aggregateTraces() const;
    void setAggregateTraces(bool aggregateTraces);

    virtual void initialize();
    virtual void finalize();
    virtual void clear();

    void clearAll();

    QFuture<void> save(const QString &filename);
    QFuture<void> load(const QString &filename);

signals:
    void error(const QString &error);
    void loadFinished();
    void saveFinished();

    void availableFeaturesChanged(quint64 features);
    void visibleFeaturesChanged(quint64 features);
    void recordedFeaturesChanged(quint64 features);

protected:
    void swapEventStorage(std::unique_ptr<TraceEventStorage> &other);
    virtual void clearEventStorage();
    virtual void clearTypeStorage();

    void restrictByFilter(TraceEventFilter filter);

    void appendEvent(TraceEvent &&event);
    const TraceEventType &eventType(int typeId) const;
    void setEventType(int typeId, TraceEventType &&type);
    int appendEventType(TraceEventType &&type);

    virtual TimelineTraceFile *createTraceFile() = 0;
    virtual void replayEvents(TraceEventLoader loader, Initializer initializer, Finalizer finalizer,
                              ErrorHandler errorHandler, QFutureInterface<void> &future) const = 0;

private:
    class TimelineTraceManagerPrivate;
    TimelineTraceManagerPrivate *d;
};

} // namespace Timeline
