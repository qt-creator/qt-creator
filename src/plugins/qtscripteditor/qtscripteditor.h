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

#ifndef QTSCRIPTDITORW_H
#define QTSCRIPTDITORW_H

#include <texteditor/basetexteditor.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}

namespace QtScriptEditor {
namespace Internal {

class QtScriptHighlighter;

class ScriptEditor;

class ScriptEditorEditable : public TextEditor::BaseTextEditorEditable
{
public:
    ScriptEditorEditable(ScriptEditor *, const QList<int> &);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;

    bool isTemporary() const { return false; }

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

class ScriptEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef QList<int> Context;

    ScriptEditor(const Context &context,
                 QWidget *parent = 0);
    ~ScriptEditor();

    QList<Declaration> declarations() const;
    QStringList words() const;

    void unCommentSelection(); // from basetexteditor

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void updateDocument();
    void updateDocumentNow();
    void jumpToMethod(int index);
    void updateMethodBoxIndex();
    void updateMethodBoxToolTip();
    void updateFileName();

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    TextEditor::BaseTextEditorEditable *createEditableInterface();
    void createToolBar(ScriptEditorEditable *editable);

private:
    virtual bool isElectricCharacter(const QChar &ch) const;
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

    const Context m_context;

    QTimer *m_updateDocumentTimer;
    QComboBox *m_methodCombo;
    QList<Declaration> m_declarations;
    QStringList m_words;
};

} // namespace Internal
} // namespace QtScriptEditor

#endif // QTSCRIPTDITORW_H
