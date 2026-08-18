// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "traceformat.h"

#include <coreplugin/idocument.h>

#include <tracing/rangedetailswidget.h>

#include <QList>
#include <QPointer>

namespace Profiler::Internal {

class ProfilerTraceBackend;

// One profiling run or loaded trace, as a document. Each owns its own backends,
// so several traces can be open at once, each in its own editor.
//
// A document is either file-backed -- a trace loaded from disk, or one a
// recording wrote -- or live, filled by a running profiling session and saveable
// afterwards.
class ProfilerTraceDocument : public Core::IDocument
{
    Q_OBJECT

public:
    // Builds the backends the format calls for. A combined trace gets two of
    // them and shows both their view sets.
    ProfilerTraceDocument(Utils::Id editorId, TraceFormat format);
    ~ProfilerTraceDocument() override;

    TraceFormat format() const { return m_format; }
    const QList<ProfilerTraceBackend *> &backends() const { return m_backends; }
    // The panel every backend of this document fills. Shared so a trace shows a
    // single "Details" view; a combined trace would otherwise show two.
    Timeline::RangeDetailsWidget *rangeDetails() const { return m_rangeDetails; }

    Utils::Result<> open(const Utils::FilePath &filePath,
                         const Utils::FilePath &realFilePath) override;
    // A live document starts out empty and is filled by a running session;
    // there are no contents to hand it.
    Utils::Result<> setContents(const QByteArray &contents) override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    Utils::Result<> reload(ReloadFlag flag, ChangeType type) override;

signals:
    // A load or save is running; the editor disables its views meanwhile.
    void busyChanged(bool busy);

protected:
    Utils::Result<> saveImpl(const Utils::FilePath &filePath, SaveOption option) override;

private:
    void load(const Utils::FilePath &path);

    const TraceFormat m_format;
    QList<ProfilerTraceBackend *> m_backends;
    QPointer<Timeline::RangeDetailsWidget> m_rangeDetails;
};

} // namespace Profiler::Internal
