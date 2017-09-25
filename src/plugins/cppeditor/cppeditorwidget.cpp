/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppeditorwidget.h"

#include "cppautocompleter.h"
#include "cppdocumentationcommenthelper.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditorplugin.h"
#include "cppfunctiondecldeflink.h"
#include "cpphighlighter.h"
#include "cpplocalrenaming.h"
#include "cppminimizableinfobars.h"
#include "cpppreprocessordialog.h"
#include "cppquickfixassistant.h"
#include "cppuseselectionsupdater.h"

#include <clangsupport/sourcelocationscontainer.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/infobar.h>

#include <cpptools/cppcanonicalsymbol.h>
#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppeditoroutline.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cppselectionchanger.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/cppworkingcopy.h>
#include <cpptools/refactoringengineinterface.h>
#include <cpptools/followsymbolinterface.h>
#include <cpptools/symbolfinder.h>

#include <texteditor/behaviorsettings.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/completionsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/projecttree.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/FastPreprocessor.h>
#include <cplusplus/MatchingText.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QMenu>
#include <QPointer>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

enum { UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200 };

using namespace Core;
using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {

class CppEditorWidgetPrivate
{
public:
    CppEditorWidgetPrivate(CppEditorWidget *q);

public:
    QPointer<CppModelManager> m_modelManager;

    CppEditorDocument *m_cppEditorDocument;
    CppEditorOutline *m_cppEditorOutline;

    QTimer m_updateFunctionDeclDefLinkTimer;
    SemanticInfo m_lastSemanticInfo;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;

    QAction *m_parseContextAction = nullptr;
    ParseContextWidget *m_parseContextWidget = nullptr;
    QToolButton *m_preprocessorButton = nullptr;
    MinimizableInfoBars::Actions m_showInfoBarActions;

    CppLocalRenaming m_localRenaming;
    CppUseSelectionsUpdater m_useSelectionsUpdater;
    CppSelectionChanger m_cppSelectionChanger;
};

CppEditorWidgetPrivate::CppEditorWidgetPrivate(CppEditorWidget *q)
    : m_modelManager(CppModelManager::instance())
    , m_cppEditorDocument(qobject_cast<CppEditorDocument *>(q->textDocument()))
    , m_cppEditorOutline(new CppEditorOutline(q))
    , m_declDefLinkFinder(new FunctionDeclDefLinkFinder(q))
    , m_localRenaming(q)
    , m_useSelectionsUpdater(q)
    , m_cppSelectionChanger()
{}

CppEditorWidget::CppEditorWidget()
    : d(new CppEditorWidgetPrivate(this))
{
    qRegisterMetaType<SemanticInfo>("CppTools::SemanticInfo");
}

void CppEditorWidget::finalizeInitialization()
{
    d->m_cppEditorDocument = qobject_cast<CppEditorDocument *>(textDocument());

    setLanguageSettingsId(CppTools::Constants::CPP_SETTINGS_ID);

    // clang-format off
    // function combo box sorting
    connect(CppEditorPlugin::instance(), &CppEditorPlugin::outlineSortingChanged,
            outline(), &CppEditorOutline::setSorted);

    connect(d->m_cppEditorDocument, &CppEditorDocument::codeWarningsUpdated,
            this, &CppEditorWidget::onCodeWarningsUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::ifdefedOutBlocksUpdated,
            this, &CppEditorWidget::onIfdefedOutBlocksUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::cppDocumentUpdated,
            this, &CppEditorWidget::onCppDocumentUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::semanticInfoUpdated,
            this, [this](const CppTools::SemanticInfo &info) { updateSemanticInfo(info); });

    connect(d->m_declDefLinkFinder, &FunctionDeclDefLinkFinder::foundLink,
            this, &CppEditorWidget::onFunctionDeclDefLinkFound);

    connect(&d->m_useSelectionsUpdater,
            &CppUseSelectionsUpdater::selectionsForVariableUnderCursorUpdated,
            &d->m_localRenaming,
            &CppLocalRenaming::updateSelectionsForVariableUnderCursor);

    connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished, this,
            [this] (SemanticInfo::LocalUseMap localUses, bool success) {
                if (success) {
                    d->m_lastSemanticInfo.localUsesUpdated = true;
                    d->m_lastSemanticInfo.localUses = localUses;
                }
    });

    connect(document(), &QTextDocument::contentsChange,
            &d->m_localRenaming, &CppLocalRenaming::onContentsChangeOfEditorWidgetDocument);
    connect(&d->m_localRenaming, &CppLocalRenaming::finished, [this] {
        cppEditorDocument()->recalculateSemanticInfoDetached();
    });
    connect(&d->m_localRenaming, &CppLocalRenaming::processKeyPressNormally,
            this, &CppEditorWidget::processKeyNormally);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            d->m_cppEditorOutline, &CppEditorOutline::updateIndex);

    connect(cppEditorDocument(), &CppEditorDocument::preprocessorSettingsChanged, this,
            [this](bool customSettings) {
        updateWidgetHighlighting(d->m_preprocessorButton, customSettings);
    });

    // set up function declaration - definition link
    d->m_updateFunctionDeclDefLinkTimer.setSingleShot(true);
    d->m_updateFunctionDeclDefLinkTimer.setInterval(UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(&d->m_updateFunctionDeclDefLinkTimer, &QTimer::timeout,
            this, &CppEditorWidget::updateFunctionDeclDefLinkNow);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);
    connect(this, &QPlainTextEdit::textChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);

    // set up the use highlighitng
    connect(this, &CppEditorWidget::cursorPositionChanged, this, [this]() {
        if (!d->m_localRenaming.isActive())
            d->m_useSelectionsUpdater.scheduleUpdate();

        // Notify selection expander about the changed cursor.
        d->m_cppSelectionChanger.onCursorPositionChanged(textCursor());
    });

    // Toolbar: Outline/Overview combo box
    insertExtraToolBarWidget(TextEditorWidget::Left, d->m_cppEditorOutline->widget());

    // Toolbar: Parse context
    ParseContextModel &parseContextModel = cppEditorDocument()->parseContextModel();
    d->m_parseContextWidget = new ParseContextWidget(parseContextModel, this);
    d->m_parseContextAction = insertExtraToolBarWidget(TextEditorWidget::Left,
                                                       d->m_parseContextWidget);
    d->m_parseContextAction->setVisible(false);
    connect(&parseContextModel, &ParseContextModel::updated,
            this, [this](bool areMultipleAvailable) {
        d->m_parseContextAction->setVisible(areMultipleAvailable);
    });
    // clang-format on
    // Toolbar: '#' Button
    // TODO: Make "Additional Preprocessor Directives" also useful with Clang Code Model.
    if (!d->m_modelManager->isClangCodeModelActive()) {
        d->m_preprocessorButton = new QToolButton(this);
        d->m_preprocessorButton->setText(QLatin1String("#"));
        Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
        connect(cmd, &Command::keySequenceChanged,
                this, &CppEditorWidget::updatePreprocessorButtonTooltip);
        updatePreprocessorButtonTooltip();
        connect(d->m_preprocessorButton, &QAbstractButton::clicked,
                this, &CppEditorWidget::showPreProcessorWidget);

        insertExtraToolBarWidget(TextEditorWidget::Left, d->m_preprocessorButton);
    }

    // Toolbar: Actions to show minimized info bars
    d->m_showInfoBarActions = MinimizableInfoBars::createShowInfoBarActions([this](QWidget *w) {
        return this->insertExtraToolBarWidget(TextEditorWidget::Left, w);
    });
    connect(&cppEditorDocument()->minimizableInfoBars(), &MinimizableInfoBars::showAction,
            this, &CppEditorWidget::onShowInfoBarAction);
}

