// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/outputformatter.h>

namespace ProjectExplorer::Internal {

Utils::OutputLineParser *createSanitizerOutputParser();

void setupSanitizerOutputParser();

#ifdef WITH_TESTS
QObject *createSanitizerOutputParserTest();
#endif

} // ProjectExplorer::Internal

