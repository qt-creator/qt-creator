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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppplugin.h"
#include "cpphighlighter.h"

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
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TokenUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>
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
#include <QtGui/QTreeView>
#include <QtGui/QSortFilterProxyModel>

#include <sstream>

using namespace CPlusPlus;
using namespace CppEditor::Internal;

enum {
    UPDATE_METHOD_BOX_INTERVAL = 150,
    UPDATE_USES_INTERVAL = 300
};

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

class FindUses: protected ASTVisitor
{
    Scope *_functionScope;

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

public:
    FindUses(Control *control)
        : ASTVisitor(control)
    { }

    struct Use {
        NameAST *name;
        unsigned line;
        unsigned column;
        unsigned length;

        Use(){}

        Use(NameAST *name, unsigned line, unsigned column, unsigned length)
                : name(name), line(line), column(column), length(length) {}
    };

    typedef QHash<Symbol *, QList<Use> > LocalUseMap;
    typedef QHashIterator<Symbol *, QList<Use> > LocalUseIterator;

    typedef QHash<Identifier *, QList<Use> > ExternalUseMap;
    typedef QHashIterator<Identifier *, QList<Use> > ExternalUseIterator;

    // local and external uses.
    LocalUseMap localUses;
    ExternalUseMap externalUses;

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
                    //qDebug() << "*** found member:" << member->line() << member->column() << member->name()->identifier()->chars();
                    localUses[member].append(Use(ast, line, column, id->size()));
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

        FindScope findScope;

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
        externalUses[id].append(Use(ast, line, column, id->size()));

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);

        FindScope findScope;

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
        externalUses[id].append(Use(ast, line, column, id->size()));

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
    QTextCursor _textCursor;
    unsigned _line;
    unsigned _column;
    FunctionDefinitionAST *_functionDefinition;

public:
    FunctionDefinitionUnderCursor(Control *control)
        : ASTVisitor(control)
    { }

    FunctionDefinitionAST *operator()(AST *ast, const QTextCursor &tc)
    {
        _functionDefinition = 0;
        _textCursor = tc;
        _line = tc.blockNumber() + 1;
        _column = tc.columnNumber() + 1;
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
    , m_mouseNavigationEnabled(true)
    , m_showingLink(false)
    , m_currentRenameSelection(-1)
    , m_inRename(false)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);
    baseTextDocument()->setSyntaxHighlighter(new CppHighlighter);

    new QShortcut(QKeySequence(tr("CTRL+SHIFT+r")), this, SLOT(renameInPlace()),
                  /*ambiguousMember=*/ 0, Qt::WidgetShortcut);

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
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));
    connect(m_methodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMethodBoxToolTip()));
    connect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onContentsChanged(int,int,int)));

    connect(file(), SIGNAL(changed()), this, SLOT(updateFileName()));

    QToolBar *toolBar = editable->toolBar();
    QList<QAction*> actions = toolBar->actions();
    QWidget *w = toolBar->widgetForAction(actions.first());
    static_cast<QHBoxLayout*>(w->layout())->insertWidget(0, m_methodCombo, 1);
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

void CPPEditor::renameInPlace()
{
    updateUsesNow();

    QTextCursor c = textCursor();
    m_currentRenameSelection = -1;

    m_renameSelections = extraSelections(CodeSemanticsSelection);
    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection s = m_renameSelections.at(i);
        if (c.position() >= s.cursor.anchor()
                && c.position() <= s.cursor.position()) {
            m_currentRenameSelection = i;
            m_renameSelections[i].format.setBackground(QColor(255, 200, 200));
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);
            break;
        }
    }
}

void CPPEditor::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
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

