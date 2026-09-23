// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "json/json.hpp"

#include <QPromise>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Profiler::Internal {

// Streaming parser for Chrome Trace Format JSON files. Each trace event in the
// "traceEvents" array (or top-level array) is emitted as a separate result.
void loadChromeJson(QPromise<nlohmann::json> &promise, const QString &fileName);

// The tracepoint providers the trace in `dirPath` declares, sorted, from its
// metadata alone. A trace whose event classes state none -- a kernel recording,
// whose events are plain "sched_switch" -- has no providers to speak of and
// yields an empty list.
QStringList ctfTraceProviders(const QString &dirPath);

// Reader for CTF2 / Common Trace Format directories. Converts CTF events into
// Chrome-format trace event objects and emits them in chronological order.
// Only the events of `providers` are emitted, or all of them when it is empty.
// An event whose class names no provider belongs to none of them, and is
// emitted whatever `providers` says.
void loadCtf2Data(QPromise<nlohmann::json> &promise, const QString &dirPath,
                  const QStringList &providers = {});

} // namespace Profiler::Internal
