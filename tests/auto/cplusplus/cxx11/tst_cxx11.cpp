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
        Document::Ptr doc = Document::create(FilePath::fromString(fileName));
        QFile file(testdata(fileName));
        if (file.open(QFile::ReadOnly)) {
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
    void complexConcepts();
    void requiresClause();
    void coroutines();
    void genericLambdas();
    void ifStatementWithInitialization();
    void rangeBasedForWithoutInitialization();
    void rangeBasedForWithInitialization();
    void decltypeInBaseClause();
    void decltypeInTemplateBaseClause();
    void namedConstantTemplateParameter();
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
}

void tst_cxx11::parse()
{
    QFETCH(QString, file);
    QFETCH(int, cxxVersion);
    QFETCH(QString, errorFile);

    QByteArray errors;
    Document::Ptr doc = document(file, cxxVersion, &errors);

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

void tst_cxx11::complexConcepts()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx17Enabled = features.cxx20Enabled
        = true;

    // Extend as needed.
    const QString source = R"(
template <typename _Tp, _Tp __v> struct integral_constant {
    static constexpr _Tp value = __v;
    using value_type = _Tp;
    using type = integral_constant<_Tp, __v>;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};
template<bool __v> using __bool_constant = integral_constant<bool, __v>;
using true_type =  __bool_constant<true>;
using false_type = __bool_constant<false>;
template<typename _Tp> struct is_void : public false_type { };
template<> struct is_void<void> : public true_type { };
template<> struct is_void<const void> : public true_type { };
template<> struct is_void<volatile void> : public true_type { };
template<> struct is_void<const volatile void> : public true_type { };
template<typename> struct is_array : public false_type { };
template<typename _Tp, char8_t _Size>
struct is_array<_Tp[_Size]> : public true_type { };
template<typename _Tp> struct is_array<_Tp[]> : public true_type { };
template<typename> struct is_const : public false_type { };
template<typename _Tp> struct is_const<_Tp const> : public true_type { };
template<typename _Tp>
struct is_function : public __bool_constant<!is_const<const _Tp>::value> { };
template<typename _Tp> struct is_function<_Tp&> : public false_type { };
template<typename _Tp> struct is_function<_Tp&&> : public false_type { };
template <typename _Tp> inline constexpr bool is_array_v = false;
template <typename _Tp> inline constexpr bool is_array_v<_Tp[]> = true;
template <typename _Tp, unsigned long long _Num>
inline constexpr bool is_array_v<_Tp[_Num]> = true;
template <typename> struct is_lvalue_reference : public false_type {};
template <typename _Tp> struct is_lvalue_reference<_Tp &> : public true_type {};
template <typename _Tp> inline constexpr bool is_lvalue_reference_v = false;
template <typename _Tp> inline constexpr bool is_lvalue_reference_v<_Tp &> = true;
template <typename _Tp, typename _Up> inline constexpr bool is_same_v = false;
template <typename _Tp> inline constexpr bool is_same_v<_Tp, _Tp> = true;
template<bool, typename _Tp = void> struct enable_if {};
template<typename _Tp> struct enable_if<true, _Tp> { using type = _Tp; };
template<bool _Cond, typename _Tp = void>
using __enable_if_t = typename enable_if<_Cond, _Tp>::type;
template<typename _Tp, typename...> using __first_t = _Tp;
template<typename... _Bn> auto __or_fn(int) -> __first_t<false_type,
__enable_if_t<!bool(_Bn::value)>...>;
template<typename... _Bn> auto __or_fn(...) -> true_type;
template<typename... _Bn> struct __or_ : decltype(__or_fn<_Bn...>(0)) { };
template<typename... _Tp> struct common_reference;
template<typename... _Tp> using common_reference_t = typename common_reference<_Tp...>::type;
template<> struct common_reference<> { };
template<typename _Tp> struct __declval_protector {
    static const bool __stop = false;
};
template<typename _Tp>
auto declval() noexcept -> decltype(__declval<_Tp>(0))
{
    static_assert(__declval_protector<_Tp>::__stop,
                  "declval() must not be used!");
    return __declval<_Tp>(0);
}
template<typename _Tp0> struct common_reference<_Tp0> { using type = _Tp0; };
template<typename _Tp, typename _Up> concept __same_as = is_same_v<_Tp, _Up>;
template<typename _Tp, typename _Up> concept same_as = __same_as<_Tp, _Up> && __same_as<_Up, _Tp>;
template<typename _From,
         typename _To,
         bool = __or_<is_void<_From>, is_function<_To>, is_array<_To>>::value>
struct __is_convertible_helper
{
    using type = typename is_void<_To>::type;
};
template<typename _From, typename _To>
struct is_convertible : public __is_convertible_helper<_From, _To>::type
{};
template<typename _From, typename _To>
inline constexpr bool is_convertible_v = is_convertible<_From, _To>::value;
template<typename _From, typename _To>
concept convertible_to = is_convertible_v<_From, _To>
                         && requires { static_cast<_To>(declval<_From>()); };
template<typename _Tp> struct remove_reference { using type = _Tp; };
template<typename _Tp> struct remove_reference<_Tp &> { using type = _Tp; };
template<typename _Tp> struct remove_reference<_Tp &&> { using type = _Tp; };
template<typename _Tp> using remove_reference_t = typename remove_reference<_Tp>::type;
template<typename _Tp> using __cref = const remove_reference_t<_Tp> &;
template<typename _Tp, typename _Up>
concept common_reference_with = same_as<common_reference_t<_Tp, _Up>, common_reference_t<_Up, _Tp>>
                                && convertible_to<_Tp, common_reference_t<_Tp, _Up>>
                                && convertible_to<_Up, common_reference_t<_Tp, _Up>>;
template<typename _Lhs, typename _Rhs>
concept assignable_from = is_lvalue_reference_v<_Lhs>
                          && common_reference_with<__cref<_Lhs>, __cref<_Rhs>>
                          && requires(_Lhs __lhs, _Rhs &&__rhs) {
                                 { __lhs = static_cast<_Rhs &&>(__rhs) } -> same_as<_Lhs>;
                             };
using size_t = unsigned long;
template<typename _Type>
struct __type_identity
{
    using type = _Type;
};
template<typename _Tp>
using __type_identity_t = typename __type_identity<_Tp>::type;
template<typename _Tp, size_t = sizeof(_Tp)>
constexpr true_type __is_complete_or_unbounded(__type_identity<_Tp>)
{
    return {};
}

template<typename _TypeIdentity, typename _NestedType = typename _TypeIdentity::type>
constexpr typename __or_<is_reference<_NestedType>,
                         is_function<_NestedType>,
                         is_void<_NestedType>,
                         __is_array_unknown_bounds<_NestedType>>::type
__is_complete_or_unbounded(_TypeIdentity)
{
    return {};
}

template<typename _Tp, typename = void>
struct __add_lvalue_reference_helper
{
    using type = _Tp;
};

template<typename _Tp>
struct __add_lvalue_reference_helper<_Tp, __void_t<_Tp &>>
{
    using type = _Tp &;
};

template<typename _Tp>
using __add_lval_ref_t = typename __add_lvalue_reference_helper<_Tp>::type;

template<typename _Tp, typename... _Args>
using __is_nothrow_constructible_impl = __bool_constant<__is_nothrow_constructible(_Tp, _Args...)>;
template<typename _Tp>
struct is_nothrow_copy_constructible
    : public __is_nothrow_constructible_impl<_Tp, __add_lval_ref_t<const _Tp>>
{
    static_assert(__is_complete_or_unbounded(__type_identity<_Tp>{}),
                  "template argument must be a complete class or an unbounded array");
};

template<typename _Tp, typename... _Args>
using __is_constructible_impl = __bool_constant<__is_constructible(_Tp, _Args...)>;
/// @endcond

/// is_constructible
template<typename _Tp, typename... _Args>
struct is_constructible : public __is_constructible_impl<_Tp, _Args...>
{
    static_assert(__is_complete_or_unbounded(__type_identity<_Tp>{}),
                  "template argument must be a complete class or an unbounded array");
};

template<typename _Tp>
inline constexpr bool is_copy_constructible_v = __is_constructible(_Tp, __add_lval_ref_t<const _Tp>);
template<typename _Tp>
inline constexpr bool is_nothrow_copy_constructible_v
    = __is_nothrow_constructible(_Tp, __add_lval_ref_t<const _Tp>);

template<typename _Tp, typename... _Args>
inline constexpr bool is_constructible_v = __is_constructible(_Tp, _Args...);

template<typename _Tp>
constexpr bool __destructible_impl = false;
template<typename _Tp>
    requires requires(_Tp &__t) {
        { __t.~_Tp() } noexcept;
    }
constexpr bool __destructible_impl<_Tp> = true;

template<typename _Tp>
constexpr bool __destructible = __destructible_impl<_Tp>;
template<typename _Tp>
constexpr bool __destructible<_Tp &> = true;
template<typename _Tp>
constexpr bool __destructible<_Tp &&> = true;
template<typename _Tp, size_t _Nm>
constexpr bool __destructible<_Tp[_Nm]> = __destructible<_Tp>;

template<typename _Tp>
concept destructible = __destructible<_Tp>;

/// [concept.constructible], concept constructible_from
template<typename _Tp, typename... _Args>
concept constructible_from = destructible<_Tp> && is_constructible_v<_Tp, _Args...>;

template<typename _Tp>
concept move_constructible = constructible_from<_Tp, _Tp> && convertible_to<_Tp, _Tp>;

template<typename _Tp>
concept movable = is_object_v<_Tp> && move_constructible<_Tp> && assignable_from<_Tp &, _Tp>
                  && swappable<_Tp>;

template<typename _Tp>
concept copy_constructible = move_constructible<_Tp> && constructible_from<_Tp, _Tp &>
                             && convertible_to<_Tp &, _Tp> && constructible_from<_Tp, const _Tp &>
                             && convertible_to<const _Tp &, _Tp>
                             && constructible_from<_Tp, const _Tp>
                             && convertible_to<const _Tp, _Tp>;

template<typename _Tp>
concept copyable = copy_constructible<_Tp> && movable<_Tp> && assignable_from<_Tp &, _Tp &>
                   && assignable_from<_Tp &, const _Tp &> && assignable_from<_Tp &, const _Tp>;

template<typename _Tp>
concept __boxable = copy_constructible<_Tp> && is_object_v<_Tp>;
template<__boxable _Tp>
struct __box : std::optional<_Tp>
{
    using std::optional<_Tp>::optional;

    constexpr __box() noexcept(is_nothrow_default_constructible_v<_Tp>)
        requires default_initializable<_Tp>
        : std::optional<_Tp>{std::in_place}
    {}

    __box(const __box &) = default;
    __box(__box &&) = default;

    using std::optional<_Tp>::operator=;

    constexpr __box &operator=(const __box &__that) noexcept(is_nothrow_copy_constructible_v<_Tp>)
        requires(!copyable<_Tp>) && copy_constructible<_Tp>
    {
        if (this != std::__addressof(__that)) {
            if ((bool) __that)
                this->emplace(*__that);
            else
                this->reset();
        }
        return *this;
    }
};
template<typename _Tp>
concept __boxable_copyable = copy_constructible<_Tp>
                             && (copyable<_Tp>
                                 || (is_nothrow_move_constructible_v<_Tp>
                                     && is_nothrow_copy_constructible_v<_Tp>) );
template<typename _Tp>
concept __boxable_movable = (!copy_constructible<_Tp>)
                            && (movable<_Tp> || is_nothrow_move_constructible_v<_Tp>);

template<__boxable _Tp>
    requires __boxable_copyable<_Tp> || __boxable_movable<_Tp>
struct __box<_Tp>
{
private:
    [[no_unique_address]] _Tp _M_value = _Tp();

public:
    __box()
        requires default_initializable<_Tp>
    = default;

    constexpr explicit __box(const _Tp &__t) noexcept(is_nothrow_copy_constructible_v<_Tp>)
        requires copy_constructible<_Tp>
        : _M_value(__t)
    {}

    constexpr explicit __box(_Tp &&__t) noexcept(is_nothrow_move_constructible_v<_Tp>)
        : _M_value(std::move(__t))
    {}

    template<typename... _Args>
        requires constructible_from<_Tp, _Args...>
    constexpr explicit __box(in_place_t,
                             _Args &&...__args) noexcept(is_nothrow_constructible_v<_Tp, _Args...>)
        : _M_value(std::forward<_Args>(__args)...)
    {}

    __box(const __box &) = default;
    __box(__box &&) = default;
    __box &operator=(const __box &)
        requires copyable<_Tp>
    = default;
    __box &operator=(__box &&)
        requires movable<_Tp>
    = default;

    // When _Tp is nothrow_copy_constructible but not copy_assignable,
    // copy assignment is implemented via destroy-then-copy-construct.
    constexpr __box &operator=(const __box &__that) noexcept
        requires(!copyable<_Tp>) && copy_constructible<_Tp>
    {
        static_assert(is_nothrow_copy_constructible_v<_Tp>);
        if (this != std::__addressof(__that)) {
            _M_value.~_Tp();
            std::construct_at(std::__addressof(_M_value), *__that);
        }
        return *this;
    }

    constexpr __box &operator=(__box &&__that) noexcept
        requires(!movable<_Tp>)
    {
        static_assert(is_nothrow_move_constructible_v<_Tp>);
        if (this != std::__addressof(__that)) {
            _M_value.~_Tp();
            std::construct_at(std::__addressof(_M_value), std::move(*__that));
        }
        return *this;
    }

    constexpr bool has_value() const noexcept { return true; }
    constexpr _Tp &operator*() & noexcept { return _M_value; }
    constexpr const _Tp &operator*() const & noexcept { return _M_value; }
    constexpr _Tp &&operator*() && noexcept { return std::move(_M_value); }
    constexpr const _Tp &&operator*() const && noexcept { return std::move(_M_value); }
    constexpr _Tp *operator->() noexcept { return std::__addressof(_M_value); }
    constexpr const _Tp *operator->() const noexcept { return std::__addressof(_M_value); }
template<copy_constructible _Tp>
    requires is_object_v<_Tp>
class single_view : public view_interface<single_view<_Tp>>
{
public:
    single_view()
        requires default_initializable<_Tp>
    = default;

    constexpr explicit single_view(const _Tp &__t) noexcept(is_nothrow_copy_constructible_v<_Tp>)
        requires copy_constructible<_Tp>
        : _M_value(__t)
    {}

    constexpr explicit single_view(_Tp &&__t) noexcept(is_nothrow_move_constructible_v<_Tp>)
        : _M_value(std::move(__t))
    {}

    template<typename... _Args>
        requires constructible_from<_Tp, _Args...>
    constexpr explicit single_view(in_place_t, _Args &&...__args) noexcept(
        is_nothrow_constructible_v<_Tp, _Args...>)
        : _M_value{in_place, std::forward<_Args>(__args)...}
    {}

    constexpr _Tp *begin() noexcept { return data(); }
    constexpr const _Tp *begin() const noexcept { return data(); }
    constexpr _Tp *end() noexcept { return data() + 1; }
    constexpr const _Tp *end() const noexcept { return data() + 1; }
    static constexpr bool empty() noexcept { return false; }
    static constexpr size_t size() noexcept { return 1; }
    constexpr _Tp *data() noexcept { return _M_value.operator->(); }
    constexpr const _Tp *data() const noexcept { return _M_value.operator->(); }
private:
    [[no_unique_address]] __detail::__box<_Tp> _M_value;
};

template<typename _Tp>
single_view(_Tp) -> single_view<_Tp>;
};
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

void tst_cxx11::rangeBasedForWithoutInitialization()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx17Enabled = features.cxx20Enabled
        = true;

    const QString source = R"(
int main()
{
    std::vector<int> v;
    using Alias = int;
    for (Alias i : v)
        return 0;
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

void tst_cxx11::rangeBasedForWithInitialization()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx17Enabled = features.cxx20Enabled
        = true;

    const QString source = R"(
int main()
{
    for (std::string s = "test"; char c : s)
        return 0;
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

void tst_cxx11::decltypeInBaseClause()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = true;

    const QString source = R"(
struct Bar{};
struct Foo : public decltype(Bar()) {};
)";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::decltypeInTemplateBaseClause()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = true;

    const QString source = "template<typename T> struct X : decltype(T()) {};\n";
    QByteArray errors;
    Document::Ptr doc = Document::create(FilePath::fromPathPart(u"testFile"));
    processDocument(doc, source.toUtf8(), features, &errors);
    const bool hasErrors = !errors.isEmpty();
    if (hasErrors)
        qDebug().noquote() << errors;
    QVERIFY(!hasErrors);
}

void tst_cxx11::namedConstantTemplateParameter()
{
    LanguageFeatures features;
    features.cxxEnabled = true;
    features.cxx11Enabled = features.cxx14Enabled = features.cxx17Enabled = features.cxx20Enabled
        = true;
    const QString source = R"(
using size_t = unsigned long;
template <typename _Tp, size_t = sizeof(_Tp)> struct S {};
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
