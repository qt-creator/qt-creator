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

#ifndef MYINTERFACES_H
#define MYINTERFACES_H

#include <aggregate.h>

#include <QString>

class IComboEntry : public QObject
{
    Q_OBJECT

public:
    IComboEntry(QString title) : m_title(title) {}
    virtual ~IComboEntry() {}
    QString title() const { return m_title; }

private:
    QString m_title;
};

class IText1 : public QObject
{
    Q_OBJECT

public:
    IText1(QString text) : m_text(text) {}
    virtual ~IText1() {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

class IText2 : public QObject
{
    Q_OBJECT

public:
    IText2(QString text) : m_text(text) {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

class IText3 : public QObject
{
    Q_OBJECT

public:
    IText3(QString text) : m_text(text) {}
    virtual ~IText3() {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

#endif // MYINTERFACES_H
