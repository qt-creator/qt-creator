// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpuppet.h"

#ifdef ENABLE_INTERNAL_QML_RUNTIME
#include "runner/qmlruntime.h"
#endif

QmlBase *getQmlRunner(int &argc, char **argv)
{
#ifdef ENABLE_INTERNAL_QML_RUNTIME
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
