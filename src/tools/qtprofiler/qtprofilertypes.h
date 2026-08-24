// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QtProfiler {

// A trace file format determines which manager and view set are used. Chrome
// Trace Format and Common Trace Format both render through the CTF views.
// Combined shows the QML profiler views and the sampler views together, for a
// combined recording (native-mixed sampler trace + its source QML trace).
enum class Format { Qml, Ctf, Sampler, Combined };

} // namespace QtProfiler
