// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

// "file" or "qrc", colon, optional '//', '/' and further characters
#define QT_QML_URL_REGEXP "(?:file|qrc):(?://)?/.+?"
#define QT_ASSERT_REGEXP "ASSERT: .* in file (.+, line \\d+)"
#define QT_ASSERT_X_REGEXP "ASSERT failure in .*: \".*\", file (.+, line \\d+)"
#define QT_TEST_FAIL_UNIX_REGEXP "^   Loc: \\[((?<file>.+)(?|\\((?<line>\\d+)\\)|:(?<line>\\d+)))\\]$"
#define QT_TEST_FAIL_WIN_REGEXP "^((?<file>.+)\\((?<line>\\d+)\\)) : failure location\\s*$"

namespace QtSupport::Internal {

void setupQtOutputFormatter();

#ifdef WITH_TESTS
QObject *createQtOutputFormatterTest();
#endif

} // QtSupport::Internal
