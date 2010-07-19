/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "cppchecksymbols.h"
#include "cppquickfix.h"
#include "cpplocalsymbols.h"

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
#include <cplusplus/Overview.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/FastPreprocessor.h>

#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppcodeformatter.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
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
    UPDATE_OUTLINE_INTERVAL = 150,
    UPDATE_USES_INTERVAL = 300
};

using namespace CPlusPlus;
using namespace CppEditor::Internal;

static QList<QTextEdit::ExtraSelection> createSelections(QTextDocument *document,
                                                         const QList<CPlusPlus::Document::DiagnosticMessage> &msgs,
                                                         const QTextCharFormat &format)
{
    QList<QTextEdit::ExtraSelection> selections;

    foreach (const Document::DiagnosticMessage &m, msgs) {
        const int pos = document->findBlockByNumber(m.line() - 1).position() + m.column() - 1;
        if (pos < 0)
            continue;

        QTextCursor cursor(document);
        cursor.setPosition(pos);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, m.length());

        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format = format;
        sel.format.setToolTip(m.text());
        selections.append(sel);
    }

    return selections;
}

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

class OverviewProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    OverviewProxyModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent) :
        QSortFilterProxyModel(parent),
        m_sourceModel(sourceModel)
    {
        setSourceModel(m_sourceModel);
    }

    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
    {
        // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
        const QModelIndex sourceIndex = m_sourceModel->index(sourceRow, 0, sourceParent);
        CPlusPlus::Symbol *symbol = m_sourceModel->symbolFromIndex(sourceIndex);
        if (symbol && symbol->isGenerated())
            return false;

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
private:
    CPlusPlus::OverviewModel *m_sourceModel;
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
            name = q->name();

        if (_declarationName->isEqualTo(name))
            _functions->append(function);

        return false;
    }
};


struct CanonicalSymbol
{
    CPPEditor *editor;
    TypeOfExpression typeOfExpression;
    SemanticInfo info;

    CanonicalSymbol(CPPEditor *editor, const SemanticInfo &info)
        : editor(editor), info(info)
    {
        typeOfExpression.init(info.doc, info.snapshot);
    }

    const LookupContext &context() const
    {
        return typeOfExpression.context();
    }

    static inline bool isIdentifierChar(const QChar &ch)
    {
        return ch.isLetterOrNumber() || ch == QLatin1Char('_');
    }

    Scope *getScopeAndExpression(const QTextCursor &cursor, QString *code)
    {
        return getScopeAndExpression(editor, info, cursor, code);
    }

    static Scope *getScopeAndExpression(CPPEditor *editor, const SemanticInfo &info,
                                        const QTextCursor &cursor,
                                        QString *code)
    {
        if (! info.doc)
            return 0;

        QTextCursor tc = cursor;
        int line, col;
        editor->convertPosition(tc.position(), &line, &col);
        ++col; // 1-based line and 1-based column

        QTextDocument *document = editor->document();

        int pos = tc.position();

        if (! isIdentifierChar(document->characterAt(pos)))
            if (! (pos > 0 && isIdentifierChar(document->characterAt(pos - 1))))
                return 0;

        while (isIdentifierChar(document->characterAt(pos)))
            ++pos;
        tc.setPosition(pos);

        ExpressionUnderCursor expressionUnderCursor;
        *code = expressionUnderCursor(tc);
        return info.doc->scopeAt(line, col);
    }

    Symbol *operator()(const QTextCursor &cursor)
    {
        QString code;

        if (Scope *scope = getScopeAndExpression(cursor, &code))
            return operator()(scope, code);

        return 0;
    }

    Symbol *operator()(Scope *scope, const QString &code)
    {
        return canonicalSymbol(scope, code, typeOfExpression);
    }

    static Symbol *canonicalSymbol(Scope *scope, const QString &code, TypeOfExpression &typeOfExpression)
    {
        const QList<LookupItem> results = typeOfExpression(code, scope, TypeOfExpression::Preprocess);

        for (int i = 0; i < results.size(); ++i) { // ### TODO virtual methods and classes.
            const LookupItem &r = results.at(i);

            if (r.declaration())
                return r.declaration();
        }

        return 0;
    }

};

} // end of anonymous namespace

CPPEditorEditable::CPPEditorEditable(CPPEditor *editor)
    : BaseTextEditorEditable(editor)
{
    m_context.add(CppEditor::Constants::C_CPPEDITOR);
    m_context.add(ProjectExplorer::Constants::LANG_CXX);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
}

