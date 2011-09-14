/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cppfunctiondecldeflink.h"

#include "cppeditor.h"
#include "cppquickfixassistant.h"

#include <cplusplus/CppRewriter.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/AST.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cpptools/cpprefactoringchanges.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/tooltip/tooltip.h>
#include <texteditor/tooltip/tipcontents.h>
#include <utils/qtcassert.h>
#include <utils/proxyaction.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <texteditor/texteditorconstants.h>

#include <QtCore/QtConcurrentRun>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;

FunctionDeclDefLinkFinder::FunctionDeclDefLinkFinder(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, SIGNAL(finished()),
            this, SLOT(onFutureDone()));
}

void FunctionDeclDefLinkFinder::onFutureDone()
{
    QSharedPointer<FunctionDeclDefLink> link = m_watcher.result();
    if (link) {
        link->linkSelection = m_scannedSelection;
        link->nameSelection = m_nameSelection;
        if (m_nameSelection.selectedText() != link->nameInitial)
            link.clear();
    }
    m_scannedSelection = QTextCursor();
    m_nameSelection = QTextCursor();
    if (link)
        emit foundLink(link);
}

QTextCursor FunctionDeclDefLinkFinder::scannedSelection() const
{
    return m_scannedSelection;
}

// parent is either a FunctionDefinitionAST or a SimpleDeclarationAST
// line and column are 1-based
static bool findDeclOrDef(const Document::Ptr &doc, int line, int column,
                          DeclarationAST **parent, DeclaratorAST **decl,
                          FunctionDeclaratorAST **funcDecl)
{
    QList<AST *> path = ASTPath(doc)(line, column);

    // for function definitions, simply scan for FunctionDefinitionAST not preceded
    //    by CompoundStatement/CtorInitializer
    // for function declarations, look for SimpleDeclarations with a single Declarator
    //    with a FunctionDeclarator postfix
    FunctionDefinitionAST *funcDef = 0;
    SimpleDeclarationAST *simpleDecl = 0;
    *decl = 0;
    for (int i = path.size() - 1; i > 0; --i) {
        AST *ast = path.at(i);
        if (ast->asCompoundStatement() || ast->asCtorInitializer())
            break;
        if ((funcDef = ast->asFunctionDefinition()) != 0) {
            *parent = funcDef;
            *decl = funcDef->declarator;
            break;
        }
        if ((simpleDecl = ast->asSimpleDeclaration()) != 0) {
            *parent = simpleDecl;
            if (!simpleDecl->declarator_list || !simpleDecl->declarator_list->value)
                break;
            *decl = simpleDecl->declarator_list->value;
            break;
        }
    }
    if (!*parent || !*decl)
        return false;
    if (!(*decl)->postfix_declarator_list || !(*decl)->postfix_declarator_list->value)
        return false;
    *funcDecl = (*decl)->postfix_declarator_list->value->asFunctionDeclarator();
    return *funcDecl;
}

static void declDefLinkStartEnd(const CppTools::CppRefactoringFileConstPtr &file,
                                DeclarationAST *parent, FunctionDeclaratorAST *funcDecl,
                                int *start, int *end)
{
    *start = file->startOf(parent);
    if (funcDecl->trailing_return_type)
        *end = file->endOf(funcDecl->trailing_return_type);
    else if (funcDecl->exception_specification)
        *end = file->endOf(funcDecl->exception_specification);
    else if (funcDecl->cv_qualifier_list)
        *end = file->endOf(funcDecl->cv_qualifier_list->lastValue());
    else
        *end = file->endOf(funcDecl->rparen_token);
}

static DeclaratorIdAST *getDeclaratorId(DeclaratorAST *declarator)
{
    if (!declarator || !declarator->core_declarator)
        return 0;
    if (DeclaratorIdAST *id = declarator->core_declarator->asDeclaratorId()) {
        return id;
    }
    if (NestedDeclaratorAST *nested = declarator->core_declarator->asNestedDeclarator()) {
        return getDeclaratorId(nested->declarator);
    }
    return 0;
}

