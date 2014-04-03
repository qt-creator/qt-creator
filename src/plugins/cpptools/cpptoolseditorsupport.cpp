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

#include "cppcodemodelsettings.h"
#include "cppcompletionassistprovider.h"
#include "cpptoolseditorsupport.h"
#include "cpptoolsplugin.h"
#include "cppmodelmanager.h"
#include "cpplocalsymbols.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QList>
#include <QMutexLocker>
#include <QTextBlock>
#include <QTimer>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;
using namespace TextEditor;

namespace {
class FunctionDefinitionUnderCursor: protected ASTVisitor
{
    unsigned _line;
    unsigned _column;
    DeclarationAST *_functionDefinition;

public:
    FunctionDefinitionUnderCursor(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit),
          _line(0), _column(0)
    { }

    DeclarationAST *operator()(AST *ast, unsigned line, unsigned column)
    {
        _functionDefinition = 0;
        _line = line;
        _column = column;
        accept(ast);
        return _functionDefinition;
    }

protected:
    virtual bool preVisit(AST *ast)
    {
        if (_functionDefinition)
            return false;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition())
            return checkDeclaration(def);

        if (ObjCMethodDeclarationAST *method = ast->asObjCMethodDeclaration()) {
            if (method->function_body)
                return checkDeclaration(method);
        }

        return true;
    }

private:
    bool checkDeclaration(DeclarationAST *ast)
    {
        unsigned startLine, startColumn;
        unsigned endLine, endColumn;
        getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
        getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

        if (_line > startLine || (_line == startLine && _column >= startColumn)) {
            if (_line < endLine || (_line == endLine && _column < endColumn)) {
                _functionDefinition = ast;
                return false;
            }
        }

        return true;
    }
};

} // anonymous namespace

CppEditorSupport::CppEditorSupport(CppModelManager *modelManager, BaseTextEditor *textEditor)
    : QObject(modelManager)
    , m_modelManager(modelManager)
    , m_textEditor(textEditor)
    , m_updateDocumentInterval(UpdateDocumentDefaultInterval)
    , m_revision(0)
    , m_editorVisible(textEditor->widget()->isVisible())
    , m_cachedContentsEditorRevision(-1)
    , m_fileIsBeingReloaded(false)
    , m_initialized(false)
    , m_lastHighlightRevision(0)
    , m_lastHighlightOnCompleteSemanticInfo(true)
    , m_highlightingSupport(modelManager->highlightingSupport(textEditor))
    , m_completionAssistProvider(m_modelManager->completionAssistProvider(textEditor))
{
    connect(m_modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    if (m_highlightingSupport && m_highlightingSupport->requiresSemanticInfo()) {
        connect(this, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
                this, SLOT(startHighlighting()));
    }

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setSingleShot(true);
    m_updateDocumentTimer->setInterval(m_updateDocumentInterval);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    m_updateEditorTimer = new QTimer(this);
    m_updateEditorTimer->setInterval(UpdateEditorInterval);
    m_updateEditorTimer->setSingleShot(true);
    connect(m_updateEditorTimer, SIGNAL(timeout()),
            this, SLOT(updateEditorNow()));

    connect(m_textEditor->document(), SIGNAL(contentsChanged()), this, SLOT(updateDocument()));
    connect(this, SIGNAL(diagnosticsChanged()), this, SLOT(onDiagnosticsChanged()));

    connect(m_textEditor->document(), SIGNAL(mimeTypeChanged()),
            this, SLOT(onMimeTypeChanged()));

    connect(m_textEditor->document(), SIGNAL(aboutToReload()),
            this, SLOT(onAboutToReload()));
    connect(m_textEditor->document(), SIGNAL(reloadFinished(bool)),
            this, SLOT(onReloadFinished()));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(onCurrentEditorChanged()));
    m_editorGCTimer = new QTimer(this);
    m_editorGCTimer->setSingleShot(true);
    m_editorGCTimer->setInterval(EditorHiddenGCTimeout);
    connect(m_editorGCTimer, SIGNAL(timeout()), this, SLOT(releaseResources()));

    updateDocument();
}