CPPEditor::CPPEditor(QWidget *parent)
    : TextEditor::BaseTextEditor(parent)
    , m_currentRenameSelection(-1)
    , m_inRename(false)
    , m_inRenameChanged(false)
    , m_firstRenameChange(false)
    , m_objcEnabled(false)
{
    m_initialized = false;
    qRegisterMetaType<CppEditor::Internal::SemanticInfo>("CppEditor::Internal::SemanticInfo");

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

    m_highlightRevision = 0;
    m_nextHighlightBlockNumber = 0;
    connect(&m_highlightWatcher, SIGNAL(resultsReadyAt(int,int)), SLOT(highlightTypeUsages(int,int)));
    connect(&m_highlightWatcher, SIGNAL(finished()), SLOT(finishTypeUsages()));

    m_referencesRevision = 0;
    m_referencesCursorPosition = 0;
    connect(&m_referencesWatcher, SIGNAL(finished()), SLOT(markSymbolsNow()));
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
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    QTreeView *outlineView = new OverviewTreeView;
    outlineView->header()->hide();
    outlineView->setItemsExpandable(false);
    m_outlineCombo->setView(outlineView);
    m_outlineCombo->setMaxVisibleItems(20);

    m_outlineModel = new OverviewModel(this);
    m_proxyModel = new OverviewProxyModel(m_outlineModel, this);
    if (CppPlugin::instance()->sortedOutline())
        m_proxyModel->sort(0, Qt::AscendingOrder);
    else
        m_proxyModel->sort(-1, Qt::AscendingOrder); // don't sort yet, but set column for sortedOutline()
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_outlineCombo->setModel(m_proxyModel);

    m_outlineCombo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_sortAction = new QAction(tr("Sort Alphabetically"), m_outlineCombo);
    m_sortAction->setCheckable(true);
    m_sortAction->setChecked(sortedOutline());
    connect(m_sortAction, SIGNAL(toggled(bool)), CppPlugin::instance(), SLOT(setSortedOutline(bool)));
    m_outlineCombo->addAction(m_sortAction);

    m_updateOutlineTimer = new QTimer(this);
    m_updateOutlineTimer->setSingleShot(true);
    m_updateOutlineTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    connect(m_updateOutlineTimer, SIGNAL(timeout()), this, SLOT(updateOutlineNow()));

    m_updateOutlineIndexTimer = new QTimer(this);
    m_updateOutlineIndexTimer->setSingleShot(true);
    m_updateOutlineIndexTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    connect(m_updateOutlineIndexTimer, SIGNAL(timeout()), this, SLOT(updateOutlineIndexNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setSingleShot(true);
    m_updateUsesTimer->setInterval(UPDATE_USES_INTERVAL);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    connect(m_outlineCombo, SIGNAL(activated(int)), this, SLOT(jumpToOutlineElement(int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateOutlineIndex()));
    connect(m_outlineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateOutlineToolTip()));
    connect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onContentsChanged(int,int,int)));

    connect(file(), SIGNAL(changed()), this, SLOT(updateFileName()));


    // set up the semantic highlighter
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));

    connect(m_semanticHighlighter, SIGNAL(changed(CppEditor::Internal::SemanticInfo)),
            this, SLOT(updateSemanticInfo(CppEditor::Internal::SemanticInfo)));

    QToolBar *toolBar = static_cast<QToolBar*>(editable->toolBar());
    QList<QAction*> actions = toolBar->actions();
    QWidget *w = toolBar->widgetForAction(actions.first());
    static_cast<QHBoxLayout*>(w->layout())->insertWidget(0, m_outlineCombo, 1);
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

CppTools::CppModelManagerInterface *CPPEditor::modelManager() const
{
    return m_modelManager;
}

void CPPEditor::setMimeType(const QString &mt)
{
    BaseTextEditor::setMimeType(mt);
    setObjCEnabled(mt == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE);
}

void CPPEditor::setObjCEnabled(bool onoff)
{
    m_objcEnabled = onoff;
}

bool CPPEditor::isObjCEnabled() const
{ return m_objcEnabled; }

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

    m_updateOutlineTimer->start();
}

