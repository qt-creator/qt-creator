/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

//
// KEEP THIS FILE COMPILABLE TO ENSURE THAT WE TEST AGAINST VALID CODE.
//

// NOTE: Nice, includes are sorted
#include <QCoreApplication>
#include <QFile>
#include <QLoggingCategory>
#include <QObject>
#include <QVector>

#include <functional>
#include <iostream>
#include <vector>

static int aGlobalInt = 3;

// -------------------------------------------------------------------------------------------------
// Preprocessor
// -------------------------------------------------------------------------------------------------

// NOTE: Ops, preprocessor branches are not indented ("#define" vs "#  define")
#if defined(TEXTEDITOR_LIBRARY)
#define TEXTEDITOR_EXPORT Q_DECL_EXPORT
#else
#define TEXTEDITOR_EXPORT Q_DECL_IMPORT
#endif

// -------------------------------------------------------------------------------------------------
// Macros
// -------------------------------------------------------------------------------------------------

#define QTCREATOR_UTILS_EXPORT

// qtcassert.h:
namespace Utils {
QTCREATOR_UTILS_EXPORT void writeAssertLocation(const char *msg);
}

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) \
    ::Utils::writeAssertLocation("\"" cond "\" in file " __FILE__ \
                                 ", line " QTC_ASSERT_STRINGIFY(__LINE__))

