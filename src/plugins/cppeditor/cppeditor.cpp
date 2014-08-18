/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditor.h"

#include "cppautocompleter.h"
#include "cppcanonicalsymbol.h"
#include "cppdocumentationcommenthelper.h"
#include "cppeditorconstants.h"
#include "cppeditoroutline.h"
#include "cppeditorplugin.h"
#include "cppfollowsymbolundercursor.h"
#include "cpphighlighter.h"
#include "cpplocalrenaming.h"
#include "cpppreprocessordialog.h"
#include "cppquickfixassistant.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cpphighlightingsupport.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/cpptoolseditorsupport.h>
#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/symbolfinder.h>

#include <projectexplorer/session.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>

#include <utils/qtcassert.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/OverviewModel.h>

#include <QAction>
#include <QFutureWatcher>
#include <QMenu>
#include <QPointer>
#include <QSignalMapper>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

enum {
    UPDATE_USES_INTERVAL = 500,
    UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200
};

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppEditor::Internal;

namespace {

QTimer *newSingleShotTimer(QObject *parent, int msecInterval)
{
    QTimer *timer = new QTimer(parent);
    timer->setSingleShot(true);
    timer->setInterval(msecInterval);
    return timer;
}

} // end of anonymous namespace

namespace CppEditor {
namespace Internal {

CPPEditor::CPPEditor(CPPEditorWidget *editor)
    : BaseTextEditor(editor)
{
    m_context.add(CppEditor::Constants::C_CPPEDITOR);
    m_context.add(ProjectExplorer::Constants::LANG_CXX);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
    setDuplicateSupported(true);
}

Q_GLOBAL_STATIC(CppTools::SymbolFinder, symbolFinder)

class CPPEditorWidgetPrivate
{
public:
    CPPEditorWidgetPrivate(CPPEditorWidget *q);

public:
    CPPEditorWidget *q;

    QPointer<CppTools::CppModelManagerInterface> m_modelManager;

    CPPEditorDocument *m_cppEditorDocument;
    CppEditorOutline *m_cppEditorOutline;

    CppDocumentationCommentHelper m_cppDocumentationCommentHelper;

    QTimer *m_updateUsesTimer;
    QTimer *m_updateFunctionDeclDefLinkTimer;
    QHash<int, QTextCharFormat> m_semanticHighlightFormatMap;

    CppLocalRenaming m_localRenaming;

    CppTools::SemanticInfo m_lastSemanticInfo;
    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;

    QScopedPointer<QFutureWatcher<TextEditor::HighlightingResult> > m_highlightWatcher;
    unsigned m_highlightRevision; // the editor revision that requested the highlight

    QScopedPointer<QFutureWatcher<QList<int> > > m_referencesWatcher;
    unsigned m_referencesRevision;
    int m_referencesCursorPosition;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;

    QScopedPointer<FollowSymbolUnderCursor> m_followSymbolUnderCursor;
    QToolButton *m_preprocessorButton;
};

CPPEditorWidgetPrivate::CPPEditorWidgetPrivate(CPPEditorWidget *q)
    : q(q)
    , m_modelManager(CppModelManagerInterface::instance())
    , m_cppEditorDocument(qobject_cast<CPPEditorDocument *>(q->baseTextDocument()))
    , m_cppEditorOutline(new CppEditorOutline(q))
    , m_cppDocumentationCommentHelper(q)
    , m_localRenaming(q)
    , m_highlightRevision(0)
    , m_referencesRevision(0)
    , m_referencesCursorPosition(0)
    , m_declDefLinkFinder(new FunctionDeclDefLinkFinder(q))
    , m_followSymbolUnderCursor(new FollowSymbolUnderCursor(q))
    , m_preprocessorButton(0)
{
}

CPPEditorWidget::CPPEditorWidget(QWidget *parent)
    : TextEditor::BaseTextEditorWidget(new CPPEditorDocument(), parent)
{
    baseTextDocument()->setIndenter(new CppTools::CppQtStyleIndenter);
    ctor();
}

CPPEditorWidget::CPPEditorWidget(CPPEditorWidget *other)
    : TextEditor::BaseTextEditorWidget(other)
{
    ctor();
}

void CPPEditorWidget::ctor()
{
    d.reset(new CPPEditorWidgetPrivate(this));

    qRegisterMetaType<SemanticInfo>("CppTools::SemanticInfo");

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setAutoCompleter(new CppAutoCompleter);

    if (d->m_modelManager) {
        CppEditorSupport *editorSupport = d->m_modelManager->cppEditorSupport(editor());
        connect(editorSupport, SIGNAL(documentUpdated()),
                this, SLOT(onDocumentUpdated()));
        connect(editorSupport, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
                this, SLOT(updateSemanticInfo(CppTools::SemanticInfo)));
        connect(editorSupport, SIGNAL(highlighterStarted(QFuture<TextEditor::HighlightingResult>*,uint)),
                this, SLOT(highlighterStarted(QFuture<TextEditor::HighlightingResult>*,uint)));
    }

    connect(this, SIGNAL(refactorMarkerClicked(TextEditor::RefactorMarker)),
            this, SLOT(onRefactorMarkerClicked(TextEditor::RefactorMarker)));

    connect(d->m_declDefLinkFinder, SIGNAL(foundLink(QSharedPointer<FunctionDeclDefLink>)),
            this, SLOT(onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink>)));

