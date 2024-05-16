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
#include "bringidentifierintoscope.h"
#include "cppcodegenerationquickfixes.h"
#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"
#include "cppquickfixhelpers.h"
#include "cppquickfixprojectsettings.h"
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
#include <functional>
#include <limits>
#include <vector>

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->setChangeSet(changes);
        currentFile->apply();
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

} // anonymous namespace

void CompleteSwitchCaseStatement::doMatch(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
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

namespace {

class ConvertFromAndToPointerOp : public CppQuickFixOperation
{
public:
    enum Mode { FromPointer, FromVariable, FromReference };

    ConvertFromAndToPointerOp(const CppQuickFixInterface &interface, int priority, Mode mode,
                              bool isAutoDeclaration,
                              const SimpleDeclarationAST *simpleDeclaration,
                              const DeclaratorAST *declaratorAST,
                              const SimpleNameAST *identifierAST,
                              Symbol *symbol)
        : CppQuickFixOperation(interface, priority)
        , m_mode(mode)
        , m_isAutoDeclaration(isAutoDeclaration)
        , m_simpleDeclaration(simpleDeclaration)
        , m_declaratorAST(declaratorAST)
        , m_identifierAST(identifierAST)
        , m_symbol(symbol)
        , m_refactoring(snapshot())
        , m_file(m_refactoring.cppFile(filePath()))
        , m_document(interface.semanticInfo().doc)
    {
        setDescription(
                mode == FromPointer
                ? Tr::tr("Convert to Stack Variable")
                : Tr::tr("Convert to Pointer"));
    }

    void perform() override
    {
        ChangeSet changes;

        switch (m_mode) {
        case FromPointer:
            removePointerOperator(changes);
            convertToStackVariable(changes);
            break;
        case FromReference:
            removeReferenceOperator(changes);
            Q_FALLTHROUGH();
        case FromVariable:
            convertToPointer(changes);
            break;
        }

        m_file->setChangeSet(changes);
        m_file->apply();
    }

private:
    void removePointerOperator(ChangeSet &changes) const
    {
        if (!m_declaratorAST->ptr_operator_list)
            return;
        PointerAST *ptrAST = m_declaratorAST->ptr_operator_list->value->asPointer();
        QTC_ASSERT(ptrAST, return);
        const int pos = m_file->startOf(ptrAST->star_token);
        changes.remove(pos, pos + 1);
    }

    void removeReferenceOperator(ChangeSet &changes) const
    {
        ReferenceAST *refAST = m_declaratorAST->ptr_operator_list->value->asReference();
        QTC_ASSERT(refAST, return);
        const int pos = m_file->startOf(refAST->reference_token);
        changes.remove(pos, pos + 1);
    }

    void removeNewExpression(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        ExpressionListAST *exprlist = nullptr;
        if (newExprAST->new_initializer) {
            if (ExpressionListParenAST *ast = newExprAST->new_initializer->asExpressionListParen())
                exprlist = ast->expression_list;
            else if (BracedInitializerAST *ast = newExprAST->new_initializer->asBracedInitializer())
                exprlist = ast->expression_list;
        }

        if (exprlist) {
            // remove 'new' keyword and type before initializer
            changes.remove(m_file->startOf(newExprAST->new_token),
                           m_file->startOf(newExprAST->new_initializer));

            changes.remove(m_file->endOf(m_declaratorAST->equal_token - 1),
                           m_file->startOf(m_declaratorAST->equal_token + 1));
        } else {
            // remove the whole new expression
            changes.remove(m_file->endOf(m_identifierAST->firstToken()),
                           m_file->startOf(newExprAST->lastToken()));
        }
    }

    void removeNewKeyword(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        // remove 'new' keyword before initializer
        changes.remove(m_file->startOf(newExprAST->new_token),
                       m_file->startOf(newExprAST->new_type_id));
    }

    void convertToStackVariable(ChangeSet &changes) const
    {
        // Handle the initializer.
        if (m_declaratorAST->initializer) {
            if (NewExpressionAST *newExpression = m_declaratorAST->initializer->asNewExpression()) {
                if (m_isAutoDeclaration) {
                    if (!newExpression->new_initializer)
                        changes.insert(m_file->endOf(newExpression), QStringLiteral("()"));
                    removeNewKeyword(changes, newExpression);
                } else {
                    removeNewExpression(changes, newExpression);
                }
            }
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool declarationFound = false;
            bool starFound = false;
            int ampersandPos = 0;
            bool memberAccess = false;
            bool deleteCall = false;

            for (int i = path.count() - 2; i >= 0; --i) {
                if (path.at(i) == m_declaratorAST) {
                    declarationFound = true;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    if (m_file->tokenAt(memberAccessAST->access_token).kind() != T_ARROW)
                        continue;
                    int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 2, QLatin1String("."));
                    memberAccess = true;
                    break;
                } else if (DeleteExpressionAST *deleteAST = path.at(i)->asDeleteExpression()) {
                    const int pos = m_file->startOf(deleteAST->delete_token);
                    changes.insert(pos, QLatin1String("// "));
                    deleteCall = true;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    const Token tk = m_file->tokenAt(unaryExprAST->unary_op_token);
                    if (tk.kind() == T_STAR) {
                        if (!starFound) {
                            int pos = m_file->startOf(unaryExprAST->unary_op_token);
                            changes.remove(pos, pos + 1);
                        }
                        starFound = true;
                    } else if (tk.kind() == T_AMPER) {
                        ampersandPos = m_file->startOf(unaryExprAST->unary_op_token);
                    }
                } else if (PointerAST *ptrAST = path.at(i)->asPointer()) {
                    if (!starFound) {
                        const int pos = m_file->startOf(ptrAST->star_token);
                        changes.remove(pos, pos);
                    }
                    starFound = true;
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (!declarationFound && !starFound && !memberAccess && !deleteCall) {
                if (ampersandPos) {
                    changes.insert(ampersandPos, QLatin1String("&("));
                    changes.insert(m_file->endOf(idAST->firstToken()), QLatin1String(")"));
                } else {
                    changes.insert(m_file->startOf(idAST), QLatin1String("&"));
                }
            }
        }
    }

    QString typeNameOfDeclaration() const
    {
        if (!m_simpleDeclaration
                || !m_simpleDeclaration->decl_specifier_list
                || !m_simpleDeclaration->decl_specifier_list->value) {
            return QString();
        }
        NamedTypeSpecifierAST *namedType
                = m_simpleDeclaration->decl_specifier_list->value->asNamedTypeSpecifier();
        if (!namedType)
            return QString();

        Overview overview;
        return overview.prettyName(namedType->name->name);
    }

    void insertNewExpression(ChangeSet &changes, ExpressionAST *ast) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (CallAST *callAST = ast->asCall()) {
            if (typeName.isEmpty()) {
                changes.insert(m_file->startOf(callAST), QLatin1String("new "));
            } else {
                changes.insert(m_file->startOf(callAST),
                               QLatin1String("new ") + typeName + QLatin1Char('('));
                changes.insert(m_file->startOf(callAST->lastToken()), QLatin1String(")"));
            }
        } else {
            if (typeName.isEmpty())
                return;
            changes.insert(m_file->startOf(ast), QLatin1String(" = new ") + typeName);
        }
    }

    void insertNewExpression(ChangeSet &changes) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty())
            return;
        changes.insert(m_file->endOf(m_identifierAST->firstToken()),
                       QLatin1String(" = new ") + typeName);
    }

    void convertToPointer(ChangeSet &changes) const
    {
        // Handle initializer.
        if (m_declaratorAST->initializer) {
            if (IdExpressionAST *idExprAST = m_declaratorAST->initializer->asIdExpression()) {
                changes.insert(m_file->startOf(idExprAST), QLatin1String("&"));
            } else if (CallAST *callAST = m_declaratorAST->initializer->asCall()) {
                insertNewExpression(changes, callAST);
            } else if (ExpressionListParenAST *exprListAST = m_declaratorAST->initializer
                                                                 ->asExpressionListParen()) {
                insertNewExpression(changes, exprListAST);
            } else if (BracedInitializerAST *bracedInitializerAST = m_declaratorAST->initializer
                                                                        ->asBracedInitializer()) {
                insertNewExpression(changes, bracedInitializerAST);
            }
        } else {
            insertNewExpression(changes);
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool insertStar = true;
            for (int i = path.count() - 2; i >= 0; --i) {
                if (m_isAutoDeclaration && path.at(i) == m_declaratorAST) {
                    insertStar = false;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    const int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 1, QLatin1String("->"));
                    insertStar = false;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    if (m_file->tokenAt(unaryExprAST->unary_op_token).kind() == T_AMPER) {
                        const int pos = m_file->startOf(unaryExprAST->unary_op_token);
                        changes.remove(pos, pos + 1);
                        insertStar = false;
                        break;
                    }
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (insertStar)
                changes.insert(m_file->startOf(idAST), QLatin1String("*"));
        }
    }

    const Mode m_mode;
    const bool m_isAutoDeclaration;
    const SimpleDeclarationAST * const m_simpleDeclaration;
    const DeclaratorAST * const m_declaratorAST;
    const SimpleNameAST * const m_identifierAST;
    Symbol * const m_symbol;
    const CppRefactoringChanges m_refactoring;
    const CppRefactoringFilePtr m_file;
    const Document::Ptr m_document;
};

} // anonymous namespace

