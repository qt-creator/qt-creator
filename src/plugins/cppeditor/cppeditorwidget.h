/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <texteditor/texteditor.h>

#include <cpptools/cppeditorwidgetinterface.h>

#include <QScopedPointer>

namespace CppTools {
class CppEditorOutline;
class RefactoringEngineInterface;
class FollowSymbolInterface;
class SemanticInfo;
class ProjectPart;
}

namespace CppEditor {
namespace Internal {

class CppEditorDocument;

class CppEditorWidgetPrivate;
class FunctionDeclDefLink;

class CppEditorWidget : public TextEditor::TextEditorWidget,
        public CppTools::CppEditorWidgetInterface
{
    Q_OBJECT

public:
    CppEditorWidget();
    ~CppEditorWidget() override;

    CppEditorDocument *cppEditorDocument() const;
    CppTools::CppEditorOutline *outline() const;

    bool isSemanticInfoValidExceptLocalUses() const;
    bool isSemanticInfoValid() const;

    QSharedPointer<FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

    TextEditor::AssistInterface *createAssistInterface(
            TextEditor::AssistKind kind,
            TextEditor::AssistReason reason) const override;

    void encourageApply() override;

    void paste() override;
    void cut() override;
    void selectAll() override;

    void switchDeclarationDefinition(bool inNextSplit);
    void showPreProcessorWidget() override;

    void findUsages();
    void renameUsages(const QString &replacement = QString());
    void renameSymbolUnderCursor();

    bool selectBlockUp() override;
    bool selectBlockDown() override;

    static void updateWidgetHighlighting(QWidget *widget, bool highlight);
    static bool isWidgetHighlighted(QWidget *widget);

    CppTools::SemanticInfo semanticInfo() const override;
    void updateSemanticInfo() override;
    void invokeTextEditorWidgetAssist(TextEditor::AssistKind assistKind,
                                      TextEditor::IAssistProvider *provider) override;

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

    QMenu *createRefactorMenu(QWidget *parent) const;

    TextEditor::RefactorMarkers refactorMarkersWithoutClangMarkers() const;

    CppTools::FollowSymbolInterface &followSymbolInterface() const;
    CppTools::RefactoringEngineInterface &refactoringEngine() const;

    CppTools::ProjectPart *projectPart() const;

private:
    QScopedPointer<CppEditorWidgetPrivate> d;
};

} // namespace Internal
} // namespace CppEditor
