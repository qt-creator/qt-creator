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
#include "cppeditordocument.h"
#include "cppeditoroutline.h"
#include "cppeditorplugin.h"
#include "cppfollowsymbolundercursor.h"
#include "cpphighlighter.h"
#include "cpplocalrenaming.h"
#include "cpppreprocessordialog.h"
#include "cppquickfixassistant.h"
#include "cppuseselectionsupdater.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppworkingcopy.h>
#include <cpptools/symbolfinder.h>

#include <projectexplorer/session.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>

#include <cplusplus/ASTPath.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QMenu>
#include <QPointer>
#include <QSignalMapper>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

enum { UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200 };

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppEditor::Internal;

namespace CppEditor {
namespace Internal {

CppEditor::CppEditor()
{
    m_context.add(Constants::C_CPPEDITOR);
    m_context.add(ProjectExplorer::Constants::LANG_CXX);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
    setDuplicateSupported(true);
    setCommentStyle(Utils::CommentDefinition::CppStyle);
    setCompletionAssistProvider([this] () -> TextEditor::CompletionAssistProvider * {
        if (CppEditorDocument *document = qobject_cast<CppEditorDocument *>(textDocument()))
            return document->completionAssistProvider();
        return 0;
    });
}

Q_GLOBAL_STATIC(CppTools::SymbolFinder, symbolFinder)

class CppEditorWidgetPrivate
{
public:
    CppEditorWidgetPrivate(CppEditorWidget *q);

public:
    QPointer<CppTools::CppModelManagerInterface> m_modelManager;

    CppEditorDocument *m_cppEditorDocument;
    CppEditorOutline *m_cppEditorOutline;

    CppDocumentationCommentHelper m_cppDocumentationCommentHelper;

    QTimer m_updateFunctionDeclDefLinkTimer;

    CppLocalRenaming m_localRenaming;

    CppTools::SemanticInfo m_lastSemanticInfo;
    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;

    CppUseSelectionsUpdater m_useSelectionsUpdater;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;

    QScopedPointer<FollowSymbolUnderCursor> m_followSymbolUnderCursor;
    QToolButton *m_preprocessorButton;
};

CppEditorWidgetPrivate::CppEditorWidgetPrivate(CppEditorWidget *q)
    : m_modelManager(CppModelManagerInterface::instance())
    , m_cppEditorDocument(qobject_cast<CppEditorDocument *>(q->textDocument()))
    , m_cppEditorOutline(new CppEditorOutline(q))
    , m_cppDocumentationCommentHelper(q)
    , m_localRenaming(q)
    , m_useSelectionsUpdater(q)
    , m_declDefLinkFinder(new FunctionDeclDefLinkFinder(q))
    , m_followSymbolUnderCursor(new FollowSymbolUnderCursor(q))
    , m_preprocessorButton(0)
{
}

CppEditorWidget::CppEditorWidget()
   : d(new CppEditorWidgetPrivate(this))
{
    qRegisterMetaType<SemanticInfo>("CppTools::SemanticInfo");
}

void CppEditorWidget::finalizeInitialization()
{
    d->m_cppEditorDocument = qobject_cast<CppEditorDocument *>(textDocument());

    setLanguageSettingsId(CppTools::Constants::CPP_SETTINGS_ID);
    setCodeFoldingSupported(true);
    setMarksVisible(true);
    setParenthesesMatchingEnabled(true);
    setRevisionsVisible(true);

    // function combo box sorting
    connect(CppEditorPlugin::instance(), &CppEditorPlugin::outlineSortingChanged,
            outline(), &CppEditorOutline::setSorted);

    connect(d->m_cppEditorDocument, &CppEditorDocument::codeWarningsUpdated,
            this, &CppEditorWidget::onCodeWarningsUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::ifdefedOutBlocksUpdated,
            this, &CppEditorWidget::onIfdefedOutBlocksUpdated);
    connect(d->m_cppEditorDocument, SIGNAL(cppDocumentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onCppDocumentUpdated()));
    connect(d->m_cppEditorDocument, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
            this, SLOT(updateSemanticInfo(CppTools::SemanticInfo)));

    connect(d->m_declDefLinkFinder, SIGNAL(foundLink(QSharedPointer<FunctionDeclDefLink>)),
            this, SLOT(onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink>)));

    connect(textDocument(), SIGNAL(filePathChanged(QString,QString)),
            this, SLOT(onFilePathChanged()));

    connect(&d->m_useSelectionsUpdater,
            SIGNAL(selectionsForVariableUnderCursorUpdated(QList<QTextEdit::ExtraSelection>)),
            &d->m_localRenaming,
            SLOT(updateSelectionsForVariableUnderCursor(QList<QTextEdit::ExtraSelection>)));

    connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished,
            [this] (CppTools::SemanticInfo::LocalUseMap localUses) {
                QTC_CHECK(isSemanticInfoValidExceptLocalUses());
                d->m_lastSemanticInfo.localUsesUpdated = true;
                d->m_lastSemanticInfo.localUses = localUses;
    });

    connect(&d->m_localRenaming, &CppLocalRenaming::finished, [this] {
        cppEditorDocument()->semanticRehighlight();
    });
    connect(&d->m_localRenaming, &CppLocalRenaming::processKeyPressNormally,
            this, &CppEditorWidget::processKeyNormally);

    connect(this, SIGNAL(cursorPositionChanged()),
            d->m_cppEditorOutline, SLOT(updateIndex()));

    // set up function declaration - definition link
    d->m_updateFunctionDeclDefLinkTimer.setSingleShot(true);
    d->m_updateFunctionDeclDefLinkTimer.setInterval(UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(&d->m_updateFunctionDeclDefLinkTimer, SIGNAL(timeout()),
            this, SLOT(updateFunctionDeclDefLinkNow()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateFunctionDeclDefLink()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateFunctionDeclDefLink()));

    // set up the use highlighitng
    connect(this, &CppEditorWidget::cursorPositionChanged, [this]() {
        if (!d->m_localRenaming.isActive())
            d->m_useSelectionsUpdater.scheduleUpdate();
    });

    // Tool bar creation
    d->m_preprocessorButton = new QToolButton(this);
    d->m_preprocessorButton->setText(QLatin1String("#"));
    Core::Command *cmd = Core::ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updatePreprocessorButtonTooltip()));
    updatePreprocessorButtonTooltip();
    connect(d->m_preprocessorButton, SIGNAL(clicked()), this, SLOT(showPreProcessorWidget()));
    insertExtraToolBarWidget(TextEditor::BaseTextEditorWidget::Left, d->m_preprocessorButton);
    insertExtraToolBarWidget(TextEditor::BaseTextEditorWidget::Left, d->m_cppEditorOutline->widget());
}

void CppEditorWidget::finalizeInitializationAfterDuplication(BaseTextEditorWidget *other)
{
    QTC_ASSERT(other, return);
    CppEditorWidget *cppEditorWidget = qobject_cast<CppEditorWidget *>(other);
    QTC_ASSERT(cppEditorWidget, return);

    if (cppEditorWidget->isSemanticInfoValidExceptLocalUses())
        updateSemanticInfo(cppEditorWidget->semanticInfo());
    d->m_cppEditorOutline->update();
    const ExtraSelectionKind selectionKind = CodeWarningsSelection;
    setExtraSelections(selectionKind, cppEditorWidget->extraSelections(selectionKind));
}

CppEditorWidget::~CppEditorWidget()
{
    // non-inline destructor, see section "Forward Declared Pointers" of QScopedPointer.
}

CppEditorDocument *CppEditorWidget::cppEditorDocument() const
{
    return d->m_cppEditorDocument;
}

CppEditorOutline *CppEditorWidget::outline() const
{
    return d->m_cppEditorOutline;
}

void CppEditorWidget::paste()
{
    if (d->m_localRenaming.handlePaste())
        return;

    BaseTextEditorWidget::paste();
}

void CppEditorWidget::cut()
{
    if (d->m_localRenaming.handleCut())
        return;

    BaseTextEditorWidget::cut();
}

void CppEditorWidget::selectAll()
{
    if (d->m_localRenaming.handleSelectAll())
        return;

    BaseTextEditorWidget::selectAll();
}

void CppEditorWidget::onCppDocumentUpdated()
{
    d->m_cppEditorOutline->update();
}

void CppEditorWidget::onCodeWarningsUpdated(unsigned revision,
                                            const QList<QTextEdit::ExtraSelection> selections)
{
    if (revision != documentRevision())
        return;
    setExtraSelections(BaseTextEditorWidget::CodeWarningsSelection, selections);
}

void CppEditorWidget::onIfdefedOutBlocksUpdated(unsigned revision,
    const QList<TextEditor::BlockRange> ifdefedOutBlocks)
{
    if (revision != documentRevision())
        return;
    setIfdefedOutBlocks(ifdefedOutBlocks);
}

void CppEditorWidget::findUsages()
{
    if (!d->m_modelManager)
        return;

    SemanticInfo info = d->m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = CppTools::findCanonicalMacro(textCursor(), info.doc)) {
        d->m_modelManager->findMacroUsages(*macro);
    } else {
        CanonicalSymbol cs(info.doc, info.snapshot);
        Symbol *canonicalSymbol = cs(textCursor());
        if (canonicalSymbol)
            d->m_modelManager->findUsages(canonicalSymbol, cs.context());
    }
}