const Macro *CPPEditor::findCanonicalMacro(const QTextCursor &cursor, Document::Ptr doc) const
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
    const SemanticInfo info = m_lastSemanticInfo;

    CanonicalSymbol cs(this, info);
    Symbol *canonicalSymbol = cs(textCursor());
    if (canonicalSymbol) {
        m_modelManager->findUsages(canonicalSymbol, cs.context());
    } else if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
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
    const SemanticInfo info = m_lastSemanticInfo;

    CanonicalSymbol cs(this, info);
    if (Symbol *canonicalSymbol = cs(textCursor())) {
        if (canonicalSymbol->identifier() != 0) {
            if (showWarningMessage()) {
                Core::EditorManager::instance()->showEditorInfoBar(QLatin1String("CppEditor.Rename"),
                                                                   tr("This change cannot be undone."),
                                                                   tr("Yes, I know what I am doing."),
                                                                   this, SLOT(hideRenameNotification()));
            }

            m_modelManager->renameUsages(canonicalSymbol, cs.context());
        }
    }
}

void CPPEditor::markSymbolsNow()
{
    if (m_references.isCanceled())
        return;
    else if (m_referencesCursorPosition != position())
        return;
    else if (m_referencesRevision != editorRevision())
        return;

    const SemanticInfo info = m_lastSemanticInfo;
    TranslationUnit *unit = info.doc->translationUnit();
    const QList<int> result = m_references.result();

    QList<QTextEdit::ExtraSelection> selections;

    foreach (int index, result) {
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
}

static QList<int> lazyFindReferences(Scope *scope, QString code, const LookupContext &context)
{
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(context.thisDocument(), context.snapshot(), context.bindings());
    if (Symbol *canonicalSymbol = CanonicalSymbol::canonicalSymbol(scope, code, typeOfExpression))
        return CppTools::CppModelManagerInterface::instance()->references(canonicalSymbol, context);
    return QList<int>();
}

void CPPEditor::markSymbols(const QTextCursor &tc, const SemanticInfo &info)
{
    abortRename();

    if (! info.doc)
        return;

    CanonicalSymbol cs(this, info);
    QString expression;
    if (Scope *scope = cs.getScopeAndExpression(this, info, tc, &expression)) {
        LookupContext context(info.doc, info.snapshot);

        m_references.cancel();
        m_referencesRevision = info.revision;
        m_referencesCursorPosition = position();
        m_references = QtConcurrent::run(&lazyFindReferences, scope, expression, context);
        m_referencesWatcher.setFuture(m_references);
    }
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

void CPPEditor::jumpToOutlineElement(int)
{
    QModelIndex index = m_proxyModel->mapToSource(m_outlineCombo->view()->currentIndex());
    Symbol *symbol = m_outlineModel->symbolFromIndex(index);
    if (! symbol)
        return;

    openCppEditorAt(linkToSymbol(symbol));
}

void CPPEditor::setSortedOutline(bool sort)
{
    if (sort != sortedOutline()) {
        if (sort)
            m_proxyModel->sort(0, Qt::AscendingOrder);
        else
            m_proxyModel->sort(-1, Qt::AscendingOrder);
        bool block = m_sortAction->blockSignals(true);
        m_sortAction->setChecked(m_proxyModel->sortColumn() == 0);
        m_sortAction->blockSignals(block);
        updateOutlineIndexNow();
    }
}

bool CPPEditor::sortedOutline() const
{
    return (m_proxyModel->sortColumn() == 0);
}

void CPPEditor::updateOutlineNow()
{
    const Snapshot snapshot = m_modelManager->snapshot();
    Document::Ptr document = snapshot.document(file()->fileName());

    if (!document)
        return;

    if (document->editorRevision() != editorRevision()) {
        m_updateOutlineTimer->start();
        return;
    }

    m_outlineModel->rebuild(document);

    OverviewTreeView *treeView = static_cast<OverviewTreeView *>(m_outlineCombo->view());
    treeView->sync();
    updateOutlineIndexNow();
}

void CPPEditor::updateOutlineIndex()
{
    m_updateOutlineIndexTimer->start();
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

void CPPEditor::updateOutlineIndexNow()
{
    if (!m_outlineModel->document())
        return;

    if (m_outlineModel->document()->editorRevision() != editorRevision()) {
        m_updateOutlineIndexTimer->start();
        return;
    }

    m_updateOutlineIndexTimer->stop();

    m_outlineModelIndex = QModelIndex(); //invalidate
    QModelIndex comboIndex = outlineModelIndex();


    if (comboIndex.isValid()) {
        bool blocked = m_outlineCombo->blockSignals(true);

        // There is no direct way to select a non-root item
        m_outlineCombo->setRootModelIndex(m_proxyModel->mapFromSource(comboIndex.parent()));
        m_outlineCombo->setCurrentIndex(m_proxyModel->mapFromSource(comboIndex).row());
        m_outlineCombo->setRootModelIndex(QModelIndex());

        updateOutlineToolTip();

        m_outlineCombo->blockSignals(blocked);
    }
}

void CPPEditor::updateOutlineToolTip()
{
    m_outlineCombo->setToolTip(m_outlineCombo->currentText());
}

void CPPEditor::updateUses()
{
    if (editorRevision() != m_highlightRevision)
        m_highlighter.cancel();
    m_updateUsesTimer->start();
}

void CPPEditor::updateUsesNow()
{
    if (m_currentRenameSelection != -1)
        return;

    semanticRehighlight();
}

void CPPEditor::highlightTypeUsages(int from, int to)
{
    if (editorRevision() != m_highlightRevision)
        return; // outdated

    else if (m_highlighter.isCanceled())
        return; // aborted

    CppHighlighter *highlighter = qobject_cast<CppHighlighter*>(baseTextDocument()->syntaxHighlighter());
    Q_ASSERT(highlighter);
    QTextDocument *doc = document();

    if (m_nextHighlightBlockNumber >= doc->blockCount())
        return;

    QMap<int, QVector<SemanticInfo::Use> > chunks = CheckSymbols::chunks(m_highlighter, from, to);
    Q_ASSERT(!chunks.isEmpty());
    QTextBlock b = doc->findBlockByNumber(m_nextHighlightBlockNumber);

    QMapIterator<int, QVector<SemanticInfo::Use> > it(chunks);
    while (b.isValid() && it.hasNext()) {
        it.next();
        const int blockNumber = it.key();
        Q_ASSERT(blockNumber < doc->blockCount());

        while (m_nextHighlightBlockNumber < blockNumber) {
            highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
            b = b.next();
            ++m_nextHighlightBlockNumber;
        }

        QList<QTextLayout::FormatRange> formats;
        foreach (const SemanticInfo::Use &use, it.value()) {
            QTextLayout::FormatRange formatRange;

            switch (use.kind) {
            case SemanticInfo::Use::Type:
                formatRange.format = m_typeFormat;
                break;

            case SemanticInfo::Use::Field:
                formatRange.format = m_fieldFormat;
                break;

            case SemanticInfo::Use::Local:
                formatRange.format = m_localFormat;
                break;

            default:
                continue;
            }

            formatRange.start = use.column - 1;
            formatRange.length = use.length;
            formats.append(formatRange);
        }
        highlighter->setExtraAdditionalFormats(b, formats);
        b = b.next();
        ++m_nextHighlightBlockNumber;
    }
}

void CPPEditor::finishTypeUsages()
{
    if (editorRevision() != m_highlightRevision)
        return; // outdated

    else if (m_highlighter.isCanceled())
        return; // aborted

    CppHighlighter *highlighter = qobject_cast<CppHighlighter*>(baseTextDocument()->syntaxHighlighter());
    Q_ASSERT(highlighter);
    QTextDocument *doc = document();

    if (m_nextHighlightBlockNumber >= doc->blockCount())
        return;

    QTextBlock b = doc->findBlockByNumber(m_nextHighlightBlockNumber);

    while (b.isValid()) {
        highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
        b = b.next();
        ++m_nextHighlightBlockNumber;
    }
}


void CPPEditor::switchDeclarationDefinition()
{
    if (! m_modelManager)
        return;

    const Snapshot snapshot = m_modelManager->snapshot();

    if (Document::Ptr thisDocument = snapshot.document(file()->fileName())) {
        int line = 0, column = 0;
        convertPosition(position(), &line, &column);

        Scope *scope = thisDocument->scopeAt(line, column);
        Symbol *lastVisibleSymbol = thisDocument->lastVisibleSymbolAt(line, column);

        Scope *functionScope = 0;
        if (scope->isFunctionScope())
            functionScope = scope;
        else
            functionScope = scope->enclosingFunctionScope();

        if (! functionScope && lastVisibleSymbol) {
            if (Function *def = lastVisibleSymbol->asFunction())
                functionScope = def->members();
        }

        if (functionScope) {
            LookupContext context(thisDocument, snapshot);

            Function *functionDefinition = functionScope->owner()->asFunction();
            const QList<LookupItem> declarations = context.lookup(functionDefinition->name(), functionDefinition->scope());
            foreach (const LookupItem &r, declarations) {
                Symbol *decl = r.declaration();
                // TODO: check decl.
                openCppEditorAt(linkToSymbol(decl));
                break;
            }

        } else if (lastVisibleSymbol && lastVisibleSymbol->isDeclaration() && lastVisibleSymbol->type()->isFunctionType()) {
            if (Symbol *def = snapshot.findMatchingDefinition(lastVisibleSymbol))
                openCppEditorAt(linkToSymbol(def));
        }
    }
}

static inline LookupItem skipForwardDeclarations(const QList<LookupItem> &resolvedSymbols)
{
    QList<LookupItem> candidates = resolvedSymbols;

    LookupItem result = candidates.first();
    const FullySpecifiedType ty = result.type().simplified();

    if (ty->isForwardClassDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->isObjCForwardClassDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isObjCForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->isObjCForwardProtocolDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isObjCForwardProtocolDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    return result;
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


    int beginOfToken = 0;
    int endOfToken = 0;

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(true);
    const QString blockText = cursor.block().text();
    const QList<Token> tokens = tokenize(blockText, BackwardsScanner::previousBlockState(cursor.block()));

    bool recognizedQtMethod = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        if (((unsigned) column) >= tk.begin() && ((unsigned) column) <= tk.end()) {
            if (i >= 2 && tokens.at(i).is(T_IDENTIFIER) && tokens.at(i - 1).is(T_LPAREN)
                && (tokens.at(i - 2).is(T_SIGNAL) || tokens.at(i - 2).is(T_SLOT))) {

                // token[i] == T_IDENTIFIER
                // token[i + 1] == T_LPAREN
                // token[.....] == ....
                // token[i + n] == T_RPAREN

                if (i + 1 < tokens.size() && tokens.at(i + 1).is(T_LPAREN)) {
                    // skip matched parenthesis
                    int j = i - 1;
                    int depth = 0;

                    for (; j < tokens.size(); ++j) {
                        if (tokens.at(j).is(T_LPAREN))
                            ++depth;

                        else if (tokens.at(j).is(T_RPAREN)) {
                            if (! --depth)
                                break;
                        }
                    }

                    if (j < tokens.size()) {
                        QTextBlock block = cursor.block();

                        beginOfToken = block.position() + tokens.at(i).begin();
                        endOfToken = block.position() + tokens.at(i).end();

                        tc.setPosition(block.position() + tokens.at(j).end());
                        recognizedQtMethod = true;
                    }
                }
            }
            break;
        }
    }

    if (! recognizedQtMethod) {
        const QTextBlock block = tc.block();
        const Token tk = SimpleLexer::tokenAt(block.text(), cursor.positionInBlock(), BackwardsScanner::previousBlockState(block), true);

        beginOfToken = block.position() + tk.begin();
        endOfToken = block.position() + tk.end();

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

        tc.setPosition(endOfToken);
    }

    // Find the last symbol up to the cursor position
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return link;

    // Evaluate the type of the expression under the cursor
    ExpressionUnderCursor expressionUnderCursor;
    const QString expression = expressionUnderCursor(tc);

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    const QList<LookupItem> resolvedSymbols = typeOfExpression(expression, scope, TypeOfExpression::Preprocess);

    if (!resolvedSymbols.isEmpty()) {
        LookupItem result = skipForwardDeclarations(resolvedSymbols);

        foreach (const LookupItem &r, resolvedSymbols) {
            if (Symbol *d = r.declaration()) {
                if (d->isDeclaration() || d->isFunction()) {
                    if (file()->fileName() == QString::fromUtf8(d->fileName(), d->fileNameLength())) {
                        if (unsigned(line) == d->line() && unsigned(column) >= d->column()) { // ### TODO: check the end
                            result = r; // take the symbol under cursor.
                            break;
                        }
                    }
                }
            }
        }

        if (Symbol *symbol = result.declaration()) {
            Symbol *def = 0;

            if (resolveTarget) {
                def = findDefinition(symbol, snapshot);

                if (def == doc->lastVisibleSymbolAt(line, column))
                    def = 0; // jump to declaration then.
            }

            if (symbol->isForwardClassDeclaration()) {
                def = snapshot.findMatchingClassDeclaration(symbol);
            }

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

Symbol *CPPEditor::findDefinition(Symbol *symbol, const Snapshot &snapshot)
{
    if (symbol->isFunction())
        return 0; // symbol is a function definition.

    else if (! symbol->type()->isFunctionType())
        return 0; // not a function declaration

    return snapshot.findMatchingDefinition(symbol);
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

CPlusPlus::OverviewModel *CPPEditor::outlineModel() const
{
    return m_outlineModel;
}

QModelIndex CPPEditor::outlineModelIndex()
{
    if (!m_outlineModelIndex.isValid()) {
        int line = 0, column = 0;
        convertPosition(position(), &line, &column);
        m_outlineModelIndex = indexForPosition(line, column);
        emit outlineModelIndexChanged(m_outlineModelIndex);
    }

    return m_outlineModelIndex;
}

bool CPPEditor::isElectricCharacter(QChar ch) const
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
                                       QChar la, int *skippedChars) const
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

bool CPPEditor::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(), BackwardsScanner::previousBlockState(cursor.block()));

    // XXX Duplicated from CPPEditor::isInComment to avoid tokenizing twice
    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.end()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return false;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return false;
        }

        if (pos < tk.end())
            return false;
    }
    else if (tk.is(T_STRING_LITERAL) || tk.is(T_WIDE_STRING_LITERAL)
        || tk.is(T_CHAR_LITERAL) || tk.is(T_WIDE_CHAR_LITERAL)) {

        const unsigned pos = cursor.selectionEnd() - cursor.block().position();
        if (pos <= tk.end())
            return false;
    }

    return true;
}

bool CPPEditor::isInComment(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(), BackwardsScanner::previousBlockState(cursor.block()));

    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

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

static CppTools::QtStyleCodeFormatter setupCodeFormatter(const TextEditor::TabSettings &ts)
{
    CppTools::QtStyleCodeFormatter codeFormatter;
    codeFormatter.setIndentSize(ts.m_indentSize);
    codeFormatter.setTabSize(ts.m_tabSize);
    if (ts.m_indentBraces && ts.m_doubleIndentBlocks) { // gnu style
        codeFormatter.setIndentSubstatementBraces(true);
        codeFormatter.setIndentSubstatementStatements(true);
        codeFormatter.setIndentDeclarationBraces(false);
        codeFormatter.setIndentDeclarationMembers(true);
    } else if (ts.m_indentBraces) { // whitesmiths style
        codeFormatter.setIndentSubstatementBraces(true);
        codeFormatter.setIndentSubstatementStatements(false);
        codeFormatter.setIndentDeclarationBraces(true);
        codeFormatter.setIndentDeclarationMembers(false);
    } else { // default Qt style
        codeFormatter.setIndentSubstatementBraces(false);
        codeFormatter.setIndentSubstatementStatements(true);
        codeFormatter.setIndentDeclarationBraces(false);
        codeFormatter.setIndentDeclarationMembers(true);
    }
    return codeFormatter;
}

void CPPEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    Q_UNUSED(doc)
    Q_UNUSED(typedChar)

    const TabSettings &ts = tabSettings();
    CppTools::QtStyleCodeFormatter codeFormatter = setupCodeFormatter(ts);

    codeFormatter.updateStateUntil(block);
    const int depth = codeFormatter.indentFor(block);
    ts.indentLine(block, depth);
}

void CPPEditor::indent(QTextDocument *doc, const QTextCursor &cursor, QChar typedChar)
{
    Q_UNUSED(doc)
    Q_UNUSED(typedChar)

    maybeClearSomeExtraSelections(cursor);
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        const TabSettings &ts = tabSettings();
        CppTools::QtStyleCodeFormatter codeFormatter = setupCodeFormatter(ts);
        codeFormatter.updateStateUntil(block);

        QTextCursor tc = textCursor();
        tc.beginEditBlock();
        do {
            ts.indentLine(block, codeFormatter.indentFor(block));
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
        tc.endEditBlock();
    } else {
        indentBlock(doc, cursor.block(), typedChar);
    }
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
    TextEditor::QuickFixOperation::Ptr op = m_quickFixes.at(index);
    op->perform();
}

void CPPEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QMenu *menu = new QMenu;

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *mcontext = am->actionContainer(CppEditor::Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    CppQuickFixCollector *quickFixCollector = CppPlugin::instance()->quickFixCollector();

    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));

    if (! isOutdated()) {
        if (quickFixCollector->startCompletion(editableInterface()) != -1) {
            m_quickFixes = quickFixCollector->quickFixes();

            for (int index = 0; index < m_quickFixes.size(); ++index) {
                TextEditor::QuickFixOperation::Ptr op = m_quickFixes.at(index);
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
        if (cursor.position() == m_currentRenameSelectionBegin.position()
            && !cursor.hasSelection()) {
            // Eat backspace at start of name when there is no selection
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Delete: {
        if (cursor.position() == m_currentRenameSelectionEnd.position()
            && !cursor.hasSelection()) {
            // Eat delete at end of name when there is no selection
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

Core::Context CPPEditorEditable::context() const
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

    m_occurrencesFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES));
    m_occurrencesUnusedFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_UNUSED));
    m_occurrencesUnusedFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_occurrencesUnusedFormat.setUnderlineColor(m_occurrencesUnusedFormat.foreground().color());
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrencesUnusedFormat.setToolTip(tr("Unused variable"));
    m_occurrenceRenameFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_RENAME));
    m_typeFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_TYPE));
    m_localFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_LOCAL));
    m_fieldFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_FIELD));
    m_keywordFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_KEYWORD));

    // only set the background, we do not want to modify foreground properties set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();

    // Clear all additional formats since they may have changed
    QTextBlock b = document()->firstBlock();
    while (b.isValid()) {
        highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
        b = b.next();
    }

    // This also triggers an update of the additional formats
    highlighter->rehighlight();
}

