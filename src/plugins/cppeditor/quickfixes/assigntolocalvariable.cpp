// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assigntolocalvariable.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixprojectsettings.h"

#include <cplusplus/CppRewriter.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <projectexplorer/projecttree.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class AssignToLocalVariableOperation : public CppQuickFixOperation
{
public:
    explicit AssignToLocalVariableOperation(const CppQuickFixInterface &interface,
                                            const int insertPos, const AST *ast, const Name *name)
        : CppQuickFixOperation(interface)
        , m_insertPos(insertPos)
        , m_ast(ast)
        , m_name(name)
        , m_oo(CppCodeStyleSettings::currentProjectCodeStyleOverview())
        , m_originalName(m_oo.prettyName(m_name))
        , m_file(interface.currentFile())
    {
        setDescription(Tr::tr("Assign to Local Variable"));
    }

private:
    void perform() override
    {
        QString type = deduceType();
        if (type.isEmpty())
            return;
        const int origNameLength = m_originalName.length();
        const QString varName = constructVarName();
        const QString insertString = type.replace(type.length() - origNameLength, origNameLength,
                                                  varName + QLatin1String(" = "));
        m_file->apply(ChangeSet::makeInsert(m_insertPos, insertString));

        // move cursor to new variable name
        QTextCursor c = m_file->cursor();
        c.setPosition(m_insertPos + insertString.length() - varName.length() - 3);
        c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        editor()->setTextCursor(c);
    }

    QString deduceType() const
    {
        const auto settings = CppQuickFixProjectsSettings::getQuickFixSettings(
            ProjectExplorer::ProjectTree::currentProject());
        if (m_file->cppDocument()->languageFeatures().cxx11Enabled && settings->useAuto)
            return "auto " + m_originalName;

        TypeOfExpression typeOfExpression;
        typeOfExpression.init(semanticInfo().doc, snapshot(), context().bindings());
        typeOfExpression.setExpandTemplates(true);
        Scope * const scope = m_file->scopeAt(m_ast->firstToken());
        const QList<LookupItem> result = typeOfExpression(m_file->textOf(m_ast).toUtf8(),
                                                          scope, TypeOfExpression::Preprocess);
        if (result.isEmpty())
            return {};

        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(result.first().scope());
        ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
        if (!con)
            con = typeOfExpression.context().globalNamespace();
        UseMinimalNames q(con);
        env.enter(&q);

        Control *control = context().bindings()->control().get();
        FullySpecifiedType type = rewriteType(result.first().type(), &env, control);

        return m_oo.prettyType(type, m_name);
    }

    QString constructVarName() const
    {
        QString newName = m_originalName;
        if (newName.startsWith(QLatin1String("get"), Qt::CaseInsensitive)
            && newName.length() > 3
            && newName.at(3).isUpper()) {
            newName.remove(0, 3);
            newName.replace(0, 1, newName.at(0).toLower());
        } else if (newName.startsWith(QLatin1String("to"), Qt::CaseInsensitive)
                   && newName.length() > 2
                   && newName.at(2).isUpper()) {
            newName.remove(0, 2);
            newName.replace(0, 1, newName.at(0).toLower());
        } else {
            newName.replace(0, 1, newName.at(0).toUpper());
            newName.prepend(QLatin1String("local"));
        }
        return newName;
    }

    const int m_insertPos;
    const AST * const m_ast;
    const Name * const m_name;
    const Overview m_oo;
    const QString m_originalName;
    const CppRefactoringFilePtr m_file;
};

//! Assigns the return value of a function call or a new expression to a local variable
class AssignToLocalVariable : public CppQuickFixFactory
{
public:
    AssignToLocalVariable()
    {
        setClangdReplacement({20});
    }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        AST *outerAST = nullptr;
        SimpleNameAST *nameAST = nullptr;

