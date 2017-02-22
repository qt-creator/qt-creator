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

#include <cplusplus/CPlusPlus.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QtTest>
#include <QObject>
#include <QFile>

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

inline QString _(const QByteArray &ba) { return QString::fromUtf8(ba, ba.size()); }

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

    struct Client: DiagnosticClient {
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

            static const char *const pretty[] = {"warning", "error", "fatal"};

            QString str;
            str.sprintf("%s:%d:%d: %s: ", fileName->chars(), line, column, pretty[level]);
            errors->append(str.toUtf8());

            str.vsprintf(format, ap);
            errors->append(str.toUtf8());

            errors->append('\n');
        }
    };

    class FindLambdaFunction : public SymbolVisitor
    {
    public:
        FindLambdaFunction() : m_function(0) {}

        Function *operator()(const Document::Ptr &document)
        {
            accept(document->globalNamespace());
            return m_function;
        }

    private:
        bool preVisit(Symbol *) { return !m_function; }

        bool visit(Function *function)
        {
            if (function->name())
                return true;

            m_function = function;
            return false;
        }

    private:
        Function *m_function;
    };

    static void processDocument(const Document::Ptr doc, QByteArray source,
                                LanguageFeatures languageFeatures, QByteArray *errors)
    {
        Client client(errors);
        doc->control()->setDiagnosticClient(&client);
        doc->setUtf8Source(source);
        doc->translationUnit()->setLanguageFeatures(languageFeatures);
        doc->check();
        doc->control()->setDiagnosticClient(0);
    }

    Document::Ptr document(const QString &fileName, QByteArray *errors = 0, bool c99Enabled = false)
    {
        Document::Ptr doc = Document::create(fileName);
        QFile file(testdata(fileName));
        if (file.open(QFile::ReadOnly)) {
            LanguageFeatures features;
            features.cxx11Enabled = true;
            features.cxxEnabled = true;
            features.c99Enabled = c99Enabled;
            processDocument(doc, QTextStream(&file).readAll().toUtf8(), features, errors);
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

    void parseWithC99Enabled_data();
    void parseWithC99Enabled();

    //
    // checks for the semantic
    //
    void inlineNamespaceLookup();

    void lambdaType_data();
    void lambdaType();
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
    QTest::newRow("threadLocal.1") << "threadLocal.1.cpp" << "";
    QTest::newRow("trailingtypespec.1") << "trailingtypespec.1.cpp" << "";
    QTest::newRow("lambda.2") << "lambda.2.cpp" << "";
    QTest::newRow("userDefinedLiterals.1") << "userDefinedLiterals.1.cpp" << "";
    QTest::newRow("rawstringliterals") << "rawstringliterals.cpp" << "";
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

void tst_cxx11::parseWithC99Enabled_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("lambda.1") << "lambda.1.cpp" << "";
}

void tst_cxx11::parseWithC99Enabled()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    const bool c99Enabled = true;
    QByteArray errors;
    Document::Ptr doc = document(file, &errors, c99Enabled);

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
    QSharedPointer<Control> control = context.bindings()->control();

    QList<LookupItem> results = context.lookup(control->identifier("foo"), doc->globalNamespace());
    QCOMPARE(results.size(), 1); // the symbol is visible from the global scope
}

void tst_cxx11::lambdaType_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expectedType");

    QTest::newRow("basic1")
        << _("void f()\n"
             "{\n"
             "    [](){};\n"
             "}\n")
        << _("void ()");

    QTest::newRow("basic2")
        << _("class C {\n"
             "    void f()\n"
             "    {\n"
             "        [](){};\n"
             "    }\n"
             "};\n")
        << _("void ()");

    QTest::newRow("trailing return type")
        << _("void f()\n"
             "{\n"
             "    []() -> int { return 0; };\n"
             "}\n")
        << _("int ()");

    QTest::newRow("return expression")
        << _("void f()\n"
             "{\n"
             "    []() { return true; };\n"
             "}\n")
        << _("bool ()");
}

void tst_cxx11::lambdaType()
{
    QFETCH(QString, source);
    QFETCH(QString, expectedType);

    LanguageFeatures features;
    features.cxx11Enabled = true;
    features.cxxEnabled = true;

    QByteArray errors;
    Document::Ptr doc = Document::create(QLatin1String("testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);

    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug() << errors;
    QVERIFY(!hasErrors);

    Function *function = FindLambdaFunction()(doc);
    QVERIFY(function);

    Overview oo;
    oo.showReturnTypes = true;

    QEXPECT_FAIL("return expression", "Not implemented", Abort);
    QCOMPARE(oo.prettyType(function->type()), expectedType);
}

QTEST_APPLESS_MAIN(tst_cxx11)
#include "tst_cxx11.moc"
