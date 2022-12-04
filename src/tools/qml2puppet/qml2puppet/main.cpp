// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "runner/puppet/qmlpuppet.h"
#include "runner/runtime/qmlruntime.h"

QmlBase *getQmlRunner(int &argc, char **argv)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--qml-runtime")){
            qInfo() << "Starting QML Runtime";
            return new QmlRuntime(argc, argv);
        }
    }
#endif
    qInfo() << "Starting QML Puppet";
    return new QmlPuppet(argc, argv);
}

int main(int argc, char *argv[])
{
    QDSMeta::Logging::registerMessageHandler();
    QDSMeta::AppInfo::registerAppInfo("Qml2Puppet");

    QmlBase *qmlRunner = getQmlRunner(argc, argv);
    return qmlRunner->run();
}
