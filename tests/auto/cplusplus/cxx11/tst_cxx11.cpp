// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cplusplus/CPlusPlus.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QtTest>
#include <QObject>
#include <QFile>

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

        virtual void report(int level,
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
        Document::Ptr doc = Document::create(FilePath::fromString(fileName));
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

    void concepts();
    void requiresClause();
    void coroutines();
    void genericLambdas();
    void ifStatementWithInitialization();
};


void tst_cxx11::parse_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("inlineNamespace.1") << "inlineNamespace.1.cpp" << "inlineNamespace.1.errors.txt";
    QTest::newRow("nestedNamespace.1") << "nestedNamespace.1.cpp" << "nestedNamespace.1.errors.txt";
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

void tst_cxx11::concepts()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx20Enabled = true;

    const QString source = R"(
template<typename T> concept IsPointer = requires(T p) { *p; };
template<IsPointer T> void* func(T p) { return p; }
void *func2(IsPointer auto p)
{
    return p;
}
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::requiresClause()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx20Enabled = true;

    const QString source = R"(
template<class T> constexpr bool is_meowable = true;
template<class T> constexpr bool is_purrable() { return true; }
template<class T> void f(T) requires is_meowable<T>;
template<class T> requires is_meowable<T> void g(T) ;
template<class T> void h(T) requires (is_purrable<T>());
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::coroutines()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx20Enabled = true;

    const QString source = R"(
struct promise;
struct coroutine : std::coroutine_handle<promise>
{
    using promise_type = struct promise;
};
struct promise
{
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};
struct S
{
    int i;
    coroutine f()
    {
        std::cout << i;
        co_return;
    }
};
void good()
{
    coroutine h = [](int i) -> coroutine
    {
        std::cout << i;
        co_return;
    }(0);
    h.resume();
    h.destroy();
}
auto switch_to_new_thread(std::jthread& out)
{
    struct awaitable
    {
        std::jthread* p_out;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h)
        {
            std::jthread& out = *p_out;
            if (out.joinable())
                throw std::runtime_error("Output jthread parameter not empty");
            out = std::jthread([h] { h.resume(); });
            std::cout << "New thread ID: " << out.get_id() << '\n'; // this is OK
        }
        void await_resume() {}
    };
    return awaitable{&out};
}
struct task
{
    struct promise_type
    {
        task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};
task resuming_on_new_thread(std::jthread& out)
{
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    co_await switch_to_new_thread(out);
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << '\n';
}
void run()
{
    std::jthread out;
    resuming_on_new_thread(out);
}
template <typename T>
struct Generator
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    struct promise_type // required
    {
        T value_;
        std::exception_ptr exception_;

        Generator get_return_object()
        {
            return Generator(handle_type::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = std::current_exception(); }
        template <std::convertible_to<T> From>
        std::suspend_always yield_value(From&& from)
        {
            value_ = std::forward<From>(from);
            return {};
        }
        void return_void() { }
    };
    handle_type h_;
    Generator(handle_type h) : h_(h) {}
    ~Generator() { h_.destroy(); }
    explicit operator bool()
    {
        fill();
        return !h_.done();
    }
    T operator()()
    {
        fill();
        full_ = false;
        return std::move(h_.promise().value_);
    }
private:
    bool full_ = false;
    void fill()
    {
        if (!full_)
        {
            h_();
            if (h_.promise().exception_)
                std::rethrow_exception(h_.promise().exception_);
            full_ = true;
        }
    }
};
Generator<std::uint64_t>
fibonacci_sequence(unsigned n)
{
    if (n == 0)
        co_return;
    if (n > 94)
        throw std::runtime_error("Too big Fibonacci sequence. Elements would overflow.");
    co_yield 0;
    if (n == 1)
        co_return;
    co_yield 1;
    if (n == 2)
        co_return;
    std::uint64_t a = 0;
    std::uint64_t b = 1;
    for (unsigned i = 2; i < n; i++)
    {
        std::uint64_t s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}
int main()
{
    try
    {
        auto gen = fibonacci_sequence(10); // max 94 before uint64_t overflows
        for (int j = 0; gen; j++)
            std::cout << "fib(" << j << ")=" << gen() << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
    }
}
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::genericLambdas()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx20Enabled = true;

    const QString source = R"(
template <typename T> concept C1 = true;
template <std::size_t N> concept C2 = true;
template <typename A, typename B> concept C3 = true;
int main()
{
    auto f = []<class T>(T a, auto&& b) { return a < b; };
    auto g = []<typename... Ts>(Ts&&... ts) { return foo(std::forward<Ts>(ts)...); };
    auto h = []<typename T1, C1 T2> requires C2<sizeof(T1) + sizeof(T2)>
         (T1 a1, T1 b1, T2 a2, auto a3, auto a4) requires C3<decltype(a4), T2> {
    };
}
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::ifStatementWithInitialization()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx17Enabled = true;

    const QString source = R"(
int main()
{
    if (bool b = true; b)
        b = false;
}
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

QTEST_APPLESS_MAIN(tst_cxx11)
#include "tst_cxx11.moc"
