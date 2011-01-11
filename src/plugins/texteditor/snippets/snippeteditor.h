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

#ifndef SNIPPETEDITOR_H
#define SNIPPETEDITOR_H

#include <texteditor/texteditor_global.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QFocusEvent)

namespace TextEditor {

class SnippetEditor;
class SyntaxHighlighter;
class Indenter;

// Should not be necessary in this case, but the base text editor assumes a
// valid editable interface.
class TEXTEDITOR_EXPORT SnippetEditorEditable : public BaseTextEditorEditable
{
    Q_OBJECT
public:
    SnippetEditorEditable(SnippetEditor *editor);
    virtual ~SnippetEditorEditable();

    Core::Context context() const { return m_context; }

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /* parent */ ) { return 0; }
    bool isTemporary() const { return false; }
    virtual QString id() const;

private:
    const Core::Context m_context;
};

class TEXTEDITOR_EXPORT SnippetEditor : public BaseTextEditor
{
    Q_OBJECT
public:
    SnippetEditor(QWidget *parent);
    virtual ~SnippetEditor();

    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);

signals:
    void snippetContentChanged();

protected:
    virtual void focusOutEvent(QFocusEvent *event);

    virtual int extraAreaWidth(int * /* markWidthPtr */ = 0) const { return 0; }
    virtual BaseTextEditorEditable *createEditableInterface();
};

} // TextEditor

#endif // SNIPPETEDITOR_H
