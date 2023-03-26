// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <utils/filepath.h>

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
};

tst_Reformatter::tst_Reformatter()
{
}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_Reformatter::test_data()
{
    QTest::addColumn<QString>("path");

    QDirIterator it(TESTSRCDIR, QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"), QDir::Files);
    while (it.hasNext()) {
        const QString fileName = it.next();
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

    QStringList sourceLines = source.split(QLatin1Char('\n'));
    QStringList newLines = rewritten.split(QLatin1Char('\n'));

    // compare line by line
    int commonLines = qMin(newLines.size(), sourceLines.size());
    bool insideMultiLineComment = false;
    for (int i = 0; i < commonLines; ++i) {
        // names intentional to make 'Actual (sourceLine): ...\nExpected (newLinee): ...' line up
        const QString &sourceLine = sourceLines.at(i);
        const QString &newLinee = newLines.at(i);
        if (!insideMultiLineComment && sourceLine.trimmed().startsWith("/*")) {
            insideMultiLineComment = true;
            sourceLines.insert(i, "\n");
            continue;
        }
        if (sourceLine.trimmed().endsWith("*/"))
            insideMultiLineComment = false;
        if (sourceLine.trimmed().isEmpty() && newLinee.trimmed().isEmpty())
            continue;
        bool fail = !QCOMPARE_NOEXIT(newLinee, sourceLine);
        if (fail) {
            qDebug() << "in line" << (i + 1);
            return;
        }
    }
    QCOMPARE(sourceLines.size(), newLines.size());
}

QTEST_GUILESS_MAIN(tst_Reformatter);

#include "tst_reformatter.moc"