    connect(baseTextDocument(), SIGNAL(filePathChanged(QString,QString)),
            this, SLOT(onFilePathChanged()));

    connect(&d->m_localRenaming, SIGNAL(finished()),
            this, SLOT(onLocalRenamingFinished()));
    connect(&d->m_localRenaming, SIGNAL(processKeyPressNormally(QKeyEvent*)),
            this, SLOT(onLocalRenamingProcessKeyPressNormally(QKeyEvent*)));
}

CPPEditorWidget::~CPPEditorWidget()
{
    if (d->m_modelManager)
        d->m_modelManager->deleteCppEditorSupport(editor());
}

CPPEditorDocument *CPPEditorWidget::cppEditorDocument() const
{
    return d->m_cppEditorDocument;
}

CppEditorOutline *CPPEditorWidget::outline() const
{
    return d->m_cppEditorOutline;
}

TextEditor::BaseTextEditor *CPPEditorWidget::createEditor()
{
    CPPEditor *editable = new CPPEditor(this);
    createToolBar(editable);
    return editable;
}

void CPPEditorWidget::createToolBar(CPPEditor *editor)
{
    d->m_updateUsesTimer = newSingleShotTimer(this, UPDATE_USES_INTERVAL);
    connect(d->m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    d->m_updateFunctionDeclDefLinkTimer = newSingleShotTimer(this, UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(d->m_updateFunctionDeclDefLinkTimer, SIGNAL(timeout()),
            this, SLOT(updateFunctionDeclDefLinkNow()));

    connect(this, SIGNAL(cursorPositionChanged()),
            d->m_cppEditorOutline, SLOT(updateIndex()));

    // set up slots to document changes
    connect(document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(onContentsChanged(int,int,int)));

    // set up function declaration - definition link
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateFunctionDeclDefLink()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateFunctionDeclDefLink()));

    // set up the semantic highlighter
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));

    d->m_preprocessorButton = new QToolButton(this);
    d->m_preprocessorButton->setText(QLatin1String("#"));
    Core::Command *cmd = Core::ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updatePreprocessorButtonTooltip()));
    updatePreprocessorButtonTooltip();
    connect(d->m_preprocessorButton, SIGNAL(clicked()), this, SLOT(showPreProcessorWidget()));
    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, d->m_preprocessorButton);
    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, d->m_cppEditorOutline->widget());
}

void CPPEditorWidget::paste()
{
    if (d->m_localRenaming.handlePaste())
        return;

    BaseTextEditorWidget::paste();
}

void CPPEditorWidget::cut()
{
    if (d->m_localRenaming.handlePaste())
        return;

    BaseTextEditorWidget::cut();
}

void CPPEditorWidget::selectAll()
{
    if (d->m_localRenaming.handleSelectAll())
        return;

    BaseTextEditorWidget::selectAll();
}

/// \brief Called by \c CppEditorSupport when the document corresponding to the
///        file in this editor is updated.
void CPPEditorWidget::onDocumentUpdated()
{
    d->m_cppEditorOutline->update();
}

const Macro *CPPEditorWidget::findCanonicalMacro(const QTextCursor &cursor, Document::Ptr doc) const
{
    if (!doc)
        return 0;

    int line, col;
    convertPosition(cursor.position(), &line, &col);

    if (const Macro *macro = doc->findMacroDefinitionAt(line)) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return macro;
    } else if (const Document::MacroUse *use = doc->findMacroUseAt(cursor.position())) {
        return &use->macro();
    }

    return 0;
}