void CppEditorWidget::finalizeInitializationAfterDuplication(TextEditorWidget *other)
{
    QTC_ASSERT(other, return);
    CppEditorWidget *cppEditorWidget = qobject_cast<CppEditorWidget *>(other);
    QTC_ASSERT(cppEditorWidget, return);

    if (cppEditorWidget->isSemanticInfoValidExceptLocalUses())
        updateSemanticInfo(cppEditorWidget->semanticInfo());
    d->m_cppEditorOutline->update();
    const Id selectionKind = CodeWarningsSelection;
    setExtraSelections(selectionKind, cppEditorWidget->extraSelections(selectionKind));

    if (isWidgetHighlighted(cppEditorWidget->d->m_preprocessorButton))
        updateWidgetHighlighting(d->m_preprocessorButton, true);

    d->m_parseContextWidget->syncToModel();
    d->m_parseContextAction->setVisible(
                d->m_cppEditorDocument->parseContextModel().areMultipleAvailable());
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

    TextEditorWidget::paste();
}

void CppEditorWidget::cut()
{
    if (d->m_localRenaming.handleCut())
        return;

    TextEditorWidget::cut();
}

void CppEditorWidget::selectAll()
{
    if (d->m_localRenaming.handleSelectAll())
        return;

    TextEditorWidget::selectAll();
}

