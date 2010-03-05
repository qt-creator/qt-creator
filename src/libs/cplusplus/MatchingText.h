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
#ifndef MATCHINGTEXT_H
#define MATCHINGTEXT_H

#include <CPlusPlusForwardDeclarations.h>
#include <QtGui/QTextCursor>

namespace CPlusPlus {

class BackwardsScanner;

class CPLUSPLUS_EXPORT MatchingText
{
public:
    MatchingText();

    static bool shouldInsertMatchingText(const QTextCursor &tc);
    static bool shouldInsertMatchingText(const QChar &lookAhead);

    QString insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                const QChar &la, int *skippedChars) const;
    QString insertParagraphSeparator(const QTextCursor &tc) const;

private:
    bool shouldInsertNewline(const QTextCursor &tc) const;
};

} // end of namespace CPlusPlus

#endif // MATCHINGTEXT_H
