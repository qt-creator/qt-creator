/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppeditordocument.h"

#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cpphighlighter.h"
#include "cppquickfixassistant.h"

#include <coreplugin/infobar.h>

#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsplugin.h>

#include <projectexplorer/session.h>

#include <coreplugin/editormanager/editormanager.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextDocument>

namespace {

CppTools::CppModelManager *mm()
{
    return CppTools::CppModelManager::instance();
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

enum { processDocumentIntervalInMs = 150 };

class CppEditorDocumentHandleImpl : public CppTools::CppEditorDocumentHandle
{
public:
    CppEditorDocumentHandleImpl(CppEditorDocument *cppEditorDocument)
        : m_cppEditorDocument(cppEditorDocument)
        , m_registrationFilePath(cppEditorDocument->filePath().toString())
    {
        mm()->registerCppEditorDocument(this);
    }

    ~CppEditorDocumentHandleImpl() { mm()->unregisterCppEditorDocument(m_registrationFilePath); }

    QString filePath() const override { return m_cppEditorDocument->filePath().toString(); }
    QByteArray contents() const override { return m_cppEditorDocument->contentsText(); }
    unsigned revision() const override { return m_cppEditorDocument->contentsRevision(); }

    CppTools::BaseEditorDocumentProcessor *processor() const override
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
    : m_fileIsBeingReloaded(false)
    , m_isObjCEnabled(false)
    , m_cachedContentsRevision(-1)
    , m_processorRevision(0)
    , m_completionAssistProvider(0)
    , m_minimizableInfoBars(*infoBar())
{
    setId(CppEditor::Constants::CPPEDITOR_ID);
    setSyntaxHighlighter(new CppHighlighter);
    setIndenter(new CppTools::CppQtStyleIndenter);

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

    connect(&m_parseContextModel, &ParseContextModel::preferredParseContextChanged,
            this, &CppEditorDocument::reparseWithPreferredParseContext);

    // See also onFilePathChanged() for more initialization
}

bool CppEditorDocument::isObjCEnabled() const
{
    return m_isObjCEnabled;
}

TextEditor::CompletionAssistProvider *CppEditorDocument::completionAssistProvider() const
{
    return m_completionAssistProvider;
}

TextEditor::QuickFixAssistProvider *CppEditorDocument::quickFixAssistProvider() const
{
    return CppEditorPlugin::instance()->quickFixProvider();
}

void CppEditorDocument::recalculateSemanticInfoDetached()
{
    CppTools::BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, return);
    p->recalculateSemanticInfoDetached(true);
}

CppTools::SemanticInfo CppEditorDocument::recalculateSemanticInfo()
{
    CppTools::BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, CppTools::SemanticInfo());
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
    if (TextEditor::SyntaxHighlighter *highlighter = syntaxHighlighter()) {
        // Clear all additional formats since they may have changed
        QTextBlock b = document()->firstBlock();
        while (b.isValid()) {
            QVector<QTextLayout::FormatRange> noFormats;
            highlighter->setExtraFormats(b, noFormats);
            b = b.next();
        }
    }
    TextDocument::applyFontSettings(); // rehighlights and updates additional formats
    if (m_processor)
        m_processor->semanticRehighlight();
}

void CppEditorDocument::invalidateFormatterCache()
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorDocument::onMimeTypeChanged()
{
    const QString &mt = mimeType();
    m_isObjCEnabled = (mt == QLatin1String(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
                       || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    m_completionAssistProvider = mm()->completionAssistProvider();

    initializeTimer();
}

void CppEditorDocument::onAboutToReload()
{
    QTC_CHECK(!m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = true;
}

void CppEditorDocument::onReloadFinished()
{
    QTC_CHECK(m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = false;
}

void CppEditorDocument::reparseWithPreferredParseContext(const QString &parseContextId)
{
    using namespace CppTools;

    // Update parser
    setPreferredParseContext(parseContextId);

    // Remember the setting
    const QString key = Constants::PREFERRED_PARSE_CONTEXT + filePath().toString();
    ProjectExplorer::SessionManager::setValue(key, parseContextId);

    // Reprocess
    scheduleProcessDocument();
}

void CppEditorDocument::onFilePathChanged(const Utils::FileName &oldPath,
                                          const Utils::FileName &newPath)
{
    Q_UNUSED(oldPath);

    if (!newPath.isEmpty()) {
        setMimeType(Utils::mimeTypeForFile(newPath.toFileInfo()).name());

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
    m_processorRevision = document()->revision();
    m_processorTimer.start();
    processor()->editorDocumentTimerRestarted();
}

void CppEditorDocument::processDocument()
{
    if (processor()->isParserRunning() || m_processorRevision != contentsRevision()) {
        m_processorTimer.start();
        processor()->editorDocumentTimerRestarted();
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

    const QString key = Constants::PREFERRED_PARSE_CONTEXT + filePath().toString();
    const QString parseContextId = ProjectExplorer::SessionManager::value(key).toString();

    setPreferredParseContext(parseContextId);
}

void CppEditorDocument::applyExtraPreprocessorDirectivesFromSettings()
{
    if (filePath().isEmpty())
        return;

    const QString key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + filePath().toString();
    const QByteArray directives = ProjectExplorer::SessionManager::value(key).toString().toUtf8();

    setExtraPreprocessorDirectives(directives);
}

void CppEditorDocument::setExtraPreprocessorDirectives(const QByteArray &directives)
{
    const auto parser = processor()->parser();
    QTC_ASSERT(parser, return);

    CppTools::BaseEditorDocumentParser::Configuration config = parser->configuration();
    if (config.editorDefines != directives) {
        config.editorDefines = directives;
        processor()->setParserConfig(config);

        emit preprocessorSettingsChanged(!directives.trimmed().isEmpty());
    }
}

void CppEditorDocument::setPreferredParseContext(const QString &parseContextId)
{
    const CppTools::BaseEditorDocumentParser::Ptr parser = processor()->parser();
    QTC_ASSERT(parser, return);

    CppTools::BaseEditorDocumentParser::Configuration config = parser->configuration();
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
        disconnect(m_processor.data(), 0, this, 0);
    m_processor.reset();
}

void CppEditorDocument::showHideInfoBarAboutMultipleParseContexts(bool show)
{
    const Core::Id id = Constants::MULTIPLE_PARSE_CONTEXTS_AVAILABLE;

    if (show) {
        Core::InfoBarEntry info(id,
                                tr("Note: Multiple parse contexts are available for this file. "
                                   "Choose the preferred one from the editor toolbar."),
                                Core::InfoBarEntry::GlobalSuppressionEnabled);
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

const MinimizableInfoBars &CppEditorDocument::minimizableInfoBars() const
{
    return m_minimizableInfoBars;
}

CppTools::BaseEditorDocumentProcessor *CppEditorDocument::processor()
{
    if (!m_processor) {
        m_processor.reset(mm()->editorDocumentProcessor(this));
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::projectPartInfoUpdated,
                [this] (const CppTools::ProjectPartInfo &info)
        {
            using namespace CppTools;
            const bool hasProjectPart = !(info.hints & ProjectPartInfo::IsFallbackMatch);
            m_minimizableInfoBars.processHasProjectPart(hasProjectPart);
            m_parseContextModel.update(info);
            const bool isAmbiguous = info.hints & ProjectPartInfo::IsAmbiguousMatch;
            const bool isProjectFile = info.hints & ProjectPartInfo::IsFromProjectMatch;
            showHideInfoBarAboutMultipleParseContexts(isAmbiguous && isProjectFile);
        });
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::codeWarningsUpdated,
                [this] (unsigned revision,
                        const QList<QTextEdit::ExtraSelection> selections,
                        const std::function<QWidget*()> &creator,
                        const TextEditor::RefactorMarkers &refactorMarkers) {
            emit codeWarningsUpdated(revision, selections, refactorMarkers);
            m_minimizableInfoBars.processHeaderDiagnostics(creator);
        });
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::ifdefedOutBlocksUpdated,
                this, &CppEditorDocument::ifdefedOutBlocksUpdated);
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::cppDocumentUpdated,
                this, &CppEditorDocument::cppDocumentUpdated);
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::semanticInfoUpdated,
                this, &CppEditorDocument::semanticInfoUpdated);
    }

    return m_processor.data();
}

} // namespace Internal
} // namespace CppEditor
