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

    /* Removes 'length' characters to the right of the cursor. */
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