CppEditorSupport::~CppEditorSupport()
{
    m_documentParser.cancel();
    m_highlighter.cancel();
    m_futureSemanticInfo.cancel();

    m_documentParser.waitForFinished();
    m_highlighter.waitForFinished();
    m_futureSemanticInfo.waitForFinished();
}

QString CppEditorSupport::fileName() const
{
    return m_textEditor->document()->filePath();
}

QByteArray CppEditorSupport::contents() const
{
    QMutexLocker locker(&m_cachedContentsLock);

    const int editorRev = editorRevision();
    if (m_cachedContentsEditorRevision != editorRev && !m_fileIsBeingReloaded) {
        m_cachedContentsEditorRevision = editorRev;
        m_cachedContents = m_textEditor->textDocument()->plainText().toUtf8();
    }

    return m_cachedContents;
}

unsigned CppEditorSupport::editorRevision() const
{
    return m_textEditor->editorWidget()->document()->revision();
}

void CppEditorSupport::setExtraDiagnostics(const QString &key,
                                           const QList<Document::DiagnosticMessage> &messages)
{
    {
        QMutexLocker locker(&m_diagnosticsMutex);
        m_allDiagnostics.insert(key, messages);
    }

    emit diagnosticsChanged();
}

void CppEditorSupport::setIfdefedOutBlocks(const QList<BlockRange> &ifdefedOutBlocks)
{
    m_editorUpdates.ifdefedOutBlocks = ifdefedOutBlocks;

    emit diagnosticsChanged();
}

bool CppEditorSupport::initialized()
{
    return m_initialized;
}

SemanticInfo CppEditorSupport::recalculateSemanticInfo()
{
    m_futureSemanticInfo.cancel();
    return recalculateSemanticInfoNow(currentSource(false), /*emitSignalWhenFinished=*/ false);
}

Document::Ptr CppEditorSupport::lastSemanticInfoDocument() const
{
    return semanticInfo().doc;
}

void CppEditorSupport::recalculateSemanticInfoDetached(ForceReason forceReason)
{
    // Block premature calculation caused by CppEditorPlugin::currentEditorChanged
    // when the editor is created.
    if (!m_initialized)
        return;

    m_futureSemanticInfo.cancel();
    const bool force = forceReason != NoForce;
    SemanticInfo::Source source = currentSource(force);
    m_futureSemanticInfo = QtConcurrent::run<CppEditorSupport, void>(
                &CppEditorSupport::recalculateSemanticInfoDetached_helper, this, source);

    if (force && m_highlightingSupport && !m_highlightingSupport->requiresSemanticInfo())
        startHighlighting(forceReason);
}

CppCompletionAssistProvider *CppEditorSupport::completionAssistProvider() const
{
    return m_completionAssistProvider;
}

QSharedPointer<SnapshotUpdater> CppEditorSupport::snapshotUpdater()
{
    QSharedPointer<SnapshotUpdater> updater = snapshotUpdater_internal();
    if (!updater || updater->fileInEditor() != fileName()) {
        updater = QSharedPointer<SnapshotUpdater>(new SnapshotUpdater(fileName()));
        setSnapshotUpdater_internal(updater);

        QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
        updater->setUsePrecompiledHeaders(cms->pchUsage() != CppCodeModelSettings::PchUse_None);
    }
    return updater;
}

void CppEditorSupport::updateDocument()
{
    m_revision = editorRevision();

    if (qobject_cast<BaseTextEditorWidget*>(m_textEditor->widget()) != 0)
        m_updateEditorTimer->stop();

    m_updateDocumentTimer->start(m_updateDocumentInterval);
}

