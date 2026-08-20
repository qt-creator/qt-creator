// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertracedocument.h"

#include "combinedsampler.h"
#include "ctftracebackend.h"
#include "perfprofilertracebackend.h"
#include "profilertr.h"
#include "profilertracebackend.h"
#include "qmlprofilertracebackend.h"
#include "samplertracebackend.h"

#include "qmlprofilertool.h"

#include <coreplugin/editormanager/editormanager.h>

#include <tracing/rangedetailswidget.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QMessageBox>

using namespace Utils;

namespace Profiler::Internal {

ProfilerTraceDocument::ProfilerTraceDocument(Id editorId, TraceFormat format)
    : m_format(format)
    , m_rangeDetails(new Timeline::RangeDetailsWidget)
{
    setId(editorId);

    switch (format) {
    case TraceFormat::Qml:
        m_backends << new QmlProfilerTraceBackend(m_rangeDetails, this);
        break;
    case TraceFormat::Perf:
        m_backends << new PerfProfilerTraceBackend(m_rangeDetails, this);
        break;
    case TraceFormat::Ctf:
        m_backends << new CtfTraceBackend(m_rangeDetails, this);
        break;
    case TraceFormat::Sampler:
        m_backends << new SamplerTraceBackend(m_rangeDetails, this);
        break;
    case TraceFormat::Combined:
        // A combined bundle holds a QML trace and a native-mixed sampler trace,
        // and shows both view sets at once.
        m_backends << new QmlProfilerTraceBackend(m_rangeDetails, this);
        m_backends << new SamplerTraceBackend(m_rangeDetails, this);
        break;
    }

    for (ProfilerTraceBackend *backend : std::as_const(m_backends)) {
        connect(backend, &ProfilerTraceBackend::traceChanged, this, &IDocument::changed);
        connect(backend, &ProfilerTraceBackend::loadFinished, this, [this] {
            emit busyChanged(false);
            emit changed();
        });
        connect(backend, &ProfilerTraceBackend::error, this, [](const QString &message) {
            QmlProfilerTool::showNonmodalWarning(message);
        });
        connect(backend, &ProfilerTraceBackend::gotoSourceLocation,
                this, [](const Link &link) {
            Core::EditorManager::openEditorAt(link, {},
                                              Core::EditorManager::DoNotSwitchToDesignMode
                                                  | Core::EditorManager::DoNotSwitchToEditMode);
        });
    }

    if (auto qml = qobject_cast<QmlProfilerTraceBackend *>(m_backends.first())) {
        connect(qml, &QmlProfilerTraceBackend::busyChanged, this,
                &ProfilerTraceDocument::busyChanged);
        connect(qml, &QmlProfilerTraceBackend::saved, this, &IDocument::changed);
        // A new recording is about to discard notes the user has not saved. The
        // session cannot be paused to ask, so offer to save rather than to cancel.
        connect(qml, &QmlProfilerTraceBackend::saveBeforeRecordingRequested, this, [this] {
            const auto answer = QMessageBox::warning(
                QApplication::activeWindow(), Tr::tr("QML Profiler"),
                Tr::tr("Starting a new profiling session will discard the previous data, "
                       "including unsaved notes.\nDo you want to save the data first?"),
                QMessageBox::Save, QMessageBox::Discard);
            if (answer == QMessageBox::Save)
                Core::EditorManager::saveDocument(this);
        });
    }
}

ProfilerTraceDocument::~ProfilerTraceDocument()
{
    // The editor's widget takes the details panel over when it lays its views
    // out. If no editor ever did, it is still ours to delete.
    if (m_rangeDetails && !m_rangeDetails->parentWidget())
        delete m_rangeDetails;
}

Result<> ProfilerTraceDocument::open(const FilePath &filePath, const FilePath &realFilePath)
{
    setFilePath(filePath);
    load(realFilePath);
    return ResultOk;
}

void ProfilerTraceDocument::load(const FilePath &rawPath)
{
    emit busyChanged(true);
    m_rangeDetails->reset(); // Don't carry a previous trace's details over.

    // Opening a directory-based trace by the file that names it hands us that
    // file; the loaders want the directory.
    const FilePath path = identifyTrace(rawPath).path;

    if (m_format == TraceFormat::Combined) {
        // The QML views read the bundle's own .qtd; the sampler views wait for
        // the merged native-mixed trace (see ProfilerTraceEditor).
        m_backends.first()->load(path / combinedQmlFileName);
        return;
    }
    m_backends.first()->load(path);
}

Result<> ProfilerTraceDocument::setContents(const QByteArray &contents)
{
    if (!contents.isEmpty())
        return ResultError(Tr::tr("A trace cannot be filled from memory."));
    return ResultOk;
}

Result<> ProfilerTraceDocument::saveImpl(const FilePath &filePath, SaveOption option)
{
    Q_UNUSED(option)
    ProfilerTraceBackend *backend = m_backends.first();
    if (!backend->isSaveable())
        return ResultError(Tr::tr("This trace format cannot be saved."));

    const FilePath target = filePath.isEmpty() ? this->filePath() : filePath;
    if (target.isEmpty())
        return ResultError(Tr::tr("No file name to save the trace to."));

    // A combined bundle's only modifiable state is the notes in its QML half,
    // which go back into the bundle's own file; the document keeps pointing at
    // the bundle. Writing a copy would have to clone the sampler half too, so
    // there is no Save As (see isSaveAsAllowed()).
    if (m_format == TraceFormat::Combined) {
        if (identifyTrace(target).path != identifyTrace(this->filePath()).path)
            return ResultError(Tr::tr("A combined trace cannot be saved as a copy."));
        return backend->save(identifyTrace(target).path / combinedQmlFileName);
    }

    const Result<> result = backend->save(target);
    if (result)
        setFilePath(target);
    return result;
}

bool ProfilerTraceDocument::isModified() const
{
    return Utils::anyOf(m_backends, [](const ProfilerTraceBackend *backend) {
        return backend->isModified();
    });
}

bool ProfilerTraceDocument::isSaveAsAllowed() const
{
    // A combined bundle is two traces in a directory; a copy under a new name
    // would have to clone both. Plain save still writes the notes back.
    return m_format != TraceFormat::Combined && m_backends.first()->isSaveable();
}

Core::IDocument::ReloadBehavior ProfilerTraceDocument::reloadBehavior(ChangeTrigger state,
                                                                     ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    // A trace has no editable text to lose, so re-read it without asking.
    return BehaviorSilent;
}

Result<> ProfilerTraceDocument::reload(ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return ResultOk;
    if (type == TypeRemoved)
        return ResultOk;

    for (ProfilerTraceBackend *backend : std::as_const(m_backends))
        backend->clear();
    load(filePath());
    return ResultOk;
}

} // namespace Profiler::Internal
