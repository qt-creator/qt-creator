// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixes.h"

#include "../baseeditordocumentprocessor.h"
#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cppfunctiondecldeflink.h"
#include "../cpppointerdeclarationformatter.h"
#include "../cpprefactoringchanges.h"
#include "../cpptoolsreuse.h"
#include "assigntolocalvariable.h"
#include "bringidentifierintoscope.h"
#include "completeswitchstatement.h"
#include "convertfromandtopointer.h"
#include "convertnumericliteral.h"
#include "convertqt4connect.h"
#include "convertstringliteral.h"
#include "converttocamelcase.h"
#include "converttometamethodcall.h"
#include "cppcodegenerationquickfixes.h"
#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"
#include "cppquickfixhelpers.h"
#include "createdeclarationfromuse.h"
#include "extractfunction.h"
#include "extractliteralasparameter.h"
#include "insertfunctiondefinition.h"
#include "logicaloperationquickfixes.h"
#include "moveclasstoownfile.h"
#include "movefunctiondefinition.h"
#include "removeusingnamespace.h"
#include "rewritecomment.h"
#include "rewritecontrolstatements.h"
#include "splitsimpledeclaration.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/declarationcomments.h>
#include <cplusplus/NamePrettyPrinter.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/TypePrettyPrinter.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/tabsettings.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/codegeneration.h>
#include <utils/layoutbuilder.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QMimeData>
#include <QPair>
#include <QProxyStyle>
#include <QPushButton>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QStack>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTextCodec>
#include <QTextCursor>
#include <QVBoxLayout>

#include <cctype>

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

static QList<CppQuickFixFactory *> g_cppQuickFixFactories;

CppQuickFixFactory::CppQuickFixFactory()
{
    g_cppQuickFixFactories.append(this);
}

CppQuickFixFactory::~CppQuickFixFactory()
{
    g_cppQuickFixFactories.removeOne(this);
}

void CppQuickFixFactory::match(const Internal::CppQuickFixInterface &interface,
                               QuickFixOperations &result)
{
    if (m_clangdReplacement) {
        if (const auto clangdVersion = CppModelManager::usesClangd(
                    interface.currentFile()->editor()->textDocument());
                clangdVersion && clangdVersion >= m_clangdReplacement) {
            return;
        }
    }

    doMatch(interface, result);
}

const QList<CppQuickFixFactory *> &CppQuickFixFactory::cppQuickFixFactories()
{
    return g_cppQuickFixFactories;
}

namespace Internal {

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        int targetEndPos = currentFile->endOf(m_targetParam);
        ChangeSet changes;
        changes.flip(currentFile->startOf(m_currentParam), currentFile->endOf(m_currentParam),
                     currentFile->startOf(m_targetParam), targetEndPos);
        currentFile->setChangeSet(changes);
        currentFile->setOpenEditor(false, targetEndPos);
        currentFile->apply();
    }

private:
    AST *m_currentParam;
    AST *m_targetParam;
};

} // anonymous namespace

void RearrangeParamDeclarationList::doMatch(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
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

    ParameterDeclarationClauseAST *paramDeclClause = path.at(index-1)->asParameterDeclarationClause();
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
        result << new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                                                      prevParamListNode->value, RearrangeParamDeclarationListOp::TargetPrevious);
    if (paramListNode->next)
        result << new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                                                      paramListNode->next->value, RearrangeParamDeclarationListOp::TargetNext);
}

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

} // anonymous namespace

void ReformatPointerDeclaration::doMatch(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
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

namespace {

class ApplyDeclDefLinkOperation : public CppQuickFixOperation
{
public:
    explicit ApplyDeclDefLinkOperation(const CppQuickFixInterface &interface,
            const std::shared_ptr<FunctionDeclDefLink> &link)
        : CppQuickFixOperation(interface, 100)
        , m_link(link)
    {}

    void perform() override
    {
        if (editor()->declDefLink() == m_link)
            editor()->applyDeclDefLinkChanges(/*don't jump*/false);
    }

protected:
    virtual void performChanges(const CppRefactoringFilePtr &, const CppRefactoringChanges &)
    { /* never called since perform is overridden */ }

private:
    std::shared_ptr<FunctionDeclDefLink> m_link;
};

} // anonymous namespace

void ApplyDeclDefLinkChanges::doMatch(const CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    std::shared_ptr<FunctionDeclDefLink> link = interface.editor()->declDefLink();
    if (!link || !link->isMarkerVisible())
        return;

    auto op = new ApplyDeclDefLinkOperation(interface, link);
    op->setDescription(Tr::tr("Apply Function Signature Changes"));
    result << op;
}

void ExtraRefactoringOperations::doMatch(const CppQuickFixInterface &interface,
                                         QuickFixOperations &result)
{
    const auto processor = CppModelManager::cppEditorDocumentProcessor(interface.filePath());
    if (processor) {
        const auto clangFixItOperations = processor->extraRefactoringOperations(interface);
        result.append(clangFixItOperations);
    }
}

void createCppQuickFixes()
{
    new RearrangeParamDeclarationList;
    new ReformatPointerDeclaration;

    new ApplyDeclDefLinkChanges;

    registerInsertVirtualMethodsQuickfix();
    registerMoveClassToOwnFileQuickfix();
    registerRemoveUsingNamespaceQuickfix();
    registerCodeGenerationQuickfixes();
    registerConvertQt4ConnectQuickfix();
    registerMoveFunctionDefinitionQuickfixes();
    registerInsertFunctionDefinitionQuickfixes();
    registerBringIdentifierIntoScopeQuickfixes();
    registerConvertStringLiteralQuickfixes();
    registerCreateDeclarationFromUseQuickfixes();
    registerLogicalOperationQuickfixes();
    registerRewriteControlStatementQuickfixes();
    registerRewriteCommentQuickfixes();
    registerExtractFunctionQuickfix();
    registerExtractLiteralAsParameterQuickfix();
    registerConvertFromAndToPointerQuickfix();
    registerAssignToLocalVariableQuickfix();
    registerCompleteSwitchStatementQuickfix();
    registerConvertToMetaMethodCallQuickfix();
    registerSplitSimpleDeclarationQuickfix();
    registerConvertNumericLiteralQuickfix();
    registerConvertToCamelCaseQuickfix();

    new ExtraRefactoringOperations;
}

void destroyCppQuickFixes()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal
} // namespace CppEditor
