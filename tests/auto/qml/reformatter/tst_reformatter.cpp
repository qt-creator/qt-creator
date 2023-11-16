// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <utils/filepath.h>

#include <QApplication>
#include <QGraphicsObject>
#include <QScopedPointer>
#include <QtTest>

#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;

class tst_Reformatter : public QObject
{
    Q_OBJECT
public:
    tst_Reformatter();

private slots:
    void test();
    void test_data();

    void reformatter_data();
    void reformatter();

private:
};

tst_Reformatter::tst_Reformatter()
{
}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_Reformatter::test_data()
{
    QTest::addColumn<QString>("path");

    // This test performs line-by-line comparison and fails if reformatting
    // makes a change inline, for example whitespace removal. We omit
    // those files in this test.
    QSet<QString> excludedFiles;
    excludedFiles << QString::fromLatin1(TESTSRCDIR) + "/typeAnnotations.qml";
    excludedFiles << QString::fromLatin1(TESTSRCDIR) + "/typeAnnotations.formatted.qml";

    QDirIterator it(TESTSRCDIR, QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"), QDir::Files);
    while (it.hasNext()) {
        const QString fileName = it.next();
        if (!excludedFiles.contains(fileName))
            QTest::newRow(fileName.toLatin1()) << it.filePath();
    }
}

void tst_Reformatter::test()
{
    QFETCH(QString, path);
    Utils::FilePath fPath = Utils::FilePath::fromString(path);

    Document::MutablePtr doc = Document::create(fPath, ModelManagerInterface::guessLanguageOfFile(fPath));
    QFile file(doc->fileName().toString());
    file.open(QFile::ReadOnly | QFile::Text);
    QString source = QString::fromUtf8(file.readAll());
    doc->setSource(source);
    file.close();
    doc->parse();

    QVERIFY(!doc->source().isEmpty());
    QVERIFY(doc->diagnosticMessages().isEmpty());

    QString rewritten = reformat(doc);

    QStringList sourceLines = source.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList newLines = rewritten.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    // compare line by line
    int commonLines = qMin(newLines.size(), sourceLines.size());
    for (int i = 0; i < commonLines; ++i) {
        // names intentional to make 'Actual (sourceLine): ...\nExpected (newLinee): ...' line up
        const QString &sourceLine = sourceLines.at(i);
        const QString &newLinee = newLines.at(i);
        bool fail = !QCOMPARE_NOEXIT(newLinee, sourceLine);
        if (fail) {
            qDebug() << "in line" << (i + 1);
            return;
        }
    }
    QCOMPARE(sourceLines.size(), newLines.size());
}

void tst_Reformatter::reformatter_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("formattedFilePath");

    QTest::newRow("typeAnnotations")
        << QString::fromLatin1(TESTSRCDIR) + "/typeAnnotations.qml"
        << QString::fromLatin1(TESTSRCDIR) +"/typeAnnotations.formatted.qml";
}

void tst_Reformatter::reformatter()
{
    QFETCH(QString, filePath);
    QFETCH(QString, formattedFilePath);

    Utils::FilePath fPath = Utils::FilePath::fromString(filePath);
    Document::MutablePtr doc
        = Document::create(fPath, ModelManagerInterface::guessLanguageOfFile(fPath));

    QString fileContent;
    {
        QFile file(filePath);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        fileContent = QString::fromUtf8(file.readAll());
    }
    doc->setSource(fileContent);
    doc->parse();
    QString expected;
    {
        QFile file(formattedFilePath);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        expected = QString::fromUtf8(file.readAll());
    }

    QString formatted = reformat(doc);
    QCOMPARE(formatted, expected);
}

QTEST_GUILESS_MAIN(tst_Reformatter);

#include "tst_reformatter.moc"