static void parse(QFutureInterface<void> &future, QSharedPointer<SnapshotUpdater> updater,
                  CppModelManagerInterface::WorkingCopy workingCopy)
{
    future.setProgressRange(0, 1);
    if (future.isCanceled()) {
        future.setProgressValue(1);
        return;
    }

    CppModelManager *cmm = qobject_cast<CppModelManager *>(CppModelManager::instance());
    updater->update(workingCopy);
    cmm->finishedRefreshingSourceFiles(QStringList(updater->fileInEditor()));

    future.setProgressValue(1);
}

void CppEditorSupport::updateDocumentNow()
{
    if (m_documentParser.isRunning() || m_revision != editorRevision()) {
        m_updateDocumentTimer->start(m_updateDocumentInterval);
    } else {
        m_updateDocumentTimer->stop();

        if (m_fileIsBeingReloaded || fileName().isEmpty())
            return;

        if (m_highlightingSupport && !m_highlightingSupport->requiresSemanticInfo())
            startHighlighting();

        m_documentParser = QtConcurrent::run(&parse, snapshotUpdater(),
                                             CppModelManager::instance()->workingCopy());
    }
}

bool CppEditorSupport::isUpdatingDocument()
{
    return m_updateDocumentTimer->isActive() || m_documentParser.isRunning();
}

void CppEditorSupport::onDocumentUpdated(Document::Ptr doc)
{
    if (doc.isNull())
        return;

    if (doc->fileName() != fileName())
        return; // some other document got updated

    if (doc->editorRevision() != editorRevision())
        return; // outdated content, wait for a new document to be parsed

    // Update the ifdeffed-out blocks:
    if (m_highlightingSupport && !m_highlightingSupport->hightlighterHandlesIfdefedOutBlocks()) {
        QList<Document::Block> skippedBlocks = doc->skippedBlocks();
        QList<BlockRange> ifdefedOutBlocks;
        ifdefedOutBlocks.reserve(skippedBlocks.size());
        foreach (const Document::Block &block, skippedBlocks)
            ifdefedOutBlocks.append(BlockRange(block.begin(), block.end()));
        setIfdefedOutBlocks(ifdefedOutBlocks);
    }

    if (m_highlightingSupport && !m_highlightingSupport->hightlighterHandlesDiagnostics()) {
        // Update the parser errors/warnings:
        static const QString key = QLatin1String("CppTools.ParserDiagnostics");
        setExtraDiagnostics(key, doc->diagnosticMessages());
    }

    // Update semantic info if necessary
    if (!m_initialized || (m_textEditor->widget()->isVisible() && !isSemanticInfoValid())) {
        m_initialized = true;
        recalculateSemanticInfoDetached(ForceDueToInvalidSemanticInfo);
    }

    // Notify the editor that the document is updated
    emit documentUpdated();
}

void CppEditorSupport::startHighlighting(ForceReason forceReason)
{
    if (!m_highlightingSupport)
        return;

    if (m_highlightingSupport->requiresSemanticInfo()) {
        const SemanticInfo info = semanticInfo();
        if (info.doc.isNull())
            return;

        const bool forced = info.forced || !m_lastHighlightOnCompleteSemanticInfo;
        if (!forced && m_lastHighlightRevision == info.revision)
            return;

        m_highlighter.cancel();
        m_highlighter = m_highlightingSupport->highlightingFuture(info.doc, info.snapshot);
        m_lastHighlightRevision = info.revision;
        m_lastHighlightOnCompleteSemanticInfo = info.complete;
        emit highlighterStarted(&m_highlighter, m_lastHighlightRevision);
    } else {
        const unsigned revision = editorRevision();
        if (forceReason != ForceDueEditorRequest && m_lastHighlightRevision == revision)
            return;

        m_highlighter.cancel();
        static const Document::Ptr dummyDoc;
        static const Snapshot dummySnapshot;
        m_highlighter = m_highlightingSupport->highlightingFuture(dummyDoc, dummySnapshot);
        m_lastHighlightRevision = revision;
        emit highlighterStarted(&m_highlighter, m_lastHighlightRevision);
    }
}