void CppEditorWidget::renameUsages(const QString &replacement)
{
    if (!d->m_modelManager)
        return;

    SemanticInfo info = d->m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = CppTools::findCanonicalMacro(textCursor(), info.doc)) {
        d->m_modelManager->renameMacroUsages(*macro, replacement);
    } else {
        CanonicalSymbol cs(info.doc, info.snapshot);
        if (Symbol *canonicalSymbol = cs(textCursor()))
            if (canonicalSymbol->identifier() != 0)
                d->m_modelManager->renameUsages(canonicalSymbol, cs.context(), replacement);
    }
}

void CppEditorWidget::renameSymbolUnderCursor()
{
    updateSemanticInfo(d->m_cppEditorDocument->recalculateSemanticInfo());

    d->m_useSelectionsUpdater.abortSchedule();

    // Trigger once the use selections updater is finished and thus has updated
    // the use selections for the local renaming
    QSharedPointer<QMetaObject::Connection> connection(new QMetaObject::Connection);
    *connection.data() = connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished,
        [this, connection] () {
            QObject::disconnect(*connection);
            if (!d->m_localRenaming.start()) // Rename local symbol
                renameUsages(); // Rename non-local symbol or macro
        });

    d->m_useSelectionsUpdater.update();
}

void CppEditorWidget::updatePreprocessorButtonTooltip()
{
    QTC_ASSERT(d->m_preprocessorButton, return);
    Core::Command *cmd = Core::ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    QTC_ASSERT(cmd, return);
    d->m_preprocessorButton->setToolTip(cmd->action()->toolTip());
}

void CppEditorWidget::switchDeclarationDefinition(bool inNextSplit)
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
    CppEditorWidget::Link symbolLink;
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

CppEditorWidget::Link CppEditorWidget::findLinkAt(const QTextCursor &cursor, bool resolveTarget,
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

unsigned CppEditorWidget::documentRevision() const
{
    return document()->revision();
}

bool CppEditorWidget::isSemanticInfoValidExceptLocalUses() const
{
    return d->m_lastSemanticInfo.doc && d->m_lastSemanticInfo.revision == documentRevision();
}

bool CppEditorWidget::isSemanticInfoValid() const
{
    return isSemanticInfoValidExceptLocalUses() && d->m_lastSemanticInfo.localUsesUpdated;
}

SemanticInfo CppEditorWidget::semanticInfo() const
{
    return d->m_lastSemanticInfo;
}

bool CppEditorWidget::event(QEvent *e)
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

void CppEditorWidget::performQuickFix(int index)
{
    TextEditor::QuickFixOperation::Ptr op = d->m_quickFixes.at(index);
    op->perform();
}

void CppEditorWidget::processKeyNormally(QKeyEvent *e)
{
    BaseTextEditorWidget::keyPressEvent(e);
}

void CppEditorWidget::contextMenuEvent(QContextMenuEvent *e)
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
    if (isSemanticInfoValid()) {
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

void CppEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (d->m_localRenaming.handleKeyPressEvent(e))
        return;

    if (d->m_cppDocumentationCommentHelper.handleKeyPressEvent(e))
        return;

    TextEditor::BaseTextEditorWidget::keyPressEvent(e);
}

bool CppEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (!TextEditor::BaseTextEditor::open(errorString, fileName, realFileName))
        return false;
    textDocument()->setMimeType(Core::MimeDatabase::findByFile(QFileInfo(fileName)).type());
    return true;
}

void CppEditorWidget::applyFontSettings()
{
    // This also makes the document apply font settings
    TextEditor::BaseTextEditorWidget::applyFontSettings();
}

void CppEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

CppEditorWidget::Link CppEditorWidget::linkToSymbol(CPlusPlus::Symbol *symbol)
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

bool CppEditorWidget::openCppEditorAt(const Link &link, bool inNextSplit)
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

void CppEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision != documentRevision())
        return;

    d->m_lastSemanticInfo = semanticInfo;

    if (!d->m_localRenaming.isActive())
        d->m_useSelectionsUpdater.update();

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

TextEditor::IAssistInterface *CppEditorWidget::createAssistInterface(
    TextEditor::AssistKind kind,
    TextEditor::AssistReason reason) const
{
    if (kind == TextEditor::Completion) {
        if (CppCompletionAssistProvider *cap = cppEditorDocument()->completionAssistProvider()) {
            return cap->createAssistInterface(
                            ProjectExplorer::ProjectExplorerPlugin::currentProject(),
                            textDocument()->filePath(),
                            document(),
                            cppEditorDocument()->isObjCEnabled(),
                            position(),
                            reason);
        }
    } else if (kind == TextEditor::QuickFix) {
        if (!isSemanticInfoValid())
            return 0;
        return new CppQuickFixAssistInterface(const_cast<CppEditorWidget *>(this), reason);
    } else {
        return BaseTextEditorWidget::createAssistInterface(kind, reason);
    }
    return 0;
}

