/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qtsystemexceptionhandler.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>

#if defined(Q_OS_LINUX)
#  include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN)
#  include "client/windows/handler/exception_handler.h"
#elif defined(Q_OS_MACOS)
#  include "client/mac/handler/exception_handler.h"
#endif

#if defined(Q_OS_LINUX)
static bool exceptionHandlerCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                                     void* /*context*/,
                                     bool succeeded)
{
    if (!succeeded)
        return succeeded;

    const QStringList argumentList = {
        QString::fromLocal8Bit(descriptor.path()),
        QString::number(QtSystemExceptionHandler::startTime().toTime_t()),
        QCoreApplication::applicationName(),
        QCoreApplication::applicationVersion(),
        QtSystemExceptionHandler::plugins(),
        QtSystemExceptionHandler::buildVersion(),
        QCoreApplication::applicationFilePath()
    };

    return !QProcess::execute(QtSystemExceptionHandler::crashHandlerPath(), argumentList);
}
#elif defined(Q_OS_MACOS)
static bool exceptionHandlerCallback(const char *dump_dir,
                                     const char *minidump_id,
                                     void *context,
                                     bool succeeded)
{
    Q_UNUSED(context);

    if (!succeeded)
        return succeeded;

    const QString path = QString::fromLocal8Bit(dump_dir) + '/'
            + QString::fromLocal8Bit(minidump_id) + ".dmp";
    const QStringList argumentList = {
        path,
        QString::number(QtSystemExceptionHandler::startTime().toTime_t()),
        QCoreApplication::applicationName(),
        QCoreApplication::applicationVersion(),
        QtSystemExceptionHandler::plugins(),
        QtSystemExceptionHandler::buildVersion(),
        QCoreApplication::applicationFilePath()
    };

    return !QProcess::execute(QtSystemExceptionHandler::crashHandlerPath(), argumentList);
}
#elif defined(Q_OS_WIN)
static bool exceptionHandlerCallback(const wchar_t* dump_path,
                                     const wchar_t* minidump_id,
                                     void* context,
                                     EXCEPTION_POINTERS* exinfo,
                                     MDRawAssertionInfo* assertion,
                                     bool succeeded)
{
    Q_UNUSED(assertion);
    Q_UNUSED(exinfo);
    Q_UNUSED(context);

    if (!succeeded)
        return succeeded;

    const QString path = QString::fromWCharArray(dump_path, int(wcslen(dump_path))) + '/'
            + QString::fromWCharArray(minidump_id, int(wcslen(minidump_id))) + ".dmp";
    const QStringList argumentList = {
        path,
        QString::number(QtSystemExceptionHandler::startTime().toTime_t()),
        QCoreApplication::applicationName(),
        QCoreApplication::applicationVersion(),
        QtSystemExceptionHandler::plugins(),
        QtSystemExceptionHandler::buildVersion(),
        QCoreApplication::applicationFilePath()
    };

    return !QProcess::execute(QtSystemExceptionHandler::crashHandlerPath(), argumentList);
}
#endif

static QDateTime s_startTime;
static QString s_plugins;
static QString s_buildVersion;
static QString s_crashHandlerPath;

#if defined(Q_OS_LINUX)
QtSystemExceptionHandler::QtSystemExceptionHandler(const QString &libexecPath)
    : exceptionHandler(new google_breakpad::ExceptionHandler(
                           google_breakpad::MinidumpDescriptor(QDir::tempPath().toStdString()),
                           NULL,
                           exceptionHandlerCallback,
                           NULL,
                           true,
                           -1))
{
    init(libexecPath);
}
#elif defined(Q_OS_MACOS)
QtSystemExceptionHandler::QtSystemExceptionHandler(const QString &libexecPath)
    : exceptionHandler(new google_breakpad::ExceptionHandler(
                           QDir::tempPath().toStdString(),
                           NULL,
                           exceptionHandlerCallback,
                           NULL,
                           true,
                           NULL))
{
    init(libexecPath);
}
#elif defined(Q_OS_WIN)
QtSystemExceptionHandler::QtSystemExceptionHandler(const QString &libexecPath)
    : exceptionHandler(new google_breakpad::ExceptionHandler(
                           QDir::tempPath().toStdWString(),
                           NULL,
                           exceptionHandlerCallback,
                           NULL,
                           google_breakpad::ExceptionHandler::HANDLER_ALL))
{
    init(libexecPath);
}
#else
QtSystemExceptionHandler::QtSystemExceptionHandler(const QString & /*libexecPath*/)
    : exceptionHandler(0)
{

}
#endif

void QtSystemExceptionHandler::init(const QString &libexecPath)
{
    s_startTime = QDateTime::currentDateTime();
    s_crashHandlerPath = libexecPath + Utils::HostOsInfo::withExecutableSuffix("/qtcrashhandler");
}

QtSystemExceptionHandler::~QtSystemExceptionHandler()
{
#ifdef ENABLE_QT_BREAKPAD
    delete exceptionHandler;
#endif
}

void QtSystemExceptionHandler::setPlugins(const QStringList &pluginNameList)
{
    s_plugins = QString("{%1}").arg(pluginNameList.join(","));
}

void QtSystemExceptionHandler::setBuildVersion(const QString &version)
{
    s_buildVersion = version;
}

QString QtSystemExceptionHandler::buildVersion()
{
    return s_buildVersion;
}

QString QtSystemExceptionHandler::plugins()
{
    return s_plugins;
}

void QtSystemExceptionHandler::setCrashHandlerPath(const QString &crashHandlerPath)
{
    s_crashHandlerPath = crashHandlerPath;
}

QString QtSystemExceptionHandler::crashHandlerPath()
{
    return s_crashHandlerPath;
}

void QtSystemExceptionHandler::crash()
{
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

QDateTime QtSystemExceptionHandler::startTime()
{
    return s_startTime;
}
