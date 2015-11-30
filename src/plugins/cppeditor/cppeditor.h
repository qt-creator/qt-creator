/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppfunctiondecldeflink.h"

#include <texteditor/texteditor.h>

#include <QScopedPointer>

namespace CppTools {
class CppEditorOutline;
class SemanticInfo;
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

public slots:
    void paste() override;
    void cut() override;
    void selectAll() override;

    void switchDeclarationDefinition(bool inNextSplit);
    void showPreProcessorWidget();

    void findUsages();
    void renameSymbolUnderCursor();
    void renameUsages(const QString &replacement = QString());

protected:
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool handleStringSplitting(QKeyEvent *e) const;

    Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker) override;

protected slots:
    void slotCodeStyleSettingsChanged(const QVariant &) override;

private slots:
    void updateFunctionDeclDefLink();
    void updateFunctionDeclDefLinkNow();
    void abortDeclDefLink();
    void onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link);

    void onCppDocumentUpdated();

    void onCodeWarningsUpdated(unsigned revision,
                               const QList<QTextEdit::ExtraSelection> selections);
    void onIfdefedOutBlocksUpdated(unsigned revision,
                                   const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void updateSemanticInfo(const CppTools::SemanticInfo &semanticInfo,
                            bool updateUseSelectionSynchronously = false);
    void updatePreprocessorButtonTooltip();

    void performQuickFix(int index);

    void processKeyNormally(QKeyEvent *e);

private:
    void finalizeInitialization() override;
    void finalizeInitializationAfterDuplication(TextEditorWidget *other) override;

    unsigned documentRevision() const;

private:
    QScopedPointer<CppEditorWidgetPrivate> d;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
