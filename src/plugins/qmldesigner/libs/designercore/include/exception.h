// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QString description() const;
    void showException(const QString &title = QString()) const;
    static void setShowExceptionCallback(
        std::function<void(QStringView title, QStringView description)> callback);

    int line() const;
    QString function() const;
    QString file() const;
    QString backTrace() const;

    void createWarning() const;

    static void setShouldAssert(bool assert);
    static bool shouldAssert();
    static void setWarnAboutException(bool warn);
    static bool warnAboutException();

protected:
    Exception(int line,
              const QByteArray &function,
              const QByteArray &file,
              const QString &description);
    static QString defaultDescription(int line, const QByteArray &function, const QByteArray &file);
    QString defaultDescription();

private:
    const int m_line;
    const QString m_function;
    const QString m_file;
    const QString m_description;
    const QString m_backTrace;
    static bool s_shouldAssert;
    static bool s_warnAboutException;
};

QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const Exception &exception);

}