static void highlightUses(QTextDocument *doc,
                          const QTextCharFormat &format,
                          TranslationUnit *translationUnit,
                          const QList<FindUses::Use> &uses,
                          QList<QTextEdit::ExtraSelection> *selections)
{
    if (uses.size() <= 1)
        return;

    foreach (const FindUses::Use &use, uses) {
        NameAST *name = use.name;
        bool generated = false;

        for (unsigned tk = name->firstToken(), end = name->lastToken(); tk != end; ++tk) {
            if (translationUnit->tokenAt(tk).generated) {
                generated = true;
                break;
            }
        }

        if (! generated) {
            unsigned startLine, startColumn;
            unsigned endLine, endColumn;

            unsigned identifier_token = name->firstToken();

            translationUnit->getTokenStartPosition(identifier_token, &startLine, &startColumn);
            translationUnit->getTokenEndPosition(identifier_token, &endLine, &endColumn);

            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(doc);
            sel.cursor.setPosition(doc->findBlockByNumber(startLine - 1).position() + startColumn - 1);
            sel.cursor.setPosition(doc->findBlockByLineNumber(endLine - 1).position() + endColumn - 1,
                                   QTextCursor::KeepAnchor);
            sel.format = format;

            selections->append(sel);
        }
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

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    const Snapshot snapshot = m_modelManager->snapshot();
    const QByteArray preprocessedCode = snapshot.preprocessedCode(toPlainText(), file()->fileName());
    Document::Ptr doc = snapshot.documentFromSource(preprocessedCode, file()->fileName());
    doc->check();
    Control *control = doc->control();
    TranslationUnit *translationUnit = doc->translationUnit();
    AST *ast = translationUnit->ast();

    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(control);
    FunctionDefinitionAST *currentFunctionDefinition = functionDefinitionUnderCursor(ast, textCursor());

    QTextCharFormat format;
    format.setBackground(QColor(220, 220, 220));

    FindUses useTable(control);
    useTable(currentFunctionDefinition);

    QList<QTextEdit::ExtraSelection> selections;

    FindUses::LocalUseIterator it(useTable.localUses);
    while (it.hasNext()) {
        it.next();
        const QList<FindUses::Use> &uses = it.value();

        bool good = false;
        foreach (const FindUses::Use &use, uses) {
            unsigned l = line;
            unsigned c = column + 1; // convertCursorPosition() returns a 0-based column number.
            if (l == use.line && c >= use.column && c <= (use.column + use.length)) {
                good = true;
                break;
            }
        }

        if (! good)
            continue;

        highlightUses(document(), format, translationUnit, uses, &selections);
        break; // done
    }
#if 0
    FindUses::ExternalUseIterator it2(useTable.externalUses);
    while (it2.hasNext()) {
        it2.next();
        const QList<FindUses::Use> &uses = it2.value();

        bool good = false;
        foreach (const FindUses::Use &use, uses) {
            unsigned l = line;
            unsigned c = column + 1; // convertCursorPosition() returns a 0-based column number.
            if (l == use.line && c >= use.column && c <= (use.column + use.length)) {
                good = true;
                break;
            }
        }

        if (! good)
            continue;

        highlightUses(document(), format, translationUnit, uses, &selections);
        break; // done
    }
#endif
    setExtraSelections(CodeSemanticsSelection, selections);
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
                                      bool lookupDefinition)
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

        if (Symbol *symbol = result.second) {
            Symbol *def = 0;
            if (lookupDefinition && !lastSymbol->isFunction())
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
    openCppEditorAt(findLinkAt(textCursor()));
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

bool CPPEditor::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{') ||
        ch == QLatin1Char('}') ||
        ch == QLatin1Char('#')) {
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
    QMenu *menu = createStandardContextMenu();

    // Remove insert unicode control character
    QAction *lastAction = menu->actions().last();
    if (lastAction->menu() && QLatin1String(lastAction->menu()->metaObject()->className()) == QLatin1String("QUnicodeControlCharacterMenu"))
        menu->removeAction(lastAction);

    Core::ActionContainer *mcontext =
        Core::ICore::instance()->actionManager()->actionContainer(CppEditor::Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions())
        menu->addAction(action);

    QAction *simplifyDeclarations = new QAction(tr("Simplify Declarations"), menu);
    connect(simplifyDeclarations, SIGNAL(triggered()), this, SLOT(simplifyDeclarations()));
    menu->addAction(simplifyDeclarations);

    const QList<QTextEdit::ExtraSelection> selections =
            extraSelections(BaseTextEditor::CodeSemanticsSelection);

    if (! selections.isEmpty()) {
        const QString name = selections.first().cursor.selectedText();
        QAction *renameAction = new QAction(tr("Rename '%1'").arg(name), menu);
        connect(renameAction, SIGNAL(triggered()), this, SLOT(renameInPlace()));
        menu->addAction(renameAction);
    }

    menu->exec(e->globalPos());
    delete menu;
}

void CPPEditor::mouseMoveEvent(QMouseEvent *e)
{
    bool linkFound = false;

    if (m_mouseNavigationEnabled && e->modifiers() & Qt::ControlModifier) {
        // Link emulation behaviour for 'go to definition'
        const QTextCursor cursor = cursorForPosition(e->pos());

        // Check that the mouse was actually on the text somewhere
        bool onText = cursorRect(cursor).right() >= e->x();
        if (!onText) {
            QTextCursor nextPos = cursor;
            nextPos.movePosition(QTextCursor::Right);
            onText = cursorRect(nextPos).right() >= e->x();
        }

        const Link link = findLinkAt(cursor, false);

        if (onText && !link.fileName.isEmpty()) {
            showLink(link);
            linkFound = true;
        }
    }

    if (!linkFound)
        clearLink();

    TextEditor::BaseTextEditor::mouseMoveEvent(e);
}

void CPPEditor::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier && !(e->modifiers() & Qt::ShiftModifier)
        && e->button() == Qt::LeftButton) {

        const QTextCursor cursor = cursorForPosition(e->pos());
        if (openCppEditorAt(findLinkAt(cursor))) {
            clearLink();
            e->accept();
            return;
        }
    }

    TextEditor::BaseTextEditor::mouseReleaseEvent(e);
}

void CPPEditor::leaveEvent(QEvent *e)
{
    clearLink();
    TextEditor::BaseTextEditor::leaveEvent(e);
}

void CPPEditor::keyReleaseEvent(QKeyEvent *e)
{
    // Clear link emulation when Ctrl is released
    if (e->key() == Qt::Key_Control)
        clearLink();

    TextEditor::BaseTextEditor::keyReleaseEvent(e);
}

