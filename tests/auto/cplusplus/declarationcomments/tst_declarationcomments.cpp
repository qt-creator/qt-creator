// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cplusplus/Overview.h"
#include <cplusplus/declarationcomments.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/SymbolVisitor.h>

#include <QtTest>
#include <QTextDocument>

using namespace CPlusPlus;
using namespace Utils;

const char testSource[] = R"TheFile(
//! Unrelated comment, even though it mentions MyEnum.

//! Related comment because of proximity.
//! Related comment that mentions MyEnum.
//! Related comment that mentions MyEnumA.

enum MyEnum { MyEnumA, MyEnumB };

// Unrelated comment because of different comment type.
//! Related comment that mentions MyEnumClass::A.

enum class MyEnumClass { A, B };

// Related comment because of proximity.
void myFunc();

/*
 * Related comment because of proximity.
 */
template<typename T> class MyClassTemplate {};

/*
 * Related comment, because it mentions MyOtherClass.
 */

class MyOtherClass {
    /// Related comment for function and parameter a2, but not parameter a.
    /// @param a2
    void memberFunc(int a, int a2);
};

//! Comment for member function at implementation site.
void MyOtherClass::memberFunc(int, int) {}

//! An unrelated comment

void funcWithoutComment();

)TheFile";

class SymbolFinder : public SymbolVisitor
{
public:
    SymbolFinder(const QString &name, Document *doc, int expectedOccurrence)
        : m_name(name), m_doc(doc), m_expectedOccurrence(expectedOccurrence) {}
    const Symbol *find();

private:
    bool preVisit(Symbol *) override { return !m_symbol; }
    bool visit(Namespace *ns) override { return visitScope(ns); }
    bool visit(Enum *e) override { return visitScope(e); }
    bool visit(Class *c) override { return visitScope(c); }
    bool visit(Function *f) override { return visitScope(f); }
    bool visit(Argument *a) override;
    bool visit(Declaration *d) override;

    bool visitScope(Scope *scope);

    const QString &m_name;
    Document * const m_doc;
    const Symbol *m_symbol = nullptr;
    const int m_expectedOccurrence;
    int m_tokenLocation = -1;
};

class TestDeclarationComments : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void commentsForDecl_data();
    void commentsForDecl();

private:
    Snapshot m_snapshot;
    Document::Ptr m_cppDoc;
    QTextDocument m_textDoc;
};

void TestDeclarationComments::initTestCase()
{
    m_cppDoc = m_snapshot.preprocessedDocument(testSource, "dummy.cpp");
    m_cppDoc->check();
    const bool hasErrors = !m_cppDoc->diagnosticMessages().isEmpty();
    if (hasErrors) {
        for (const Document::DiagnosticMessage &msg : m_cppDoc->diagnosticMessages())
            qDebug() << '\t' << msg.text();
    }
    QVERIFY(!hasErrors);
    m_textDoc.setPlainText(testSource);
}

void TestDeclarationComments::commentsForDecl_data()
{
    QTest::addColumn<QString>("symbolName");
    QTest::addColumn<QString>("expectedCommentPrefix");
    QTest::addColumn<int>("occurrence");

    QTest::newRow("enum type") << "MyEnum" << "//! Related comment because of proximity" << 1;
    QTest::newRow("enum value (positive)") << "MyEnumA"
                                           << "//! Related comment because of proximity" << 1;
    QTest::newRow("enum value (negative") << "MyEnumB" << QString() << 1;

    QTest::newRow("enum class type") << "MyEnumClass"
                                     << "//! Related comment that mentions MyEnumClass::A" << 1;
    QTest::newRow("enum class value (positive)")
        << "A" << "//! Related comment that mentions MyEnumClass::A" << 1;
    QTest::newRow("enum class value (negative") << "B" << QString() << 1;

    QTest::newRow("function declaration with comment")
        << "myFunc" << "// Related comment because of proximity" << 1;

    QTest::newRow("class template")
        << "MyClassTemplate" << "/*\n * Related comment because of proximity." << 1;

    QTest::newRow("class")
        << "MyOtherClass" << "/*\n * Related comment, because it mentions MyOtherClass." << 1;
    QTest::newRow("member function declaration")
        << "memberFunc" << "/// Related comment for function and parameter a2, but not parameter a."
        << 1;
    QTest::newRow("function parameter (negative)") << "a" << QString() << 1;
    QTest::newRow("function parameter (positive)") << "a2" << "/// Related comment for function "
                                                      "and parameter a2, but not parameter a." << 1;

    QTest::newRow("member function definition") << "memberFunc" <<
        "//! Comment for member function at implementation site." << 2;

    QTest::newRow("function declaration without comment") << "funcWithoutComment" << QString() << 1;
}

void TestDeclarationComments::commentsForDecl()
{
    QFETCH(QString, symbolName);
    QFETCH(QString, expectedCommentPrefix);
    QFETCH(int, occurrence);

    SymbolFinder finder(symbolName, m_cppDoc.get(), occurrence);
    const Symbol * const symbol = finder.find();
    QVERIFY(symbol);

    const QList<Token> commentTokens = commentsForDeclaration(symbol, m_textDoc, m_cppDoc);
    if (expectedCommentPrefix.isEmpty()) {
        QVERIFY(commentTokens.isEmpty());
        return;
    }
    QVERIFY(!commentTokens.isEmpty());

    const int firstCommentPos = m_cppDoc->translationUnit()->getTokenPositionInDocument(
        commentTokens.first(), &m_textDoc);
    const QString actualCommentPrefix = m_textDoc.toPlainText().mid(firstCommentPos,
                                                                    expectedCommentPrefix.length());
    QCOMPARE(actualCommentPrefix, expectedCommentPrefix);
}


const Symbol *SymbolFinder::find()
{
    for (int i = 0, occurrences = 0; i < m_doc->translationUnit()->tokenCount(); ++i) {
        const Token &tok = m_doc->translationUnit()->tokenAt(i);
        if (tok.kind() != T_IDENTIFIER)
            continue;
        if (tok.spell() != m_name)
            continue;
        if (++occurrences == m_expectedOccurrence) {
            m_tokenLocation = i;
            break;
        }
    }
    if (m_tokenLocation == -1)
        return nullptr;

    visit(m_doc->globalNamespace());
    return m_symbol;
}

bool SymbolFinder::visit(Argument *a)
{
    if (a->sourceLocation() == m_tokenLocation)
        m_symbol = a;
    return false;
}

bool SymbolFinder::visit(Declaration *d)
{
    if (d->sourceLocation() == m_tokenLocation) {
        m_symbol = d;
        return false;
    }
    if (const auto func = d->type().type()->asFunctionType())
        return visit(func);
    return true;
}

bool SymbolFinder::visitScope(Scope *scope)
{
    for (int i = 0; i < scope->memberCount(); ++i) {
        Symbol * const s = scope->memberAt(i);
        if (s->sourceLocation() == m_tokenLocation) {
            m_symbol = s;
            return false;
        }
        accept(s);
    }
    return false;
}

QTEST_APPLESS_MAIN(TestDeclarationComments)
#include "tst_declarationcomments.moc"
