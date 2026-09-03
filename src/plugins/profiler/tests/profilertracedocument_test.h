// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Utils { class FilePath; }

namespace Profiler::Internal {

class ProfilerTraceDocument;

class ProfilerTraceDocumentTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testSourceOpensNextToTrace();
    void testSourceReusesTheSameSplit();
    void testSourceIgnoresLaterSplits();
    void testSourceKeepsItsSplitWhenTheTraceIsSplit();
    void testClosedSourceSplitIsForgotten();
    void testTraceMovedIntoTheSourceSplitGetsANewOne();

private:
    void selectEvent(const Utils::FilePath &source);

    ProfilerTraceDocument *m_document = nullptr;
};

} // namespace Profiler::Internal
