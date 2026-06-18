// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QList>
#include <QMutex>
#include <QString>

#include <qlogging.h>

namespace Mcp::Internal {

// Captures qDebug()/qCInfo()/qCWarning()/... output (including Q_LOGGING_CATEGORY
// categories) into a bounded ring buffer so MCP clients can read the application's
// own log output at runtime. The previous message handler is preserved and chained,
// so all existing routing (GUI log viewer, stderr, ...) keeps working.
class LogCapture
{
public:
    class Entry
    {
    public:
        quint64 index = 0;        // Monotonically increasing position in the global stream.
        qint64 timestamp = 0;     // Milliseconds since epoch.
        QString type;             // "debug", "info", "warning", "critical", "fatal".
        QString category;         // Logging category, e.g. "qtc.remotewindows.device".
        QString message;
    };

    class Result
    {
    public:
        QList<Entry> entries;
        quint64 nextCursor = 0;   // Pass back as sinceCursor to continue reading.
        bool truncated = false;   // True if entries were dropped before sinceCursor.
    };

    static LogCapture &instance();

    // Installs the message handler. Safe to call more than once; only the first
    // call has an effect.
    void install();

    // Returns entries with index >= sinceCursor that are still buffered, optionally
    // filtered by a logging-category prefix, limited to maxLines (most recent if the
    // range is larger than maxLines).
    Result entriesSince(quint64 sinceCursor, const QString &categoryPrefix, int maxLines) const;

private:
    LogCapture() = default;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void append(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    mutable QMutex m_mutex;
    QList<Entry> m_buffer;
    quint64 m_nextIndex = 0;
    QtMessageHandler m_previousHandler = nullptr;
    bool m_installed = false;

    static constexpr int kMaxBuffered = 5000;
};

// Registers the MCP tools that expose the captured log output and runtime logging
// rules (get_application_output, set_logging_rules). Call once during plugin startup.
void registerLogTools();

} // namespace Mcp::Internal