void CppEditorWidget::onCppDocumentUpdated()
{
    d->m_cppEditorOutline->update();
}

void CppEditorWidget::onCodeWarningsUpdated(unsigned revision,
                                            const QList<QTextEdit::ExtraSelection> selections,
                                            const TextEditor::RefactorMarkers &refactorMarkers)
{
    if (revision != documentRevision())
        return;

    setExtraSelections(TextEditorWidget::CodeWarningsSelection, selections);
    setRefactorMarkers(refactorMarkersWithoutClangMarkers() + refactorMarkers);
}

void CppEditorWidget::onIfdefedOutBlocksUpdated(unsigned revision,
                                                const QList<BlockRange> ifdefedOutBlocks)
{
    if (revision != documentRevision())
        return;
    setIfdefedOutBlocks(ifdefedOutBlocks);
}

void CppEditorWidget::onShowInfoBarAction(const Id &id, bool show)
{
    QAction *action = d->m_showInfoBarActions.value(id);
    QTC_ASSERT(action, return);
    action->setVisible(show);
}

static QString getDocumentLine(const QTextDocument &document, int line)
{
    return document.findBlockByNumber(line - 1).text();
}

static QString getFileLine(const QString &path, int line)
{
    const IDocument *document = DocumentModel::documentForFilePath(path);
    const TextDocument *textDocument = qobject_cast<const TextDocument *>(document);
    if (textDocument)
        return getDocumentLine(*textDocument->document(), line);

    const QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
    QString contents;
    Utils::TextFileFormat format;
    QString error;
    if (Utils::TextFileFormat::readFile(path, defaultCodec, &contents, &format, &error)
            != Utils::TextFileFormat::ReadSuccess) {
        qWarning() << "Error reading file " << path << " : " << error;
        return QString();
    }

    const QTextDocument tmpDocument{contents};
    return getDocumentLine(tmpDocument, line);
}

static void findRenameCallback(QTextCursor cursor,
                               const CppTools::Usages &usages,
                               bool rename = false)
{
    cursor = Utils::Text::wordStartCursor(cursor);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString text = cursor.selectedText();
    SearchResultWindow::SearchMode mode = SearchResultWindow::SearchOnly;
    if (rename)
        mode = SearchResultWindow::SearchAndReplace;
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                QObject::tr("C++ Usages:"),
                QString(),
                text,
                mode,
                SearchResultWindow::PreserveCaseDisabled,
                QLatin1String("CppEditor"));
    for (const CppTools::Usage &usage : usages) {
        const QString lineStr = getFileLine(usage.path, usage.line);
        if (lineStr.isEmpty())
            continue;
        Search::TextRange range{Search::TextPosition(usage.line, usage.column - 1),
                    Search::TextPosition(usage.line, usage.column + text.length() - 1)};
        search->addResult(usage.path, lineStr, range);
    }
    search->finishSearch(false);
    QObject::connect(search, &SearchResult::activated,
                     [](const Core::SearchResultItem& item) {
                         Core::EditorManager::openEditorAtSearchResult(item);
                     });
    search->popup();
}

void CppEditorWidget::findUsages()
{
    refactoringEngine().findUsages(CppTools::CursorInEditor{textCursor(),
                                                            textDocument()->filePath(),
                                                            this},
                                   [this](const CppTools::Usages &usages) {
                                       findRenameCallback(textCursor(), usages);
                                   });
}

void CppEditorWidget::renameUsages(const QString &replacement)
{
    refactoringEngine().globalRename(CppTools::CursorInEditor{textCursor(),
                                                              textDocument()->filePath(),
                                                              this},
                                     [this](const CppTools::Usages &usages) {
                                         findRenameCallback(textCursor(), usages, true);
                                     },
                                     replacement);
}

bool CppEditorWidget::selectBlockUp()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockUp();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ExpandSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

bool CppEditorWidget::selectBlockDown()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockDown();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ShrinkSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

void CppEditorWidget::updateWidgetHighlighting(QWidget *widget, bool highlight)
{
    if (!widget)
        return;

    widget->setProperty("highlightWidget", highlight);
    widget->update();
}