static QSharedPointer<FunctionDeclDefLink> findLinkHelper(QSharedPointer<FunctionDeclDefLink> link, CppTools::CppRefactoringChanges changes)
{
    QSharedPointer<FunctionDeclDefLink> noResult;
    const Snapshot &snapshot = changes.snapshot();

    // find the matching decl/def symbol
    Symbol *target = 0;
    if (FunctionDefinitionAST *funcDef = link->sourceDeclaration->asFunctionDefinition()) {
        QList<Declaration *> nameMatch, argumentCountMatch, typeMatch;
        findMatchingDeclaration(LookupContext(link->sourceDocument, snapshot),
                                funcDef->symbol,
                                &typeMatch, &argumentCountMatch, &nameMatch);
        if (!typeMatch.isEmpty())
            target = typeMatch.first();
    } else if (link->sourceDeclaration->asSimpleDeclaration()) {
        target = snapshot.findMatchingDefinition(link->sourceFunctionDeclarator->symbol, true);
    }
    if (!target) {
        return noResult;
    }

    // parse the target file to get the linked decl/def
    const QString targetFileName = QString::fromUtf8(
                target->fileName(), target->fileNameLength());
    CppTools::CppRefactoringFileConstPtr targetFile = changes.fileNoEditor(targetFileName);
    if (!targetFile->isValid())
        return noResult;

    DeclarationAST *targetParent = 0;
    FunctionDeclaratorAST *targetFuncDecl = 0;
    DeclaratorAST *targetDeclarator = 0;
    if (!findDeclOrDef(targetFile->cppDocument(), target->line(), target->column(),
                       &targetParent, &targetDeclarator, &targetFuncDecl))
        return noResult;

    // the parens are necessary for finding good places for changes
    if (!targetFuncDecl->lparen_token || !targetFuncDecl->rparen_token)
        return noResult;

    int targetStart, targetEnd;
    declDefLinkStartEnd(targetFile, targetParent, targetFuncDecl, &targetStart, &targetEnd);
    QString targetInitial = targetFile->textOf(
                targetFile->startOf(targetParent),
                targetEnd);

    targetFile->lineAndColumn(targetStart, &link->targetLine, &link->targetColumn);
    link->targetInitial = targetInitial;

    link->targetFile = targetFile;
    link->targetFunction = targetFuncDecl->symbol;
    link->targetFunctionDeclarator = targetFuncDecl;
    link->targetDeclaration = targetParent;

    return link;
}

void FunctionDeclDefLinkFinder::startFindLinkAt(
        QTextCursor cursor, const Document::Ptr &doc, const Snapshot &snapshot)
{
    // check if cursor is on function decl/def
    DeclarationAST *parent = 0;
    FunctionDeclaratorAST *funcDecl = 0;
    DeclaratorAST *declarator = 0;
    if (!findDeclOrDef(doc, cursor.blockNumber() + 1, cursor.columnNumber() + 1,
                       &parent, &declarator, &funcDecl))
        return;

    // find the start/end offsets
    CppTools::CppRefactoringChanges refactoringChanges(snapshot);
    CppTools::CppRefactoringFilePtr sourceFile = refactoringChanges.file(doc->fileName());
    sourceFile->setCppDocument(doc);
    int start, end;
    declDefLinkStartEnd(sourceFile, parent, funcDecl, &start, &end);

    // if already scanning, don't scan again
    if (!m_scannedSelection.isNull()
            && m_scannedSelection.selectionStart() == start
            && m_scannedSelection.selectionEnd() == end) {
        return;
    }

    // build the selection for the currently scanned area
    m_scannedSelection = cursor;
    m_scannedSelection.setPosition(end);
    m_scannedSelection.setPosition(start, QTextCursor::KeepAnchor);
    m_scannedSelection.setKeepPositionOnInsert(true);

    // build selection for the name
    DeclaratorIdAST *declId = getDeclaratorId(declarator);
    m_nameSelection = cursor;
    m_nameSelection.setPosition(sourceFile->endOf(declId));
    m_nameSelection.setPosition(sourceFile->startOf(declId), QTextCursor::KeepAnchor);
    m_nameSelection.setKeepPositionOnInsert(true);

    // set up a base result
    QSharedPointer<FunctionDeclDefLink> result(new FunctionDeclDefLink);
    result->nameInitial = m_nameSelection.selectedText();
    result->sourceDocument = doc;
    result->sourceFunction = funcDecl->symbol;
    result->sourceDeclaration = parent;
    result->sourceFunctionDeclarator = funcDecl;

    // handle the rest in a thread
    m_watcher.setFuture(QtConcurrent::run(&findLinkHelper, result, refactoringChanges));
}

