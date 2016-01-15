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

    Document::MutablePtr doc = Document::create(path, ModelManagerInterface::guessLanguageOfFile(path));
    QFile file(doc->fileName());
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
    for (int i = 0; i < commonLines; ++i) {
        // names intentional to make 'Actual (sourceLine): ...\nExpected (newLinee): ...' line up
        const QString &sourceLine = sourceLines.at(i);
        const QString &newLinee = newLines.at(i);
        if (sourceLine.trimmed().isEmpty() && newLinee.trimmed().isEmpty())
            continue;
        bool fail = !QCOMPARE_NOEXIT(sourceLine, newLinee);
        if (fail) {
            qDebug() << "in line" << (i + 1);
            return;
        }
    }
    QCOMPARE(sourceLines.size(), newLines.size());
}

QTEST_MAIN(tst_Reformatter);

#include "tst_reformatter.moc"
