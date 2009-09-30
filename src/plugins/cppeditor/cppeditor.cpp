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

#include <cpptools/cpptoolsplugin.h>

#include <AST.h>
#include <Control.h>
#include <Token.h>
#include <Scope.h>
#include <Symbols.h>
#include <Names.h>
#include <Control.h>
#include <CoreTypes.h>
#include <Literals.h>
#include <PrettyPrinter.h>
#include <Semantic.h>
#include <SymbolVisitor.h>
#include <TranslationUnit.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TokenUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
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

class FindUses: protected ASTVisitor
{
    Scope *_functionScope;

    FindScope findScope;

public:
    FindUses(Control *control)
        : ASTVisitor(control)
    { }

    // local and external uses.
    SemanticInfo::LocalUseMap localUses;
    SemanticInfo::ExternalUseMap externalUses;

    void operator()(FunctionDefinitionAST *ast)
    {
        localUses.clear();
        externalUses.clear();

        if (ast && ast->symbol) {
            _functionScope = ast->symbol->members();
            accept(ast);
        }
    }

protected:
    using ASTVisitor::visit;

    bool findMember(Scope *scope, NameAST *ast, unsigned line, unsigned column)
    {
        if (! (ast && ast->name))
            return false;

        Identifier *id = ast->name->identifier();

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

    virtual bool visit(SimpleNameAST *ast)
    {
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

        Identifier *id = identifier(ast->identifier_token);
        externalUses[id].append(SemanticInfo::Use(line, column, id->size()));

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
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

        Identifier *id = identifier(ast->identifier_token);
        externalUses[id].append(SemanticInfo::Use(line, column, id->size()));

        for (TemplateArgumentListAST *arg = ast->template_arguments; arg; arg = arg->next)
            accept(arg);

        return false;
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        if (! ast->global_scope_token) {
            if (ast->nested_name_specifier) {
                accept(ast->nested_name_specifier->class_or_namespace_name);

                for (NestedNameSpecifierAST *it = ast->nested_name_specifier->next; it; it = it->next) {
                    if (NameAST *class_or_namespace_name = it->class_or_namespace_name) {
                        if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId()) {
                            for (TemplateArgumentListAST *arg = template_id->template_arguments; arg; arg = arg->next)
                                accept(arg);
                        }
                    }
                }
            }

            accept(ast->unqualified_name);
        }

        return false;
    }

