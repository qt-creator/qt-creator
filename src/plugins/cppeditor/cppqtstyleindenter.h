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

#ifndef CPPQTSTYLEINDENTER_H
#define CPPQTSTYLEINDENTER_H

#include <texteditor/indenter.h>

namespace CppEditor {
namespace Internal {

class CppQtStyleIndenter : public TextEditor::Indenter
{
public:
    CppQtStyleIndenter();
    virtual ~CppQtStyleIndenter();

    virtual bool isElectricCharacter(const QChar &ch) const;
    virtual void indentBlock(QTextDocument *doc,
                             const QTextBlock &block,
                             const QChar &typedChar,
                             TextEditor::BaseTextEditor *editor);

    virtual void indent(QTextDocument *doc,
                        const QTextCursor &cursor,
                        const QChar &typedChar,
                        TextEditor::BaseTextEditor *editor);
};

} // Internal
} // CppEditor

#endif // CPPQTSTYLEINDENTER_H