void CPPEditorWidget::findUsages()
{
    if (!d->m_modelManager)
        return;

    SemanticInfo info = d->m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        d->m_modelManager->findMacroUsages(*macro);
    } else {
        CanonicalSymbol cs(this, info.doc, info.snapshot);
        Symbol *canonicalSymbol = cs(textCursor());
        if (canonicalSymbol)
            d->m_modelManager->findUsages(canonicalSymbol, cs.context());
    }
}

void CPPEditorWidget::renameUsages(const QString &replacement)
{
    if (!d->m_modelManager)
        return;

    SemanticInfo info = d->m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        d->m_modelManager->renameMacroUsages(*macro, replacement);
    } else {
        CanonicalSymbol cs(this, info.doc, info.snapshot);
        if (Symbol *canonicalSymbol = cs(textCursor()))
            if (canonicalSymbol->identifier() != 0)
                d->m_modelManager->renameUsages(canonicalSymbol, cs.context(), replacement);
    }
}

void CPPEditorWidget::markSymbolsNow()
{
    QTC_ASSERT(d->m_referencesWatcher, return);
    if (!d->m_referencesWatcher->isCanceled()
            && d->m_referencesCursorPosition == position()
            && d->m_referencesRevision == editorRevision()) {
        const SemanticInfo info = d->m_lastSemanticInfo;
        TranslationUnit *unit = info.doc->translationUnit();
        const QList<int> result = d->m_referencesWatcher->result();

        QList<QTextEdit::ExtraSelection> selections;

        foreach (int index, result) {
            unsigned line, column;
            unit->getTokenPosition(index, &line, &column);

            if (column)
                --column;  // adjust the column position.

            const int len = unit->tokenAt(index).utf16chars();

            QTextCursor cursor(document()->findBlockByNumber(line - 1));
            cursor.setPosition(cursor.position() + column);
            cursor.setPosition(cursor.position() + len, QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.format = textCharFormat(TextEditor::C_OCCURRENCES);
            sel.cursor = cursor;
            selections.append(sel);
        }

        setExtraSelections(CodeSemanticsSelection, selections);
    }
    d->m_referencesWatcher.reset();
}

static QList<int> lazyFindReferences(Scope *scope, QString code, Document::Ptr doc,
                                     Snapshot snapshot)
{
    TypeOfExpression typeOfExpression;
    snapshot.insert(doc);
    typeOfExpression.init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    if (Symbol *canonicalSymbol = CanonicalSymbol::canonicalSymbol(scope, code, typeOfExpression))
        return CppModelManagerInterface::instance()->references(canonicalSymbol,
                                                                typeOfExpression.context());
    return QList<int>();
}

void CPPEditorWidget::markSymbols(const QTextCursor &tc, const SemanticInfo &info)
{
    d->m_localRenaming.stop();

    if (!info.doc)
        return;
    const QTextCharFormat &occurrencesFormat = textCharFormat(TextEditor::C_OCCURRENCES);
    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        QList<QTextEdit::ExtraSelection> selections;

        //Macro definition
        if (macro->fileName() == info.doc->fileName()) {
            QTextCursor cursor(document());
            cursor.setPosition(macro->utf16CharOffset());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                macro->nameToQString().size());

            QTextEdit::ExtraSelection sel;
            sel.format = occurrencesFormat;
            sel.cursor = cursor;
            selections.append(sel);
        }

        //Other macro uses
        foreach (const Document::MacroUse &use, info.doc->macroUses()) {
            const Macro &useMacro = use.macro();
            if (useMacro.line() != macro->line()
                    || useMacro.utf16CharOffset() != macro->utf16CharOffset()
                    || useMacro.length() != macro->length()
                    || useMacro.fileName() != macro->fileName())
                continue;

            QTextCursor cursor(document());
            cursor.setPosition(use.utf16charsBegin());
            cursor.setPosition(use.utf16charsEnd(), QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.format = occurrencesFormat;
            sel.cursor = cursor;
            selections.append(sel);
        }

        setExtraSelections(CodeSemanticsSelection, selections);
    } else {
        CanonicalSymbol cs(this, info.doc, info.snapshot);
        QString expression;
        if (Scope *scope = cs.getScopeAndExpression(tc, &expression)) {
            if (d->m_referencesWatcher)
                d->m_referencesWatcher->cancel();
            d->m_referencesWatcher.reset(new QFutureWatcher<QList<int> >);
            connect(d->m_referencesWatcher.data(), SIGNAL(finished()), SLOT(markSymbolsNow()));

            d->m_referencesRevision = info.revision;
            d->m_referencesCursorPosition = position();
            d->m_referencesWatcher->setFuture(
                QtConcurrent::run(&lazyFindReferences, scope, expression, info.doc, info.snapshot));
        } else {
            const QList<QTextEdit::ExtraSelection> selections = extraSelections(CodeSemanticsSelection);

            if (!selections.isEmpty())
                setExtraSelections(CodeSemanticsSelection, QList<QTextEdit::ExtraSelection>());
        }
    }
}

