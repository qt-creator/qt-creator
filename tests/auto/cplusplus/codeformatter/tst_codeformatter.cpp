/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QtTest>
#include <QObject>
#include <QList>
#include <QTextDocument>
#include <QTextBlock>

#include <cpptools/cppcodeformatter.h>
using namespace CppTools;

class tst_CodeFormatter: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ifStatementWithoutBraces1();
    void ifStatementWithoutBraces2();
    void ifStatementWithBraces1();
    void ifStatementWithBraces2();
    void ifStatementMixed();
    void ifStatementAndComments();
    void ifStatementLongCondition();
    void strayElse();
    void macrosNoSemicolon();
    void oneLineIf();
    void doWhile();
    void closingCurlies();
    void ifdefedInsideIf();
    void ifdefs();
    void preprocessorContinuation();
    void cStyleComments();
    void cppStyleComments();
    void expressionContinuation();
    void classAccess();
    void ternary();
    void objcAtDeclarations();
    void objcCall();
    void objcCallAndFor();
    void braceList();
    void bug1();
    void bug2();
    void bug3();
    void switch1();
    void memberInitializer();
    void templates();
    void operatorOverloads();
    void gnuStyle();
    void whitesmithsStyle();
    void singleLineEnum();
    void functionReturnType();
    void streamOp();
    void blockStmtInIf();
    void nestedInitializer();
    void forStatement();
    void templateSingleline();
    void macrosNoSemicolon2();
    void renamedNamespace();
    void cpp0xFor();
    void gnuStyleSwitch();
    void whitesmithsStyleSwitch();
    void indentToNextToken();
    void labels();
};

struct Line {
    Line(QString l)
        : line(l)
        , expectedIndent(-1)
        , expectedPadding(0)
    {
        for (int i = 0; i < l.size(); ++i) {
            if (!l.at(i).isSpace()) {
                expectedIndent = i;
                break;
            }
        }
        if (expectedIndent == -1)
            expectedIndent = l.size();
        if (expectedIndent < l.size() && l.at(expectedIndent) == '~') {
            line[expectedIndent] = ' ';
            for (int i = expectedIndent + 1; i < l.size(); ++i) {
                if (!l.at(i).isSpace()) {
                    expectedPadding = i - expectedIndent;
                    break;
                }
            }
        }
    }

    Line(QString l, int expectIndent, int expectPadding = 0)
        : line(l), expectedIndent(expectIndent), expectedPadding(expectPadding)
    {
        for (int i = 0; i < line.size(); ++i) {
            if (line.at(i) == '~') {
                line[i] = ' ';
                break;
            }
            if (!line.at(i).isSpace())
                break;
        }
    }

    QString line;
    int expectedIndent;
    int expectedPadding;
};

QString concatLines(QList<Line> lines)
{
    QString result;
    foreach (const Line &l, lines) {
        result += l.line;
        result += "\n";
    }
    return result;
}

void checkIndent(QList<Line> data, int style = 0)
{
    QString text = concatLines(data);
    QTextDocument document(text);
    QtStyleCodeFormatter formatter;
    if (style == 1) {// gnu
        formatter.setIndentSubstatementBraces(true);
    } else if (style == 2) { // whitesmiths
        formatter.setIndentSubstatementStatements(false);
        formatter.setIndentSubstatementBraces(true);
        formatter.setIndentDeclarationMembers(false);
        formatter.setIndentDeclarationBraces(true);
    }

    int i = 0;
    foreach (const Line &l, data) {
        QTextBlock b = document.findBlockByLineNumber(i);
        if (l.expectedIndent != -1) {
            int indent, padding;
            formatter.indentFor(b, &indent, &padding);
            QVERIFY2(indent == l.expectedIndent, qPrintable(QString("Wrong indent in line %1 with text '%2', expected indent %3, got %4")
                                                            .arg(QString::number(i+1), l.line, QString::number(l.expectedIndent), QString::number(indent))));
            QVERIFY2(padding == l.expectedPadding, qPrintable(QString("Wrong padding in line %1 with text '%2', expected padding %3, got %4")
                                                              .arg(QString::number(i+1), l.line, QString::number(l.expectedPadding), QString::number(padding))));
        }
        formatter.updateLineStateChange(b);
        ++i;
    }
}

