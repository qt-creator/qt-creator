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

#include "clangeditordocumentprocessor.h"

#include "clangbackendcommunicator.h"
#include "clangdiagnostictooltipwidget.h"
#include "clangfixitoperation.h"
#include "clangfixitoperationsextractor.h"
#include "clangmodelmanagersupport.h"
#include "clanghighlightingresultreporter.h"
#include "clangprojectsettings.h"
#include "clangutils.h"

#include <diagnosticcontainer.h>
#include <sourcelocationcontainer.h>

#include <cpptools/builtincursorinfo.h>
#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsbridge.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppworkingcopy.h>
#include <cpptools/editordocumenthandle.h>

#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>

#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextBlock>
#include <QVBoxLayout>
#include <QWidget>

namespace ClangCodeModel {
namespace Internal {

static ClangProjectSettings &getProjectSettings(ProjectExplorer::Project *project)
{
    QTC_CHECK(project);
    return ModelManagerSupportClang::instance()->projectSettings(project);
}

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(
        BackendCommunicator &communicator,
        TextEditor::TextDocument *document)
    : BaseEditorDocumentProcessor(document->document(), document->filePath().toString())
    , m_document(*document)
    , m_diagnosticManager(document)
    , m_communicator(communicator)
    , m_parser(new ClangEditorDocumentParser(document->filePath().toString()))
    , m_parserRevision(0)
    , m_semanticHighlighter(document)
    , m_builtinProcessor(document, /*enableSemanticHighlighter=*/ false)
{
    m_updateBackendDocumentTimer.setSingleShot(true);
    m_updateBackendDocumentTimer.setInterval(350);
    connect(&m_updateBackendDocumentTimer, &QTimer::timeout,
            this, &ClangEditorDocumentProcessor::updateBackendDocumentIfProjectPartExists);

    connect(m_parser.data(), &ClangEditorDocumentParser::projectPartInfoUpdated,
            this, &BaseEditorDocumentProcessor::projectPartInfoUpdated);

    // Forwarding the semantic info from the builtin processor enables us to provide all
    // editor (widget) related features that are not yet implemented by the clang plugin.
    connect(&m_builtinProcessor, &CppTools::BuiltinEditorDocumentProcessor::cppDocumentUpdated,
            this, &ClangEditorDocumentProcessor::cppDocumentUpdated);
    connect(&m_builtinProcessor, &CppTools::BuiltinEditorDocumentProcessor::semanticInfoUpdated,
            this, &ClangEditorDocumentProcessor::semanticInfoUpdated);
}

ClangEditorDocumentProcessor::~ClangEditorDocumentProcessor()
{
    m_updateBackendDocumentTimer.stop();

    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();

    if (m_projectPart)
        closeBackendDocument();
}

void ClangEditorDocumentProcessor::runImpl(
        const CppTools::BaseEditorDocumentParser::UpdateParams &updateParams)
{
    m_updateBackendDocumentTimer.start();

    // Run clang parser
    disconnect(&m_parserWatcher, &QFutureWatcher<void>::finished,
               this, &ClangEditorDocumentProcessor::onParserFinished);
    m_parserWatcher.cancel();
    m_parserWatcher.setFuture(QFuture<void>());

    m_parserRevision = revision();
    connect(&m_parserWatcher, &QFutureWatcher<void>::finished,
            this, &ClangEditorDocumentProcessor::onParserFinished);
    const QFuture<void> future = ::Utils::runAsync(&runParser, parser(), updateParams);
    m_parserWatcher.setFuture(future);

    // Run builtin processor
    m_builtinProcessor.runImpl(updateParams);
}

void ClangEditorDocumentProcessor::recalculateSemanticInfoDetached(bool force)
{
    m_builtinProcessor.recalculateSemanticInfoDetached(force);
}

void ClangEditorDocumentProcessor::semanticRehighlight()
{
    m_semanticHighlighter.updateFormatMapFromFontSettings();

    if (m_projectPart)
        requestAnnotationsFromBackend(m_projectPart->id());
}

CppTools::SemanticInfo ClangEditorDocumentProcessor::recalculateSemanticInfo()
{
    return m_builtinProcessor.recalculateSemanticInfo();
}

CppTools::BaseEditorDocumentParser::Ptr ClangEditorDocumentProcessor::parser()
{
    return m_parser;
}

CPlusPlus::Snapshot ClangEditorDocumentProcessor::snapshot()
{
   return m_builtinProcessor.snapshot();
}

bool ClangEditorDocumentProcessor::isParserRunning() const
{
    return m_parserWatcher.isRunning();
}

bool ClangEditorDocumentProcessor::hasProjectPart() const
{
    return m_projectPart;
}

CppTools::ProjectPart::Ptr ClangEditorDocumentProcessor::projectPart() const
{
    return m_projectPart;
}

void ClangEditorDocumentProcessor::clearProjectPart()
{
    m_projectPart.clear();
}

Core::Id ClangEditorDocumentProcessor::diagnosticConfigId() const
{
    return m_diagnosticConfigId;
}

void ClangEditorDocumentProcessor::updateCodeWarnings(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
        const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic,
        uint documentRevision)
{
    if (documentRevision == revision()) {
        m_diagnosticManager.processNewDiagnostics(diagnostics, m_isProjectFile);
        const auto codeWarnings = m_diagnosticManager.takeExtraSelections();
        const auto fixitAvailableMarkers = m_diagnosticManager.takeFixItAvailableMarkers();
        const auto creator = creatorForHeaderErrorDiagnosticWidget(firstHeaderErrorDiagnostic);

        emit codeWarningsUpdated(revision(),
                                 codeWarnings,
                                 creator,
                                 fixitAvailableMarkers);
    }
}
namespace {

int positionInText(QTextDocument *textDocument,
                   const ClangBackEnd::SourceLocationContainer &sourceLocationContainer)
{
    auto textBlock = textDocument->findBlockByNumber(int(sourceLocationContainer.line) - 1);

    return textBlock.position() + int(sourceLocationContainer.column) - 1;
}

TextEditor::BlockRange
toTextEditorBlock(QTextDocument *textDocument,
                  const ClangBackEnd::SourceRangeContainer &sourceRangeContainer)
{
    return TextEditor::BlockRange(positionInText(textDocument, sourceRangeContainer.start),
                                  positionInText(textDocument, sourceRangeContainer.end));
}

QList<TextEditor::BlockRange>
toTextEditorBlocks(QTextDocument *textDocument,
                   const QVector<ClangBackEnd::SourceRangeContainer> &ifdefedOutRanges)
{
    QList<TextEditor::BlockRange> blockRanges;
    blockRanges.reserve(ifdefedOutRanges.size());

    for (const auto &range : ifdefedOutRanges)
        blockRanges.append(toTextEditorBlock(textDocument, range));

    return blockRanges;
}
}

const QVector<ClangBackEnd::TokenInfoContainer>
&ClangEditorDocumentProcessor::tokenInfos() const
{
    return m_tokenInfos;
}

void ClangEditorDocumentProcessor::clearTaskHubIssues()
{
    ClangDiagnosticManager::clearTaskHubIssues();
}

void ClangEditorDocumentProcessor::generateTaskHubIssues()
{
    m_diagnosticManager.generateTaskHubIssues();
}

void ClangEditorDocumentProcessor::updateHighlighting(
        const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
        const QVector<ClangBackEnd::SourceRangeContainer> &skippedPreprocessorRanges,
        uint documentRevision)
{
    if (documentRevision == revision()) {
        const auto skippedPreprocessorBlocks = toTextEditorBlocks(textDocument(), skippedPreprocessorRanges);
        emit ifdefedOutBlocksUpdated(documentRevision, skippedPreprocessorBlocks);

        m_semanticHighlighter.setHighlightingRunner(
            [tokenInfos]() {
                auto *reporter = new HighlightingResultReporter(tokenInfos);
                return reporter->start();
            });
        m_semanticHighlighter.run();
    }
}

void ClangEditorDocumentProcessor::updateTokenInfos(
        const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
        uint documentRevision)
{
    if (documentRevision != revision())
        return;
    m_tokenInfos = tokenInfos;
    emit tokenInfosUpdated();
}

static int currentLine(const TextEditor::AssistInterface &assistInterface)
{
    int line, column;
    ::Utils::Text::convertPosition(assistInterface.textDocument(), assistInterface.position(),
                                   &line, &column);
    return line;
}

TextEditor::QuickFixOperations ClangEditorDocumentProcessor::extraRefactoringOperations(
        const TextEditor::AssistInterface &assistInterface)
{
    ClangFixItOperationsExtractor extractor(m_diagnosticManager.diagnosticsWithFixIts());

    return extractor.extract(assistInterface.fileName(), currentLine(assistInterface));
}

bool ClangEditorDocumentProcessor::hasDiagnosticsAt(uint line, uint column) const
{
    return m_diagnosticManager.hasDiagnosticsAt(line, column);
}

void ClangEditorDocumentProcessor::addDiagnosticToolTipToLayout(uint line,
                                                                uint column,
                                                                QLayout *target) const
{
    using Internal::ClangDiagnosticWidget;

    const QVector<ClangBackEnd::DiagnosticContainer> diagnostics
        = m_diagnosticManager.diagnosticsAt(line, column);

    target->addWidget(ClangDiagnosticWidget::create(diagnostics, ClangDiagnosticWidget::ToolTip));
    auto link = TextEditor::DisplaySettings::createAnnotationSettingsLink();
    target->addWidget(link);
    target->setAlignment(link, Qt::AlignRight);
}

void ClangEditorDocumentProcessor::editorDocumentTimerRestarted()
{
    m_updateBackendDocumentTimer.stop(); // Wait for the next call to run().
}

void ClangEditorDocumentProcessor::invalidateDiagnostics()
{
    m_diagnosticManager.invalidateDiagnostics();
}

void ClangEditorDocumentProcessor::setParserConfig(
        const CppTools::BaseEditorDocumentParser::Configuration config)
{
    m_parser->setConfiguration(config);
    m_builtinProcessor.parser()->setConfiguration(config);
}

static bool isCursorOnIdentifier(const QTextCursor &textCursor)
{
    QTextDocument *document = textCursor.document();
    return CppTools::isValidIdentifierChar(document->characterAt(textCursor.position()));
}

static QFuture<CppTools::CursorInfo> defaultCursorInfoFuture()
{
    QFutureInterface<CppTools::CursorInfo> futureInterface;
    futureInterface.reportResult(CppTools::CursorInfo());
    futureInterface.reportFinished();

    return futureInterface.future();
}

static bool convertPosition(const QTextCursor &textCursor, int *line, int *column)
{
    const bool converted = ::Utils::Text::convertPosition(textCursor.document(),
                                                          textCursor.position(),
                                                          line,
                                                          column);
    QTC_CHECK(converted);
    return converted;
}

QFuture<CppTools::CursorInfo>
ClangEditorDocumentProcessor::cursorInfo(const CppTools::CursorInfoParams &params)
{
    int line, column;
    convertPosition(params.textCursor, &line, &column);

    if (!isCursorOnIdentifier(params.textCursor))
        return defaultCursorInfoFuture();

    column = Utils::clangColumn(params.textCursor.document()->findBlockByNumber(line - 1), column);
    const CppTools::SemanticInfo::LocalUseMap localUses
        = CppTools::BuiltinCursorInfo::findLocalUses(params.semanticInfo.doc, line, column);

    return m_communicator.requestReferences(simpleFileContainer(),
                                            static_cast<quint32>(line),
                                            static_cast<quint32>(column),
                                            localUses);
}

QFuture<CppTools::CursorInfo> ClangEditorDocumentProcessor::requestLocalReferences(
        const QTextCursor &cursor)
{
    int line, column;
    convertPosition(cursor, &line, &column);
    ++column; // for 1-based columns

    // TODO: check that by highlighting items
    if (!isCursorOnIdentifier(cursor))
        return defaultCursorInfoFuture();

    return m_communicator.requestLocalReferences(simpleFileContainer(),
                                                 static_cast<quint32>(line),
                                                 static_cast<quint32>(column));
}

QFuture<CppTools::SymbolInfo>
ClangEditorDocumentProcessor::requestFollowSymbol(int line, int column)
{
    return m_communicator.requestFollowSymbol(simpleFileContainer(),
                                              static_cast<quint32>(line),
                                              static_cast<quint32>(column));
}

QFuture<CppTools::ToolTipInfo> ClangEditorDocumentProcessor::toolTipInfo(const QByteArray &codecName,
                                                                         int line,
                                                                         int column)
{
    return m_communicator.requestToolTip(simpleFileContainer(codecName),
                                         static_cast<quint32>(line),
                                         static_cast<quint32>(column));
}

void ClangEditorDocumentProcessor::clearDiagnosticsWithFixIts()
{
    m_diagnosticManager.clearDiagnosticsWithFixIts();
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    auto *processor = CppTools::CppToolsBridge::baseEditorDocumentProcessor(filePath);

    return qobject_cast<ClangEditorDocumentProcessor*>(processor);
}

static bool isProjectPartLoadedOrIsFallback(CppTools::ProjectPart::Ptr projectPart)
{
    return projectPart
        && (projectPart->id().isEmpty() || ClangCodeModel::Utils::isProjectPartLoaded(projectPart));
}

void ClangEditorDocumentProcessor::updateBackendProjectPartAndDocument()
{
    const CppTools::ProjectPart::Ptr projectPart = m_parser->projectPartInfo().projectPart;

    if (isProjectPartLoadedOrIsFallback(projectPart)) {
        updateBackendDocument(*projectPart.data());

        m_projectPart = projectPart;
        m_isProjectFile = m_parser->projectPartInfo().hints
                & CppTools::ProjectPartInfo::IsFromProjectMatch;
    }
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    updateBackendProjectPartAndDocument();
}

namespace {
// TODO: Can we marry this with CompilerOptionsBuilder?
class FileOptionsBuilder
{
public:
    FileOptionsBuilder(const QString &filePath, CppTools::ProjectPart &projectPart)
        : m_filePath(filePath)
        , m_projectPart(projectPart)
    {
        addLanguageOptions();
        addGlobalDiagnosticOptions(); // Before addDiagnosticOptions() so users still can overwrite.
        addDiagnosticOptions();
        addGlobalOptions();
        addPrecompiledHeaderOptions();
    }

