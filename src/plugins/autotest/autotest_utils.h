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

#ifndef AUTOTEST_UTILS_H
#define AUTOTEST_UTILS_H

#include <QStringList>

namespace Autotest {
namespace Internal {

class TestUtils
{
public:
    static bool isGTestMacro(const QString &macro)
    {
        static QStringList valid = {
            QStringLiteral("TEST"), QStringLiteral("TEST_F"), QStringLiteral("TEST_P"),
            QStringLiteral("TYPED_TEST"), QStringLiteral("TYPED_TEST_P")
        };
        return valid.contains(macro);
    }

    static bool isGTestParameterized(const QString &macro)
    {
        return macro == QStringLiteral("TEST_P") || macro == QStringLiteral("TYPED_TEST_P");
    }

    static bool isGTestTyped(const QString &macro)
    {
        return macro == QStringLiteral("TYPED_TEST") || macro == QStringLiteral("TYPED_TEST_P");
    }

    static bool isQTestMacro(const QByteArray &macro)
    {
        static QByteArrayList valid = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};
        return valid.contains(macro);
    }

    static bool isQuickTestMacro(const QByteArray &macro)
    {
        static const QByteArrayList valid = {"QUICK_TEST_MAIN", "QUICK_TEST_OPENGL_MAIN"};
        return valid.contains(macro);
    }
};

} // namespace Internal
} // namespace Autotest

#endif // AUTOTEST_UTILS_H
