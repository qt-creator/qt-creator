/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/runcontrol.h>

// "file" or "qrc", colon, optional '//', '/' and further characters
#define QT_QML_URL_REGEXP "(?:file|qrc):(?://)?/.+?"
#define QT_ASSERT_REGEXP "ASSERT: .* in file (.+, line \\d+)"
#define QT_ASSERT_X_REGEXP "ASSERT failure in .*: \".*\", file (.+, line \\d+)"
#define QT_TEST_FAIL_UNIX_REGEXP "^   Loc: \\[((?<file>.+)(?|\\((?<line>\\d+)\\)|:(?<line>\\d+)))\\]$"
#define QT_TEST_FAIL_WIN_REGEXP "^((?<file>.+)\\((?<line>\\d+)\\)) : failure location\\s*$"

namespace QtSupport {
namespace Internal {

class QtOutputFormatterFactory : public ProjectExplorer::OutputFormatterFactory
{
public:
    QtOutputFormatterFactory();
};

} // namespace Internal
} // namespace QtSupport
