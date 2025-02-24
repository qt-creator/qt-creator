// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpuppet.h"
#include "qmlrenderer.h"

#ifdef ENABLE_INTERNAL_QML_RUNTIME
#include "runner/qmlruntime.h"
#endif

#include <QLoggingCategory>

inline Q_LOGGING_CATEGORY(deprecated, "qt.tools.qds.deprecated");
inline Q_LOGGING_CATEGORY(verbose1, "qt.tools.qds.verbose1");
inline Q_LOGGING_CATEGORY(verbose2, "qt.tools.qds.verbose2");

void registerMessageHandler(
    QtMsgType type, [[maybe_unused]] const QMessageLogContext &context, const QString &msg)
{
    auto tPrinter = [&](const QString &msgPrefix) {
        fprintf(
            stderr, "%s: %s\n", msgPrefix.toLocal8Bit().constData(), msg.toLocal8Bit().constData());
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
}

auto getQmlRunner(int &argc, char **argv)
{
    const QString qmlRuntime("--qml-runtime");
    const QString qmlRenderer("--qml-renderer");
    for (int i = 0; i < argc; i++) {
        const QString currentArg = QString::fromLocal8Bit(argv[i]);
        if (!qmlRuntime.compare(currentArg)) {
#ifdef ENABLE_INTERNAL_QML_RUNTIME
            qInfo() << "Starting QML Runtime";
            return std::unique_ptr<QmlBase>(new QmlRuntime(argc, argv));
#else
            qInfo() << "QML Runtime not supported";
#endif
        } else if (!qmlRenderer.compare(currentArg)) {
            qInfo() << "Starting QML Renderer";
            return std::unique_ptr<QmlBase>(new QmlRenderer(argc, argv));

        }
    }
    qInfo() << "Starting QML Puppet";
    return std::unique_ptr<QmlBase>(new QmlPuppet(argc, argv));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(registerMessageHandler);
    return getQmlRunner(argc, argv)->run();
}
