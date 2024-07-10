// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "splitsimpledeclaration.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

static bool checkDeclarationForSplit(SimpleDeclarationAST *declaration)
{
    if (!declaration->semicolon_token)
        return false;

    if (!declaration->decl_specifier_list)
        return false;

    for (SpecifierListAST *it = declaration->decl_specifier_list; it; it = it->next) {
        SpecifierAST *specifier = it->value;
        if (specifier->asEnumSpecifier() || specifier->asClassSpecifier())
            return false;
    }

    return declaration->declarator_list && declaration->declarator_list->next;
}

class SplitSimpleDeclarationOp : public CppQuickFixOperation
{
public:
    SplitSimpleDeclarationOp(const CppQuickFixInterface &interface, int priority,
                             SimpleDeclarationAST *decl)
        : CppQuickFixOperation(interface, priority)
        , declaration(decl)
    {
        setDescription(Tr::tr("Split Declaration"));
    }

    void perform() override
    {
        ChangeSet changes;

        SpecifierListAST *specifiers = declaration->decl_specifier_list;
        int declSpecifiersStart = currentFile()->startOf(specifiers->firstToken());
        int declSpecifiersEnd = currentFile()->endOf(specifiers->lastToken() - 1);
        int insertPos = currentFile()->endOf(declaration->semicolon_token);

        DeclaratorAST *prevDeclarator = declaration->declarator_list->value;

        for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
            DeclaratorAST *declarator = it->value;

            changes.insert(insertPos, QLatin1String("\n"));
            changes.copy(declSpecifiersStart, declSpecifiersEnd, insertPos);
            changes.insert(insertPos, QLatin1String(" "));
            changes.move(currentFile()->range(declarator), insertPos);
            changes.insert(insertPos, QLatin1String(";"));

            const int prevDeclEnd = currentFile()->endOf(prevDeclarator);
            changes.remove(prevDeclEnd, currentFile()->startOf(declarator));

            prevDeclarator = declarator;
        }

        currentFile()->apply(changes);
    }

private:
    SimpleDeclarationAST *declaration;
};

/*!
  Rewrite
    int *a, b;

  As
    int *a;
    int b;

  Activates on: the type or the variable names.
*/
class SplitSimpleDeclaration : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        CoreDeclaratorAST *core_declarator = nullptr;
        const QList<AST *> &path = interface.path();
        CppRefactoringFilePtr file = interface.currentFile();
        const int cursorPosition = file->cursor().selectionStart();

        for (int index = path.size() - 1; index != -1; --index) {
            AST *node = path.at(index);

            if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator()) {
                core_declarator = coreDecl;
            } else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                if (checkDeclarationForSplit(simpleDecl)) {
                    SimpleDeclarationAST *declaration = simpleDecl;

                    const int startOfDeclSpecifier = file->startOf(declaration->decl_specifier_list->firstToken());
                    const int endOfDeclSpecifier = file->endOf(declaration->decl_specifier_list->lastToken() - 1);

                    if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier) {
                        // the AST node under cursor is a specifier.
                        result << new SplitSimpleDeclarationOp(interface, index, declaration);
                        return;
                    }

                    if (core_declarator && interface.isCursorOn(core_declarator)) {
                        // got a core-declarator under the text cursor.
                        result << new SplitSimpleDeclarationOp(interface, index, declaration);
                        return;
                    }
                }

                return;
            }
        }
    }
};

} // namespace

void registerSplitSimpleDeclarationQuickfix()
{
    CppQuickFixFactory::registerFactory<SplitSimpleDeclaration>();
}

} // namespace CppEditor::Internal
