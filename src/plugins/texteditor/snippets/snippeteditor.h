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

#ifndef SNIPPETEDITOR_H
#define SNIPPETEDITOR_H

#include <texteditor/texteditor_global.h>
#include <texteditor/basetexteditor.h>

namespace TextEditor {

class SnippetEditorWidget;
class SyntaxHighlighter;
class Indenter;

// Should not be necessary in this case, but the base text editor assumes a
// valid editable interface.
class TEXTEDITOR_EXPORT SnippetEditor : public BaseTextEditor
{
    Q_OBJECT

public:
    SnippetEditor(SnippetEditorWidget *editorWidget);

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /* parent */ ) { return 0; }
    bool isTemporary() const { return false; }
    Core::Id id() const;
};

class TEXTEDITOR_EXPORT SnippetEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT

public:
    SnippetEditorWidget(QWidget *parent);

    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);

signals:
    void snippetContentChanged();

protected:
    virtual void focusOutEvent(QFocusEvent *event);

    virtual int extraAreaWidth(int * /* markWidthPtr */ = 0) const { return 0; }
    virtual BaseTextEditor *createEditor();
};

} // TextEditor

#endif // SNIPPETEDITOR_H
