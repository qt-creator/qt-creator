// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindings.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Profiler::Internal {

static QString severityString(Finding::Severity severity)
{
    switch (severity) {
    case Finding::Critical:
        return "critical";
    case Finding::Warning:
        return "warning";
    case Finding::Info:
        break;
    }
    return "info";
}

QJsonObject findingsToJson(const QList<Finding> &findings, qint64 traceStartNs, qint64 traceEndNs)
{
    QJsonArray array;
    for (const Finding &finding : findings) {
        QJsonObject object{
            {"ruleId", finding.ruleId},
            {"severity", severityString(finding.severity)},
            {"file", finding.location.filename()},
            {"what", finding.what},
            {"why", finding.why},
            {"suggestion", finding.suggestion},
            {"costNs", finding.costNs},
            {"occurrences", finding.occurrences},
        };
        // Only report a position where there is one: pixmap events and Compiling ranges
        // carry no line, and the trace reports 0 for them.
        if (finding.location.line() > 0) {
            object.insert("line", finding.location.line());
            object.insert("column", finding.location.column());
        }
        array.append(object);
    }

    return QJsonObject{
        // Consumers outside Qt Creator read this: keep the version explicit from the
        // start, so a later change to the shape is something they can detect.
        {"version", 1},
        {"traceStartNs", traceStartNs},
        {"traceEndNs", traceEndNs},
        {"findings", array},
    };
}

} // namespace Profiler::Internal
