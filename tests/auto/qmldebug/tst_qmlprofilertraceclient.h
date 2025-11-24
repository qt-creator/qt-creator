// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmlprofilertraceclient.h>
#include <tracing/tracestashfile.h>

#include <QObject>
#include <QFutureInterface>

class ModelManager
{
public:
    typedef std::function<void()> Initializer;
    typedef std::function<void()> Finalizer;
    typedef std::function<void(const QString &)> ErrorHandler;

    ModelManager();

    using QmlEventLoader
        = std::function<void (const QmlDebug::QmlEvent &, const QmlDebug::QmlEventType &)>;

    int appendEventType(QmlDebug::QmlEventType &&type);
    void appendEvent(QmlDebug::QmlEvent &&event);

    void replayQmlEvents(QmlEventLoader loader, Finalizer finalizer,
                         ErrorHandler errorHandler) const;

    void clearAll();
private:
    Timeline::TraceStashFile<QmlDebug::QmlEvent> events;
    QList<QmlDebug::QmlEventType> types;
};

class QmlProfilerTraceClientTest : public QObject
{
    Q_OBJECT

public:
    static void initMain();
    QmlProfilerTraceClientTest(QObject *parent = nullptr);

private slots:
    void testMessageReceived();

private:
    ModelManager modelManager;
    QmlDebug::QmlProfilerTraceClient traceClient;
};

