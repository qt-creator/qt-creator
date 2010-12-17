/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "linecolumnlabel.h"

namespace Utils {

LineColumnLabel::LineColumnLabel(QWidget *parent)
  : QLabel(parent), m_unused(0)
{
}

LineColumnLabel::~LineColumnLabel()
{
}

void LineColumnLabel::setText(const QString &text, const QString &maxText)
{
    QLabel::setText(text);
    m_maxText = maxText;
}
QSize LineColumnLabel::sizeHint() const
{
    return fontMetrics().boundingRect(m_maxText).size();
}

QString LineColumnLabel::maxText() const
{
    return m_maxText;
}

void LineColumnLabel::setMaxText(const QString &maxText)
{
     m_maxText = maxText;
}

} // namespace Utils
