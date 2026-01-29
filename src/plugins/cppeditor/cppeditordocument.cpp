// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"
#include "cppcodeformatter.h"
#include "cppcompletionassistprovider.h"
#include "cppeditorconstants.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditorlogging.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cpphighlighter.h"
#include "cppmodelmanager.h"
#include "cppoutlinemodel.h"
#include "cppparsecontext.h"
#include "editordocumenthandle.h"
#include "quickfixes/cppquickfixassistant.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/session.h>

#include <cplusplus/ASTPath.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectwizardpage.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/storagesettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <utils/infobar.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/minimizableinfobars.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/wizard.h>

#include <QApplication>
#include <QScopeGuard>
#include <QTextDocument>

#include <memory>

const char NO_PROJECT_CONFIGURATION[] = "NoProject";

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

using namespace Internal;

static InfoBarEntry createInfoBarEntry(const FilePath &filePath)
{
    InfoBarEntry infoBarEntry(
        NO_PROJECT_CONFIGURATION,
        Tr::tr(
            "<b>Warning</b>: This file is not part of any project. "
            "The code model might have issues parsing this file properly."));
    InfoBarEntry::CallBack addToProject = [filePath] {
        Wizard wizard;
        const std::unique_ptr<ProjectWizardPage> wizardPage = std::make_unique<ProjectWizardPage>(
            dialogParent());
        wizard.setWindowTitle(Tr::tr("Add File to Project"));
        wizard.setProperty(
            ProjectExplorer::Constants::PROJECT_POINTER,
            QVariant::fromValue(static_cast<void *>(ProjectManager::startupProject())));
        wizard.addPage(wizardPage.get());
        wizardPage->setFiles({filePath});
        wizardPage->initializeVersionControls();
        wizardPage->initializeProjectTree(
            nullptr, {}, Core::IWizardFactory::FileWizard, ProjectExplorer::AddExistingFile, false);
        if (wizard.exec() == QDialog::Accepted && wizardPage->currentNode())
            ProjectExplorerPlugin::addExistingFiles(wizardPage->currentNode(), {filePath});
    };
    const bool enableAddToProjectButton = !ProjectManager::isAnyProjectParsing()
                                          && !ProjectManager::isKnownFile(filePath);
    infoBarEntry.addCustomButton(Tr::tr("Add to Project..."), addToProject, {}, {},
                                 enableAddToProjectButton);
    return infoBarEntry;
}

enum { processDocumentIntervalInMs = 150 };

class CppEditorDocument::Private
{
public:
    explicit Private(CppEditorDocument *parent)
        : q(parent)
    {}

    void invalidateFormatterCache();
    void onFilePathChanged(const Utils::FilePath &oldPath, const Utils::FilePath &newPath);
    void onMimeTypeChanged();

    void onAboutToReload();
    void onReloadFinished();
    void onDiagnosticsChanged(const Utils::FilePath &fileName, const QString &kind);

    void updateInfoBarEntryIfVisible();

    void reparseWithPreferredParseContext(const QString &id);

    void processDocument();

    QByteArray contentsText() const;
    unsigned contentsRevision() const;

    BaseEditorDocumentProcessor *processor();
    void resetProcessor();
    void applyPreferredParseContextFromSettings();
    void applyExtraPreprocessorDirectivesFromSettings();
    void releaseResources();

    void showHideInfoBarAboutMultipleParseContexts(bool show);
    void applyIfdefedOutBlocks();

    void initializeTimer();

    FilePath filePath() const { return q->filePath(); }
    QTextDocument *document() const { return q->document(); }

    bool m_fileIsBeingReloaded = false;
    bool m_isObjCEnabled = false;

    // Caching contents
    mutable QMutex m_cachedContentsLock;
    mutable QByteArray m_cachedContents;
    mutable int m_cachedContentsRevision = -1;

    unsigned m_processorRevision = 0;
    QTimer m_processorTimer;
    QScopedPointer<BaseEditorDocumentProcessor> m_processor;