FunctionDeclDefLink::FunctionDeclDefLink()
{
    hasMarker = false;
    targetLine = 0;
    targetColumn = 0;
    sourceFunction = 0;
    targetFunction = 0;
    targetDeclaration = 0;
    targetFunctionDeclarator = 0;
}

FunctionDeclDefLink::~FunctionDeclDefLink()
{
}

bool FunctionDeclDefLink::isValid() const
{
    return !linkSelection.isNull();
}

bool FunctionDeclDefLink::isMarkerVisible() const
{
    return hasMarker;
}

static bool namesEqual(const Name *n1, const Name *n2)
{
    return n1 == n2 || (n1 && n2 && n1->isEqualTo(n2));
}

void FunctionDeclDefLink::apply(CPPEditorWidget *editor, bool jumpToMatch)
{
    Snapshot snapshot = editor->semanticInfo().snapshot;

    // first verify the interesting region of the target file is unchanged
    CppTools::CppRefactoringChanges refactoringChanges(snapshot);
    CppTools::CppRefactoringFilePtr newTargetFile = refactoringChanges.file(targetFile->fileName());
    if (!newTargetFile->isValid())
        return;
    const int targetStart = newTargetFile->position(targetLine, targetColumn);
    const int targetEnd = targetStart + targetInitial.size();
    if (targetInitial == newTargetFile->textOf(targetStart, targetEnd)) {
        const Utils::ChangeSet changeset = changes(snapshot, targetStart);
        newTargetFile->setChangeSet(changeset);
        if (jumpToMatch) {
            const int jumpTarget = newTargetFile->position(targetFunction->line(), targetFunction->column());
            newTargetFile->setOpenEditor(true, jumpTarget);
        }
        newTargetFile->apply();
    } else {
        TextEditor::ToolTip::instance()->show(
                    editor->toolTipPosition(linkSelection),
                    TextEditor::TextContent(
                        tr("Target file was changed, could not apply changes")));
    }
}

template <class T>
static QList<TextEditor::RefactorMarker> removeMarkersOfType(const QList<TextEditor::RefactorMarker> &markers)
{
    QList<TextEditor::RefactorMarker> result;
    foreach (const TextEditor::RefactorMarker &marker, markers) {
        if (!marker.data.canConvert<T>())
            result += marker;
    }
    return result;
}

void FunctionDeclDefLink::hideMarker(CPPEditorWidget *editor)
{
    if (!hasMarker)
        return;
    editor->setRefactorMarkers(
                removeMarkersOfType<Marker>(editor->refactorMarkers()));
    hasMarker = false;
}

