// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/blockrange.h>
#include <texteditor/codeassist/assistenums.h>
#include <texteditor/texteditor.h>

#include <QScopedPointer>

#include <functional>

namespace TextEditor {
class IAssistProposal;
class IAssistProvider;
}

namespace CppEditor {
class SemanticInfo;
class ProjectPart;

namespace Internal {
class CppEditorDocument;
class CppEditorOutline;
class CppEditorWidgetPrivate;
class FunctionDeclDefLink;
} // namespace Internal

class CPPEDITOR_EXPORT CppEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    CppEditorWidget();
    ~CppEditorWidget() override;

    Internal::CppEditorDocument *cppEditorDocument() const;

    bool isSemanticInfoValidExceptLocalUses() const;
    bool isSemanticInfoValid() const;
    bool isRenaming() const;

    QSharedPointer<Internal::FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

    std::unique_ptr<TextEditor::AssistInterface> createAssistInterface(
            TextEditor::AssistKind kind,
            TextEditor::AssistReason reason) const override;

    void encourageApply() override;

    void paste() override;
    void cut() override;
    void selectAll() override;

    void switchDeclarationDefinition(bool inNextSplit);
    void showPreProcessorWidget();

    void findUsages() override;
    void findUsages(QTextCursor cursor);
    void renameUsages(const QString &replacement = QString(),
                      QTextCursor cursor = QTextCursor());
    void renameUsages(const Utils::FilePath &filePath,
                      const QString &replacement = QString(),
                      QTextCursor cursor = QTextCursor(),
                      const std::function<void()> &callback = {});
    void renameSymbolUnderCursor() override;

    bool selectBlockUp() override;
    bool selectBlockDown() override;

    static void updateWidgetHighlighting(QWidget *widget, bool highlight);
    static bool isWidgetHighlighted(QWidget *widget);

    SemanticInfo semanticInfo() const;
    void updateSemanticInfo();
    void invokeTextEditorWidgetAssist(TextEditor::AssistKind assistKind,
                                      TextEditor::IAssistProvider *provider);

    static const QList<QTextEdit::ExtraSelection>
    unselectLeadingWhitespace(const QList<QTextEdit::ExtraSelection> &selections);

    bool isInTestMode() const;
    void setProposals(const TextEditor::IAssistProposal *immediateProposal,
                      const TextEditor::IAssistProposal *finalProposal);
#ifdef WITH_TESTS
    void enableTestMode();
signals:
    void proposalsReady(const TextEditor::IAssistProposal *immediateProposal,
                        const TextEditor::IAssistProposal *finalProposal);
#endif

protected:
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool handleStringSplitting(QKeyEvent *e) const;

    void findLinkAt(const QTextCursor &cursor,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void findTypeAt(const QTextCursor &cursor,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void slotCodeStyleSettingsChanged(const QVariant &) override;

private:
    void updateFunctionDeclDefLink();
    void updateFunctionDeclDefLinkNow();
    void abortDeclDefLink();
    void onFunctionDeclDefLinkFound(QSharedPointer<Internal::FunctionDeclDefLink> link);

    void onCodeWarningsUpdated(unsigned revision,
                               const QList<QTextEdit::ExtraSelection> selections,
                               const TextEditor::RefactorMarkers &refactorMarkers);
    void onIfdefedOutBlocksUpdated(unsigned revision,
                                   const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void updateSemanticInfo(const SemanticInfo &semanticInfo,
                            bool updateUseSelectionSynchronously = false);
    void updatePreprocessorButtonTooltip();

    void processKeyNormally(QKeyEvent *e);

    void finalizeInitialization() override;
    void finalizeInitializationAfterDuplication(TextEditorWidget *other) override;

    unsigned documentRevision() const;
    bool isOldStyleSignalOrSlot() const;
    bool followUrl(const QTextCursor &cursor, const Utils::LinkHandler &processLinkCallback);

    QMenu *createRefactorMenu(QWidget *parent) const;

    const ProjectPart *projectPart() const;

    void handleOutlineChanged(const QWidget* newOutline);
    void showRenameWarningIfFileIsGenerated(const Utils::FilePath &filePath);
    void addRefactoringActions(QMenu *menu) const;

private:
    QScopedPointer<Internal::CppEditorWidgetPrivate> d;
};

} // namespace CppEditor
