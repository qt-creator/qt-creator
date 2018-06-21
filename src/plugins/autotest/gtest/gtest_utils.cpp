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

#include "gtest_utils.h"

#include <QRegularExpression>
#include <QStringList>

namespace Autotest {
namespace Internal {
namespace GTestUtils {

static const QStringList valid = {
    QStringLiteral("TEST"), QStringLiteral("TEST_F"), QStringLiteral("TEST_P"),
    QStringLiteral("TYPED_TEST"), QStringLiteral("TYPED_TEST_P")
};

bool isGTestMacro(const QString &macro)
{
    return valid.contains(macro);
}

bool isGTestParameterized(const QString &macro)
{
    return macro == QStringLiteral("TEST_P") || macro == QStringLiteral("TYPED_TEST_P");
}

bool isGTestTyped(const QString &macro)
{
    return macro == QStringLiteral("TYPED_TEST") || macro == QStringLiteral("TYPED_TEST_P");
}

bool isValidGTestFilter(const QString &filterExpression)
{
    // this still is not a 100% validation - but a compromise
    // - numbers after '.' should get prohibited
    // - more than one '.' inside a single filter should be prohibited
    static const QRegularExpression regex("^(:*([_a-zA-Z*.?][_a-zA-Z0-9*.?]*:*)*)?"
                                          "(-(:*([_a-zA-Z*.?][_a-zA-Z0-9*.?]*:*)*)?)?$");

    return regex.match(filterExpression).hasMatch();
}

} // namespace GTestUtils
} // namespace Internal
} // namespace Autotest
