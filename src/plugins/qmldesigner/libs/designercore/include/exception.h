// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <sqliteexception.h>

#include <QString>
#include <QDebug>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT Exception : public Sqlite::Exception
{
public:
    virtual QString type() const = 0;
    virtual QString description() const = 0;

    void showException(const QString &title = QString()) const;
    static void setShowExceptionCallback(
        std::function<void(QStringView title, QStringView description)> callback);

    int line() const;
    QString file() const;

    static void setWarnAboutException(bool warn);
    static bool warnAboutException();

    friend QDebug operator<<(QDebug debug, const Exception &exception);

protected:
    Exception(const Sqlite::source_location &location = Sqlite::source_location::current())
        : Sqlite::Exception{location}
    {}

    static QString defaultDescription(const Sqlite::source_location &sourceLocation);
    void createWarning() const;

private:
    static bool s_warnAboutException;
};
}
