// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppeditordocument.h"

#include "baseeditordocumentparser.h"
#include "cppcodeformatter.h"
#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"
#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppeditortr.h"
#include "cpphighlighter.h"
#include "cppquickfixassistant.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/session.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/storagesettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <utils/infobar.h>
#include <utils/mimeutils.h>
#include <utils/minimizableinfobars.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QScopeGuard>
#include <QTextDocument>

const char NO_PROJECT_CONFIGURATION[] = "NoProject";

using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

enum { processDocumentIntervalInMs = 150 };

class CppEditorDocumentHandleImpl : public CppEditorDocumentHandle
{
public:
    CppEditorDocumentHandleImpl(CppEditorDocument *cppEditorDocument)
        : m_cppEditorDocument(cppEditorDocument)
        , m_registrationFilePath(cppEditorDocument->filePath().toString())
    {
        CppModelManager::registerCppEditorDocument(this);
    }

    ~CppEditorDocumentHandleImpl() override
    {
        CppModelManager::unregisterCppEditorDocument(m_registrationFilePath);
    }

    FilePath filePath() const override { return m_cppEditorDocument->filePath(); }
    QByteArray contents() const override { return m_cppEditorDocument->contentsText(); }
    unsigned revision() const override { return m_cppEditorDocument->contentsRevision(); }

    BaseEditorDocumentProcessor *processor() const override
    { return m_cppEditorDocument->processor(); }

    void resetProcessor() override
    { m_cppEditorDocument->resetProcessor(); }

private:
    CppEditor::Internal::CppEditorDocument * const m_cppEditorDocument;
    // The file path of the editor document can change (e.g. by "Save As..."), so make sure
    // that un-registration happens with the path the document was registered.
    const QString m_registrationFilePath;
};

CppEditorDocument::CppEditorDocument()
{
    setId(CppEditor::Constants::CPPEDITOR_ID);
    setSyntaxHighlighter(new CppHighlighter);

    ICodeStylePreferencesFactory *factory
        = TextEditorSettings::codeStyleFactory(Constants::CPP_SETTINGS_ID);
    setIndenter(factory->createIndenter(document()));

    connect(this, &TextEditor::TextDocument::tabSettingsChanged,
            this, &CppEditorDocument::invalidateFormatterCache);
    connect(this, &Core::IDocument::mimeTypeChanged,
            this, &CppEditorDocument::onMimeTypeChanged);

    connect(this, &Core::IDocument::aboutToReload,
            this, &CppEditorDocument::onAboutToReload);
    connect(this, &Core::IDocument::reloadFinished,
            this, &CppEditorDocument::onReloadFinished);
    connect(this, &IDocument::filePathChanged,
            this, &CppEditorDocument::onFilePathChanged);

    connect(CppModelManager::instance(), &CppModelManager::diagnosticsChanged,
            this, &CppEditorDocument::onDiagnosticsChanged);

    connect(&m_parseContextModel, &ParseContextModel::preferredParseContextChanged,
            this, &CppEditorDocument::reparseWithPreferredParseContext);

    minimizableInfoBars()->setSettingsGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    minimizableInfoBars()->setPossibleInfoBarEntries(
        {{NO_PROJECT_CONFIGURATION,
          Tr::tr("<b>Warning</b>: This file is not part of any project. "
                 "The code model might have issues parsing this file properly.")}});

    // See also onFilePathChanged() for more initialization
}

bool CppEditorDocument::isObjCEnabled() const
{
    return m_isObjCEnabled;
}

void CppEditorDocument::setCompletionAssistProvider(TextEditor::CompletionAssistProvider *provider)
{
    TextDocument::setCompletionAssistProvider(provider);
    m_completionAssistProvider = nullptr;
}

CompletionAssistProvider *CppEditorDocument::completionAssistProvider() const
{
    return m_completionAssistProvider
            ? m_completionAssistProvider : TextDocument::completionAssistProvider();
}

TextEditor::IAssistProvider *CppEditorDocument::quickFixAssistProvider() const
{
    if (const auto baseProvider = TextDocument::quickFixAssistProvider())
        return baseProvider;
    return CppEditorPlugin::instance()->quickFixProvider();
}

void CppEditorDocument::recalculateSemanticInfoDetached()
{
    BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, return);
    p->recalculateSemanticInfoDetached(true);
}

SemanticInfo CppEditorDocument::recalculateSemanticInfo()
{
    BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, return SemanticInfo());
    return p->recalculateSemanticInfo();
}

