// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "json/json.hpp"

#include <QPromise>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Profiler::Internal {

// Streaming parser for Chrome Trace Format JSON files. Each trace event in the
// "traceEvents" array (or top-level array) is emitted as a separate result.
void loadChromeJson(QPromise<nlohmann::json> &promise, const QString &fileName);

// Reader for CTF2 / Common Trace Format directories. Converts CTF events into
// Chrome-format trace event objects and emits them in chronological order.
void loadCtf2Data(QPromise<nlohmann::json> &promise, const QString &dirPath);

} // namespace Profiler::Internal
