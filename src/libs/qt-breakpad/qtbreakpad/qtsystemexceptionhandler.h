// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDateTime>

namespace google_breakpad {
    class ExceptionHandler;
}

class QtSystemExceptionHandler
{
public:
    QtSystemExceptionHandler(const QString &libexecPath);
    ~QtSystemExceptionHandler();

    static void crash();
    static void setPlugins(const QStringList &pluginNameList);
    static void setBuildVersion(const QString &version);
    static void setCrashHandlerPath(const QString &crashHandlerPath);

    static QString plugins();
    static QString buildVersion();
    static QString crashHandlerPath();

    static QDateTime startTime();

protected:
    void init(const QString &libexecPath);

private:
    google_breakpad::ExceptionHandler *exceptionHandler = nullptr;
};