void ConvertFromAndToPointer::doMatch(const CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    if (path.count() < 2)
        return;
    SimpleNameAST *identifier = path.last()->asSimpleName();
    if (!identifier)
        return;
    SimpleDeclarationAST *simpleDeclaration = nullptr;
    DeclaratorAST *declarator = nullptr;
    bool isFunctionLocal = false;
    bool isClassLocal = false;
    ConvertFromAndToPointerOp::Mode mode = ConvertFromAndToPointerOp::FromVariable;
    for (int i = path.count() - 2; i >= 0; --i) {
        AST *ast = path.at(i);
        if (!declarator && (declarator = ast->asDeclarator()))
            continue;
        if (!simpleDeclaration && (simpleDeclaration = ast->asSimpleDeclaration()))
            continue;
        if (declarator && simpleDeclaration) {
            if (ast->asClassSpecifier()) {
                isClassLocal = true;
            } else if (ast->asFunctionDefinition() && !isClassLocal) {
                isFunctionLocal = true;
                break;
            }
        }
    }
    if (!isFunctionLocal || !simpleDeclaration || !declarator)
        return;

    Symbol *symbol = nullptr;
    for (List<Symbol *> *lst = simpleDeclaration->symbols; lst; lst = lst->next) {
        if (lst->value->name() == identifier->name) {
            symbol = lst->value;
            break;
        }
    }
    if (!symbol)
        return;

    bool isAutoDeclaration = false;
    if (symbol->storage() == Symbol::Auto) {
        // For auto variables we must deduce the type from the initializer.
        if (!declarator->initializer)
            return;

        isAutoDeclaration = true;
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
        typeOfExpression.setExpandTemplates(true);
        CppRefactoringFilePtr file = interface.currentFile();
        Scope *scope = file->scopeAt(declarator->firstToken());
        QList<LookupItem> result = typeOfExpression(file->textOf(declarator->initializer).toUtf8(),
                                                    scope, TypeOfExpression::Preprocess);
        if (!result.isEmpty() && result.first().type()->asPointerType())
            mode = ConvertFromAndToPointerOp::FromPointer;
    } else if (declarator->ptr_operator_list) {
        for (PtrOperatorListAST *ops = declarator->ptr_operator_list; ops; ops = ops->next) {
            if (ops != declarator->ptr_operator_list) {
                // Bail out on more complex pointer types (e.g. pointer of pointer,
                // or reference of pointer).
                return;
            }
            if (ops->value->asPointer())
                mode = ConvertFromAndToPointerOp::FromPointer;
            else if (ops->value->asReference())
                mode = ConvertFromAndToPointerOp::FromReference;
        }
    }

    const int priority = path.size() - 1;
    result << new ConvertFromAndToPointerOp(interface, priority, mode, isAutoDeclaration,
                                            simpleDeclaration, declarator, identifier, symbol);
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
        , m_file(CppRefactoringChanges(snapshot()).cppFile(filePath()))
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
        ChangeSet changes;
        changes.insert(m_insertPos, insertString);
        m_file->setChangeSet(changes);
        m_file->apply();

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

} // anonymous namespace

void AssignToLocalVariable::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

void ExtraRefactoringOperations::doMatch(const CppQuickFixInterface &interface,
                                         QuickFixOperations &result)
{
    const auto processor = CppModelManager::cppEditorDocumentProcessor(interface.filePath());
    if (processor) {
        const auto clangFixItOperations = processor->extraRefactoringOperations(interface);
        result.append(clangFixItOperations);
    }
}

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
} // namespace

void ConvertToMetaMethodCall::doMatch(const CppQuickFixInterface &interface,
                                      TextEditor::QuickFixOperations &result)
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

void createCppQuickFixes()
{
    new ConvertToCamelCase;

    new ConvertNumericLiteral;

    new SplitSimpleDeclaration;

    new RearrangeParamDeclarationList;
    new ReformatPointerDeclaration;

    new CompleteSwitchCaseStatement;

    new ApplyDeclDefLinkChanges;
    new ConvertFromAndToPointer;

    new AssignToLocalVariable;

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

    new ExtraRefactoringOperations;

    new ConvertToMetaMethodCall;
}

void destroyCppQuickFixes()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal
} // namespace CppEditor
