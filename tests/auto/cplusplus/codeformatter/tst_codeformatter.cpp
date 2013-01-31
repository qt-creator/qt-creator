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
    void expressionContinuation1();
    void expressionContinuation2();
    void assignContinuation1();
    void assignContinuation2();
    void declarationContinuation();
    void classAccess();
    void ternary();
    void objcAtDeclarations();
    void objcCall();
    void objcCallAndFor();
    void braceList();
    void bug1();
    void bug2();
    void bug3();
    void bug4();
    void switch1();
    void switch2();
    void switch3();
    void switch4();
    void switch5();
    void blocks();
    void memberInitializer();
    void memberInitializer2();
    void memberInitializer3();
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
    void functionsWithExtraSpecifier();
    void externSpec();
    void indentNamespace();
    void indentNamespace2();
    void accessSpecifiers1();
    void accessSpecifiers2();
    void accessSpecifiers3();
    void accessSpecifiers4();
    void accessSpecifiers5();
    void accessSpecifiers6();
    void functionBodyAndBraces1();
    void functionBodyAndBraces2();
    void functionBodyAndBraces3();
    void functionBodyAndBraces4();
    void constructor1();
    void constructor2();
    void constructor3();
    void caseBody1();
    void caseBody2();
    void caseBody3();
    void caseBody4();
    void caseBody5();
    void caseBody6();
    void blockBraces1();
    void functionDefaultArgument();
    void attributeInAccessSpecifier();
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

