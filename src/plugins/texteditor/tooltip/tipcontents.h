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

#include <QtGui/QColor>

namespace TextEditor {
namespace Internal {

class QColorContent;

class TipContent
{
public:
    TipContent();
    virtual ~TipContent();

    virtual bool isValid() const = 0;
    virtual int showTime() const = 0;
    virtual bool equals(const TipContent *tipContent) const = 0;
    virtual bool equals(const QColorContent *colorContent) const;
};

class QColorContent : public TipContent
{
public:
    QColorContent(const QColor &color);
    virtual ~QColorContent();

    virtual bool isValid() const;
    virtual int showTime() const;
    virtual bool equals(const TipContent *tipContent) const;
    virtual bool equals(const QColorContent *colorContent) const;

    const QColor &color() const;

private:
    QColor m_color;
};

} // namespace Internal
} // namespace TextEditor

#endif // TIPCONTENTS_H
