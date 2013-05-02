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

#ifndef CPPTOOLSEDITORSUPPORT_H
#define CPPTOOLSEDITORSUPPORT_H

#include "cpphighlightingsupport.h"
#include "cppmodelmanager.h"
#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>

#include <QFuture>
#include <QObject>
#include <QPointer>
#include <QTimer>

namespace CPlusPlus { class AST; }

namespace TextEditor {
class BaseTextEditor;
class ITextMark;
} // namespace TextEditor

namespace CppTools {

/**
 * \brief The CppEditorSupport class oversees the actions that happen when a C++ text editor updates
 *        its document.
 *
 * The following steps are taken:
 * 1. the text editor fires a contentsChanged() signal that triggers updateDocument
 * 2. update document will start a timer, or reset the timer if it was already running. This way
 *    subsequent updates (e.g. keypresses) get bunched together instead of running the subsequent
 *    actions for every key press
 * 3. when the timer from step 2 fires, updateDocumentNow() is triggered. That tells the
 *    model-manager to update the CPlusPlus::Document by re-indexing it.
 * 4. when the model-manager finishes, it fires a documentUpdated(CPlusPlus::Document::Ptr) signal,
 *    that is connected to onDocumentUpdated(CPlusPlus::Document::Ptr), which does 4 things:
 *    a) updates the ifdeffed-out blocks in the EditorUpdate
 *    b) calls setExtraDiagnostics with the diagnostics from the parser, which in turn calls
 *       onDiagnosticsChanged on the UI thread, and that schedules an editor update timer. When this
 *       timer fires, updateEditorNow() is called, which will apply the updates to the editor.
 *    c) a semantic-info recalculation is started in a future
 *    d) the documentUpdated() signal is emitted, which can be used by a widget to do things
 * 5. semantic-info calculation from 4c is done by a future that calls recalculateSemanticInfoNow(),
 *    which emits semanticInfoUpdated() when it is finished. Editors can also listen in on this
 *    signal to do things like highlighting the local usages.
 * 6. the semanticInfoUpdated() is connected to the startHighlighting() slot, which will start
 *    another future for doing the semantic highlighting. The highlighterStarted signal is emitted,
 *    with the highlighting future as a parameter, so editors can hook it up to a QFutureWatcher
 *    and get notifications.
 *
 * Both the semantic info calculation and the highlighting calculation will cancel an already running
 * future. They will also check that the result of a previous step is not already outdated, meaning
 * that they check the revision of the editor document to see if a user changed the document while
 * the calculation was running.
 */
class CPPTOOLS_EXPORT CppEditorSupport: public QObject
{
    Q_OBJECT

public:
    CppEditorSupport(Internal::CppModelManager *modelManager, TextEditor::BaseTextEditor *textEditor);
    virtual ~CppEditorSupport();

    QString fileName() const;

    QString contents() const;
    unsigned editorRevision() const;

    void setExtraDiagnostics(const QString &key,
                             const QList<CPlusPlus::Document::DiagnosticMessage> &messages);

    /// True after the document was parsed/updated for the first time
    /// and the first semantic info calculation was started.
    bool initialized();

    /// Retrieve the semantic info, which will get recalculated on the current
    /// thread if it is outdate.
    SemanticInfo recalculateSemanticInfo(bool emitSignalWhenFinished = true);

    /// Recalculates the semantic info in a future, and will emit the
    /// semanticInfoUpdated() signal when finished.
    /// Requires that initialized() is true.
    /// \param force do not check if the old semantic info is still valid
    void recalculateSemanticInfoDetached(bool force = false);

signals:
    void documentUpdated();
    void diagnosticsChanged();
    void semanticInfoUpdated(CppTools::SemanticInfo);
    void highlighterStarted(QFuture<TextEditor::HighlightingResult> *, unsigned revision);

private slots:
    void onMimeTypeChanged();

    void updateDocument();
    void updateDocumentNow();

    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void startHighlighting();

    void onDiagnosticsChanged();

    void updateEditor();
    void updateEditorNow();

private:
    typedef TextEditor::BaseTextEditorWidget::BlockRange BlockRange;
    struct EditorUpdates {
        EditorUpdates()
            : revision(-1)
        {}
        int revision;
        QList<QTextEdit::ExtraSelection> selections;
        QList<BlockRange> ifdefedOutBlocks;
    };

    enum {
        UpdateDocumentDefaultInterval = 150,
        UpdateEditorInterval = 300
    };

private:
    SemanticInfo::Source currentSource(bool force);
    void recalculateSemanticInfoNow(const SemanticInfo::Source &source, bool emitSignalWhenFinished,
                                    CPlusPlus::TopLevelDeclarationProcessor *processor = 0);
    void recalculateSemanticInfoDetached_helper(QFutureInterface<void> &future,
                                                SemanticInfo::Source source);

private:
    Internal::CppModelManager *m_modelManager;
    QPointer<TextEditor::BaseTextEditor> m_textEditor;
    QTimer *m_updateDocumentTimer;
    int m_updateDocumentInterval;
    unsigned m_revision;
    QFuture<void> m_documentParser;

    // content caching
    mutable QString m_cachedContents;
    mutable int m_cachedContentsEditorRevision;

    QTimer *m_updateEditorTimer;
    EditorUpdates m_editorUpdates;

    QMutex m_diagnosticsMutex;
    QHash<QString, QList<CPlusPlus::Document::DiagnosticMessage> > m_allDiagnostics;

    // Semantic info:
    bool m_initialized;
    mutable QMutex m_lastSemanticInfoLock;
    SemanticInfo m_lastSemanticInfo;
    QFuture<void> m_futureSemanticInfo;

    // Highlighting:
    unsigned m_lastHighlightRevision;
    QFuture<TextEditor::HighlightingResult> m_highlighter;
    QScopedPointer<CppTools::CppHighlightingSupport> m_highlightingSupport;
};

} // namespace CppTools

#endif // CPPTOOLSEDITORSUPPORT_H
