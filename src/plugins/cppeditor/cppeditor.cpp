/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppplugin.h"
#include "cpphighlighter.h"
#include "cppquickfix.h"
#include <cpptools/cpptoolsplugin.h>

#include <AST.h>
#include <Control.h>
#include <Token.h>
#include <Scope.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>
#include <Literals.h>
#include <Semantic.h>
#include <ASTVisitor.h>
#include <SymbolVisitor.h>
#include <TranslationUnit.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TokenUnderCursor.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/FastPreprocessor.h>
#include <cplusplus/CppBindings.h>

#include <cpptools/cppmodelmanagerinterface.h>

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <utils/uncommentselection.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textblockiterator.h>
#include <indenter.h>

#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QStack>
#include <QtCore/QSettings>
#include <QtCore/QSignalMapper>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QShortcut>
#include <QtGui/QTextEdit>
#include <QtGui/QComboBox>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>
#include <QtGui/QSortFilterProxyModel>

#include <sstream>

enum {
    UPDATE_METHOD_BOX_INTERVAL = 150,
    UPDATE_USES_INTERVAL = 300
};

using namespace CPlusPlus;
using namespace CppEditor::Internal;

namespace {

class OverviewTreeView : public QTreeView
{
public:
    OverviewTreeView(QWidget *parent = 0)
        : QTreeView(parent)
    {
        // TODO: Disable the root for all items (with a custom delegate?)
        setRootIsDecorated(false);
    }

    void sync()
    {
        expandAll();
        setMinimumWidth(qMax(sizeHintForColumn(0), minimumSizeHint().width()));
    }
};

class FindScope: protected SymbolVisitor
{
    TranslationUnit *_unit;
    Scope *_scope;
    unsigned _line;
    unsigned _column;

public:
    Scope *operator()(unsigned line, unsigned column,
                      Symbol *root, TranslationUnit *unit)
    {
        _unit = unit;
        _scope = 0;
        _line = line;
        _column = column;
        accept(root);
        return _scope;
    }

private:
    using SymbolVisitor::visit;

    virtual bool preVisit(Symbol *)
    { return ! _scope; }

    virtual bool visit(Block *block)
    { return processScope(block->members()); }

    virtual bool visit(Function *function)
    { return processScope(function->members()); }

    virtual bool visit(ObjCMethod *method)
    { return processScope(method->members()); }

    bool processScope(Scope *scope)
    {
        if (_scope || ! scope)
            return false;

        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            accept(scope->symbolAt(i));

            if (_scope)
                return false;
        }

        unsigned startOffset = scope->owner()->startOffset();
        unsigned endOffset = scope->owner()->endOffset();

        unsigned startLine, startColumn;
        unsigned endLine, endColumn;

        _unit->getPosition(startOffset, &startLine, &startColumn);
        _unit->getPosition(endOffset, &endLine, &endColumn);

        if (_line > startLine || (_line == startLine && _column >= startColumn)) {
            if (_line < endLine || (_line == endLine && _column < endColumn)) {
                _scope = scope;
            }
        }

        return false;
    }
};

class FindLocalUses: protected ASTVisitor
{
    Scope *_functionScope;
    FindScope findScope;

public:
    FindLocalUses(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit), hasD(false), hasQ(false)
    { }

    // local and external uses.
    SemanticInfo::LocalUseMap localUses;
    bool hasD;
    bool hasQ;

    void operator()(DeclarationAST *ast)
    {
        localUses.clear();

        if (!ast)
            return;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            if (def->symbol) {
                _functionScope = def->symbol->members();
                accept(ast);
            }
        } else if (ObjCMethodDeclarationAST *decl = ast->asObjCMethodDeclaration()) {
            if (decl->method_prototype->symbol) {
                _functionScope = decl->method_prototype->symbol->members();
                accept(ast);
            }
        }
    }

protected:
    using ASTVisitor::visit;

    bool findMember(Scope *scope, NameAST *ast, unsigned line, unsigned column)
    {
        if (! (ast && ast->name))
            return false;

        const Identifier *id = ast->name->identifier();

        if (scope) {
            for (Symbol *member = scope->lookat(id); member; member = member->next()) {
                if (member->identifier() != id)
                    continue;
                else if (member->line() < line || (member->line() == line && member->column() <= column)) {
                    localUses[member].append(SemanticInfo::Use(line, column, id->size()));
                    return true;
                }
            }
        }

        return false;
    }

    void searchUsesInTemplateArguments(NameAST *name)
    {
        if (! name)
            return;

        else if (TemplateIdAST *template_id = name->asTemplateId()) {
            for (TemplateArgumentListAST *it = template_id->template_argument_list; it; it = it->next) {
                accept(it->value);
            }
        }
    }

    virtual bool visit(SimpleNameAST *ast)
    { return findMemberForToken(ast->firstToken(), ast); }

    virtual bool visit(ObjCMessageArgumentDeclarationAST *ast)
    { return findMemberForToken(ast->param_name_token, ast); }

    bool findMemberForToken(unsigned tokenIdx, NameAST *ast)
    {
        unsigned line, column;
        getTokenStartPosition(tokenIdx, &line, &column);

        Scope *scope = findScope(line, column,
                                 _functionScope->owner(),
                                 translationUnit());

        while (scope) {
            if (scope->isFunctionScope()) {
                Function *fun = scope->owner()->asFunction();
                if (findMember(fun->members(), ast, line, column))
                    return false;
                else if (findMember(fun->arguments(), ast, line, column))
                    return false;
            } else if (scope->isObjCMethodScope()) {
                ObjCMethod *method = scope->owner()->asObjCMethod();
                if (findMember(method->members(), ast, line, column))
                    return false;
                else if (findMember(method->arguments(), ast, line, column))
                    return false;
            } else if (scope->isBlockScope()) {
                if (findMember(scope, ast, line, column))
                    return false;
            } else {
                break;
            }

            scope = scope->enclosingScope();
        }

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        for (TemplateArgumentListAST *arg = ast->template_argument_list; arg; arg = arg->next)
            accept(arg->value);

        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);

        Scope *scope = findScope(line, column,
                                 _functionScope->owner(),
                                 translationUnit());

        while (scope) {
            if (scope->isFunctionScope()) {
                Function *fun = scope->owner()->asFunction();
                if (findMember(fun->members(), ast, line, column))
                    return false;
                else if (findMember(fun->arguments(), ast, line, column))
                    return false;
            } else if (scope->isBlockScope()) {
                if (findMember(scope, ast, line, column))
                    return false;
            } else {
                break;
            }

            scope = scope->enclosingScope();
        }

        return false;
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next)
            searchUsesInTemplateArguments(it->value->class_or_namespace_name);

        searchUsesInTemplateArguments(ast->unqualified_name);
        return false;
    }

    virtual bool visit(PostfixExpressionAST *ast)
    {
        accept(ast->base_expression);
        for (PostfixListAST *it = ast->postfix_expression_list; it; it = it->next) {
            PostfixAST *fx = it->value;
            if (fx->asMemberAccess() != 0)
                continue; // skip members
            accept(fx);
        }
        return false;
    }

    virtual bool visit(ElaboratedTypeSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(ClassSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(EnumSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(UsingDirectiveAST *)
    {
        return false;
    }

    virtual bool visit(UsingAST *ast)
    {
        accept(ast->name);
        return false;
    }

    virtual bool visit(QtMemberDeclarationAST *ast)
    {
        if (tokenKind(ast->q_token) == T_Q_D)
            hasD = true;
        else
            hasQ = true;

        return true;
    }

    virtual bool visit(ExpressionOrDeclarationStatementAST *ast)
    {
        accept(ast->declaration);
        return false;
    }

    virtual bool visit(FunctionDeclaratorAST *ast)
    {
        accept(ast->parameters);

        for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next)
            accept(it->value);

        accept(ast->exception_specification);

        return false;
    }

    virtual bool visit(ObjCMethodPrototypeAST *ast)
    {
        accept(ast->argument_list);
        return false;
    }
};


class FunctionDefinitionUnderCursor: protected ASTVisitor
{
    unsigned _line;
    unsigned _column;
    DeclarationAST *_functionDefinition;

public:
    FunctionDefinitionUnderCursor(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit),
          _line(0), _column(0)
    { }

    DeclarationAST *operator()(AST *ast, unsigned line, unsigned column)
    {
        _functionDefinition = 0;
        _line = line;
        _column = column;
        accept(ast);
        return _functionDefinition;
    }

protected:
    virtual bool preVisit(AST *ast)
    {
        if (_functionDefinition)
            return false;

        else if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            return checkDeclaration(def);
        }

        else if (ObjCMethodDeclarationAST *method = ast->asObjCMethodDeclaration()) {
            if (method->function_body)
                return checkDeclaration(method);
        }

        return true;
    }

private:
    bool checkDeclaration(DeclarationAST *ast)
    {
        unsigned startLine, startColumn;
        unsigned endLine, endColumn;
        getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
        getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

        if (_line > startLine || (_line == startLine && _column >= startColumn)) {
            if (_line < endLine || (_line == endLine && _column < endColumn)) {
                _functionDefinition = ast;
                return false;
            }
        }

        return true;
    }
};

