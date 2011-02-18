/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DUMMY_H
#define DUMMY_H

#include <QString>

namespace test {

class Dummy
{
public:
    Dummy();
    Dummy(int a);

    typedef int INT;

    enum Values {
        v1,
        v2,
        v3
    };

    static const int ONE = 1;
    static const double PI;

    static void sfunc();

    struct Internal
    {
        QString one;
        typedef double DOUBLE;
    };

    void bla(int);
    void bla(const QString &);
    void bla(const QString &) const;
    void bla(int, const QString &) const;

    void foo(int) const {}
    void foo(const QString &) const {}

    QString one;
};

class ChildDummy : public Dummy {};

class GrandChildDummy : public Dummy {};

} // namespace test

#endif // DUMMY_H
