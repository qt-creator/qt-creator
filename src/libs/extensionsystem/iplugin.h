// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QObject>

#include <functional>

namespace ExtensionSystem {

namespace Internal { class IPluginPrivate; }

using TestCreator = std::function<QObject *()>;

class EXTENSIONSYSTEM_EXPORT IPlugin : public QObject
{
    Q_OBJECT

public:
    enum ShutdownFlag {
        SynchronousShutdown,
        AsynchronousShutdown
    };

    IPlugin();
    ~IPlugin() override;

    virtual bool initialize(const QStringList &arguments, QString *errorString);
    virtual void extensionsInitialized() {}
    virtual bool delayedInitialize() { return false; }
    virtual ShutdownFlag aboutToShutdown() { return SynchronousShutdown; }
    virtual QObject *remoteCommand(const QStringList & /* options */,
                                   const QString & /* workingDirectory */,
                                   const QStringList & /* arguments */) { return nullptr; }


    // Deprecated in 10.0, use addTest()
    virtual QVector<QObject *> createTestObjects() const;

protected:
    virtual void initialize() {}

    template <typename Test, typename ...Args>
    void addTest(Args && ...args) { addTestCreator([args...] { return new Test(args...); }); }
    void addTestCreator(const TestCreator &creator);

signals:
    void asynchronousShutdownFinished();

private:
    Internal::IPluginPrivate *d;
};

} // namespace ExtensionSystem
