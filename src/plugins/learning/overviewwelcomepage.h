// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Learning::Internal {

void setupOverviewWelcomePage(QObject *guard);

#ifdef WITH_TESTS
class LearningTest final : public QObject {
    Q_OBJECT
private slots:
    void testFlagsMatching();
    void testFlagsMatching_data();
};
#endif // WITH_TESTS

} // namespace Learning::Internal
