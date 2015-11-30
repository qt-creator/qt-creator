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

#ifndef QMLJSSCANNER_H
#define QMLJSSCANNER_H

#include <qmljs/qmljs_global.h>

#include <QStringList>

namespace QmlJS {

class QMLJS_EXPORT Token
{
public:
    enum Kind {
        EndOfFile,
        Keyword,
        Identifier,
        String,
        Comment,
        Number,
        LeftParenthesis,
        RightParenthesis,
        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        Semicolon,
        Colon,
        Comma,
        Dot,
        Delimiter,
        RegExp
    };

    inline Token(): offset(0), length(0), kind(EndOfFile) {}
    inline Token(int o, int l, Kind k): offset(o), length(l), kind(k) {}
    inline int begin() const { return offset; }
    inline int end() const { return offset + length; }
    inline bool is(int k) const { return k == kind; }
    inline bool isNot(int k) const { return k != kind; }

public:
    int offset;
    int length;
    Kind kind;
};

class QMLJS_EXPORT Scanner
{
public:
    enum {
        Normal = 0,
        MultiLineComment = 1,
        MultiLineStringDQuote = 2,
        MultiLineStringSQuote = 3,
        MultiLineMask = 3,

        RegexpMayFollow = 4 // flag that may be combined with the above
    };

    Scanner();
    virtual ~Scanner();

    bool scanComments() const;
    void setScanComments(bool scanComments);

    QList<Token> operator()(const QString &text, int startState = Normal);
    int state() const;

    bool isKeyword(const QString &text) const;
    static QStringList keywords();

private:
    int _state;
    bool _scanComments: 1;
};

} // namespace QmlJS

#endif // QMLJSSCANNER_H
