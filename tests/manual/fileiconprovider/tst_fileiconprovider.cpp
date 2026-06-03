// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Stress-tests FileIconProvider::icon() for thread safety.
//
// What it does:
//   - Spawns kNumThreads background threads that each call icon() in a tight loop.
//   - The main thread also calls icon() and intermittently registers new overlays,
//     exercising concurrent registration vs. lookup.
//   - Runs for kDurationSecs seconds then prints a summary.
//
// Pass criteria:
//   - No crash, no Qt assertion failure, no data-race report from TSan.
//   - "All icons valid" in the status line (icon() must never return a null QIcon).
//
// Run with TSan for the strongest check:
//   TSAN_OPTIONS=halt_on_error=1 ./tst_manual_fileiconprovider

#include <utils/devicefileaccess.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/filepath.h>

#include <QApplication>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QStyle>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <atomic>

using namespace Utils;

// Minimal stub so FilePath::fileAccess() doesn't fire QTC_CHECK(false) for non-local paths.
// isDirectory() answers based on whether the path component ends in "dir".
class StubDeviceFileAccess : public DeviceFileAccess
{
protected:
    Result<bool> isDirectory(const FilePath &fp) const override
    {
        return fp.path().endsWith("/dir");
    }
    Result<bool> isFile(const FilePath &fp) const override
    {
        return !fp.path().endsWith("/dir");
    }
};

static void setupDeviceHooks()
{
    static StubDeviceFileAccess stub;
    DeviceFileHooks hooks;
    hooks.fileAccess = [](const FilePath &) -> Result<DeviceFileAccessPtr> {
        static auto ptr = std::shared_ptr<DeviceFileAccess>(&stub, [](auto *) {});
        return ptr;
    };
    hooks.isSameDevice = [](const FilePath &lhs, const FilePath &rhs) {
        return lhs.scheme() == rhs.scheme() && lhs.host() == rhs.host();
    };
    DeviceFileHooks::setupDeviceFileHooks(hooks);
}

static constexpr int kDurationSecs = 60;
static constexpr int kNumThreads = 8;

// Paths chosen to hit all branches in icon():
//   - local files with registered suffixes (cpp, h)
//   - local files with unregistered suffixes (py, js, txt)
//   - local directories
//   - non-existent files (exercises the cache-miss path)
//   - a fake remote path (exercises the !isLocal() early-return)
static const char *kPaths[] = {
    "/tmp/test.cpp",
    "/tmp/test.h",
    "/tmp/test.py",
    "/tmp/test.js",
    "/tmp/test.txt",
    "/tmp/test.qml",
    "/tmp/test.cmake",
    "/tmp/test.nonexistent_suffix_xyz",
    "/tmp",
    "/usr",
    "/__qtc_devices__/docker/mycontainer/tmp/file.cpp",
    "/__qtc_devices__/docker/mycontainer/tmp/dir",
};
static constexpr int kNumPaths = static_cast<int>(std::size(kPaths));

class WorkerThread : public QThread
{
public:
    WorkerThread(int id, std::atomic<bool> &stop)
        : m_stop(stop)
    {
        setObjectName(QString("worker-%1").arg(id));
    }

    qint64 callCount() const { return m_callCount.loadRelaxed(); }
    bool hadNullIcon() const { return m_hadNullIcon.loadRelaxed() != 0; }

protected:
    void run() override
    {
        while (!m_stop.load(std::memory_order_relaxed)) {
            for (const char *path : kPaths) {
                const QIcon ic = FileIconProvider::icon(FilePath::fromString(path));
                if (ic.isNull())
                    m_hadNullIcon.storeRelaxed(1);
                m_callCount.fetchAndAddRelaxed(1);
            }
        }
    }

private:
    std::atomic<bool> &m_stop;
    QAtomicInt m_callCount{0};
    QAtomicInt m_hadNullIcon{0};
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    setupDeviceHooks();

    // Register overlays up front so background threads immediately exercise the
    // "hit in global suffixCache" path. Use a real QIcon so the overlay is valid.
    const QIcon overlayIcon = app.style()->standardIcon(QStyle::SP_FileIcon);
    FileIconProvider::registerIconOverlayForMimeType(overlayIcon, "text/x-csrc");   // .c
    FileIconProvider::registerIconOverlayForMimeType(overlayIcon, "text/x-chdr");   // .h

