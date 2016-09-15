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

#include "googletest.h"

#include <cpptools/senddocumenttracker.h>

using CppTools::SendDocumentTracker;
using testing::PrintToString;

namespace {

MATCHER_P2(HasValues, lastSentRevision, lastCompletionPosition,
           std::string(negation ? "isn't" : "is")
           + " DocumentRevisionManagement with last sent revision "+ PrintToString(lastSentRevision)
           + " , last completion position " + PrintToString(lastCompletionPosition))
{
    return arg.lastSentRevision() == lastSentRevision
        && arg.lastCompletionPosition() == lastCompletionPosition;
}

class SendDocumentTracker : public testing::Test
{
protected:
    ::SendDocumentTracker tracker;
};

TEST_F(SendDocumentTracker, DefaultLastSentRevision)
{
    ASSERT_THAT(tracker, HasValues(-1, -1));
}

TEST_F(SendDocumentTracker, SetRevision)
{
    tracker.setLastSentRevision(46);

    ASSERT_THAT(tracker, HasValues(46, -1));
}

TEST_F(SendDocumentTracker, SetLastCompletionPosition)
{
    tracker.setLastCompletionPosition(33);

    ASSERT_THAT(tracker, HasValues(-1, 33));
}

TEST_F(SendDocumentTracker, ApplyContentChange)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    tracker.applyContentChange(10);

    ASSERT_THAT(tracker, HasValues(46, -1));
}

TEST_F(SendDocumentTracker, DontSendCompletionIfPositionIsEqual)
{
    tracker.setLastCompletionPosition(33);

    ASSERT_FALSE(tracker.shouldSendCompletion(33));
}

TEST_F(SendDocumentTracker, SendCompletionIfPositionIsDifferent)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    ASSERT_TRUE(tracker.shouldSendCompletion(22));
}

TEST_F(SendDocumentTracker, SendCompletionIfChangeIsBeforeCompletionPositionAndPositionIsEqual)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    tracker.applyContentChange(10);

    ASSERT_TRUE(tracker.shouldSendCompletion(33));
}

TEST_F(SendDocumentTracker, DontSendCompletionIfChangeIsAfterCompletionPositionAndPositionIsEqual)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    tracker.applyContentChange(40);

    ASSERT_FALSE(tracker.shouldSendCompletion(33));
}

TEST_F(SendDocumentTracker, DontSendRevisionIfRevisionIsEqual)
{
    tracker.setLastSentRevision(46);

    ASSERT_FALSE(tracker.shouldSendRevision(46));
}

TEST_F(SendDocumentTracker, SendRevisionIfRevisionIsDifferent)
{
    tracker.setLastSentRevision(46);

    ASSERT_TRUE(tracker.shouldSendRevision(21));
}

TEST_F(SendDocumentTracker, DontSendRevisionWithDefaults)
{
    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(21, 33));
}

TEST_F(SendDocumentTracker, DontSendIfRevisionIsDifferentAndCompletionPositionIsEqualAndNoContentChange)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(21, 33));
}

TEST_F(SendDocumentTracker, DontSendIfRevisionIsDifferentAndCompletionPositionIsDifferentAndNoContentChange)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(21, 44));
}

TEST_F(SendDocumentTracker, DontSendIfRevisionIsEqualAndCompletionPositionIsDifferentAndNoContentChange)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(46,44));
}

TEST_F(SendDocumentTracker, SendIfChangeIsBeforeCompletionAndPositionIsEqualAndRevisionIsDifferent)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(33);

    tracker.applyContentChange(10);

    ASSERT_TRUE(tracker.shouldSendRevisionWithCompletionPosition(45, 33));
}

TEST_F(SendDocumentTracker, DontSendIfChangeIsAfterCompletionPositionAndRevisionIsDifferent)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);

    tracker.applyContentChange(40);

    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

TEST_F(SendDocumentTracker, SendIfChangeIsBeforeCompletionPositionAndRevisionIsDifferent)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);

    tracker.applyContentChange(30);

    ASSERT_TRUE(tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

TEST_F(SendDocumentTracker, ResetChangedContentStartPositionIfLastRevisionIsSet)
{
    tracker.setLastSentRevision(46);
    tracker.setLastCompletionPosition(50);
    tracker.applyContentChange(30);

    tracker.setLastSentRevision(47);

    ASSERT_FALSE(tracker.shouldSendRevisionWithCompletionPosition(45, 36));
}

}
