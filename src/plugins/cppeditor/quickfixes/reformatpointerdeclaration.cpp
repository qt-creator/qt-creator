// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "reformatpointerdeclaration.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cpppointerdeclarationformatter.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <cplusplus/Overview.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class ReformatPointerDeclarationOp: public CppQuickFixOperation
{
public:
    ReformatPointerDeclarationOp(const CppQuickFixInterface &interface, const ChangeSet change)
        : CppQuickFixOperation(interface)
        , m_change(change)
    {
        QString description;
        if (m_change.operationList().size() == 1) {
            description = Tr::tr(
                              "Reformat to \"%1\"").arg(m_change.operationList().constFirst().text());
        } else { // > 1
            description = Tr::tr("Reformat Pointers or References");
        }
        setDescription(description);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        currentFile->setChangeSet(m_change);
        currentFile->apply();
    }

private:
    ChangeSet m_change;
};

/// Filter the results of ASTPath.
/// The resulting list contains the supported AST types only once.
/// For this, the results of ASTPath are iterated in reverse order.
class ReformatPointerDeclarationASTPathResultsFilter
{
public:
    QList<AST*> filter(const QList<AST*> &astPathList)
    {
        QList<AST*> filtered;

        for (int i = astPathList.size() - 1; i >= 0; --i) {
            AST *ast = astPathList.at(i);

            if (!m_hasSimpleDeclaration && ast->asSimpleDeclaration()) {
                m_hasSimpleDeclaration = true;
                filtered.append(ast);
            } else if (!m_hasFunctionDefinition && ast->asFunctionDefinition()) {
                m_hasFunctionDefinition = true;
                filtered.append(ast);
            } else if (!m_hasParameterDeclaration && ast->asParameterDeclaration()) {
                m_hasParameterDeclaration = true;
                filtered.append(ast);
            } else if (!m_hasIfStatement && ast->asIfStatement()) {
                m_hasIfStatement = true;
                filtered.append(ast);
            } else if (!m_hasWhileStatement && ast->asWhileStatement()) {
                m_hasWhileStatement = true;
                filtered.append(ast);
            } else if (!m_hasForStatement && ast->asForStatement()) {
                m_hasForStatement = true;
                filtered.append(ast);
            } else if (!m_hasForeachStatement && ast->asForeachStatement()) {
                m_hasForeachStatement = true;
                filtered.append(ast);
            }
        }

        return filtered;
    }

private:
    bool m_hasSimpleDeclaration = false;
    bool m_hasFunctionDefinition = false;
    bool m_hasParameterDeclaration = false;
    bool m_hasIfStatement = false;
    bool m_hasWhileStatement = false;
    bool m_hasForStatement = false;
    bool m_hasForeachStatement = false;
};

/*!
  Reformats a pointer, reference or rvalue reference type/declaration.

  Works also with selections (except when the cursor is not on any AST).

  Activates on: simple declarations, parameters and return types of function
                declarations and definitions, control flow statements (if,
                while, for, foreach) with declarations.
*/
class ReformatPointerDeclaration : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        CppRefactoringFilePtr file = interface.currentFile();

        Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        overview.showArgumentNames = true;
        overview.showReturnTypes = true;

        const QTextCursor cursor = file->cursor();
        ChangeSet change;
        PointerDeclarationFormatter formatter(file, overview,
                                              PointerDeclarationFormatter::RespectCursor);

        if (cursor.hasSelection()) {
            // This will no work always as expected since this function is only called if
            // interface-path() is not empty. If the user selects the whole document via
            // ctrl-a and there is an empty line in the end, then the cursor is not on
            // any AST and therefore no quick fix will be triggered.
            change = formatter.format(file->cppDocument()->translationUnit()->ast());
            if (!change.isEmpty())
                result << new ReformatPointerDeclarationOp(interface, change);
        } else {
            const QList<AST *> suitableASTs
                = ReformatPointerDeclarationASTPathResultsFilter().filter(path);
            for (AST *ast : suitableASTs) {
                change = formatter.format(ast);
                if (!change.isEmpty()) {
                    result << new ReformatPointerDeclarationOp(interface, change);
                    return;
                }
            }
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class ReformatPointerDeclarationTest : public QObject
{
    Q_OBJECT

private slots:
    void test()
    {
        // Check: Just a basic test since the main functionality is tested in
        // cpppointerdeclarationformatter_test.cpp
        ReformatPointerDeclaration factory;
        QuickFixOperationTest(
            singleDocument(QByteArray("char@*s;"), QByteArray("char *s;")), &factory);
    }
};

QObject *ReformatPointerDeclaration::createTest() { return new ReformatPointerDeclarationTest; }

#endif // WITH_TESTS
}

void registerReformatPointerDeclarationQuickfix()
{
    CppQuickFixFactory::registerFactory<ReformatPointerDeclaration>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <reformatpointerdeclaration.moc>
#endif
