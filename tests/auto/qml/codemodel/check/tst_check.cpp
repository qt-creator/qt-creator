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

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QtTest>
#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;

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

static bool offsetComparator(DiagnosticMessage lhs, DiagnosticMessage rhs)
{
    return lhs.loc.offset < rhs.loc.offset;
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
    Document::Ptr doc = Document::create(path, Document::QmlLanguage);
    QFile file(doc->fileName());
    file.open(QFile::ReadOnly | QFile::Text);
    doc->setSource(file.readAll());
    file.close();
    doc->parse();
    snapshot.insert(doc);

    QVERIFY(!doc->source().isEmpty());
    QVERIFY(doc->diagnosticMessages().isEmpty());

    ContextPtr context = Link(snapshot, QStringList(), LibraryInfo())();

    Check checker(doc, context);
    QList<DiagnosticMessage> messages = checker();
    std::sort(messages.begin(), messages.end(), &offsetComparator);

    const QRegExp errorPattern(" E (\\d+) (\\d+)(.*)");
    const QRegExp warningPattern(" W (\\d+) (\\d+)(.*)");
    const QRegExp defaultmsgPattern(" DEFAULTMSG (.*)");

    QList<DiagnosticMessage> expectedMessages;
    QString defaultMessage;
    foreach (const AST::SourceLocation &comment, doc->engine()->comments()) {
        const QString text = doc->source().mid(comment.begin(), comment.end() - comment.begin());

        if (defaultmsgPattern.indexIn(text) != -1) {
            defaultMessage = defaultmsgPattern.cap(1);
            continue;
        }

        const QRegExp *match = 0;
        DiagnosticMessage::Kind kind = DiagnosticMessage::Error;
        if (errorPattern.indexIn(text) != -1) {
            match = &errorPattern;
        } else if (warningPattern.indexIn(text) != -1) {
            kind = DiagnosticMessage::Warning;
            match = &warningPattern;
        }
        if (!match)
            continue;
        const int columnStart = match->cap(1).toInt();
        const int columnEnd = match->cap(2).toInt() + 1;
        QString message = match->cap(3);
        if (message.isEmpty())
            message = defaultMessage;
        else
            message = message.mid(1);
        expectedMessages += DiagnosticMessage(
                    kind,
                    SourceLocation(
                        comment.offset - comment.startColumn + columnStart,
                        columnEnd - columnStart,
                        comment.startLine,
                        columnStart),
                    message);
    }

    for (int i = 0; i < messages.size(); ++i) {
        DiagnosticMessage expected;
        if (i < expectedMessages.size())
            expected = expectedMessages.at(i);
        DiagnosticMessage actual = messages.at(i);
        bool fail = false;
        fail |= !QCOMPARE_NOEXIT(actual.message, expected.message);
        fail |= !QCOMPARE_NOEXIT(actual.loc.startLine, expected.loc.startLine);
        if (fail)
            return;
        fail |= !QCOMPARE_NOEXIT(actual.kind, expected.kind);
        fail |= !QCOMPARE_NOEXIT(actual.loc.startColumn, expected.loc.startColumn);
        fail |= !QCOMPARE_NOEXIT(actual.loc.offset, expected.loc.offset);
        fail |= !QCOMPARE_NOEXIT(actual.loc.length, expected.loc.length);
        if (fail) {
            qDebug() << "Failed for message on line" << actual.loc.startLine << actual.message;
            return;
        }
    }
    if (expectedMessages.size() > messages.size()) {
        DiagnosticMessage missingMessage = expectedMessages.at(messages.size());
        qDebug() << "expected more messages: " << missingMessage.loc.startLine << missingMessage.message;
        QFAIL("more messages expected");
    }
}

QTEST_MAIN(tst_Check);

#include "tst_check.moc"
