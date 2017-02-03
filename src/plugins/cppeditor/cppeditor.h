/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cppfunctiondecldeflink.h"

#include <texteditor/texteditor.h>

#include <QScopedPointer>

namespace CppTools {
class CppEditorOutline;
class RefactoringEngineInterface;
class SemanticInfo;
class ProjectPart;
}

namespace CppEditor {
namespace Internal {

class CppEditorDocument;

class CppEditorWidgetPrivate;
class FollowSymbolUnderCursor;
class FunctionDeclDefLink;

class CppEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    CppEditor();
};

class CppEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    CppEditorWidget();
    ~CppEditorWidget() override;

    CppEditorDocument *cppEditorDocument() const;
    CppTools::CppEditorOutline *outline() const;

    CppTools::SemanticInfo semanticInfo() const;
    bool isSemanticInfoValidExceptLocalUses() const;
    bool isSemanticInfoValid() const;

    QSharedPointer<FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

    TextEditor::AssistInterface *createAssistInterface(
            TextEditor::AssistKind kind,
            TextEditor::AssistReason reason) const override;

    FollowSymbolUnderCursor *followSymbolUnderCursorDelegate(); // exposed for tests

    void encourageApply() override;

    void paste() override;
    void cut() override;
    void selectAll() override;

    void switchDeclarationDefinition(bool inNextSplit);
    void showPreProcessorWidget();

    void findUsages();
    void renameSymbolUnderCursor();
    void renameUsages(const QString &replacement = QString());

    bool selectBlockUp() override;
    bool selectBlockDown() override;

    static void updateWidgetHighlighting(QWidget *widget, bool highlight);
    static bool isWidgetHighlighted(QWidget *widget);

protected:
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool handleStringSplitting(QKeyEvent *e) const;

    Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker) override;

    void slotCodeStyleSettingsChanged(const QVariant &) override;

private:
    void updateFunctionDeclDefLink();
    void updateFunctionDeclDefLinkNow();
    void abortDeclDefLink();
    void onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link);

    void onCppDocumentUpdated();

    void onCodeWarningsUpdated(unsigned revision,
                               const QList<QTextEdit::ExtraSelection> selections,
                               const TextEditor::RefactorMarkers &refactorMarkers);
    void onIfdefedOutBlocksUpdated(unsigned revision,
                                   const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void onShowInfoBarAction(const Core::Id &id, bool show);

    void updateSemanticInfo(const CppTools::SemanticInfo &semanticInfo,
                            bool updateUseSelectionSynchronously = false);
    void updatePreprocessorButtonTooltip();

    void processKeyNormally(QKeyEvent *e);

    void finalizeInitialization() override;
    void finalizeInitializationAfterDuplication(TextEditorWidget *other) override;

    unsigned documentRevision() const;

    TextEditor::RefactorMarkers refactorMarkersWithoutClangMarkers() const;

    CppTools::RefactoringEngineInterface *refactoringEngine() const;

    void renameSymbolUnderCursorClang();
    void renameSymbolUnderCursorBuiltin();

    CppTools::ProjectPart *projectPart() const;

private:
    QScopedPointer<CppEditorWidgetPrivate> d;
};

} // namespace Internal
} // namespace CppEditor
