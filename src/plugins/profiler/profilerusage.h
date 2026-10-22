// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"
#include "traceformat.h"

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

#include <chrono>

namespace Profiler::Internal {

// Where the profiler says what is being used, without knowing who listens. The
// standalone viewer hands the events to its own tracker, and Qt Creator's
// telemetry plugin picks them up from the plugin manager's object pool under
// the object name "ProfilerUsage". With nobody connected, reporting does nothing.
//
// Events carry no paths, targets or command lines: only which backend and trace
// format were used, and whether a recording worked.
class PROFILER_EXPORT ProfilerUsage : public QObject
{
    Q_OBJECT

public:
    static ProfilerUsage *instance();

    static void recordingStarted(const QString &backend);
    static void recordingFinished(const QString &backend, bool success,
                                  std::chrono::milliseconds recordedTime);
    static void traceLoaded(TraceFormat format);

    // Which screen the user is on, for following the flow through the profiler:
    // the page to start a recording, the running recording, a trace and, within
    // a trace, the view that is in front. Reporting the screen already shown is
    // a no-op.
    static void welcomeShown();
    static void recordingShown();
    static void traceShown(TraceFormat format);
    static void viewShown(TraceFormat format, const QWidget *view);

    // The last screen reported, or empty. For a listener that connects late.
    Q_INVOKABLE QString currentScreen() const { return m_currentScreen; }

signals:
    // `screen` names a state in the flow, such as ":PROFILER:Trace:ctf".
    void usageTransition(const QString &screen);
    // `data` is a compact JSON object.
    void usageEvent(const QString &key, const QString &data);

private:
    ProfilerUsage();
    static void showScreen(const QString &name);

    QString m_currentScreen;
};

} // namespace Profiler::Internal
