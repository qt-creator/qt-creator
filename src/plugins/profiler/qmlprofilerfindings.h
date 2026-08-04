// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventlocation.h>
#include <qmldebug/qmleventtype.h>

#include <QJsonObject>
#include <QList>
#include <QString>

#include <memory>

namespace Profiler::Internal {

class QmlProfilerModelManager;

// A single actionable observation about the profiled application, derived from the trace
// by a FindingRule. Kept plain and copyable so it can be serialized without the model.
class Finding
{
public:
    enum Severity {
        Info,
        Warning,
        Critical
    };

    QString ruleId;
    Severity severity = Warning;
    QmlDebug::QmlEventLocation location;
    int typeIndex = -1; // -1 when the finding has no single originating event type
    QString what;
    QString why;
    QString suggestion;
    qint64 costNs = 0;
    int occurrences = 0;
};

// Rules see every event of the features they ask for, in trace order, and turn what they
// collected into findings once the trace has been replayed completely.
class FindingRule
{
public:
    virtual ~FindingRule() = default;

    // Features this rule needs. The model registers the union of all rules' features.
    virtual quint64 features() const = 0;

    virtual void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) = 0;
    virtual QList<Finding> finalize(const QmlProfilerModelManager *manager) = 0;
    virtual void clear() = 0;
};

using FindingRules = std::vector<std::unique_ptr<FindingRule>>;

FindingRules defaultFindingRules();

// Serializes findings for consumers outside the views: the shape is a contract, so it
// carries an explicit version.
QJsonObject findingsToJson(const QList<Finding> &findings, qint64 traceStartNs,
                           qint64 traceEndNs);

} // namespace Profiler::Internal