void CPPEditor::keyPressEvent(QKeyEvent *e)
{
    if (m_currentRenameSelection == -1) {
        TextEditor::BaseTextEditor::keyPressEvent(e);
        return;
    }

    QTextEdit::ExtraSelection currentRenameSelection = m_renameSelections.at(m_currentRenameSelection);

    QTextCursor::MoveMode moveMode = (e->modifiers() & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        abortRename();
        e->accept();
        return;
    case Qt::Key_Home: {
        QTextCursor c = textCursor();
        if (c.position() > currentRenameSelection.cursor.anchor()
               && c.position() <= currentRenameSelection.cursor.position()) {
            c.setPosition(currentRenameSelection.cursor.anchor(), moveMode);
            setTextCursor(c);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_End: {
        QTextCursor c = textCursor();
        if (c.position() >= currentRenameSelection.cursor.anchor()
               && c.position() < currentRenameSelection.cursor.position()) {
            c.setPosition(currentRenameSelection.cursor.position(), moveMode);
            setTextCursor(c);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Backspace: {
        QTextCursor c = textCursor();

        if (c.position() == currentRenameSelection.cursor.anchor()) {
            // Eat
            e->accept();
            return;
        } else if (c.position() > currentRenameSelection.cursor.anchor()
                && c.position() <= currentRenameSelection.cursor.position()) {

            int offset = c.position() - currentRenameSelection.cursor.anchor();

            m_inRename = true;

            c.beginEditBlock();
            for (int i = 0; i < m_renameSelections.size(); ++i) {
                QTextEdit::ExtraSelection &s = m_renameSelections[i];
                int pos = s.cursor.anchor();
                int endPos = s.cursor.position();
                s.cursor.setPosition(s.cursor.anchor() + offset);
                s.cursor.deletePreviousChar();
                s.cursor.setPosition(pos);
                s.cursor.setPosition(endPos - 1, QTextCursor::KeepAnchor);
            }
            c.endEditBlock();

            m_inRename = false;

            setTextCursor(c);
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);

            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Delete: {
        QTextCursor c = textCursor();

        if (c.position() == currentRenameSelection.cursor.position()) {
            // Eat
            e->accept();
            return;
        } else if (c.position() >= currentRenameSelection.cursor.anchor()
                && c.position() < currentRenameSelection.cursor.position()) {

            int offset = c.position() - currentRenameSelection.cursor.anchor();

            m_inRename = true;

            c.beginEditBlock();
            for (int i = 0; i < m_renameSelections.size(); ++i) {
                QTextEdit::ExtraSelection &s = m_renameSelections[i];
                int pos = s.cursor.anchor();
                int endPos = s.cursor.position();
                s.cursor.setPosition(s.cursor.anchor() + offset);
                s.cursor.deleteChar();
                s.cursor.setPosition(pos);
                s.cursor.setPosition(endPos - 1, QTextCursor::KeepAnchor);
            }
            c.endEditBlock();

            m_inRename = false;

            setTextCursor(c);
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);

            e->accept();
            return;
        }
        break;
    }
    default: {
        QString text = e->text();

        if (! text.isEmpty() && text.at(0).isPrint()) {
            QTextCursor c = textCursor();

            if (c.position() >= currentRenameSelection.cursor.anchor()
                    && c.position() <= currentRenameSelection.cursor.position()) {

                int offset = c.position() - currentRenameSelection.cursor.anchor();

                m_inRename = true;

                c.beginEditBlock();
                for (int i = 0; i < m_renameSelections.size(); ++i) {
                    QTextEdit::ExtraSelection &s = m_renameSelections[i];
                    int pos = s.cursor.anchor();
                    int endPos = s.cursor.position();
                    s.cursor.setPosition(s.cursor.anchor() + offset);
                    s.cursor.insertText(text);
                    s.cursor.setPosition(pos);
                    s.cursor.setPosition(endPos + text.length(), QTextCursor::KeepAnchor);
                }
                c.endEditBlock();

                m_inRename = false;

                setTextCursor(c);
                setExtraSelections(CodeSemanticsSelection, m_renameSelections);

                e->accept();
                return;
            }
        }
        break;
    }
    }

    TextEditor::BaseTextEditor::keyPressEvent(e);
}

void CPPEditor::showLink(const Link &link)
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = textCursor();
    sel.cursor.setPosition(link.pos);
    sel.cursor.setPosition(link.pos + link.length, QTextCursor::KeepAnchor);
    sel.format = m_linkFormat;
    sel.format.setFontUnderline(true);
    setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>() << sel);
    viewport()->setCursor(Qt::PointingHandCursor);
    m_showingLink = true;
}

void CPPEditor::clearLink()
{
    if (!m_showingLink)
        return;

    setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>());
    viewport()->setCursor(Qt::IBeamCursor);
    m_showingLink = false;
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

    m_linkFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_LINK));
}

void CPPEditor::setDisplaySettings(const TextEditor::DisplaySettings &ds)
{
    TextEditor::BaseTextEditor::setDisplaySettings(ds);
    m_mouseNavigationEnabled = ds.m_mouseNavigation;
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
