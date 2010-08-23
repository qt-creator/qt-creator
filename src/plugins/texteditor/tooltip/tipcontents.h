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

#ifndef TIPCONTENTS_H
#define TIPCONTENTS_H

#include "texteditor/texteditor_global.h"

#include <QtCore/QString>
#include <QtGui/QColor>

namespace TextEditor {

class TEXTEDITOR_EXPORT TipContent
{
protected:
    TipContent();

public:
    virtual ~TipContent();

    virtual TipContent *clone() const = 0;
    virtual int typeId() const = 0;
    virtual bool isValid() const = 0;
    virtual int showTime() const = 0;
    virtual bool equals(const TipContent &tipContent) const = 0;
};

class TEXTEDITOR_EXPORT ColorContent : public TipContent
{
public:
    ColorContent(const QColor &color);
    virtual ~ColorContent();

    virtual TipContent *clone() const;
    virtual int typeId() const;
    virtual bool isValid() const;
    virtual int showTime() const;
    virtual bool equals(const TipContent &tipContent) const;

    const QColor &color() const;

    static const int COLOR_CONTENT_ID = 0;

private:
    QColor m_color;
};

class TEXTEDITOR_EXPORT TextContent : public TipContent
{
public:
    TextContent(const QString &text);
    virtual ~TextContent();

    virtual TipContent *clone() const;
    virtual int typeId() const;
    virtual bool isValid() const;
    virtual int showTime() const;
    virtual bool equals(const TipContent &tipContent) const;

    const QString &text() const;

    static const int TEXT_CONTENT_ID = 1;

private:
    QString m_text;
};

} // namespace TextEditor

#endif // TIPCONTENTS_H
