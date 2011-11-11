/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IWELCOMEPAGE_H
#define IWELCOMEPAGE_H

#include "utils_global.h"

#include <QtCore/QObject>
#include <QtCore/QUrl>

QT_FORWARD_DECLARE_CLASS(QDeclarativeEngine)

namespace Utils {

class IWelcomePagePrivate;

class QTCREATOR_UTILS_EXPORT IWelcomePage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QUrl pageLocation READ pageLocation CONSTANT)
    Q_PROPERTY(int priority READ priority CONSTANT)
    Q_PROPERTY(bool hasSearchBar READ hasSearchBar CONSTANT)

public:
    IWelcomePage();
    virtual ~IWelcomePage();

    virtual QUrl pageLocation() const = 0;
    virtual QString title() const = 0;
    virtual int priority() const { return 0; }
    virtual void facilitateQml(QDeclarativeEngine *) {}
    virtual bool hasSearchBar() const { return false; }

private:
    // not used atm
    IWelcomePagePrivate *d;
};

}

#endif // IWELCOMEPAGE_H
