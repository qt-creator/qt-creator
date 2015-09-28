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
