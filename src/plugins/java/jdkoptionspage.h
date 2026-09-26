// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS
class QObject;
#endif

namespace Java::Internal {

void setupJdkOptionsPage();

#ifdef WITH_TESTS
QObject *createJdkOptionsPageTest();
#endif

} // namespace Java::Internal