// NOTE: Ops, macro definitions: are more verbose
#define QTC_ASSERT(cond, action) \
    if (cond) { \
    } else { \
        QTC_ASSERT_STRING(#cond); \
        action; \
    } \
    do { \
    } while (0)
#define QTC_CHECK(cond) \
    if (cond) { \
    } else { \
        QTC_ASSERT_STRING(#cond); \
    } \
    do { \
    } while (0)
#define QTC_GUARD(cond) ((cond) ? true : (QTC_ASSERT_STRING(#cond), false))

void lala(int foo)
{
    Q_UNUSED(foo)
    Q_UNUSED(foo);
    QTC_ASSERT(true, return ); // NOTE: Ops, extra space with QTC_ASSERT macro and return keyword
    QTC_ASSERT(true, return;);
    while (true)
        QTC_ASSERT(true, break); // ...but this is fine
}

// -------------------------------------------------------------------------------------------------
// Namespaces
// -------------------------------------------------------------------------------------------------

namespace N {
namespace C {

struct Foo
{};

struct Bar
{};

namespace {
class ClassInUnnamedNamespace
{};
} // namespace

// NOTE: Nice, namespace end comments are added/updated automatically
} // namespace C

struct Baz
{};

} // namespace N
namespace N2 {
}

// -------------------------------------------------------------------------------------------------
// Forward declarations
// -------------------------------------------------------------------------------------------------

// NOTE: Ops, no one-liner anymore: forward declarations within namespace
namespace N {
struct Baz;
}

class SourceLocation;
class SourceRange;
class ClangString;

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QTextBlock);

// -------------------------------------------------------------------------------------------------
// Using declarations, using directives, using alias, typedefs
// -------------------------------------------------------------------------------------------------

// NOTE: OK, using directives are not sorted
using namespace std;
using namespace N2;
using namespace N;

// NOTE: Nice, using declarations are sorted
using N::C::Bar;
using N::C::Foo;

// NOTE: OK, typedefs are not sorted
typedef N::C::Foo TypedefedFoor;
typedef N::C::Bar TypedefedBar;

// NOTE: OK, using aliases are not sorted
using AliasedFoor = N::C::Foo;
using AliasedBar = N::C::Bar;

// -------------------------------------------------------------------------------------------------
// Comments
// -------------------------------------------------------------------------------------------------

// NOTE: OK, reflowing/wrapping of comments is turned off for now.
// This is a fancy comment that might be reflowed or not or not or not or not or not or not or not or not or not.

// NOTE: Nice, comments after declarations are aligned
int foo;           // fancy comment describing foo
int superDuperFoo; // fancy comment describing superDuperFoo
int lalaFoo;       // fancy comment describing lalaFoo

// -------------------------------------------------------------------------------------------------
// Function declarations
// -------------------------------------------------------------------------------------------------

void f();

void f2() {}

void f3(int parameter1, int parameter2, int parameter3);

// NOTE: Ops, awkward placement of parameter list, there is no equivalent of
//       PenaltyBreakBeforeFirstCallParameter for parameter list
void f3(
    int parameter1, int parameter2, int parameter3, int parameter4, int parameter5, int parameter6);

void f3(int parameter1,
        int parameter2,
        int parameter3,
        int parameter4,
        int parameter5,
        int parameter6,
        int parrameter7,
        int p = aGlobalInt);

bool operator==(N::Baz, N::Baz);

template<class T>
void fancyTemplateFunction();

template<typename... Type>
void variadicTemplateFunction(Type *... arg);

// -------------------------------------------------------------------------------------------------
// Inside functions
// -------------------------------------------------------------------------------------------------

int myfunction(int parameter1, int parameter2)
{
    // Function calls
    myfunction(parameter1, parameter2);

    // Casts
    int value = 3;
    int z = (int) value;
    int a = static_cast<int>(value + z);

    // Pointer/references alignment
    int *p = nullptr;
    int &r = *p;

    // Operators
    int result = parameter1 + parameter1 + parameter1 + parameter1 + parameter1 + parameter1
                 + parameter1 + parameter1;

    // Long expressions
    int someVeryLooooooooooooooooooooooooooooooooooooooooooooooooooongInt = -1;
    bool condition1 = false;
    bool condition2 = false;
    // NOTE: Ops, alignment of readable, boolean and complex expressions will be reverted
    if (condition1 || condition2
        || someVeryLooooooooooooooooooooooooooooooooooooooooooooooooooongInt) {
        return value;
    }

    // Initializer lists
    vector<int> x{1, 2, 3, 4};
    vector<Foo> y{{}, {}, {}, {}};
    new int[3]{1, 2, 3};

    // Streams
    // NOTE: OK, there is a heuristic (endl, '\n') to wrap statements with stream operators.
    cout << "Hello" << parameter1 << endl
         << parameter2 << endl
         << result << endl
         << condition1 << someVeryLooooooooooooooooooooooooooooooooooooooooooooooooooongInt;
    cout << "Hello" << parameter1 << '\n'
         << parameter2 << '\n'
         << a << '\n'
         << condition1 << someVeryLooooooooooooooooooooooooooooooooooooooooooooooooooongInt;
    // ...but here not:
    cout << "Hello" << parameter1 << '\t' << parameter2 << '\t' << z << '\t' << condition1
         << someVeryLooooooooooooooooooooooooooooooooooooooooooooooooooongInt << r;

    return 1;
}

// -------------------------------------------------------------------------------------------------
// Ternary Operator
// -------------------------------------------------------------------------------------------------

bool ternary()
{
    bool someWhatLongerName;
    bool someWhatLongerName2;
    bool isThatReallyReallyReallyReallyReallyReallyReallyReallyTrue = false;
    bool isThatReallyReallyReallyReallyReallyReallyReallyReallyReallyReallyTrue = false;

    bool yesno = true ? true : false;
    bool sino = isThatReallyReallyReallyReallyReallyReallyReallyReallyTrue ? someWhatLongerName
                                                                           : someWhatLongerName2;
    bool danet = isThatReallyReallyReallyReallyReallyReallyReallyReallyReallyReallyTrue
                     ? someWhatLongerName
                     : someWhatLongerName2;

    return yesno && sino && danet;
}

// -------------------------------------------------------------------------------------------------
// Penalty Test
// -------------------------------------------------------------------------------------------------

int functionToCall(int parameter1)
{
    return 1;
}

int functionToCall(int paramter1, int parameter2)
{
    return 1;
}

int functionToCall(int paramter1, int parameter2, int parameter3)
{
    return 1;
}

int functionToCall(int paramter1, int parameter2, int parameter3, int parameter4, int parameter5)
{
    return 1;
}

int functionToCall(
    int paramter1, int parameter2, int parameter3, int parameter4, int parameter5, int paramete6)
{
    return 1;
}

int functionToCall(int paramter1,
                   int parameter2,
                   int parameter3,
                   int parameter4,
                   int parameter5,
                   int paramete6,
                   int parameter6)
{
    return 1;
}

bool functionToCallSt(const QString &paramter1, const QStringList &list)
{
    return 1;
}

void emitAddOutput(const QString &text, int priority) {}
void emitAddOutput(const QString &text, int priority, int category) {}

void penaltyTests(bool isThatTrue)
{
    int valueX = 3;
    int valueY = 1;
    int valueZ = 1;
    int valueXTimesY = valueX * valueY;
    int unbelievableBigValue = 1000000;
    int unlimitedValueunbelievableBigValue = 1000000;
    QString arguments;
    QString argumentsVeryLong;

    const QFile::OpenMode openMode = isThatTrue ? QIODevice::ReadOnly
                                                : (QIODevice::ReadOnly | QIODevice::Text);

    const auto someValue10 = functionToCall(valueX, valueY, valueXTimesY);
    const auto someValue11 = functionToCall(valueX,
                                            valueY,
                                            valueXTimesY,
                                            unbelievableBigValue,
                                            unbelievableBigValue);
    const auto someValue12 = functionToCall(valueX,
                                            valueY,
                                            valueXTimesY,
                                            unbelievableBigValue,
                                            unbelievableBigValue * unbelievableBigValue,
                                            unbelievableBigValue);

    const auto someValue13 = functionToCall(valueX,
                                            valueY,
                                            valueXTimesY,
                                            unbelievableBigValue,
                                            functionToCall(functionToCall(valueX),
                                                           functionToCall(valueY)),
                                            unbelievableBigValue);

    const auto someValue14WithAnOutstandingLongName
        = functionToCall(valueX,
                         valueY,
                         valueXTimesY,
                         unbelievableBigValue,
                         functionToCall(functionToCall(valueX), functionToCall(valueY)),
                         unbelievableBigValue);

    const bool someValue20 = functionToCall(valueX, valueY, valueXTimesY) || functionToCall(3);
    const bool someValue21 = functionToCall(valueX, valueY, valueXTimesY)
                             || functionToCall(valueX, valueY);

    emitAddOutput(QCoreApplication::tr("Starting: \"%1\" %2")
                      .arg("/some/very/very/very/very/long/path/to/an/executable", arguments),
                  functionToCall(3),
                  functionToCall(3) | valueX);

    emitAddOutput(QCoreApplication::tr("Starting: \"%1\" %2")
                      .arg("/some/very/very/very/very/long/path/to/an/executable",
                           argumentsVeryLong),
                  functionToCall(3),
                  functionToCall(3) | unlimitedValueunbelievableBigValue
                      | unlimitedValueunbelievableBigValue);

    const QString path;
    const bool someLongerNameNNNNNNNNNN = functionToCallSt(path,
                                                           QStringList(QLatin1String("-print-env")));
}

// -------------------------------------------------------------------------------------------------
// Classes and member functions
// -------------------------------------------------------------------------------------------------

class Base1
{};
class Base2
{};

class MyClass : public Base1, public Base2, public QObject
{
    Q_OBJECT

public:
    friend class FriendX;      // for X::foo()
    friend class OtherFriendY; // for Y::bar()

    MyClass() {}

    MyClass(int d1)
        : data1(d1)
    {}

    MyClass(int d1, int d2)
        : data1(d1)
        , data2(d2)
        , data3(d2)
    {}

    MyClass(int initialData1,
            int initialData2,
            int initialData3,
            int initialData4,
            int initialData5,
            int initialData6)
        : data1(initialData1)
        , data2(initialData2)
        , data3(initialData3)
        , data4(initialData4)
        , data5(initialData5)
        , data6(initialData6)
    {
        // NOTE: Ops, manual alignment of QObject::connect() arguments is reverted
        // (we tend to write emitter/receiver on separate lines)
        connect(this, &MyClass::inlineZeroStatements, this, &MyClass::nonInlineSingleStatement);
    }

    void inlineZeroStatements() {}
    void inlineSingleStatement() { aGlobalInt = 2; }
    void inlineMultipleStatements()
    {
        aGlobalInt = 2;
        aGlobalInt = 3;
    }

    void nonInlineZeroStatements();
    void nonInlineSingleStatement();
    void nonInlineMultipleStatements();

    virtual void shortVirtual(int parameter1, int parameter2, int parameter3) = 0;

    // NOTE: Ops, awkward placement of "= 0;"
    virtual void longerVirtual(
        int parameter1, int parameter2, int parameter3, int parameter4, int parameter5)
        = 0;

    int someGetter() const;

private:
    int data1 = 0;
    int data2 = 0;
    int data3 = 0;
    int data4 = 0;
    int data5 = 0;
    int data6 = 0;
};

void MyClass::nonInlineZeroStatements() {}

void MyClass::nonInlineSingleStatement()
{
    aGlobalInt = 2;
}

void MyClass::nonInlineMultipleStatements()
{
    aGlobalInt = 2;
    aGlobalInt = 2;
}

template<class T>
class TemplateClass
{
};

// -------------------------------------------------------------------------------------------------
// Enums
// -------------------------------------------------------------------------------------------------

// NOTE: OK, enums: one-liner are possible
enum MyEnum { SourceFiles = 0x1, GeneratedFiles = 0x2, AllFiles = SourceFiles | GeneratedFiles };

// NOTE: OK, enums: a comment breaks one-liners
enum MyEnumX {
    OSourceFiles = 0x1 // fancy
};

// NOTE: OK, enums: longer ones are wrapped
enum MyOtherEnum {
    XSourceFiles = 0x1,
    XGeneratedFiles = 0x2,
    XAllFiles = XSourceFiles | XGeneratedFiles
};

enum {
    Match_None,
    Match_TooManyArgs,
    Match_TooFewArgs,
    Match_Ok
} matchType
    = Match_None; // NOTE: Ops, awkward placement of "= value;" at enum definition+use

// -------------------------------------------------------------------------------------------------
// Lambdas
// -------------------------------------------------------------------------------------------------

void takeLambda(std::function<int()>) {}
void takeLambdaWithSomeLongerNameHa(std::function<int()>) {}
bool UtilsFiltered(QVector<N::Baz>, std::function<int(const N::Baz &)>)
{
    return true;
}

void lambda()
{
    auto shortLambda = []() { return 1; };
    auto longLambda = []() {
        aGlobalInt = 3;
        return;
    };

    takeLambda([]() { return true; });
    takeLambda([]() {
        aGlobalInt = 3;
        return aGlobalInt;
    });

    // NOTE: Ops, lambda: capture and parameter list should be on separate line
    int thisCouldBeSomeLongerFunctionCallExpressionOrSoSoSo;
    takeLambdaWithSomeLongerNameHa(
        [&]() { return thisCouldBeSomeLongerFunctionCallExpressionOrSoSoSo; });

    QVector<N::Baz> myClasses;
    UtilsFiltered(myClasses, [](const N::Baz &) { return 1; });

    // NOTE: Ops, lambda: lambda not started on new line in if-condition
    if (UtilsFiltered(myClasses, [](const N::Baz &) { return 1; })) {
        ++aGlobalInt;
    }
}

// -------------------------------------------------------------------------------------------------
// Control flow
// -------------------------------------------------------------------------------------------------

int fn(int, int, int)
{
    return 1;
}

void controlFlow(int x)
{
    int y = -1;

    // if
    if (true)
        fn(1, 2, 3);

    int value = 3;
    if (value) {
        value += value + 1;
        --value;
    }

    if (x == y)
        fn(1, 2, 3);
    else if (x > y)
        fn(1, 2, 3);
    else
        fn(1, 2, 3);

    if (x == y) {
        fn(1, 2, 3);
        return;
    } else if (x > y) {
        fn(1, 2, 3);
    } else {
        fn(1, 2, 3);
    }

    // switch
    switch (x) {
    case 0:
        ++aGlobalInt;
        return;
    case 1:
        fn(1, 2, 3);
        ++aGlobalInt;
        break;
    case 2:
        fn(1, 2, 3);
        break;
    default:
        break;
    }

    // do-while
    do {
        value += value + 1;
        ++value;
    } while (true);

    // for
    QVector<int> myVector;
    for (int i = 0; i < myVector.size(); ++i)
        ++aGlobalInt;

    for (QVector<int>::const_iterator i = myVector.begin(); i != myVector.end(); ++i)
        ++aGlobalInt;

    QVector<int> myVectorWithLongName;
    for (QVector<int>::const_iterator i = myVectorWithLongName.begin(),
                                      ei = myVectorWithLongName.end();
         i != ei;
         ++i) {
    }

    forever {
        ++aGlobalInt;
        --aGlobalInt;
    }
}

// -------------------------------------------------------------------------------------------------
// Function declarations and calls - extreme cases
// -------------------------------------------------------------------------------------------------

void extremeFunction(const char[]) {}
void extremeFunction(
    int uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuunbelievableLongParameter)
{
    extremeFunction(
        uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuunbelievableLongParameter);

    int uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuunbelievableLongValue
        = 3;
    ++uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuunbelievableLongValue;

    extremeFunction(
        "some super duper super duper super duper super duper super duper super duper super duper long");
}

void extremeFunction2(int parameter1,
                      int parameter2,
                      int parameter3WithAVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVerVeryVeryLong)
{
    extremeFunction2(parameter1,
                     parameter2,
                     parameter3WithAVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVerVeryVeryLong);
}

void extremeFunction3(int parameter1,
                      int parameter2,
                      int parameter3WithAVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVerVeryVeryLongNameX)
{
    extremeFunction3(parameter1,
                     parameter2,
                     parameter3WithAVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVerVeryVeryLongNameX);
}

// -------------------------------------------------------------------------------------------------
// Misc
// -------------------------------------------------------------------------------------------------

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.builtineditordocumentprocessor")

    int hello; // NOTE: Ops, awkward placement of next token after Q_LOGGING_CATEGORY (semicolon helps)

// -------------------------------------------------------------------------------------------------

int main() {}
