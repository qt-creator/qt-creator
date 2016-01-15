/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "callgrindstackbrowser.h"

namespace Valgrind {
namespace Callgrind {

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

} // namespace Callgrind
} // namespace Valgrind
