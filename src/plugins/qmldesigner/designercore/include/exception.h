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

#pragma once

#include <qmldesignercorelib_global.h>

#include <QString>
#include <QDebug>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT Exception
{
public:
    Exception(int line,
              const QByteArray &function,
              const QByteArray &file);
    virtual ~Exception();

    virtual QString type() const = 0;
    virtual QString description() const;
    virtual void showException(const QString &title = QString()) const;

    int line() const;
    QString function() const;
    QString file() const;
    QString backTrace() const;

    void createWarning() const;

    static void setShouldAssert(bool assert);
    static bool shouldAssert();
    static bool warnAboutException();

private:
    int m_line;
    QString m_function;
    QString m_file;
    QString m_backTrace;
    static bool s_shouldAssert;
};

QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const Exception &exception);

}