void CPPEditor::setTabSettings(const TextEditor::TabSettings &ts)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());

    TextEditor::BaseTextEditor::setTabSettings(ts);
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
        editorManager->cutForwardNavigationHistory();
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

    const SemanticInfo previousSemanticInfo = m_lastSemanticInfo;
    m_lastSemanticInfo = semanticInfo; // update the semantic info

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

    if (m_lastSemanticInfo.forced || previousSemanticInfo.revision != semanticInfo.revision) {
        QTextCharFormat diagnosticMessageFormat;
        diagnosticMessageFormat.setUnderlineColor(Qt::darkYellow); // ### hardcoded
        diagnosticMessageFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline); // ### hardcoded

        setExtraSelections(UndefinedSymbolSelection, createSelections(document(),
                                                                      semanticInfo.diagnosticMessages,
                                                                      diagnosticMessageFormat));

        m_highlighter.cancel();

        if (semanticInfo.doc) {
            LookupContext context(semanticInfo.doc, semanticInfo.snapshot);
            CheckSymbols::Future f = CheckSymbols::go(semanticInfo.doc, context);
            m_highlighter = f;
            m_highlightRevision = semanticInfo.revision;
            m_nextHighlightBlockNumber = 0;
            m_highlightWatcher.setFuture(m_highlighter);
        }

