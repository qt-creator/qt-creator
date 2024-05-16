// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "converttometamethodcall.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class ConvertToMetaMethodCallOp : public CppQuickFixOperation
{
public:
    ConvertToMetaMethodCallOp(const CppQuickFixInterface &interface, CallAST *callAst)
        : CppQuickFixOperation(interface), m_callAst(callAst)
    {
        setDescription(Tr::tr("Convert Function Call to Qt Meta-Method Invocation"));
    }

private:
    void perform() override
    {
        // Construct the argument list.
        Overview ov;
        QStringList arguments;
        for (ExpressionListAST *it = m_callAst->expression_list; it; it = it->next) {
            if (!it->value)
                return;
            const FullySpecifiedType argType
                = typeOfExpr(it->value, currentFile(), snapshot(), context());
            if (!argType.isValid())
                return;
            arguments << QString::fromUtf8("Q_ARG(%1, %2)")
                             .arg(ov.prettyType(argType), currentFile()->textOf(it->value));
        }
        QString argsString = arguments.join(", ");
        if (!argsString.isEmpty())
            argsString.prepend(", ");

        // Construct the replace string.
        const auto memberAccessAst = m_callAst->base_expression->asMemberAccess();
        QTC_ASSERT(memberAccessAst, return);
        QString baseExpr = currentFile()->textOf(memberAccessAst->base_expression);
        const FullySpecifiedType baseExprType
            = typeOfExpr(memberAccessAst->base_expression, currentFile(), snapshot(), context());
        if (!baseExprType.isValid())
            return;
        if (!baseExprType->asPointerType())
            baseExpr.prepend('&');
        const QString functionName = currentFile()->textOf(memberAccessAst->member_name);
        const QString qMetaObject = "QMetaObject";
        const QString newCall = QString::fromUtf8("%1::invokeMethod(%2, \"%3\"%4)")
                                    .arg(qMetaObject, baseExpr, functionName, argsString);

        // Determine the start and end positions of the replace operation.
        // If the call is preceded by an "emit" keyword, that one has to be removed as well.
        int firstToken = m_callAst->firstToken();
        if (firstToken > 0)
            switch (semanticInfo().doc->translationUnit()->tokenKind(firstToken - 1)) {
            case T_EMIT: case T_Q_EMIT: --firstToken; break;
            default: break;
            }
        const TranslationUnit *const tu = semanticInfo().doc->translationUnit();
        const int startPos = tu->getTokenPositionInDocument(firstToken, textDocument());
        const int endPos = tu->getTokenPositionInDocument(m_callAst->lastToken(), textDocument());

        // Replace the old call with the new one.
        ChangeSet changes;
        changes.replace(startPos, endPos, newCall);

        // Insert include for QMetaObject, if necessary.
        const Identifier qMetaObjectId(qPrintable(qMetaObject), qMetaObject.size());
        Scope * const scope = currentFile()->scopeAt(firstToken);
        const QList<LookupItem> results = context().lookup(&qMetaObjectId, scope);
        bool isDeclared = false;
        for (const LookupItem &item : results) {
            if (Symbol *declaration = item.declaration(); declaration && declaration->asClass()) {
                isDeclared = true;
                break;
            }
        }
        if (!isDeclared) {
            insertNewIncludeDirective('<' + qMetaObject + '>', currentFile(), semanticInfo().doc,
                                      changes);
        }

        // Apply the changes.
        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

    const CallAST * const m_callAst;
};

//! Converts a normal function call into a meta method invocation, if the functions is
//! marked as invokable.
class ConvertToMetaMethodCall : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif
private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const Document::Ptr &cppDoc = interface.currentFile()->cppDocument();
        const QList<AST *> path = ASTPath(cppDoc)(interface.cursor());
        if (path.isEmpty())
            return;

        // Are we on a member function call?
        CallAST *callAst = nullptr;
        for (auto it = path.crbegin(); it != path.crend(); ++it) {
            if ((callAst = (*it)->asCall()))
                break;
        }
        if (!callAst || !callAst->base_expression)
            return;
        ExpressionAST *baseExpr = nullptr;
        const NameAST *nameAst = nullptr;
        if (const MemberAccessAST * const ast = callAst->base_expression->asMemberAccess()) {
            baseExpr = ast->base_expression;
            nameAst = ast->member_name;
        }
        if (!baseExpr || !nameAst || !nameAst->name)
            return;

        // Locate called function and check whether it is invokable.
        Scope *scope = cppDoc->globalNamespace();
        for (auto it = path.crbegin(); it != path.crend(); ++it) {
            if (const CompoundStatementAST * const stmtAst = (*it)->asCompoundStatement()) {
                scope = stmtAst->symbol;
                break;
            }
        }
        const LookupContext context(cppDoc, interface.snapshot());
        TypeOfExpression exprType;
        exprType.setExpandTemplates(true);
        exprType.init(cppDoc, interface.snapshot());
        const QList<LookupItem> typeMatches = exprType(callAst->base_expression, cppDoc, scope);
        for (const LookupItem &item : typeMatches) {
            if (const auto func = item.type()->asFunctionType(); func && func->methodKey()) {
                result << new ConvertToMetaMethodCallOp(interface, callAst);
                return;
            }
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class ConvertToMetaMethodCallTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QByteArray>("expected");

        // ^ marks the cursor locations.
        // $ marks the replacement regions.
        // The quoted string in the comment is the data tag.
        // The rest of the comment is the replacement string.
        const QByteArray allCases = R"(
class C {
public:
    C() {
        $this->^aSignal()$; // "signal from region on pointer to object" QMetaObject::invokeMethod(this, "aSignal")
        C c;
        $c.^aSignal()$; // "signal from region on object value" QMetaObject::invokeMethod(&c, "aSignal")
        $(new C)->^aSignal()$; // "signal from region on expression" QMetaObject::invokeMethod((new C), "aSignal")
        $emit this->^aSignal()$; // "signal from region, with emit" QMetaObject::invokeMethod(this, "aSignal")
        $Q_EMIT this->^aSignal()$; // "signal from region, with Q_EMIT" QMetaObject::invokeMethod(this, "aSignal")
        $this->^aSlot()$; // "slot from region" QMetaObject::invokeMethod(this, "aSlot")
        $this->^noArgs()$; // "Q_SIGNAL, no arguments" QMetaObject::invokeMethod(this, "noArgs")
        $this->^oneArg(0)$; // "Q_SLOT, one argument" QMetaObject::invokeMethod(this, "oneArg", Q_ARG(int, 0))
        $this->^twoArgs(0, c)$; // "Q_INVOKABLE, two arguments" QMetaObject::invokeMethod(this, "twoArgs", Q_ARG(int, 0), Q_ARG(C, c))
        this->^notInvokable(); // "not invokable"
    }

signals:
    void aSignal();

private slots:
    void aSlot();

private:
    Q_SIGNAL void noArgs();
    Q_SLOT void oneArg(int index);
    Q_INVOKABLE void twoArgs(int index, const C &value);
    void notInvokable();
};
)";

        qsizetype nextCursor = allCases.indexOf('^');
        while (nextCursor != -1) {
            const int commentStart = allCases.indexOf("//", nextCursor);
            QVERIFY(commentStart != -1);
            const int tagStart = allCases.indexOf('"', commentStart + 2);
            QVERIFY(tagStart != -1);
            const int tagEnd = allCases.indexOf('"', tagStart + 1);
            QVERIFY(tagEnd != -1);
            QByteArray input = allCases;
            QByteArray output = allCases;
            input.replace(nextCursor, 1, "@");
            const QByteArray tag = allCases.mid(tagStart + 1, tagEnd - tagStart - 1);
            const int prevNewline = allCases.lastIndexOf('\n', nextCursor);
            const int regionStart = allCases.lastIndexOf('$', nextCursor);
            bool hasReplacement = false;
            if (regionStart != -1 && regionStart > prevNewline) {
                const int regionEnd = allCases.indexOf('$', regionStart + 1);
                QVERIFY(regionEnd != -1);
                const int nextNewline = allCases.indexOf('\n', tagEnd);
                QVERIFY(nextNewline != -1);
                const QByteArray replacement
                    = allCases.mid(tagEnd + 1, nextNewline - tagEnd - 1).trimmed();
                output.replace(regionStart, regionEnd - regionStart, replacement);
                hasReplacement = true;
            }
            static const auto matcher = [](char c) { return c == '^' || c == '$'; };
            input.removeIf(matcher);
            if (hasReplacement) {
                output.removeIf(matcher);
                output.prepend("#include <QMetaObject>\n\n");
            } else {
                output.clear();
            }
            QTest::newRow(tag.data()) << input << output;
            nextCursor = allCases.indexOf('^', nextCursor + 1);
        }
    }

    void test()
    {
        QFETCH(QByteArray, input);
        QFETCH(QByteArray, expected);
        ConvertToMetaMethodCall factory;
        QuickFixOperationTest({CppTestDocument::create("file.cpp", input, expected)}, &factory);
    }
};

QObject *ConvertToMetaMethodCall::createTest() { return new ConvertToMetaMethodCallTest; }

#endif // WITH_TESTS
} // namespace

void registerConvertToMetaMethodCallQuickfix()
{
    CppQuickFixFactory::registerFactory<ConvertToMetaMethodCall>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <converttometamethodcall.moc>
#endif
