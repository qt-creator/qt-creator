/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#ifndef CPLUSPLUS_MATCHINGTEXT_H
#define CPLUSPLUS_MATCHINGTEXT_H

#include <CPlusPlusForwardDeclarations.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)
QT_FORWARD_DECLARE_CLASS(QChar)

namespace CPlusPlus {

class BackwardsScanner;
class TokenCache;

class CPLUSPLUS_EXPORT MatchingText
{
public:
    static bool shouldInsertMatchingText(const QTextCursor &tc);
    static bool shouldInsertMatchingText(QChar lookAhead);

    QString insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                QChar la, int *skippedChars) const;
    QString insertParagraphSeparator(const QTextCursor &tc) const;

private:
    bool shouldInsertNewline(const QTextCursor &tc) const;
};

} // namespace CPlusPlus

#endif // MATCHINGTEXT_H
