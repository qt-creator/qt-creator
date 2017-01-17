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

#include "builtineditordocumentprocessor.h"

#include "cppchecksymbols.h"
#include "cppcodemodelsettings.h"
#include "cppmodelmanager.h"
#include "cpptoolsplugin.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <texteditor/convenience.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QLoggingCategory>
#include <QTextBlock>

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.builtineditordocumentprocessor")

namespace {

QList<QTextEdit::ExtraSelection> toTextEditorSelections(
                const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics,
                QTextDocument *textDocument)
{
    const TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

    QTextCharFormat warningFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
    QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

    QList<QTextEdit::ExtraSelection> result;
    foreach (const CPlusPlus::Document::DiagnosticMessage &m, diagnostics) {
        QTextEdit::ExtraSelection sel;
        if (m.isWarning())
            sel.format = warningFormat;
        else
            sel.format = errorFormat;

        QTextCursor c(textDocument->findBlockByNumber(m.line() - 1));
        const QString text = c.block().text();
        const int startPos = m.column() > 0 ? m.column() - 1 : 0;
        if (m.length() > 0 && startPos + m.length() <= (unsigned)text.size()) {
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

CppTools::CheckSymbols *createHighlighter(const CPlusPlus::Document::Ptr &doc,
                                          const CPlusPlus::Snapshot &snapshot,
                                          QTextDocument *textDocument)
{
    QTC_ASSERT(doc, return 0);
    QTC_ASSERT(doc->translationUnit(), return 0);
    QTC_ASSERT(doc->translationUnit()->ast(), return 0);
    QTC_ASSERT(textDocument, return 0);

    using namespace CPlusPlus;
    using namespace CppTools;
    typedef TextEditor::HighlightingResult Result;
    QList<Result> macroUses;

    using TextEditor::Convenience::convertPosition;

    // Get macro definitions
    foreach (const CPlusPlus::Macro& macro, doc->definedMacros()) {
        int line, column;
        convertPosition(textDocument, macro.utf16CharOffset(), &line, &column);

        ++column; //Highlighting starts at (column-1) --> compensate here
        Result use(line, column, macro.nameToQString().size(), SemanticHighlighter::MacroUse);
        macroUses.append(use);
    }

    const LanguageFeatures features = doc->languageFeatures();

    // Get macro uses
    foreach (const Document::MacroUse &macro, doc->macroUses()) {
        const QString name = macro.macro().nameToQString();

        //Filter out QtKeywords
        if (features.qtKeywordsEnabled && isQtKeyword(QStringRef(&name)))
            continue;

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(features);

        // Filter out C++ keywords
        const Tokens tokens = tokenize(name);
        if (tokens.length() && (tokens.at(0).isKeyword() || tokens.at(0).isObjCAtKeyword()))
            continue;

        int line, column;
        convertPosition(textDocument, macro.utf16charsBegin(), &line, &column);
        ++column; //Highlighting starts at (column-1) --> compensate here
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
    foreach (const CPlusPlus::Document::Block &block, skippedBlocks)
        result.append(TextEditor::BlockRange(block.utf16charsBegin(), block.utf16charsEnd()));
    return result;
}

} // anonymous namespace

namespace CppTools {

BuiltinEditorDocumentProcessor::BuiltinEditorDocumentProcessor(
        TextEditor::TextDocument *document,
        bool enableSemanticHighlighter)
    : BaseEditorDocumentProcessor(document->document(), document->filePath().toString())
    , m_parser(new BuiltinEditorDocumentParser(document->filePath().toString()))
    , m_codeWarningsUpdated(false)
    , m_semanticHighlighter(enableSemanticHighlighter
                            ? new CppTools::SemanticHighlighter(document)
                            : 0)
{
    using namespace Internal;

    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();

    BaseEditorDocumentParser::Configuration config = m_parser->configuration();
    config.usePrecompiledHeaders = cms->pchUsage() != CppCodeModelSettings::PchUse_None;
    m_parser->setConfiguration(config);

    if (m_semanticHighlighter) {
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
    }

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
    m_parserFuture.waitForFinished();
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
    if (m_semanticHighlighter && m_semanticInfoUpdater.semanticInfo().doc) {
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

void BuiltinEditorDocumentProcessor::onParserFinished(CPlusPlus::Document::Ptr document,
                                                      CPlusPlus::Snapshot snapshot)
{
    if (document.isNull())
        return;

    if (document->fileName() != filePath())
        return; // some other document got updated

    if (document->editorRevision() != revision())
        return; // outdated content, wait for a new document to be parsed

    qCDebug(log) << "document parsed" << document->fileName() << document->editorRevision();

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(document->skippedBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Store parser warnings
    m_codeWarnings = toTextEditorSelections(document->diagnosticMessages(), textDocument());
    m_codeWarningsUpdated = false;

    emit cppDocumentUpdated(document);

    m_documentSnapshot = snapshot;
    const auto source = createSemanticInfoSource(false);
    QTC_CHECK(source.snapshot.contains(document->fileName()));
    m_semanticInfoUpdater.updateDetached(source);
}

void BuiltinEditorDocumentProcessor::onSemanticInfoUpdated(const SemanticInfo semanticInfo)
{
    qCDebug(log) << "semantic info updated"
                 << semanticInfo.doc->fileName() << semanticInfo.revision << semanticInfo.complete;

    emit semanticInfoUpdated(semanticInfo);

    if (m_semanticHighlighter)
        m_semanticHighlighter->run();
}

void BuiltinEditorDocumentProcessor::onCodeWarningsUpdated(
        CPlusPlus::Document::Ptr document,
        const QList<CPlusPlus::Document::DiagnosticMessage> &codeWarnings)
{
    if (document.isNull())
        return;

    if (document->fileName() != filePath())
        return; // some other document got updated

    if (document->editorRevision() != revision())
        return; // outdated content, wait for a new document to be parsed

    if (m_codeWarningsUpdated)
        return; // code warnings already updated

    m_codeWarnings += toTextEditorSelections(codeWarnings, textDocument());
    m_codeWarningsUpdated = true;
    emit codeWarningsUpdated(revision(),
                             m_codeWarnings,
                             HeaderErrorDiagnosticWidgetCreator(),
                             TextEditor::RefactorMarkers());
}

SemanticInfo::Source BuiltinEditorDocumentProcessor::createSemanticInfoSource(bool force) const
{
    const WorkingCopy workingCopy = CppTools::CppModelManager::instance()->workingCopy();
    const QString path = filePath();
    return SemanticInfo::Source(path,
                                workingCopy.source(path),
                                workingCopy.revision(path),
                                m_documentSnapshot,
                                force);
}

} // namespace CppTools
