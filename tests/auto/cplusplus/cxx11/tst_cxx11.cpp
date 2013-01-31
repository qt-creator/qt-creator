/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
      QByteArray expectedErrors; \
      if (!errorFile.isEmpty()) { \
        QFile e(testdata(errorFile)); \
        if (e.open(QFile::ReadOnly)) \
          expectedErrors = QTextStream(&e).readAll().toUtf8(); \
      } \
      QCOMPARE(QString::fromLatin1(errors), QString::fromLatin1(expectedErrors)); \
    } while (0)


class tst_cxx11: public QObject
{
    Q_OBJECT

    /*
        Returns the path to some testdata file or directory.
    */
    static QString testdata(const QString &name = QString())
    {
        static const QString dataDirectory = QLatin1String(SRCDIR "/data");

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
        } else {
            qWarning() << "could not read file" << fileName;
        }
        return doc;
    }

private Q_SLOTS:
    //
    // checks for the syntax
    //
    void parse_data();
    void parse();

    //
    // checks for the semantic
    //
    void inlineNamespaceLookup();
};


void tst_cxx11::parse_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("inlineNamespace.1") << "inlineNamespace.1.cpp" << "inlineNamespace.1.errors.txt";
    QTest::newRow("staticAssert.1") << "staticAssert.1.cpp" << "staticAssert.1.errors.txt";
    QTest::newRow("noExcept.1") << "noExcept.1.cpp" << "noExcept.1.errors.txt";
    QTest::newRow("braceInitializers.1") << "braceInitializers.1.cpp" << "braceInitializers.1.errors.txt";
    QTest::newRow("braceInitializers.2") << "braceInitializers.2.cpp" << "";
    QTest::newRow("braceInitializers.3") << "braceInitializers.3.cpp" << "";
    QTest::newRow("defaultdeleteInitializer.1") << "defaultdeleteInitializer.1.cpp" << "";
    QTest::newRow("refQualifier.1") << "refQualifier.1.cpp" << "";
    QTest::newRow("alignofAlignas.1") << "alignofAlignas.1.cpp" << "";
    QTest::newRow("rangeFor.1") << "rangeFor.1.cpp" << "";
    QTest::newRow("aliasDecl.1") << "aliasDecl.1.cpp" << "";
    QTest::newRow("enums.1") << "enums.1.cpp" << "";
    QTest::newRow("templateGreaterGreater.1") << "templateGreaterGreater.1.cpp" << "";
    QTest::newRow("packExpansion.1") << "packExpansion.1.cpp" << "";
    QTest::newRow("declType.1") << "declType.1.cpp" << "";
}

void tst_cxx11::parse()
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

QTEST_APPLESS_MAIN(tst_cxx11)
#include "tst_cxx11.moc"