class ProcessDeclarators: protected ASTVisitor
{
    QList<DeclaratorIdAST *> _declarators;
    bool _visitFunctionDeclarator;

public:
    ProcessDeclarators(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit),
          _visitFunctionDeclarator(true)
    { }

    QList<DeclaratorIdAST *> operator()(FunctionDefinitionAST *ast)
    {
        _declarators.clear();

        if (ast) {
            if (ast->declarator) {
                _visitFunctionDeclarator = true;
                accept(ast->declarator->postfix_declarator_list);
            }

            _visitFunctionDeclarator = false;
            accept(ast->function_body);
        }

        return _declarators;
    }

protected:
    using ASTVisitor::visit;

    virtual bool visit(FunctionDeclaratorAST *)
    { return _visitFunctionDeclarator; }

    virtual bool visit(DeclaratorIdAST *ast)
    {
        _declarators.append(ast);
        return true;
    }
};

class FindFunctionDefinitions: protected SymbolVisitor
{
    const Name *_declarationName;
    QList<Function *> *_functions;

public:
    FindFunctionDefinitions()
        : _declarationName(0),
          _functions(0)
    { }

    void operator()(const Name *declarationName, Scope *globals,
                    QList<Function *> *functions)
    {
        _declarationName = declarationName;
        _functions = functions;

        for (unsigned i = 0; i < globals->symbolCount(); ++i) {
            accept(globals->symbolAt(i));
        }
    }

protected:
    using SymbolVisitor::visit;

    virtual bool visit(Function *function)
    {
        const Name *name = function->name();
        if (const QualifiedNameId *q = name->asQualifiedNameId())
            name = q->unqualifiedNameId();

        if (_declarationName->isEqualTo(name))
            _functions->append(function);

        return false;
    }
};

} // end of anonymous namespace

static const QualifiedNameId *qualifiedNameIdForSymbol(Symbol *s, const LookupContext &context)
{
    const Name *symbolName = s->name();
    if (! symbolName)
        return 0; // nothing to do.

    QVector<const Name *> names;

    for (Scope *scope = s->scope(); scope; scope = scope->enclosingScope()) {
        if (scope->isClassScope() || scope->isNamespaceScope()) {
            if (scope->owner() && scope->owner()->name()) {
                const Name *ownerName = scope->owner()->name();
                if (const QualifiedNameId *q = ownerName->asQualifiedNameId()) {
                    for (unsigned i = 0; i < q->nameCount(); ++i) {
                        names.prepend(q->nameAt(i));
                    }
                } else {
                    names.prepend(ownerName);
                }
            }
        }
    }

    if (const QualifiedNameId *q = symbolName->asQualifiedNameId()) {
        for (unsigned i = 0; i < q->nameCount(); ++i) {
            names.append(q->nameAt(i));
        }
    } else {
        names.append(symbolName);
    }

    return context.control()->qualifiedNameId(names.constData(), names.size());
}

CPPEditorEditable::CPPEditorEditable(CPPEditor *editor)
    : BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(CppEditor::Constants::C_CPPEDITOR);
    m_context << uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

CPPEditor::CPPEditor(QWidget *parent)
    : TextEditor::BaseTextEditor(parent)
    , m_currentRenameSelection(-1)
    , m_inRename(false)
    , m_inRenameChanged(false)
    , m_firstRenameChange(false)
{
    m_initialized = false;
    qRegisterMetaType<SemanticInfo>("SemanticInfo");

    m_semanticHighlighter = new SemanticHighlighter(this);
    m_semanticHighlighter->start();

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);
    baseTextDocument()->setSyntaxHighlighter(new CppHighlighter);

    m_modelManager = CppTools::CppModelManagerInterface::instance();

    if (m_modelManager) {
        connect(m_modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
                this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    }
}

CPPEditor::~CPPEditor()
{
    Core::EditorManager::instance()->hideEditorInfoBar(QLatin1String("CppEditor.Rename"));

    m_semanticHighlighter->abort();
    m_semanticHighlighter->wait();
}

