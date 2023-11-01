// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppheadersource_test.h"

#include "cppeditorplugin.h"
#include "cpptoolsreuse.h"
#include "cpptoolstestcase.h"
#include "cppfilesettingspage.h"

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QtTest>

using namespace Utils;

static inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

static void createTempFile(const FilePath &filePath)
{
    QString fileName = filePath.toString();
    QFile file(fileName);
    QDir(QFileInfo(fileName).absolutePath()).mkpath(_("."));
    file.open(QFile::WriteOnly);
    file.close();
}

static QString baseTestDir()
{
    return Utils::TemporaryDirectory::masterDirectoryPath() + "/qtc_cppheadersource/";
}

namespace CppEditor::Internal {

void HeaderSourceTest::test()
{
    QFETCH(QString, sourceFileName);
    QFETCH(QString, headerFileName);

    CppEditor::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QDir path = QDir(temporaryDir.path() + QLatin1Char('/') + _(QTest::currentDataTag()));
    const FilePath sourcePath = FilePath::fromString(path.absoluteFilePath(sourceFileName));
    const FilePath headerPath = FilePath::fromString(path.absoluteFilePath(headerFileName));
    createTempFile(sourcePath);
    createTempFile(headerPath);

    bool wasHeader;
    CppEditorPlugin::clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(sourcePath, &wasHeader), headerPath);
    QVERIFY(!wasHeader);
    CppEditorPlugin::clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(headerPath, &wasHeader), sourcePath);
    QVERIFY(wasHeader);
}

void HeaderSourceTest::test_data()
{
    QTest::addColumn<QString>("sourceFileName");
    QTest::addColumn<QString>("headerFileName");
    QTest::newRow("samedir") << _("foo.cpp") << _("foo.h");
    QTest::newRow("includesub") << _("foo.cpp") << _("include/foo.h");
    QTest::newRow("headerprefix") << _("foo.cpp") << _("testh_foo.h");
    QTest::newRow("sourceprefixwsub") << _("testc_foo.cpp") << _("include/foo.h");
    QTest::newRow("sourceAndHeaderPrefixWithBothsub") << _("src/testc_foo.cpp") << _("include/testh_foo.h");
}

void HeaderSourceTest::initTestCase()
{
    QDir(baseTestDir()).mkpath(_("."));
    CppFileSettings fs = CppEditorPlugin::fileSettings(nullptr);
    fs.headerSearchPaths.append(QLatin1String("include"));
    fs.headerSearchPaths.append(QLatin1String("../include"));
    fs.sourceSearchPaths.append(QLatin1String("src"));
    fs.sourceSearchPaths.append(QLatin1String("../src"));
    fs.headerPrefixes.append(QLatin1String("testh_"));
    fs.sourcePrefixes.append(QLatin1String("testc_"));
    CppEditorPlugin::setGlobalFileSettings(fs);
}

void HeaderSourceTest::cleanupTestCase()
{
    Utils::FilePath::fromString(baseTestDir()).removeRecursively();
    CppFileSettings fs = CppEditorPlugin::fileSettings(nullptr);
    fs.headerSearchPaths.removeLast();
    fs.headerSearchPaths.removeLast();
    fs.sourceSearchPaths.removeLast();
    fs.sourceSearchPaths.removeLast();
    fs.headerPrefixes.removeLast();
    fs.sourcePrefixes.removeLast();
    CppEditorPlugin::setGlobalFileSettings(fs);
}

} // namespace CppEditor::Internal