bool CppEditorWidget::isWidgetHighlighted(QWidget *widget)
{
    return widget ? widget->property("highlightWidget").toBool() : false;
}

namespace {

QList<ProjectPart::Ptr> fetchProjectParts(CppTools::CppModelManager *modelManager,
                                     const Utils::FileName &filePath)
{
    QList<ProjectPart::Ptr> projectParts = modelManager->projectPart(filePath);

    if (projectParts.isEmpty())
        projectParts = modelManager->projectPartFromDependencies(filePath);
    if (projectParts.isEmpty())
        projectParts.append(modelManager->fallbackProjectPart());

    return projectParts;
}

ProjectPart *findProjectPartForCurrentProject(const QList<ProjectPart::Ptr> &projectParts,
                                              ProjectExplorer::Project *currentProject)
{
    auto found = std::find_if(projectParts.cbegin(),
                              projectParts.cend(),
                              [&](const CppTools::ProjectPart::Ptr &projectPart) {
                                  return projectPart->project == currentProject;
                              });

    if (found != projectParts.cend())
        return (*found).data();

    return 0;
}

} // namespace

ProjectPart *CppEditorWidget::projectPart() const
{
    if (!d->m_modelManager)
        return 0;

    auto projectParts = fetchProjectParts(d->m_modelManager, textDocument()->filePath());

    return findProjectPartForCurrentProject(projectParts,
                                            ProjectExplorer::ProjectTree::currentProject());
}

namespace {

using ClangBackEnd::V2::SourceLocationContainer;
using Utils::Text::selectAt;

QTextCharFormat occurrencesTextCharFormat()
{
    using TextEditor::TextEditorSettings;

    return TextEditorSettings::fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
}

QList<QTextEdit::ExtraSelection> sourceLocationsToExtraSelections(
    const std::vector<SourceLocationContainer> &sourceLocations,
    uint selectionLength,
    CppEditorWidget *cppEditorWidget)
{
    const auto textCharFormat = occurrencesTextCharFormat();

    QList<QTextEdit::ExtraSelection> selections;
    selections.reserve(int(sourceLocations.size()));

    auto sourceLocationToExtraSelection = [&](const SourceLocationContainer &sourceLocation) {
        QTextEdit::ExtraSelection selection;

        selection.cursor = selectAt(cppEditorWidget->textCursor(),
                                    sourceLocation.line(),
                                    sourceLocation.column(),
                                    selectionLength);
        selection.format = textCharFormat;

        return selection;
    };

    std::transform(sourceLocations.begin(),
                   sourceLocations.end(),
                   std::back_inserter(selections),
                   sourceLocationToExtraSelection);

    return selections;
};

}

void CppEditorWidget::renameSymbolUnderCursor()
{
    using ClangBackEnd::SourceLocationsContainer;

    ProjectPart *projPart = projectPart();
    if (!projPart)
        return;

    d->m_useSelectionsUpdater.abortSchedule();

    QPointer<CppEditorWidget> cppEditorWidget = this;

    auto renameSymbols = [=](const QString &symbolName,
                             const SourceLocationsContainer &sourceLocations,
                             int revision) {
        if (cppEditorWidget) {
            viewport()->setCursor(Qt::IBeamCursor);

            if (revision != document()->revision())
                return;
            if (sourceLocations.hasContent()) {
                QList<QTextEdit::ExtraSelection> selections
                    = sourceLocationsToExtraSelections(sourceLocations.sourceLocationContainers(),
                                                       static_cast<uint>(symbolName.size()),
                                                       cppEditorWidget);
                setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, selections);
                d->m_localRenaming.updateSelectionsForVariableUnderCursor(selections);
            }
            if (!d->m_localRenaming.start())
                cppEditorWidget->renameUsages();
        }
    };

    viewport()->setCursor(Qt::BusyCursor);
    refactoringEngine().startLocalRenaming(CppTools::CursorInEditor{textCursor(),
                                                                     textDocument()->filePath(),
                                                                     this},
                                            projPart,
                                            std::move(renameSymbols));
}