TextEditor::BaseTextEditorEditable *CPPEditor::createEditableInterface()
{
    CPPEditorEditable *editable = new CPPEditorEditable(this);
    createToolBar(editable);
    return editable;
}

void CPPEditor::createToolBar(CPPEditorEditable *editable)
{
    m_methodCombo = new QComboBox;
    m_methodCombo->setMinimumContentsLength(22);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_methodCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_methodCombo->setSizePolicy(policy);

    QTreeView *methodView = new OverviewTreeView;
    methodView->header()->hide();
    methodView->setItemsExpandable(false);
    m_methodCombo->setView(methodView);
    m_methodCombo->setMaxVisibleItems(20);

    m_overviewModel = new OverviewModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_overviewModel);
    if (CppPlugin::instance()->sortedMethodOverview())
        m_proxyModel->sort(0, Qt::AscendingOrder);
    else
        m_proxyModel->sort(-1, Qt::AscendingOrder); // don't sort yet, but set column for sortedMethodOverview()
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_methodCombo->setModel(m_proxyModel);

    m_methodCombo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_sortAction = new QAction(tr("Sort alphabetically"), m_methodCombo);
    m_sortAction->setCheckable(true);
    m_sortAction->setChecked(sortedMethodOverview());
    connect(m_sortAction, SIGNAL(toggled(bool)), CppPlugin::instance(), SLOT(setSortedMethodOverview(bool)));
    m_methodCombo->addAction(m_sortAction);

    m_updateMethodBoxTimer = new QTimer(this);
    m_updateMethodBoxTimer->setSingleShot(true);
    m_updateMethodBoxTimer->setInterval(UPDATE_METHOD_BOX_INTERVAL);
    connect(m_updateMethodBoxTimer, SIGNAL(timeout()), this, SLOT(updateMethodBoxIndexNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setSingleShot(true);
    m_updateUsesTimer->setInterval(UPDATE_USES_INTERVAL);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    connect(m_methodCombo, SIGNAL(activated(int)), this, SLOT(jumpToMethod(int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateMethodBoxIndex()));
    connect(m_methodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMethodBoxToolTip()));
    connect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onContentsChanged(int,int,int)));

    connect(file(), SIGNAL(changed()), this, SLOT(updateFileName()));


    // set up the semantic highlighter
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));

    connect(m_semanticHighlighter, SIGNAL(changed(SemanticInfo)),
            this, SLOT(updateSemanticInfo(SemanticInfo)));

    QToolBar *toolBar = static_cast<QToolBar*>(editable->toolBar());
    QList<QAction*> actions = toolBar->actions();
    QWidget *w = toolBar->widgetForAction(actions.first());
    static_cast<QHBoxLayout*>(w->layout())->insertWidget(0, m_methodCombo, 1);
}

void CPPEditor::inAllRenameSelections(EditOperation operation,
                                      const QTextEdit::ExtraSelection &currentRenameSelection,
                                      QTextCursor cursor,
                                      const QString &text)
{
    cursor.beginEditBlock();

    const int startOffset = cursor.selectionStart() - currentRenameSelection.cursor.anchor();
    const int endOffset = cursor.selectionEnd() - currentRenameSelection.cursor.anchor();
    const int length = endOffset - startOffset;

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection &s = m_renameSelections[i];
        int pos = s.cursor.anchor();
        int endPos = s.cursor.position();

        s.cursor.setPosition(pos + startOffset);
        s.cursor.setPosition(pos + endOffset, QTextCursor::KeepAnchor);

        switch (operation) {
        case DeletePreviousChar:
            s.cursor.deletePreviousChar();
            endPos -= qMax(1, length);
            break;
        case DeleteChar:
            s.cursor.deleteChar();
            endPos -= qMax(1, length);
            break;
        case InsertText:
            s.cursor.insertText(text);
            endPos += text.length() - length;
            break;
        }

        s.cursor.setPosition(pos);
        s.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    }

    cursor.endEditBlock();

    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
    setTextCursor(cursor);
}

void CPPEditor::paste()
{
    if (m_currentRenameSelection == -1) {
        BaseTextEditor::paste();
        return;
    }

    startRename();
    BaseTextEditor::paste();
    finishRename();
}

void CPPEditor::cut()
{
    if (m_currentRenameSelection == -1) {
        BaseTextEditor::cut();
        return;
    }

    startRename();
    BaseTextEditor::cut();
    finishRename();
}

void CPPEditor::startRename()
{
    m_inRenameChanged = false;
}

void CPPEditor::finishRename()
{
    if (!m_inRenameChanged)
        return;

    m_inRename = true;

    QTextCursor cursor = textCursor();
    cursor.joinPreviousEditBlock();

    cursor.setPosition(m_currentRenameSelectionEnd.position());
    cursor.setPosition(m_currentRenameSelectionBegin.position(), QTextCursor::KeepAnchor);
    m_renameSelections[m_currentRenameSelection].cursor = cursor;
    QString text = cursor.selectedText();

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        if (i == m_currentRenameSelection)
            continue;
        QTextEdit::ExtraSelection &s = m_renameSelections[i];
        int pos = s.cursor.selectionStart();
        s.cursor.removeSelectedText();
        s.cursor.insertText(text);
        s.cursor.setPosition(pos, QTextCursor::KeepAnchor);
    }

    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
    cursor.endEditBlock();

    m_inRename = false;
}

void CPPEditor::abortRename()
{
    if (m_currentRenameSelection < 0)
        return;
    m_renameSelections[m_currentRenameSelection].format = m_occurrencesFormat;
    m_currentRenameSelection = -1;
    m_currentRenameSelectionBegin = QTextCursor();
    m_currentRenameSelectionEnd = QTextCursor();
    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
}

void CPPEditor::onDocumentUpdated(Document::Ptr doc)
{
    if (doc->fileName() != file()->fileName())
        return;

    if (doc->editorRevision() != editorRevision())
        return;

    if (! m_initialized) {
        m_initialized = true;

        const SemanticHighlighter::Source source = currentSource(/*force = */ true);
        m_semanticHighlighter->rehighlight(source);
    }

    m_overviewModel->rebuild(doc);
    OverviewTreeView *treeView = static_cast<OverviewTreeView *>(m_methodCombo->view());
    treeView->sync();
    updateMethodBoxIndexNow();
}

