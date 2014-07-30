/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef PLAINTEXTEDITOR_H
#define PLAINTEXTEDITOR_H

#include "basetexteditor.h"

namespace TextEditor {

class PlainTextEditorWidget;

class TEXTEDITOR_EXPORT PlainTextEditor : public BaseTextEditor
{
    Q_OBJECT
public:
    PlainTextEditor(PlainTextEditorWidget *);

    Core::IEditor *duplicate();
};

class TEXTEDITOR_EXPORT PlainTextDocument : public BaseTextDocument
{
    Q_OBJECT
public:
    PlainTextDocument();
private slots:
    void updateTabSettings();
};

class TEXTEDITOR_EXPORT PlainTextEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT

public:
    PlainTextEditorWidget(PlainTextDocument *doc, QWidget *parent = 0);
    PlainTextEditorWidget(PlainTextEditorWidget *other);

protected:
    virtual BaseTextEditor *createEditor() { return new PlainTextEditor(this); }

private:
    PlainTextEditorWidget(TextEditor::BaseTextEditorWidget *); // avoid stupidity
    PlainTextEditorWidget(BaseTextDocument *, QWidget *); //avoid stupidity
    void ctor();
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