    virtual bool visit(PostfixExpressionAST *ast)
    {
        accept(ast->base_expression);
        for (PostfixAST *it = ast->postfix_expressions; it; it = it->next) {
            if (it->asMemberAccess() != 0)
                continue; // skip members
            accept(it);
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
};


class FunctionDefinitionUnderCursor: protected ASTVisitor
{
    unsigned _line;
    unsigned _column;
    FunctionDefinitionAST *_functionDefinition;

public:
    FunctionDefinitionUnderCursor(Control *control)
        : ASTVisitor(control),
          _line(0), _column(0)
    { }

    FunctionDefinitionAST *operator()(AST *ast, unsigned line, unsigned column)
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
            unsigned startLine, startColumn;
            unsigned endLine, endColumn;
            getTokenStartPosition(def->firstToken(), &startLine, &startColumn);
            getTokenEndPosition(def->lastToken() - 1, &endLine, &endColumn);

            if (_line > startLine || (_line == startLine && _column >= startColumn)) {
                if (_line < endLine || (_line == endLine && _column < endColumn)) {
                    _functionDefinition = def;
                    return false;
                }
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
    ProcessDeclarators(Control *control)
            : ASTVisitor(control), _visitFunctionDeclarator(true)
    { }

    QList<DeclaratorIdAST *> operator()(FunctionDefinitionAST *ast)
    {
        _declarators.clear();

        if (ast) {
            if (ast->declarator) {
                _visitFunctionDeclarator = true;
                accept(ast->declarator->postfix_declarators);
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
    Name *_declarationName;
    QList<Function *> *_functions;

public:
    FindFunctionDefinitions()
        : _declarationName(0),
          _functions(0)
    { }

    void operator()(Name *declarationName, Scope *globals,
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
        Name *name = function->name();
        if (QualifiedNameId *q = name->asQualifiedNameId())
            name = q->unqualifiedNameId();

        if (_declarationName->isEqualTo(name))
            _functions->append(function);

        return false;
    }
};

} // end of anonymous namespace

static QualifiedNameId *qualifiedNameIdForSymbol(Symbol *s, const LookupContext &context)
{
    Name *symbolName = s->name();
    if (! symbolName)
        return 0; // nothing to do.

    QVector<Name *> names;

    for (Scope *scope = s->scope(); scope; scope = scope->enclosingScope()) {
        if (scope->isClassScope() || scope->isNamespaceScope()) {
            if (scope->owner() && scope->owner()->name()) {
                Name *ownerName = scope->owner()->name();
                if (QualifiedNameId *q = ownerName->asQualifiedNameId()) {
                    for (unsigned i = 0; i < q->nameCount(); ++i) {
                        names.prepend(q->nameAt(i));
                    }
                } else {
                    names.prepend(ownerName);
                }
            }
        }
    }

    if (QualifiedNameId *q = symbolName->asQualifiedNameId()) {
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
    , m_allowSkippingOfBlockEnd(false)
{
    qRegisterMetaType<SemanticInfo>("SemanticInfo");

    m_semanticHighlighter = new SemanticHighlighter(this);
    m_semanticHighlighter->start();

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);
    baseTextDocument()->setSyntaxHighlighter(new CppHighlighter);

#ifdef WITH_TOKEN_MOVE_POSITION
    new QShortcut(QKeySequence::MoveToPreviousWord, this, SLOT(moveToPreviousToken()),
                  /*ambiguousMember=*/ 0, Qt::WidgetShortcut);

    new QShortcut(QKeySequence::MoveToNextWord, this, SLOT(moveToNextToken()),
                  /*ambiguousMember=*/ 0, Qt::WidgetShortcut);

    new QShortcut(QKeySequence::DeleteStartOfWord, this, SLOT(deleteStartOfToken()),
                  /*ambiguousMember=*/ 0, Qt::WidgetShortcut);

    new QShortcut(QKeySequence::DeleteEndOfWord, this, SLOT(deleteEndOfToken()),
                  /*ambiguousMember=*/ 0, Qt::WidgetShortcut);
#endif

    m_modelManager = ExtensionSystem::PluginManager::instance()
        ->getObject<CppTools::CppModelManagerInterface>();

    if (m_modelManager) {
        connect(m_modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
                this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    }
}

CPPEditor::~CPPEditor()
{
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
    m_inRename = true;
    cursor.beginEditBlock();

    const int offset = cursor.position() - currentRenameSelection.cursor.anchor();

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection &s = m_renameSelections[i];
        int pos = s.cursor.anchor();
        int endPos = s.cursor.position();
        s.cursor.setPosition(s.cursor.anchor() + offset);

        switch (operation) {
        case DeletePreviousChar:
            s.cursor.deletePreviousChar();
            --endPos;
            break;
        case DeleteChar:
            s.cursor.deleteChar();
            --endPos;
            break;
        case InsertText:
            s.cursor.insertText(text);
            endPos += text.length();
            break;
        }

        s.cursor.setPosition(pos);
        s.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    }

    cursor.endEditBlock();
    m_inRename = false;

    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
    setTextCursor(cursor);
}

void CPPEditor::abortRename()
{
    m_currentRenameSelection = -1;
    m_renameSelections.clear();
    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
}

int CPPEditor::previousBlockState(QTextBlock block) const
{
    block = block.previous();
    if (block.isValid()) {
        int state = block.userState();

        if (state != -1)
            return state;
    }
    return 0;
}

QTextCursor CPPEditor::moveToPreviousToken(QTextCursor::MoveMode mode) const
{
    SimpleLexer tokenize;
    QTextCursor c(textCursor());
    QTextBlock block = c.block();
    int column = c.columnNumber();

    for (; block.isValid(); block = block.previous()) {
        const QString textBlock = block.text();
        QList<SimpleToken> tokens = tokenize(textBlock, previousBlockState(block));

        if (! tokens.isEmpty()) {
            tokens.prepend(SimpleToken());

            for (int index = tokens.size() - 1; index != -1; --index) {
                const SimpleToken &tk = tokens.at(index);
                if (tk.position() < column) {
                    c.setPosition(block.position() + tk.position(), mode);
                    return c;
                }
            }
        }

        column = INT_MAX;
    }

    c.movePosition(QTextCursor::Start, mode);
    return c;
}

QTextCursor CPPEditor::moveToNextToken(QTextCursor::MoveMode mode) const
{
    SimpleLexer tokenize;
    QTextCursor c(textCursor());
    QTextBlock block = c.block();
    int column = c.columnNumber();

    for (; block.isValid(); block = block.next()) {
        const QString textBlock = block.text();
        QList<SimpleToken> tokens = tokenize(textBlock, previousBlockState(block));

        if (! tokens.isEmpty()) {
            for (int index = 0; index < tokens.size(); ++index) {
                const SimpleToken &tk = tokens.at(index);
                if (tk.position() > column) {
                    c.setPosition(block.position() + tk.position(), mode);
                    return c;
                }
            }
        }

        column = -1;
    }

    c.movePosition(QTextCursor::End, mode);
    return c;
}

void CPPEditor::moveToPreviousToken()
{
    setTextCursor(moveToPreviousToken(QTextCursor::MoveAnchor));
}

void CPPEditor::moveToNextToken()
{
    setTextCursor(moveToNextToken(QTextCursor::MoveAnchor));
}

void CPPEditor::deleteStartOfToken()
{
    QTextCursor c = moveToPreviousToken(QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void CPPEditor::deleteEndOfToken()
{
    QTextCursor c = moveToNextToken(QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void CPPEditor::onDocumentUpdated(Document::Ptr doc)
{
    if (doc->fileName() != file()->fileName())
        return;

    m_overviewModel->rebuild(doc);
    OverviewTreeView *treeView = static_cast<OverviewTreeView *>(m_methodCombo->view());
    treeView->sync();
    updateMethodBoxIndexNow();
}

void CPPEditor::reformatDocument()
{
    using namespace CPlusPlus;

    QByteArray source = toPlainText().toUtf8();

    Control control;
    StringLiteral *fileId = control.findOrInsertStringLiteral("<file>");
    TranslationUnit unit(&control, fileId);
    unit.setQtMocRunEnabled(true);
    unit.setSource(source.constData(), source.length());
    unit.parse();
    if (! unit.ast())
        return;

    std::ostringstream s;

    TranslationUnitAST *ast = unit.ast()->asTranslationUnit();
    PrettyPrinter pp(&control, s);
    pp(ast, source);

    const std::string str = s.str();
    QTextCursor c = textCursor();
    c.setPosition(0);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.insertText(QString::fromUtf8(str.c_str(), str.length()));
}

CPlusPlus::Symbol *CPPEditor::findCanonicalSymbol(const QTextCursor &cursor,
                                                  Document::Ptr doc,
                                                  const Snapshot &snapshot) const
{
    QTextCursor tc = cursor;
    int line, col;
    convertPosition(tc.position(), &line, &col);
    ++col;

    tc.movePosition(QTextCursor::EndOfWord);

    ExpressionUnderCursor expressionUnderCursor;
    const QString code = expressionUnderCursor(tc);
    // qDebug() << "code:" << code;

    const QString fileName = const_cast<CPPEditor *>(this)->file()->fileName();

    TypeOfExpression typeOfExpression;
    typeOfExpression.setSnapshot(snapshot);

    Symbol *lastVisibleSymbol = doc->findSymbolAt(line, col);

    const QList<TypeOfExpression::Result> results = typeOfExpression(code, doc,
                                                                     lastVisibleSymbol,
                                                                     TypeOfExpression::Preprocess);

    Symbol *canonicalSymbol = LookupContext::canonicalSymbol(results);
    return canonicalSymbol;
}

void CPPEditor::findReferences()
{
    m_currentRenameSelection = -1;

    QList<QTextEdit::ExtraSelection> selections;

    SemanticInfo info = m_lastSemanticInfo;

    if (info.doc) {
        if (Symbol *canonicalSymbol = findCanonicalSymbol(textCursor(), info.doc, info.snapshot)) {
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

            setExtraSelections(CodeSemanticsSelection, selections);

            m_modelManager->findReferences(canonicalSymbol);
        }
    }
}

void CPPEditor::renameSymbolUnderCursor()
{
    updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QTextCursor c = textCursor();
    m_currentRenameSelection = -1;

    m_renameSelections = extraSelections(CodeSemanticsSelection);
    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection s = m_renameSelections.at(i);
        if (c.position() >= s.cursor.anchor()
                && c.position() <= s.cursor.position()) {
            m_currentRenameSelection = i;
            m_renameSelections[i].format = m_occurrenceRenameFormat;
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);
            break;
        }
    }

    if (m_renameSelections.isEmpty())
        findReferences();
}

void CPPEditor::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position)
    Q_UNUSED(charsAdded)

    if (m_currentRenameSelection == -1)
        return;

    if (!m_inRename)
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
    m_updateMethodBoxTimer->start(UPDATE_METHOD_BOX_INTERVAL);
}

void CPPEditor::highlightUses(const QList<SemanticInfo::Use> &uses,
                              QList<QTextEdit::ExtraSelection> *selections)
{
    bool isUnused = false;
    if (uses.size() == 1) {
        isUnused = true;
        return; // ###
    }

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

        selections->append(sel);
    }
}

void CPPEditor::updateMethodBoxIndexNow()
{
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
    m_updateUsesTimer->start(UPDATE_USES_INTERVAL);
}

void CPPEditor::updateUsesNow()
{
    m_updateUsesTimer->stop();

    if (m_currentRenameSelection != -1)
        return;

    semanticRehighlight();
}

static bool isCompatible(Name *name, Name *otherName)
{
    if (NameId *nameId = name->asNameId()) {
        if (TemplateNameId *otherTemplId = otherName->asTemplateNameId())
            return nameId->identifier()->isEqualTo(otherTemplId->identifier());
    } else if (TemplateNameId *templId = name->asTemplateNameId()) {
        if (NameId *otherNameId = otherName->asNameId())
            return templId->identifier()->isEqualTo(otherNameId->identifier());
    }

    return name->isEqualTo(otherName);
}

static bool isCompatible(Function *definition, Symbol *declaration, QualifiedNameId *declarationName)
{
    Function *declTy = declaration->type()->asFunctionType();
    if (! declTy)
        return false;

    Name *definitionName = definition->name();
    if (QualifiedNameId *q = definitionName->asQualifiedNameId()) {
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
            Name *n = q->nameAt(q->nameCount() - i - 1);
            Name *m = declarationName->nameAt(declarationName->nameCount() - i - 1);
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

    Document::Ptr doc = snapshot.value(file()->fileName());
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
        QList<TypeOfExpression::Result> resolvedSymbols = typeOfExpression(QString(), doc, lastSymbol);
        const LookupContext &context = typeOfExpression.lookupContext();

        QualifiedNameId *q = qualifiedNameIdForSymbol(f, context);
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
    Document::Ptr doc = snapshot.value(file()->fileName());
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

    // Handle include directives
    if (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)) {
        const unsigned lineno = cursor.blockNumber() + 1;
        foreach (const Document::Include &incl, doc->includes()) {
            if (incl.line() == lineno && incl.resolved()) {
                link.fileName = incl.fileName();
                link.pos = cursor.block().position() + tk.position() + 1;
                link.length = tk.length() - 2;
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

    const int nameStart = tk.position();
    const int nameLength = tk.length();
    const int endOfName = block.position() + nameStart + nameLength;

    const QString name = block.text().mid(nameStart, nameLength);
    tc.setPosition(endOfName);

    // Evaluate the type of the expression under the cursor
    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);
    TypeOfExpression typeOfExpression;
    typeOfExpression.setSnapshot(snapshot);
    QList<TypeOfExpression::Result> resolvedSymbols =
            typeOfExpression(expression, doc, lastSymbol);

    if (!resolvedSymbols.isEmpty()) {
        TypeOfExpression::Result result = resolvedSymbols.first();

        if (result.first->isForwardClassDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                TypeOfExpression::Result r = resolvedSymbols.takeFirst();

                if (! r.first->isForwardClassDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (result.first->isObjCForwardClassDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                TypeOfExpression::Result r = resolvedSymbols.takeFirst();

                if (! r.first->isObjCForwardClassDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (result.first->isObjCForwardProtocolDeclarationType()) {
            while (! resolvedSymbols.isEmpty()) {
                TypeOfExpression::Result r = resolvedSymbols.takeFirst();

                if (! r.first->isObjCForwardProtocolDeclarationType()) {
                    result = r;
                    break;
                }
            }
        }

        if (Symbol *symbol = result.second) {
            Symbol *def = 0;
            if (resolveTarget && !lastSymbol->isFunction())
                def = findDefinition(symbol);

            link = linkToSymbol(def ? def : symbol);
            link.pos = block.position() + nameStart;
            link.length = nameLength;
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
        foreach (const Document::MacroUse use, doc->macroUses()) {
            if (use.contains(endOfName - 1)) {
                const Macro &macro = use.macro();
                link.fileName = macro.fileName();
                link.line = macro.line();
                link.pos = use.begin();
                link.length = use.end() - use.begin();
                return link;
            }
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

    Name *name = symbol->name();
    if (! name)
        return 0; // skip anonymous functions!

    if (QualifiedNameId *q = name->asQualifiedNameId())
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
        Document::Ptr thisDocument = snapshot.value(it.key());

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


static void countBracket(QChar open, QChar close, QChar c, int *errors, int *stillopen)
{
    if (c == open)
        ++*stillopen;
    else if (c == close)
        --*stillopen;

    if (*stillopen < 0) {
        *errors += -1 * (*stillopen);
        *stillopen = 0;
    }
}

void countBrackets(QTextCursor cursor, int from, int end, QChar open, QChar close, int *errors, int *stillopen)
{
    cursor.setPosition(from);
    QTextBlock block = cursor.block();
    while (block.isValid() && block.position() < end) {
        TextEditor::Parentheses parenList = TextEditor::TextEditDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextEditor::TextEditDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                TextEditor::Parenthesis paren = parenList.at(i);
                int position = block.position() + paren.pos;
                if (position < from || position >= end)
                    continue;
                countBracket(open, close, paren.chr, errors, stillopen);
            }
        }
        block = block.next();
    }
}

QString CPPEditor::autoComplete(QTextCursor &cursor, const QString &textToInsert) const
{
    const bool checkBlockEnd = m_allowSkippingOfBlockEnd;
    m_allowSkippingOfBlockEnd = false; // consume blockEnd.

    if (!contextAllowsAutoParentheses(cursor, textToInsert))
        return QString();

    QString text = textToInsert;
    const QChar lookAhead = characterAt(cursor.selectionEnd());

    QChar character = textToInsert.at(0);
    QString parentheses = QLatin1String("()");
    QString brackets = QLatin1String("[]");
    if (parentheses.contains(character) || brackets.contains(character)) {
        QTextCursor tmp= cursor;
        TextEditor::TextBlockUserData::findPreviousBlockOpenParenthesis(&tmp);
        int blockStart = tmp.isNull() ? 0 : tmp.position();
        tmp = cursor;
        TextEditor::TextBlockUserData::findNextBlockClosingParenthesis(&tmp);
        int blockEnd = tmp.isNull() ? (cursor.document()->characterCount()-1) : tmp.position();
        QChar openChar = parentheses.contains(character) ? QLatin1Char('(') : QLatin1Char('[');
        QChar closeChar = parentheses.contains(character) ? QLatin1Char(')') : QLatin1Char(']');

        int errors = 0;
        int stillopen = 0;
        countBrackets(cursor, blockStart, blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsBeforeInsertion = errors + stillopen;
        errors = 0;
        stillopen = 0;
        countBrackets(cursor, blockStart, cursor.position(), openChar, closeChar, &errors, &stillopen);
        countBracket(openChar, closeChar, character, &errors, &stillopen);
        countBrackets(cursor, cursor.position(), blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsAfterInsertion = errors + stillopen;
        if (errorsAfterInsertion < errorsBeforeInsertion)
            return QString(); // insertion fixes parentheses or bracket errors, do not auto complete
    }

    MatchingText matchingText;
    int skippedChars = 0;
    const QString autoText = matchingText.insertMatchingBrace(cursor, text, lookAhead, &skippedChars);

    if (checkBlockEnd && textToInsert.at(0) == QLatin1Char('}')) {
        if (textToInsert.length() > 1)
            qWarning() << "*** handle event compression";

        int startPos = cursor.selectionEnd(), pos = startPos;
        while (characterAt(pos).isSpace())
            ++pos;

        if (characterAt(pos) == QLatin1Char('}'))
            skippedChars += (pos - startPos) + 1;
    }

    if (skippedChars) {
        const int pos = cursor.position();
        cursor.setPosition(pos + skippedChars);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
    }

    return autoText;
}

bool CPPEditor::autoBackspace(QTextCursor &cursor)
{
    m_allowSkippingOfBlockEnd = false;

    int pos = cursor.position();
    QTextCursor c = cursor;
    c.setPosition(pos - 1);

    QChar lookAhead = characterAt(pos);
    QChar lookBehind = characterAt(pos-1);
    QChar lookFurtherBehind = characterAt(pos-2);

    QChar character = lookBehind;
    if (character == QLatin1Char('(') || character == QLatin1Char('[')) {
        QTextCursor tmp = cursor;
        TextEditor::TextBlockUserData::findPreviousBlockOpenParenthesis(&tmp);
        int blockStart = tmp.isNull() ? 0 : tmp.position();
        tmp = cursor;
        TextEditor::TextBlockUserData::findNextBlockClosingParenthesis(&tmp);
        int blockEnd = tmp.isNull() ? (cursor.document()->characterCount()-1) : tmp.position();
        QChar openChar = character;
        QChar closeChar = (character == QLatin1Char('(')) ? QLatin1Char(')') : QLatin1Char(']');

        int errors = 0;
        int stillopen = 0;
        countBrackets(cursor, blockStart, blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsBeforeDeletion = errors + stillopen;
        errors = 0;
        stillopen = 0;
        countBrackets(cursor, blockStart, pos - 1, openChar, closeChar, &errors, &stillopen);
        countBrackets(cursor, pos, blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsAfterDeletion = errors + stillopen;

        if (errorsAfterDeletion < errorsBeforeDeletion)
            return false; // insertion fixes parentheses or bracket errors, do not auto complete
    }

    if    ((lookBehind == QLatin1Char('(') && lookAhead == QLatin1Char(')'))
        || (lookBehind == QLatin1Char('[') && lookAhead == QLatin1Char(']'))
        || (lookBehind == QLatin1Char('"') && lookAhead == QLatin1Char('"')
            && lookFurtherBehind != QLatin1Char('\\'))
        || (lookBehind == QLatin1Char('\'') && lookAhead == QLatin1Char('\'')
            && lookFurtherBehind != QLatin1Char('\\'))) {
        if (! isInComment(c)) {
            cursor.beginEditBlock();
            cursor.deleteChar();
            cursor.deletePreviousChar();
            cursor.endEditBlock();
            return true;
        }
    }
    return false;
}

int CPPEditor::paragraphSeparatorAboutToBeInserted(QTextCursor &cursor)
{
    if (characterAt(cursor.position()-1) != QLatin1Char('{'))
        return 0;

    if (!contextAllowsAutoParentheses(cursor))
        return 0;

    // verify that we indeed do have an extra opening brace in the document
    int braceDepth = document()->lastBlock().userState();
    if (braceDepth >= 0)
        braceDepth >>= 8;
    else
        braceDepth= 0;

    if (braceDepth <= 0)
        return 0; // braces are all balanced or worse, no need to do anything

    // we have an extra brace , let's see if we should close it


    /* verify that the next block is not further intended compared to the current block.
       This covers the following case:

            if (condition) {|
                statement;
    */
    const TabSettings &ts = tabSettings();
    QTextBlock block = cursor.block();
    int indentation = ts.indentationColumn(block.text());
    if (block.next().isValid()
        && ts.indentationColumn(block.next().text()) > indentation)
        return 0;

    int pos = cursor.position();

    MatchingText matchingText;
    const QString textToInsert = matchingText.insertParagraphSeparator(cursor);

    cursor.insertText(textToInsert);
    cursor.setPosition(pos);
    if (ts.m_autoIndent) {
        cursor.insertBlock();
        indent(document(), cursor, QChar::Null);
    } else {
        QString previousBlockText = cursor.block().text();
        cursor.insertBlock();
        cursor.insertText(ts.indentationString(previousBlockText));
    }
    cursor.setPosition(pos);
    m_allowSkippingOfBlockEnd = true;
    return 1;
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

void CPPEditor::indentInsertedText(const QTextCursor &tc)
{
    indent(tc.document(), tc, QChar::Null);
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

    const TextEditor::TextBlockIterator current(block);
    const int indent = indenter.indentForBottomLine(current, programBegin, programEnd, typedChar);
    ts.indentLine(block, indent);
}

void CPPEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    QTextCursor tc(block);
    tc.movePosition(QTextCursor::EndOfBlock);

    BackwardsScanner tk(tc, QString(), 400);
    const int tokenCount = tk.startToken();
    const int indentSize = tabSettings().m_indentSize;

    if (tokenCount != 0) {
        const SimpleToken firstToken = tk[0];

        if (firstToken.is(T_COLON)) {
            const int indent = tk.indentation(-1) +  // indentation of the previous newline
                               indentSize;
            tabSettings().indentLine(block, indent);
            return;
        } else if ((firstToken.is(T_PUBLIC) || firstToken.is(T_PROTECTED) || firstToken.is(T_PRIVATE) ||
                    firstToken.is(T_Q_SIGNALS) || firstToken.is(T_Q_SLOTS)) && tk[1].is(T_COLON)) {
            const int startOfBlock = tk.startOfBlock(0);
            if (startOfBlock != 0) {
                const int indent = tk.indentation(startOfBlock);
                tabSettings().indentLine(block, indent);
                return;
            }
        } else if (firstToken.is(T_CASE) || firstToken.is(T_DEFAULT)) {
            const int startOfBlock = tk.startOfBlock(0);
            if (startOfBlock != 0) {
                const int indent = tk.indentation(startOfBlock);
                tabSettings().indentLine(block, indent);
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
            const int indent = tk.indentation(tokenIndex);
            tabSettings().indentLine(block, indent);
            return;
        }
    }

    const TextEditor::TextBlockIterator begin(doc->begin());
    const TextEditor::TextBlockIterator end(block.next());
    indentCPPBlock(tabSettings(), block, begin, end, typedChar);
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

void CPPEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QMenu *menu = new QMenu();


//    QMenu *menu = createStandardContextMenu();
//
//    // Remove insert unicode control character
//    QAction *lastAction = menu->actions().last();
//    if (lastAction->menu() && QLatin1String(lastAction->menu()->metaObject()->className()) == QLatin1String("QUnicodeControlCharacterMenu"))
//        menu->removeAction(lastAction);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *mcontext = am->actionContainer(CppEditor::Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions())
        menu->addAction(action);

    const QList<QTextEdit::ExtraSelection> selections =
            extraSelections(BaseTextEditor::CodeSemanticsSelection);

    menu->exec(e->globalPos());
    delete menu;
}

void CPPEditor::keyPressEvent(QKeyEvent *e)
{
    if (m_currentRenameSelection == -1) {
        TextEditor::BaseTextEditor::keyPressEvent(e);
        return;
    }

    const QTextEdit::ExtraSelection &currentRenameSelection = m_renameSelections.at(m_currentRenameSelection);
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
        if (cursor.position() > currentRenameSelection.cursor.anchor()
               && cursor.position() <= currentRenameSelection.cursor.position()) {
            cursor.setPosition(currentRenameSelection.cursor.anchor(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_End: {
        // Send end to end of name when within the name and not at the end
        if (cursor.position() >= currentRenameSelection.cursor.anchor()
               && cursor.position() < currentRenameSelection.cursor.position()) {
            cursor.setPosition(currentRenameSelection.cursor.position(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Backspace: {
        if (cursor.position() == currentRenameSelection.cursor.anchor()) {
            // Eat backspace at start of name
            e->accept();
            return;
        } else if (cursor.position() > currentRenameSelection.cursor.anchor()
                && cursor.position() <= currentRenameSelection.cursor.position()) {

            inAllRenameSelections(DeletePreviousChar, currentRenameSelection, cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Delete: {
        if (cursor.position() == currentRenameSelection.cursor.position()) {
            // Eat delete at end of name
            e->accept();
            return;
        } else if (cursor.position() >= currentRenameSelection.cursor.anchor()
                && cursor.position() < currentRenameSelection.cursor.position()) {

            inAllRenameSelections(DeleteChar, currentRenameSelection, cursor);
            e->accept();
            return;
        }
        break;
    }
    default: {
        QString text = e->text();
        if (! text.isEmpty() && text.at(0).isPrint()) {
            if (cursor.position() >= currentRenameSelection.cursor.anchor()
                    && cursor.position() <= currentRenameSelection.cursor.position()) {

                inAllRenameSelections(InsertText, currentRenameSelection, cursor, text);
                e->accept();
                return;
            }
        }
        break;
    }
    }

    TextEditor::BaseTextEditor::keyPressEvent(e);
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

const char *CPPEditorEditable::kind() const
{
    return CppEditor::Constants::CPPEDITOR_KIND;
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
                   << QLatin1String(TextEditor::Constants::C_DOXYGEN_TAG);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    highlighter->rehighlight();

    m_occurrencesFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES));
    m_occurrencesUnusedFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_UNUSED));
    m_occurrenceRenameFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_RENAME));

    // only set the background, we do not want to modify foreground properties set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();
}

void CPPEditor::unCommentSelection()
{
    Core::Utils::unCommentSelection(this);
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
                                                    Constants::C_CPPEDITOR);
}

void CPPEditor::semanticRehighlight()
{
    m_semanticHighlighter->rehighlight(currentSource());
}

void CPPEditor::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision != document()->revision()) {
        // got outdated semantic info
        semanticRehighlight();
        return;
    }

    m_lastSemanticInfo = semanticInfo;

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    QList<QTextEdit::ExtraSelection> selections;

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

        if (uses.size() == 1 || good)
            highlightUses(uses, &selections);
    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

SemanticHighlighter::Source CPPEditor::currentSource()
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    const Snapshot snapshot = m_modelManager->snapshot();
    const QString fileName = file()->fileName();

    QString code;
    if (m_lastSemanticInfo.revision != document()->revision())
        code = toPlainText(); // get the source code only when needed.

    const int revision = document()->revision();
    const SemanticHighlighter::Source source(snapshot, fileName, code,
                                             line, column, revision);
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

    if (revision == source.revision) {
        m_mutex.lock();
        snapshot = m_lastSemanticInfo.snapshot;
        doc = m_lastSemanticInfo.doc;
        m_mutex.unlock();
    }

    if (!doc) {
        const QByteArray preprocessedCode = source.snapshot.preprocessedCode(source.code, source.fileName);

        doc = source.snapshot.documentFromSource(preprocessedCode, source.fileName);
        doc->check();

        snapshot = source.snapshot;
    }

    Control *control = doc->control();
    TranslationUnit *translationUnit = doc->translationUnit();
    AST *ast = translationUnit->ast();

    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(control);
    FunctionDefinitionAST *currentFunctionDefinition = functionDefinitionUnderCursor(ast, source.line, source.column);

    FindUses useTable(control);
    useTable(currentFunctionDefinition);

    SemanticInfo semanticInfo;
    semanticInfo.revision = source.revision;
    semanticInfo.snapshot = snapshot;
    semanticInfo.doc = doc;
    semanticInfo.localUses = useTable.localUses;
    semanticInfo.externalUses = useTable.externalUses;

    return semanticInfo;
}
