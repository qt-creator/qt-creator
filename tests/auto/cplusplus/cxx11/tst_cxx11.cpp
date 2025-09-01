// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cplusplus/CPlusPlus.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QFile>
#include <QObject>
#include <QTest>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;
using namespace Utils;

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

        void report(int level,
                    const StringLiteral *fileName,
                    int line, int column,
                    const char *format, va_list ap) override
        {
            if (! errors)
                return;

            static const char *const pretty[] = {"warning", "error", "fatal"};

            QString str = QString::asprintf("%s:%d:%d: %s: ", fileName->chars(), line, column, pretty[level]);
            errors->append(str.toUtf8());

            str += QString::vasprintf(format, ap);
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
        bool preVisit(Symbol *) override { return !m_function; }

        bool visit(Function *function) override
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
        doc->control()->setDiagnosticClient(&client, true);
        doc->setUtf8Source(source);
        doc->translationUnit()->setLanguageFeatures(languageFeatures);
        doc->check();
        doc->control()->setDiagnosticClient(0, false);
    }

    Document::Ptr document(
        const QString &fileName,
        int cxxVersion = 2011,
        QByteArray *errors = 0,
        bool c99Enabled = false)
    {
        QFile file(testdata(fileName));
        if (!file.open(QFile::ReadOnly)) {
            qDebug() << file.errorString() << fileName;
            return {};
        }
        const Document::Ptr doc = Document::create(FilePath::fromString(fileName));
        LanguageFeatures features;
        features.cxxEnabled = true;
        features.cxx11Enabled = true;
        if (cxxVersion >= 2014)
            features.cxx14Enabled = true;
        if (cxxVersion >= 2017)
            features.cxx17Enabled = true;
        if (cxxVersion >= 2020)
            features.cxx20Enabled = true;
        if (cxxVersion >= 2023)
            features.cxx23Enabled = true;
        features.c99Enabled = c99Enabled;
        processDocument(doc, QTextStream(&file).readAll().toUtf8(), features, errors);
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
    QTest::addColumn<int>("cxxVersion");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("inlineNamespace.1") << "inlineNamespace.1.cpp" << 2011 << "inlineNamespace.1.errors.txt";
    QTest::newRow("nestedNamespace.1") << "nestedNamespace.1.cpp" << 2011 << "nestedNamespace.1.errors.txt";
    QTest::newRow("staticAssert.1") << "staticAssert.1.cpp" << 2011 << "staticAssert.1.errors.txt";
    QTest::newRow("noExcept.1") << "noExcept.1.cpp" << 2011 << "noExcept.1.errors.txt";
    QTest::newRow("braceInitializers.1") << "braceInitializers.1.cpp" << 2011 << "braceInitializers.1.errors.txt";
    QTest::newRow("braceInitializers.2") << "braceInitializers.2.cpp" << 2011 << "";
    QTest::newRow("braceInitializers.3") << "braceInitializers.3.cpp" << 2011 << "";
    QTest::newRow("defaultdeleteInitializer.1") << "defaultdeleteInitializer.1.cpp" << 2011 << "";
    QTest::newRow("refQualifier.1") << "refQualifier.1.cpp" << 2011 << "";
    QTest::newRow("alignofAlignas.1") << "alignofAlignas.1.cpp" << 2011 << "";
    QTest::newRow("rangeFor.1") << "rangeFor.1.cpp" << 2011 << "";
    QTest::newRow("rangeFor.2") << "rangeFor.2.cpp" << 2011 << "";
    QTest::newRow("rangeFor.3") << "rangeFor.3.cpp" << 2020 << "";
    QTest::newRow("aliasDecl.1") << "aliasDecl.1.cpp" << 2011 << "";
    QTest::newRow("enums.1") << "enums.1.cpp" << 2011 << "";
    QTest::newRow("templateGreaterGreater.1") << "templateGreaterGreater.1.cpp" << 2011 << "";
    QTest::newRow("packExpansion.1") << "packExpansion.1.cpp" << 2011 << "";
    QTest::newRow("declType.1") << "declType.1.cpp" << 2011 << "";
    QTest::newRow("threadLocal.1") << "threadLocal.1.cpp" << 2011 << "";
    QTest::newRow("trailingtypespec.1") << "trailingtypespec.1.cpp" << 2011 << "";
    QTest::newRow("lambda.2") << "lambda.2.cpp" << 2011 << "";
    QTest::newRow("userDefinedLiterals.1") << "userDefinedLiterals.1.cpp" << 2011 << "";
    QTest::newRow("userDefinedLiterals.2") << "userDefinedLiterals.2.cpp" << 2011 << "";
    QTest::newRow("rawstringliterals") << "rawstringliterals.cpp" << 2011 << "";
    QTest::newRow("friends") << "friends.cpp" << 2011 << "";
    QTest::newRow("attributes") << "attributes.cpp" << 2011 << "";
    QTest::newRow("foldExpressions") << "foldExpressions.cpp" << 2017 << "";
    QTest::newRow("explicitObjectParameters") << "explicitObjParam.cpp" << 2023 << "";
    QTest::newRow("placeholderReturnType") << "placeholderReturnType.cpp" << 2014 << "";
    QTest::newRow("builtinTypeTraits") << "builtinTypeTraits.cpp" << 2020 << "";
    QTest::newRow("templateTemplateTypeInDependentName")
        << "templateTemplateTypeInDependentName.cpp" << 2011 << "";
    QTest::newRow("constevalIf") << "constevalIf.cpp" << 2023 << "";
    QTest::newRow("int128") << "int128.cpp" << 2011 << "";
    QTest::newRow("namedConstantTemplateParameter")
        << "namedConstantTemplateParam.cpp" << 2020 << "";
    QTest::newRow("declTypeInBaseClause") << "declTypeInBaseClause.1.cpp" << 2020 << "";
    QTest::newRow("declTypeInTemplateBaseClause") << "declTypeInBaseClause.2.cpp" << 2020 << "";
    QTest::newRow("ifWithInit") << "ifWithInit.cpp" << 2020 << "";
    QTest::newRow("genericLambdas") << "genericLambdas.cpp" << 2020 << "";
    QTest::newRow("coroutines") << "coroutines.cpp" << 2020 << "";
    QTest::newRow("requiresClause") << "requiresClause.cpp" << 2020 << "";
    QTest::newRow("concepts.1") << "concepts.1.cpp" << 2020 << "";
    QTest::newRow("concepts.2") << "concepts.2.cpp" << 2020 << "";
}

void tst_cxx11::parse()
{
    QFETCH(QString, file);
    QFETCH(int, cxxVersion);
    QFETCH(QString, errorFile);

    QByteArray errors;
    Document::Ptr doc = document(file, cxxVersion, &errors);
    QVERIFY(doc);

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
    Document::Ptr doc = document(file, 2011, &errors, c99Enabled);
    QVERIFY(doc);

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
    QVERIFY(doc);
    Snapshot snapshot;
    snapshot.insert(doc);

    LookupContext context(doc, snapshot);
    std::shared_ptr<Control> control = context.bindings()->control();

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
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
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
