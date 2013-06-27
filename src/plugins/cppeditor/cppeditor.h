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

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppfunctiondecldeflink.h"

#include <cpptools/commentssettings.h>
#include <cpptools/cppsemanticinfo.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/uncommentselection.h>

#include <QFutureWatcher>
#include <QModelIndex>
#include <QMutex>
#include <QThread>
#include <QVector>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace CPlusPlus {
class OverviewModel;
class Symbol;
}

namespace CppTools {
class CppCodeStyleSettings;
class CppModelManagerInterface;
class CppRefactoringFile;
}

namespace TextEditor { class FontSettings; }

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    CPPEditor(CPPEditorWidget *);

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    Core::Id id() const;

    bool isTemporary() const { return false; }
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    const Utils::CommentDefinition *commentDefinition() const;
private:
    Utils::CommentDefinition m_commentDefinition;
};

class CPPEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    typedef TextEditor::TabSettings TabSettings;

    CPPEditorWidget(QWidget *parent);
    ~CPPEditorWidget();
    void unCommentSelection();

    unsigned editorRevision() const;
    bool isOutdated() const;
    CppTools::SemanticInfo semanticInfo() const;

    CPlusPlus::OverviewModel *outlineModel() const;
    QModelIndex outlineModelIndex();

    virtual void paste(); // reimplemented from BaseTextEditorWidget
    virtual void cut(); // reimplemented from BaseTextEditorWidget
    virtual void selectAll(); // reimplemented from BaseTextEditorWidget

    CppTools::CppModelManagerInterface *modelManager() const;

    virtual void setMimeType(const QString &mt);

    void setObjCEnabled(bool onoff);
    bool isObjCEnabled() const;

    bool openLink(const Link &link, bool inNextSplit) { return openCppEditorAt(link, inNextSplit); }

    static Link linkToSymbol(CPlusPlus::Symbol *symbol);

    static QVector<TextEditor::TextStyle> highlighterFormatCategories();

    virtual TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind kind,
                                                                TextEditor::AssistReason reason) const;

    QSharedPointer<FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

    void updateContentsChangedSignal();

Q_SIGNALS:
    void outlineModelIndexChanged(const QModelIndex &index);

public Q_SLOTS:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    virtual void setTabSettings(const TextEditor::TabSettings &);
    void setSortedOutline(bool sort);
    void switchDeclarationDefinition(bool inNextSplit);
    void renameSymbolUnderCursor();
    void renameUsages();
    void findUsages();
    void renameUsagesNow(const QString &replacement = QString());
    void semanticRehighlight(bool force = false);
    void highlighterStarted(QFuture<TextEditor::HighlightingResult> *highlighter,
                            unsigned revision);

protected:
    bool event(QEvent *e);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *e);

    TextEditor::BaseTextEditor *createEditor();

    const CPlusPlus::Macro *findCanonicalMacro(const QTextCursor &cursor,
                                               CPlusPlus::Document::Ptr doc) const;
protected Q_SLOTS:
    void slotCodeStyleSettingsChanged(const QVariant &);

private Q_SLOTS:
    void updateFileName();
    void jumpToOutlineElement(int index);
    void updateOutlineNow();
    void updateOutlineIndex();
    void updateOutlineIndexNow();
    void updateOutlineToolTip();
    void updateUses();
    void updateUsesNow();
    void updateFunctionDeclDefLink();
    void updateFunctionDeclDefLinkNow();
    void onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link);
    void onDocumentUpdated();
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

    void updateSemanticInfo(const CppTools::SemanticInfo &semanticInfo);
    void highlightSymbolUsages(int from, int to);
    void finishHighlightSymbolUsages();

    void markSymbolsNow();

    void performQuickFix(int index);

    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);

    void onCommentsSettingsChanged(const CppTools::CommentsSettings &settings);

private:
    void markSymbols(const QTextCursor &tc, const CppTools::SemanticInfo &info);
    bool sortedOutline() const;
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol,
                                      const CPlusPlus::Snapshot &snapshot) const;

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    void highlightUses(const QList<TextEditor::HighlightingResult> &uses,
                       QList<QTextEdit::ExtraSelection> *selections);

    void createToolBar(CPPEditor *editable);

    void startRename();
    void finishRename();
    void abortRename();

    Q_SLOT void abortDeclDefLink();

    Link attemptFuncDeclDef(const QTextCursor &cursor,
                            const CPlusPlus::Document::Ptr &doc,
                            CPlusPlus::Snapshot snapshot) const;
    Link findLinkAt(const QTextCursor &, bool resolveTarget = true);
    Link findMacroLink(const QByteArray &name) const;
    Link findMacroLink(const QByteArray &name, CPlusPlus::Document::Ptr doc,
                       const CPlusPlus::Snapshot &snapshot, QSet<QString> *processed) const;
    QString identifierUnderCursor(QTextCursor *macroCursor) const;
    bool openCppEditorAt(const Link &, bool inNextSplit = false);

    QModelIndex indexForPosition(int line, int column,
                                 const QModelIndex &rootIndex = QModelIndex()) const;

    bool handleDocumentationComment(QKeyEvent *e);
    bool isStartOfDoxygenComment(const QTextCursor &cursor) const;

    CppTools::CppModelManagerInterface *m_modelManager;

    QComboBox *m_outlineCombo;
    CPlusPlus::OverviewModel *m_outlineModel;
    QModelIndex m_outlineModelIndex;
    QSortFilterProxyModel *m_proxyModel;
    QAction *m_sortAction;
    QTimer *m_updateOutlineTimer;
    QTimer *m_updateOutlineIndexTimer;
    QTimer *m_updateUsesTimer;
    QTimer *m_updateFunctionDeclDefLinkTimer;
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrencesUnusedFormat;
    QTextCharFormat m_occurrenceRenameFormat;
    QHash<int, QTextCharFormat> m_semanticHighlightFormatMap;
    QTextCharFormat m_keywordFormat;

    QList<QTextEdit::ExtraSelection> m_renameSelections;
    int m_currentRenameSelection;
    static const int NoCurrentRenameSelection = -1;
    bool m_inRename, m_inRenameChanged, m_firstRenameChange;
    QTextCursor m_currentRenameSelectionBegin;
    QTextCursor m_currentRenameSelectionEnd;

    CppTools::SemanticInfo m_lastSemanticInfo;
    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;
    bool m_objcEnabled;

    QFuture<TextEditor::HighlightingResult> m_highlighter;
    QFutureWatcher<TextEditor::HighlightingResult> m_highlightWatcher;
    unsigned m_highlightRevision; // the editor revision that requested the highlight

    QFuture<QList<int> > m_references;
    QFutureWatcher<QList<int> > m_referencesWatcher;
    unsigned m_referencesRevision;
    int m_referencesCursorPosition;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;

    CppTools::CommentsSettings m_commentsSettings;

    CppTools::CppCompletionSupport *m_completionSupport;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
