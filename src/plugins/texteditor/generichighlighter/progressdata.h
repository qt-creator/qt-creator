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

#pragma once

#include <QStringList>

namespace TextEditor {
namespace Internal {

class Rule;

class ProgressData
{
public:
    ProgressData();
    ~ProgressData();

    void setOffset(const int offset);
    int offset() const;

    void incrementOffset();
    void incrementOffset(const int increment);

    void saveOffset();
    void restoreOffset();

    void setOnlySpacesSoFar(const bool onlySpaces);
    bool isOnlySpacesSoFar() const;

    void setOpeningBraceMatchAtFirstNonSpace(const bool match);
    bool isOpeningBraceMatchAtFirstNonSpace() const;

    void setClosingBraceMatchAtNonEnd(const bool match);
    bool isClosingBraceMatchAtNonEnd() const;

    void clearBracesMatches();

    void setWillContinueLine(const bool willContinue);
    bool isWillContinueLine() const;

    void setCaptures(const QStringList &captures);
    const QStringList &captures() const;

    void trackRule(Rule *rule);

private:
    int m_offset;
    int m_savedOffset;
    bool m_onlySpacesSoFar;
    bool m_openingBraceMatchAtFirstNonSpace;
    bool m_closingBraceMatchAtNonEnd;
    bool m_willContinueLine;
    QStringList m_captures;
    QList<Rule *> m_trackedRules;
};

} // namespace Internal
} // namespace TextEditor
