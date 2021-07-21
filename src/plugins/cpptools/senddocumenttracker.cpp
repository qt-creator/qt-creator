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

#ifdef WITH_TESTS
#include "cpptoolsplugin.h"
#include <QtTest>
#endif

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

#ifdef WITH_TESTS
namespace  Internal {

void CppToolsPlugin::test_documentTracker_defaultLastSentRevision()
{
    SendDocumentTracker tracker;

    QCOMPARE(tracker.lastSentRevision(), -1);
    QCOMPARE(tracker.lastCompletionPosition(), -1);
}

void CppToolsPlugin::test_documentTracker_setRevision()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);

    QCOMPARE(tracker.lastSentRevision(), 46);
    QCOMPARE(tracker.lastCompletionPosition(), -1);
}

void CppToolsPlugin::test_documentTracker_setLastCompletionPosition()
{
    SendDocumentTracker tracker;
    tracker.setLastCompletionPosition(33);

    QCOMPARE(tracker.lastSentRevision(), -1);
    QCOMPARE(tracker.lastCompletionPosition(), 33);
}

void CppToolsPlugin::test_documentTracker_applyContentChange()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);
    tracker.applyContentChange(10);

    QCOMPARE(tracker.lastSentRevision(), 46);
    QCOMPARE(tracker.lastCompletionPosition(), -1);
}

void CppToolsPlugin::test_documentTracker_dontSendCompletionIfPositionIsEqual()
{
    SendDocumentTracker tracker;
    tracker.setLastCompletionPosition(33);

    QVERIFY(!tracker.shouldSendCompletion(33));
}

void CppToolsPlugin::test_documentTracker_sendCompletionIfPositionIsDifferent()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    QVERIFY(tracker.shouldSendCompletion(22));
}

void CppToolsPlugin::test_documentTracker_sendCompletionIfChangeIsBeforeCompletionPositionAndPositionIsEqual()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);
    tracker.applyContentChange(10);

    QVERIFY(tracker.shouldSendCompletion(33));
}

void CppToolsPlugin::test_documentTracker_dontSendCompletionIfChangeIsAfterCompletionPositionAndPositionIsEqual()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);
    tracker.applyContentChange(40);

    QVERIFY(!tracker.shouldSendCompletion(33));
}

void CppToolsPlugin::test_documentTracker_dontSendRevisionIfRevisionIsEqual()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);

    QVERIFY(!tracker.shouldSendRevision(46));
}

void CppToolsPlugin::test_documentTracker_sendRevisionIfRevisionIsDifferent()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);

    QVERIFY(tracker.shouldSendRevision(21));
}

void CppToolsPlugin::test_documentTracker_dontSendRevisionWithDefaults()
{
    SendDocumentTracker tracker;
    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(21, 33));
}

void CppToolsPlugin::test_documentTracker_dontSendIfRevisionIsDifferentAndCompletionPositionIsEqualAndNoContentChange()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(21, 33));
}

void CppToolsPlugin::test_documentTracker_dontSendIfRevisionIsDifferentAndCompletionPositionIsDifferentAndNoContentChange()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(21, 44));
}

void CppToolsPlugin::test_documentTracker_dontSendIfRevisionIsEqualAndCompletionPositionIsDifferentAndNoContentChange()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(46,44));
}

void CppToolsPlugin::test_documentTracker_sendIfChangeIsBeforeCompletionAndPositionIsEqualAndRevisionIsDifferent()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);
    tracker.applyContentChange(10);

    QVERIFY(tracker.shouldSendRevisionWithCompletionPosition(45, 33));
}

void CppToolsPlugin::test_documentTracker_dontSendIfChangeIsAfterCompletionPositionAndRevisionIsDifferent()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);
    tracker.applyContentChange(40);

    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

void CppToolsPlugin::test_documentTracker_sendIfChangeIsBeforeCompletionPositionAndRevisionIsDifferent()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);
    tracker.applyContentChange(30);

    QVERIFY(tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

void CppToolsPlugin::test_documentTracker_resetChangedContentStartPositionIfLastRevisionIsSet()
{
    SendDocumentTracker tracker;
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);
    tracker.applyContentChange(30);
    tracker.setLastSentRevision(47);

    QVERIFY(!tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

} // namespace Internal
#endif

} // namespace CppTools