/// \brief This slot puts the new diagnostics into the editorUpdates. This function has to be called
///        on the UI thread.
void CppEditorSupport::onDiagnosticsChanged()
{
    QList<Document::DiagnosticMessage> allDiagnostics;
    {
        QMutexLocker locker(&m_diagnosticsMutex);
        foreach (const QList<Document::DiagnosticMessage> &msgs, m_allDiagnostics)
            allDiagnostics.append(msgs);
    }

    if (!m_textEditor)
        return;

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    // set up the format for the warnings.
    QTextCharFormat warningFormat;
    warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    warningFormat.setUnderlineColor(Qt::darkYellow);

    QTextDocument *doc = m_textEditor->editorWidget()->document();

    m_editorUpdates.selections.clear();
    foreach (const Document::DiagnosticMessage &m, allDiagnostics) {
        QTextEdit::ExtraSelection sel;
        if (m.isWarning())
            sel.format = warningFormat;
        else
            sel.format = errorFormat;

        QTextCursor c(doc->findBlockByNumber(m.line() - 1));
        const QString text = c.block().text();
        if (m.length() > 0 && m.column() + m.length() < (unsigned)text.size()) {
            int column = m.column() > 0 ? m.column() - 1 : 0;
            c.setPosition(c.position() + column);
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
        m_editorUpdates.selections.append(sel);
    }

    m_editorUpdates.revision = doc->revision();

    updateEditor();
}
void CppEditorSupport::updateEditor()
{
    m_updateEditorTimer->start(UpdateEditorInterval);
}

void CppEditorSupport::updateEditorNow()
{
    if (!m_textEditor)
        return;

    BaseTextEditorWidget *editorWidget = m_textEditor->editorWidget();
    if (editorWidget->document()->revision() != m_editorUpdates.revision)
        return; // outdated

    editorWidget->setExtraSelections(BaseTextEditorWidget::CodeWarningsSelection,
                                     m_editorUpdates.selections);
    editorWidget->setIfdefedOutBlocks(m_editorUpdates.ifdefedOutBlocks);
}

void CppEditorSupport::onCurrentEditorChanged()
{
    bool editorVisible = m_textEditor->widget()->isVisible();

    if (m_editorVisible != editorVisible) {
        m_editorVisible = editorVisible;
        if (editorVisible) {
            m_editorGCTimer->stop();
            if (!lastSemanticInfoDocument())
                updateDocumentNow();
        } else {
            m_editorGCTimer->start(EditorHiddenGCTimeout);
        }
    }
}

void CppEditorSupport::releaseResources()
{
    m_highlighter.cancel();
    m_highlighter = QFuture<TextEditor::HighlightingResult>();
    snapshotUpdater()->releaseSnapshot();
    setSemanticInfo(SemanticInfo(), /*emitSignal=*/ false);
    m_lastHighlightOnCompleteSemanticInfo = true;
}

SemanticInfo::Source CppEditorSupport::currentSource(bool force)
{
    int line = 0, column = 0;
    m_textEditor->convertPosition(m_textEditor->editorWidget()->position(), &line, &column);

    return SemanticInfo::Source(Snapshot(), fileName(), contents(), line, column, editorRevision(),
                                force);
}

SemanticInfo CppEditorSupport::recalculateSemanticInfoNow(const SemanticInfo::Source &source,
    bool emitSignalWhenFinished,
    FuturizedTopLevelDeclarationProcessor *processor)
{
    const SemanticInfo lastSemanticInfo = semanticInfo();
    SemanticInfo newSemanticInfo;

    newSemanticInfo.forced = source.force;
    newSemanticInfo.revision = source.revision;

    // Try to reuse as much as possible from the last semantic info
    if (!source.force
            && lastSemanticInfo.complete
            && lastSemanticInfo.revision == source.revision
            && lastSemanticInfo.doc
            && lastSemanticInfo.doc->translationUnit()->ast()
            && lastSemanticInfo.doc->fileName() == source.fileName) {
        newSemanticInfo.snapshot = lastSemanticInfo.snapshot; // ### TODO: use the new snapshot.
        newSemanticInfo.doc = lastSemanticInfo.doc;

    // Otherwise reprocess document
    } else {
        const QSharedPointer<SnapshotUpdater> snapshotUpdater = snapshotUpdater_internal();
        QTC_ASSERT(snapshotUpdater, return newSemanticInfo);
        newSemanticInfo.snapshot = snapshotUpdater->snapshot();
        QTC_ASSERT(newSemanticInfo.snapshot.contains(source.fileName), return newSemanticInfo);
        Document::Ptr doc = newSemanticInfo.snapshot.preprocessedDocument(source.code,
                                                                          source.fileName);
        if (processor)
            doc->control()->setTopLevelDeclarationProcessor(processor);
        doc->check();
        if (processor && processor->isCanceled())
            newSemanticInfo.complete = false;
        newSemanticInfo.doc = doc;
    }

    // Update local uses for the document
    TranslationUnit *translationUnit = newSemanticInfo.doc->translationUnit();
    AST *ast = translationUnit->ast();
    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(newSemanticInfo.doc->translationUnit());
    const LocalSymbols useTable(newSemanticInfo.doc,
                                functionDefinitionUnderCursor(ast, source.line, source.column));
    newSemanticInfo.localUses = useTable.uses;

    // Update semantic info
    setSemanticInfo(newSemanticInfo, emitSignalWhenFinished);

    return newSemanticInfo;
}

void CppEditorSupport::recalculateSemanticInfoDetached_helper(QFutureInterface<void> &future, SemanticInfo::Source source)
{
    FuturizedTopLevelDeclarationProcessor processor(future);
    recalculateSemanticInfoNow(source, true, &processor);
}

bool CppEditorSupport::isSemanticInfoValid() const
{
    const Document::Ptr document = lastSemanticInfoDocument();
    return document
            && document->translationUnit()->ast()
            && document->fileName() == fileName();
}

SemanticInfo CppEditorSupport::semanticInfo() const
{
    QMutexLocker locker(&m_lastSemanticInfoLock);
    return m_lastSemanticInfo;
}

void CppEditorSupport::setSemanticInfo(const SemanticInfo &semanticInfo, bool emitSignal)
{
    {
        QMutexLocker locker(&m_lastSemanticInfoLock);
        m_lastSemanticInfo = semanticInfo;
    }
    if (emitSignal)
        emit semanticInfoUpdated(semanticInfo);
}

QSharedPointer<SnapshotUpdater> CppEditorSupport::snapshotUpdater_internal() const
{
    QMutexLocker locker(&m_snapshotUpdaterLock);
    return m_snapshotUpdater;
}

void CppEditorSupport::setSnapshotUpdater_internal(const QSharedPointer<SnapshotUpdater> &updater)
{
    QMutexLocker locker(&m_snapshotUpdaterLock);
    m_snapshotUpdater = updater;
}

void CppEditorSupport::onMimeTypeChanged()
{
    m_highlighter.cancel();
    m_highlighter.waitForFinished();

    m_highlightingSupport.reset(m_modelManager->highlightingSupport(m_textEditor));

    disconnect(this, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
               this, SLOT(startHighlighting()));
    if (m_highlightingSupport && m_highlightingSupport->requiresSemanticInfo())
        connect(this, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
                this, SLOT(startHighlighting()));

    m_completionAssistProvider = m_modelManager->completionAssistProvider(m_textEditor);

    updateDocumentNow();
}

void CppEditorSupport::onAboutToReload()
{
    QTC_CHECK(!m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = true;
}

void CppEditorSupport::onReloadFinished()
{
    QTC_CHECK(m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = false;
    updateDocument();
}
