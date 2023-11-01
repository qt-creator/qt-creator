// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace QmlProfiler::Internal {

class QmlProfilerSettings : public Utils::AspectContainer
{
public:
    QmlProfilerSettings();

    Utils::BoolAspect flushEnabled{this};
    Utils::IntegerAspect flushInterval{this};
    Utils::FilePathAspect lastTraceFile{this};
    Utils::BoolAspect aggregateTraces{this};
};

QmlProfilerSettings &globalSettings();

} // QmlProfiler::Internal