void CPPEditorWidget::renameSymbolUnderCursor()
{
    if (!d->m_modelManager)
        return;

    CppEditorSupport *ces = d->m_modelManager->cppEditorSupport(editor());
    updateSemanticInfo(ces->recalculateSemanticInfo());

    if (!d->m_localRenaming.start()) // Rename local symbol
        renameUsages(); // Rename non-local symbol or macro
}

void CPPEditorWidget::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position)
    Q_UNUSED(charsAdded)

    if (charsRemoved > 0)
        updateUses();
}

void CPPEditorWidget::updatePreprocessorButtonTooltip()
{
    QTC_ASSERT(d->m_preprocessorButton, return);
    Core::Command *cmd = Core::ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    QTC_ASSERT(cmd, return);
    d->m_preprocessorButton->setToolTip(cmd->action()->toolTip());
}

QList<QTextEdit::ExtraSelection> CPPEditorWidget::createSelectionsFromUses(
        const QList<SemanticInfo::Use> &uses)
{
    QList<QTextEdit::ExtraSelection> result;
    const bool isUnused = uses.size() == 1;

    foreach (const SemanticInfo::Use &use, uses) {
        if (use.isInvalid())
            continue;

        QTextEdit::ExtraSelection sel;
        if (isUnused)
            sel.format = textCharFormat(TextEditor::C_OCCURRENCES_UNUSED);
        else
            sel.format = textCharFormat(TextEditor::C_OCCURRENCES);

        const int position = document()->findBlockByNumber(use.line - 1).position() + use.column - 1;
        const int anchor = position + use.length;

        sel.cursor = QTextCursor(document());
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        result.append(sel);
    }

    return result;
}

void CPPEditorWidget::updateUses()
{
    // Block premature semantic info calculation when editor is created.
    if (d->m_modelManager && d->m_modelManager->cppEditorSupport(editor())->initialized())
        d->m_updateUsesTimer->start();
}

void CPPEditorWidget::updateUsesNow()
{
    if (d->m_localRenaming.isActive())
        return;

    semanticRehighlight();
}

void CPPEditorWidget::highlightSymbolUsages(int from, int to)
{
    if (editorRevision() != d->m_highlightRevision)
        return; // outdated

    else if (!d->m_highlightWatcher || d->m_highlightWatcher->isCanceled())
        return; // aborted

    TextEditor::SyntaxHighlighter *highlighter = baseTextDocument()->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);

    TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
                highlighter, d->m_highlightWatcher->future(), from, to, d->m_semanticHighlightFormatMap);
}

void CPPEditorWidget::finishHighlightSymbolUsages()
{
    QTC_ASSERT(d->m_highlightWatcher, return);
    if (!d->m_highlightWatcher->isCanceled()
            && editorRevision() == d->m_highlightRevision
            && !d->m_lastSemanticInfo.doc.isNull()) {
        TextEditor::SyntaxHighlighter *highlighter = baseTextDocument()->syntaxHighlighter();
        QTC_CHECK(highlighter);
        if (highlighter)
            TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(highlighter,
                d->m_highlightWatcher->future());
    }
    d->m_highlightWatcher.reset();
}

