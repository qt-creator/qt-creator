/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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


#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QtTest>
#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;

class tst_Check : public QObject
{
    Q_OBJECT
public:
    tst_Check();

private slots:
    void test();
    void test_data();

    void initTestCase();
};

tst_Check::tst_Check()
{
}


#ifdef Q_OS_MAC
#  define SHARE_PATH "/Resources"
#else
#  define SHARE_PATH "/share/qtcreator"
#endif

QString resourcePath()
{
    return QDir::cleanPath(QTCREATORDIR + QLatin1String(SHARE_PATH));
}

void tst_Check::initTestCase()
{
    // the resource path is wrong, have to load things manually
    QFileInfo builtins(resourcePath() + "/qml-type-descriptions/builtins.qmltypes");
    QStringList errors, warnings;
    CppQmlTypesLoader::defaultQtObjects = CppQmlTypesLoader::loadQmlTypes(QFileInfoList() << builtins, &errors, &warnings);
}

static bool offsetComparator(const Message &lhs, const Message &rhs)
{
    return lhs.location.offset < rhs.location.offset;
}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_Check::test_data()
{
    QTest::addColumn<QString>("path");

    QDirIterator it(TESTSRCDIR, QStringList("*.qml"), QDir::Files);
    while (it.hasNext()) {
        const QString fileName = it.next();
        QTest::newRow(fileName.toLatin1()) << it.filePath();
    }
}

void tst_Check::test()
{
    QFETCH(QString, path);

    Snapshot snapshot;
    Document::MutablePtr doc = Document::create(path, Dialect::Qml);
    QFile file(doc->fileName());
    file.open(QFile::ReadOnly | QFile::Text);
    doc->setSource(file.readAll());
    file.close();
    doc->parse();
    snapshot.insert(doc);

    QVERIFY(!doc->source().isEmpty());
    QVERIFY(doc->diagnosticMessages().isEmpty());

    ViewerContext vContext;
    vContext.flags = ViewerContext::Complete;
    ContextPtr context = Link(snapshot, vContext, LibraryInfo())();

    Check checker(doc, context);
    QList<Message> messages = checker();
    std::sort(messages.begin(), messages.end(), &offsetComparator);

    const QRegExp messagePattern(" (\\d+) (\\d+) (\\d+)");

    QList<Message> expectedMessages;
    foreach (const AST::SourceLocation &comment, doc->engine()->comments()) {
        const QString text = doc->source().mid(comment.begin(), comment.end() - comment.begin());

        if (messagePattern.indexIn(text) == -1)
            continue;
        const int type = messagePattern.cap(1).toInt();
        const int columnStart = messagePattern.cap(2).toInt();
        const int columnEnd = messagePattern.cap(3).toInt() + 1;

        Message message;
        message.location = SourceLocation(
                    comment.offset - comment.startColumn + columnStart,
                    columnEnd - columnStart,
                    comment.startLine,
                    columnStart),
        message.type = static_cast<Type>(type);
        expectedMessages += message;
    }

    for (int i = 0; i < messages.size(); ++i) {
        if (i >= expectedMessages.size())
            break;
        Message actual = messages.at(i);
        Message expected = expectedMessages.at(i);
        bool fail = false;
        fail |= !QCOMPARE_NOEXIT(actual.location.startLine, expected.location.startLine);
        fail |= !QCOMPARE_NOEXIT((int)actual.type, (int)expected.type);
        if (fail)
            return;
        fail |= !QCOMPARE_NOEXIT(actual.location.startColumn, expected.location.startColumn);
        fail |= !QCOMPARE_NOEXIT(actual.location.offset, expected.location.offset);
        fail |= !QCOMPARE_NOEXIT(actual.location.length, expected.location.length);
        if (fail) {
            qDebug() << "Failed for message on line" << actual.location.startLine << actual.message;
            return;
        }
    }
    if (expectedMessages.size() > messages.size()) {
        for (int i = messages.size(); i < expectedMessages.size(); ++i) {
            Message missingMessage = expectedMessages.at(i);
            qDebug() << "expected message of type" << missingMessage.type << "on line" << missingMessage.location.startLine;
        }
        QFAIL("more messages expected");
    }
    if (expectedMessages.size() < messages.size()) {
        for (int i = expectedMessages.size(); i < messages.size(); ++i) {
            Message extraMessage = messages.at(i);
            qDebug() << "unexpected message of type" << extraMessage.type << "on line" << extraMessage.location.startLine;
        }
        QFAIL("fewer messages expected");
    }
}

QTEST_MAIN(tst_Check);

#include "tst_check.moc"
