// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef TRACING_WITH_GPU_BACKEND
#include <tracing/trackpaintergpu.h>
#endif
#include <tracing/trackpainterraster.h>

#include <QTest>

namespace Timeline {

// Adds a "backend" column with a row for each track painter built here.
inline void addTrackPainterBackends()
{
    QTest::addColumn<TrackBackend>("backend");
    QTest::newRow("software") << TrackBackend::Software;
#ifdef TRACING_WITH_GPU_BACKEND
    QTest::newRow("gpu") << TrackBackend::Gpu;
#endif
}

// Runs test.template operator()<Painter>() with the painter class of `backend`.
template<typename Test>
void withTrackPainter(TrackBackend backend, const Test &test)
{
#ifdef TRACING_WITH_GPU_BACKEND
    if (backend == TrackBackend::Gpu)
        return test.template operator()<TrackPainterGpu>();
#else
    Q_UNUSED(backend)
#endif
    test.template operator()<TrackPainterRaster>();
}

// The same, with the backend of the current row of initTestCase_data().
template<typename Test>
void withTrackPainter(const Test &test)
{
    QFETCH_GLOBAL(TrackBackend, backend);
    withTrackPainter(backend, test);
}

} // namespace Timeline
