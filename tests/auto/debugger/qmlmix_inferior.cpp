// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmix_inferior.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

// Out of line, and calling a second function of its own: the native mixed
// tests step through both from QML.
int QmlEntryPoint::process(int value)
{
    int doubled = value * 2;
    return doubled + offset(value);
}

int QmlEntryPoint::offset(int value) const
{
    return value % 3;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("MixTest", "Main");
    return app.exec();
}
