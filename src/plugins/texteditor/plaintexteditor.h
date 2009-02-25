/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PLAINTEXTEDITOR_H
#define PLAINTEXTEDITOR_H

#include "basetexteditor.h"

#include <QtCore/QList>

namespace TextEditor {

class PlainTextEditor;

class TEXTEDITOR_EXPORT PlainTextEditorEditable : public BaseTextEditorEditable
{
public:
    PlainTextEditorEditable(PlainTextEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    const char *kind() const;
private:
    QList<int> m_context;
};

class TEXTEDITOR_EXPORT PlainTextEditor : public BaseTextEditor
{
    Q_OBJECT

public:
    PlainTextEditor(QWidget *parent);

protected:
    BaseTextEditorEditable *createEditableInterface() { return new PlainTextEditorEditable(this); }
    // Indent a text block based on previous line.
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
