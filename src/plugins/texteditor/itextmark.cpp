/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "itextmark.h"

using namespace TextEditor;

ITextMark::~ITextMark()
{
    if (m_markableInterface)
        m_markableInterface->removeMark(this);
    m_markableInterface = 0;
}

int ITextMark::lineNumber() const
{
    return m_lineNumber;
}

void ITextMark::paint(QPainter *painter, const QRect &rect) const
{
    m_icon.paint(painter, rect, Qt::AlignCenter);
}

void ITextMark::updateLineNumber(int lineNumber)
{
    m_lineNumber = lineNumber;
}

void ITextMark::updateBlock(const QTextBlock &)
{}

void ITextMark::removedFromEditor()
{}

void ITextMark::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void ITextMark::updateMarker()
{
    if (m_markableInterface)
        m_markableInterface->updateMark(this);
}

void ITextMark::setPriority(Priority priority)
{
    m_priority = priority;
}

ITextMark::Priority ITextMark::priority() const
{
    return m_priority;
}

bool ITextMark::isVisible() const
{
    return m_visible;
}

void ITextMark::setVisible(bool visible)
{
    m_visible = visible;
    if (m_markableInterface)
        m_markableInterface->updateMark(this);
}

double ITextMark::widthFactor() const
{
    return m_widthFactor;
}

void ITextMark::setWidthFactor(double factor)
{
    m_widthFactor = factor;
}

bool ITextMark::isClickable() const
{
    return false;
}

void ITextMark::clicked()
{}

bool ITextMark::isDraggable() const
{
    return false;
}

void ITextMark::dragToLine(int lineNumber)
{
    Q_UNUSED(lineNumber);
}

ITextMarkable *ITextMark::markableInterface() const
{
    return m_markableInterface;
}

void ITextMark::setMarkableInterface(ITextMarkable *markableInterface)
{
    m_markableInterface = markableInterface;
}

