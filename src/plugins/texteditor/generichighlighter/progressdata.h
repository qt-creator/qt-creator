/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROGRESSDATA_H
#define PROGRESSDATA_H

#include <QtCore/QStringList>

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

#endif // PROGRESSDATA_H
