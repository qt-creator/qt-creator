// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppbuiltinmodelmanagersupport.h"

#include "builtineditordocumentprocessor.h"
#include "cppcanonicalsymbol.h"
#include "cppcompletionassist.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppelementevaluator.h"
#include "cppfollowsymbolundercursor.h"
#include "cpptoolsreuse.h"
#include "symbolfinder.h"

#include <coreplugin/messagemanager.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/TranslationUnit.h>
#include <texteditor/basehoverhandler.h>
#include <texteditor/textdocumentlayout.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QCoreApplication>
#include <QFile>
#include <QScopeGuard>
#include <QTextDocument>

using namespace Core;
using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class CppHoverHandler : public TextEditor::BaseHoverHandler
{
private:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override
    {
        if (CppModelManager::usesClangd(editorWidget->textDocument())) {
            report(Priority_None);
            return;
        }

        const QScopeGuard cleanup([this, report] { report(priority()); });

        QTextCursor tc(editorWidget->document());
        tc.setPosition(pos);

        CppElementEvaluator evaluator(editorWidget);
        evaluator.setTextCursor(tc);
        evaluator.execute();
        QString tip;
        if (evaluator.hasDiagnosis()) {
            tip += evaluator.diagnosis();
            setPriority(Priority_Diagnostic);
        }
        const FilePath filePath = editorWidget->textDocument()->filePath();
        const QStringList fallback = identifierWordsUnderCursor(tc);
        const std::shared_ptr<CppElement> &cppElement = evaluator.cppElement();
        if (cppElement) {
            const QStringList candidates = cppElement->helpIdCandidates;
            const HelpItem helpItem(candidates + fallback,
                                    filePath,
                                    cppElement->helpMark,
                                    cppElement->helpCategory);
            setLastHelpItemIdentified(helpItem);
            if (!helpItem.isValid())
                tip += cppElement->tooltip;
        } else {
            setLastHelpItemIdentified({fallback, filePath, {}, HelpItem::Unknown});
        }
        setToolTip(tip);
    }
};
} // anonymous namespace

BuiltinModelManagerSupport::BuiltinModelManagerSupport()
    : m_completionAssistProvider(new InternalCompletionAssistProvider),
      m_followSymbol(new FollowSymbolUnderCursor)
{
}

BuiltinModelManagerSupport::~BuiltinModelManagerSupport() = default;

BaseEditorDocumentProcessor *BuiltinModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new BuiltinEditorDocumentProcessor(baseTextDocument);
}

CppCompletionAssistProvider *BuiltinModelManagerSupport::completionAssistProvider()
{
    return m_completionAssistProvider.data();
}


TextEditor::BaseHoverHandler &BuiltinModelManagerSupport::cppHoverHandler()
{
    static CppHoverHandler theCppHoverHandler;
    return theCppHoverHandler;
}

void BuiltinModelManagerSupport::followSymbol(const CursorInEditor &data,
                                              const Utils::LinkHandler &processLinkCallback,
                                              FollowSymbolMode mode,
                                              bool resolveTarget, bool inNextSplit)
{
    // findMatchingDefinition() has an "strict" parameter, but it doesn't seem worth to
    // pass the mode down all the way. In practice, we are always fuzzy.
    Q_UNUSED(mode)

    SymbolFinder finder;
    m_followSymbol->findLink(data, processLinkCallback,
            resolveTarget, CppModelManager::snapshot(),
            data.editorWidget() ? data.editorWidget()->semanticInfo().doc : data.cppDocument(),
            &finder, inNextSplit);
}

void BuiltinModelManagerSupport::followSymbolToType(const CursorInEditor &data,
                                                    const Utils::LinkHandler &processLinkCallback,
                                                    bool inNextSplit)
{
    Q_UNUSED(data)
    Q_UNUSED(processLinkCallback)
    Q_UNUSED(inNextSplit)
    MessageManager::writeDisrupting(
                Tr::tr("Follow Symbol to Type is only available when using clangd"));
}

void BuiltinModelManagerSupport::followFunctionToParentImpl(
    const CursorInEditor &data, const Utils::LinkHandler &processLinkCallback)
{
    SymbolFinder finder;
    m_followSymbol->findParentImpl(
        data,
        processLinkCallback,
        CppModelManager::snapshot(),
        data.editorWidget()->semanticInfo().doc,
        &finder);
}

void BuiltinModelManagerSupport::switchDeclDef(const CursorInEditor &data,
                                               const Utils::LinkHandler &processLinkCallback)
{
    SymbolFinder finder;
    m_followSymbol->switchDeclDef(data, processLinkCallback,
            CppModelManager::snapshot(), data.editorWidget()->semanticInfo().doc,
            &finder);
}