CPlusPlus::Symbol *CPPEditor::findCanonicalSymbol(const QTextCursor &cursor,
                                                  Document::Ptr doc,
                                                  const Snapshot &snapshot) const
{
    if (! doc)
        return 0;

    QTextCursor tc = cursor;
    int line, col;
    convertPosition(tc.position(), &line, &col);
    ++col; // 1-based line and 1-based column

    int pos = tc.position();
    while (document()->characterAt(pos).isLetterOrNumber() ||
           document()->characterAt(pos) == QLatin1Char('_'))
        ++pos;
    tc.setPosition(pos);

    ExpressionUnderCursor expressionUnderCursor;
    const QString code = expressionUnderCursor(tc);
    // qDebug() << "code:" << code;

    TypeOfExpression typeOfExpression;
    typeOfExpression.setSnapshot(snapshot);

    Symbol *lastVisibleSymbol = doc->findSymbolAt(line, col);

    const QList<LookupItem> results = typeOfExpression(code, doc,
                                                   lastVisibleSymbol,
                                                   TypeOfExpression::Preprocess);

    NamespaceBindingPtr glo = bind(doc, snapshot);
    Symbol *canonicalSymbol = LookupContext::canonicalSymbol(results, glo.data());

    return canonicalSymbol;
}

const Macro *CPPEditor::findCanonicalMacro(const QTextCursor &cursor,
                                                  Document::Ptr doc) const
{
    if (! doc)
        return 0;

    int line, col;
    convertPosition(cursor.position(), &line, &col);

    if (const Macro *macro = doc->findMacroDefinitionAt(line))
        return macro;

    if (const Document::MacroUse *use = doc->findMacroUseAt(cursor.position()))
        return &use->macro();

    return 0;
}

void CPPEditor::findUsages()
{
    if (Symbol *canonicalSymbol = markSymbols()) {
        m_modelManager->findUsages(canonicalSymbol);
    } else if (const Macro *macro = findCanonicalMacro(textCursor(), m_lastSemanticInfo.doc)) {
        m_modelManager->findMacroUsages(*macro);
    }
}

void CPPEditor::renameUsages()
{
    renameUsagesNow();
}

bool CPPEditor::showWarningMessage() const
{
    // Restore settings
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("CppEditor"));
    settings->beginGroup(QLatin1String("Rename"));
    const bool showWarningMessage = settings->value(QLatin1String("ShowWarningMessage"), true).toBool();
    settings->endGroup();
    settings->endGroup();
    return showWarningMessage;
}

void CPPEditor::setShowWarningMessage(bool showWarningMessage)
{
    // Restore settings
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("CppEditor"));
    settings->beginGroup(QLatin1String("Rename"));
    settings->setValue(QLatin1String("ShowWarningMessage"), showWarningMessage);
    settings->endGroup();
    settings->endGroup();
}

void CPPEditor::hideRenameNotification()
{
    setShowWarningMessage(false);
    Core::EditorManager::instance()->hideEditorInfoBar(QLatin1String("CppEditor.Rename"));
}

void CPPEditor::renameUsagesNow()
{
    if (Symbol *canonicalSymbol = markSymbols()) {
        if (canonicalSymbol->identifier() != 0) {
            if (showWarningMessage()) {
                Core::EditorManager::instance()->showEditorInfoBar(QLatin1String("CppEditor.Rename"),
                                                                   tr("This change cannot be undone."),
                                                                   tr("Yes, I know what I am doing."),
                                                                   this, SLOT(hideRenameNotification()));
            }

            m_modelManager->renameUsages(canonicalSymbol);
        }
    }
}

Symbol *CPPEditor::markSymbols()
{
    updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    abortRename();

    QList<QTextEdit::ExtraSelection> selections;

    SemanticInfo info = m_lastSemanticInfo;

    Symbol *canonicalSymbol = findCanonicalSymbol(textCursor(), info.doc, info.snapshot);
    if (canonicalSymbol) {
        TranslationUnit *unit = info.doc->translationUnit();

        const QList<int> references = m_modelManager->references(canonicalSymbol, info.doc, info.snapshot);
        foreach (int index, references) {
            unsigned line, column;
            unit->getTokenPosition(index, &line, &column);

            if (column)
                --column;  // adjust the column position.

            const int len = unit->tokenAt(index).f.length;

            QTextCursor cursor(document()->findBlockByNumber(line - 1));
            cursor.setPosition(cursor.position() + column);
            cursor.setPosition(cursor.position() + len, QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.format = m_occurrencesFormat;
            sel.cursor = cursor;
            selections.append(sel);
        }
    }

    setExtraSelections(CodeSemanticsSelection, selections);
    return canonicalSymbol;
}

void CPPEditor::renameSymbolUnderCursor()
{
    updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));
    abortRename();

    QTextCursor c = textCursor();

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection s = m_renameSelections.at(i);
        if (c.position() >= s.cursor.anchor()
                && c.position() <= s.cursor.position()) {
            m_currentRenameSelection = i;
            m_firstRenameChange = true;
            m_currentRenameSelectionBegin = QTextCursor(c.document()->docHandle(),
                                                        m_renameSelections[i].cursor.selectionStart());
            m_currentRenameSelectionEnd = QTextCursor(c.document()->docHandle(),
                                                        m_renameSelections[i].cursor.selectionEnd());
            m_renameSelections[i].format = m_occurrenceRenameFormat;
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);
            break;
        }
    }

    if (m_renameSelections.isEmpty())
        renameUsages();
}

void CPPEditor::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position)

    if (m_currentRenameSelection == -1 || m_inRename)
        return;

    if (position + charsAdded == m_currentRenameSelectionBegin.position()) {
        // we are inserting at the beginning of the rename selection => expand
        m_currentRenameSelectionBegin.setPosition(position);
        m_renameSelections[m_currentRenameSelection].cursor.setPosition(position, QTextCursor::KeepAnchor);
    }

    // the condition looks odd, but keep in mind that the begin and end cursors do move automatically
    m_inRenameChanged = (position >= m_currentRenameSelectionBegin.position()
                         && position + charsAdded <= m_currentRenameSelectionEnd.position());

    if (!m_inRenameChanged)
        abortRename();

    if (charsRemoved > 0)
        updateUses();
}

void CPPEditor::updateFileName()
{ }

void CPPEditor::jumpToMethod(int)
{
    QModelIndex index = m_proxyModel->mapToSource(m_methodCombo->view()->currentIndex());
    Symbol *symbol = m_overviewModel->symbolFromIndex(index);
    if (! symbol)
        return;

    openCppEditorAt(linkToSymbol(symbol));
}

void CPPEditor::setSortedMethodOverview(bool sort)
{
    if (sort != sortedMethodOverview()) {
        if (sort)
            m_proxyModel->sort(0, Qt::AscendingOrder);
        else
            m_proxyModel->sort(-1, Qt::AscendingOrder);
        bool block = m_sortAction->blockSignals(true);
        m_sortAction->setChecked(m_proxyModel->sortColumn() == 0);
        m_sortAction->blockSignals(block);
        updateMethodBoxIndexNow();
    }
}

