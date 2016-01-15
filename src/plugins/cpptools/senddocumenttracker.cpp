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

#include "senddocumenttracker.h"

#include <algorithm>

namespace CppTools {

void SendDocumentTracker::setLastSentRevision(int revision)
{
    m_lastSentRevision = revision;
    m_contentChangeStartPosition = std::numeric_limits<int>::max();
}

int SendDocumentTracker::lastSentRevision() const
{
    return m_lastSentRevision;
}

void SendDocumentTracker::setLastCompletionPosition(int lastCompletionPosition)
{
    m_lastCompletionPosition = lastCompletionPosition;
}

int SendDocumentTracker::lastCompletionPosition() const
{
    return m_lastCompletionPosition;
}

void SendDocumentTracker::applyContentChange(int startPosition)
{
    if (startPosition < m_lastCompletionPosition)
        m_lastCompletionPosition = -1;

    m_contentChangeStartPosition = std::min(startPosition, m_contentChangeStartPosition);
}

bool SendDocumentTracker::shouldSendCompletion(int newCompletionPosition) const
{
    return m_lastCompletionPosition != newCompletionPosition;
}

bool SendDocumentTracker::shouldSendRevision(uint newRevision) const
{
    return m_lastSentRevision != int(newRevision);
}

bool SendDocumentTracker::shouldSendRevisionWithCompletionPosition(int newRevision, int newCompletionPosition) const
{
    if (shouldSendRevision(newRevision))
        return changedBeforeCompletionPosition(newCompletionPosition);

    return false;
}

bool SendDocumentTracker::changedBeforeCompletionPosition(int newCompletionPosition) const
{
    return m_contentChangeStartPosition < newCompletionPosition;
}

} // namespace CppTools
