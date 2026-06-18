// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcplogcapture.h"

#include <mcp/server/toolregistry.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace Mcp::Schema;

namespace Mcp::Internal {

LogCapture &LogCapture::instance()
{
    // Intentionally leaked. The installed message handler is never uninstalled and
    // can fire from any thread (process/SSH reader threads, watchers) during static
    // destruction. Letting the instance live for the whole process avoids returning
    // a dangling reference and locking a destroyed mutex / mutating a destroyed list.
    static LogCapture *theInstance = new LogCapture;
    return *theInstance;
}

void LogCapture::install()
{
    QMutexLocker locker(&m_mutex);
    if (m_installed)
        return;
    m_installed = true;
    m_previousHandler = qInstallMessageHandler(&LogCapture::messageHandler);
}

static QString typeToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "debug";
    case QtInfoMsg:
        return "info";
    case QtWarningMsg:
        return "warning";
    case QtCriticalMsg:
        return "critical";
    case QtFatalMsg:
        return "fatal";
    }
    return "unknown";
}

void LogCapture::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    LogCapture &self = instance();
    self.append(type, context, msg);

    // Chain to the previously installed handler so existing routing keeps working.
    QtMessageHandler previous = nullptr;
    {
        QMutexLocker locker(&self.m_mutex);
        previous = self.m_previousHandler;
    }
    if (previous)
        previous(type, context, msg);
}

void LogCapture::append(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Entry entry;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.type = typeToString(type);
    entry.category = QString::fromUtf8(context.category ? context.category : "default");
    entry.message = msg;

    QMutexLocker locker(&m_mutex);
    entry.index = m_nextIndex++;
    m_buffer.append(entry);
    if (m_buffer.size() > kMaxBuffered)
        m_buffer.remove(0, m_buffer.size() - kMaxBuffered);
}

LogCapture::Result LogCapture::entriesSince(
    quint64 sinceCursor, const QString &categoryPrefix, int maxLines) const
{
    QMutexLocker locker(&m_mutex);

    Result result;
    result.nextCursor = m_nextIndex;

    if (m_buffer.isEmpty())
        return result;

    const quint64 firstBuffered = m_buffer.first().index;
    if (sinceCursor < firstBuffered)
        result.truncated = true;

    for (const Entry &entry : m_buffer) {
        if (entry.index < sinceCursor)
            continue;
        if (!categoryPrefix.isEmpty() && !entry.category.startsWith(categoryPrefix))
            continue;
        result.entries.append(entry);
    }

    if (maxLines > 0 && result.entries.size() > maxLines)
        result.entries = result.entries.mid(result.entries.size() - maxLines);

    return result;
}

void registerLogTools()
{
    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    const auto wrap = [](const SimplifiedCallback &cb) -> Server::ToolCallback {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("get_application_output")
            .title("Read application log output")
            .description(
                "Returns recent log output captured from Qt Creator's qDebug()/qCInfo()/"
                "qCWarning()/... stream, including Q_LOGGING_CATEGORY output. Read incrementally "
                "by passing the 'cursor' returned by the previous call as 'sinceCursor'. Optionally "
                "filter by a logging-category prefix, e.g. 'qtc.remotewindows'.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "category",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Only return lines whose logging category starts with this prefix "
                             "(optional)."}})
                    .addProperty(
                        "sinceCursor",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Return only lines newer than this cursor. Use the 'cursor' from a "
                             "previous response. Omit or 0 to read from the start of the buffer."}})
                    .addProperty(
                        "maxLines",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Maximum number of lines to return (most recent within the range). "
                             "Defaults to 200."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("lines", QJsonObject{{"type", "array"}})
                    .addProperty("cursor", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("lines")
                    .addRequired("cursor")),
        wrap([](const QJsonObject &p) {
            const QString category = p.value("category").toString();
            const quint64 sinceCursor = quint64(p.value("sinceCursor").toDouble(0));
            const int maxLines = p.value("maxLines").toInt(200);

            const LogCapture::Result res
                = LogCapture::instance().entriesSince(sinceCursor, category, maxLines);

            QJsonArray lines;
            for (const LogCapture::Entry &entry : res.entries) {
                lines.append(QJsonObject{
                    {"index", double(entry.index)},
                    {"timestamp", double(entry.timestamp)},
                    {"type", entry.type},
                    {"category", entry.category},
                    {"message", entry.message},
                });
            }
            return QJsonObject{
                {"lines", lines},
                {"cursor", double(res.nextCursor)},
                {"truncated", res.truncated},
            };
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("set_logging_rules")
            .title("Set runtime logging rules")
            .description(
                "Applies QLoggingCategory filter rules at runtime, equivalent to QT_LOGGING_RULES, "
                "e.g. 'qtc.remotewindows.*=true'. Separate multiple rules with newlines. Use this "
                "to enable a logging category before reading it back with get_application_output.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "rules",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Logging rules, one per line, e.g. 'qtc.*.debug=true'."}})
                    .addRequired("rules"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            QLoggingCategory::setFilterRules(p.value("rules").toString());
            return QJsonObject{{"success", true}};
        }));
}

} // namespace Mcp::Internal
