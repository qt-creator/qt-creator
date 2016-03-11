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

#include <clangstaticanalyzer/clangstaticanalyzerlogfilereader.h>

#include <utils/fileutils.h>

#include <QtTest>

enum { debug = 0 };

using namespace Debugger;
using namespace ClangStaticAnalyzer::Internal;

namespace {

QDebug operator<<(QDebug dbg, const ExplainingStep &step)
{
    dbg << '\n'
        << "    ExplainingStep\n"
        << "      location:" << step.location << '\n'
        << "      ranges:\n";
    foreach (const DiagnosticLocation &location, step.ranges)
        dbg << "      " << location << '\n';
    dbg
        << "      message:" << step.message << '\n'
        << "      extendedMessage:" << step.extendedMessage << '\n'
        << "      depth:" << step.depth << '\n';
    return dbg;
}

QDebug operator<<(QDebug dbg, const Diagnostic &diagnostic)
{
    dbg << "\nDiagnostic\n"
        << "  description:" << diagnostic.description << '\n'
        << "  category:" << diagnostic.category << '\n'
        << "  type:" << diagnostic.type << '\n'
        << "  issueContextKind:" << diagnostic.issueContextKind << '\n'
        << "  issueContext:" << diagnostic.issueContext << '\n'
        << "  location:" << diagnostic.location << '\n'
        << "  explaining steps:\n";
    foreach (const ExplainingStep &explaingStep, diagnostic.explainingSteps)
        dbg << explaingStep;
    return dbg;
}

bool createEmptyFile(const QString &filePath)
{
    Utils::FileSaver saver(filePath);
    return saver.write("") && saver.finalize();
}

QString testFilePath(const QString &relativePath)
{
    const QString fullPath = QString::fromLatin1(SRCDIR) + relativePath;
    const QFileInfo fi(fullPath);
    if (fi.exists() && fi.isReadable())
        return fullPath;
    return QString();
}

} // anonymous namespace

class ClangStaticAnalyzerLogFileReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void readEmptyFile();
    void readFileWithNoDiagnostics();
    void readFileWithDiagnostics();
};

void ClangStaticAnalyzerLogFileReaderTest::readEmptyFile()
{
    const QString filePath = QDir::tempPath() + QLatin1String("/empty.file");
    QVERIFY(createEmptyFile(filePath));

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(filePath, &errorMessage);
    QVERIFY(!errorMessage.isEmpty());
    if (debug)
        qDebug() << errorMessage;
    QVERIFY(diagnostics.isEmpty());
}

void ClangStaticAnalyzerLogFileReaderTest::readFileWithNoDiagnostics()
{
    const QString filePath = testFilePath(QLatin1String("/data/noDiagnostics.plist"));

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(filePath, &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(diagnostics.isEmpty());
}

void ClangStaticAnalyzerLogFileReaderTest::readFileWithDiagnostics()
{
    const QString filePath = testFilePath(QLatin1String("/data/someDiagnostics.plist"));

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(filePath, &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!diagnostics.isEmpty());

    const QString commonPath = QLatin1String("../csatestproject/core.CallAndMessage3.cpp");

    const Diagnostic d1 = diagnostics.first();
    if (debug)
        qDebug() << d1;
    QCOMPARE(d1.description, QLatin1String("Called function pointer is null (null dereference)"));
    QCOMPARE(d1.category, QLatin1String("Logic error"));
    QCOMPARE(d1.type, d1.description);
    QCOMPARE(d1.issueContextKind, QLatin1String("function"));
    QCOMPARE(d1.issueContext, QLatin1String("test"));
    QCOMPARE(d1.location, DiagnosticLocation(commonPath, 36, 3));

    QCOMPARE(d1.explainingSteps.size(), 2);
    const ExplainingStep step1 = d1.explainingSteps.at(0);
    QCOMPARE(step1.location, DiagnosticLocation(commonPath, 35, 3));
    QCOMPARE(step1.ranges.size(), 2);
    QCOMPARE(step1.ranges.at(0), DiagnosticLocation(commonPath, 35, 3));
    QCOMPARE(step1.ranges.at(1), DiagnosticLocation(commonPath, 35, 9));
    QCOMPARE(step1.depth, 0);
    QCOMPARE(step1.message, QLatin1String("Null pointer value stored to 'foo'"));
    QCOMPARE(step1.extendedMessage, step1.message);

    const ExplainingStep step2 = d1.explainingSteps.at(1);
    QCOMPARE(step2.location, DiagnosticLocation(commonPath, 36, 3));
    QCOMPARE(step2.ranges.size(), 2);
    QCOMPARE(step2.ranges.at(0), DiagnosticLocation(commonPath, 36, 3));
    QCOMPARE(step2.ranges.at(1), DiagnosticLocation(commonPath, 36, 5));
    QCOMPARE(step2.depth, 0);
    QCOMPARE(step2.message, QLatin1String("Called function pointer is null (null dereference)"));
    QCOMPARE(step2.extendedMessage, step2.message);
}

QTEST_MAIN(ClangStaticAnalyzerLogFileReaderTest)

#include "tst_clangstaticanalyzerlogfilereader.moc"