void CPPEditorWidget::switchDeclarationDefinition(bool inNextSplit)
{
    if (!d->m_modelManager)
        return;

    if (!d->m_lastSemanticInfo.doc)
        return;

    // Find function declaration or definition under cursor
    Function *functionDefinitionSymbol = 0;
    Symbol *functionDeclarationSymbol = 0;

    ASTPath astPathFinder(d->m_lastSemanticInfo.doc);
    const QList<AST *> astPath = astPathFinder(textCursor());

    for (int i = 0, size = astPath.size(); i < size; ++i) {
        AST *ast = astPath.at(i);
        if (FunctionDefinitionAST *functionDefinitionAST = ast->asFunctionDefinition()) {
            if ((functionDefinitionSymbol = functionDefinitionAST->symbol))
                break; // Function definition found!
        } else if (SimpleDeclarationAST *simpleDeclaration = ast->asSimpleDeclaration()) {
            if (List<Symbol *> *symbols = simpleDeclaration->symbols) {
                if (Symbol *symbol = symbols->value) {
                    if (symbol->isDeclaration() && symbol->type()->isFunctionType()) {
                        functionDeclarationSymbol = symbol;
                        break; // Function declaration found!
                    }
                }
            }
        }
    }

    // Link to function definition/declaration
    CPPEditorWidget::Link symbolLink;
    if (functionDeclarationSymbol) {
        symbolLink = linkToSymbol(symbolFinder()
            ->findMatchingDefinition(functionDeclarationSymbol, d->m_modelManager->snapshot()));
    } else if (functionDefinitionSymbol) {
        const Snapshot snapshot = d->m_modelManager->snapshot();
        LookupContext context(d->m_lastSemanticInfo.doc, snapshot);
        ClassOrNamespace *binding = context.lookupType(functionDefinitionSymbol);
        const QList<LookupItem> declarations = context.lookup(functionDefinitionSymbol->name(),
            functionDefinitionSymbol->enclosingScope());

        QList<Symbol *> best;
        foreach (const LookupItem &r, declarations) {
            if (Symbol *decl = r.declaration()) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->match(functionDefinitionSymbol)) {
                        if (decl != functionDefinitionSymbol && binding == r.binding())
                            best.prepend(decl);
                        else
                            best.append(decl);
                    }
                }
            }
        }

        if (best.isEmpty())
            return;
        symbolLink = linkToSymbol(best.first());
    }

    // Open Editor at link position
    if (symbolLink.hasValidTarget())
        openCppEditorAt(symbolLink, inNextSplit != alwaysOpenLinksInNextSplit());
}

QString CPPEditorWidget::identifierUnderCursor(QTextCursor *macroCursor)
{
    macroCursor->movePosition(QTextCursor::StartOfWord);
    macroCursor->movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return macroCursor->selectedText();
}

CPPEditorWidget::Link CPPEditorWidget::findLinkAt(const QTextCursor &cursor, bool resolveTarget,
                                                  bool inNextSplit)
{
    if (!d->m_modelManager)
        return Link();

    return d->m_followSymbolUnderCursor->findLink(cursor, resolveTarget,
                                                  d->m_modelManager->snapshot(),
                                                  d->m_lastSemanticInfo.doc,
                                                  symbolFinder(),
                                                  inNextSplit);
}

unsigned CPPEditorWidget::editorRevision() const
{
    return document()->revision();
}

bool CPPEditorWidget::isOutdated() const
{
    if (d->m_lastSemanticInfo.revision != editorRevision())
        return true;

    return false;
}

SemanticInfo CPPEditorWidget::semanticInfo() const
{
    return d->m_lastSemanticInfo;
}

bool CPPEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        // handle escape manually if a rename is active
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && d->m_localRenaming.isActive()) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return BaseTextEditorWidget::event(e);
}

void CPPEditorWidget::performQuickFix(int index)
{
    TextEditor::QuickFixOperation::Ptr op = d->m_quickFixes.at(index);
    op->perform();
}

void CPPEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QPointer<QMenu> menu(new QMenu(this));

    Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    QMenu *quickFixMenu = new QMenu(tr("&Refactor"), menu);
    quickFixMenu->addAction(Core::ActionManager::command(
                                Constants::RENAME_SYMBOL_UNDER_CURSOR)->action());

    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));
    if (!isOutdated()) {
        TextEditor::IAssistInterface *interface =
            createAssistInterface(TextEditor::QuickFix, TextEditor::ExplicitlyInvoked);
        if (interface) {
            QScopedPointer<TextEditor::IAssistProcessor> processor(
                        CppEditorPlugin::instance()->quickFixProvider()->createProcessor());
            QScopedPointer<TextEditor::IAssistProposal> proposal(processor->perform(interface));
            if (!proposal.isNull()) {
                TextEditor::BasicProposalItemListModel *model =
                        static_cast<TextEditor::BasicProposalItemListModel *>(proposal->model());
                for (int index = 0; index < model->size(); ++index) {
                    TextEditor::BasicProposalItem *item =
                            static_cast<TextEditor::BasicProposalItem *>(model->proposalItem(index));
                    TextEditor::QuickFixOperation::Ptr op =
                            item->data().value<TextEditor::QuickFixOperation::Ptr>();
                    d->m_quickFixes.append(op);
                    QAction *action = quickFixMenu->addAction(op->description());
                    mapper.setMapping(action, index);
                    connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
                }
                delete model;
            }
        }
    }

    foreach (QAction *action, contextMenu->actions()) {
        menu->addAction(action);
        if (action->objectName() == QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT))
            menu->addMenu(quickFixMenu);
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (!menu)
        return;
    d->m_quickFixes.clear();
    delete menu;
}

void CPPEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (d->m_localRenaming.handleKeyPressEvent(e))
        return;

    if (d->m_cppDocumentationCommentHelper.handleKeyPressEvent(e))
        return;

    TextEditor::BaseTextEditorWidget::keyPressEvent(e);
}

Core::IEditor *CPPEditor::duplicate()
{
    CPPEditorWidget *newEditor = new CPPEditorWidget(
                qobject_cast<CPPEditorWidget *>(editorWidget()));
    CppEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editor();
}

bool CPPEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (!TextEditor::BaseTextEditor::open(errorString, fileName, realFileName))
        return false;
    baseTextDocument()->setMimeType(Core::MimeDatabase::findByFile(QFileInfo(fileName)).type());
    return true;
}

const Utils::CommentDefinition *CPPEditor::commentDefinition() const
{
    return &m_commentDefinition;
}

TextEditor::CompletionAssistProvider *CPPEditor::completionAssistProvider()
{
    return CppModelManagerInterface::instance()->cppEditorSupport(this)->completionAssistProvider();
}

void CPPEditorWidget::applyFontSettings()
{
    const TextEditor::FontSettings &fs = baseTextDocument()->fontSettings();

    d->m_semanticHighlightFormatMap[CppHighlightingSupport::TypeUse] =
            fs.toTextCharFormat(TextEditor::C_TYPE);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::LocalUse] =
            fs.toTextCharFormat(TextEditor::C_LOCAL);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::FieldUse] =
            fs.toTextCharFormat(TextEditor::C_FIELD);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::EnumerationUse] =
            fs.toTextCharFormat(TextEditor::C_ENUMERATION);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::VirtualMethodUse] =
            fs.toTextCharFormat(TextEditor::C_VIRTUAL_METHOD);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::LabelUse] =
            fs.toTextCharFormat(TextEditor::C_LABEL);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::MacroUse] =
            fs.toTextCharFormat(TextEditor::C_PREPROCESSOR);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::FunctionUse] =
            fs.toTextCharFormat(TextEditor::C_FUNCTION);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::PseudoKeywordUse] =
            fs.toTextCharFormat(TextEditor::C_KEYWORD);
    d->m_semanticHighlightFormatMap[CppHighlightingSupport::StringUse] =
            fs.toTextCharFormat(TextEditor::C_STRING);

    // this also makes the document apply font settings
    TextEditor::BaseTextEditorWidget::applyFontSettings();
    semanticRehighlight(true);
}

void CPPEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

void CPPEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

CPPEditorWidget::Link CPPEditorWidget::linkToSymbol(CPlusPlus::Symbol *symbol)
{
    if (!symbol)
        return Link();

    const QString filename = QString::fromUtf8(symbol->fileName(),
                                               symbol->fileNameLength());

    unsigned line = symbol->line();
    unsigned column = symbol->column();

    if (column)
        --column;

    if (symbol->isGenerated())
        column = 0;

    return Link(filename, line, column);
}

bool CPPEditorWidget::openCppEditorAt(const Link &link, bool inNextSplit)
{
    if (!link.hasValidTarget())
        return false;

    Core::EditorManager::OpenEditorFlags flags;
    if (inNextSplit)
        flags |= Core::EditorManager::OpenInOtherSplit;
    return Core::EditorManager::openEditorAt(link.targetFileName,
                                             link.targetLine,
                                             link.targetColumn,
                                             Constants::CPPEDITOR_ID,
                                             flags);
}

void CPPEditorWidget::semanticRehighlight(bool force)
{
    if (d->m_modelManager) {
        const CppEditorSupport::ForceReason forceReason = force
                ? CppEditorSupport::ForceDueEditorRequest
                : CppEditorSupport::NoForce;
        d->m_modelManager->cppEditorSupport(editor())->recalculateSemanticInfoDetached(forceReason);
    }
}

void CPPEditorWidget::highlighterStarted(QFuture<TextEditor::HighlightingResult> *highlighter,
                                         unsigned revision)
{
    d->m_highlightRevision = revision;

    d->m_highlightWatcher.reset(new QFutureWatcher<TextEditor::HighlightingResult>);
    connect(d->m_highlightWatcher.data(), SIGNAL(resultsReadyAt(int,int)),
            SLOT(highlightSymbolUsages(int,int)));
    connect(d->m_highlightWatcher.data(), SIGNAL(finished()),
            SLOT(finishHighlightSymbolUsages()));

    d->m_highlightWatcher->setFuture(QFuture<TextEditor::HighlightingResult>(*highlighter));
}

void CPPEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision != editorRevision()) {
        // got outdated semantic info
        semanticRehighlight();
        return;
    }

    d->m_lastSemanticInfo = semanticInfo; // update the semantic info

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    QList<QTextEdit::ExtraSelection> unusedSelections;
    QList<QTextEdit::ExtraSelection> selections;

    // We can use the semanticInfo's snapshot (and avoid locking), but not its
    // document, since it doesn't contain expanded macros.
    LookupContext context(semanticInfo.snapshot.document(baseTextDocument()->filePath()),
                          semanticInfo.snapshot);

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

        if (uses.size() == 1) {
            if (!CppTools::isOwnershipRAIIType(it.key(), context))
                unusedSelections << createSelectionsFromUses(uses); // unused declaration
        } else if (good && selections.isEmpty()) {
            selections << createSelectionsFromUses(uses);
        }
    }

    setExtraSelections(UnusedSymbolSelection, unusedSelections);

    if (!selections.isEmpty()) {
        setExtraSelections(CodeSemanticsSelection, selections);
        d->m_localRenaming.updateLocalUseSelections(selections);
    }  else {
        markSymbols(textCursor(), semanticInfo);
    }

    d->m_lastSemanticInfo.forced = false; // clear the forced flag

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

TextEditor::IAssistInterface *CPPEditorWidget::createAssistInterface(
    TextEditor::AssistKind kind,
    TextEditor::AssistReason reason) const
{
    if (kind == TextEditor::Completion) {
        CppEditorSupport *ces = CppModelManagerInterface::instance()->cppEditorSupport(editor());
        CppCompletionAssistProvider *cap = ces->completionAssistProvider();
        if (cap) {
            return cap->createAssistInterface(
                            ProjectExplorer::ProjectExplorerPlugin::currentProject(),
                            editor(), document(), cppEditorDocument()->isObjCEnabled(), position(),
                            reason);
        }
    } else if (kind == TextEditor::QuickFix) {
        if (!semanticInfo().doc || isOutdated())
            return 0;
        return new CppQuickFixAssistInterface(const_cast<CPPEditorWidget *>(this), reason);
    } else {
        return BaseTextEditorWidget::createAssistInterface(kind, reason);
    }
    return 0;
}

QSharedPointer<FunctionDeclDefLink> CPPEditorWidget::declDefLink() const
{
    return d->m_declDefLink;
}

void CPPEditorWidget::onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker)
{
    if (marker.data.canConvert<FunctionDeclDefLink::Marker>())
        applyDeclDefLinkChanges(true);
}

void CPPEditorWidget::updateFunctionDeclDefLink()
{
    const int pos = textCursor().selectionStart();

    // if there's already a link, abort it if the cursor is outside or the name changed
    // (adding a prefix is an exception since the user might type a return type)
    if (d->m_declDefLink
            && (pos < d->m_declDefLink->linkSelection.selectionStart()
                || pos > d->m_declDefLink->linkSelection.selectionEnd()
                || !d->m_declDefLink->nameSelection.selectedText().trimmed()
                    .endsWith(d->m_declDefLink->nameInitial))) {
        abortDeclDefLink();
        return;
    }

    // don't start a new scan if there's one active and the cursor is already in the scanned area
    const QTextCursor scannedSelection = d->m_declDefLinkFinder->scannedSelection();
    if (!scannedSelection.isNull()
            && scannedSelection.selectionStart() <= pos
            && scannedSelection.selectionEnd() >= pos) {
        return;
    }

    d->m_updateFunctionDeclDefLinkTimer->start();
}

void CPPEditorWidget::updateFunctionDeclDefLinkNow()
{
    if (Core::EditorManager::currentEditor() != editor())
        return;
    const Snapshot semanticSnapshot = d->m_lastSemanticInfo.snapshot;
    const Document::Ptr semanticDoc = d->m_lastSemanticInfo.doc;
    if (d->m_declDefLink) {
        // update the change marker
        const Utils::ChangeSet changes = d->m_declDefLink->changes(semanticSnapshot);
        if (changes.isEmpty())
            d->m_declDefLink->hideMarker(this);
        else
            d->m_declDefLink->showMarker(this);
        return;
    }
    if (semanticDoc.isNull() || isOutdated())
        return;

    Snapshot snapshot = CppModelManagerInterface::instance()->snapshot();
    snapshot.insert(semanticDoc);

    d->m_declDefLinkFinder->startFindLinkAt(textCursor(), semanticDoc, snapshot);
}