void tst_CodeFormatter::ifStatementWithoutBraces1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a)")
         << Line("        if (b)")
         << Line("            foo;")
         << Line("        else if (c)")
         << Line("            foo;")
         << Line("        else")
         << Line("            if (d)")
         << Line("                foo;")
         << Line("            else")
         << Line("                while (e)")
         << Line("                    bar;")
         << Line("    else")
         << Line("        foo;")         
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementWithoutBraces2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a)")
         << Line("        if (b)")
         << Line("            foo;")
         << Line("    if (a) b();")
         << Line("    if (a) b(); else")
         << Line("        foo;")
         << Line("    if (a)")
         << Line("        if (b)")
         << Line("            foo;")
         << Line("        else if (c)")
         << Line("            foo;")
         << Line("        else")
         << Line("            if (d)")
         << Line("                foo;")
         << Line("            else")
         << Line("                while (e)")
         << Line("                    bar;")
         << Line("    else")
         << Line("        foo;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementWithBraces1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a) {")
         << Line("        if (b) {")
         << Line("            foo;")
         << Line("        } else if (c) {")
         << Line("            foo;")
         << Line("        } else {")
         << Line("            if (d) {")
         << Line("                foo;")
         << Line("            } else {")
         << Line("                foo;")
         << Line("            }")
         << Line("        }")
         << Line("    } else {")
         << Line("        foo;")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementWithBraces2()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    if (a)")
         << Line("    {")
         << Line("        if (b)")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        else if (c)")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        else")
         << Line("        {")
         << Line("            if (d)")
         << Line("            {")
         << Line("                foo;")
         << Line("            }")
         << Line("            else")
         << Line("            {")
         << Line("                foo;")
         << Line("            }")
         << Line("        }")
         << Line("    }")
         << Line("    else")
         << Line("    {")
         << Line("        foo;")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementMixed()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    if (foo)")
         << Line("        if (bar)")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        else")
         << Line("            if (car)")
         << Line("            {}")
         << Line("            else doo;")
         << Line("    else abc;")
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementAndComments()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    if (foo)")
         << Line("        ; // bla")
         << Line("    else if (bar)")
         << Line("        ;")
         << Line("    if (foo)")
         << Line("        ; /* bla")
         << Line("      bla */")
         << Line("    else if (bar)")
         << Line("        // foobar")
         << Line("        ;")
         << Line("    else if (bar)")
         << Line("        /* bla")
         << Line("  bla */")
         << Line("        ;")
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::ifStatementLongCondition()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    if (foo &&")
         << Line("    ~       bar")
         << Line("    ~       || (a + b > 4")
         << Line("    ~           && foo(bar)")
         << Line("    ~           )")
         << Line("    ~       ) {")
         << Line("        foo;")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::strayElse()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    while( true ) {}")
         << Line("    else", -1)
         << Line("    else {", -1)
         << Line("    }", -1)
         << Line("}");
    checkIndent(data);
}

