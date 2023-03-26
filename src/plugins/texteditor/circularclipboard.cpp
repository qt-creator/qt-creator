// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "circularclipboard.h"

using namespace TextEditor::Internal;

static const int kMaxSize = 10;

CircularClipboard::CircularClipboard() = default;

CircularClipboard::~CircularClipboard() = default;

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