bool CPPEditor::sortedMethodOverview() const
{
    return (m_proxyModel->sortColumn() == 0);
}

void CPPEditor::updateMethodBoxIndex()
{
    m_updateMethodBoxTimer->start();
}

void CPPEditor::highlightUses(const QList<SemanticInfo::Use> &uses,
                              const SemanticInfo &semanticInfo,
                              QList<QTextEdit::ExtraSelection> *selections)
{
    bool isUnused = false;

    if (uses.size() == 1)
        isUnused = true;

    foreach (const SemanticInfo::Use &use, uses) {
        QTextEdit::ExtraSelection sel;

        if (isUnused)
            sel.format = m_occurrencesUnusedFormat;
        else
            sel.format = m_occurrencesFormat;

        const int anchor = document()->findBlockByNumber(use.line - 1).position() + use.column - 1;
        const int position = anchor + use.length;

        sel.cursor = QTextCursor(document());
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        if (isUnused) {
            if (semanticInfo.hasQ && sel.cursor.selectedText() == QLatin1String("q"))
                continue; // skip q

            else if (semanticInfo.hasD && sel.cursor.selectedText() == QLatin1String("d"))
                continue; // skip d
        }

        selections->append(sel);
    }
}

void CPPEditor::updateMethodBoxIndexNow()
{
    if (! m_overviewModel->document())
        return;

    if (m_overviewModel->document()->editorRevision() != editorRevision()) {
        m_updateMethodBoxTimer->start();
        return;
    }

    m_updateMethodBoxTimer->stop();

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    QModelIndex lastIndex;

    const int rc = m_overviewModel->rowCount();
    for (int row = 0; row < rc; ++row) {
        const QModelIndex index = m_overviewModel->index(row, 0, QModelIndex());
        Symbol *symbol = m_overviewModel->symbolFromIndex(index);
        if (symbol && symbol->line() > unsigned(line))
            break;
        lastIndex = index;
    }

    if (lastIndex.isValid()) {
        bool blocked = m_methodCombo->blockSignals(true);
        m_methodCombo->setCurrentIndex(m_proxyModel->mapFromSource(lastIndex).row());
        updateMethodBoxToolTip();
        (void) m_methodCombo->blockSignals(blocked);
    }
}

void CPPEditor::updateMethodBoxToolTip()
{
    m_methodCombo->setToolTip(m_methodCombo->currentText());
}

void CPPEditor::updateUses()
{
    m_updateUsesTimer->start();
}

void CPPEditor::updateUsesNow()
{
    if (m_currentRenameSelection != -1)
        return;

    semanticRehighlight();
}

static bool isCompatible(const Name *name, const Name *otherName)
{
    if (const NameId *nameId = name->asNameId()) {
        if (const TemplateNameId *otherTemplId = otherName->asTemplateNameId())
            return nameId->identifier()->isEqualTo(otherTemplId->identifier());
    } else if (const TemplateNameId *templId = name->asTemplateNameId()) {
        if (const NameId *otherNameId = otherName->asNameId())
            return templId->identifier()->isEqualTo(otherNameId->identifier());
    }

    return name->isEqualTo(otherName);
}

static bool isCompatible(Function *definition, Symbol *declaration,
                         const QualifiedNameId *declarationName)
{
    Function *declTy = declaration->type()->asFunctionType();
    if (! declTy)
        return false;

    const Name *definitionName = definition->name();
    if (const QualifiedNameId *q = definitionName->asQualifiedNameId()) {
        if (! isCompatible(q->unqualifiedNameId(), declaration->name()))
            return false;
        else if (q->nameCount() > declarationName->nameCount())
            return false;
        else if (declTy->argumentCount() != definition->argumentCount())
            return false;
        else if (declTy->isConst() != definition->isConst())
            return false;
        else if (declTy->isVolatile() != definition->isVolatile())
            return false;

        for (unsigned i = 0; i < definition->argumentCount(); ++i) {
            Symbol *arg = definition->argumentAt(i);
            Symbol *otherArg = declTy->argumentAt(i);
            if (! arg->type().isEqualTo(otherArg->type()))
                return false;
        }

        for (unsigned i = 0; i != q->nameCount(); ++i) {
            const Name *n = q->nameAt(q->nameCount() - i - 1);
            const Name *m = declarationName->nameAt(declarationName->nameCount() - i - 1);
            if (! isCompatible(n, m))
                return false;
        }
        return true;
    } else {
        // ### TODO: implement isCompatible for unqualified name ids.
    }
    return false;
}

void CPPEditor::switchDeclarationDefinition()
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    if (!m_modelManager)
        return;

    const Snapshot snapshot = m_modelManager->snapshot();

    Document::Ptr doc = snapshot.document(file()->fileName());
    if (!doc)
        return;
    Symbol *lastSymbol = doc->findSymbolAt(line, column);
    if (!lastSymbol || !lastSymbol->scope())
        return;

    Function *f = lastSymbol->asFunction();
    if (!f) {
        Scope *fs = lastSymbol->scope();
        if (!fs->isFunctionScope())
            fs = fs->enclosingFunctionScope();
        if (fs)
            f = fs->owner()->asFunction();
    }

    if (f) {
        TypeOfExpression typeOfExpression;
        typeOfExpression.setSnapshot(m_modelManager->snapshot());
        QList<LookupItem> resolvedSymbols = typeOfExpression(QString(), doc, lastSymbol);
        const LookupContext &context = typeOfExpression.lookupContext();

        const QualifiedNameId *q = qualifiedNameIdForSymbol(f, context);
        QList<Symbol *> symbols = context.resolve(q);

        Symbol *declaration = 0;
        foreach (declaration, symbols) {
            if (isCompatible(f, declaration, q))
                break;
        }

        if (! declaration && ! symbols.isEmpty())
            declaration = symbols.first();

        if (declaration)
            openCppEditorAt(linkToSymbol(declaration));
    } else if (lastSymbol->type()->isFunctionType()) {
        if (Symbol *def = findDefinition(lastSymbol))
            openCppEditorAt(linkToSymbol(def));
    }
}

