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

    void scheduleProcessDocument();

    ParseContextModel &parseContextModel();
    OutlineModel &outlineModel();
    void updateOutline();

    QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params);
    TextEditor::TabSettings tabSettings() const override;

    bool usesClangd() const;

signals:
    void codeWarningsUpdated(unsigned contentsRevision,
                             const QList<QTextEdit::ExtraSelection> selections,
                             const TextEditor::RefactorMarkers &refactorMarkers);

    void ifdefedOutBlocksUpdated(unsigned contentsRevision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const SemanticInfo semanticInfo); // TODO: Remove me

    void preprocessorSettingsChanged(bool customSettings);

protected:
    void applyFontSettings() override;
    bool saveImpl(QString *errorString,
                  const Utils::FilePath &filePath = Utils::FilePath(),
                  bool autoSave = false) override;

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
};

} // namespace Internal
} // namespace CppEditor
