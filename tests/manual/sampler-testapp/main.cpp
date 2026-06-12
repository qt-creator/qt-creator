// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Manual test target for the qmltraceviewer sampling profiler: spawns named
// threads with distinctive call stacks and CPU behaviors. Set
// RecordProcessName=sampler-testapp in the viewer settings, then record:
//  - "fibonacci": always busy in a deep recursive chain (heaviest path)
//  - "hasher":    always busy in a second hot root (tree merging)
//  - "bursty":    100 ms busy / 400 ms asleep (CPU-usage wave)
//  - "sleeper":   almost always blocked (should nearly vanish from the tree)
//  - main:        idle event loop with a 1 s heartbeat (low-weight stack)
//  - QML window:  a looping animation drives the GUI and scene-graph render
//                 threads, so the sampler also sees Qt Quick activity.

#include <QGuiApplication>
#include <QElapsedTimer>
#include <QList>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <atomic>
#include <csignal>
#include <cstdio>

namespace {

std::atomic_bool stopRequested{false};

// Every chain below is built from noinline functions writing through volatile
// sinks, so the optimizer cannot collapse the frames and the sampler sees each
// function as its own stack level.

[[gnu::noinline]] quint64 fib(quint64 n)
{
    if (n < 2)
        return n;
    return fib(n - 1) + fib(n - 2);
}

[[gnu::noinline]] void computeFibonacci()
{
    volatile quint64 sink = fib(30);
    (void) sink;
}

[[gnu::noinline]] void fibonacciWorker()
{
    while (!stopRequested.load(std::memory_order_relaxed))
        computeFibonacci();
}

[[gnu::noinline]] quint64 mixBytes(quint64 state)
{
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 0x2545F4914F6CDD1DULL; // xorshift* multiplier
}

[[gnu::noinline]] quint64 hashBlock(quint64 seed)
{
    quint64 state = seed | 1;
    for (int i = 0; i < 100000; ++i)
        state = mixBytes(state);
    return state;
}

[[gnu::noinline]] void hasherWorker()
{
    volatile quint64 sink = 0;
    while (!stopRequested.load(std::memory_order_relaxed))
        sink = hashBlock(sink);
}

[[gnu::noinline]] void spinFor(qint64 ms)
{
    QElapsedTimer timer;
    timer.start();
    volatile quint64 sink = 0;
    while (!timer.hasExpired(ms) && !stopRequested.load(std::memory_order_relaxed))
        sink = mixBytes(sink + 1);
    (void) sink;
}

[[gnu::noinline]] void burstOfWork()
{
    spinFor(100);
    QThread::msleep(400);
}

[[gnu::noinline]] void burstyWorker()
{
    while (!stopRequested.load(std::memory_order_relaxed))
        burstOfWork();
}

[[gnu::noinline]] void waitAround()
{
    QThread::msleep(1000);
}

[[gnu::noinline]] void sleeperWorker()
{
    while (!stopRequested.load(std::memory_order_relaxed))
        waitAround();
}

[[gnu::noinline]] void printHeartbeat()
{
    static int beat = 0;
    std::printf("heartbeat %d\n", ++beat);
    std::fflush(stdout);
}

QThread *startWorker(const QString &name, void (*worker)())
{
    QThread *thread = QThread::create(worker);
    thread->setObjectName(name); // becomes the OS thread name the sampler reads
    thread->start();
    return thread;
}

void requestStop(int)
{
    stopRequested.store(true); // async-signal-safe: just set the flag
}

} // namespace

// A self-contained Qt Quick window with a never-ending animation. Loaded from
// data so the test app needs no .qrc; the running animation keeps the GUI and
// scene-graph render threads busy for the sampler to capture.
static const char *const qmlSource = R"qml(
import QtQuick
import QtQuick.Window

Window {
    width: 400
    height: 400
    visible: true
    color: "#202020"
    title: "sampler-testapp"

    // Deliberately heavy JS triggered ~125x/second, so a sampler that catches
    // the QML/JS engine sees spinInQml() on the stack often.
    property real qmlAccumulator: 0
    function spinInQml() {
        var sum = 0;
        for (var i = 1; i < 200000; ++i)
            sum += Math.sqrt(i) * Math.sin(i);
        qmlAccumulator = sum;
    }

    Timer {
        interval: 8
        running: true
        repeat: true
        onTriggered: spinInQml()
    }

    Rectangle {
        id: box
        anchors.centerIn: parent
        width: 120
        height: 120
        radius: 12
        color: "#3daee9"

        RotationAnimation on rotation {
            from: 0; to: 360
            duration: 2000
            loops: Animation.Infinite
            running: true
        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: true
            NumberAnimation { from: 1.0; to: 1.6; duration: 1000; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 1.6; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
        }
    }
}
)qml";

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("sampler-testapp");

    std::printf("sampler-testapp running, pid %lld\n",
                qint64(QCoreApplication::applicationPid()));
    std::printf("Set RecordProcessName=sampler-testapp in the trace viewer settings, "
                "then record. Ctrl+C stops.\n");
    std::fflush(stdout);

    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);

    QQmlApplicationEngine engine;
    engine.loadData(QByteArray(qmlSource), QUrl("qrc:/sampler-testapp/main.qml"));

    const QList<QThread *> workers = {
        startWorker("fibonacci", fibonacciWorker),
        startWorker("hasher", hasherWorker),
        startWorker("bursty", burstyWorker),
        startWorker("sleeper", sleeperWorker),
    };

    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, &app, [] { printHeartbeat(); });
    heartbeat.start(1000);

    // The handler only sets the flag; this watcher turns it into a clean quit.
    QTimer stopWatcher;
    QObject::connect(&stopWatcher, &QTimer::timeout, &app, [&app] {
        if (stopRequested.load(std::memory_order_relaxed))
            app.quit();
    });
    stopWatcher.start(100);

    const int result = app.exec();

    for (QThread *worker : workers) {
        worker->wait(); // sleeper may take up to 1 s to notice the flag
        delete worker;
    }
    std::printf("stopped\n");
    std::fflush(stdout);
    return result;
}