        for (int i = path.size() - 3; i >= 0; --i) {
            if (CallAST *callAST = path.at(i)->asCall()) {
                if (!interface.isCursorOn(callAST))
                    return;
                if (i - 2 >= 0) {
                    const int idx = i - 2;
                    if (path.at(idx)->asSimpleDeclaration())
                        return;
                    if (path.at(idx)->asExpressionStatement())
                        return;
                    if (path.at(idx)->asMemInitializer())
                        return;
                    if (path.at(idx)->asCall()) { // Fallback if we have a->b()->c()...
                        --i;
                        continue;
                    }
                }
                for (int a = i - 1; a > 0; --a) {
                    if (path.at(a)->asBinaryExpression())
                        return;
                    if (path.at(a)->asReturnStatement())
                        return;
                    if (path.at(a)->asCall())
                        return;
                }

                if (MemberAccessAST *member = path.at(i + 1)->asMemberAccess()) { // member
                    if (NameAST *name = member->member_name)
                        nameAST = name->asSimpleName();
                } else if (QualifiedNameAST *qname = path.at(i + 2)->asQualifiedName()) { // static or
                    nameAST = qname->unqualified_name->asSimpleName();                    // func in ns
                } else { // normal
                    nameAST = path.at(i + 2)->asSimpleName();
                }

                if (nameAST) {
                    outerAST = callAST;
                    break;
                }
            } else if (NewExpressionAST *newexp = path.at(i)->asNewExpression()) {
                if (!interface.isCursorOn(newexp))
                    return;
                if (i - 2 >= 0) {
                    const int idx = i - 2;
                    if (path.at(idx)->asSimpleDeclaration())
                        return;
                    if (path.at(idx)->asExpressionStatement())
                        return;
                    if (path.at(idx)->asMemInitializer())
                        return;
                }
                for (int a = i - 1; a > 0; --a) {
                    if (path.at(a)->asReturnStatement())
                        return;
                    if (path.at(a)->asCall())
                        return;
                }

                if (NamedTypeSpecifierAST *ts = path.at(i + 2)->asNamedTypeSpecifier()) {
                    nameAST = ts->name->asSimpleName();
                    outerAST = newexp;
                    break;
                }
            }
        }

        if (outerAST && nameAST) {
            const CppRefactoringFilePtr file = interface.currentFile();
            QList<LookupItem> items;
            TypeOfExpression typeOfExpression;
            typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot(),
                                  interface.context().bindings());
            typeOfExpression.setExpandTemplates(true);

            // If items are empty, AssignToLocalVariableOperation will fail.
            items = typeOfExpression(file->textOf(outerAST).toUtf8(),
                                     file->scopeAt(outerAST->firstToken()),
                                     TypeOfExpression::Preprocess);
            if (items.isEmpty())
                return;

            if (CallAST *callAST = outerAST->asCall()) {
                items = typeOfExpression(file->textOf(callAST->base_expression).toUtf8(),
                                         file->scopeAt(callAST->base_expression->firstToken()),
                                         TypeOfExpression::Preprocess);
            } else {
                items = typeOfExpression(file->textOf(nameAST).toUtf8(),
                                         file->scopeAt(nameAST->firstToken()),
                                         TypeOfExpression::Preprocess);
            }

            for (const LookupItem &item : std::as_const(items)) {
                if (!item.declaration())
                    continue;

                if (Function *func = item.declaration()->asFunction()) {
                    if (func->isSignal() || func->returnType()->asVoidType())
                        return;
                } else if (Declaration *dec = item.declaration()->asDeclaration()) {
                    if (Function *func = dec->type()->asFunctionType()) {
                        if (func->isSignal() || func->returnType()->asVoidType())
                            return;
                    }
                }

                const Name *name = nameAST->name;
                const int insertPos = interface.currentFile()->startOf(outerAST);
                result << new AssignToLocalVariableOperation(interface, insertPos, outerAST, name);
                return;
            }
        }
    }
};

#ifdef WITH_TESTS
class AssignToLocalVariableTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerAssignToLocalVariableQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(AssignToLocalVariable);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <assigntolocalvariable.moc>
#endif
