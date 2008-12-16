/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef SIMPLELEXER_H
#define SIMPLELEXER_H

#include <CPlusPlusForwardDeclarations.h>

#include <QString>
#include <QList>

namespace CPlusPlus {

class SimpleLexer;

class CPLUSPLUS_EXPORT SimpleToken
{
public:
    SimpleToken()
        : _kind(0),
          _position(0),
          _length(0)
    { }

    inline int kind() const
    { return _kind; }

    inline int position() const
    { return _position; }

    inline int length() const
    { return _length; }

    inline QStringRef text() const
    { return _text; }

    inline bool is(int k) const    { return _kind == k; }
    inline bool isNot(int k) const { return _kind != k; }

    bool isLiteral() const;
    bool isOperator() const;
    bool isKeyword() const;

public:
    int _kind;
    int _position;
    int _length;
    QStringRef _text;

    friend class SimpleLexer;
};

class CPLUSPLUS_EXPORT SimpleLexer
{
public:
    SimpleLexer();
    ~SimpleLexer();

    bool skipComments() const;
    void setSkipComments(bool skipComments);

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool enabled);

    QList<SimpleToken> operator()(const QString &text, int state = 0);

    int state() const
    { return _lastState; }

private:
    int _lastState;
    bool _skipComments: 1;
    bool _qtMocRunEnabled: 1;
};

} // end of namespace CPlusPlus

#endif // SIMPLELEXER_H