    const QStringList &options() const { return m_options; }
    const Core::Id &diagnosticConfigId() const { return m_diagnosticConfigId; }

private:
    void addLanguageOptions()
    {
        // Determine file kind with respect to ambiguous headers.
        CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::classify(m_filePath);
        if (fileKind == CppTools::ProjectFile::AmbiguousHeader) {
            fileKind = m_projectPart.languageVersion <= CppTools::ProjectPart::LatestCVersion
                 ? CppTools::ProjectFile::CHeader
                 : CppTools::ProjectFile::CXXHeader;
        }

        CppTools::CompilerOptionsBuilder builder(m_projectPart);
        builder.addLanguageOption(fileKind);

        m_options.append(builder.options());
    }

    void addDiagnosticOptions()
    {
        if (m_projectPart.project) {
            ClangProjectSettings &projectSettings = getProjectSettings(m_projectPart.project);
            if (!projectSettings.useGlobalConfig()) {
                const Core::Id warningConfigId = projectSettings.warningConfigId();
                const CppTools::ClangDiagnosticConfigsModel configsModel(
                            CppTools::codeModelSettings()->clangCustomDiagnosticConfigs());
                if (configsModel.hasConfigWithId(warningConfigId)) {
                    addDiagnosticOptionsForConfig(configsModel.configWithId(warningConfigId));
                    return;
                }
            }
        }

        addDiagnosticOptionsForConfig(CppTools::codeModelSettings()->clangDiagnosticConfig());
    }

