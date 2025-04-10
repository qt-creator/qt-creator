// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completeswitchstatement.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class CaseStatementCollector : public ASTVisitor
{
public:
    CaseStatementCollector(Document::Ptr document, const Snapshot &snapshot,
                           Scope *scope)
        : ASTVisitor(document->translationUnit()),
        document(document),
        scope(scope)
    {
        typeOfExpression.init(document, snapshot);
    }

    QStringList operator ()(AST *ast)
    {
        values.clear();
        foundCaseStatementLevel = false;
        accept(ast);
        return values;
    }

    bool preVisit(AST *ast) override {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *csExpression = cs->expression) {
                if (ExpressionAST *expression = csExpression->asIdExpression()) {
                    QList<LookupItem> candidates = typeOfExpression(expression, document, scope);
                    if (!candidates.isEmpty() && candidates.first().declaration()) {
                        Symbol *decl = candidates.first().declaration();
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                    }
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel = false;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class CompleteSwitchCaseStatementOp: public CppQuickFixOperation
{
public:
    CompleteSwitchCaseStatementOp(const CppQuickFixInterface &interface,
                                  int priority, CompoundStatementAST *compoundStatement, const QStringList &values)
        : CppQuickFixOperation(interface, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(Tr::tr("Complete Switch Statement"));
    }

    void perform() override
    {
        currentFile()->apply(ChangeSet::makeInsert(
            currentFile()->endOf(compoundStatement->lbrace_token),
            QLatin1String("\ncase ") + values.join(QLatin1String(":\nbreak;\ncase "))
                + QLatin1String(":\nbreak;")));
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

static Enum *findEnum(const QList<LookupItem> &results, const LookupContext &ctxt)
{
    for (const LookupItem &result : results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            if (ClassOrNamespace *con = ctxt.lookupType(namedType->name(), result.scope())) {
                QList<Enum *> enums = con->unscopedEnums();
                const QList<Symbol *> symbols = con->symbols();
                for (Symbol * const s : symbols) {
                    if (const auto e = s->asEnum())
                        enums << e;
                }
                const Name *referenceName = namedType->name();
                if (const QualifiedNameId *qualifiedName = referenceName->asQualifiedNameId())
                    referenceName = qualifiedName->name();
                for (Enum *e : std::as_const(enums)) {
                    if (const Name *candidateName = e->name()) {
                        if (candidateName->match(referenceName))
                            return e;
                    }
                }
            }
        }
    }

    return nullptr;
}

Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface.semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.setExpandTemplates(true);
    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       interface.semanticInfo().doc,
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

//! Adds missing case statements for "switch (enumVariable)"
class CompleteSwitchStatement: public CppQuickFixFactory
{
public:
    CompleteSwitchStatement() { setClangdReplacement({12}); }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();

        if (path.isEmpty())
            return;

        // look for switch statement
        for (int depth = path.size() - 1; depth >= 0; --depth) {
            AST *ast = path.at(depth);
            SwitchStatementAST *switchStatement = ast->asSwitchStatement();
            if (switchStatement) {
                if (!switchStatement->statement || !switchStatement->symbol)
                    return;
                CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
                if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                    return;
                // look if the condition's type is an enum
                if (Enum *e = conditionEnum(interface, switchStatement)) {
                    // check the possible enum values
                    QStringList values;
                    Overview prettyPrint;
                    for (int i = 0; i < e->memberCount(); ++i) {
                        if (Declaration *decl = e->memberAt(i)->asDeclaration())
                            values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                    }
                    // Get the used values
                    Block *block = switchStatement->symbol;
                    CaseStatementCollector caseValues(interface.semanticInfo().doc, interface.snapshot(),
                                                      interface.semanticInfo().doc->scopeAt(block->line(), block->column()));
                    const QStringList usedValues = caseValues(switchStatement);
                    // save the values that would be added
                    for (const QString &usedValue : usedValues)
                        values.removeAll(usedValue);
                    if (!values.isEmpty())
                        result << new CompleteSwitchCaseStatementOp(interface, depth,
                                                                    compoundStatement, values);
                    return;
                }

                return;
            }
        }
    }
};

#ifdef WITH_TESTS
class CompleteSwitchStatementTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerCompleteSwitchStatementQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(CompleteSwitchStatement);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <completeswitchstatement.moc>
#endif
