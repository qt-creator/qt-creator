// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace PerfProfiler {
namespace Internal {

class PerfResourceCounterTest : public QObject
{
    Q_OBJECT
private slots:
    void testMallocFree();
    void testRandomFill();
    void testUnitSized();
    void testRandomAlternate();
    void testGuesses();
    void testNegative();
    void testInvalidId();
    void testMultiCounter();
    void testMove();
};

} // namespace Internal
} // namespace PerfProfiler