CPPEditor::Link CPPEditor::findLinkAt(const QTextCursor &cursor,
                                      bool resolveTarget)
{
    Link link;

    if (!m_modelManager)
        return link;

    const Snapshot snapshot = m_modelManager->snapshot();
    int line = 0, column = 0;
    convertPosition(cursor.position(), &line, &column);
    Document::Ptr doc = snapshot.document(file()->fileName());
    if (!doc)
        return link;

    QTextCursor tc = cursor;

    // Make sure we're not at the start of a word
    {
        const QChar c = characterAt(tc.position());
        if (c.isLetter() || c == QLatin1Char('_'))
            tc.movePosition(QTextCursor::Right);
    }

    static TokenUnderCursor tokenUnderCursor;

    QTextBlock block;
    const SimpleToken tk = tokenUnderCursor(tc, &block);

    const int beginOfToken = block.position() + tk.begin();
    const int endOfToken = block.position() + tk.end();

    // Handle include directives
    if (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)) {
        const unsigned lineno = cursor.blockNumber() + 1;
        foreach (const Document::Include &incl, doc->includes()) {
            if (incl.line() == lineno && incl.resolved()) {
                link.fileName = incl.fileName();
                link.begin = beginOfToken + 1;
                link.end = endOfToken - 1;
                return link;
            }
        }
    }

    if (tk.isNot(T_IDENTIFIER))
        return link;

    // Find the last symbol up to the cursor position
    Symbol *lastSymbol = doc->findSymbolAt(line, column);
    if (!lastSymbol)
        return link;

    tc.setPosition(endOfToken);

    // Evaluate the type of the expression under the cursor
    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);
    TypeOfExpression typeOfExpression;
    typeOfExpression.setSnapshot(snapshot);
    QList<LookupItem> resolvedSymbols =
            typeOfExpression(expression, doc, lastSymbol);

    if (!resolvedSymbols.isEmpty()) {
        LookupItem result = resolvedSymbols.first();
        const FullySpecifiedType ty = result.type().simplified();

        if (ty->isForwardClassDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                LookupItem r = resolvedSymbols.takeFirst();

                if (! r.type()->isForwardClassDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (ty->isObjCForwardClassDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                LookupItem r = resolvedSymbols.takeFirst();

                if (! r.type()->isObjCForwardClassDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (ty->isObjCForwardProtocolDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                LookupItem r = resolvedSymbols.takeFirst();

                if (! r.type()->isObjCForwardProtocolDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (Symbol *symbol = result.lastVisibleSymbol()) {
            Symbol *def = 0;
            if (resolveTarget && !lastSymbol->isFunction())
                def = findDefinition(symbol);

            link = linkToSymbol(def ? def : symbol);
            link.begin = beginOfToken;
            link.end = endOfToken;
            return link;

        // This would jump to the type of a name
#if 0
        } else if (NamedType *namedType = firstType->asNamedType()) {
            QList<Symbol *> candidates = context.resolve(namedType->name());
            if (!candidates.isEmpty()) {
                Symbol *s = candidates.takeFirst();
                openCppEditorAt(s->fileName(), s->line(), s->column());
            }
#endif
        }
    } else {
        // Handle macro uses
        const Document::MacroUse *use = doc->findMacroUseAt(endOfToken - 1);
        if (use) {
            const Macro &macro = use->macro();
            link.fileName = macro.fileName();
            link.line = macro.line();
            link.begin = use->begin();
            link.end = use->end();
            return link;
        }
    }

    return link;
}

void CPPEditor::jumpToDefinition()
{
    openLink(findLinkAt(textCursor()));
}

Symbol *CPPEditor::findDefinition(Symbol *symbol)
{
    if (symbol->isFunction())
        return 0; // symbol is a function definition.

    Function *funTy = symbol->type()->asFunctionType();
    if (! funTy)
        return 0; // symbol does not have function type.

    const Name *name = symbol->name();
    if (! name)
        return 0; // skip anonymous functions!

    if (const QualifiedNameId *q = name->asQualifiedNameId())
        name = q->unqualifiedNameId();

    // map from file names to function definitions.
    QMap<QString, QList<Function *> > functionDefinitions;

    // find function definitions.
    FindFunctionDefinitions findFunctionDefinitions;

    // save the current snapshot
    const Snapshot snapshot = m_modelManager->snapshot();

    foreach (Document::Ptr doc, snapshot) {
        if (Scope *globals = doc->globalSymbols()) {
            QList<Function *> *localFunctionDefinitions =
                    &functionDefinitions[doc->fileName()];

            findFunctionDefinitions(name, globals,
                                    localFunctionDefinitions);
        }
    }

    // a dummy document.
    Document::Ptr expressionDocument = Document::create("<empty>");

    QMapIterator<QString, QList<Function *> > it(functionDefinitions);
    while (it.hasNext()) {
        it.next();

        // get the instance of the document.
        Document::Ptr thisDocument = snapshot.document(it.key());

        foreach (Function *f, it.value()) {
            // create a lookup context
            const LookupContext context(f, expressionDocument,
                                        thisDocument, snapshot);

            // search the matching definition for the function declaration `symbol'.
            foreach (Symbol *s, context.resolve(f->name())) {
                if (s == symbol)
                    return f;
            }
        }
    }

    return 0;
}

unsigned CPPEditor::editorRevision() const
{
    return document()->revision();
}

bool CPPEditor::isOutdated() const
{
    if (m_lastSemanticInfo.revision != editorRevision())
        return true;

    return false;
}

SemanticInfo CPPEditor::semanticInfo() const
{
    return m_lastSemanticInfo;
}

bool CPPEditor::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{') ||
        ch == QLatin1Char('}') ||
        ch == QLatin1Char(':') ||
        ch == QLatin1Char('#')) {
        return true;
    }
    return false;
}

QString CPPEditor::insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                       const QChar &la, int *skippedChars) const
{
    MatchingText m;
    return m.insertMatchingBrace(tc, text, la, skippedChars);
}

QString CPPEditor::insertParagraphSeparator(const QTextCursor &tc) const
{
    MatchingText m;
    return m.insertParagraphSeparator(tc);
}


bool CPPEditor::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                             const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (! (MatchingText::shouldInsertMatchingText(cursor) || ch == QLatin1Char('\'') || ch == QLatin1Char('"')))
        return false;
    else if (isInComment(cursor))
        return false;

    return true;
}

bool CPPEditor::isInComment(const QTextCursor &cursor) const
{
    CPlusPlus::TokenUnderCursor tokenUnderCursor;
    const SimpleToken tk = tokenUnderCursor(cursor);

    if (tk.isComment()) {
        const int pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.end()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return true;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return true;
        }

        if (pos < tk.end())
            return true;
    }

    return false;
}

// Indent a code line based on previous
template <class Iterator>
static void indentCPPBlock(const CPPEditor::TabSettings &ts,
                           const QTextBlock &block,
                           const Iterator &programBegin,
                           const Iterator &programEnd,
                           QChar typedChar)
{
    typedef typename SharedTools::Indenter<Iterator> Indenter;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(ts.m_indentSize);
    indenter.setTabSize(ts.m_tabSize);
    indenter.setIndentBraces(ts.m_indentBraces);

    const TextEditor::TextBlockIterator current(block);
    const int indent = indenter.indentForBottomLine(current, programBegin, programEnd, typedChar);
    ts.indentLine(block, indent);
}

static int indentationColumn(const TextEditor::TabSettings &tabSettings,
                             const BackwardsScanner &scanner,
                             int index)
{
    return tabSettings.indentationColumn(scanner.indentationString(index));
}

void CPPEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    QTextCursor tc(block);
    tc.movePosition(QTextCursor::EndOfBlock);

    const TabSettings &ts = tabSettings();

    BackwardsScanner tk(tc, QString(), 400);
    const int tokenCount = tk.startToken();

    if (tokenCount != 0) {
        const SimpleToken firstToken = tk[0];

        if (firstToken.is(T_COLON)) {
            const int previousLineIndent = indentationColumn(ts, tk, -1);
            ts.indentLine(block, previousLineIndent + ts.m_indentSize);
            return;
        } else if ((firstToken.is(T_PUBLIC) || firstToken.is(T_PROTECTED) || firstToken.is(T_PRIVATE) ||
                    firstToken.is(T_Q_SIGNALS) || firstToken.is(T_Q_SLOTS)) &&
                    tk.size() > 1 && tk[1].is(T_COLON)) {
            const int startOfBlock = tk.startOfBlock(0);
            if (startOfBlock != 0) {
                const int indent = indentationColumn(ts, tk, startOfBlock);
                ts.indentLine(block, indent);
                return;
            }
        } else if (firstToken.is(T_CASE) || firstToken.is(T_DEFAULT)) {
            const int startOfBlock = tk.startOfBlock(0);
            if (startOfBlock != 0) {
                const int indent = indentationColumn(ts, tk, startOfBlock);
                ts.indentLine(block, indent);
                return;
            }
            return;
        }
    }

    if ((tokenCount == 0 || tk[0].isNot(T_POUND)) && typedChar.isNull() && (tk[-1].is(T_IDENTIFIER) || tk[-1].is(T_RPAREN))) {
        int tokenIndex = -1;
        if (tk[-1].is(T_RPAREN)) {
            const int matchingBrace = tk.startOfMatchingBrace(0);
            if (matchingBrace != 0 && tk[matchingBrace - 1].is(T_IDENTIFIER)) {
                tokenIndex = matchingBrace - 1;
            }
        }

        const QString spell = tk.text(tokenIndex);
        if (tk[tokenIndex].followsNewline() && (spell.startsWith(QLatin1String("QT_")) ||
                                                spell.startsWith(QLatin1String("Q_")))) {
            const int indent = indentationColumn(ts, tk, tokenIndex);
            ts.indentLine(block, indent);
            return;
        }
    }

    const TextEditor::TextBlockIterator begin(doc->begin());
    const TextEditor::TextBlockIterator end(block.next());
    indentCPPBlock(ts, block, begin, end, typedChar);
}

