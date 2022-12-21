// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "builtineditordocumentprocessor.h"

#include "builtincursorinfo.h"
#include "cppchecksymbols.h"
#include "cppcodemodelsettings.h"
#include "cppeditorplugin.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QLoggingCategory>
#include <QTextBlock>

static Q_LOGGING_CATEGORY(log, "qtc.cppeditor.builtineditordocumentprocessor", QtWarningMsg)

namespace CppEditor {
namespace {

QList<QTextEdit::ExtraSelection> toTextEditorSelections(
                const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics,
                QTextDocument *textDocument)
{
    const TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::fontSettings();

    QTextCharFormat warningFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
    QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

    QList<QTextEdit::ExtraSelection> result;
    for (const CPlusPlus::Document::DiagnosticMessage &m : diagnostics) {
        QTextEdit::ExtraSelection sel;
        if (m.isWarning())
            sel.format = warningFormat;
        else
            sel.format = errorFormat;

        QTextCursor c(textDocument->findBlockByNumber(m.line() - 1));
        const QString text = c.block().text();
        const int startPos = m.column() > 0 ? m.column() - 1 : 0;
        if (m.length() > 0 && startPos + m.length() <= text.size()) {
            c.setPosition(c.position() + startPos);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, m.length());
        } else {
            for (int i = 0; i < text.size(); ++i) {
                if (!text.at(i).isSpace()) {
                    c.setPosition(c.position() + i);
                    break;
                }
            }
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }
        sel.cursor = c;
        sel.format.setToolTip(m.text());
        result.append(sel);
    }

    return result;
}

CheckSymbols *createHighlighter(const CPlusPlus::Document::Ptr &doc,
                                          const CPlusPlus::Snapshot &snapshot,
                                          QTextDocument *textDocument)
{
    QTC_ASSERT(doc, return nullptr);
    QTC_ASSERT(doc->translationUnit(), return nullptr);
    QTC_ASSERT(doc->translationUnit()->ast(), return nullptr);
    QTC_ASSERT(textDocument, return nullptr);

    using namespace CPlusPlus;
    using Result = TextEditor::HighlightingResult;
    QList<Result> macroUses;

    using Utils::Text::convertPosition;

    // Get macro definitions
    for (const CPlusPlus::Macro &macro : doc->definedMacros()) {
        int line, column;
        convertPosition(textDocument, macro.utf16CharOffset(), &line, &column);

        Result use(line, column, macro.nameToQString().size(), SemanticHighlighter::MacroUse);
        macroUses.append(use);
    }

    const LanguageFeatures features = doc->languageFeatures();

    // Get macro uses
    for (const Document::MacroUse &macro : doc->macroUses()) {
        const QString name = macro.macro().nameToQString();

        //Filter out QtKeywords
        if (features.qtKeywordsEnabled && isQtKeyword(name))
            continue;

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(features);

        // Filter out C++ keywords
        const Tokens tokens = tokenize(name);
        if (!tokens.isEmpty() && (tokens.at(0).isKeyword() || tokens.at(0).isObjCAtKeyword()))
            continue;

        int line, column;
        convertPosition(textDocument, macro.utf16charsBegin(), &line, &column);

        Result use(line, column, name.size(), SemanticHighlighter::MacroUse);
        macroUses.append(use);
    }

    LookupContext context(doc, snapshot);
    return CheckSymbols::create(doc, context, macroUses);
}

QList<TextEditor::BlockRange> toTextEditorBlocks(
        const QList<CPlusPlus::Document::Block> &skippedBlocks)
{
    QList<TextEditor::BlockRange> result;
    result.reserve(skippedBlocks.size());
    for (const CPlusPlus::Document::Block &block : skippedBlocks)
        result.append(TextEditor::BlockRange(block.utf16charsBegin(), block.utf16charsEnd()));
    return result;
}

} // anonymous namespace

BuiltinEditorDocumentProcessor::BuiltinEditorDocumentProcessor(TextEditor::TextDocument *document)
    : BaseEditorDocumentProcessor(document->document(), document->filePath())
    , m_parser(new BuiltinEditorDocumentParser(document->filePath(), indexerFileSizeLimitInMb()))
    , m_codeWarningsUpdated(false)
    , m_semanticHighlighter(new SemanticHighlighter(document))
{
    using namespace Internal;

    const CppCodeModelSettings *cms = CppEditorPlugin::instance()->codeModelSettings();

    BaseEditorDocumentParser::Configuration config = m_parser->configuration();
    config.usePrecompiledHeaders = cms->pchUsage() != CppCodeModelSettings::PchUse_None;
    m_parser->setConfiguration(config);

    m_semanticHighlighter->setHighlightingRunner(
                [this]() -> QFuture<TextEditor::HighlightingResult> {
                    const SemanticInfo semanticInfo = m_semanticInfoUpdater.semanticInfo();
                    CheckSymbols *checkSymbols = createHighlighter(semanticInfo.doc, semanticInfo.snapshot,
                    textDocument());
                    QTC_ASSERT(checkSymbols, return QFuture<TextEditor::HighlightingResult>());
                    connect(checkSymbols, &CheckSymbols::codeWarningsUpdated,
                    this, &BuiltinEditorDocumentProcessor::onCodeWarningsUpdated);
                    return checkSymbols->start();
                });

    connect(m_parser.data(), &BuiltinEditorDocumentParser::projectPartInfoUpdated,
            this, &BaseEditorDocumentProcessor::projectPartInfoUpdated);
    connect(m_parser.data(), &BuiltinEditorDocumentParser::finished,
            this, &BuiltinEditorDocumentProcessor::onParserFinished);
    connect(&m_semanticInfoUpdater, &SemanticInfoUpdater::updated,
            this, &BuiltinEditorDocumentProcessor::onSemanticInfoUpdated);
}