QByteArray CppEditorDocument::contentsText() const
{
    QMutexLocker locker(&m_cachedContentsLock);

    const int currentRevision = document()->revision();
    if (m_cachedContentsRevision != currentRevision && !m_fileIsBeingReloaded) {
        m_cachedContentsRevision = currentRevision;
        m_cachedContents = plainText().toUtf8();
    }

    return m_cachedContents;
}

void CppEditorDocument::applyFontSettings()
{
    if (TextEditor::SyntaxHighlighter *highlighter = syntaxHighlighter())
        highlighter->clearAllExtraFormats(); // Clear all additional formats since they may have changed
    TextDocument::applyFontSettings(); // rehighlights and updates additional formats
    if (m_processor)
        m_processor->semanticRehighlight();
}

void CppEditorDocument::invalidateFormatterCache()
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorDocument::onMimeTypeChanged()
{
    const QString &mt = mimeType();
    m_isObjCEnabled = (mt == QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
                       || mt == QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    m_completionAssistProvider = CppModelManager::completionAssistProvider();

    initializeTimer();
}

void CppEditorDocument::onAboutToReload()
{
    QTC_CHECK(!m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = true;

    processor()->invalidateDiagnostics();
}

void CppEditorDocument::onReloadFinished()
{
    QTC_CHECK(m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = false;

    m_processorRevision = document()->revision();
    processDocument();
}

void CppEditorDocument::reparseWithPreferredParseContext(const QString &parseContextId)
{
    // Update parser
    setPreferredParseContext(parseContextId);

    // Remember the setting
    const Key key = Constants::PREFERRED_PARSE_CONTEXT + keyFromString(filePath().toString());
    Core::SessionManager::setValue(key, parseContextId);

    // Reprocess
    scheduleProcessDocument();
}

void CppEditorDocument::onFilePathChanged(const FilePath &oldPath, const FilePath &newPath)
{
    Q_UNUSED(oldPath)

    if (!newPath.isEmpty()) {
        indenter()->setFileName(newPath);
        setMimeType(mimeTypeForFile(newPath).name());

        connect(this, &Core::IDocument::contentsChanged,
                this, &CppEditorDocument::scheduleProcessDocument,
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
    if (m_fileIsBeingReloaded)
        return;

    m_processorRevision = document()->revision();
    m_processorTimer.start();
}

void CppEditorDocument::processDocument()
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

void CppEditorDocument::resetProcessor()
{
    releaseResources();
    processor(); // creates a new processor
}

void CppEditorDocument::applyPreferredParseContextFromSettings()
{
    if (filePath().isEmpty())
        return;

    const Key key = Constants::PREFERRED_PARSE_CONTEXT + keyFromString(filePath().toString());
    const QString parseContextId = Core::SessionManager::value(key).toString();

    setPreferredParseContext(parseContextId);
}

void CppEditorDocument::applyExtraPreprocessorDirectivesFromSettings()
{
    if (filePath().isEmpty())
        return;

    const Key key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + keyFromString(filePath().toString());
    const QByteArray directives = Core::SessionManager::value(key).toString().toUtf8();

    setExtraPreprocessorDirectives(directives);
}

void CppEditorDocument::setExtraPreprocessorDirectives(const QByteArray &directives)
{
    const auto parser = processor()->parser();
    QTC_ASSERT(parser, return);

    BaseEditorDocumentParser::Configuration config = parser->configuration();
    if (config.editorDefines != directives) {
        config.editorDefines = directives;
        processor()->setParserConfig(config);

        emit preprocessorSettingsChanged(!directives.trimmed().isEmpty());
    }
}

void CppEditorDocument::setPreferredParseContext(const QString &parseContextId)
{
    const BaseEditorDocumentParser::Ptr parser = processor()->parser();
    QTC_ASSERT(parser, return);

    BaseEditorDocumentParser::Configuration config = parser->configuration();
    if (config.preferredProjectPartId != parseContextId) {
        config.preferredProjectPartId = parseContextId;
        processor()->setParserConfig(config);
    }
}

unsigned CppEditorDocument::contentsRevision() const
{
    return document()->revision();
}

void CppEditorDocument::releaseResources()
{
    if (m_processor)
        disconnect(m_processor.data(), nullptr, this, nullptr);
    m_processor.reset();
}

void CppEditorDocument::showHideInfoBarAboutMultipleParseContexts(bool show)
{
    const Id id = Constants::MULTIPLE_PARSE_CONTEXTS_AVAILABLE;

    if (show) {
        InfoBarEntry info(id,
                          Tr::tr("Note: Multiple parse contexts are available for this file. "
                                 "Choose the preferred one from the editor toolbar."),
                          InfoBarEntry::GlobalSuppression::Enabled);
        info.removeCancelButton();
        if (infoBar()->canInfoBeAdded(id))
            infoBar()->addInfo(info);
    } else {
        infoBar()->removeInfo(id);
    }
}

void CppEditorDocument::initializeTimer()
{
    m_processorTimer.setSingleShot(true);
    m_processorTimer.setInterval(processDocumentIntervalInMs);

    connect(&m_processorTimer,
            &QTimer::timeout,
            this,
            &CppEditorDocument::processDocument,
            Qt::UniqueConnection);
}

ParseContextModel &CppEditorDocument::parseContextModel()
{
    return m_parseContextModel;
}

OutlineModel &CppEditorDocument::outlineModel()
{
    return m_overviewModel;
}

void CppEditorDocument::updateOutline()
{
    CPlusPlus::Document::Ptr document;
    if (!usesClangd())
        document = CppModelManager::snapshot().document(filePath());
    m_overviewModel.update(document);
}

QFuture<CursorInfo> CppEditorDocument::cursorInfo(const CursorInfoParams &params)
{
    return processor()->cursorInfo(params);
}

BaseEditorDocumentProcessor *CppEditorDocument::processor()
{
    if (!m_processor) {
        m_processor.reset(CppModelManager::createEditorDocumentProcessor(this));
        connect(m_processor.data(), &BaseEditorDocumentProcessor::projectPartInfoUpdated, this,
                [this](const ProjectPartInfo &info) {
                    const bool hasProjectPart = !(info.hints & ProjectPartInfo::IsFallbackMatch);
                    minimizableInfoBars()->setInfoVisible(NO_PROJECT_CONFIGURATION, !hasProjectPart);
                    m_parseContextModel.update(info);
                    const bool isAmbiguous = info.hints & ProjectPartInfo::IsAmbiguousMatch;
                    const bool isProjectFile = info.hints & ProjectPartInfo::IsFromProjectMatch;
                    showHideInfoBarAboutMultipleParseContexts(isAmbiguous && isProjectFile);
                });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::codeWarningsUpdated, this,
                [this](unsigned revision,
                       const QList<QTextEdit::ExtraSelection> selections,
                       const TextEditor::RefactorMarkers &refactorMarkers) {
            emit codeWarningsUpdated(revision, selections, refactorMarkers);
        });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::ifdefedOutBlocksUpdated,
                this, &CppEditorDocument::ifdefedOutBlocksUpdated);
        connect(m_processor.data(), &BaseEditorDocumentProcessor::cppDocumentUpdated, this,
                [this](const CPlusPlus::Document::Ptr document) {
                    // Update syntax highlighter
                    auto *highlighter = qobject_cast<CppHighlighter *>(syntaxHighlighter());
                    highlighter->setLanguageFeatures(document->languageFeatures());

                    m_overviewModel.update(usesClangd() ? nullptr : document);

                    // Forward signal
                    emit cppDocumentUpdated(document);

        });
        connect(m_processor.data(), &BaseEditorDocumentProcessor::semanticInfoUpdated,
                this, &CppEditorDocument::semanticInfoUpdated);
    }

    return m_processor.data();
}

TextEditor::TabSettings CppEditorDocument::tabSettings() const
{
    return indenter()->tabSettings().value_or(TextEditor::TextDocument::tabSettings());
}

bool CppEditorDocument::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    if (!indenter()->formatOnSave() || autoSave)
        return TextEditor::TextDocument::saveImpl(errorString, filePath, autoSave);

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

    return TextEditor::TextDocument::saveImpl(errorString, filePath, autoSave);
}

bool CppEditorDocument::usesClangd() const
{
    return CppModelManager::usesClangd(this);
}

void CppEditorDocument::onDiagnosticsChanged(const FilePath &fileName, const QString &kind)
{
    if (fileName != filePath())
        return;

    TextMarks removedMarks = marks();

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
            addMark(mark);
        }
    }

    for (auto it = removedMarks.begin(); it != removedMarks.end(); ++it) {
        if ((*it)->category().id == category) {
            removeMark(*it);
            delete *it;
        }
    }
}

} // namespace Internal
} // namespace CppEditor
