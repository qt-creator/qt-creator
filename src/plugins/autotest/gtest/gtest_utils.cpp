// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtest_utils.h"

#include <QRegularExpression>
#include <QStringList>

namespace Autotest {
namespace Internal {
namespace GTestUtils {

static const QStringList valid = {
    QStringLiteral("TEST"), QStringLiteral("TEST_F"), QStringLiteral("TEST_P"),
    QStringLiteral("TYPED_TEST"), QStringLiteral("TYPED_TEST_P"),
    QStringLiteral("GTEST_TEST")
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
