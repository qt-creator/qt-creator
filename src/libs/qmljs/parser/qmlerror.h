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

#ifndef QQMLERROR_H
#define QQMLERROR_H



#include <QtCore/qurl.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDebug;
class QmlErrorPrivate;
class QmlError
{
public:
    QmlError();
    QmlError(const QmlError &);
    QmlError &operator=(const QmlError &);
    ~QmlError();

    bool isValid() const;

    QUrl url() const;
    void setUrl(const QUrl &);
    QString description() const;
    void setDescription(const QString &);
    int line() const;
    void setLine(int);
    int column() const;
    void setColumn(int);

    QString toString() const;
private:
    QmlErrorPrivate *d;
};

QDebug operator<<(QDebug debug, const QmlError &error);

Q_DECLARE_TYPEINFO(QmlError, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQMLERROR_H