void CppEditorWidget::updatePreprocessorButtonTooltip()
{
    if (!d->m_preprocessorButton)
        return;

    Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    QTC_ASSERT(cmd, return );
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
        symbolLink = linkToSymbol(
            d->m_modelManager->symbolFinder()
                ->findMatchingDefinition(functionDeclarationSymbol, d->m_modelManager->snapshot()));
    } else if (functionDefinitionSymbol) {
        const Snapshot snapshot = d->m_modelManager->snapshot();
        LookupContext context(d->m_lastSemanticInfo.doc, snapshot);
        ClassOrNamespace *binding = context.lookupType(functionDefinitionSymbol);
        const QList<LookupItem> declarations
            = context.lookup(functionDefinitionSymbol->name(),
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
        openLink(symbolLink, inNextSplit != alwaysOpenLinksInNextSplit());
}

CppEditorWidget::Link CppEditorWidget::findLinkAt(const QTextCursor &cursor,
                                                  bool resolveTarget,
                                                  bool inNextSplit)
{
    if (!d->m_modelManager)
        return Link();

    const Utils::FileName &filePath = textDocument()->filePath();

    return followSymbolInterface().findLink(CppTools::CursorInEditor{cursor, filePath, this},
                                             resolveTarget,
                                             d->m_modelManager->snapshot(),
                                             d->m_lastSemanticInfo.doc,
                                             d->m_modelManager->symbolFinder(),
                                             inNextSplit);
}

unsigned CppEditorWidget::documentRevision() const
{
    return document()->revision();
}

static bool isClangFixItAvailableMarker(const RefactorMarker &marker)
{
    return marker.data.toString()
           == QLatin1String(CppTools::Constants::CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID);
}

RefactorMarkers CppEditorWidget::refactorMarkersWithoutClangMarkers() const
{
    RefactorMarkers clearedRefactorMarkers;

    foreach (const RefactorMarker &marker, refactorMarkers()) {
        if (isClangFixItAvailableMarker(marker))
            continue;

        clearedRefactorMarkers.append(marker);
    }

    return clearedRefactorMarkers;
}

RefactoringEngineInterface &CppEditorWidget::refactoringEngine() const
{
    return *CppTools::CppModelManager::instance();
}

CppTools::FollowSymbolInterface &CppEditorWidget::followSymbolInterface() const
{
    return CppTools::CppModelManager::instance()->followSymbolInterface();
}

bool CppEditorWidget::isSemanticInfoValidExceptLocalUses() const
{
    return d->m_lastSemanticInfo.doc && d->m_lastSemanticInfo.revision == documentRevision()
           && !d->m_lastSemanticInfo.snapshot.isEmpty();
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
        if (static_cast<QKeyEvent *>(e)->key() == Qt::Key_Escape && d->m_localRenaming.isActive()) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return TextEditorWidget::event(e);
}

void CppEditorWidget::processKeyNormally(QKeyEvent *e)
{
    TextEditorWidget::keyPressEvent(e);
}

static void addRefactoringActions(QMenu *menu, AssistInterface *iface)
{
    if (!iface || !menu)
        return;

    using Processor = QScopedPointer<IAssistProcessor>;
    using Proposal = QScopedPointer<IAssistProposal>;
    using Model = QScopedPointer<GenericProposalModel>;

    const Processor processor(CppEditorPlugin::instance()->quickFixProvider()->createProcessor());
    const Proposal proposal(processor->perform(iface)); // OK, perform() takes ownership of iface.
    if (proposal) {
        Model model(static_cast<GenericProposalModel *>(proposal->model()));
        for (int index = 0; index < model->size(); ++index) {
            const auto item = static_cast<AssistProposalItem *>(model->proposalItem(index));
            const QuickFixOperation::Ptr op = item->data().value<QuickFixOperation::Ptr>();
            const QAction *action = menu->addAction(op->description());
            QObject::connect(action, &QAction::triggered, menu, [op] { op->perform(); });
        }
    }
}

class ProgressIndicatorMenuItem : public QWidgetAction
{
    Q_OBJECT

public:
    ProgressIndicatorMenuItem(QObject *parent) : QWidgetAction(parent) {}

protected:
    QWidget *createWidget(QWidget *parent = nullptr) override
    {
        return new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small, parent);
    }
};