QSharedPointer<FunctionDeclDefLink> CppEditorWidget::declDefLink() const
{
    return d->m_declDefLink;
}

void CppEditorWidget::onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker)
{
    if (marker.data.canConvert<FunctionDeclDefLink::Marker>())
        applyDeclDefLinkChanges(true);
}

void CppEditorWidget::updateFunctionDeclDefLink()
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

    d->m_updateFunctionDeclDefLinkTimer.start();
}

void CppEditorWidget::updateFunctionDeclDefLinkNow()
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

    if (!isSemanticInfoValidExceptLocalUses())
        return;

    Snapshot snapshot = CppModelManagerInterface::instance()->snapshot();
    snapshot.insert(semanticDoc);

    d->m_declDefLinkFinder->startFindLinkAt(textCursor(), semanticDoc, snapshot);
}

void CppEditorWidget::onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    d->m_declDefLink = link;
    Core::IDocument *targetDocument
            = Core::DocumentModel::documentForFilePath( d->m_declDefLink->targetFile->fileName());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<TextEditor::BaseTextDocument *>(targetDocument))
            connect(textDocument, SIGNAL(contentsChanged()),
                    this, SLOT(abortDeclDefLink()));
    }

}

void CppEditorWidget::onFilePathChanged()
{
    QTC_ASSERT(d->m_modelManager, return);
    QByteArray additionalDirectives;
    const QString &filePath = textDocument()->filePath();
    if (!filePath.isEmpty()) {
        const QString &projectFile = ProjectExplorer::SessionManager::value(
                    QLatin1String(Constants::CPP_PREPROCESSOR_PROJECT_PREFIX) + filePath).toString();
        additionalDirectives = ProjectExplorer::SessionManager::value(
                    projectFile + QLatin1Char(',') + filePath).toString().toUtf8();

        BaseEditorDocumentParser *parser = BaseEditorDocumentParser::get(filePath);
        QTC_ASSERT(parser, return);
        parser->setProjectPart(d->m_modelManager->projectPartForProjectFile(projectFile));
        parser->setEditorDefines(additionalDirectives);
    }
    d->m_preprocessorButton->setProperty("highlightWidget", !additionalDirectives.trimmed().isEmpty());
    d->m_preprocessorButton->update();
}

void CppEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!d->m_declDefLink)
        return;
    d->m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

FollowSymbolUnderCursor *CppEditorWidget::followSymbolUnderCursorDelegate()
{
    return d->m_followSymbolUnderCursor.data();
}

void CppEditorWidget::abortDeclDefLink()
{
    if (!d->m_declDefLink)
        return;

    Core::IDocument *targetDocument
            = Core::DocumentModel::documentForFilePath(d->m_declDefLink->targetFile->fileName());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<TextEditor::BaseTextDocument *>(targetDocument))
            disconnect(textDocument, SIGNAL(contentsChanged()),
                    this, SLOT(abortDeclDefLink()));
    }

    d->m_declDefLink->hideMarker(this);
    d->m_declDefLink.clear();
}

void CppEditorWidget::showPreProcessorWidget()
{
    const QString &fileName = editor()->document()->filePath();

    // Check if this editor belongs to a project
    QList<ProjectPart::Ptr> projectParts = d->m_modelManager->projectPart(fileName);
    if (projectParts.isEmpty())
        projectParts = d->m_modelManager->projectPartFromDependencies(fileName);
    if (projectParts.isEmpty())
        projectParts << d->m_modelManager->fallbackProjectPart();

    CppPreProcessorDialog preProcessorDialog(this, textDocument()->filePath(), projectParts);
    if (preProcessorDialog.exec() == QDialog::Accepted) {
        BaseEditorDocumentParser *parser = BaseEditorDocumentParser::get(fileName);
        QTC_ASSERT(parser, return);
        const QString &additionals = preProcessorDialog.additionalPreProcessorDirectives();
        parser->setProjectPart(preProcessorDialog.projectPart());
        parser->setEditorDefines(additionals.toUtf8());
        parser->update(d->m_modelManager->workingCopy());

        d->m_preprocessorButton->setProperty("highlightWidget", !additionals.trimmed().isEmpty());
        d->m_preprocessorButton->update();
    }
}

} // namespace Internal
} // namespace CppEditor
