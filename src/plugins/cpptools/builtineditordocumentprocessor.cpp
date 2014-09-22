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

#include "builtineditordocumentprocessor.h"

#include "cppchecksymbols.h"
#include "cppcodemodelsettings.h"
#include "cppmodelmanager.h"
#include "cpptoolsplugin.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/convenience.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/QtConcurrentTools>
#include <utils/qtcassert.h>

enum { debug = 0 };

namespace {

QFuture<TextEditor::HighlightingResult> runHighlighter(const CPlusPlus::Document::Ptr &doc,
                                                       const CPlusPlus::Snapshot &snapshot,
                                                       QTextDocument *textDocument)
{
    QFuture<TextEditor::HighlightingResult> failed;
    QTC_ASSERT(doc, return failed);
    QTC_ASSERT(doc->translationUnit(), return failed);
    QTC_ASSERT(doc->translationUnit()->ast(), return failed);
    QTC_ASSERT(textDocument, return failed);

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

    // Get macro uses
    foreach (const Document::MacroUse &macro, doc->macroUses()) {
        const QString name = macro.macro().nameToQString();

        //Filter out QtKeywords
        if (isQtKeyword(QStringRef(&name)))
            continue;

        // Filter out C++ keywords
        // FIXME: Check default values or get from document.
        LanguageFeatures features;
        features.cxx11Enabled = true;
        features.c99Enabled = true;

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(features);

        const QList<Token> tokens = tokenize(name);
        if (tokens.length() && (tokens.at(0).isKeyword() || tokens.at(0).isObjCAtKeyword()))
            continue;

        int line, column;
        convertPosition(textDocument, macro.utf16charsBegin(), &line, &column);
        ++column; //Highlighting starts at (column-1) --> compensate here
        Result use(line, column, name.size(), SemanticHighlighter::MacroUse);
        macroUses.append(use);
    }

    LookupContext context(doc, snapshot);
    return CheckSymbols::go(doc, context, macroUses);
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
    : BaseEditorDocumentProcessor(document)
    , m_parser(document->filePath())
    , m_semanticHighlighter(enableSemanticHighlighter
                            ? new CppTools::SemanticHighlighter(document)
                            : 0)
{
    using namespace Internal;

    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
    m_parser.setUsePrecompiledHeaders(cms->pchUsage() != CppCodeModelSettings::PchUse_None);

    if (m_semanticHighlighter) {
        m_semanticHighlighter->setHighlightingRunner(
            [this]() -> QFuture<TextEditor::HighlightingResult> {
                const SemanticInfo semanticInfo = m_semanticInfoUpdater.semanticInfo();
                return runHighlighter(semanticInfo.doc, semanticInfo.snapshot,
                                      baseTextDocument()->document());
            });
    }

    connect(&m_parser, &BuiltinEditorDocumentParser::finished,
            this, &BuiltinEditorDocumentProcessor::onParserFinished);
    connect(&m_semanticInfoUpdater, &SemanticInfoUpdater::updated,
            this, &BuiltinEditorDocumentProcessor::onSemanticInfoUpdated);
}

BuiltinEditorDocumentProcessor::~BuiltinEditorDocumentProcessor()
{
    m_parserFuture.cancel();
    m_parserFuture.waitForFinished();
}

void BuiltinEditorDocumentProcessor::run()
{
    m_parserFuture = QtConcurrent::run(&runParser, parser(), CppTools::CppModelManager::instance()->workingCopy());
}

BaseEditorDocumentParser *BuiltinEditorDocumentProcessor::parser()
{
    return &m_parser;
}

void BuiltinEditorDocumentProcessor::semanticRehighlight(bool force)
{
    const auto source = createSemanticInfoSource(force);
    m_semanticInfoUpdater.updateDetached(source);
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

    if (debug) {
        qDebug() << "BuiltinEditorDocumentProcessor: document parsed" << document->fileName()
                 << document->editorRevision();
    }

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(document->skippedBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Emit code warnings
    auto codeWarnings = toTextEditorSelections(document->diagnosticMessages(), textDocument());
    emit codeWarningsUpdated(revision(), codeWarnings);

    emit cppDocumentUpdated(document);

    m_documentSnapshot = snapshot;
    const auto source = createSemanticInfoSource(false);
    QTC_CHECK(source.snapshot.contains(document->fileName()));
    m_semanticInfoUpdater.updateDetached(source);
}

void BuiltinEditorDocumentProcessor::onSemanticInfoUpdated(const SemanticInfo semanticInfo)
{
    if (debug) {
        qDebug() << "BuiltinEditorDocumentProcessor: semantic info updated"
                 << semanticInfo.doc->fileName() << semanticInfo.revision << semanticInfo.complete;
    }

    emit semanticInfoUpdated(semanticInfo);

    if (m_semanticHighlighter)
        m_semanticHighlighter->run();
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
