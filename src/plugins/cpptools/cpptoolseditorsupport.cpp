/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "cpptoolseditorsupport.h"
#include "cppmodelmanager.h"
#include "cpplocalsymbols.h"

#include <coreplugin/editormanager/editormanager.h>

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

        else if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            return checkDeclaration(def);
        }

        else if (ObjCMethodDeclarationAST *method = ast->asObjCMethodDeclaration()) {
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
    , m_cachedContentsEditorRevision(-1)
    , m_initialized(false)
    , m_lastHighlightRevision(0)
{
    connect(m_modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setSingleShot(true);
    m_updateDocumentTimer->setInterval(m_updateDocumentInterval);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    m_updateEditorTimer = new QTimer(this);
    m_updateEditorTimer->setInterval(UpdateEditorInterval);
    m_updateEditorTimer->setSingleShot(true);
    connect(m_updateEditorTimer, SIGNAL(timeout()),
            this, SLOT(updateEditorNow()));

    connect(m_textEditor, SIGNAL(contentsChanged()), this, SLOT(updateDocument()));
    connect(this, SIGNAL(diagnosticsChanged()), this, SLOT(onDiagnosticsChanged()));

    connect(m_textEditor->document(), SIGNAL(mimeTypeChanged()),
            this, SLOT(onMimeTypeChanged()));
}

CppEditorSupport::~CppEditorSupport()
{
    m_highlighter.cancel();
    m_futureSemanticInfo.cancel();

    m_highlighter.waitForFinished();
    m_futureSemanticInfo.waitForFinished();
}

QString CppEditorSupport::fileName() const
{
    return m_textEditor->document()->fileName();
}

QString CppEditorSupport::contents() const
{
    const int editorRev = editorRevision();
    if (m_cachedContentsEditorRevision != editorRev) {
        m_cachedContentsEditorRevision = editorRev;
        m_cachedContents = m_textEditor->textDocument()->contents();
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

bool CppEditorSupport::initialized()
{
    return m_initialized;
}

SemanticInfo CppEditorSupport::recalculateSemanticInfo(bool emitSignalWhenFinished)
{
    m_futureSemanticInfo.cancel();

    SemanticInfo::Source source = currentSource(false);
    recalculateSemanticInfoNow(source, emitSignalWhenFinished);
    return m_lastSemanticInfo;
}

void CppEditorSupport::recalculateSemanticInfoDetached(bool force)
{
    // Block premature calculation caused by CppEditorPlugin::currentEditorChanged
    // when the editor is created.
    if (!m_initialized)
        return;

    m_futureSemanticInfo.cancel();
    SemanticInfo::Source source = currentSource(force);
    m_futureSemanticInfo = QtConcurrent::run<CppEditorSupport, void>(
                &CppEditorSupport::recalculateSemanticInfoDetached_helper, this, source);

    if (force && m_highlightingSupport && !m_highlightingSupport->requiresSemanticInfo())
        startHighlighting();
}

void CppEditorSupport::updateDocument()
{
    m_revision = editorRevision();

    if (qobject_cast<BaseTextEditorWidget*>(m_textEditor->widget()) != 0)
        m_updateEditorTimer->stop();

    m_updateDocumentTimer->start(m_updateDocumentInterval);
}

void CppEditorSupport::updateDocumentNow()
{
    if (m_documentParser.isRunning() || m_revision != editorRevision()) {
        m_updateDocumentTimer->start(m_updateDocumentInterval);
    } else {
        m_updateDocumentTimer->stop();

        if (m_highlightingSupport && !m_highlightingSupport->requiresSemanticInfo()) {
            startHighlighting();
        }

        const QStringList sourceFiles(m_textEditor->document()->fileName());
        m_documentParser = m_modelManager->updateSourceFiles(sourceFiles);
    }
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
    QList<Document::Block> skippedBlocks = doc->skippedBlocks();
    m_editorUpdates.ifdefedOutBlocks.clear();
    m_editorUpdates.ifdefedOutBlocks.reserve(skippedBlocks.size());
    foreach (const Document::Block &block, skippedBlocks) {
        m_editorUpdates.ifdefedOutBlocks.append(BlockRange(block.begin(), block.end()));
    }

    if (m_highlightingSupport && !m_highlightingSupport->hightlighterHandlesDiagnostics()) {
        // Update the parser errors/warnings:
        static const QString key = QLatin1String("CppTools.ParserDiagnostics");
        setExtraDiagnostics(key, doc->diagnosticMessages());
    }

    // update semantic info in a future
    if (! m_initialized ||
            (m_textEditor->widget()->isVisible()
             && (m_lastSemanticInfo.doc.isNull()
                 || m_lastSemanticInfo.doc->translationUnit()->ast() == 0
                 || m_lastSemanticInfo.doc->fileName() != fileName()))) {
        m_initialized = true;
        recalculateSemanticInfoDetached(/* force = */ true);
    }

    // notify the editor that the document is updated
    emit documentUpdated();
}

void CppEditorSupport::startHighlighting()
{
    if (!m_highlightingSupport)
        return;

    // Start highlighting only if the editor is or would be visible
    // (in case another mode is active) in the edit mode.
    if (!Core::EditorManager::instance()->visibleEditors().contains(m_textEditor))
        return;

    if (m_highlightingSupport->requiresSemanticInfo()) {
        Snapshot snapshot;
        Document::Ptr doc;
        unsigned revision;
        bool forced;

        {
            QMutexLocker locker(&m_lastSemanticInfoLock);
            snapshot = m_lastSemanticInfo.snapshot;
            doc = m_lastSemanticInfo.doc;
            revision = m_lastSemanticInfo.revision;
            forced = m_lastSemanticInfo.forced;
        }

        if (doc.isNull())
            return;
        if (!forced && m_lastHighlightRevision == revision)
            return;
        m_highlighter.cancel();

        m_highlighter = m_highlightingSupport->highlightingFuture(doc, snapshot);
        m_lastHighlightRevision = revision;
        emit highlighterStarted(&m_highlighter, m_lastHighlightRevision);
    } else {
        static const Document::Ptr dummyDoc;
        static const Snapshot dummySnapshot;
        m_highlighter = m_highlightingSupport->highlightingFuture(dummyDoc, dummySnapshot);
        m_lastHighlightRevision = editorRevision();
        emit highlighterStarted(&m_highlighter, m_lastHighlightRevision);
    }
}

/// \brief This slot puts the new diagnostics into the editorUpdates. This method has to be called
///        on the UI thread.
void CppEditorSupport::onDiagnosticsChanged()
{
    QList<Document::DiagnosticMessage> allDiagnostics;
    {
        QMutexLocker locker(&m_diagnosticsMutex);
        foreach (const QList<Document::DiagnosticMessage> &msgs, m_allDiagnostics.values())
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
                if (! text.at(i).isSpace()) {
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

SemanticInfo::Source CppEditorSupport::currentSource(bool force)
{
    int line = 0, column = 0;
    m_textEditor->convertPosition(m_textEditor->editorWidget()->position(), &line, &column);

    const Snapshot snapshot = m_modelManager->snapshot();

    QString code;
    if (force || m_lastSemanticInfo.revision != editorRevision())
        code = contents(); // get the source code only when needed.

    const unsigned revision = editorRevision();
    SemanticInfo::Source source(snapshot, fileName(), code, line, column, revision, force);
    return source;
}

void CppEditorSupport::recalculateSemanticInfoNow(const SemanticInfo::Source &source,
                                                  bool emitSignalWhenFinished,
                                                  TopLevelDeclarationProcessor *processor)
{
    SemanticInfo semanticInfo;

    {
        QMutexLocker locker(&m_lastSemanticInfoLock);
        semanticInfo.revision = m_lastSemanticInfo.revision;
        semanticInfo.forced = source.force;

        if (!source.force
                && m_lastSemanticInfo.revision == source.revision
                && m_lastSemanticInfo.doc
                && m_lastSemanticInfo.doc->translationUnit()->ast()
                && m_lastSemanticInfo.doc->fileName() == source.fileName) {
            semanticInfo.snapshot = m_lastSemanticInfo.snapshot; // ### TODO: use the new snapshot.
            semanticInfo.doc = m_lastSemanticInfo.doc;
        }
    }

    if (semanticInfo.doc.isNull()) {
        semanticInfo.snapshot = source.snapshot;
        if (source.snapshot.contains(source.fileName)) {
            Document::Ptr doc = source.snapshot.preprocessedDocument(source.code, source.fileName);
            if (processor)
                doc->control()->setTopLevelDeclarationProcessor(processor);
            doc->check();
            semanticInfo.doc = doc;
        } else {
            return;
        }
    }

    if (semanticInfo.doc) {
        TranslationUnit *translationUnit = semanticInfo.doc->translationUnit();
        AST * ast = translationUnit->ast();

        FunctionDefinitionUnderCursor functionDefinitionUnderCursor(semanticInfo.doc->translationUnit());
        DeclarationAST *currentFunctionDefinition = functionDefinitionUnderCursor(ast, source.line, source.column);

        const LocalSymbols useTable(semanticInfo.doc, currentFunctionDefinition);
        semanticInfo.revision = source.revision;
        semanticInfo.localUses = useTable.uses;
    }

    {
        QMutexLocker locker(&m_lastSemanticInfoLock);
        m_lastSemanticInfo = semanticInfo;
    }

    if (emitSignalWhenFinished)
        emit semanticInfoUpdated(semanticInfo);
}

void CppEditorSupport::recalculateSemanticInfoDetached_helper(QFutureInterface<void> &future, SemanticInfo::Source source)
{
    class TLDProc: public TopLevelDeclarationProcessor
    {
        QFutureInterface<void> m_theFuture;

    public:
        TLDProc(QFutureInterface<void> &aFuture): m_theFuture(aFuture) {}
        virtual ~TLDProc() {}
        virtual bool processDeclaration(DeclarationAST *ast) {
            Q_UNUSED(ast);
            return m_theFuture.isCanceled();
        }
    };

    TLDProc tldProc(future);
    recalculateSemanticInfoNow(source, true, &tldProc);
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

    updateDocumentNow();
}
