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

#include "cppsourceprocessertesthelper.h"

#include <QDir>

namespace CppTools {
namespace Tests {

QString TestIncludePaths::includeBaseDirectory()
{
    return QLatin1String(SRCDIR)
            + QLatin1String("/../../../tests/auto/cplusplus/preprocessor/data/include-data");
}

QString TestIncludePaths::globalQtCoreIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/QtCore"));
}

QString TestIncludePaths::globalIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/global"));
}

QString TestIncludePaths::directoryOfTestFile()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/local"));
}

QString TestIncludePaths::testFilePath(const QString &fileName)
{
    return Tests::TestIncludePaths::directoryOfTestFile() + QLatin1Char('/') + fileName;
}

} // namespace Tests
} // namespace CppTools
