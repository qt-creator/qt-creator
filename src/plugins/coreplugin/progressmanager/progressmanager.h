// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QFuture>
#include <QFutureInterfaceBase>
#include <QObject>

namespace Core {
class FutureProgress;

namespace Internal { class ProgressManagerPrivate; }

class CORE_EXPORT ProgressManager : public QObject
{
    Q_OBJECT
public:
    enum ProgressFlag {
        KeepOnFinish = 0x01,
        ShowInApplicationIcon = 0x02
    };
    Q_DECLARE_FLAGS(ProgressFlags, ProgressFlag)

    static ProgressManager *instance();

    template <typename T>
    static FutureProgress *addTask(const QFuture<T> &future, const QString &title,
                                   Utils::Id type, ProgressFlags flags = {}) {
        return addTask(QFuture<void>(future), title, type, flags);
    }

    static FutureProgress *addTask(const QFuture<void> &future, const QString &title,
                                   Utils::Id type, ProgressFlags flags = {});
    static FutureProgress *addTimedTask(const QFutureInterface<void> &fi, const QString &title,
                                        Utils::Id type, int expectedSeconds, ProgressFlags flags = {});
    static FutureProgress *addTimedTask(const QFuture<void> &future, const QString &title,
                                        Utils::Id type, int expectedSeconds, ProgressFlags flags = {});
    static void setApplicationLabel(const QString &text);

public slots:
    static void cancelTasks(Utils::Id type);

signals:
    void taskStarted(Utils::Id type);
    void allTasksFinished(Utils::Id type);

private:
    ProgressManager();
    ~ProgressManager() override;

    friend class Core::Internal::ProgressManagerPrivate;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::ProgressManager::ProgressFlags)
