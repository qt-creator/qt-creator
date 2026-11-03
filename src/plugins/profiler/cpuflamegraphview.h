// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <tracing/flamegraphwidget.h>

namespace Profiler::Internal {

class CpuFlameGraphView final : public Timeline::FlameGraphWidget
{
    Q_OBJECT

public:
    enum class Measures { Samples, SamplesAndResources };

    CpuFlameGraphView(QAbstractItemModel *model, Measures measures, QWidget *parent = nullptr);
};

} // namespace Profiler::Internal
