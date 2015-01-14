/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "circularclipboard.h"

using namespace TextEditor::Internal;

static const int kMaxSize = 10;

CircularClipboard::CircularClipboard()
    : m_current(-1)
{}

CircularClipboard::~CircularClipboard()
{
}

CircularClipboard *CircularClipboard::instance()
{
    static CircularClipboard clipboard;
    return &clipboard;
}

void CircularClipboard::collect(const QMimeData *mimeData)
{
    collect(QSharedPointer<const QMimeData>(mimeData));
}

void CircularClipboard::collect(const QSharedPointer<const QMimeData> &mimeData)
{
    //Avoid duplicates
    const QString text = mimeData->text();
    for (QList< QSharedPointer<const QMimeData> >::iterator i = m_items.begin(); i != m_items.end(); ++i) {
        if (mimeData == *i || text == (*i)->text()) {
            m_items.erase(i);
            break;
        }
    }
    if (m_items.size() >= kMaxSize)
        m_items.removeLast();
    m_items.prepend(mimeData);
}

QSharedPointer<const QMimeData> CircularClipboard::next() const
{
    if (m_items.isEmpty())
        return QSharedPointer<const QMimeData>();

    if (m_current == m_items.length() - 1)
        m_current = 0;
    else
        ++m_current;

    return m_items.at(m_current);
}

void CircularClipboard::toLastCollect()
{
    m_current = -1;
}

int CircularClipboard::size() const
{
    return m_items.size();
}
