// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindstackbrowser.h"

namespace Valgrind::Callgrind {

StackBrowser::StackBrowser(QObject *parent)
    : QObject(parent)
{
}

void StackBrowser::clear()
{
    m_stack.clear();
    m_redoStack.clear();
    emit currentChanged();
}

void StackBrowser::select(const Function *item)
{
    if (!m_stack.isEmpty() && m_stack.top() == item)
        return;

    m_stack.push(item);
    m_redoStack.clear();
    emit currentChanged();
}

const Function *StackBrowser::current() const
{
    return m_stack.isEmpty() ? 0 : m_stack.top();
}

void StackBrowser::goBack()
{
    if (m_stack.isEmpty())
        return;

    m_redoStack.push(m_stack.pop());
    emit currentChanged();
}

void StackBrowser::goNext()
{
    if (m_redoStack.isEmpty())
        return;

    m_stack.push(m_redoStack.pop());
    emit currentChanged();
}

} // namespace Valgrind::Callgrind
