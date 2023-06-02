// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "async.h"

namespace Utils {

static int s_maxThreadCount = INT_MAX;

class AsyncThreadPool : public QThreadPool
{
public:
    AsyncThreadPool(QThread::Priority priority) {
        setThreadPriority(priority);
        setMaxThreadCount(s_maxThreadCount);
        moveToThread(qApp->thread());
    }
};

#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_idle,         (QThread::IdlePriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_lowest,       (QThread::LowestPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_low,          (QThread::LowPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_normal,       (QThread::NormalPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_high,         (QThread::HighPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_highest,      (QThread::HighestPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_timeCritical, (QThread::TimeCriticalPriority));
Q_GLOBAL_STATIC_WITH_ARGS(AsyncThreadPool, s_inherit,      (QThread::InheritPriority));
#else
Q_GLOBAL_STATIC(AsyncThreadPool, s_idle,         QThread::IdlePriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_lowest,       QThread::LowestPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_low,          QThread::LowPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_normal,       QThread::NormalPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_high,         QThread::HighPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_highest,      QThread::HighestPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_timeCritical, QThread::TimeCriticalPriority);
Q_GLOBAL_STATIC(AsyncThreadPool, s_inherit,      QThread::InheritPriority);
#endif

QThreadPool *asyncThreadPool(QThread::Priority priority)
{
    switch (priority) {
    case QThread::IdlePriority         : return s_idle;
    case QThread::LowestPriority       : return s_lowest;
    case QThread::LowPriority          : return s_low;
    case QThread::NormalPriority       : return s_normal;
    case QThread::HighPriority         : return s_high;
    case QThread::HighestPriority      : return s_highest;
    case QThread::TimeCriticalPriority : return s_timeCritical;
    case QThread::InheritPriority      : return s_inherit;
    }
    return nullptr;
}

} // namespace Utils
