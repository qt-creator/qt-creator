// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/outputformatter.h>

namespace Android::Internal {

Utils::OutputLineParser *createAndroidLogcatCrashParser();

void setupAndroidLogcatCrashParser();

#ifdef WITH_TESTS
QObject *createAndroidLogcatCrashParserTest();
#endif

} // namespace Android::Internal
