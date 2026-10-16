// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Ripgrep::Internal {

class OutputParserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();
    void testChunksEndingInsideALine();
    void testRegularExpressionUserData();
    void testCapturesStayInTheRange();
    void testManyMatchesOnALongLine();
};

} // namespace Ripgrep::Internal
