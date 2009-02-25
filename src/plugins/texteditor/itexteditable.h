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

#ifndef ITEXTEDITABLE_H
#define ITEXTEDITABLE_H

#include "texteditor_global.h"
#include "itexteditor.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT ITextEditable : public ITextEditor
{
    Q_OBJECT
public:
    ITextEditable() {}
    virtual ~ITextEditable() {}

    /* Removes 'length' characteres to the right of the cursor. */
    virtual void remove(int length) = 0;

    /* Inserts the given string to the right of the cursor. */
    virtual void insert(const QString &string) = 0;

    /* Replaces 'length' characters to the right of the cursor with the given string. */
    virtual void replace(int length, const QString &string) = 0;

    virtual void setCurPos(int pos) = 0;

    virtual void select(int toPos) = 0;
};

} // namespace TextEditor

#endif // ITEXTEDITABLE_H