void checkIndent(QList<Line> data, QtStyleCodeFormatter formatter)
{
    QString text = concatLines(data);
    QTextDocument document(text);

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

void checkIndent(QList<Line> data, CppCodeStyleSettings style)
{
    QtStyleCodeFormatter formatter;
    formatter.setCodeStyleSettings(style);
    checkIndent(data, formatter);
}

void checkIndent(QList<Line> data, int style = 0)
{
    CppCodeStyleSettings codeStyle;
    QtStyleCodeFormatter formatter;
    if (style == 1) {// gnu
        codeStyle.indentBlockBraces = true;
        codeStyle.indentSwitchLabels = true;
        codeStyle.indentBlocksRelativeToSwitchLabels = true;
    } else if (style == 2) { // whitesmiths
        codeStyle.indentBlockBody = false;
        codeStyle.indentBlockBraces = true;
        codeStyle.indentClassBraces = true;
        codeStyle.indentNamespaceBraces = true;
        codeStyle.indentEnumBraces = true;
        codeStyle.indentFunctionBody = false;
        codeStyle.indentFunctionBraces = true;
        codeStyle.indentSwitchLabels = true;
        codeStyle.indentBlocksRelativeToSwitchLabels = true;
    }
    formatter.setCodeStyleSettings(codeStyle);

    checkIndent(data, formatter);
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

void tst_CodeFormatter::expressionContinuation1()
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
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::expressionContinuation2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    i += abc +")
         << Line("    ~       foo(,")
         << Line("    ~           bar,")
         << Line("    ~           2")
         << Line("    ~           );")
         << Line("    i += abc +")
         << Line("    ~       foo(,")
         << Line("    ~           bar(")
         << Line("    ~               bar,")
         << Line("    ~               2")
         << Line("    ~               ),")
         << Line("    ~           abc);")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::assignContinuation1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    abcdefgh = a +")
         << Line("    ~       b;")
         << Line("    a = a +")
         << Line("    ~       b;")
         << Line("    (a = a +")
         << Line("    ~       b);")
         << Line("    abcdefgh =")
         << Line("    ~       a + b;")
         << Line("    a =")
         << Line("    ~       a + b;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::assignContinuation2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    abcdefgh = a +")
         << Line("    ~          b;")
         << Line("    a = a +")
         << Line("    ~   b;")
         << Line("    (a = a +")
         << Line("    ~    b);")
         << Line("    abcdefgh =")
         << Line("    ~       a + b;")
         << Line("    a =")
         << Line("    ~       a + b;")
         << Line("}")
         ;
    CppCodeStyleSettings style;
    style.alignAssignments = true;
    checkIndent(data, style);
}

void tst_CodeFormatter::declarationContinuation()
{
    QList<Line> data;
    data << Line("void foo(")
         << Line("~       int a,")
         << Line("~       int b);")
         << Line("void foo(int a,")
         << Line("~        int b);")
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

void tst_CodeFormatter::bug4()
{
    QList<Line> data;
    data << Line("void test()")
         << Line("{")
         << Line("    int a = 0, b = {0};")
         << Line("    int a = {0}, b = {0};")
         << Line("    int b;")
         << Line("}")
         << Line("int c;")
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
         << Line("    case 4:")
         << Line("    {")
         << Line("        if (a) {")
         << Line("        }")
         << Line("    }")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::switch2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (a) {")
         << Line("        case 1:")
         << Line("            foo;")
         << Line("            if (a);")
         << Line("        case 2:")
         << Line("        case 3: {")
         << Line("            foo;")
         << Line("        }")
         << Line("        case 4:")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        case bar:")
         << Line("            break;")
         << Line("        case 4:")
         << Line("        {")
         << Line("            if (a) {")
         << Line("            }")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::switch3()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (a)")
         << Line("        {")
         << Line("    case 1:")
         << Line("        foo;")
         << Line("        if (a);")
         << Line("    case 2:")
         << Line("    case 3:")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("    case bar:")
         << Line("        break;")
         << Line("    case 4:")
         << Line("        {")
         << Line("            if (a)")
         << Line("                {")
         << Line("                }")
         << Line("        }")
         << Line("        }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentBlockBraces = true;
    codeStyle.indentBlocksRelativeToSwitchLabels= true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::switch4()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (a)")
         << Line("        {")
         << Line("        case 1:")
         << Line("            foo;")
         << Line("            if (a);")
         << Line("        case 2:")
         << Line("        case 4:")
         << Line("            {")
         << Line("                foo;")
         << Line("            }")
         << Line("        case bar:")
         << Line("            break;")
         << Line("        case 4:")
         << Line("            {")
         << Line("                if (a)")
         << Line("                    {")
         << Line("                    }")
         << Line("            }")
         << Line("        }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentBlockBraces = true;
    codeStyle.indentBlocksRelativeToSwitchLabels = true;
    codeStyle.indentSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::switch5()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    switch (a)")
         << Line("        {")
         << Line("        case 1:")
         << Line("            foo;")
         << Line("            if (a);")
         << Line("        case 2:")
         << Line("        case 4:")
         << Line("            {")
         << Line("            foo;")
         << Line("            }")
         << Line("        case bar:")
         << Line("            break;")
         << Line("        case 4:")
         << Line("            {")
         << Line("            if (a)")
         << Line("                {")
         << Line("                }")
         << Line("            }")
         << Line("        }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentBlockBraces = true;
    codeStyle.indentBlockBody = false;
    codeStyle.indentBlocksRelativeToSwitchLabels = true;
    codeStyle.indentSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::blocks()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    int a;")
         << Line("    {")
         << Line("    int b;")
         << Line("    {")
         << Line("    int c;")
         << Line("    }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentBlockBody = false;
    codeStyle.indentBlockBraces = true;
    checkIndent(data, codeStyle);
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
         << Line("    ~            42),")
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

void tst_CodeFormatter::memberInitializer2()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("~   : foo()")
         << Line("~   , foo()")
         << Line("~   , foo()")
         << Line("{}")
         << Line("void foo()")
         << Line("~ : foo()", 0, 4)
         << Line("~ , foo()")
         << Line("~ , foo()")
         << Line("{}")
         << Line("void foo()")
         << Line("~   : foo(),")
         << Line("~     foo(),")
         << Line("~     foo()")
         << Line("{}")
         << Line("void foo()")
         << Line("~ : foo(),", 0, 4)
         << Line("~   foo(),")
         << Line("~   foo()")
         << Line("{}")
         << Line("void foo()")
         << Line("~   :")
         << Line("~     foo(),")
         << Line("~     foo(),")
         << Line("~     foo()")
         << Line("{}")
         << Line("void foo()")
         << Line("~   :")
         << Line("~   foo(),", 0, 6)
         << Line("~   foo(),")
         << Line("~   foo()")
         << Line("{}")
            ;
    checkIndent(data);
}

void tst_CodeFormatter::memberInitializer3()
{
    QList<Line> data;
    data << Line("void foo() :")
         << Line("~   foo(),")
         << Line("~   foo(),")
         << Line("~   foo()")
         << Line("{}")
         << Line("void foo() :")
         << Line("~     foo(),", 0, 4)
         << Line("~     foo(),")
         << Line("~     foo()")
         << Line("{}")
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
         << Line("                fpp;")
         << Line("            }")
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
         << Line("enum E")
         << Line("    {")
         << Line("    FOO")
         << Line("    };")
         << Line("namespace")
         << Line("    {")
         << Line("    int i;")
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

void tst_CodeFormatter::functionsWithExtraSpecifier()
{
    QList<Line> data;
    data << Line("extern void foo() {}")
         << Line("struct Foo bar() {}")
         << Line("enum Foo bar() {}")
         << Line("namespace foo {")
         << Line("}")
         << Line("int a;")
            ;
       checkIndent(data);
}

void tst_CodeFormatter::indentNamespace()
{
    QList<Line> data;
    data << Line("namespace Foo {")
         << Line("int x;")
         << Line("class C;")
         << Line("struct S {")
         << Line("    int a;")
         << Line("};")
         << Line("}")
         << Line("int j;")
         << Line("namespace {")
         << Line("namespace Foo {")
         << Line("int j;")
         << Line("}")
         << Line("int j;")
         << Line("namespace {")
         << Line("int j;")
         << Line("}")
         << Line("}")
         << Line("int j;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::externSpec()
{
    QList<Line> data;
    data << Line("extern void foo() {}")
         << Line("extern \"C\" {")
         << Line("void foo() {}")
         << Line("int a;")
         << Line("class C {")
         << Line("    int a;")
         << Line("}")
         << Line("}")
         << Line("int a;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::indentNamespace2()
{
    QList<Line> data;
    data << Line("namespace Foo {")
         << Line("    int x;")
         << Line("    class C;")
         << Line("    struct S {")
         << Line("        int a;")
         << Line("    };")
         << Line("}")
         << Line("int j;")
         << Line("namespace {")
         << Line("    int j;")
         << Line("    namespace Foo {")
         << Line("        int j;")
         << Line("    }")
         << Line("    int j;")
         << Line("    namespace {")
         << Line("        int j;")
         << Line("    }")
         << Line("}")
         << Line("int j;")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentNamespaceBody = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers1()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("    public:")
         << Line("    int i;")
         << Line("    protected:")
         << Line("    int i;")
         << Line("    private:")
         << Line("    int i;")
         << Line("    private slots:")
         << Line("    void foo();")
         << Line("    signals:")
         << Line("    void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = true;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers2()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("    public:")
         << Line("        int i;")
         << Line("    protected:")
         << Line("        int i;")
         << Line("    private:")
         << Line("        int i;")
         << Line("    private slots:")
         << Line("        void foo();")
         << Line("    signals:")
         << Line("        void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = true;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers3()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("public:")
         << Line("int i;")
         << Line("protected:")
         << Line("int i;")
         << Line("private:")
         << Line("int i;")
         << Line("private slots:")
         << Line("void foo();")
         << Line("signals:")
         << Line("void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = false;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers4()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("public:")
         << Line("    int i;")
         << Line("protected:")
         << Line("    int i;")
         << Line("private:")
         << Line("    int i;")
         << Line("private slots:")
         << Line("    void foo();")
         << Line("signals:")
         << Line("    void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = false;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers5()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("public:")
         << Line("      int i;", 4)
         << Line("protected:")
         << Line("      int i;")
         << Line("private:")
         << Line("  int i;", 6)
         << Line("private slots:")
         << Line("  void foo();")
         << Line("signals:")
         << Line("  void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = false;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::accessSpecifiers6()
{
    // not great, but the best we can do with the current scheme
    QList<Line> data;
    data << Line("class C {")
         << Line("    public:")
         << Line("      int i;", 8)
         << Line("    protected:")
         << Line("      int i;")
         << Line("    private:")
         << Line("  int i;", 6)
         << Line("    private slots:")
         << Line("  void foo();")
         << Line("    signals:")
         << Line("  void foo();")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentAccessSpecifiers = true;
    codeStyle.indentDeclarationsRelativeToAccessSpecifiers = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::functionBodyAndBraces1()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("    int i;")
         << Line("}")
         << Line("void bar()")
         << Line("{")
         << Line("      int i;", 4)
         << Line("      int j;")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentFunctionBody = true;
    codeStyle.indentFunctionBraces = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::functionBodyAndBraces2()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("    {")
         << Line("        int i;")
         << Line("    }")
         << Line("void bar()")
         << Line("    {")
         << Line("          int i;", 8)
         << Line("          int j;")
         << Line("    }")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentFunctionBody = true;
    codeStyle.indentFunctionBraces = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::functionBodyAndBraces3()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("{")
         << Line("int i;")
         << Line("}")
         << Line("void bar()")
         << Line("{")
         << Line("  int i;", 0)
         << Line("  int j;")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentFunctionBody = false;
    codeStyle.indentFunctionBraces = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::functionBodyAndBraces4()
{
    QList<Line> data;
    data << Line("void foo()")
         << Line("    {")
         << Line("    int i;")
         << Line("    }")
         << Line("void bar()")
         << Line("    {")
         << Line("      int i;", 4)
         << Line("      int j;")
         << Line("    };")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentFunctionBody = false;
    codeStyle.indentFunctionBraces = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::constructor1()
{
    QList<Line> data;
    data << Line("class Foo {")
         << Line("    Foo() : _a(0)")
         << Line("        {")
         << Line("        _b = 0")
         << Line("        }")
         << Line("    int _a;")
         << Line("    int _b;")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentFunctionBody = false;
    codeStyle.indentFunctionBraces = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::constructor2()
{
    QList<Line> data;
    data << Line("class Foo {")
         << Line("    Foo() : _a(0)")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("    int _a;")
         << Line("    Foo()")
         << Line("    ~   : _foo(1),")
         << Line("    ~     _bar(2),")
         << Line("    ~     _carooooo(")
         << Line("    ~         foo() + 12),")
         << Line("    ~     _carooooo(foo(),")
         << Line("    ~               12)")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("    int _b;")
         << Line("    Foo()")
         << Line("    ~   : _foo(1)")
         << Line("    ~   , _bar(2)")
         << Line("    ~   , _carooooo(")
         << Line("    ~         foo() + 12)")
         << Line("    ~   , _carooooo(foo(),")
         << Line("    ~               12)")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    checkIndent(data);
}

void tst_CodeFormatter::constructor3()
{
    QList<Line> data;
    data << Line("class Foo {")
         << Line("    Foo() : _a{0}, _b{1, {2, {3, \"foo\"}, 3}}")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("    int _a;")
         << Line("    Foo()")
         << Line("    ~   : _foo{1},")
         << Line("    ~     _bar{2},")
         << Line("    ~     _carooooo(")
         << Line("    ~         foo() + 12),")
         << Line("    ~     _carooooo{foo(),")
         << Line("    ~               12}")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("    int _b;")
         << Line("    Foo()")
         << Line("    ~   : _foo{1}")
         << Line("    ~   , _bar{2}")
         << Line("    ~   , _carooooo{")
         << Line("    ~         foo() + 12}")
         << Line("    ~   , _carooooo{foo(),")
         << Line("    ~               12}")
         << Line("    {")
         << Line("        _b = 0")
         << Line("    }")
         << Line("};")
         ;
    CppCodeStyleSettings codeStyle;
    checkIndent(data);
}

void tst_CodeFormatter::caseBody1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("    case 1:")
         << Line("    a = b;")
         << Line("    break;")
         << Line("    case 2:")
         << Line("    a = b;")
         << Line("    case 3: {")
         << Line("        a = b;")
         << Line("    }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentStatementsRelativeToSwitchLabels = false;
    codeStyle.indentBlocksRelativeToSwitchLabels = false;
    codeStyle.indentControlFlowRelativeToSwitchLabels = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::caseBody2()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("    case 1:")
         << Line("        a = b;")
         << Line("        break;")
         << Line("    case 2:")
         << Line("        a = b;")
         << Line("    case 3: {")
         << Line("        a = b;")
         << Line("    }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentStatementsRelativeToSwitchLabels = true;
    codeStyle.indentBlocksRelativeToSwitchLabels = false;
    codeStyle.indentControlFlowRelativeToSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::caseBody3()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("    case 1:")
         << Line("        a = b;")
         << Line("        break;")
         << Line("    case 2:")
         << Line("        a = b;")
         << Line("    case 3: {")
         << Line("            a = b;")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentStatementsRelativeToSwitchLabels = true;
    codeStyle.indentBlocksRelativeToSwitchLabels = true;
    codeStyle.indentControlFlowRelativeToSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::caseBody4()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("        case 1:")
         << Line("        a = b;")
         << Line("        break;")
         << Line("        case 2:")
         << Line("        a = b;")
         << Line("        case 3: {")
         << Line("            a = b;")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentSwitchLabels = true;
    codeStyle.indentStatementsRelativeToSwitchLabels = false;
    codeStyle.indentBlocksRelativeToSwitchLabels = false;
    codeStyle.indentControlFlowRelativeToSwitchLabels = false;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::caseBody5()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("        case 1:")
         << Line("            a = b;")
         << Line("            break;")
         << Line("        case 2:")
         << Line("            a = b;")
         << Line("        case 3: {")
         << Line("            a = b;")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentSwitchLabels = true;
    codeStyle.indentStatementsRelativeToSwitchLabels = true;
    codeStyle.indentBlocksRelativeToSwitchLabels = false;
    codeStyle.indentControlFlowRelativeToSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::caseBody6()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    switch (f) {")
         << Line("        case 1:")
         << Line("            a = b;")
         << Line("            break;")
         << Line("        case 2:")
         << Line("            a = b;")
         << Line("        case 3: {")
         << Line("                a = b;")
         << Line("            }")
         << Line("    }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentSwitchLabels = true;
    codeStyle.indentStatementsRelativeToSwitchLabels = true;
    codeStyle.indentBlocksRelativeToSwitchLabels = true;
    codeStyle.indentControlFlowRelativeToSwitchLabels = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::blockBraces1()
{
    QList<Line> data;
    data << Line("void foo() {")
         << Line("    if (a) {")
         << Line("            int a;")
         << Line("        }")
         << Line("    if (a)")
         << Line("        {")
         << Line("            int a;")
         << Line("        }")
         << Line("}")
         ;
    CppCodeStyleSettings codeStyle;
    codeStyle.indentBlockBraces = true;
    checkIndent(data, codeStyle);
}

void tst_CodeFormatter::functionDefaultArgument()
{
    QList<Line> data;
    data << Line("void foo(int a = 3) {")
         << Line("    if (a)")
         << Line("        int a;")
         << Line("}")
         << Line("int b;")
         ;
    checkIndent(data);
}

void tst_CodeFormatter::attributeInAccessSpecifier()
{
    QList<Line> data;
    data << Line("class C {")
         << Line("public __attribute__((annotate(\"foo\"))):")
         << Line("    int a;")
         << Line("private __attribute__((annotate(\"foo\"))):")
         << Line("    int a;")
         << Line("};")
         << Line("int b;")
         ;
    checkIndent(data);
}

QTEST_MAIN(tst_CodeFormatter)

#include "tst_codeformatter.moc"


