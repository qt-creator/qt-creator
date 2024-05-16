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
#include "../insertionpointlocator.h"
#include "../symbolfinder.h"
#include "assigntolocalvariable.h"
#include "bringidentifierintoscope.h"
#include "completeswitchstatement.h"
#include "convertfromandtopointer.h"
#include "converttometamethodcall.h"
#include "cppcodegenerationquickfixes.h"
#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"
#include "cppquickfixhelpers.h"
#include "convertqt4connect.h"
#include "convertstringliteral.h"
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

#include <bitset>
#include <cctype>
#include <limits>

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

namespace {

class SplitSimpleDeclarationOp: public CppQuickFixOperation
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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;

        SpecifierListAST *specifiers = declaration->decl_specifier_list;
        int declSpecifiersStart = currentFile->startOf(specifiers->firstToken());
        int declSpecifiersEnd = currentFile->endOf(specifiers->lastToken() - 1);
        int insertPos = currentFile->endOf(declaration->semicolon_token);

        DeclaratorAST *prevDeclarator = declaration->declarator_list->value;

        for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
            DeclaratorAST *declarator = it->value;

            changes.insert(insertPos, QLatin1String("\n"));
            changes.copy(declSpecifiersStart, declSpecifiersEnd, insertPos);
            changes.insert(insertPos, QLatin1String(" "));
            changes.move(currentFile->range(declarator), insertPos);
            changes.insert(insertPos, QLatin1String(";"));

            const int prevDeclEnd = currentFile->endOf(prevDeclarator);
            changes.remove(prevDeclEnd, currentFile->startOf(declarator));

            prevDeclarator = declarator;
        }

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    SimpleDeclarationAST *declaration;
};

} // anonymous namespace

void SplitSimpleDeclaration::doMatch(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
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

namespace {

class ConvertNumericLiteralOp: public CppQuickFixOperation
{
public:
    ConvertNumericLiteralOp(const CppQuickFixInterface &interface, int start, int end,
                            const QString &replacement)
        : CppQuickFixOperation(interface)
        , start(start)
        , end(end)
        , replacement(replacement)
    {}

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    int start, end;
    QString replacement;
};

} // anonymous namespace

void ConvertNumericLiteral::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

    if (path.isEmpty())
        return;

    NumericLiteralAST *literal = path.last()->asNumericLiteral();

    if (!literal)
        return;

    Token token = file->tokenAt(literal->asNumericLiteral()->literal_token);
    if (!token.is(T_NUMERIC_LITERAL))
        return;
    const NumericLiteral *numeric = token.number;
    if (numeric->isDouble() || numeric->isFloat())
        return;

    // remove trailing L or U and stuff
    const char * const spell = numeric->chars();
    int numberLength = numeric->size();
    while (numberLength > 0 && !std::isxdigit(spell[numberLength - 1]))
        --numberLength;
    if (numberLength < 1)
        return;

    // convert to number
    bool valid;
    ulong value = 0;
    const QString x = QString::fromUtf8(spell).left(numberLength);
    if (x.startsWith("0b", Qt::CaseInsensitive))
        value = x.mid(2).toULong(&valid, 2);
    else
        value = x.toULong(&valid, 0);

    if (!valid)
        return;

    const int priority = path.size() - 1; // very high priority
    const int start = file->startOf(literal);
    const char * const str = numeric->chars();

    const bool isBinary = numberLength > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B');
    const bool isOctal = numberLength >= 2 && str[0] == '0' && str[1] >= '0' && str[1] <= '7';
    const bool isDecimal = !(isBinary || isOctal || numeric->isHex());

    if (!numeric->isHex()) {
        /*
          Convert integer literal to hex representation.
          Replace
            0b100000
            32
            040
          With
            0x20

        */
        const QString replacement = QString::asprintf("0x%lX", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Hexadecimal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isOctal) {
        /*
          Convert integer literal to octal representation.
          Replace
            0b100000
            32
            0x20
          With
            040
        */
        const QString replacement = QString::asprintf("0%lo", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Octal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isDecimal) {
        /*
          Convert integer literal to decimal representation.
          Replace
            0b100000
            0x20
            040
           With
            32
        */
        const QString replacement = QString::asprintf("%lu", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Decimal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isBinary) {
        /*
          Convert integer literal to binary representation.
          Replace
            32
            0x20
            040
          With
            0b100000
        */
        QString replacement = "0b";
        if (value == 0) {
            replacement.append('0');
        } else {
            std::bitset<std::numeric_limits<decltype (value)>::digits> b(value);
            QRegularExpression re("^[0]*");
            replacement.append(QString::fromStdString(b.to_string()).remove(re));
        }
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Binary"));
        op->setPriority(priority);
        result << op;
    }
}

namespace {


} // anonymous namespace

namespace {

class ConvertToCamelCaseOp: public CppQuickFixOperation
{
public:
    ConvertToCamelCaseOp(const CppQuickFixInterface &interface, const QString &name,
                         const AST *nameAst, bool test)
        : CppQuickFixOperation(interface, -1)
        , m_name(name)
        , m_nameAst(nameAst)
        , m_isAllUpper(name.isUpper())
        , m_test(test)
    {
        setDescription(Tr::tr("Convert to Camel Case"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        QString newName = m_isAllUpper ? m_name.toLower() : m_name;
        for (int i = 1; i < newName.length(); ++i) {
            const QChar c = newName.at(i);
            if (c.isUpper() && m_isAllUpper) {
                newName[i] = c.toLower();
            } else if (i < newName.length() - 1 && isConvertibleUnderscore(newName, i)) {
                newName.remove(i, 1);
                newName[i] = newName.at(i).toUpper();
            }
        }
        if (m_test) {
            ChangeSet changeSet;
            changeSet.replace(currentFile->range(m_nameAst), newName);
            currentFile->setChangeSet(changeSet);
            currentFile->apply();
        } else {
            editor()->renameUsages(newName);
        }
    }

    static bool isConvertibleUnderscore(const QString &name, int pos)
    {
        return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
                && !(pos == 1 && name.at(0) == QLatin1Char('m'));
    }

private:
    const QString m_name;
    const AST * const m_nameAst;
    const bool m_isAllUpper;
    const bool m_test;
};

} // anonymous namespace

void ConvertToCamelCase::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    const Name *name = nullptr;
    const AST *astForName = nullptr;
    if (const NameAST * const nameAst = ast->asName()) {
        if (nameAst->name && nameAst->name->asNameId()) {
            astForName = nameAst;
            name = nameAst->name;
        }
    } else if (const NamespaceAST * const namespaceAst = ast->asNamespace()) {
        astForName = namespaceAst;
        name = namespaceAst->symbol->name();
    }

    if (!name)
        return;

    QString nameString = QString::fromUtf8(name->identifier()->chars());
    if (nameString.length() < 3)
        return;
    for (int i = 1; i < nameString.length() - 1; ++i) {
        if (ConvertToCamelCaseOp::isConvertibleUnderscore(nameString, i)) {
            result << new ConvertToCamelCaseOp(interface, nameString, astForName, m_test);
            return;
        }
    }
}

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
    new ConvertToCamelCase;

    new ConvertNumericLiteral;

    new SplitSimpleDeclaration;

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

    new ExtraRefactoringOperations;
}

void destroyCppQuickFixes()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal
} // namespace CppEditor
