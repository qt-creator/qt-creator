// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilerusage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>

using namespace Qt::StringLiterals;

namespace Profiler::Internal {

static void report(const QString &key, const QJsonObject &data)
{
    emit ProfilerUsage::instance()->usageEvent(
        key, QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
}

static QString formatName(TraceFormat format)
{
    switch (format) {
    case TraceFormat::Qml: return u"qml"_s;
    case TraceFormat::Perf: return u"perf"_s;
    case TraceFormat::Ctf: return u"ctf"_s;
    case TraceFormat::Sampler: return u"sampler"_s;
    case TraceFormat::Combined: return u"combined"_s;
    }
    return u"unknown"_s;
}

void ProfilerUsage::showScreen(const QString &name)
{
    ProfilerUsage *usage = instance();
    if (usage->m_currentScreen == name)
        return;
    usage->m_currentScreen = name;
    emit usage->usageTransition(name);
}

ProfilerUsage *ProfilerUsage::instance()
{
    static ProfilerUsage usage;
    return &usage;
}

ProfilerUsage::ProfilerUsage()
{
    setObjectName("ProfilerUsage");
}

void ProfilerUsage::recordingStarted(const QString &backend)
{
    report(u"ProfilerRecordingStarted"_s, {{u"backend"_s, backend}});
}

void ProfilerUsage::recordingFinished(const QString &backend, bool success,
                                      std::chrono::milliseconds recordedTime)
{
    report(u"ProfilerRecordingFinished"_s,
           {{u"backend"_s, backend},
            {u"success"_s, success},
            {u"seconds"_s, int(std::chrono::duration_cast<std::chrono::seconds>(recordedTime).count())}});
}

void ProfilerUsage::traceLoaded(TraceFormat format)
{
    report(u"ProfilerTraceLoaded"_s, {{u"format"_s, formatName(format)}});
}

void ProfilerUsage::welcomeShown()
{
    showScreen(u":PROFILER:Welcome"_s);
}

void ProfilerUsage::recordingShown()
{
    showScreen(u":PROFILER:Recording"_s);
}

void ProfilerUsage::traceShown(TraceFormat format)
{
    showScreen(u":PROFILER:Trace:"_s + formatName(format));
}

void ProfilerUsage::viewShown(TraceFormat format, const QWidget *view)
{
    if (!view)
        return;
    // The object name is not translated, unlike the title. Not every view has one.
    const QString name = view->objectName().isEmpty() ? QString::fromLatin1(view->metaObject()->className())
                                                      : view->objectName();
    showScreen(u":PROFILER:Trace:"_s + formatName(format) + u':' + name);
}

} // namespace Profiler::Internal
