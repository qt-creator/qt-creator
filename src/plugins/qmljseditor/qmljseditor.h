/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSEDITOR_H
#define QMLJSEDITOR_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljsinterpreter.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/quickfix.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QModelIndex>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace QmlJS {
    class ModelManagerInterface;
    class IContextPane;
    class LookupContext;
}

/*!
    The top-level namespace of the QmlJSEditor plug-in.
 */
namespace QmlJSEditor {
class QmlJSEditorEditable;
class FindReferences;

namespace Internal {
class QmlOutlineModel;
class SemanticHighlighter;
struct SemanticHighlighterSource;
} // namespace Internal

struct QMLJSEDITOR_EXPORT Declaration
{
    QString text;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;

    Declaration()
        : startLine(0),
        startColumn(0),
        endLine(0),
        endColumn(0)
    { }
};

class QMLJSEDITOR_EXPORT Range
{
public:
    Range(): ast(0) {}

public: // attributes
    QmlJS::AST::Node *ast;
    QTextCursor begin;
    QTextCursor end;
};

class QMLJSEDITOR_EXPORT SemanticInfo
{
public:
    SemanticInfo() {}

    bool isValid() const;
    int revision() const;

    // Returns the declaring member
    QmlJS::AST::Node *declaringMember(int cursorPosition) const;
    QmlJS::AST::Node *declaringMemberNoProperties(int cursorPosition) const;

    // Returns the AST node under cursor
    QmlJS::AST::Node *nodeUnderCursor(int cursorPosition) const;

    // Returns the list of nodes that enclose the given position.
    QList<QmlJS::AST::Node *> astPath(int cursorPosition) const;

    // Returns a context for the given path
    QSharedPointer<QmlJS::LookupContext> lookupContext(const QList<QmlJS::AST::Node *> &path = QList<QmlJS::AST::Node *>()) const;

public: // attributes
    QmlJS::Document::Ptr document;
    QmlJS::Snapshot snapshot;
    QList<Range> ranges;
    QHash<QString, QList<QmlJS::AST::SourceLocation> > idLocations;
    QList<Declaration> declarations;

    // these are in addition to the parser messages in the document
    QList<QmlJS::DiagnosticMessage> semanticMessages;

private:
    QSharedPointer<const QmlJS::Interpreter::Context> m_context;

    friend class Internal::SemanticHighlighter;
};

class QMLJSEDITOR_EXPORT QmlJSTextEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    QmlJSTextEditor(QWidget *parent = 0);
    ~QmlJSTextEditor();

    virtual void unCommentSelection();

    SemanticInfo semanticInfo() const;
    int editorRevision() const;
    bool isOutdated() const;

    Internal::QmlOutlineModel *outlineModel() const;
    QModelIndex outlineModelIndex();

    bool updateSelectedElements() const;
    void setUpdateSelectedElements(bool value);

    void renameId(const QString &oldId, const QString &newId);

    static QVector<QString> highlighterFormatCategories();

public slots:
    void followSymbolUnderCursor();
    void findUsages();
    void showContextPane();
    virtual void setFontSettings(const TextEditor::FontSettings &);

signals:
    void outlineModelIndexChanged(const QModelIndex &index);
    void selectedElementsChanged(QList<int> offsets, const QString &wordAtCursor);

private slots:
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void modificationChanged(bool);

    void updateDocument();
    void updateDocumentNow();
    void jumpToOutlineElement(int index);
    void updateOutlineNow();
    void updateOutlineIndexNow();
    void updateCursorPositionNow();
    void showTextMarker();
    void updateFileName();

    void updateUses();
    void updateUsesNow();

    // refactoring ops
    void renameIdUnderCursor();

    void semanticRehighlight();
    void forceSemanticRehighlight();
    void updateSemanticInfo(const QmlJSEditor::SemanticInfo &semanticInfo);
    void onCursorPositionChanged();
    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);

    void performQuickFix(int index);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    bool event(QEvent *e);
    void wheelEvent(QWheelEvent *event);
    void resizeEvent(QResizeEvent *event);
    void scrollContentsBy(int dx, int dy);
    TextEditor::BaseTextEditorEditable *createEditableInterface();
    void createToolBar(QmlJSEditorEditable *editable);
    TextEditor::BaseTextEditor::Link findLinkAt(const QTextCursor &cursor, bool resolveTarget = true);

private:
    bool isClosingBrace(const QList<QmlJS::Token> &tokens) const;

    void setSelectedElements();
    QString wordUnderCursor() const;

    Internal::SemanticHighlighterSource currentSource(bool force = false);
    QModelIndex indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex = QModelIndex()) const;
    bool hideContextPane();

    const Core::Context m_context;

    QTimer *m_updateDocumentTimer;
    QTimer *m_updateUsesTimer;
    QTimer *m_semanticRehighlightTimer;
    QTimer *m_updateOutlineTimer;
    QTimer *m_updateOutlineIndexTimer;
    QTimer *m_cursorPositionTimer;
    QComboBox *m_outlineCombo;
    Internal::QmlOutlineModel *m_outlineModel;
    QModelIndex m_outlineModelIndex;
    QmlJS::ModelManagerInterface *m_modelManager;
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrencesUnusedFormat;
    QTextCharFormat m_occurrenceRenameFormat;

    Internal::SemanticHighlighter *m_semanticHighlighter;
    SemanticInfo m_semanticInfo;

    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;

    QmlJS::IContextPane *m_contextPane;
    int m_oldCursorPosition;
    bool m_updateSelectedElements;

    FindReferences *m_findReferences;
};

} // namespace QmlJSEditor

#endif // QMLJSEDITOR_H