    CppCompletionAssistProvider *m_completionAssistProvider = nullptr;

    // (Un)Registration in CppModelManager
    QScopedPointer<CppEditorDocumentHandle> m_editorDocumentHandle;

    Internal::ParseContextModel m_parseContextModel;
    Internal::OutlineModel m_overviewModel;
    QList<TextEditor::BlockRange> m_ifdefedOutBlocks;

    CppEditorDocument *q = nullptr;
};

class CppEditorDocumentHandleImpl : public CppEditorDocumentHandle
{
public:
    CppEditorDocumentHandleImpl(CppEditorDocument::Private *cppEditorDocumentPrivate)
        : m_cppEditorDocumentPrivate(cppEditorDocumentPrivate)
        , m_registrationFilePath(cppEditorDocumentPrivate->filePath())
    {
        CppModelManager::registerCppEditorDocument(this);
    }

    ~CppEditorDocumentHandleImpl() override
    {
        CppModelManager::unregisterCppEditorDocument(m_registrationFilePath);
    }

    FilePath filePath() const override { return m_cppEditorDocumentPrivate->filePath(); }
    QByteArray contents() const override { return m_cppEditorDocumentPrivate->contentsText(); }
    unsigned revision() const override { return m_cppEditorDocumentPrivate->contentsRevision(); }

    BaseEditorDocumentProcessor *processor() const override
    { return m_cppEditorDocumentPrivate->processor(); }

    void resetProcessor() override
    { m_cppEditorDocumentPrivate->resetProcessor(); }

private:
    CppEditorDocument::Private * const m_cppEditorDocumentPrivate;
    // The file path of the editor document can change (e.g. by "Save As..."), so make sure
    // that un-registration happens with the path the document was registered.
    const FilePath m_registrationFilePath;
};

CppEditorDocument::CppEditorDocument()
    : d(new CppEditorDocument::Private(this))
{
    setId(CppEditor::Constants::CPPEDITOR_ID);
    resetSyntaxHighlighter([] { return new CppHighlighter(); });
    connect(syntaxHighlighter(), &SyntaxHighlighter::finished,
            this, [this]{ d->applyIfdefedOutBlocks(); });

    ICodeStylePreferencesFactory *factory
        = TextEditorSettings::codeStyleFactory(Constants::CPP_SETTINGS_ID);
    setIndenter(factory->createIndenter(document()));

    connect(this, &TextEditor::TextDocument::tabSettingsChanged, this, [this] {
        d->invalidateFormatterCache();
    });
    connect(this, &Core::IDocument::mimeTypeChanged, this, [this] { d->onMimeTypeChanged(); });

    connect(this, &Core::IDocument::aboutToReload, this, [this] { d->onAboutToReload(); });
    connect(this, &Core::IDocument::reloadFinished, this, [this] { d->onReloadFinished(); });
    connect(
        this,
        &IDocument::filePathChanged,
        this,
        [this](const Utils::FilePath &oldName, const Utils::FilePath &newName) {
            d->onFilePathChanged(oldName, newName);
        });

    connect(
        CppModelManager::instance(),
        &CppModelManager::diagnosticsChanged,
        this,
        [this](const Utils::FilePath &filePath, const QString &kind) {
            d->onDiagnosticsChanged(filePath, kind);
        });

    connect(
        &d->m_parseContextModel,
        &ParseContextModel::preferredParseContextChanged,
        this,
        [this](const QString &id) { d->reparseWithPreferredParseContext(id); });

    minimizableInfoBars()->setSettingsGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    minimizableInfoBars()->setPossibleInfoBarEntries({createInfoBarEntry(filePath())});
    connect(ProjectManager::instance(), &ProjectManager::projectAdded, this, [this] {
        d->updateInfoBarEntryIfVisible();
    });
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved, this, [this] {
        d->updateInfoBarEntryIfVisible();
    });
    connect(ProjectManager::instance(), &ProjectManager::projectStartedParsing, this, [this] {
        d->updateInfoBarEntryIfVisible();
    });
    connect(ProjectManager::instance(), &ProjectManager::projectFinishedParsing, this, [this] {
        d->updateInfoBarEntryIfVisible();
    });
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &CppEditorDocument::updateSoftPreferredParseContext);

    // See also onFilePathChanged() for more initialization
}

