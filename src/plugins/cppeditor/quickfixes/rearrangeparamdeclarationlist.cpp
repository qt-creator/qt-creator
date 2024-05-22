// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rearrangeparamdeclarationlist.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#ifdef WITH_TESTS
#include <QObject>
#endif

using namespace CPlusPlus;

namespace CppEditor::Internal {
namespace {

class RearrangeParamDeclarationListOp: public CppQuickFixOperation
{
public:
    enum Target { TargetPrevious, TargetNext };

    RearrangeParamDeclarationListOp(const CppQuickFixInterface &interface, AST *currentParam,
                                    AST *targetParam, Target target)
        : CppQuickFixOperation(interface)
        , m_currentParam(currentParam)
        , m_targetParam(targetParam)
    {
        QString targetString;
        if (target == TargetPrevious)
            targetString = Tr::tr("Switch with Previous Parameter");
        else
            targetString = Tr::tr("Switch with Next Parameter");
        setDescription(targetString);
    }

    void perform() override
    {
        int targetEndPos = currentFile()->endOf(m_targetParam);
        currentFile()->setOpenEditor(false, targetEndPos);
        currentFile()->apply(Utils::ChangeSet::makeFlip(
            currentFile()->startOf(m_currentParam),
            currentFile()->endOf(m_currentParam),
            currentFile()->startOf(m_targetParam),
            targetEndPos));
    }

private:
    AST *m_currentParam;
    AST *m_targetParam;
};


/*!
  Switches places of the parameter declaration under cursor
  with the next or the previous one in the parameter declaration list

  Activates on: parameter declarations
*/
class RearrangeParamDeclarationList : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> path = interface.path();

        ParameterDeclarationAST *paramDecl = nullptr;
        int index = path.size() - 1;
        for (; index != -1; --index) {
            paramDecl = path.at(index)->asParameterDeclaration();
            if (paramDecl)
                break;
        }

        if (index < 1)
            return;

        ParameterDeclarationClauseAST *paramDeclClause
            = path.at(index - 1)->asParameterDeclarationClause();
        QTC_ASSERT(paramDeclClause && paramDeclClause->parameter_declaration_list, return);

        ParameterDeclarationListAST *paramListNode = paramDeclClause->parameter_declaration_list;
        ParameterDeclarationListAST *prevParamListNode = nullptr;
        while (paramListNode) {
            if (paramDecl == paramListNode->value)
                break;
            prevParamListNode = paramListNode;
            paramListNode = paramListNode->next;
        }

        if (!paramListNode)
            return;

        if (prevParamListNode)
            result << new RearrangeParamDeclarationListOp(
                interface,
                paramListNode->value,
                prevParamListNode->value,
                RearrangeParamDeclarationListOp::TargetPrevious);
        if (paramListNode->next)
            result << new RearrangeParamDeclarationListOp(
                interface,
                paramListNode->value,
                paramListNode->next->value,
                RearrangeParamDeclarationListOp::TargetNext);
    }
};

} // namespace

void registerRearrangeParamDeclarationListQuickfix()
{
    CppQuickFixFactory::registerFactory<RearrangeParamDeclarationList>();
}

} // namespace CppEditor::Internal
