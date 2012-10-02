/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <QObject>
#include <QFile>
#include <CPlusPlus.h>
#include <CppDocument.h>
#include <LookupContext.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

#define VERIFY_ERRORS() \
    do { \
      QFile e(testdata(errorFile)); \
      QByteArray expectedErrors; \
      if (e.open(QFile::ReadOnly)) \
        expectedErrors = QTextStream(&e).readAll().toUtf8(); \
      QCOMPARE(errors, expectedErrors); \
    } while (0)


class tst_cxx11: public QObject
{
    Q_OBJECT

    /*
        Returns the path to some testdata file or directory.
    */
    static QString testdata(const QString &name = QString())
    {
        static const QString dataDirectory = QDir::currentPath() + QLatin1String("/data");
        QString result = dataDirectory;
        if (!name.isEmpty()) {
            result += QLatin1Char('/');
            result += name;
        }
        return result;
    }

    struct Client: CPlusPlus::DiagnosticClient {
        QByteArray *errors;

        Client(QByteArray *errors)
            : errors(errors)
        {
        }

        virtual void report(int level,
                            const StringLiteral *fileName,
                            unsigned line, unsigned column,
                            const char *format, va_list ap)
        {
            if (! errors)
                return;

            static const char *const pretty[] = { "warning", "error", "fatal" };

            QString str;
            str.sprintf("%s:%d:%d: %s: ", fileName->chars(), line, column, pretty[level]);
            errors->append(str.toUtf8());

            str.vsprintf(format, ap);
            errors->append(str.toUtf8());

            errors->append('\n');
        }
    };

    Document::Ptr document(const QString &fileName, QByteArray *errors = 0)
    {
        Document::Ptr doc = Document::create(fileName);
        QFile file(testdata(fileName));
        if (file.open(QFile::ReadOnly)) {
            Client client(errors);
            doc->control()->setDiagnosticClient(&client);
            doc->setUtf8Source(QTextStream(&file).readAll().toUtf8());
            doc->translationUnit()->setCxxOxEnabled(true);
            doc->check();
            doc->control()->setDiagnosticClient(0);
        }
        return doc;
    }

private Q_SLOTS:
    //
    // checks for the syntax
    //
    void inlineNamespace_data();
    void inlineNamespace();
    void staticAssert();
    void staticAssert_data();
    void noExcept();
    void noExcept_data();

    //
    // checks for the semantic
    //
    void inlineNamespaceLookup();
};


void tst_cxx11::inlineNamespace_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("inlineNamespace.1") << "inlineNamespace.1.cpp" << "inlineNamespace.1.errors.txt";
}

void tst_cxx11::inlineNamespace()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QByteArray errors;
    Document::Ptr doc = document(file, &errors);

    if (! qgetenv("DEBUG").isNull())
        printf("%s\n", errors.constData());

    VERIFY_ERRORS();
}

//
// check the visibility of symbols declared inside inline namespaces
//
void tst_cxx11::inlineNamespaceLookup()
{
    Document::Ptr doc = document("inlineNamespace.1.cpp");
    Snapshot snapshot;
    snapshot.insert(doc);

    LookupContext context(doc, snapshot);
    QSharedPointer<Control> control = context.control();

    QList<LookupItem> results = context.lookup(control->identifier("foo"), doc->globalNamespace());
    QCOMPARE(results.size(), 1); // the symbol is visible from the global scope
}

void tst_cxx11::staticAssert_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("staticAssert.1") << "staticAssert.1.cpp" << "staticAssert.1.errors.txt";
}

void tst_cxx11::staticAssert()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QByteArray errors;
    Document::Ptr doc = document(file, &errors);

    if (! qgetenv("DEBUG").isNull())
        printf("%s\n", errors.constData());

    VERIFY_ERRORS();
}

void tst_cxx11::noExcept_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("noExcept.1") << "noExcept.1.cpp" << "noExcept.1.errors.txt";
}

void tst_cxx11::noExcept()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QByteArray errors;
    Document::Ptr doc = document(file, &errors);

    if (! qgetenv("DEBUG").isNull())
        printf("%s\n", errors.constData());

    VERIFY_ERRORS();
}

QTEST_APPLESS_MAIN(tst_cxx11)
#include "tst_cxx11.moc"