BuiltinEditorDocumentProcessor::~BuiltinEditorDocumentProcessor()
{
    m_parserFuture.cancel();
}

void BuiltinEditorDocumentProcessor::runImpl(
        const BaseEditorDocumentParser::UpdateParams &updateParams)
{
    m_parserFuture = Utils::runAsync(CppModelManager::instance()->sharedThreadPool(),
                                     runParser,
                                     parser(),
                                     updateParams);
}

BaseEditorDocumentParser::Ptr BuiltinEditorDocumentProcessor::parser()
{
    return m_parser;
}

CPlusPlus::Snapshot BuiltinEditorDocumentProcessor::snapshot()
{
    return m_parser->snapshot();
}

void BuiltinEditorDocumentProcessor::recalculateSemanticInfoDetached(bool force)
{
    const auto source = createSemanticInfoSource(force);
    m_semanticInfoUpdater.updateDetached(source);
}

void BuiltinEditorDocumentProcessor::semanticRehighlight()
{
    if (m_semanticInfoUpdater.semanticInfo().doc) {
        if (const CPlusPlus::Document::Ptr document = m_documentSnapshot.document(filePath())) {
            m_codeWarnings = toTextEditorSelections(document->diagnosticMessages(), textDocument());
            m_codeWarningsUpdated = false;
        }

        m_semanticHighlighter->updateFormatMapFromFontSettings();
        m_semanticHighlighter->run();
    }
}

SemanticInfo BuiltinEditorDocumentProcessor::recalculateSemanticInfo()
{
    const auto source = createSemanticInfoSource(false);
    return m_semanticInfoUpdater.update(source);
}

bool BuiltinEditorDocumentProcessor::isParserRunning() const
{
    return m_parserFuture.isRunning();
}

QFuture<CursorInfo>
BuiltinEditorDocumentProcessor::cursorInfo(const CursorInfoParams &params)
{
    return BuiltinCursorInfo::run(params);
}

void BuiltinEditorDocumentProcessor::setSemanticHighlightingChecker(
            const SemanticHighlightingChecker &checker)
{
    m_semanticHighlightingChecker = checker;
}

void BuiltinEditorDocumentProcessor::onParserFinished(CPlusPlus::Document::Ptr document,
                                                      CPlusPlus::Snapshot snapshot)
{
    if (document.isNull())
        return;

    if (document->filePath() != filePath())
        return; // some other document got updated

    if (document->editorRevision() != revision())
        return; // outdated content, wait for a new document to be parsed

    qCDebug(log) << "document parsed" << document->filePath() << document->editorRevision();

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(document->skippedBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Store parser warnings
    m_codeWarnings = toTextEditorSelections(document->diagnosticMessages(), textDocument());
    m_codeWarningsUpdated = false;

    emit cppDocumentUpdated(document);

    m_documentSnapshot = snapshot;
    const auto source = createSemanticInfoSource(false);
    QTC_CHECK(source.snapshot.contains(document->filePath()));
    m_semanticInfoUpdater.updateDetached(source);
}

void BuiltinEditorDocumentProcessor::onSemanticInfoUpdated(const SemanticInfo semanticInfo)
{
    qCDebug(log) << "semantic info updated"
                 << semanticInfo.doc->filePath() << semanticInfo.revision << semanticInfo.complete;

    emit semanticInfoUpdated(semanticInfo);

    if (!m_semanticHighlightingChecker || m_semanticHighlightingChecker())
        m_semanticHighlighter->run();
}

void BuiltinEditorDocumentProcessor::onCodeWarningsUpdated(
        CPlusPlus::Document::Ptr document,
        const QList<CPlusPlus::Document::DiagnosticMessage> &codeWarnings)
{
    if (document.isNull())
        return;

    if (document->filePath() != filePath())
        return; // some other document got updated

    if (document->editorRevision() != revision())
        return; // outdated content, wait for a new document to be parsed

    if (m_codeWarningsUpdated)
        return; // code warnings already updated

    m_codeWarnings += toTextEditorSelections(codeWarnings, textDocument());
    m_codeWarningsUpdated = true;
    emit codeWarningsUpdated(revision(),
                             m_codeWarnings,
                             TextEditor::RefactorMarkers());
}

SemanticInfo::Source BuiltinEditorDocumentProcessor::createSemanticInfoSource(bool force) const
{
    const WorkingCopy workingCopy = CppModelManager::instance()->workingCopy();
    return SemanticInfo::Source(filePath().toString(),
                                workingCopy.source(filePath()),
                                workingCopy.revision(filePath()),
                                m_documentSnapshot,
                                force);
}

} // namespace CppEditor