#if 0 // ### TODO: enable objc semantic highlighting
        setExtraSelections(ObjCSelection, createSelections(document(),
                                                           semanticInfo.objcKeywords,
                                                           m_keywordFormat));
#endif
    }


    setExtraSelections(UnusedSymbolSelection, unusedSelections);

    if (! m_renameSelections.isEmpty())
        setExtraSelections(CodeSemanticsSelection, m_renameSelections); // ###
    else {
        markSymbols(textCursor(), semanticInfo);
    }

    m_lastSemanticInfo.forced = false; // clear the forced flag
}

namespace {

class FindObjCKeywords: public ASTVisitor
{
public:
    FindObjCKeywords(TranslationUnit *unit)
        : ASTVisitor(unit)
    {}

    QList<SemanticInfo::Use> operator()()
    {
        _keywords.clear();
        accept(translationUnit()->ast());
        return _keywords;
    }

    virtual bool visit(ObjCClassDeclarationAST *ast)
    {
        addToken(ast->interface_token);
        addToken(ast->implementation_token);
        addToken(ast->end_token);
        return true;
    }

    virtual bool visit(ObjCClassForwardDeclarationAST *ast)
    { addToken(ast->class_token); return true; }

    virtual bool visit(ObjCProtocolDeclarationAST *ast)
    { addToken(ast->protocol_token); addToken(ast->end_token); return true; }

