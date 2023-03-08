// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QCommandLineParser>
#include <QLoggingCategory>

// Common functions can be used in all QDS apps
namespace QDSMeta {

namespace Logging {
inline Q_LOGGING_CATEGORY(deprecated, "qt.tools.qds.deprecated");
inline Q_LOGGING_CATEGORY(verbose1, "qt.tools.qds.verbose1");
inline Q_LOGGING_CATEGORY(verbose2, "qt.tools.qds.verbose2");

inline void registerMessageHandler()
{
    qInstallMessageHandler(
        [](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
            auto tPrinter = [&](const QString &msgPrefix) {
                fprintf(stderr,
                        "%s: %s (%s:%u, %s)\n",
                        msgPrefix.toLocal8Bit().constData(),
                        msg.toLocal8Bit().constData(),
                        context.file,
                        context.line,
                        context.function);
            };

            if (type == QtDebugMsg)
                tPrinter("Debug");
            else if (type == QtInfoMsg)
                tPrinter("Info");
            else if (type == QtWarningMsg)
                tPrinter("Warning");
            else if (type == QtCriticalMsg)
                tPrinter("Critical");
            else if (type == QtFatalMsg) {
                tPrinter("Fatal");
                abort();
            }
        });
}
} // namespace Logging

namespace AppInfo {

#define STRINGIFY_INTERNAL(x) #x
#define QDS_STRINGIFY(x) STRINGIFY_INTERNAL(x)

void printAppInfo();
void registerAppInfo(const QString &appName);

} // namespace AppInfo
} // namespace QDSMeta
