// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <QObject>

#include <limits>

namespace CppEditor {

class CPPEDITOR_EXPORT SendDocumentTracker
{
public:
    void setLastSentRevision(int lastSentRevision);
    int lastSentRevision() const;

    void setLastCompletionPosition(int lastCompletionPosition);
    int lastCompletionPosition() const;

    void applyContentChange(int startPosition);

    bool shouldSendCompletion(int newCompletionPosition) const;
    bool shouldSendRevision(uint newRevision) const;
    bool shouldSendRevisionWithCompletionPosition(int newRevision, int newCompletionPosition) const;

private:
    bool changedBeforeCompletionPosition(int newCompletionPosition) const;

private:
    int m_lastSentRevision = -1;
    int m_lastCompletionPosition = -1;
    int m_contentChangeStartPosition = std::numeric_limits<int>::max();
};

#ifdef WITH_TESTS
namespace Internal {
class DocumentTrackerTest : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultLastSentRevision();
    void testSetRevision();
    void testSetLastCompletionPosition();
    void testApplyContentChange();
    void testDontSendCompletionIfPositionIsEqual();
    void testSendCompletionIfPositionIsDifferent();
    void testSendCompletionIfChangeIsBeforeCompletionPositionAndPositionIsEqual();
    void testDontSendCompletionIfChangeIsAfterCompletionPositionAndPositionIsEqual();
    void testDontSendRevisionIfRevisionIsEqual();
    void testSendRevisionIfRevisionIsDifferent();
    void testDontSendRevisionWithDefaults();
    void testDontSendIfRevisionIsDifferentAndCompletionPositionIsEqualAndNoContentChange();
    void testDontSendIfRevisionIsDifferentAndCompletionPositionIsDifferentAndNoContentChange();
    void testDontSendIfRevisionIsEqualAndCompletionPositionIsDifferentAndNoContentChange();
    void testSendIfChangeIsBeforeCompletionAndPositionIsEqualAndRevisionIsDifferent();
    void testDontSendIfChangeIsAfterCompletionPositionAndRevisionIsDifferent();
    void testSendIfChangeIsBeforeCompletionPositionAndRevisionIsDifferent();
    void testResetChangedContentStartPositionIfLastRevisionIsSet();
};
} // namespace Internal
#endif // WITH_TESTS

} // namespace CppEditor