    virtual bool visit(ObjCProtocolForwardDeclarationAST *ast)
    { addToken(ast->protocol_token); return true; }

    virtual bool visit(ObjCProtocolExpressionAST *ast)
    { addToken(ast->protocol_token); return true; }

    virtual bool visit(ObjCTypeNameAST *) { return true; }

    virtual bool visit(ObjCEncodeExpressionAST *ast)
    { addToken(ast->encode_token); return true; }

    virtual bool visit(ObjCSelectorExpressionAST *ast)
    { addToken(ast->selector_token); return true; }

    virtual bool visit(ObjCVisibilityDeclarationAST *ast)
    { addToken(ast->visibility_token); return true; }

    virtual bool visit(ObjCPropertyAttributeAST *ast)
    {
        const Identifier *attrId = identifier(ast->attribute_identifier_token);
        if (attrId == control()->objcAssignId()
                || attrId == control()->objcCopyId()
                || attrId == control()->objcGetterId()
                || attrId == control()->objcNonatomicId()
                || attrId == control()->objcReadonlyId()
                || attrId == control()->objcReadwriteId()
                || attrId == control()->objcRetainId()
                || attrId == control()->objcSetterId())
            addToken(ast->attribute_identifier_token);
        return true;
    }

    virtual bool visit(ObjCPropertyDeclarationAST *ast)
    { addToken(ast->property_token); return true; }