void FunctionDeclDefLink::showMarker(CPPEditorWidget *editor)
{
    if (hasMarker)
        return;

    QList<TextEditor::RefactorMarker> markers = removeMarkersOfType<Marker>(editor->refactorMarkers());
    TextEditor::RefactorMarker marker;

    // show the marker at the end of the linked area, with a special case
    // to avoid it overlapping with a trailing semicolon
    marker.cursor = editor->textCursor();
    marker.cursor.setPosition(linkSelection.selectionEnd());
    const int endBlockNr = marker.cursor.blockNumber();
    marker.cursor.setPosition(linkSelection.selectionEnd() + 1, QTextCursor::KeepAnchor);
    if (marker.cursor.blockNumber() != endBlockNr
            || marker.cursor.selectedText() != QLatin1String(";")) {
        marker.cursor.setPosition(linkSelection.selectionEnd());
    }

    QString message;
    if (targetDeclaration->asFunctionDefinition())
        message = tr("Apply changes to definition");
    else
        message = tr("Apply changes to declaration");

    Core::ActionManager *actionManager = Core::ICore::instance()->actionManager();
    Core::Command *quickfixCommand = actionManager->command(TextEditor::Constants::QUICKFIX_THIS);
    if (quickfixCommand)
        message = Utils::ProxyAction::stringWithAppendedShortcut(message, quickfixCommand->keySequence());

    marker.tooltip = message;
    marker.data = QVariant::fromValue(Marker());
    markers += marker;
    editor->setRefactorMarkers(markers);

    hasMarker = true;
}

// does consider foo(void) to have one argument
static int declaredArgumentCount(Function *function)
{
    int c = function->memberCount();
    if (c > 0 && function->memberAt(c - 1)->isBlock())
        return c - 1;
    return c;
}

Q_GLOBAL_STATIC(QRegExp, commentArgNameRegexp)

static bool hasCommentedName(
        TranslationUnit *unit,
        const QString &source,
        FunctionDeclaratorAST *declarator,
        int i)
{
    if (!declarator
            || !declarator->parameter_declaration_clause
            || !declarator->parameter_declaration_clause->parameter_declaration_list)
        return false;

    if (Function *f = declarator->symbol) {
        if (Symbol *a = f->argumentAt(i)) {
            if (a->name())
                return false;
        }
    }

    ParameterDeclarationListAST *list = declarator->parameter_declaration_clause->parameter_declaration_list;
    while (list && i) {
        list = list->next;
        --i;
    }
    if (!list || !list->value || i)
        return false;

    ParameterDeclarationAST *param = list->value;
    if (param->symbol && param->symbol->name()) {
        return false;
    }

    // maybe in a comment but in the right spot?
    int nameStart = 0;
    if (param->declarator)
        nameStart = unit->tokenAt(param->declarator->lastToken() - 1).end();
    else if (param->type_specifier_list)
        nameStart = unit->tokenAt(param->type_specifier_list->lastToken() - 1).end();
    else
        nameStart = unit->tokenAt(param->firstToken()).begin();

    int nameEnd = 0;
    if (param->equal_token)
        nameEnd = unit->tokenAt(param->equal_token).begin();
    else
        nameEnd = unit->tokenAt(param->lastToken()).begin(); // one token after

    QString text = source.mid(nameStart, nameEnd - nameStart);

    if (commentArgNameRegexp()->isEmpty()) {
        *commentArgNameRegexp() = QRegExp(QLatin1String("/\\*\\s*(\\w*)\\s*\\*/"));
    }
    return commentArgNameRegexp()->indexIn(text) != -1;
}

static bool canReplaceSpecifier(TranslationUnit *translationUnit, SpecifierAST *specifier)
{
    if (SimpleSpecifierAST *simple = specifier->asSimpleSpecifier()) {
        switch (translationUnit->tokenAt(simple->specifier_token).kind()) {
        case T_CONST:
        case T_VOLATILE:
        case T_CHAR:
        case T_WCHAR_T:
        case T_BOOL:
        case T_SHORT:
        case T_INT:
        case T_LONG:
        case T_SIGNED:
        case T_UNSIGNED:
        case T_FLOAT:
        case T_DOUBLE:
        case T_VOID:
        case T_AUTO:
        case T___TYPEOF__:
        case T___ATTRIBUTE__:
            return true;
        default:
            return false;
        }
    }
    if (specifier->asAttributeSpecifier())
        return false;
    return true;
}

static SpecifierAST *findFirstReplaceableSpecifier(TranslationUnit *translationUnit, SpecifierListAST *list)
{
    for (SpecifierListAST *it = list; it; it = it->next) {
        if (canReplaceSpecifier(translationUnit, it->value))
            return it->value;
    }
    return 0;
}