QMenu *CppEditorWidget::createRefactorMenu(QWidget *parent) const
{
    auto *menu = new QMenu(tr("&Refactor"), parent);
    menu->addAction(ActionManager::command(Constants::RENAME_SYMBOL_UNDER_CURSOR)->action());

    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    if (isSemanticInfoValidExceptLocalUses()) {
        d->m_useSelectionsUpdater.abortSchedule();

        const CppUseSelectionsUpdater::RunnerInfo runnerInfo = d->m_useSelectionsUpdater.update();
        switch (runnerInfo) {
        case CppUseSelectionsUpdater::RunnerInfo::AlreadyUpToDate:
            addRefactoringActions(menu, createAssistInterface(QuickFix, ExplicitlyInvoked));
            break;
        case CppUseSelectionsUpdater::RunnerInfo::Started: {
            // Update the refactor menu once we get the results.
            auto *progressIndicatorMenuItem = new ProgressIndicatorMenuItem(menu);
            menu->addAction(progressIndicatorMenuItem);

            connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished,
                    menu, [=] (SemanticInfo::LocalUseMap, bool success) {
                QTC_CHECK(success);
                menu->removeAction(progressIndicatorMenuItem);
                addRefactoringActions(menu, createAssistInterface(QuickFix, ExplicitlyInvoked));
            });
            break;
        }
        case CppUseSelectionsUpdater::RunnerInfo::FailedToStart:
        case CppUseSelectionsUpdater::RunnerInfo::Invalid:
            QTC_CHECK(false && "Unexpected CppUseSelectionsUpdater runner result");
        }
    }

    return menu;
}

static void appendCustomContextMenuActionsAndMenus(QMenu *menu, QMenu *refactorMenu)
{
    bool isRefactoringMenuAdded = false;
    const QMenu *contextMenu = ActionManager::actionContainer(Constants::M_CONTEXT)->menu();
    for (QAction *action : contextMenu->actions()) {
        menu->addAction(action);
        if (action->objectName() == Constants::M_REFACTORING_MENU_INSERTION_POINT) {
            isRefactoringMenuAdded = true;
            menu->addMenu(refactorMenu);
        }
    }

    QTC_CHECK(isRefactoringMenuAdded);
}

void CppEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    const QPointer<QMenu> menu(new QMenu(this));

    appendCustomContextMenuActionsAndMenus(menu, createRefactorMenu(menu));
    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (menu)
        delete menu; // OK, menu was not already deleted by closed editor widget.
}

void CppEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (d->m_localRenaming.handleKeyPressEvent(e))
        return;

    if (handleStringSplitting(e))
        return;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (trySplitComment(this, semanticInfo().snapshot)) {
            e->accept();
            return;
        }
    }

    TextEditorWidget::keyPressEvent(e);
}

bool CppEditorWidget::handleStringSplitting(QKeyEvent *e) const
{
    if (!TextEditorSettings::completionSettings().m_autoSplitStrings)
        return false;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();

        const Kind stringKind = CPlusPlus::MatchingText::stringKindAtCursor(cursor);
        if (stringKind >= T_FIRST_STRING_LITERAL && stringKind < T_FIRST_RAW_STRING_LITERAL) {
            cursor.beginEditBlock();
            if (cursor.positionInBlock() > 0
                && cursor.block().text().at(cursor.positionInBlock() - 1) == QLatin1Char('\\')) {
                // Already escaped: simply go back to line, but do not indent.
                cursor.insertText(QLatin1String("\n"));
            } else if (e->modifiers() & Qt::ShiftModifier) {
                // With 'shift' modifier, escape the end of line character
                // and start at beginning of next line.
                cursor.insertText(QLatin1String("\\\n"));
            } else {
                // End the current string, and start a new one on the line, properly indented.
                cursor.insertText(QLatin1String("\"\n\""));
                textDocument()->autoIndent(cursor);
            }
            cursor.endEditBlock();
            e->accept();
            return true;
        }
    }

    return false;
}

void CppEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorWidget::updateSemanticInfo()
{
    updateSemanticInfo(d->m_cppEditorDocument->recalculateSemanticInfo(),
                       /*updateUseSelectionSynchronously=*/ true);
}

void CppEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo,
                                         bool updateUseSelectionSynchronously)
{
    if (semanticInfo.revision != documentRevision())
        return;

    d->m_lastSemanticInfo = semanticInfo;

    if (!d->m_localRenaming.isActive()) {
        const CppUseSelectionsUpdater::CallType type = updateUseSelectionSynchronously
            ? CppUseSelectionsUpdater::CallType::Synchronous
            : CppUseSelectionsUpdater::CallType::Asynchronous;
        d->m_useSelectionsUpdater.update(type);
    }

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

AssistInterface *CppEditorWidget::createAssistInterface(AssistKind kind, AssistReason reason) const
{
    if (kind == Completion) {
        if (CppCompletionAssistProvider *cap = qobject_cast<CppCompletionAssistProvider *>(
                cppEditorDocument()->completionAssistProvider())) {
            LanguageFeatures features = LanguageFeatures::defaultFeatures();
            if (Document::Ptr doc = d->m_lastSemanticInfo.doc)
                features = doc->languageFeatures();
            features.objCEnabled |= cppEditorDocument()->isObjCEnabled();
            return cap->createAssistInterface(textDocument()->filePath().toString(),
                                              this,
                                              features,
                                              position(),
                                              reason);
        }
    } else if (kind == QuickFix) {
        if (isSemanticInfoValid())
            return new CppQuickFixInterface(const_cast<CppEditorWidget *>(this), reason);
    } else {
        return TextEditorWidget::createAssistInterface(kind, reason);
    }
    return 0;
}

QSharedPointer<FunctionDeclDefLink> CppEditorWidget::declDefLink() const
{
    return d->m_declDefLink;
}

void CppEditorWidget::onRefactorMarkerClicked(const RefactorMarker &marker)
{
    if (marker.data.canConvert<FunctionDeclDefLink::Marker>()) {
        applyDeclDefLinkChanges(true);
    } else if (isClangFixItAvailableMarker(marker)) {
        int line, column;
        if (Utils::Text::convertPosition(document(), marker.cursor.position(), &line, &column)) {
            setTextCursor(marker.cursor);
            invokeAssist(TextEditor::QuickFix);
        }
    }
}

void CppEditorWidget::updateFunctionDeclDefLink()
{
    const int pos = textCursor().selectionStart();

    // if there's already a link, abort it if the cursor is outside or the name changed
    // (adding a prefix is an exception since the user might type a return type)
    if (d->m_declDefLink
        && (pos < d->m_declDefLink->linkSelection.selectionStart()
            || pos > d->m_declDefLink->linkSelection.selectionEnd()
            || !d->m_declDefLink->nameSelection.selectedText().trimmed().endsWith(
                   d->m_declDefLink->nameInitial))) {
        abortDeclDefLink();
        return;
    }

    // don't start a new scan if there's one active and the cursor is already in the scanned area
    const QTextCursor scannedSelection = d->m_declDefLinkFinder->scannedSelection();
    if (!scannedSelection.isNull() && scannedSelection.selectionStart() <= pos
        && scannedSelection.selectionEnd() >= pos) {
        return;
    }

    d->m_updateFunctionDeclDefLinkTimer.start();
}

void CppEditorWidget::updateFunctionDeclDefLinkNow()
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor || editor->widget() != this)
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

    Snapshot snapshot = CppModelManager::instance()->snapshot();
    snapshot.insert(semanticDoc);

    d->m_declDefLinkFinder->startFindLinkAt(textCursor(), semanticDoc, snapshot);
}

void CppEditorWidget::onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    d->m_declDefLink = link;
    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->fileName());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            connect(textDocument,
                    &IDocument::contentsChanged,
                    this,
                    &CppEditorWidget::abortDeclDefLink);
    }
}

void CppEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!d->m_declDefLink)
        return;
    d->m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

void CppEditorWidget::encourageApply()
{
    if (d->m_localRenaming.encourageApply())
        return;

    TextEditorWidget::encourageApply();
}

void CppEditorWidget::abortDeclDefLink()
{
    if (!d->m_declDefLink)
        return;

    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->fileName());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            disconnect(textDocument,
                       &IDocument::contentsChanged,
                       this,
                       &CppEditorWidget::abortDeclDefLink);
    }

    d->m_declDefLink->hideMarker(this);
    d->m_declDefLink.clear();
}

void CppEditorWidget::showPreProcessorWidget()
{
    const QString filePath = textDocument()->filePath().toString();

    CppPreProcessorDialog dialog(filePath, this);
    if (dialog.exec() == QDialog::Accepted) {
        const QByteArray extraDirectives = dialog.extraPreprocessorDirectives().toUtf8();
        cppEditorDocument()->setExtraPreprocessorDirectives(extraDirectives);
        cppEditorDocument()->scheduleProcessDocument();
    }
}

void CppEditorWidget::invokeTextEditorWidgetAssist(TextEditor::AssistKind assistKind,
                                                   TextEditor::IAssistProvider *provider)
{
    invokeAssist(assistKind, provider);
}

} // namespace Internal
} // namespace CppEditor

#include "cppeditorwidget.moc"