void tst_CodeFormatter::macrosNoSemicolon()
{
    QList<Line> data;
    data << Line("QT_DECLARE_METATYPE(foo)")
         << Line("int i;")
         << Line("Q_BLABLA")
         << Line("int i;")
         << Line("Q_BLABLA;")
         << Line("int i;")
         << Line("Q_BLABLA();")
         << Line("int i;")
         << Line("QML_DECLARE_TYPE(a, b, c, d)")
         << Line("int i;")
         << Line("Q_PROPERTY(abc)")
         << Line("QDOC_PROPERTY(abc)")
         << Line("void foo() {")
         << Line("    QT_DECLARE_METATYPE(foo)")
         << Line("    int i;")
         << Line("    Q_BLABLA")
         << Line("    int i;")
         << Line("    Q_BLABLA;")
         << Line("    int i;")
         << Line("    Q_BLABLA();")
         << Line("    Q_PROPERTY(abc)")
         << Line("    QDOC_PROPERTY(abc)")
         << Line("    int i;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::oneLineIf()
{
    QList<Line> data;
    data << Line("class F {")
         << Line("    void foo()")
         << Line("    { if (showIt) show(); }")
         << Line("};")
         ;
    checkIndent(data);

}

void tst_CodeFormatter::doWhile()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    do { if (c) foo; } while(a);")
         << Line("    do {")
         << Line("        if(a);")
         << Line("    } while(a);")
         << Line("    do")
         << Line("        foo;")
         << Line("    while(a);")
         << Line("    do foo; while(a);")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::closingCurlies()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a)")
         << Line("        if (b) {")
         << Line("            foo;")
         << Line("        }")
         << Line("    {")
         << Line("        foo();")
         << Line("    }")
         << Line("    foo();")
         << Line("    {")
         << Line("        foo();")
         << Line("    }")
         << Line("    while (a) {")
         << Line("        if (a);")
         << Line("    }")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::ifdefedInsideIf()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a) {")
         << Line("#ifndef Q_WS_WINCE")
         << Line("        if (b) {")
         << Line("#else")
         << Line("        if (c) {")
         << Line("#endif")
         << Line("        }")
         << Line("    } else if (d) {")
         << Line("    }")
         << Line("    if (a)")
         << Line("        ;")
         << Line("    else if (type == Qt::Dialog || type == Qt::Sheet)")
         << Line("#ifndef Q_WS_WINCE")
         << Line("        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint;")
         << Line("#else")
         << Line("        bar;")
         << Line("#endif")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::ifdefs()
{
    QList<Line> data;
    data << Line("#ifdef FOO")
         << Line("#include <bar>")
         << Line("void foo()")
         << Line("{")
         << Line("    if (bar)")
         << Line("#if 1")
         << Line("        foo;")
         << Line("    else")
         << Line("#endif")
         << Line("        foo;")
         << Line("}")
         << Line("#endif")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::preprocessorContinuation()
{
    QList<Line> data;
    data << Line("#define x \\")
         << Line("    foo(x) + \\")
         << Line("    bar(x)")
         << Line("int i;")
         << Line("void foo() {")
         << Line("#define y y")
         << Line("#define x \\")
         << Line("    foo(x) + \\")
         << Line("    bar(x)")
         << Line("    int j;")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::cStyleComments()
{
    QList<Line> data;
    data << Line("/*")
         << Line("  ")
         << Line("      foo")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("*/")
         << Line("void foo() {")
         << Line("    /*")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("    */")
         << Line("    /* bar */")
         << Line("}")
         << Line("struct s {")
         << Line("    /* foo */")
         << Line("    /*")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("    */")
         << Line("    /* bar */")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::cppStyleComments()
{
    QList<Line> data;
    data << Line("// abc")
         << Line("class C {  ")
         << Line("    // ghij")
         << Line("    // def")
         << Line("    // ghij")
         << Line("    int i; // hik")
         << Line("    // doo")
         << Line("} // ba")
         << Line("// ba")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::expressionContinuation()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    return (a <= b &&")
         << Line("    ~       c <= d);")
         << Line("    return (qMax <= qMin() &&")
         << Line("    ~       qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));")
         << Line("    return a")
         << Line("    ~       == b && c == d;")
         << Line("    return a ==")
         << Line("    ~       b && c == d;")
         << Line("    return a == b")
         << Line("    ~       && c == d;")
         << Line("    return a == b &&")
         << Line("    ~       c == d;")
         << Line("    return a == b && c")
         << Line("    ~       == d;")
         << Line("    return a == b && c ==")
         << Line("    ~       d;")
         << Line("    return a == b && c == d;")
         << Line("    qDebug() << foo")
         << Line("    ~        << bar << moose")
         << Line("    ~        << bar +")
         << Line("    ~           foo - blah(1)")
         << Line("    ~        << '?'")
         << Line("    ~        << \"\\n\";")
         << Line("    i += foo(")
         << Line("    ~           bar,")
         << Line("    ~           2);")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::classAccess()
{
    QList<Line> data;
    data << Line("class foo {")
         << Line("    int i;")
         << Line("public:")
         << Line("    class bar {")
         << Line("    private:")
         << Line("        int i;")
         << Line("    public:")
         << Line("    private slots:")
         << Line("        void foo();")
         << Line("    public Q_SLOTS:")
         << Line("    Q_SIGNALS:")
         << Line("    };")
         << Line("    float f;")
         << Line("private:")
         << Line("    void bar(){}")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::ternary()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    int i = a ? b : c;")
         << Line("    foo += a_bigger_condition ?")
         << Line("    ~           b")
         << Line("    ~         : c;")
         << Line("    int i = a ?")
         << Line("    ~           b : c;")
         << Line("    int i = a ? b")
         << Line("    ~         : c +")
         << Line("    ~           2;")
         << Line("    int i = (a ? b : c) + (foo")
         << Line("    ~                      bar);")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::bug1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (attribute < int(8*sizeof(uint))) {")
         << Line("        if (on)")
         << Line("            data->widget_attributes |= (1<<attribute);")
         << Line("        else")
         << Line("            data->widget_attributes &= ~(1<<attribute);")
         << Line("    } else {")
         << Line("        const int x = attribute - 8*sizeof(uint);")
         << Line("    }")
         << Line("    int i;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::bug2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    const int sourceY = foo(")
         << Line("    ~           bar(")
         << Line("    ~               car(a,")
         << Line("    ~                   b),")
         << Line("    ~               b),")
         << Line("    ~           foo);")
         << Line("    const int sourceY =")
         << Line("    ~       foo(")
         << Line("    ~           bar(a,")
         << Line("    ~               b),")
         << Line("    ~           b);")
         << Line("    int j;")
         << Line("    const int sourceY =")
         << Line("    ~       (direction == DirectionEast || direction == DirectionWest) ?")
         << Line("    ~           (sourceRect.top() + (sourceRect.bottom() - sourceRect.top()) / 2)")
         << Line("    ~         : (direction == DirectionSouth ? sourceRect.bottom() : sourceRect.top());")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::bug3()
{
    QList<Line> data;
    data << Line("class AutoAttack")
         << Line("{")
         << Line("public:")
         << Line("    AutoAttack(unsigned delay, unsigned warmup)")
         << Line("    ~   : mWarmup(warmup && warmup < delay ? warmup : delay >> 2)")
         << Line("    {}")
         << Line("    unsigned getWarmup() const { return mWarmup; }")
         << Line("private:")
         << Line("    unsigned mWarmup;")
         << Line("}")
            ;
    checkIndent(data);
}

void tst_CodeFormatter::braceList()
{
    QList<Line> data;
    data << Line("enum Foo {")
         << Line("    a,")
         << Line("    b,")
         << Line("};")
         << Line("enum Foo { a = 2,")
         << Line("    ~      a = 3,")
         << Line("    ~      b = 4")
         << Line("~        };")
         << Line("void foo () {")
         << Line("    int a[] = { foo, bar, ")
         << Line("    ~           car };")
         << Line("    int a[] = {")
         << Line("    ~   a, b,")
         << Line("    ~   c")
         << Line("    };")
         << Line("    int k;")
         ;
    checkIndent(data);

}

void tst_CodeFormatter::objcAtDeclarations()
{
    QList<Line> data;
    data << Line("@class Forwarded;")
         << Line("@protocol Forwarded;")
         << Line("int i;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::objcCall()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    [NSApp windows];")
         << Line("    [NSObject class];")
         << Line("    if (a)")
         << Line("        int a = [window drawers];")
         << Line("}")
         << Line("int y;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::objcCallAndFor()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    NSArray *windows = [NSApp windows];")
         << Line("    for (NSWindow *window in windows) {")
         << Line("        NSArray *drawers = [window drawers];")
         << Line("        for (NSDrawer *drawer in drawers) {")
         << Line("            NSArray *views = [[drawer contentView] subviews];")
         << Line("            int x;")
         << Line("        }")
         << Line("    }")
         << Line("}")
         << Line("int y;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::switch1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (a) {")
         << Line("    case 1:")
         << Line("        foo;")
         << Line("        if (a);")
         << Line("    case 2:")
         << Line("    case 3: {")
         << Line("        foo;")
         << Line("    }")
         << Line("    case 4:")
         << Line("    {")
         << Line("        foo;")
         << Line("    }")
         << Line("    case bar:")
         << Line("        break;")
         << Line("    }")
         << Line("    case 4:")
         << Line("    {")
         << Line("        if (a) {")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::memberInitializer()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("~   : baR()")
         << Line("~   , m_member(23)")
         << Line("{")
         << Line("}")
         << Line("class Foo {")
         << Line("    Foo()")
         << Line("    ~   : baR(),")
         << Line("    ~     moodoo(barR + ")
         << Line("    ~         42),")
         << Line("    ~     xyz()")
         << Line("    {}")
         << Line("};")
         << Line("class Foo {")
         << Line("    Foo() :")
         << Line("    ~   baR(),")
         << Line("    ~   moo(barR)")
         << Line("    ~ , moo(barR)")
         << Line("    {}")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::templates()
{
    QList<Line> data;
    data << Line("template <class T, typename F, int i>")
         << Line("class Foo {")
         << Line("private:")
         << Line("};")
         << Line("template <class T,")
         << Line("~         typename F, int i")
         << Line("~         >")
         << Line("class Foo {")
         << Line("private:")
         << Line("};")
         << Line("template <template <class F,")
         << Line("~                   class D>,")
         << Line("~         typename F>")
         << Line("class Foo { };")
         << Line("template <")
         << Line("~       template <")
         << Line("~           class F, class D>,")
         << Line("~       typename F>")
         << Line("class Foo { };")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::operatorOverloads()
{
    QList<Line> data;
    data << Line("struct S {")
         << Line("    void operator()() {")
         << Line("        foo;")
         << Line("    }")
         << Line("    void operator<<() {")
         << Line("        foo;")
         << Line("    }")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::gnuStyle()
{
    QList<Line> data;
    data << Line("struct S")
         << Line("{")
         << Line("    void foo()")
         << Line("    {")
         << Line("        if (a)")
         << Line("            {")
         << Line("                fpp;")
         << Line("            }")
         << Line("        else if (b)")
         << Line("            {")
         << Line("                fpp;")
         << Line("            }")
         << Line("        else")
         << Line("            {")
         << Line("            }")
         << Line("        if (b) {")
         << Line("            fpp;")
         << Line("        }")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("    }")
         << Line("};")
         ;
    checkIndent(data, 1);
}

void tst_CodeFormatter::whitesmithsStyle()
{
    QList<Line> data;
    data << Line("struct S")
         << Line("    {")
         << Line("    void foo()")
         << Line("        {")
         << Line("        if (a)")
         << Line("            {")
         << Line("            fpp;")
         << Line("            }")
         << Line("        if (b) {")
         << Line("            fpp;")
         << Line("            }")
         << Line("        {")
         << Line("        foo;")
         << Line("        }")
         << Line("        }")
         << Line("    };")
         ;
    checkIndent(data, 2);
}

void tst_CodeFormatter::singleLineEnum()
{
    QList<Line> data;
    data << Line("enum { foo, bar, car = 2 };")
         << Line("void blah() {")
         << Line("    enum { foo, bar, car = 2 };")
         << Line("    int i;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::functionReturnType()
{
    QList<Line> data;
    data
         << Line("void")
         << Line("foo(int) {}")
         << Line("")
         << Line("const QList<int> &")
         << Line("A::foo() {}")
         << Line("")
         << Line("template <class T>")
         << Line("const QList<QMap<T, T> > &")
         << Line("A::B::foo() {}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::streamOp()
{
    QList<Line> data;
    data
         << Line("void foo () {")
         << Line("    qDebug() << foo")
         << Line("    ~        << bar << moose")
         << Line("    ~        << bar +")
         << Line("    ~           foo - blah(1)")
         << Line("    ~        << '?'")
         << Line("    ~        << \"\\n\";")
         << Line("    qDebug() << foo")
         << Line("        << bar << moose", 4, 9)
         << Line("    ~   << bar +")
         << Line("    ~      foo - blah(1)")
         << Line("    ~   << '?'")
         << Line("    ~   << \"\\n\";")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::blockStmtInIf()
{
    QList<Line> data;
    data
         << Line("void foo () {")
         << Line("    if (a) {")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("    } else {")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::nestedInitializer()
{
    QList<Line> data;
    data
         << Line("SomeStruct v[] = {")
         << Line("~   {2}, {3},")
         << Line("~   {4}, {5},")
         << Line("};")
         << Line("S v[] = {{1}, {2},")
         << Line("~        {3}, {4},")
         << Line("~       };")
         << Line("SomeStruct v[] = {")
         << Line("~   {")
         << Line("~       {2, 3,")
         << Line("~        4, 5},")
         << Line("~       {1},")
         << Line("~   }")
         << Line("};")
         << Line("SomeStruct v[] = {{{2, 3},")
         << Line("~                  {4, 5}")
         << Line("~                 },")
         << Line("~                 {{2, 3},")
         << Line("~                  {4, 5},")
         << Line("~                 }")
         << Line("~                };")
         << Line("int i;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::forStatement()
{
    QList<Line> data;
    data
         << Line("void foo()")
         << Line("{")
         << Line("    for (a; b; c)")
         << Line("        bar();")
         << Line("    for (a; b; c) {")
         << Line("        bar();")
         << Line("    }")
         << Line("    for (a; b; c)")
         << Line("    {")
         << Line("        bar();")
         << Line("    }")
         << Line("    for (a;")
         << Line("    ~    b;")
         << Line("    ~    c)")
         << Line("        bar();")
         << Line("    for (a;")
         << Line("    ~    b;")
         << Line("    ~    c) {")
         << Line("        bar();")
         << Line("    }")
         << Line("    for (a;")
         << Line("    ~    b;")
         << Line("    ~    c)")
         << Line("    {")
         << Line("        bar();")
         << Line("    }")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::templateSingleline()
{
    QList<Line> data;
    data
         << Line("template <typename T> class Foo")
         << Line("{")
         << Line("    T t;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::macrosNoSemicolon2()
{
    QList<Line> data;
    data
         << Line("FOO(ABC)")
         << Line("{")
         << Line("    BAR(FOO)")
         << Line("}")
         << Line("int i;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::renamedNamespace()
{
    QList<Line> data;
    data
         << Line("namespace X = Y;")
         << Line("void foo()")
         << Line("{")
         << Line("    return;")
         << Line("}")
         << Line("int i;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::cpp0xFor()
{
    QList<Line> data;
    data
         << Line("void foo()")
         << Line("{")
         << Line("    vector<int> x = setup();")
         << Line("    for(int p : x) {")
         << Line("        bar(p);")
         << Line("    }")
         << Line("}")
         << Line("void car()")
         << Line("{")
         << Line("    int i;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::gnuStyleSwitch()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    switch (a)")
         << Line("        {")
         << Line("        case 1:")
         << Line("            foo;")
         << Line("            break;")
         << Line("        case 2: {")
         << Line("                bar;")
         << Line("                continue;")
         << Line("            }")
         << Line("        case 3:")
         << Line("            {")
         << Line("                bar;")
         << Line("                continue;")
         << Line("            }")
         << Line("        case 4:")
         << Line("        case 5:")
         << Line("            ;")
         << Line("        }")
         << Line("}")
         ;
    checkIndent(data, 1);
}

void tst_CodeFormatter::whitesmithsStyleSwitch()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("    {")
         << Line("    switch (a)")
         << Line("        {")
         << Line("        case 1:")
         << Line("            foo;")
         << Line("            break;")
         << Line("        case 2: {")
         << Line("            bar;")
         << Line("            continue;")
         << Line("            }")
         << Line("        case 3:")
         << Line("            {")
         << Line("            bar;")
         << Line("            continue;")
         << Line("            }")
         << Line("        case 4:")
         << Line("        case 5:")
         << Line("            ;")
         << Line("        }")
         << Line("    }")
         ;
    checkIndent(data, 2);
}

void tst_CodeFormatter::indentToNextToken()
{
    QList<Line> data;
    data << Line("void foo( int i,")
         << Line("~         int j) {")
         << Line("    a <<     foo + ")
         << Line("    ~        bar;")
         << Line("    if (a &&")
         << Line("    ~       b) {")
         << Line("        foo; }")
         << Line("    if ( a &&")
         << Line("    ~    b) {")
         << Line("        foo; }")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::labels()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("lab:")
         << Line("    int abc;")
         << Line("def:")
         << Line("    if (a)")
         << Line("boo:")
         << Line("        foo;")
         << Line("    int j;")
         ;
    checkIndent(data);
}

QTEST_APPLESS_MAIN(tst_CodeFormatter)
#include "tst_codeformatter.moc"


