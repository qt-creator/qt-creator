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

#include "testdatadir.h"

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTest>

using namespace Core::Tests;

TestDataDir::TestDataDir(const QString &directory)
    : m_directory(directory)
{
    QFileInfo fi(m_directory);
    QVERIFY(fi.exists());
    QVERIFY(fi.isDir());
}

QString TestDataDir::file(const QString &fileName) const
{
    return directory() + QLatin1Char('/') + fileName;
}

QString TestDataDir::path() const
{
    return m_directory;
}

QString TestDataDir::directory(const QString &subdir, bool clean) const
{
    QString path = m_directory;
    if (!subdir.isEmpty())
        path += QLatin1Char('/') + subdir;
    if (clean)
        path = QDir::cleanPath(path);
    return path;
}