bool CPPEditor::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_currentRenameSelection != -1) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return BaseTextEditor::event(e);
}

void CPPEditor::performQuickFix(int index)
{
    CPPQuickFixCollector *quickFixCollector = CppPlugin::instance()->quickFixCollector();
    QuickFixOperationPtr op = m_quickFixes.at(index);
    quickFixCollector->perform(op);
    //op->createChangeSet();
    //setChangeSet(op->changeSet());
}

void CPPEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QMenu *menu = new QMenu();

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *mcontext = am->actionContainer(CppEditor::Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    CPPQuickFixCollector *quickFixCollector = CppPlugin::instance()->quickFixCollector();

    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));

    if (! isOutdated()) {
        if (quickFixCollector->startCompletion(editableInterface()) != -1) {
            m_quickFixes = quickFixCollector->quickFixes();

            for (int index = 0; index < m_quickFixes.size(); ++index) {
                QuickFixOperationPtr op = m_quickFixes.at(index);
                QAction *action = menu->addAction(op->description());
                mapper.setMapping(action, index);
                connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
            }

            if (! m_quickFixes.isEmpty())
                menu->addSeparator();
        }
    }

    foreach (QAction *action, contextMenu->actions())
        menu->addAction(action);

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    quickFixCollector->cleanup();
    m_quickFixes.clear();
    delete menu;
}