    virtual bool visit(ObjCSynthesizedPropertiesDeclarationAST *ast)
    { addToken(ast->synthesized_token); return true; }

    virtual bool visit(ObjCDynamicPropertiesDeclarationAST *ast)
    { addToken(ast->dynamic_token); return true; }

    virtual bool visit(ObjCFastEnumerationAST *ast)
    { addToken(ast->for_token); addToken(ast->in_token); return true; }

    virtual bool visit(ObjCSynchronizedStatementAST *ast)
    { addToken(ast->synchronized_token); return true; }

protected:
    void addToken(unsigned token)
    {
        if (token) {
            SemanticInfo::Use use;
            getTokenStartPosition(token, &use.line, &use.column);
            use.length = tokenAt(token).length();
            _keywords.append(use);
        }
    }

private:
    QList<SemanticInfo::Use> _keywords;
};

} // anonymous namespace

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
    QList<Document::DiagnosticMessage> diagnosticMessages;
    QList<SemanticInfo::Use> objcKeywords;

    if (! source.force && revision == source.revision) {
        m_mutex.lock();
        snapshot = m_lastSemanticInfo.snapshot; // ### TODO: use the new snapshot.
        doc = m_lastSemanticInfo.doc;
        diagnosticMessages = m_lastSemanticInfo.diagnosticMessages;
        objcKeywords = m_lastSemanticInfo.objcKeywords;
        m_mutex.unlock();
    }

    if (! doc) {
        snapshot = source.snapshot;
        const QByteArray preprocessedCode = snapshot.preprocessedCode(source.code, source.fileName);

        doc = snapshot.documentFromSource(preprocessedCode, source.fileName);
        doc->check();

#if 0
        if (TranslationUnit *unit = doc->translationUnit()) {
            FindObjCKeywords findObjCKeywords(unit); // ### remove me
            objcKeywords = findObjCKeywords();
        }
#endif
    }

    TranslationUnit *translationUnit = doc->translationUnit();
    AST *ast = translationUnit->ast();

    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(translationUnit);
    DeclarationAST *currentFunctionDefinition = functionDefinitionUnderCursor(ast, source.line, source.column);

    const LocalSymbols useTable(doc, currentFunctionDefinition);

    SemanticInfo semanticInfo;
    semanticInfo.revision = source.revision;
    semanticInfo.snapshot = snapshot;
    semanticInfo.doc = doc;
    semanticInfo.localUses = useTable.uses;
    semanticInfo.hasQ = useTable.hasQ;
    semanticInfo.hasD = useTable.hasD;
    semanticInfo.forced = source.force;
    semanticInfo.diagnosticMessages = diagnosticMessages;
    semanticInfo.objcKeywords = objcKeywords;

    return semanticInfo;
}

QModelIndex CPPEditor::indexForPosition(int line, int column, const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;

    const int rowCount = m_outlineModel->rowCount(rootIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_outlineModel->index(row, 0, rootIndex);
        Symbol *symbol = m_outlineModel->symbolFromIndex(index);
        if (symbol && symbol->line() > unsigned(line))
            break;
        lastIndex = index;
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(line, column, lastIndex);
    }

    return lastIndex;
}

#include "cppeditor.moc"
