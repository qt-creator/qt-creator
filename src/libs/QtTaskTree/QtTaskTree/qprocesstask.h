// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROCESSTASK_H
#define QPROCESSTASK_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(process)

class QProcessDeleter final
{
public:
    // Blocking, should be called after all QProcessAdapter instances are deleted.
    Q_TASKTREE_EXPORT static void syncAll();
    Q_TASKTREE_EXPORT void operator()(QProcess *process);
};

class QProcessTaskAdapter final
{
public:
    Q_TASKTREE_EXPORT void operator()(QProcess *task, QTaskInterface *iface);
};

using QProcessTask = QCustomTask<QProcess, QProcessTaskAdapter, QProcessDeleter>;

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#endif // QPROCESSTASK_H
