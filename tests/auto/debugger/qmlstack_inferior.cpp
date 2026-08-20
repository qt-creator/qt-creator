// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QtQml/qqmldebug.h>

class QmlEntryPoint : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    Q_INVOKABLE int process(int value);
};

// Out of line and with a local of its own: the tests step into this from QML
// and read the local in the C++ frame.
int QmlEntryPoint::process(int value)
{
    int doubled = value * 2;
    return doubled + 1;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QmlEntryPoint backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/qmlstack_inferior.qml"));
    return app.exec();
}

#include "qmlstack_inferior.moc"
