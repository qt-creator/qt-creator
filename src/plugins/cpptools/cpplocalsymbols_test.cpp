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

#include "cpptoolsplugin.h"

#include "cpplocalsymbols.h"
#include "cppsemanticinfo.h"

#include <cplusplus/Overview.h>
#include <utils/algorithm.h>

#include <QtTest>

namespace {

class FindFirstFunctionDefinition: protected CPlusPlus::ASTVisitor
{
public:
    FindFirstFunctionDefinition(CPlusPlus::TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
        , m_definition(0)
    {}

    CPlusPlus::FunctionDefinitionAST *operator()()
    {
        accept(translationUnit()->ast());
        return m_definition;
    }

private:
    bool preVisit(CPlusPlus::AST *ast)
    {
        if (CPlusPlus::FunctionDefinitionAST *f = ast->asFunctionDefinition()) {
            m_definition = f;
            return false;
        }
        return true;
    }

private:
    CPlusPlus::FunctionDefinitionAST *m_definition;
};

struct Result
{
    Result() : line(0), column(0), length(0) {}
    Result(const QByteArray &name, unsigned line, unsigned column, unsigned length)
        : name(name), line(line), column(column), length(length)
    {}

    QByteArray name;
    unsigned line;
    unsigned column;
    unsigned length;

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

    static QList<Result> fromLocalUses(CppTools::SemanticInfo::LocalUseMap localUses)
    {
        QList<Result> result;

        CppTools::SemanticInfo::LocalUseIterator it(localUses);
        while (it.hasNext()) {
            it.next();
            const CPlusPlus::Symbol *symbol = it.key();
            const QList<CppTools::SemanticInfo::Use> &uses = it.value();
            foreach (const CppTools::SemanticInfo::Use &use, uses)
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

namespace CppTools {
namespace Internal {

void CppToolsPlugin::test_cpplocalsymbols_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QList<Result>>("expectedUses");

    typedef QByteArray _;

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

void CppToolsPlugin::test_cpplocalsymbols()
{
    QFETCH(QByteArray, source);
    QFETCH(QList<Result>, expectedUses);

    CPlusPlus::Document::Ptr document = CPlusPlus::Document::create(QLatin1String("test.cpp"));
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
//    foreach (const Result &result, actualUses)
//        qDebug() << QTest::toString(result);
    QCOMPARE(actualUses, expectedUses);
}

} // namespace Internal
} // namespace CppTools
