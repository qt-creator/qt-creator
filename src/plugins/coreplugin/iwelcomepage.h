/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef IWELCOMEPAGE_H
#define IWELCOMEPAGE_H

#include "core_global.h"

#include "id.h"

#include <QObject>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)

namespace Core {

class CORE_EXPORT IWelcomePage : public QObject
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
    virtual void facilitateQml(QQmlEngine *) {}
    virtual bool hasSearchBar() const { return false; }
    virtual Core::Id id() const = 0;
};

} // Core

#endif // IWELCOMEPAGE_H
