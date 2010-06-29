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
#ifndef CPLUSPLUS_SIMPLELEXER_H
#define CPLUSPLUS_SIMPLELEXER_H

#include <CPlusPlusForwardDeclarations.h>

#include <QString>
#include <QList>

namespace CPlusPlus {

class SimpleLexer;
class Token;

class CPLUSPLUS_EXPORT SimpleLexer
{
public:
    SimpleLexer();
    ~SimpleLexer();

    bool skipComments() const;
    void setSkipComments(bool skipComments);

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool enabled);

    bool objCEnabled() const;
    void setObjCEnabled(bool onoff);

    bool endedJoined() const;

    QList<Token> operator()(const QString &text, int state = 0);

    int state() const
    { return _lastState; }

    static int tokenAt(const QList<Token> &tokens, unsigned offset);
    static Token tokenAt(const QString &text,
                         unsigned offset,
                         int state,
                         bool qtMocRunEnabled = false);

    static int tokenBefore(const QList<Token> &tokens, unsigned offset);

private:
    int _lastState;
    bool _skipComments: 1;
    bool _qtMocRunEnabled: 1;
    bool _objCEnabled: 1;
    bool _endedJoined: 1;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_SIMPLELEXER_H
