// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

namespace NanotraceHR {

enum class Tracing { IsDisabled, IsEnabled };

template<Tracing isEnabled>
class TraceFile;

using EnabledTraceFile = TraceFile<Tracing::IsEnabled>;

} // namespace NanotraceHR
