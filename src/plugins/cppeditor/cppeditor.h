/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"
#include "cppquickfix.h"
#include <cplusplus/CppDocument.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace CPlusPlus {
class OverviewModel;
class Symbol;
}

namespace CppTools {
class CppModelManagerInterface;
}

namespace TextEditor {
class FontSettings;
}

namespace CppEditor {
namespace Internal {

class CPPEditor;
class SemanticHighlighter;

class SemanticInfo
{
public:
    struct Use {
        unsigned line;
        unsigned column;
        unsigned length;

        Use(unsigned line = 0, unsigned column = 0, unsigned length = 0)
            : line(line), column(column), length(length) {}
    };

    typedef QHash<CPlusPlus::Symbol *, QList<Use> > LocalUseMap;
    typedef QHashIterator<CPlusPlus::Symbol *, QList<Use> > LocalUseIterator;

    SemanticInfo()
        : revision(0), hasQ(false), hasD(false)
    { }

    unsigned revision;
    bool hasQ: 1;
    bool hasD: 1;
    CPlusPlus::Snapshot snapshot;
    CPlusPlus::Document::Ptr doc;
    LocalUseMap localUses;
};

class SemanticHighlighter: public QThread
{
    Q_OBJECT

public:
    SemanticHighlighter(QObject *parent = 0);
    virtual ~SemanticHighlighter();

    void abort();

    struct Source
    {
        CPlusPlus::Snapshot snapshot;
        QString fileName;
        QString code;
        int line;
        int column;
        int revision;
        bool force;

        Source()
            : line(0), column(0), revision(0), force(false)
        { }

        Source(const CPlusPlus::Snapshot &snapshot,
               const QString &fileName,
               const QString &code,
               int line, int column,
               int revision)
            : snapshot(snapshot), fileName(fileName),
              code(code), line(line), column(column),
              revision(revision), force(false)
        { }

        void clear()
        {
            snapshot = CPlusPlus::Snapshot();
            fileName.clear();
            code.clear();
            line = 0;
            column = 0;
            revision = 0;
            force = false;
        }
    };

    SemanticInfo semanticInfo(const Source &source);

    void rehighlight(const Source &source);

Q_SIGNALS:
    void changed(const SemanticInfo &semanticInfo);

protected:
    virtual void run();

private:
    bool isOutdated();

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_done;
    Source m_source;
    SemanticInfo m_lastSemanticInfo;
};

class CPPEditorEditable : public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT
public:
    CPPEditorEditable(CPPEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    const char *kind() const;

    bool isTemporary() const { return false; }
    virtual bool open(const QString & fileName);

private:
    QList<int> m_context;
};

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef TextEditor::TabSettings TabSettings;

    CPPEditor(QWidget *parent);
    ~CPPEditor();
    void unCommentSelection();

    void indentInsertedText(const QTextCursor &tc);

    unsigned editorRevision() const;
    bool isOutdated() const;
    SemanticInfo semanticInfo() const;

    virtual void paste(); // reimplemented from BaseTextEditor
    virtual void cut(); // reimplemented from BaseTextEditor

public Q_SLOTS:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    void setSortedMethodOverview(bool sort);
    void switchDeclarationDefinition();
    void jumpToDefinition();
    void renameSymbolUnderCursor();
    void renameUsages();
    void findUsages();
    void renameUsagesNow();
    void hideRenameNotification();

protected:
    bool event(QEvent *e);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *);

    TextEditor::BaseTextEditorEditable *createEditableInterface();

    // These override BaseTextEditor
    bool isElectricCharacter(const QChar &ch) const;
    QString autoComplete(QTextCursor &cursor, const QString &text) const;
    bool autoBackspace(QTextCursor &cursor);
    int paragraphSeparatorAboutToBeInserted(QTextCursor &cursor);

    bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                      const QString &textToInsert = QString()) const;

    bool isInComment(const QTextCursor &cursor) const;

    CPlusPlus::Symbol *findCanonicalSymbol(const QTextCursor &cursor,
                                           CPlusPlus::Document::Ptr doc,
                                           const CPlusPlus::Snapshot &snapshot) const;
    const CPlusPlus::Macro *findCanonicalMacro(const QTextCursor &cursor,
                                               CPlusPlus::Document::Ptr doc) const;

private Q_SLOTS:
    void updateFileName();
    void jumpToMethod(int index);
    void updateMethodBoxIndex();
    void updateMethodBoxIndexNow();
    void updateMethodBoxToolTip();
    void updateUses();
    void updateUsesNow();
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

    void semanticRehighlight();
    void updateSemanticInfo(const SemanticInfo &semanticInfo);

    void performQuickFix(int index);

private:
    bool showWarningMessage() const;
    void setShowWarningMessage(bool showWarningMessage);

    CPlusPlus::Symbol *markSymbols();
    bool sortedMethodOverview() const;
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol);
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    SemanticHighlighter::Source currentSource(bool force = false);

    void highlightUses(const QList<SemanticInfo::Use> &uses,
                       const SemanticInfo &semanticInfo,
                       QList<QTextEdit::ExtraSelection> *selections);

    void createToolBar(CPPEditorEditable *editable);

    enum EditOperation {
        DeleteChar,
        DeletePreviousChar,
        InsertText
    };
    void inAllRenameSelections(EditOperation operation,
                               const QTextEdit::ExtraSelection &currentRenameSelection,
                               QTextCursor cursor,
                               const QString &text = QString());
    void startRename();
    void finishRename();
    void abortRename();

    Link findLinkAt(const QTextCursor &, bool resolveTarget = true);
    bool openLink(const Link &link) { return openCppEditorAt(link); }
    bool openCppEditorAt(const Link &);

    static Link linkToSymbol(CPlusPlus::Symbol *symbol);

    CppTools::CppModelManagerInterface *m_modelManager;

    QList<int> m_contexts;
    QComboBox *m_methodCombo;
    CPlusPlus::OverviewModel *m_overviewModel;
    QSortFilterProxyModel *m_proxyModel;
    QAction *m_sortAction;
    QTimer *m_updateMethodBoxTimer;
    QTimer *m_updateUsesTimer;
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrencesUnusedFormat;
    QTextCharFormat m_occurrenceRenameFormat;

    QList<QTextEdit::ExtraSelection> m_renameSelections;
    int m_currentRenameSelection;
    bool m_inRename, m_inRenameChanged, m_firstRenameChange;
    QTextCursor m_currentRenameSelectionBegin;
    QTextCursor m_currentRenameSelectionEnd;

    mutable bool m_allowSkippingOfBlockEnd;

    SemanticHighlighter *m_semanticHighlighter;
    SemanticInfo m_lastSemanticInfo;
    QList<QuickFixOperationPtr> m_quickFixes;
    bool m_initialized;
};


} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
