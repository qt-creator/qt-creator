// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlconfiguration.h"

#include "QtQml/qqmlcomponent.h"
#include <QCoreApplication>
#include <QQmlApplicationEngine>

//// Listens to the appEngine signals to determine if all files failed to load
class LoadWatcher : public QObject
{
    //    Q_OBJECT
public:
    LoadWatcher(QQmlApplicationEngine *e, int expected, Config *conf)
        : QObject(e)
        , qae(e)
        , conf(conf)
        , expectedFileCount(expected)
    {
        connect(e, &QQmlApplicationEngine::objectCreated, this, &LoadWatcher::checkFinished);
        // QQmlApplicationEngine also connects quit() to QCoreApplication::quit
        // and exit() to QCoreApplication::exit but if called before exec()
        // then QCoreApplication::quit or QCoreApplication::exit does nothing
        connect(e, &QQmlEngine::quit, this, &LoadWatcher::quit);
        connect(e, &QQmlEngine::exit, this, &LoadWatcher::exit);
    }

    int returnCode = 0;
    bool earlyExit = false;

public Q_SLOTS:
    void checkFinished(QObject *o, const QUrl &url)
    {
        Q_UNUSED(url);
        if (o) {
            checkForWindow(o);
            if (conf && qae) {
                for (PartialScene *ps : std::as_const(conf->completers)) {
                    if (o->inherits(ps->itemType().toUtf8().constData()))
                        contain(o, ps->container());
                }
            }
        }
        if (haveWindow)
            return;

        if (!--expectedFileCount) {
            printf("qml: Did not load any objects, exiting.\n");
            exit(2);
            QCoreApplication::exit(2);
        }
    }

    void quit()
    {
        // Will be checked before calling exec()
        earlyExit = true;
        returnCode = 0;
    }
    void exit(int retCode)
    {
        earlyExit = true;
        returnCode = retCode;
    }

private:
    void contain(QObject *o, const QUrl &containPath)
    {
        QQmlComponent c(qae, containPath);
        QObject *o2 = c.create();
        if (!o2)
            return;
        o2->setParent(this);
        checkForWindow(o2);
        bool success = false;
        int idx;
        if ((idx = o2->metaObject()->indexOfProperty("containedObject")) != -1)
            success = o2->metaObject()->property(idx).write(o2, QVariant::fromValue<QObject *>(o));
        if (!success)
            o->setParent(o2); // Set QObject parent, and assume container will react as needed
    }
    void checkForWindow(QObject *o)
    {
        if (o->isWindowType() && o->inherits("QQuickWindow"))
            haveWindow = true;
    }

private:
    QQmlApplicationEngine *qae;
    Config *conf;

    bool haveWindow = false;
    int expectedFileCount;
};
