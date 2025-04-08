// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseeditordocumentprocessor.h"
#include "cppcompletionassistprovider.h"
#include "cppoutlinemodel.h"
#include "cppparsecontext.h"
#include "cppsemanticinfo.h"
#include "editordocumenthandle.h"

#include <texteditor/textdocument.h>

#include <QMutex>
#include <QTimer>

namespace CppEditor {
namespace Internal {

class CppEditorDocument : public TextEditor::TextDocument
{
    Q_OBJECT

    friend class CppEditorDocumentHandleImpl;

public:
    explicit CppEditorDocument();

    bool isObjCEnabled() const;
    void setCompletionAssistProvider(TextEditor::CompletionAssistProvider *provider) override;
    TextEditor::CompletionAssistProvider *completionAssistProvider() const override;
    TextEditor::IAssistProvider *quickFixAssistProvider() const override;

    void recalculateSemanticInfoDetached();
    SemanticInfo recalculateSemanticInfo(); // TODO: Remove me

    void setPreferredParseContext(const QString &parseContextId);
    void setExtraPreprocessorDirectives(const QByteArray &directives);

    // the blocks list must be sorted
    void setIfdefedOutBlocks(const QList<TextEditor::BlockRange> &blocks);

    void scheduleProcessDocument();

    ParseContextModel &parseContextModel();
    OutlineModel &outlineModel();
    void updateOutline();

    QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params);
    TextEditor::TabSettings tabSettings() const override;

    bool usesClangd() const;

#ifdef WITH_TESTS
    QList<TextEditor::BlockRange> ifdefedOutBlocks() const { return m_ifdefedOutBlocks; }
#endif

signals:
    void codeWarningsUpdated(unsigned contentsRevision,
                             const QList<QTextEdit::ExtraSelection> selections,
                             const TextEditor::RefactorMarkers &refactorMarkers);

    void ifdefedOutBlocksUpdated(unsigned contentsRevision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const SemanticInfo semanticInfo); // TODO: Remove me

    void preprocessorSettingsChanged(bool customSettings);

#ifdef WITH_TESTS
    void ifdefedOutBlocksApplied();
#endif

protected:
    void applyFontSettings() override;
    Utils::Result<> saveImpl(const Utils::FilePath &filePath, bool autoSave) override;
    void slotCodeStyleSettingsChanged() override;

private:
    void invalidateFormatterCache();
    void onFilePathChanged(const Utils::FilePath &oldPath, const Utils::FilePath &newPath);
    void onMimeTypeChanged();

    void onAboutToReload();
    void onReloadFinished();
    void onDiagnosticsChanged(const Utils::FilePath &fileName, const QString &kind);


    void reparseWithPreferredParseContext(const QString &id);

    void processDocument();

    QByteArray contentsText() const;
    unsigned contentsRevision() const;

    BaseEditorDocumentProcessor *processor();
    void resetProcessor();
    void applyPreferredParseContextFromSettings();
    void applyExtraPreprocessorDirectivesFromSettings();
    void releaseResources();

    void showHideInfoBarAboutMultipleParseContexts(bool show);
    void applyIfdefedOutBlocks();

    void initializeTimer();

private:
    bool m_fileIsBeingReloaded = false;
    bool m_isObjCEnabled = false;

    // Caching contents
    mutable QMutex m_cachedContentsLock;
    mutable QByteArray m_cachedContents;
    mutable int m_cachedContentsRevision = -1;

    unsigned m_processorRevision = 0;
    QTimer m_processorTimer;
    QScopedPointer<BaseEditorDocumentProcessor> m_processor;

    CppCompletionAssistProvider *m_completionAssistProvider = nullptr;

    // (Un)Registration in CppModelManager
    QScopedPointer<CppEditorDocumentHandle> m_editorDocumentHandle;

    ParseContextModel m_parseContextModel;
    OutlineModel m_overviewModel;
    QList<TextEditor::BlockRange> m_ifdefedOutBlocks;
};

} // namespace Internal
} // namespace CppEditor