    void addDiagnosticOptionsForConfig(const CppTools::ClangDiagnosticConfig &diagnosticConfig)
    {
        m_diagnosticConfigId = diagnosticConfig.id();

        m_options.append(diagnosticConfig.clangOptions());
        addClangTidyOptions(diagnosticConfig);
        addClazyOptions(diagnosticConfig.clazyChecks());
    }

    void addXclangArg(const QString &argName, const QString &argValue = QString())
    {
        m_options.append("-Xclang");
        m_options.append(argName);
        if (!argValue.isEmpty()) {
            m_options.append("-Xclang");
            m_options.append(argValue);
        }
    }

    void addClangTidyOptions(const CppTools::ClangDiagnosticConfig &diagnosticConfig)
    {
        using Mode = CppTools::ClangDiagnosticConfig::TidyMode;
        Mode tidyMode = diagnosticConfig.clangTidyMode();
        if (tidyMode == Mode::Disabled)
            return;

        addXclangArg("-add-plugin", "clang-tidy");

        if (tidyMode == Mode::File)
            return;

        const QString checks = diagnosticConfig.clangTidyChecks();
        if (!checks.isEmpty())
            addXclangArg("-plugin-arg-clang-tidy", "-checks=" + checks);
    }

    void addClazyOptions(const QString &checks)
    {
        if (checks.isEmpty())
            return;

        addXclangArg("-add-plugin", "clang-lazy");
        addXclangArg("-plugin-arg-clang-lazy", "enable-all-fixits");
        addXclangArg("-plugin-arg-clang-lazy", "no-autowrite-fixits");
        addXclangArg("-plugin-arg-clang-lazy", checks);

        // NOTE: we already use -isystem for all include paths to make libclang skip diagnostics for
        // all of them. That means that ignore-included-files will not change anything unless we decide
        // to return the original -I prefix for some include paths.
        addXclangArg("-plugin-arg-clang-lazy", "ignore-included-files");
    }

