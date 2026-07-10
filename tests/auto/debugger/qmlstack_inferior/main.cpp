// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlentrypoint.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QtQml/qqmldebug.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QmlEntryPoint backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/main.qml"));
    return app.exec();
}
