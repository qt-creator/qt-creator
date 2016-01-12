/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
            QStringLiteral("TEST"), QStringLiteral("TEST_F"), QStringLiteral("TEST_P")
        };
        return valid.contains(macro);
    }

    static bool isGTestParameterized(const QString &macro)
    {
        return macro == QStringLiteral("TEST_P");
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
