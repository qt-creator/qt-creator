// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/result.h>

#include <QObject>
#include <QWidgetList>

#include <chrono>

namespace Profiler::Internal {

// One trace's worth of models and views. A ProfilerTraceDocument owns its
// backends -- nothing is shared between documents -- which is what lets several
// traces be open at once, each in its own editor.
//
// A document usually has one backend; a combined recording has two, and shows
// both their view sets (see design-docs/native-mixed-profiler-design.md).
class ProfilerTraceBackend : public QObject
{
    Q_OBJECT

public:
    explicit ProfilerTraceBackend(QObject *parent = nullptr);
    ~ProfilerTraceBackend() override;

    // The views to show, in tab order; their windowTitle() names the tab. Called
    // once, when the editor builds its widget.
    virtual QWidgetList views(QWidget *parent) = 0;

    // Controls for the editor's toolbar, in order. No ownership passed.
    virtual QList<QWidget *> toolBarWidgets() { return {}; }

    virtual void load(const Utils::FilePath &path) = 0;

    // Whether this backend can write its trace back out. Only the QML profiler
    // and perf can; the sampler and CTF backends read what another tool wrote.
    virtual bool isSaveable() const { return false; }
    virtual Utils::Result<> save(const Utils::FilePath &path);

    // Whether the trace holds changes worth saving, such as timeline notes.
    virtual bool isModified() const { return false; }

    virtual void clear() = 0;
    virtual std::chrono::milliseconds traceDuration() const = 0;

signals:
    void error(const QString &message);
    void loadFinished();
    void traceChanged();
    // A source location the user asked to jump to, already resolved to a local
    // file: only the backend knows how to map a recorded path onto this machine.
    void gotoSourceLocation(const Utils::Link &link);
};

} // namespace Profiler::Internal