Utils::ChangeSet FunctionDeclDefLink::changes(const Snapshot &snapshot, int targetOffset)
{
    Utils::ChangeSet changes;

    // parse the current source declaration
    TypeOfExpression typeOfExpression; // ### just need to preprocess...
    typeOfExpression.init(sourceDocument, snapshot);

    QString newDeclText = linkSelection.selectedText();
    for (int i = 0; i < newDeclText.size(); ++i) {
        if (newDeclText.at(i).toAscii() == 0)
            newDeclText[i] = QLatin1Char('\n');
    }
    newDeclText.append(QLatin1String("{}"));
    const QString newDeclTextPreprocessed = typeOfExpression.preprocess(newDeclText);

    Document::Ptr newDeclDoc = Document::create(QLatin1String("<decl>"));
    newDeclDoc->setSource(newDeclTextPreprocessed.toUtf8());
    newDeclDoc->parse(Document::ParseDeclaration);
    newDeclDoc->check();

    // extract the function symbol
    if (!newDeclDoc->translationUnit()->ast())
        return changes;
    FunctionDefinitionAST *newDef = newDeclDoc->translationUnit()->ast()->asFunctionDefinition();
    if (!newDef)
        return changes;
    Function *newFunction = newDef->symbol;
    if (!newFunction)
        return changes;

    LookupContext sourceContext(sourceDocument, snapshot);
    LookupContext targetContext(targetFile->cppDocument(), snapshot);

    Overview overview;
    overview.setShowReturnTypes(true);
    overview.setShowTemplateParameters(true);
    overview.setShowArgumentNames(true);
    overview.setShowFunctionSignatures(true);

    // sync return type
    {
        // set up for rewriting return type
        SubstitutionEnvironment env;
        env.setContext(sourceContext);
        env.switchScope(sourceFunction->enclosingScope());
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetFunction->enclosingScope());
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();
        UseMinimalNames q(targetCoN);
        env.enter(&q);
        Control *control = sourceContext.control().data();

        // get return type start position and declarator info from declaration
        DeclaratorAST *declarator = 0;
        SpecifierAST *firstReplaceableSpecifier = 0;
        TranslationUnit *targetTranslationUnit = targetFile->cppDocument()->translationUnit();
        if (SimpleDeclarationAST *simple = targetDeclaration->asSimpleDeclaration()) {
            declarator = simple->declarator_list->value;
            firstReplaceableSpecifier = findFirstReplaceableSpecifier(
                        targetTranslationUnit, simple->decl_specifier_list);
        } else if (FunctionDefinitionAST *def = targetDeclaration->asFunctionDefinition()) {
            declarator = def->declarator;
            firstReplaceableSpecifier = findFirstReplaceableSpecifier(
                        targetTranslationUnit, def->decl_specifier_list);
        }

        int returnTypeStart = 0;
        if (firstReplaceableSpecifier)
            returnTypeStart = targetFile->startOf(firstReplaceableSpecifier);
        else
            returnTypeStart = targetFile->startOf(declarator);

        if (!newFunction->returnType().isEqualTo(sourceFunction->returnType())
                && !newFunction->returnType().isEqualTo(targetFunction->returnType())) {
            FullySpecifiedType type = rewriteType(newFunction->returnType(), &env, control);
            const QString replacement = overview(type, targetFunction->name());
            changes.replace(returnTypeStart,
                            targetFile->startOf(targetFunctionDeclarator->lparen_token),
                            replacement);
        }
    }

    // sync parameters
    {
        // set up for rewriting parameter types
        SubstitutionEnvironment env;
        env.setContext(sourceContext);
        env.switchScope(sourceFunction);
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetFunction);
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();
        UseMinimalNames q(targetCoN);
        env.enter(&q);
        Control *control = sourceContext.control().data();
        Overview overview;

        const unsigned sourceArgCount = declaredArgumentCount(sourceFunction);
        const unsigned newArgCount = declaredArgumentCount(newFunction);
        const unsigned targetArgCount = declaredArgumentCount(targetFunction);

        // check if parameter types or names have changed
        const unsigned existingArgs = qMin(targetArgCount, newArgCount);
        ParameterDeclarationClauseAST *targetParameterDecl =
                targetFunctionDeclarator->parameter_declaration_clause;
        ParameterDeclarationListAST *firstTargetParameterDeclIt =
                targetParameterDecl ? targetParameterDecl->parameter_declaration_list : 0;
        ParameterDeclarationListAST *targetParameterDeclIt = firstTargetParameterDeclIt;
        for (unsigned i = 0;
             i < existingArgs && targetParameterDeclIt;
             ++i, targetParameterDeclIt = targetParameterDeclIt->next) {
            Symbol *targetParam = targetFunction->argumentAt(i);
            Symbol *newParam = newFunction->argumentAt(i);

            // if new's name and type are the same as source's, forbid changes
            bool allowChangeType = true;
            const Name *replacementName = newParam->name();
            if (i < sourceArgCount) {
                Symbol *sourceParam = sourceFunction->argumentAt(i);
                if (newParam->type().isEqualTo(sourceParam->type()))
                    allowChangeType = false;
                if (namesEqual(replacementName, sourceParam->name()))
                    replacementName = targetParam->name();
            }

            // don't change the name if it's in a comment
            if (hasCommentedName(targetFile->cppDocument()->translationUnit(),
                                 targetFile->cppDocument()->source(),
                                 targetFunctionDeclarator, i))
                replacementName = 0;

            // find the end of the parameter declaration
            ParameterDeclarationAST *targetParamAst = targetParameterDeclIt->value;
            int parameterNameEnd = 0;
            if (targetParamAst->declarator)
                parameterNameEnd = targetFile->endOf(targetParamAst->declarator);
            else if (targetParamAst->type_specifier_list)
                parameterNameEnd = targetFile->endOf(targetParamAst->type_specifier_list->lastToken() - 1);
            else
                parameterNameEnd = targetFile->startOf(targetParamAst);

            // change name and type?
            if (allowChangeType
                    && !targetParam->type().isEqualTo(newParam->type())) {
                FullySpecifiedType replacementType = rewriteType(newParam->type(), &env, control);
                const QString replacement = overview(replacementType, replacementName);

                changes.replace(targetFile->startOf(targetParamAst),
                                parameterNameEnd,
                                replacement);
            }
            // change the name only?
            else if (!namesEqual(targetParam->name(), replacementName)) {
                DeclaratorIdAST *id = getDeclaratorId(targetParamAst->declarator);
                QString replacementNameStr = overview(replacementName);
                if (id) {
                    changes.replace(targetFile->range(id), replacementNameStr);
                } else {
                    // add name to unnamed parameter
                    replacementNameStr.prepend(QLatin1Char(' '));
                    int end;
                    if (targetParamAst->equal_token) {
                        end = targetFile->startOf(targetParamAst->equal_token);
                        replacementNameStr.append(QLatin1Char(' '));
                    } else {
                        // one past end on purpose
                        end = targetFile->startOf(targetParamAst->lastToken());
                    }
                    changes.replace(parameterNameEnd, end, replacementNameStr);
                }
            }
        }
        // remove some parameters?
        if (newArgCount < targetArgCount) {
            targetParameterDeclIt = firstTargetParameterDeclIt;
            if (targetParameterDeclIt) {
                if (newArgCount == 0) {
                    changes.remove(targetFile->startOf(targetParameterDeclIt->firstToken()),
                                   targetFile->endOf(targetParameterDeclIt->lastToken() - 1));
                } else {
                    // get the last valid argument
                    for (unsigned i = 0; i < newArgCount - 1 && targetParameterDeclIt; ++i)
                        targetParameterDeclIt = targetParameterDeclIt->next;
                    if (targetParameterDeclIt) {
                        const int start = targetFile->endOf(targetParameterDeclIt->value);
                        const int end = targetFile->endOf(targetParameterDecl->lastToken() - 1);
                        changes.remove(start, end);
                    }
                }
            }
        }
        // add some parameters?
        else if (newArgCount > targetArgCount) {
            QString newParams;
            for (unsigned i = targetArgCount; i < newArgCount; ++i) {
                Symbol *param = newFunction->argumentAt(i);
                FullySpecifiedType type = rewriteType(param->type(), &env, control);
                if (i != 0)
                    newParams += QLatin1String(", ");
                newParams += overview(type, param->name());
            }
            targetParameterDeclIt = firstTargetParameterDeclIt;
            if (targetParameterDeclIt) {
                while (targetParameterDeclIt->next)
                    targetParameterDeclIt = targetParameterDeclIt->next;
                changes.insert(targetFile->endOf(targetParameterDeclIt->value), newParams);
            } else {
                changes.insert(targetFile->endOf(targetFunctionDeclarator->lparen_token), newParams);
            }
        }
    }

    // sync cv qualification
    if (targetFunction->isConst() != newFunction->isConst()
            || targetFunction->isVolatile() != newFunction->isVolatile()) {
        QString cvString;
        if (newFunction->isConst())
            cvString += QLatin1String("const");
        if (newFunction->isVolatile()) {
            if (!cvString.isEmpty())
                cvString += QLatin1Char(' ');
            cvString += QLatin1String("volatile");
        }
        const int rparenEnd = targetFile->endOf(targetFunctionDeclarator->rparen_token);
        if (targetFunctionDeclarator->cv_qualifier_list) {
            const int cvEnd = targetFile->endOf(targetFunctionDeclarator->cv_qualifier_list->lastToken() - 1);
            // if the qualifies changed, replace
            if (!cvString.isEmpty()) {
                changes.replace(targetFile->startOf(targetFunctionDeclarator->cv_qualifier_list->firstToken()),
                                cvEnd, cvString);
            } else {
                // remove
                changes.remove(rparenEnd, cvEnd);
            }
        } else {
            // otherwise add
            cvString.prepend(QLatin1Char(' '));
            changes.insert(rparenEnd, cvString);
        }
    }

    if (targetOffset != -1) {
        // move all change operations to have the right start offset
        const int moveAmount = targetOffset - targetFile->startOf(targetDeclaration);
        QList<Utils::ChangeSet::EditOp> ops = changes.operationList();
        for (int i = 0; i < ops.size(); ++i) {
            ops[i].pos1 += moveAmount;
            ops[i].pos2 += moveAmount;
        }
        changes = Utils::ChangeSet(ops);
    }

    return changes;
}

