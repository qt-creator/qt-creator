/****************************************************************************
**
** Copyright (C) 2015 Orgad Shaneh <orgads@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsplugin.h"
#include "cpptoolsreuse.h"
#include "cpptoolstestcase.h"
#include "cppfilesettingspage.h"

#include <utils/fileutils.h>

#include <QDir>
#include <QtTest>

static inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

static void createTempFile(const QString &fileName)
{
    QFile file(fileName);
    QDir(QFileInfo(fileName).absolutePath()).mkpath(_("."));
    file.open(QFile::WriteOnly);
    file.close();
}

static QString baseTestDir()
{
    return QDir::tempPath() + _("/qtc_cppheadersource/");
}

namespace CppTools {
namespace Internal {

void CppToolsPlugin::test_headersource()
{
    QFETCH(QString, sourceFileName);
    QFETCH(QString, headerFileName);

    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QDir path = QDir(temporaryDir.path() + QLatin1Char('/') + _(QTest::currentDataTag()));
    const QString sourcePath = path.absoluteFilePath(sourceFileName);
    const QString headerPath = path.absoluteFilePath(headerFileName);
    createTempFile(sourcePath);
    createTempFile(headerPath);

    bool wasHeader;
    clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(sourcePath, &wasHeader), headerPath);
    QVERIFY(!wasHeader);
    clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(headerPath, &wasHeader), sourcePath);
    QVERIFY(wasHeader);
}

void CppToolsPlugin::test_headersource_data()
{
    QTest::addColumn<QString>("sourceFileName");
    QTest::addColumn<QString>("headerFileName");
    QTest::newRow("samedir") << _("foo.cpp") << _("foo.h");
    QTest::newRow("includesub") << _("foo.cpp") << _("include/foo.h");
    QTest::newRow("headerprefix") << _("foo.cpp") << _("testh_foo.h");
    QTest::newRow("sourceprefixwsub") << _("testc_foo.cpp") << _("include/foo.h");
    QTest::newRow("sourceAndHeaderPrefixWithBothsub") << _("src/testc_foo.cpp") << _("include/testh_foo.h");
}

void CppToolsPlugin::initTestCase()
{
    QDir(baseTestDir()).mkpath(_("."));
    m_fileSettings->headerSearchPaths.append(QLatin1String("include"));
    m_fileSettings->headerSearchPaths.append(QLatin1String("../include"));
    m_fileSettings->sourceSearchPaths.append(QLatin1String("src"));
    m_fileSettings->sourceSearchPaths.append(QLatin1String("../src"));
    m_fileSettings->headerPrefixes.append(QLatin1String("testh_"));
    m_fileSettings->sourcePrefixes.append(QLatin1String("testc_"));
}

void CppToolsPlugin::cleanupTestCase()
{
    Utils::FileUtils::removeRecursively(Utils::FileName::fromString(baseTestDir()));
    m_fileSettings->headerSearchPaths.removeLast();
    m_fileSettings->headerSearchPaths.removeLast();
    m_fileSettings->sourceSearchPaths.removeLast();
    m_fileSettings->sourceSearchPaths.removeLast();
    m_fileSettings->headerPrefixes.removeLast();
    m_fileSettings->sourcePrefixes.removeLast();
}

} // namespace Internal
} // namespace CppTools