CppEditorDocument::~CppEditorDocument()
{
    delete d;
}

bool CppEditorDocument::isObjCEnabled() const
{
    return d->m_isObjCEnabled;
}

void CppEditorDocument::setCompletionAssistProvider(TextEditor::CompletionAssistProvider *provider)
{
    TextDocument::setCompletionAssistProvider(provider);
    d->m_completionAssistProvider = nullptr;
}

CompletionAssistProvider *CppEditorDocument::completionAssistProvider() const
{
    return d->m_completionAssistProvider
            ? d->m_completionAssistProvider : TextDocument::completionAssistProvider();
}

TextEditor::IAssistProvider *CppEditorDocument::quickFixAssistProvider() const
{
    if (const auto baseProvider = TextDocument::quickFixAssistProvider())
        return baseProvider;
    return &cppQuickFixAssistProvider();
}

void CppEditorDocument::recalculateSemanticInfoDetached()
{
    BaseEditorDocumentProcessor *p = d->processor();
    QTC_ASSERT(p, return);
    p->recalculateSemanticInfoDetached(true);
}

SemanticInfo CppEditorDocument::recalculateSemanticInfo()
{
    BaseEditorDocumentProcessor *p = d->processor();
    QTC_ASSERT(p, return SemanticInfo());
    return p->recalculateSemanticInfo();
}

QByteArray CppEditorDocument::Private::contentsText() const
{
    QMutexLocker locker(&m_cachedContentsLock);

    const int currentRevision = document()->revision();
    if (m_cachedContentsRevision != currentRevision && !m_fileIsBeingReloaded) {
        m_cachedContentsRevision = currentRevision;
        m_cachedContents = q->plainText().toUtf8();
    }

    return m_cachedContents;
}

void CppEditorDocument::applyFontSettings()
{
    if (TextEditor::SyntaxHighlighter *highlighter = syntaxHighlighter())
        highlighter->clearAllExtraFormats(); // Clear all additional formats since they may have changed
    TextDocument::applyFontSettings(); // rehighlights and updates additional formats
    if (d->m_processor)
        d->m_processor->semanticRehighlight();
}

void CppEditorDocument::slotCodeStyleSettingsChanged()
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorDocument::removeTrailingWhitespace(const QTextBlock &block)
{
    const auto baseImpl = [&] { TextDocument::removeTrailingWhitespace(block); };

    CPlusPlus::Document::Ptr doc;
    for (CppEditorWidget * const editorWidget : CppEditorWidget::editorWidgetsForDocument(this)) {
        if (editorWidget->isSemanticInfoValidExceptLocalUses()) {
            doc = editorWidget->semanticInfo().doc;
            QTC_ASSERT(doc, continue);
            break;
        }
    }
    if (!doc)
        return baseImpl();

    QTextCursor cursor(block);
    cursor.setPosition(block.position() + block.length() - 1);
    const QList<CPlusPlus::AST*> astPath = CPlusPlus::ASTPath(doc)(cursor);
    if (astPath.isEmpty())
        return baseImpl();
    const CPlusPlus::Token &tok = doc->translationUnit()->tokenAt(astPath.last()->firstToken());
    if (!tok.isRawStringLiteral())
        baseImpl();
}

void CppEditorDocument::processDocument()
{
    d->processDocument();
}