class ApplyDeclDefLinkOperation : public CppQuickFixOperation
{
public:
    explicit ApplyDeclDefLinkOperation(
            const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface,
            const QSharedPointer<FunctionDeclDefLink> &link,
            int priority = -1)
        : CppQuickFixOperation(interface, priority)
        , m_link(link)
    {}

    virtual void perform()
    {
        CPPEditorWidget *editor = assistInterface()->editor();
        QSharedPointer<FunctionDeclDefLink> link = editor->declDefLink();
        if (link != m_link)
            return;

        return editor->applyDeclDefLinkChanges(/*don't jump*/false);
    }

protected:
    virtual void performChanges(const CppTools::CppRefactoringFilePtr &, const CppTools::CppRefactoringChanges &)
    { /* never called since perform is overridden */ }

private:
    QSharedPointer<FunctionDeclDefLink> m_link;
};

QList<CppQuickFixOperation::Ptr> ApplyDeclDefLinkChanges::match(const QSharedPointer<const CppQuickFixAssistInterface> &interface)
{
    QList<CppQuickFixOperation::Ptr> results;

    QSharedPointer<FunctionDeclDefLink> link = interface->editor()->declDefLink();
    if (!link || !link->isMarkerVisible())
        return results;

    QSharedPointer<ApplyDeclDefLinkOperation> op(new ApplyDeclDefLinkOperation(interface, link));
    op->setDescription(FunctionDeclDefLink::tr("Apply function signature changes"));
    op->setPriority(0);
    results += op;

    return results;
}
