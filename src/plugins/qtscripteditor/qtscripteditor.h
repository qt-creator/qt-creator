/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef QTSCRIPTDITORW_H
#define QTSCRIPTDITORW_H

#include <texteditor/basetexteditor.h>

namespace Core {
    class ICore;
}

namespace QtScriptEditor {
namespace Internal {

class  QtScriptHighlighter;

class ScriptEditor;

class ScriptEditorEditable : public TextEditor::BaseTextEditorEditable
{
public:
    ScriptEditorEditable(ScriptEditor *, const QList<int>&);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    const char *kind() const;
    QToolBar *toolBar() { return 0; }

private:
    QList<int> m_context;
};


class ScriptEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef QList<int> Context;

    ScriptEditor(const Context &context,
                   Core::ICore *core,
                   TextEditor::TextEditorActionHandler *ah,
                   QWidget *parent = 0);
    ~ScriptEditor();

    ScriptEditor *duplicate(QWidget *parent);

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    TextEditor::BaseTextEditorEditable *createEditableInterface() { return new ScriptEditorEditable(this, m_context); }

private:
    virtual bool isElectricCharacter(const QChar &ch) const;
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

    const Context m_context;
    Core::ICore *m_core;
    TextEditor::TextEditorActionHandler *m_ah;
};

} // namespace Internal
} // namespace QtScriptEditor

#endif // QTSCRIPTDITORW_H
