// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "circularclipboard.h"

#include <utils/algorithm.h>

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
    collect(std::shared_ptr<const QMimeData>(mimeData));
}

void CircularClipboard::collect(const std::shared_ptr<const QMimeData> &mimeData)
{
    //Avoid duplicates
    const QString text = mimeData->text();
    Utils::eraseOne(m_items, [&](const std::shared_ptr<const QMimeData> &it) {
        return mimeData == it || text == it->text();
    });
    if (m_items.size() >= kMaxSize)
        m_items.removeLast();
    m_items.prepend(mimeData);
}

std::shared_ptr<const QMimeData> CircularClipboard::next() const
{
    if (m_items.isEmpty())
        return {};

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
