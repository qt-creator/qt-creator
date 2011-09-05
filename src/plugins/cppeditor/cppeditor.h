/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"
#include "cppsemanticinfo.h"
#include "cppfunctiondecldeflink.h"

#include <cplusplus/ModelManagerInterface.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/quickfix.h>

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
class CppCodeStyleSettings;
class CppRefactoringFile;
}

namespace TextEditor {
class FontSettings;
}

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;

class SemanticHighlighter: public QThread, CPlusPlus::TopLevelDeclarationProcessor
{
    Q_OBJECT

public:
    SemanticHighlighter(QObject *parent = 0);
    virtual ~SemanticHighlighter();

    virtual bool processDeclaration(CPlusPlus::DeclarationAST *) { return m_done; }

    void abort();

    struct Source
    {
        CPlusPlus::Snapshot snapshot;
        QString fileName;
        QString code;
        int line;
        int column;
        unsigned revision;
        bool force;

        Source()
            : line(0), column(0), revision(0), force(false)
        { }

        Source(const CPlusPlus::Snapshot &snapshot,
               const QString &fileName,
               const QString &code,
               int line, int column,
               unsigned revision)
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

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    CPPEditor(CPPEditorWidget *);

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;

    bool isTemporary() const { return false; }
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
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
    SemanticInfo semanticInfo() const;

    CPlusPlus::OverviewModel *outlineModel() const;
    QModelIndex outlineModelIndex();

    virtual void paste(); // reimplemented from BaseTextEditorWidget
    virtual void cut(); // reimplemented from BaseTextEditorWidget

    CPlusPlus::CppModelManagerInterface *modelManager() const;

    virtual void setMimeType(const QString &mt);

    void setObjCEnabled(bool onoff);
    bool isObjCEnabled() const;

    bool openLink(const Link &link) { return openCppEditorAt(link); }

    static Link linkToSymbol(CPlusPlus::Symbol *symbol);

    static QVector<QString> highlighterFormatCategories();

    virtual TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind kind,
                                                                TextEditor::AssistReason reason) const;

    QSharedPointer<FunctionDeclDefLink> declDefLink() const;
    void applyDeclDefLinkChanges(bool jumpToMatch);

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
    void rehighlight(bool force = false);

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
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

    void semanticRehighlight();
    void updateSemanticInfo(const CppEditor::Internal::SemanticInfo &semanticInfo);
    void highlightSymbolUsages(int from, int to);
    void finishHighlightSymbolUsages();

    void markSymbolsNow();

    void performQuickFix(int index);

    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);

private:
    void markSymbols(const QTextCursor &tc, const SemanticInfo &info);
    bool sortedOutline() const;
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol, const CPlusPlus::Snapshot &snapshot) const;

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    SemanticHighlighter::Source currentSource(bool force = false);

    void highlightUses(const QList<SemanticInfo::Use> &uses,
                       const SemanticInfo &semanticInfo,
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

    SemanticHighlighter *m_semanticHighlighter;
    SemanticInfo m_lastSemanticInfo;
    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;
    bool m_objcEnabled;
    bool m_initialized;

    QFuture<SemanticInfo::Use> m_highlighter;
    QFutureWatcher<SemanticInfo::Use> m_highlightWatcher;
    unsigned m_highlightRevision; // the editor revision that requested the highlight

    QFuture<QList<int> > m_references;
    QFutureWatcher<QList<int> > m_referencesWatcher;
    unsigned m_referencesRevision;
    int m_referencesCursorPosition;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;
};


} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
