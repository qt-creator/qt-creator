// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>

#include <functional>

namespace Debugger::Internal {

class DebuggerEngine;

class McpSessionState final : public QObject
{
    Q_OBJECT

public:
    static void startTracking();
    static McpSessionState *forEngine(DebuggerEngine *engine);

    DebuggerEngine *engine() const { return m_engine; }
    QString sessionId() const { return m_sessionId; }
    int stopId() const { return m_stopId; }
    int runFailures() const { return m_runFailures; }

    bool isStopped() const;
    bool hasEnded() const;
    QString currentThreadId() const;
    int currentFrameLevel() const;

    bool isStackReady() const;
    bool isLocalsReady() const;
    int localsUpdates() const { return m_localsUpdates; }

    QJsonObject context() const;
    QJsonObject readiness() const;

signals:
    void changed();

private:
    explicit McpSessionState(DebuggerEngine *engine);

    void handleStateChange();
    void handleStackChanged();
    void handleLocalsFinished();
    void handleThreadsReported();
    QString currentFrameKey() const;

    QPointer<DebuggerEngine> m_engine;
    QString m_sessionId;
    int m_stopId = 0;
    int m_epoch = 0;
    int m_lastState = 0;
    int m_runFailures = 0;
    bool m_runFailed = false;

    int m_stackEpoch = -1;
    int m_stackGeneration = 0;
    QString m_stackThreadId;
    bool m_stackBeforeThreads = false;

    int m_threadsEpoch = -1;

    bool m_localsAborted = false;
    int m_localsUpdates = 0;
    int m_localsEpoch = -1;
    int m_localsStackGeneration = -1;
    QString m_localsFrame;
    class LocalsRequest
    {
    public:
        QString frame;
        QString threadId;
    };
    QList<LocalsRequest> m_localsRequests; // Started, but not finished yet.
    QString m_localsThreadId;
    bool m_localsBeforeThreads = false;
};

enum class WaitOutcome { Ready, TimedOut, SessionEnded };

void waitUntil(McpSessionState *state,
               int timeoutMs,
               const std::function<bool()> &ready,
               const std::function<void(WaitOutcome)> &done);

} // namespace Debugger::Internal
