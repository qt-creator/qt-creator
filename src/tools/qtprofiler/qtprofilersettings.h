// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace QtProfiler {

class QtProfilerSettings : public Utils::AspectContainer
{
public:
    QtProfilerSettings();

    Utils::FilePathAspect lastTraceFile{this};
    Utils::ByteArrayAspect windowGeometry{this};

    // Not persisted. A command line passed via --launch; the window seeds the
    // active backend's launch settings from these. Backend-specific recording
    // options (executable, arguments, interval, host/port, ...) live in each
    // backend's own settings.
    Utils::FilePathAspect recordExecutable{this};
    Utils::StringAspect recordArguments{this};
    Utils::BoolAspect exitOnError{this};
    Utils::BoolAspect withRpc{this};
};

QtProfilerSettings &settings();

} // namespace QtProfiler
