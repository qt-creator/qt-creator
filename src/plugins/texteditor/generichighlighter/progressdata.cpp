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

#include "progressdata.h"
#include "rule.h"

using namespace TextEditor;
using namespace Internal;

ProgressData::ProgressData() :
    m_offset(0),
    m_savedOffset(-1),
    m_onlySpacesSoFar(true),
    m_openingBraceMatchAtFirstNonSpace(false),
    m_closingBraceMatchAtNonEnd(false),
    m_willContinueLine(false)
{}

ProgressData::~ProgressData()
{
    foreach (Rule *rule, m_trackedRules)
        rule->progressFinished();
}

void ProgressData::setOffset(const int offset)
{ m_offset = offset; }

int ProgressData::offset() const
{ return m_offset; }

void ProgressData::incrementOffset()
{ ++m_offset; }

void ProgressData::incrementOffset(const int increment)
{ m_offset += increment; }

void ProgressData::saveOffset()
{ m_savedOffset = m_offset; }

void ProgressData::restoreOffset()
{
    Q_ASSERT(m_savedOffset != -1);
    m_offset = m_savedOffset;
    m_savedOffset = -1;
}

void ProgressData::setOnlySpacesSoFar(const bool onlySpaces)
{ m_onlySpacesSoFar = onlySpaces; }

bool ProgressData::isOnlySpacesSoFar() const
{ return m_onlySpacesSoFar; }

void ProgressData::setOpeningBraceMatchAtFirstNonSpace(const bool match)
{ m_openingBraceMatchAtFirstNonSpace = match; }

bool ProgressData::isOpeningBraceMatchAtFirstNonSpace() const
{ return m_openingBraceMatchAtFirstNonSpace; }

void ProgressData::setClosingBraceMatchAtNonEnd(const bool match)
{ m_closingBraceMatchAtNonEnd = match; }

bool ProgressData::isClosingBraceMatchAtNonEnd() const
{ return m_closingBraceMatchAtNonEnd; }

void ProgressData::clearBracesMatches()
{
    if (m_openingBraceMatchAtFirstNonSpace)
        m_openingBraceMatchAtFirstNonSpace = false;
    if (m_closingBraceMatchAtNonEnd)
        m_closingBraceMatchAtNonEnd = false;
}

void ProgressData::setWillContinueLine(const bool willContinue)
{ m_willContinueLine = willContinue; }

bool ProgressData::isWillContinueLine() const
{ return m_willContinueLine; }

void ProgressData::setCaptures(const QStringList &captures)
{ m_captures = captures; }

const QStringList &ProgressData::captures() const
{ return m_captures; }

void ProgressData::trackRule(Rule *rule)
{
    m_trackedRules.append(rule);
}
