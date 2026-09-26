// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Android::Internal {

// Tells the Java plugin what the Java of an Android build configuration is:
// the platform of the SDK it targets, the Java that Qt ships for Android,
// and the package source directories of its products. That is what the
// language server of such a project comes to resolve against.
void setupAndroidJavaClassPath();

#ifdef WITH_TESTS
QObject *createAndroidJavaClassPathTest();
#endif // WITH_TESTS

} // namespace Android::Internal