void BuiltinModelManagerSupport::startLocalRenaming(const CursorInEditor &data,
                                                    const ProjectPart *,
                                                    RenameCallback &&renameSymbolsCallback)
{
    CppEditorWidget *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, renameSymbolsCallback(QString(), {}, 0); return;);
    editorWidget->updateSemanticInfo();
    // Call empty callback
    renameSymbolsCallback(QString(), {}, data.cursor().document()->revision());
}

void BuiltinModelManagerSupport::globalRename(const CursorInEditor &data,
                                              const QString &replacement,
                                              const std::function<void()> &callback)
{
    CppEditorWidget *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, return;);

    SemanticInfo info = editorWidget->semanticInfo();
    info.snapshot = CppModelManager::snapshot();
    info.snapshot.insert(info.doc);
    const QTextCursor &cursor = data.cursor();
    if (const CPlusPlus::Macro *macro = findCanonicalMacro(cursor, info.doc)) {
        CppModelManager::renameMacroUsages(*macro, replacement);
    } else {
        Internal::CanonicalSymbol cs(info.doc, info.snapshot);
        CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
        if (canonicalSymbol)
            CppModelManager::renameUsages(canonicalSymbol, cs.context(), replacement, callback);
    }
}

void BuiltinModelManagerSupport::findUsages(const CursorInEditor &data) const
{
    CppEditorWidget *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, return;);

    SemanticInfo info = editorWidget->semanticInfo();
    info.snapshot = CppModelManager::snapshot();
    info.snapshot.insert(info.doc);
    const QTextCursor &cursor = data.cursor();
    if (const CPlusPlus::Macro *macro = findCanonicalMacro(cursor, info.doc)) {
        CppModelManager::findMacroUsages(*macro);
    } else {
        Internal::CanonicalSymbol cs(info.doc, info.snapshot);
        CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
        if (canonicalSymbol)
            CppModelManager::findUsages(canonicalSymbol, cs.context());
    }
}

void BuiltinModelManagerSupport::switchHeaderSource(const FilePath &filePath,
                                                    bool inNextSplit)
{
    const FilePath otherFile = correspondingHeaderOrSource(filePath);
    if (!otherFile.isEmpty())
        openEditor(otherFile, inNextSplit);
}

void BuiltinModelManagerSupport::foldOrUnfoldComments(BaseTextEditor *editor, bool fold)
{
    const auto editorWidget = qobject_cast<CppEditorWidget*>(editor->widget());
    if (!editorWidget)
        return;
    TextEditor::TextDocument * const textDoc = editorWidget->textDocument();
    QTC_ASSERT(textDoc, return);

    const Document::Ptr cppDoc = CppModelManager::snapshot().preprocessedDocument(
        textDoc->contents(), textDoc->filePath());
    QTC_ASSERT(cppDoc, return);
    cppDoc->tokenize();
    TranslationUnit * const tu = cppDoc->translationUnit();
    if (!tu || !tu->isTokenized())
        return;

    for (int commentTokIndex = 0; commentTokIndex < tu->commentCount(); ++commentTokIndex) {
        const Token &tok = tu->commentAt(commentTokIndex);
        if (tok.kind() != T_COMMENT && tok.kind() != T_DOXY_COMMENT)
            continue;
        const int tokenPos = tu->getTokenPositionInDocument(tok, textDoc->document());
        const int tokenEndPos = tu->getTokenEndPositionInDocument(tok, textDoc->document());
        const QTextBlock tokenBlock = textDoc->document()->findBlock(tokenPos);
        if (!tokenBlock.isValid())
            continue;
        const QTextBlock nextBlock = tokenBlock.next();
        if (!nextBlock.isValid())
            continue;
        if (tokenEndPos < nextBlock.position())
            continue;
        if (TextEditor::TextBlockUserData::foldingIndent(tokenBlock)
            >= TextEditor::TextBlockUserData::foldingIndent(nextBlock)) {
            continue;
        }
        if (fold)
            editorWidget->fold(tokenBlock);
        else
            editorWidget->unfold(tokenBlock);
    }
}

void BuiltinModelManagerSupport::checkUnused(const Link &link, SearchResult *search,
                                             const LinkHandler &callback)
{
    CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    QFile file(link.targetFilePath.toFSPathString());
    if (!file.open(QIODevice::ReadOnly))
        return callback(link);
    const QByteArray &contents = file.readAll();
    CPlusPlus::Document::Ptr cppDoc = snapshot.preprocessedDocument(contents, link.targetFilePath);
    if (!cppDoc->parse())
        return callback(link);
    cppDoc->check();
    snapshot.insert(cppDoc);
    QTextDocument doc(QString::fromUtf8(contents));
    const QTextCursor cursor = link.target.toTextCursor(&doc);
    Internal::CanonicalSymbol cs(cppDoc, snapshot);
    CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
    if (!canonicalSymbol || !canonicalSymbol->identifier())
        return callback(link);
    CppModelManager::checkForUnusedSymbol(search, link, canonicalSymbol, cs.context(), callback);
}

} // namespace CppEditor::Internal