    auto *window = new QWidget;
    window->setWindowTitle(
        QString("FileIconProvider stress test — %1 threads, %2 s").arg(kNumThreads).arg(kDurationSecs));

    auto *root = new QVBoxLayout(window);

    auto *topRow = new QHBoxLayout;
    root->addLayout(topRow);

    auto *progress = new QProgressBar;
    progress->setRange(0, kDurationSecs);
    topRow->addWidget(progress);

    static constexpr const char *kSpinnerFrames[] = {"|", "/", "-", "\\"};
    auto *spinnerLabel = new QLabel(kSpinnerFrames[0]);
    spinnerLabel->setFixedWidth(16);
    spinnerLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(spinnerLabel);

    auto *grid = new QGridLayout;
    root->addLayout(grid);

    QList<QLabel *> threadLabels;
    for (int i = 0; i < kNumThreads; ++i) {
        auto *lbl = new QLabel(QString("Thread %1: starting…").arg(i));
        grid->addWidget(lbl, i / 2, i % 2);
        threadLabels.append(lbl);
    }

    auto *mainLabel = new QLabel("Main thread: 0 calls");
    root->addWidget(mainLabel);

    auto *statusLabel = new QLabel("Running…");
    root->addWidget(statusLabel);

    window->resize(520, 380);
    window->show();

    // Start background threads.
    std::atomic<bool> stop{false};
    QList<WorkerThread *> threads;
    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i) {
        auto *t = new WorkerThread(i, stop);
        threads.append(t);
        t->start();
    }

    QElapsedTimer elapsed;
    elapsed.start();
    qint64 mainCalls = 0;

    // Periodically register new overlays while threads are running.
    // This exercises concurrent write (global SynchronizedValue write lock) vs. read.
    static const char *kExtraSuffixes[] = {"foo", "bar", "baz", "qux"};
    int regIdx = 0;
    auto *regTimer = new QTimer(&app);
    regTimer->setInterval(3000);
    QObject::connect(regTimer, &QTimer::timeout, [&]() {
        FileIconProvider::registerIconOverlayForSuffix(
            "/nonexistent/overlay.png", kExtraSuffixes[regIdx++ % 4]);
    });
    regTimer->start();

    int spinnerIdx = 0;
    auto *spinnerTimer = new QTimer(&app);
    spinnerTimer->setInterval(16);
    QObject::connect(spinnerTimer, &QTimer::timeout, [&]() {
        spinnerLabel->setText(kSpinnerFrames[++spinnerIdx % 4]);
    });
    spinnerTimer->start();

    // Update UI and drive the main-thread icon() calls.
    auto *updateTimer = new QTimer(&app);
    updateTimer->setInterval(200);
    QObject::connect(updateTimer, &QTimer::timeout, [&]() {
        const int sec = static_cast<int>(elapsed.elapsed() / 1000);
        progress->setValue(sec);

        // Main-thread calls — exercises the OS-lookup and local cache path.
        for (const char *path : kPaths)
            FileIconProvider::icon(FilePath::fromString(path));
        mainCalls += kNumPaths;
        mainLabel->setText(QString("Main thread: %1 calls").arg(mainCalls));

        bool anyNull = false;
        qint64 total = mainCalls;
        for (int i = 0; i < threads.size(); ++i) {
            const qint64 n = threads[i]->callCount();
            const bool bad = threads[i]->hadNullIcon();
            anyNull |= bad;
            total += n;
            threadLabels[i]->setText(
                QString("Thread %1: %2 calls%3").arg(i).arg(n).arg(bad ? " *** NULL ICON ***" : ""));
        }

        if (sec < kDurationSecs)
            return;

        stop.store(true, std::memory_order_relaxed);
        updateTimer->stop();
        regTimer->stop();
        spinnerTimer->stop();
        FileIconProvider::shutdown();

        for (auto *t : threads)
            t->wait();

        const qint64 rate = kDurationSecs > 0 ? total / kDurationSecs : 0;
        statusLabel->setText(
            QString("%1 — %2 total calls in %3 s (%4 calls/s)")
                .arg(anyNull ? "FAIL: null icon(s) returned" : "PASS: all icons valid")
                .arg(total)
                .arg(kDurationSecs)
                .arg(rate));
    });
    updateTimer->start();

    const int ret = app.exec();

    // Signal threads to stop, then drain any requests they may be blocking on,
    // then join — all before QApplication destructs.
    stop.store(true, std::memory_order_relaxed);

    for (auto *t : threads)
        t->wait();

    return ret;
}
