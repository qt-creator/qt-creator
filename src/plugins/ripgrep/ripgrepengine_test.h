// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Ripgrep::Internal {

class EngineTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_data();
    void test();
    void testSearchOptions_data();
    void testSearchOptions();
    void testErrorIsReported_data();
    void testErrorIsReported();
    void testUnreadableDirectory();
    void testEngineIsEnabled();

private:
    bool m_hasPcre2 = false;
};

} // namespace Ripgrep::Internal