void CPPEditorWidget::onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    d->m_declDefLink = link;
    Core::IDocument *targetDocument
            = Core::DocumentModel::documentForFilePath( d->m_declDefLink->targetFile->fileName());
    if (baseTextDocument() != targetDocument) {
        if (TextEditor::ITextEditorDocument *textEditorDocument = qobject_cast<TextEditor::ITextEditorDocument *>(targetDocument))
            connect(textEditorDocument, SIGNAL(contentsChanged()),
                    this, SLOT(abortDeclDefLink()));
    }

}

void CPPEditorWidget::onFilePathChanged()
{
    QTC_ASSERT(d->m_modelManager, return);
    QByteArray additionalDirectives;
    const QString &filePath = baseTextDocument()->filePath();
    if (!filePath.isEmpty()) {
        const QString &projectFile = ProjectExplorer::SessionManager::value(
                    QLatin1String(Constants::CPP_PREPROCESSOR_PROJECT_PREFIX) + filePath).toString();
        additionalDirectives = ProjectExplorer::SessionManager::value(
                    projectFile + QLatin1Char(',') + filePath).toString().toUtf8();

        QSharedPointer<SnapshotUpdater> updater
                = d->m_modelManager->cppEditorSupport(editor())->snapshotUpdater();
        updater->setProjectPart(d->m_modelManager->projectPartForProjectFile(projectFile));
        updater->setEditorDefines(additionalDirectives);
    }
    d->m_preprocessorButton->setProperty("highlightWidget", !additionalDirectives.trimmed().isEmpty());
    d->m_preprocessorButton->update();
}

void CPPEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!d->m_declDefLink)
        return;
    d->m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

FollowSymbolUnderCursor *CPPEditorWidget::followSymbolUnderCursorDelegate()
{
    return d->m_followSymbolUnderCursor.data();
}

void CPPEditorWidget::abortDeclDefLink()
{
    if (!d->m_declDefLink)
        return;

    Core::IDocument *targetDocument
            = Core::DocumentModel::documentForFilePath(d->m_declDefLink->targetFile->fileName());
    if (baseTextDocument() != targetDocument) {
        if (TextEditor::ITextEditorDocument *textEditorDocument = qobject_cast<TextEditor::ITextEditorDocument *>(targetDocument))
            disconnect(textEditorDocument, SIGNAL(contentsChanged()),
                    this, SLOT(abortDeclDefLink()));
    }

    d->m_declDefLink->hideMarker(this);
    d->m_declDefLink.clear();
}

void CPPEditorWidget::onLocalRenamingFinished()
{
    semanticRehighlight(true);
}

void CPPEditorWidget::onLocalRenamingProcessKeyPressNormally(QKeyEvent *e)
{
    BaseTextEditorWidget::keyPressEvent(e);
}

QTextCharFormat CPPEditorWidget::textCharFormat(TextEditor::TextStyle category)
{
    return baseTextDocument()->fontSettings().toTextCharFormat(category);
}

void CPPEditorWidget::showPreProcessorWidget()
{
    const QString &fileName = editor()->document()->filePath();

    // Check if this editor belongs to a project
    QList<ProjectPart::Ptr> projectParts = d->m_modelManager->projectPart(fileName);
    if (projectParts.isEmpty())
        projectParts = d->m_modelManager->projectPartFromDependencies(fileName);
    if (projectParts.isEmpty())
        projectParts << d->m_modelManager->fallbackProjectPart();

    CppPreProcessorDialog preProcessorDialog(this, baseTextDocument()->filePath(), projectParts);
    if (preProcessorDialog.exec() == QDialog::Accepted) {
        QSharedPointer<SnapshotUpdater> updater
                = d->m_modelManager->cppEditorSupport(editor())->snapshotUpdater();
        const QString &additionals = preProcessorDialog.additionalPreProcessorDirectives();
        updater->setProjectPart(preProcessorDialog.projectPart());
        updater->setEditorDefines(additionals.toUtf8());
        updater->update(d->m_modelManager->workingCopy());

        d->m_preprocessorButton->setProperty("highlightWidget", !additionals.trimmed().isEmpty());
        d->m_preprocessorButton->update();
    }
}

} // namespace Internal
} // namespace CppEditor