void CPPEditor::keyPressEvent(QKeyEvent *e)
{
    if (m_currentRenameSelection == -1) {
        TextEditor::BaseTextEditor::keyPressEvent(e);
        return;
    }

    QTextCursor cursor = textCursor();
    const QTextCursor::MoveMode moveMode =
            (e->modifiers() & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        abortRename();
        e->accept();
        return;
    case Qt::Key_Home: {
        // Send home to start of name when within the name and not at the start
        if (cursor.position() > m_currentRenameSelectionBegin.position()
               && cursor.position() <= m_currentRenameSelectionEnd.position()) {
            cursor.setPosition(m_currentRenameSelectionBegin.position(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_End: {
        // Send end to end of name when within the name and not at the end
        if (cursor.position() >= m_currentRenameSelectionBegin.position()
               && cursor.position() < m_currentRenameSelectionEnd.position()) {
            cursor.setPosition(m_currentRenameSelectionEnd.position(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Backspace: {
        if (cursor.position() == m_currentRenameSelectionBegin.position()) {
            // Eat backspace at start of name
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Delete: {
        if (cursor.position() == m_currentRenameSelectionEnd.position()) {
            // Eat delete at end of name
            e->accept();
            return;
        }
        break;
    }
    default: {
        break;
    }
    } // switch

    startRename();

    bool wantEditBlock = (cursor.position() >= m_currentRenameSelectionBegin.position()
            && cursor.position() <= m_currentRenameSelectionEnd.position());

    if (wantEditBlock) {
        // possible change inside rename selection
        if (m_firstRenameChange)
            cursor.beginEditBlock();
        else
            cursor.joinPreviousEditBlock();
        m_firstRenameChange = false;
    }
    TextEditor::BaseTextEditor::keyPressEvent(e);
    if (wantEditBlock)
        cursor.endEditBlock();
    finishRename();
}

QList<int> CPPEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *CPPEditorEditable::duplicate(QWidget *parent)
{
    CPPEditor *newEditor = new CPPEditor(parent);
    newEditor->duplicateFrom(editor());
    CppPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

QString CPPEditorEditable::id() const
{
    return QLatin1String(CppEditor::Constants::CPPEDITOR_ID);
}

bool CPPEditorEditable::open(const QString & fileName)
{
    bool b = TextEditor::BaseTextEditorEditable::open(fileName);
    editor()->setMimeType(Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return b;
}

void CPPEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    CppHighlighter *highlighter = qobject_cast<CppHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_NUMBER)
                   << QLatin1String(TextEditor::Constants::C_STRING)
                   << QLatin1String(TextEditor::Constants::C_TYPE)
                   << QLatin1String(TextEditor::Constants::C_KEYWORD)
                   << QLatin1String(TextEditor::Constants::C_OPERATOR)
                   << QLatin1String(TextEditor::Constants::C_PREPROCESSOR)
                   << QLatin1String(TextEditor::Constants::C_LABEL)
                   << QLatin1String(TextEditor::Constants::C_COMMENT)
                   << QLatin1String(TextEditor::Constants::C_DOXYGEN_COMMENT)
                   << QLatin1String(TextEditor::Constants::C_DOXYGEN_TAG)
                   << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    highlighter->rehighlight();

    m_occurrencesFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES));
    m_occurrencesUnusedFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_UNUSED));
    m_occurrencesUnusedFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_occurrencesUnusedFormat.setUnderlineColor(m_occurrencesUnusedFormat.foreground().color());
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrencesUnusedFormat.setToolTip(tr("Unused variable"));
    m_occurrenceRenameFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_RENAME));

    // only set the background, we do not want to modify foreground properties set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();
}

void CPPEditor::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

CPPEditor::Link CPPEditor::linkToSymbol(CPlusPlus::Symbol *symbol)
{
    const QString fileName = QString::fromUtf8(symbol->fileName(),
                                               symbol->fileNameLength());
    unsigned line = symbol->line();
    unsigned column = symbol->column();

    if (column)
        --column;

    if (symbol->isGenerated())
        column = 0;

    return Link(fileName, line, column);
}

bool CPPEditor::openCppEditorAt(const Link &link)
{
    if (link.fileName.isEmpty())
        return false;

    if (baseTextDocument()->fileName() == link.fileName) {
        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->addCurrentPositionToNavigationHistory();
        gotoLine(link.line, link.column);
        setFocus();
        return true;
    }

    return TextEditor::BaseTextEditor::openEditorAt(link.fileName,
                                                    link.line,
                                                    link.column,
                                                    Constants::CPPEDITOR_ID);
}

void CPPEditor::semanticRehighlight()
{
    m_semanticHighlighter->rehighlight(currentSource());
}

void CPPEditor::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision != editorRevision()) {
        // got outdated semantic info
        semanticRehighlight();
        return;
    }

    m_lastSemanticInfo = semanticInfo;

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    QList<QTextEdit::ExtraSelection> unusedSelections;

    m_renameSelections.clear();

    SemanticInfo::LocalUseIterator it(semanticInfo.localUses);
    while (it.hasNext()) {
        it.next();
        const QList<SemanticInfo::Use> &uses = it.value();

        bool good = false;
        foreach (const SemanticInfo::Use &use, uses) {
            unsigned l = line;
            unsigned c = column + 1; // convertCursorPosition() returns a 0-based column number.
            if (l == use.line && c >= use.column && c <= (use.column + use.length)) {
                good = true;
                break;
            }
        }

        if (uses.size() == 1)
            // it's an unused declaration
            highlightUses(uses, semanticInfo, &unusedSelections);

        else if (good && m_renameSelections.isEmpty())
            highlightUses(uses, semanticInfo, &m_renameSelections);
    }

    setExtraSelections(UnusedSymbolSelection, unusedSelections);
    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
}

SemanticHighlighter::Source CPPEditor::currentSource(bool force)
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    const Snapshot snapshot = m_modelManager->snapshot();
    const QString fileName = file()->fileName();

    QString code;
    if (force || m_lastSemanticInfo.revision != editorRevision())
        code = toPlainText(); // get the source code only when needed.

    const unsigned revision = editorRevision();
    SemanticHighlighter::Source source(snapshot, fileName, code,
                                       line, column, revision);
    source.force = force;
    return source;
}

SemanticHighlighter::SemanticHighlighter(QObject *parent)
        : QThread(parent),
          m_done(false)
{
}

SemanticHighlighter::~SemanticHighlighter()
{
}

void SemanticHighlighter::abort()
{
    QMutexLocker locker(&m_mutex);
    m_done = true;
    m_condition.wakeOne();
}

void SemanticHighlighter::rehighlight(const Source &source)
{
    QMutexLocker locker(&m_mutex);
    m_source = source;
    m_condition.wakeOne();
}

bool SemanticHighlighter::isOutdated()
{
    QMutexLocker locker(&m_mutex);
    const bool outdated = ! m_source.fileName.isEmpty() || m_done;
    return outdated;
}

void SemanticHighlighter::run()
{
    setPriority(QThread::IdlePriority);

    forever {
        m_mutex.lock();

        while (! (m_done || ! m_source.fileName.isEmpty()))
            m_condition.wait(&m_mutex);

        const bool done = m_done;
        const Source source = m_source;
        m_source.clear();

        m_mutex.unlock();

        if (done)
            break;

        const SemanticInfo info = semanticInfo(source);

        if (! isOutdated()) {
            m_mutex.lock();
            m_lastSemanticInfo = info;
            m_mutex.unlock();

            emit changed(info);
        }
    }
}

SemanticInfo SemanticHighlighter::semanticInfo(const Source &source)
{
    m_mutex.lock();
    const int revision = m_lastSemanticInfo.revision;
    m_mutex.unlock();

    Snapshot snapshot;
    Document::Ptr doc;

    if (! source.force && revision == source.revision) {
        m_mutex.lock();
        snapshot = m_lastSemanticInfo.snapshot;
        doc = m_lastSemanticInfo.doc;
        m_mutex.unlock();
    }

    if (! doc) {
        const QByteArray preprocessedCode = source.snapshot.preprocessedCode(source.code, source.fileName);

        snapshot = source.snapshot;
        doc = source.snapshot.documentFromSource(preprocessedCode, source.fileName);
        doc->check();
    }

    TranslationUnit *translationUnit = doc->translationUnit();
    AST *ast = translationUnit->ast();

    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(translationUnit);
    DeclarationAST *currentFunctionDefinition = functionDefinitionUnderCursor(ast, source.line, source.column);

    FindLocalUses useTable(translationUnit);
    useTable(currentFunctionDefinition);

    SemanticInfo semanticInfo;
    semanticInfo.revision = source.revision;
    semanticInfo.snapshot = snapshot;
    semanticInfo.doc = doc;
    semanticInfo.localUses = useTable.localUses;
    semanticInfo.hasQ = useTable.hasQ;
    semanticInfo.hasD = useTable.hasD;

    return semanticInfo;
}
