/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