    void addGlobalDiagnosticOptions()
    {
        m_options += CppTools::ClangDiagnosticConfigsModel::globalDiagnosticOptions();
    }

    void addGlobalOptions()
    {
        if (!m_projectPart.project)
            m_options.append(ClangProjectSettings::globalCommandLineOptions());
        else
            m_options.append(getProjectSettings(m_projectPart.project).commandLineOptions());
    }

    void addPrecompiledHeaderOptions()
    {
        using namespace CppTools;

        if (getPchUsage() == CompilerOptionsBuilder::PchUsage::None)
            return;

        if (m_projectPart.precompiledHeaders.contains(m_filePath))
            return;

        CompilerOptionsBuilder builder(m_projectPart);
        builder.addPrecompiledHeaderOptions(CompilerOptionsBuilder::PchUsage::Use);

        m_options.append(builder.options());
    }

private:
    const QString &m_filePath;
    const CppTools::ProjectPart &m_projectPart;

    Core::Id m_diagnosticConfigId;
    QStringList m_options;
};
} // namespace

void ClangEditorDocumentProcessor::updateBackendDocument(
    CppTools::ProjectPart &projectPart)
{
    // On registration we send the document content immediately as an unsaved
    // file, because
    //   (1) a refactoring action might have opened and already modified
    //       this document.
    //   (2) it prevents an extra preamble generation on first user
    //       modification of the document in case the line endings on disk
    //       differ from the ones returned by textDocument()->toPlainText(),
    //       like on Windows.

    if (m_projectPart) {
        if (projectPart.id() == m_projectPart->id())
            return;
    }

    const FileOptionsBuilder fileOptions(filePath(), projectPart);
    m_diagnosticConfigId = fileOptions.diagnosticConfigId();
    m_communicator.documentsOpened(
        {fileContainerWithOptionsAndDocumentContent(projectPart, fileOptions.options())});
    ClangCodeModel::Utils::setLastSentDocumentRevision(filePath(), revision());
}

void ClangEditorDocumentProcessor::closeBackendDocument()
{
    QTC_ASSERT(m_projectPart, return);
    m_communicator.documentsClosed({ClangBackEnd::FileContainer(filePath(), m_projectPart->id())});
}

void ClangEditorDocumentProcessor::updateBackendDocumentIfProjectPartExists()
{
    if (m_projectPart) {
        const ClangBackEnd::FileContainer fileContainer = fileContainerWithDocumentContent(
            m_projectPart->id());
        m_communicator.documentsChangedWithRevisionCheck(fileContainer);
    }
}

void ClangEditorDocumentProcessor::requestAnnotationsFromBackend(const QString &projectpartId)
{
    const auto fileContainer = fileContainerWithDocumentContent(projectpartId);

    m_communicator.requestAnnotations(fileContainer);
}

CppTools::BaseEditorDocumentProcessor::HeaderErrorDiagnosticWidgetCreator
ClangEditorDocumentProcessor::creatorForHeaderErrorDiagnosticWidget(
        const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic)
{
    if (firstHeaderErrorDiagnostic.text.isEmpty())
        return CppTools::BaseEditorDocumentProcessor::HeaderErrorDiagnosticWidgetCreator();

    return [firstHeaderErrorDiagnostic]() {
        auto vbox = new QVBoxLayout;
        vbox->setMargin(0);
        vbox->setContentsMargins(10, 0, 0, 2);
        vbox->setSpacing(2);

        vbox->addWidget(ClangDiagnosticWidget::create({firstHeaderErrorDiagnostic},
                                                      ClangDiagnosticWidget::InfoBar));

        auto widget = new QWidget;
        widget->setLayout(vbox);

        return widget;
    };
}

ClangBackEnd::FileContainer ClangEditorDocumentProcessor::simpleFileContainer(
    const QByteArray &codecName) const
{
    Utf8String projectPartId;
    if (m_projectPart)
        projectPartId = m_projectPart->id();

    return ClangBackEnd::FileContainer(filePath(),
                                       projectPartId,
                                       Utf8String(),
                                       false,
                                       revision(),
                                       Utf8String::fromByteArray(codecName));
}

ClangBackEnd::FileContainer ClangEditorDocumentProcessor::fileContainerWithOptionsAndDocumentContent(
    CppTools::ProjectPart &projectPart, const QStringList &fileOptions) const
{
    return ClangBackEnd::FileContainer(filePath(),
                                       projectPart.id(),
                                       Utf8StringVector(fileOptions),
                                       textDocument()->toPlainText(),
                                       true,
                                       revision());
}

ClangBackEnd::FileContainer
ClangEditorDocumentProcessor::fileContainerWithDocumentContent(const QString &projectpartId) const
{
    return ClangBackEnd::FileContainer(filePath(),
                                       projectpartId,
                                       textDocument()->toPlainText(),
                                       true,
                                       revision());
}

} // namespace Internal
} // namespace ClangCodeModel
