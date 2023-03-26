// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocalsymbols_test.h"

#include "cpplocalsymbols.h"
#include "cppsemanticinfo.h"

#include <cplusplus/Overview.h>
#include <utils/algorithm.h>

#include <QtTest>

using namespace Utils;

namespace {

class FindFirstFunctionDefinition: protected CPlusPlus::ASTVisitor
{
public:
    explicit FindFirstFunctionDefinition(CPlusPlus::TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    {}

    CPlusPlus::FunctionDefinitionAST *operator()()
    {
        accept(translationUnit()->ast());
        return m_definition;
    }

private:
    bool preVisit(CPlusPlus::AST *ast) override
    {
        if (CPlusPlus::FunctionDefinitionAST *f = ast->asFunctionDefinition()) {
            m_definition = f;
            return false;
        }
        return true;
    }

private:
    CPlusPlus::FunctionDefinitionAST *m_definition = nullptr;
};

struct Result
{
    Result() = default;
    Result(const QByteArray &name, unsigned line, unsigned column, unsigned length)
        : name(name), line(line), column(column), length(length)
    {}

    QByteArray name;
    unsigned line = 0;
    unsigned column = 0;
    unsigned length = 0;

    bool operator==(const Result &other) const
    {
        return name == other.name
                && line == other.line
                && column == other.column
                && length == other.length;
    }

    static Result fromHighlightingResult(const CPlusPlus::Symbol *symbol,
                                         TextEditor::HighlightingResult result)
    {
        const QByteArray name = CPlusPlus::Overview().prettyName(symbol->name()).toLatin1();
        return Result(name, result.line, result.column, result.length);
    }

    static QList<Result> fromLocalUses(CppEditor::SemanticInfo::LocalUseMap localUses)
    {
        QList<Result> result;

        for (auto it = localUses.cbegin(), end = localUses.cend(); it != end; ++it) {
            const CPlusPlus::Symbol *symbol = it.key();
            const QList<CppEditor::SemanticInfo::Use> &uses = it.value();
            for (const CppEditor::SemanticInfo::Use &use : uses)
                result << fromHighlightingResult(symbol, use);
        }

        Utils::sort(result, [](const Result &r1, const Result &r2) -> bool {
            if (r1.line == r2.line)
                return r1.column < r2.column;
            return r1.line < r2.line;
        });
        return result;
    }
};

} // anonymous namespace

Q_DECLARE_METATYPE(Result)

QT_BEGIN_NAMESPACE
namespace QTest {
    template<>
    char *toString(const Result &result)
    {
        QByteArray ba = "Result(";
        ba += "_(\"" + result.name + "\"), ";
        ba += QByteArray::number(result.line) + ", ";
        ba += QByteArray::number(result.column) + ", ";
        ba += QByteArray::number(result.length) + ")";
        return qstrdup(ba.data());
    }
}
QT_END_NAMESPACE

namespace CppEditor::Internal {

void LocalSymbolsTest::test_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QList<Result>>("expectedUses");

    using _ = QByteArray;

    QTest::newRow("basic")
        << _("int f(int arg)\n"
             "{\n"
             "    int local;\n"
             "    g(&local);\n"
             "    return local + arg;\n"
             "}\n")
        << (QList<Result>()
            << Result(_("arg"), 0, 10, 3)
            << Result(_("local"), 2, 9, 5)
            << Result(_("local"), 3, 8, 5)
            << Result(_("local"), 4, 12, 5)
            << Result(_("arg"), 4, 20, 3));

    QTest::newRow("lambda")
        << _("void f()\n"
             "{\n"
             "    auto func = [](int arg) { return arg; };\n"
             "    func(1);\n"
             "}\n")
        << (QList<Result>()
            << Result(_("func"), 2, 10, 4)
            << Result(_("arg"), 2, 24, 3)
            << Result(_("arg"), 2, 38, 3)
            << Result(_("func"), 3, 5, 4));
}

void LocalSymbolsTest::test()
{
    QFETCH(QByteArray, source);
    QFETCH(QList<Result>, expectedUses);

    CPlusPlus::Document::Ptr document =
            CPlusPlus::Document::create(FilePath::fromPathPart(u"test.cpp"));
    document->setUtf8Source(source);
    document->check();
    QVERIFY(document->diagnosticMessages().isEmpty());
    QVERIFY(document->translationUnit());
    QVERIFY(document->translationUnit()->ast());
    QVERIFY(document->globalNamespace());
    FindFirstFunctionDefinition find(document->translationUnit());
    CPlusPlus::DeclarationAST *functionDefinition = find();

    LocalSymbols localSymbols(document, functionDefinition);

    const QList<Result> actualUses = Result::fromLocalUses(localSymbols.uses);
//    for (const Result &result : actualUses)
//        qDebug() << QTest::toString(result);
    QCOMPARE(actualUses, expectedUses);
}

} // namespace CppEditor::Internal
