// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QtQml/qqmldebug.h>

#include <stdio.h>

// What the document itself cannot read: the environment and the working
// directory it was started with, on the standard output rather than through
// the logging categories.
class Reporter : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void report()
    {
        const QString marker = qEnvironmentVariable("QTC_BACKEND_ENV_MARKER");
        if (!marker.isEmpty())
            printf("env=%s\n", qPrintable(marker));
        printf("cwd=%s\n", qPrintable(QDir::currentPath()));
        printf("after bump\n");
        fflush(stdout);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Reporter reporter;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("reporter", &reporter);
    engine.load(QUrl("qrc:/qmlserver_inferior.qml"));
    return app.exec();
}

#include "qmlserver_inferior.moc"
