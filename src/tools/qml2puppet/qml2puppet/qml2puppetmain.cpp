// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpuppet.h"

#ifdef ENABLE_INTERNAL_QML_RUNTIME
#include "runner/qmlruntime.h"
#endif

auto getQmlRunner(int &argc, char **argv)
{
#ifdef ENABLE_INTERNAL_QML_RUNTIME
    QString qmlRuntime("--qml-runtime");
    for (int i = 0; i < argc; i++) {
        if (!qmlRuntime.compare(QString::fromLocal8Bit(argv[i]))) {
            qInfo() << "Starting QML Runtime";
            return std::unique_ptr<QmlBase>(new QmlRuntime(argc, argv));
        }
    }
#endif
    qInfo() << "Starting QML Puppet";
    return std::unique_ptr<QmlBase>(new QmlPuppet(argc, argv));
}

int main(int argc, char *argv[])
{
    QDSMeta::Logging::registerMessageHandler();
    QDSMeta::AppInfo::registerAppInfo("Qml2Puppet");
    return getQmlRunner(argc, argv)->run();
}
