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

#include "tipcontents.h"

using namespace TextEditor;
using namespace Internal;

TipContent::TipContent()
{}

TipContent::~TipContent()
{}

QColorContent::QColorContent(const QColor &color) : m_color(color)
{}

QColorContent::~QColorContent()
{}

bool TipContent::equals(const QColorContent *colorContent) const
{
    Q_UNUSED(colorContent)
    return false;
}

bool QColorContent::isValid() const
{
    return m_color.isValid();
}

int QColorContent::showTime() const
{
    return 4000;
}

bool QColorContent::equals(const TipContent *tipContent) const
{
    return tipContent->equals(this);
}

bool QColorContent::equals(const QColorContent *colorContent) const
{
    return m_color == colorContent->color();
}

const QColor &QColorContent::color() const
{
    return m_color;
}
