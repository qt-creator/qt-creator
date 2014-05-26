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

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditordocument.h"

#include "cppfunctiondecldeflink.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcoverride.h>
#include <utils/uncommentselection.h>

#include <QScopedPointer>

namespace CPlusPlus { class Symbol; }
namespace CppTools { class SemanticInfo; }

namespace CppEditor {
namespace Internal {

class CppEditorOutline;
class CPPEditorWidget;
class CPPEditorWidgetPrivate;
class FollowSymbolUnderCursor;

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    CPPEditor(CPPEditorWidget *);

    Core::IEditor *duplicate() QTC_OVERRIDE;

    bool open(QString *errorString,
              const QString &fileName,
              const QString &realFileName) QTC_OVERRIDE;

    const Utils::CommentDefinition *commentDefinition() const QTC_OVERRIDE;
    TextEditor::CompletionAssistProvider *completionAssistProvider() QTC_OVERRIDE;

private:
    Utils::CommentDefinition m_commentDefinition;
};

class CPPEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    static Link linkToSymbol(CPlusPlus::Symbol *symbol);
    static QString identifierUnderCursor(QTextCursor *macroCursor);

public:
    CPPEditorWidget(QWidget *parent = 0);
    CPPEditorWidget(CPPEditorWidget *other);
    ~CPPEditorWidget();

    CPPEditorDocument *cppEditorDocument() const;
    CppEditorOutline *outline() const;

    CppTools::SemanticInfo semanticInfo() const;

    QSharedPointer<FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

    TextEditor::IAssistInterface *createAssistInterface(
            TextEditor::AssistKind kind,
            TextEditor::AssistReason reason) const QTC_OVERRIDE;

    FollowSymbolUnderCursor *followSymbolUnderCursorDelegate(); // exposed for tests

public slots:
    void paste() QTC_OVERRIDE;
    void cut() QTC_OVERRIDE;
    void selectAll() QTC_OVERRIDE;

    void unCommentSelection() QTC_OVERRIDE;
    void switchDeclarationDefinition(bool inNextSplit);
    void showPreProcessorWidget();

    void findUsages();
    void renameSymbolUnderCursor();
    void renameUsages(const QString &replacement = QString());

    void semanticRehighlight(bool force = false);
    void highlighterStarted(QFuture<TextEditor::HighlightingResult> *highlighter,
                            unsigned revision);

protected:
    bool event(QEvent *e) QTC_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *) QTC_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) QTC_OVERRIDE;

    void applyFontSettings() QTC_OVERRIDE;
    TextEditor::BaseTextEditor *createEditor() QTC_OVERRIDE;

    bool openLink(const Link &link, bool inNextSplit) QTC_OVERRIDE
    { return openCppEditorAt(link, inNextSplit); }

    Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                    bool inNextSplit = false) QTC_OVERRIDE;

protected slots:
    void slotCodeStyleSettingsChanged(const QVariant &) QTC_OVERRIDE;

private slots:
    void updateUses();
    void updateUsesNow();
    void updateFunctionDeclDefLink();
    void updateFunctionDeclDefLinkNow();
    void onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link);
    void onFilePathChanged();
    void onDocumentUpdated();
    void onContentsChanged(int position, int charsRemoved, int charsAdded);
    void updatePreprocessorButtonTooltip();

    void updateSemanticInfo(const CppTools::SemanticInfo &semanticInfo);
    void highlightSymbolUsages(int from, int to);
    void finishHighlightSymbolUsages();

    void markSymbolsNow();
    void performQuickFix(int index);
    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);
    void abortDeclDefLink();

    void onLocalRenamingFinished();
    void onLocalRenamingProcessKeyPressNormally(QKeyEvent *e);

private:
    static bool openCppEditorAt(const Link &, bool inNextSplit = false);

    CPPEditorWidget(TextEditor::BaseTextEditorWidget *); // avoid stupidity
    void ctor();

    void createToolBar(CPPEditor *editable);

    unsigned editorRevision() const;
    bool isOutdated() const;

    const CPlusPlus::Macro *findCanonicalMacro(const QTextCursor &cursor,
                                               CPlusPlus::Document::Ptr doc) const;

    QTextCharFormat textCharFormat(TextEditor::TextStyle category);

    void markSymbols(const QTextCursor &tc, const CppTools::SemanticInfo &info);

    QList<QTextEdit::ExtraSelection> createSelectionsFromUses(
            const QList<TextEditor::HighlightingResult> &uses);

private:
    QScopedPointer<CPPEditorWidgetPrivate> d;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
