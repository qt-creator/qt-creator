// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>

#include <texteditor/refactoroverlay.h>
#include <texteditor/textdocument.h>

#include <QTextEdit>

namespace CppEditor {

namespace Internal {
class OutlineModel;
class ParseContextModel;
}

class CursorInfo;
class CursorInfoParams;

class CPPEDITOR_EXPORT CppEditorDocument : public TextEditor::TextDocument
{
    Q_OBJECT

    friend class CppEditorDocumentHandleImpl;

public:
    explicit CppEditorDocument();
    ~CppEditorDocument() override;

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

    Internal::ParseContextModel &parseContextModel();
    Internal::OutlineModel &outlineModel();
    void updateOutline();

    QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params);
    TextEditor::TabSettings tabSettings() const override;

    bool usesClangd() const;

#ifdef WITH_TESTS
    QList<TextEditor::BlockRange> ifdefedOutBlocks() const;
#endif

signals:
    void codeWarningsUpdated(unsigned contentsRevision,
                             const QList<QTextEdit::ExtraSelection> selections,
                             const QList<TextEditor::RefactorMarker> &refactorMarkers);

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
    void removeTrailingWhitespace(const QTextBlock &block) override;

private:
    void processDocument();

    class Private;
    Private * const d;
};

} // namespace CppEditor
