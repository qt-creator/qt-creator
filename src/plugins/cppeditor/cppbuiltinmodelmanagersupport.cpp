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
#include <texteditor/basehoverhandler.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QCoreApplication>
#include <QFile>
#include <QScopeGuard>
#include <QTextDocument>

using namespace Core;
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
        if (evaluator.identifiedCppElement()) {
            const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
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


TextEditor::BaseHoverHandler *BuiltinModelManagerSupport::createHoverHandler()
{
    return new CppHoverHandler;
}

void BuiltinModelManagerSupport::followSymbol(const CursorInEditor &data,
                                              const Utils::LinkHandler &processLinkCallback,
                                              bool resolveTarget, bool inNextSplit)
{
    SymbolFinder finder;
    m_followSymbol->findLink(data, processLinkCallback,
            resolveTarget, CppModelManager::snapshot(),
            data.editorWidget()->semanticInfo().doc, &finder, inNextSplit);
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

void BuiltinModelManagerSupport::checkUnused(const Utils::Link &link, SearchResult *search,
                                             const Utils::LinkHandler &callback)
{
    CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    QFile file(link.targetFilePath.toString());
    if (!file.open(QIODevice::ReadOnly))
        return callback(link);
    const QByteArray &contents = file.readAll();
    CPlusPlus::Document::Ptr cppDoc = snapshot.preprocessedDocument(contents, link.targetFilePath);
    if (!cppDoc->parse())
        return callback(link);
    cppDoc->check();
    snapshot.insert(cppDoc);
    QTextDocument doc(QString::fromUtf8(contents));
    QTextCursor cursor(&doc);
    cursor.setPosition(Utils::Text::positionInText(&doc, link.targetLine, link.targetColumn + 1));
    Internal::CanonicalSymbol cs(cppDoc, snapshot);
    CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
    if (!canonicalSymbol || !canonicalSymbol->identifier())
        return callback(link);
    CppModelManager::checkForUnusedSymbol(search, link, canonicalSymbol, cs.context(), callback);
}

} // namespace CppEditor::Internal
