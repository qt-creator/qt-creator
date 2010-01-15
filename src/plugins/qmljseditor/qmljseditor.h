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

#ifndef QMLJSEDITOR_H
#define QMLJSEDITOR_H

#include <qmljs/qmldocument.h>
#include <qscripthighlighter/qscriptincrementalscanner.h>
#include <texteditor/basetexteditor.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace Qml {
    class QmlTypeSystem;
}

namespace QmlJSEditor {

class QmlModelManagerInterface;

namespace Internal {

class QmlHighlighter;
class QmlJSTextEditor;

class QmlJSEditorEditable : public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT

public:
    QmlJSEditorEditable(QmlJSTextEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;
    bool isTemporary() const { return false; }
    virtual bool open(const QString & fileName);

private:
    QList<int> m_context;
};


struct Declaration
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

class QmlJSTextEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef QList<int> Context;

    QmlJSTextEditor(QWidget *parent = 0);
    ~QmlJSTextEditor();

    QList<Declaration> declarations() const;
    QStringList keywords() const;

    QList<QmlJS::DiagnosticMessage> diagnosticMessages() const
    { return m_diagnosticMessages; }

    virtual void unCommentSelection();

    Qml::QmlDocument::Ptr qmlDocument() const { return m_document; }

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void onDocumentUpdated(Qml::QmlDocument::Ptr doc);

    void updateDocument();
    void updateDocumentNow();
    void jumpToMethod(int index);
    void updateMethodBoxIndex();
    void updateMethodBoxToolTip();
    void updateFileName();

    void updateUses();
    void updateUsesNow();

    // refactoring ops
    void renameIdUnderCursor();

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    TextEditor::BaseTextEditorEditable *createEditableInterface();
    void createToolBar(QmlJSEditorEditable *editable);
    TextEditor::BaseTextEditor::Link findLinkAt(const QTextCursor &cursor, bool resolveTarget = true);

    //// brace matching
    virtual bool contextAllowsAutoParentheses(const QTextCursor &cursor, const QString &textToInsert = QString()) const;
    virtual bool isInComment(const QTextCursor &cursor) const;
    virtual QString insertMatchingBrace(const QTextCursor &tc, const QString &text, const QChar &la, int *skippedChars) const;
    virtual QString insertParagraphSeparator(const QTextCursor &tc) const;

private:
    virtual bool isElectricCharacter(const QChar &ch) const;
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);
    bool isClosingBrace(const QList<SharedTools::QScriptIncrementalScanner::Token> &tokens) const;

    QString wordUnderCursor() const;

    const Context m_context;

    QTimer *m_updateDocumentTimer;
    QTimer *m_updateUsesTimer;
    QComboBox *m_methodCombo;
    QList<Declaration> m_declarations;
    QMap<QString, QList<QmlJS::AST::SourceLocation> > m_ids; // ### use QMultiMap
    int m_idsRevision;
    QList<QmlJS::DiagnosticMessage> m_diagnosticMessages;
    Qml::QmlDocument::Ptr m_document;
    QmlModelManagerInterface *m_modelManager;
    Qml::QmlTypeSystem *m_typeSystem;
    QTextCharFormat m_occurrencesFormat;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITOR_H
