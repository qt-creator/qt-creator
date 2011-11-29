/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/


#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljs/parser/qmljsast_p.h>

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

    Document::MutablePtr doc = Document::create(path, Document::guessLanguageFromSuffix(path));
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
