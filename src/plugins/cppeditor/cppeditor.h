/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"
#include "cppquickfix.h"
#include "cppsemanticinfo.h"

#include <cplusplus/ModelManagerInterface.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QFutureWatcher>
#include <QtCore/QModelIndex>
#include <QtCore/QVector>

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
    void changed(const CppEditor::Internal::SemanticInfo &semanticInfo);

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
    Core::Context context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;

    bool isTemporary() const { return false; }
    virtual bool open(const QString & fileName);

private:
    Core::Context m_context;
};

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef TextEditor::TabSettings TabSettings;

    CPPEditor(QWidget *parent);
    ~CPPEditor();
    void unCommentSelection();

    unsigned editorRevision() const;
    bool isOutdated() const;
    SemanticInfo semanticInfo() const;

    CPlusPlus::OverviewModel *outlineModel() const;
    QModelIndex outlineModelIndex();

    virtual void paste(); // reimplemented from BaseTextEditor
    virtual void cut(); // reimplemented from BaseTextEditor

    CPlusPlus::CppModelManagerInterface *modelManager() const;

    virtual void setMimeType(const QString &mt);

    void setObjCEnabled(bool onoff);
    bool isObjCEnabled() const;

    bool openLink(const Link &link) { return openCppEditorAt(link); }

    static Link linkToSymbol(CPlusPlus::Symbol *symbol);

    static QVector<QString> highlighterFormatCategories();

Q_SIGNALS:
    void outlineModelIndexChanged(const QModelIndex &index);

public Q_SLOTS:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    virtual void setTabSettings(const TextEditor::TabSettings &);
    void setSortedOutline(bool sort);
    void switchDeclarationDefinition();
    void jumpToDefinition();
    void renameSymbolUnderCursor();
    void renameUsages();
    void findUsages();
    void renameUsagesNow(const QString &replacement = QString());
    void hideRenameNotification();
    void rehighlight(bool force = false);

protected:
    bool event(QEvent *e);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *);

    TextEditor::BaseTextEditorEditable *createEditableInterface();

    const CPlusPlus::Macro *findCanonicalMacro(const QTextCursor &cursor,
                                               CPlusPlus::Document::Ptr doc) const;

private Q_SLOTS:
    void updateFileName();
    void jumpToOutlineElement(int index);
    void updateOutlineNow();
    void updateOutlineIndex();
    void updateOutlineIndexNow();
    void updateOutlineToolTip();
    void updateUses();
    void updateUsesNow();
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

    void semanticRehighlight();
    void updateSemanticInfo(const CppEditor::Internal::SemanticInfo &semanticInfo);
    void highlightSymbolUsages(int from, int to);
    void finishHighlightSymbolUsages();

    void markSymbolsNow();

    void performQuickFix(int index);

private:
    bool showWarningMessage() const;
    void setShowWarningMessage(bool showWarningMessage);

    void markSymbols(const QTextCursor &tc, const SemanticInfo &info);
    bool sortedOutline() const;
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol, const CPlusPlus::Snapshot &snapshot) const;

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    SemanticHighlighter::Source currentSource(bool force = false);

    void highlightUses(const QList<SemanticInfo::Use> &uses,
                       const SemanticInfo &semanticInfo,
                       QList<QTextEdit::ExtraSelection> *selections);

    void createToolBar(CPPEditorEditable *editable);

    void startRename();
    void finishRename();
    void abortRename();

    Link attemptFuncDeclDef(const QTextCursor &cursor,
                            const CPlusPlus::Document::Ptr &doc,
                            CPlusPlus::Snapshot snapshot) const;
    Link findLinkAt(const QTextCursor &, bool resolveTarget = true);
    Link findMacroLink(const QByteArray &name) const;
    Link findMacroLink(const QByteArray &name, CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot,
                       QSet<QString> *processed) const;
    QString identifierUnderCursor(QTextCursor *macroCursor) const;
    bool openCppEditorAt(const Link &);

    QModelIndex indexForPosition(int line, int column, const QModelIndex &rootIndex = QModelIndex()) const;

    CPlusPlus::CppModelManagerInterface *m_modelManager;

    QComboBox *m_outlineCombo;
    CPlusPlus::OverviewModel *m_outlineModel;
    QModelIndex m_outlineModelIndex;
    QSortFilterProxyModel *m_proxyModel;
    QAction *m_sortAction;
    QTimer *m_updateOutlineTimer;
    QTimer *m_updateOutlineIndexTimer;
    QTimer *m_updateUsesTimer;
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrencesUnusedFormat;
    QTextCharFormat m_occurrenceRenameFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_localFormat;
    QTextCharFormat m_fieldFormat;
    QTextCharFormat m_staticFormat;
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_virtualMethodFormat;

    QList<QTextEdit::ExtraSelection> m_renameSelections;
    int m_currentRenameSelection;
    static const int NoCurrentRenameSelection = -1;
    bool m_inRename, m_inRenameChanged, m_firstRenameChange;
    QTextCursor m_currentRenameSelectionBegin;
    QTextCursor m_currentRenameSelectionEnd;

    SemanticHighlighter *m_semanticHighlighter;
    SemanticInfo m_lastSemanticInfo;
    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;
    bool m_objcEnabled;
    bool m_initialized;

    QFuture<SemanticInfo::Use> m_highlighter;
    QFutureWatcher<SemanticInfo::Use> m_highlightWatcher;
    unsigned m_highlightRevision; // the editor revision that requested the highlight
    int m_nextHighlightBlockNumber;

    QFuture<QList<int> > m_references;
    QFutureWatcher<QList<int> > m_referencesWatcher;
    unsigned m_referencesRevision;
    int m_referencesCursorPosition;
};


} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