void CppEditorDocument::Private::invalidateFormatterCache()
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorDocument::Private::onMimeTypeChanged()
{
    const QString &mt = q->mimeType();
    m_isObjCEnabled = (mt == QLatin1String(Utils::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
                       || mt == QLatin1String(Utils::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    m_completionAssistProvider = CppModelManager::completionAssistProvider();

    initializeTimer();
}

void CppEditorDocument::Private::onAboutToReload()
{
    QTC_CHECK(!m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = true;

    processor()->invalidateDiagnostics();
}

void CppEditorDocument::Private::onReloadFinished()
{
    QTC_CHECK(m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = false;

    m_processorRevision = document()->revision();
    processDocument();
}

void CppEditorDocument::Private::reparseWithPreferredParseContext(const QString &parseContextId)
{
    // Update parser
    q->setPreferredParseContext(parseContextId);

    // Remember the setting
    const Key key = Constants::PREFERRED_PARSE_CONTEXT + keyFromString(filePath().toUrlishString());
    Core::SessionManager::setValue(key, parseContextId);

    // Reprocess
    q->scheduleProcessDocument();
}

void CppEditorDocument::Private::onFilePathChanged(const FilePath &oldPath, const FilePath &newPath)
{
    Q_UNUSED(oldPath)

    if (!newPath.isEmpty()) {
        q->setMimeType(mimeTypeForFile(newPath).name());

        connect(q, &Core::IDocument::contentsChanged,
                q, &CppEditorDocument::scheduleProcessDocument,
                Qt::UniqueConnection);

        // Un-Register/Register in ModelManager
        m_editorDocumentHandle.reset();
        m_editorDocumentHandle.reset(new CppEditorDocumentHandleImpl(this));

        resetProcessor();
        applyPreferredParseContextFromSettings();
        applyExtraPreprocessorDirectivesFromSettings();
        m_processorRevision = document()->revision();
        processDocument();
    }
}

void CppEditorDocument::scheduleProcessDocument()
{
    if (d->m_fileIsBeingReloaded)
        return;

    d->m_processorRevision = document()->revision();
    d->m_processorTimer.start();
}

void CppEditorDocument::Private::processDocument()
{
    processor()->invalidateDiagnostics();

    if (processor()->isParserRunning() || m_processorRevision != contentsRevision()) {
        m_processorTimer.start();
        return;
    }

    m_processorTimer.stop();
    if (m_fileIsBeingReloaded || filePath().isEmpty())
        return;

    processor()->run();
}

void CppEditorDocument::Private::resetProcessor()
{
    releaseResources();
    processor(); // creates a new processor
}

void CppEditorDocument::Private::applyPreferredParseContextFromSettings()
{
    if (filePath().isEmpty())
        return;

    const Key key = Constants::PREFERRED_PARSE_CONTEXT + keyFromString(filePath().toUrlishString());
    const QString parseContextId = Core::SessionManager::value(key).toString();

    q->setPreferredParseContext(parseContextId);
}

void CppEditorDocument::Private::applyExtraPreprocessorDirectivesFromSettings()
{
    if (filePath().isEmpty())
        return;

    const Key key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + keyFromString(filePath().toUrlishString());
    const QByteArray directives = Core::SessionManager::value(key).toString().toUtf8();

    q->setExtraPreprocessorDirectives(directives);
}

void CppEditorDocument::setExtraPreprocessorDirectives(const QByteArray &directives)
{
    const auto parser = d->processor()->parser();
    QTC_ASSERT(parser, return);

    BaseEditorDocumentParser::Configuration config = parser->configuration();
    if (config.setEditorDefines(directives)) {
        d->processor()->setParserConfig(config);
        emit preprocessorSettingsChanged(!directives.trimmed().isEmpty());
    }
}

void CppEditorDocument::setIfdefedOutBlocks(const QList<TextEditor::BlockRange> &blocks)
{
    d->m_ifdefedOutBlocks = blocks;
    d->applyIfdefedOutBlocks();
}

void CppEditorDocument::Private::applyIfdefedOutBlocks()
{
    if (!q->syntaxHighlighter() || !q->syntaxHighlighter()->syntaxHighlighterUpToDate())
        return;

    auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = document()->firstBlock();
    bool needUpdate = false;
    int rangeNumber = 0;
    int previousBraceDepth = 0;
    while (block.isValid()) {
        bool resetToPrevious = false;
        if (rangeNumber < m_ifdefedOutBlocks.size()) {
            const BlockRange &range = m_ifdefedOutBlocks.at(rangeNumber);
            if (block.position() >= range.first()
                && ((block.position() + block.length() - 1) <= range.last() || !range.last())) {
                TextBlockUserData::setIfdefedOut(block);
                resetToPrevious = true;
            } else {
                TextBlockUserData::clearIfdefedOut(block);
                previousBraceDepth = TextBlockUserData::braceDepth(block);
                resetToPrevious = false;
            }
            if (block.contains(range.last()))
                ++rangeNumber;
        } else {
            TextBlockUserData::clearIfdefedOut(block);
            resetToPrevious = false;
        }

        // Do not change brace depth and folding indent in ifdefed-out code.
        if (resetToPrevious) {
            const int currentBraceDepth = TextBlockUserData::braceDepth(block);
            const int currentFoldingIndent = TextBlockUserData::foldingIndent(block);
            if (currentBraceDepth != previousBraceDepth
                || currentFoldingIndent != previousBraceDepth) {
                TextBlockUserData::setBraceDepth(block, previousBraceDepth);
                if (!q->syntaxHighlighter()->ignoresFolding())
                    TextBlockUserData::setFoldingIndent(block, previousBraceDepth);
                needUpdate = true;
                qCDebug(highlighterLog)
                    << "changing brace depth and folding indent to" << previousBraceDepth
                    << "for line" << (block.blockNumber() + 1) << "in ifdefed out code";
            }
        }

        block = block.next();
    }

    if (needUpdate)
        documentLayout->requestUpdate();

#ifdef WITH_TESTS
    emit q->ifdefedOutBlocksApplied();
#endif
}

void CppEditorDocument::setPreferredParseContext(const QString &parseContextId)
{
    const BaseEditorDocumentParser::Ptr parser = d->processor()->parser();
    QTC_ASSERT(parser, return);

    BaseEditorDocumentParser::Configuration config = parser->configuration();
    if (config.setPreferredProjectPartId(parseContextId))
        d->processor()->setParserConfig(config);
}

void CppEditorDocument::updateSoftPreferredParseContext(const Node *currentNode)
{
    if (!currentNode || filePath().isEmpty() || filePath() != currentNode->filePath())
        return;

    const BaseEditorDocumentParser::Ptr parser = d->processor()->parser();
    QTC_ASSERT(parser, return);

    const ProjectPartInfo projectPartInfo = parser->projectPartInfo();
    if (projectPartInfo.projectParts.size() < 2)
        return;

    const ProjectNode *projectNode = nullptr;
    for (projectNode = currentNode->parentProjectNode(); projectNode && !projectNode->isProduct();
         projectNode = projectNode->parentProjectNode()) {
        ;
    }
    if (!projectNode)
        return;
    QTC_ASSERT(projectNode->isProduct(), return);

    for (const auto &pp : projectPartInfo.projectParts) {
        if (!pp->buildSystemTarget.isEmpty() && pp->buildSystemTarget == projectNode->buildKey()) {
            BaseEditorDocumentParser::Configuration config = parser->configuration();
            if (config.setSoftPreferredProjectPartId(pp->id()))
                d->processor()->setParserConfig(config);
            break;
        }
    }
}

unsigned CppEditorDocument::Private::contentsRevision() const
{
    return document()->revision();
}

void CppEditorDocument::Private::releaseResources()
{
    if (m_processor)
        disconnect(m_processor.data(), nullptr, q, nullptr);
    m_processor.reset();
}

void CppEditorDocument::Private::showHideInfoBarAboutMultipleParseContexts(bool show)
{
    const Id id = Constants::MULTIPLE_PARSE_CONTEXTS_AVAILABLE;

    if (show) {
        InfoBarEntry info(id,
                          Tr::tr("Note: Multiple parse contexts are available for this file. "
                                 "Choose the preferred one from the editor toolbar."),
                          InfoBarEntry::GlobalSuppression::Enabled);
        info.removeCancelButton();
        if (q->infoBar()->canInfoBeAdded(id))
            q->infoBar()->addInfo(info);
    } else {
        q->infoBar()->removeInfo(id);
    }
}

void CppEditorDocument::Private::initializeTimer()
{
    m_processorTimer.setSingleShot(true);
    m_processorTimer.setInterval(processDocumentIntervalInMs);

    connect(&m_processorTimer, &QTimer::timeout,
            q, &CppEditorDocument::processDocument, Qt::UniqueConnection);
}

ParseContextModel &CppEditorDocument::parseContextModel()
{
    return d->m_parseContextModel;
}

OutlineModel &CppEditorDocument::outlineModel()
{
    return d->m_overviewModel;
}

void CppEditorDocument::updateOutline()
{
    CPlusPlus::Document::Ptr document;
    if (!usesClangd())
        document = CppModelManager::snapshot().document(filePath());
    d->m_overviewModel.update(document);
}

QFuture<CursorInfo> CppEditorDocument::cursorInfo(const CursorInfoParams &params)
{
    return d->processor()->cursorInfo(params);
}

BaseEditorDocumentProcessor *CppEditorDocument::Private::processor()
{
    if (!m_processor) {
        m_processor.reset(CppModelManager::createEditorDocumentProcessor(q));
        connect(m_processor.data(), &BaseEditorDocumentProcessor::projectPartInfoUpdated, q,
                [this](const ProjectPartInfo &info) {
                    const bool hasProjectPart = !(info.hints & ProjectPartInfo::IsFallbackMatch);
                    q->minimizableInfoBars()->setInfoVisible(NO_PROJECT_CONFIGURATION, !hasProjectPart);
                    updateInfoBarEntryIfVisible();
                    m_parseContextModel.update(info);
                    const bool isAmbiguous = info.hints & ProjectPartInfo::IsAmbiguousMatch;
                    const bool isProjectFile = info.hints & ProjectPartInfo::IsFromProjectMatch;
                    showHideInfoBarAboutMultipleParseContexts(isAmbiguous && isProjectFile);
                });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::codeWarningsUpdated, q,
                [this](unsigned revision,
                       const QList<QTextEdit::ExtraSelection> selections,
                       const TextEditor::RefactorMarkers &refactorMarkers) {
            emit q->codeWarningsUpdated(revision, selections, refactorMarkers);
        });
        connect(
            m_processor.data(),
            &BaseEditorDocumentProcessor::ifdefedOutBlocksUpdated,
            q,
            [this](unsigned revision, const QList<TextEditor::BlockRange> &ifdefedOutBlocks) {
                if (int(revision) == document()->revision())
                    q->setIfdefedOutBlocks(ifdefedOutBlocks);
            });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::cppDocumentUpdated, q,
                [this](const CPlusPlus::Document::Ptr document) {
                    // Update syntax highlighter
                    if (SyntaxHighlighter *highlighter = q->syntaxHighlighter())
                        highlighter->setLanguageFeaturesFlags(document->languageFeatures().flags);

                    m_overviewModel.update(q->usesClangd() ? nullptr : document);

                    // Forward signal
                    emit q->cppDocumentUpdated(document);

        });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::semanticInfoUpdated,
                q, &CppEditorDocument::semanticInfoUpdated);
    }

    return m_processor.data();
}

TextEditor::TabSettings CppEditorDocument::tabSettings() const
{
    return indenter()->tabSettings().value_or(TextEditor::TextDocument::tabSettings());
}

Result<> CppEditorDocument::saveImpl(const FilePath &filePath, SaveOption option)
{
    if (!indenter()->formatOnSave() || option != SaveOption::None)
        return TextEditor::TextDocument::saveImpl(filePath, option);

    auto *layout = qobject_cast<TextEditor::TextDocumentLayout *>(document()->documentLayout());
    const int documentRevision = layout->lastSaveRevision;

    TextEditor::RangesInLines editedRanges;
    TextEditor::RangeInLines lastRange{-1, -1};
    for (int i = 0; i < document()->blockCount(); ++i) {
        const QTextBlock block = document()->findBlockByNumber(i);
        if (block.revision() == documentRevision) {
            if (lastRange.startLine != -1)
                editedRanges.push_back(lastRange);

            lastRange.startLine = lastRange.endLine = -1;
            continue;
        }

        // block.revision() != documentRevision
        if (lastRange.startLine == -1)
            lastRange.startLine = block.blockNumber() + 1;
        lastRange.endLine = block.blockNumber() + 1;
    }

    if (lastRange.startLine != -1)
        editedRanges.push_back(lastRange);

    if (!editedRanges.empty()) {
        QTextCursor cursor(document());
        cursor.joinPreviousEditBlock();
        indenter()->format(editedRanges, Indenter::FormattingMode::Forced);
        cursor.endEditBlock();
    }

    TextEditor::StorageSettings settings = storageSettings();
    const QScopeGuard cleanup([this, settings] { setStorageSettings(settings); });
    settings.m_cleanWhitespace = false;
    setStorageSettings(settings);

    return TextEditor::TextDocument::saveImpl(filePath, option);
}

bool CppEditorDocument::usesClangd() const
{
    return CppModelManager::usesClangd(this).has_value();
}

void CppEditorDocument::Private::onDiagnosticsChanged(const FilePath &fileName, const QString &kind)
{
    if (fileName != filePath())
        return;

    TextMarks removedMarks = q->marks();

    const Utils::Id category = Utils::Id::fromString(kind);

    for (const auto &diagnostic : CppModelManager::diagnosticMessages()) {
        if (diagnostic.filePath() == filePath()) {
            auto it = std::find_if(std::begin(removedMarks),
                                   std::end(removedMarks),
                                   [&category, &diagnostic](TextMark *existing) {
                                       return (diagnostic.line() == existing->lineNumber()
                                               && diagnostic.text() == existing->lineAnnotation()
                                               && category == existing->category().id);
                                   });

            if (it != std::end(removedMarks)) {
                removedMarks.erase(it);
                continue;
            }

            auto mark = new TextMark(filePath(),
                                     diagnostic.line(),
                                     {Tr::tr("C++ Code Model"), category});
            mark->setLineAnnotation(diagnostic.text());
            mark->setToolTip(diagnostic.text());

            mark->setIcon(diagnostic.isWarning() ? Utils::Icons::CODEMODEL_WARNING.icon()
                                                 : Utils::Icons::CODEMODEL_ERROR.icon());
            mark->setColor(diagnostic.isWarning() ? Utils::Theme::CodeModel_Warning_TextMarkColor
                                                  : Utils::Theme::CodeModel_Error_TextMarkColor);
            mark->setPriority(diagnostic.isWarning() ? TextEditor::TextMark::NormalPriority
                                                     : TextEditor::TextMark::HighPriority);
            q->addMark(mark);
        }
    }

    for (auto it = removedMarks.begin(); it != removedMarks.end(); ++it) {
        if ((*it)->category().id == category) {
            q->removeMark(*it);
            delete *it;
        }
    }
}

void CppEditorDocument::Private::updateInfoBarEntryIfVisible()
{
    if (q->minimizableInfoBars()->isShownInInfoBar(NO_PROJECT_CONFIGURATION))
        q->minimizableInfoBars()->updateEntry(createInfoBarEntry(filePath()));
}

#ifdef WITH_TESTS

QList<BlockRange> CppEditorDocument::ifdefedOutBlocks() const
{
    return d->m_ifdefedOutBlocks;
}

#endif

} // namespace CppEditor
