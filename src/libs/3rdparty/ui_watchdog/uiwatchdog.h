/*
  This file is part of KDToolBox.

  SPDX-FileCopyrightText: 2018 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: MIT
*/

#ifndef UIWATCHDOG_H
#define UIWATCHDOG_H

#include <QDebug>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QMutex>
#include <QThread>
#include <QTimer>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#define MAX_TIME_BLOCKED 300 // ms

Q_DECLARE_LOGGING_CATEGORY(uidelays)
Q_LOGGING_CATEGORY(uidelays, "uidelays")

class UiWatchdog;
class UiWatchdogWorker : public QObject
{
public:
    enum Option
    {
        OptionNone = 0,
        OptionDebugBreak = 1
    };
    typedef int Options;

private:
    UiWatchdogWorker(Options options)
        : QObject()
        , m_watchTimer(new QTimer(this))
        , m_options(options)
    {
        qCDebug(uidelays) << "UiWatchdogWorker created";
        connect(m_watchTimer, &QTimer::timeout, this, &UiWatchdogWorker::checkUI);
    }

    ~UiWatchdogWorker()
    {
        qCDebug(uidelays) << "UiWatchdogWorker destroyed";
        stop();
    }

    void start(int frequency_msecs = 200)
    {
        m_watchTimer->start(frequency_msecs);
        m_elapsedTimeSinceLastBeat.start();
    }

    void stop() { m_watchTimer->stop(); }

    void checkUI()
    {
        qint64 elapsed;

        {
            QMutexLocker l(&m_mutex);
            elapsed = m_elapsedTimeSinceLastBeat.elapsed();
        }

        if (elapsed > MAX_TIME_BLOCKED)
        {
            qDebug() << "UI is blocked !" << elapsed; // Add custom action here!
            if ((m_options & OptionDebugBreak))
                debugBreak();
        }
    }

    void debugBreak()
    {
#ifdef Q_OS_WIN
        DebugBreak();
#endif
    }

    void reset()
    {
        QMutexLocker l(&m_mutex);
        m_elapsedTimeSinceLastBeat.restart();
    }

    QTimer *const m_watchTimer;
    QElapsedTimer m_elapsedTimeSinceLastBeat;
    QMutex m_mutex;
    const Options m_options;
    friend class UiWatchdog;
};

class UiWatchdog : public QObject
{
public:
    explicit UiWatchdog(UiWatchdogWorker::Options options = UiWatchdogWorker::OptionNone, QObject *parent = nullptr)
        : QObject(parent)
        , m_uiTimer(new QTimer(this))
        , m_options(options)
    {
        QLoggingCategory::setFilterRules(QStringLiteral("uidelays.debug=false"));
        qCDebug(uidelays) << "UiWatchdog created";
        connect(m_uiTimer, &QTimer::timeout, this, &UiWatchdog::onUiBeat);
    }

    ~UiWatchdog()
    {
        stop();
        qCDebug(uidelays) << "UiWatchdog destroyed";
    }

    void start(int frequency_msecs = 100)
    {
        if (m_worker)
            return;

        m_uiTimer->start(frequency_msecs);

        m_worker = new UiWatchdogWorker(m_options);
        m_watchDogThread = new QThread(this);
        m_worker->moveToThread(m_watchDogThread);
        m_watchDogThread->start();
        connect(m_watchDogThread, &QThread::started, m_worker,
                [this, frequency_msecs] { m_worker->start(frequency_msecs); });
    }

    void stop()
    {
        if (!m_worker)
            return;

        m_uiTimer->stop();
        m_worker->deleteLater();
        m_watchDogThread->quit();
        const bool didquit = m_watchDogThread->wait(2000);
        qCDebug(uidelays) << "watch thread quit?" << didquit;
        delete m_watchDogThread;

        m_watchDogThread = nullptr;
        m_worker = nullptr;
    }

    void onUiBeat() { m_worker->reset(); }

private:
    QTimer *const m_uiTimer;
    QThread *m_watchDogThread = nullptr;
    UiWatchdogWorker *m_worker = nullptr;
    const UiWatchdogWorker::Options m_options;
};

#endif
