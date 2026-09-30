// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>

#include <QByteArray>
#include <QDeadlineTimer>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace Profiler::Internal {

class PerfByteQueue;

// Where the binaries of a recording are to be found on this machine, for a
// recording taken on another one: the target's root filesystem, and
// directories holding the application's own binaries.
struct PerfConversionOptions
{
    Utils::FilePath sysroot;
    Utils::FilePaths searchPaths;
    QString architecture; // as a kit's ABI names it; empty for this machine's
};

// Converts a perf recording for the CPU Usage analyzer, in this process, on
// a thread of its own (see PerfTraceConverter): from what write() is fed, as
// "perf record -o -" writes it, from a perf.data file, or from a TCP
// connection a device sends it over. The converted stream arrives through
// readyRead(), in chunks.
class PROFILER_EXPORT PerfConversion : public QObject
{
    Q_OBJECT

public:
    enum class Result { NotRun, Success, Failed, Canceled };

    explicit PerfConversion(QObject *parent = nullptr);
    ~PerfConversion() override;

    void setOptions(const PerfConversionOptions &options) { m_options = options; }
    // Converts the file at `path` instead of what write() is fed.
    void setInputFile(const Utils::FilePath &path) { m_inputFile = path; }
    // Converts what a connection to `url` delivers instead.
    void setInputUrl(const QUrl &url) { m_inputUrl = url; }
    // How much of what the connection delivers is queued for the conversion
    // at most. The rest stays with the device until the conversion catches up.
    void setMaxQueuedBytes(qint64 bytes) { m_maxQueuedBytes = bytes; }
    // How long a refused connection is tried again, for a device that only
    // starts listening once its recording is under way.
    void setConnectTimeout(std::chrono::milliseconds timeout) { m_connectTimeout = timeout; }
    // Whether the recording comes through write().
    bool isFed() const { return m_inputFile.isEmpty() && m_inputUrl.isEmpty(); }
    // Whether the recording is still being made, fed or over a connection.
    bool isLive() const { return m_inputFile.isEmpty(); }

    // Ends a run still in progress first, discarding what it has not
    // converted yet.
    void start();
    bool isRunning() const { return m_thread.joinable() && !m_finished; }

    // Hands the converter more of the recording; returns how much it took.
    qint64 write(const QByteArray &data);
    // The recording fed through write() is complete.
    void closeWriteChannel();

    // Stops converting, discarding what is not converted yet.
    void kill();
    void waitForFinished();

    QByteArray readAllOutput();
    Result result() const { return m_result; }
    QString errorString() const { return m_errorString; }
    // What the user should know about a conversion that went on regardless.
    QStringList warnings() const;

signals:
    void started();
    void readyRead();
    void done();

private:
    void run(std::shared_ptr<PerfByteQueue> queue, PerfConversionOptions options,
             Utils::FilePath inputFile, quint64 generation);
    void finishRun();
    void connectSocket(QTcpSocket *socket, const std::shared_ptr<PerfByteQueue> &queue,
                       const QDeadlineTimer &deadline);
    void readFromSocket();
    void abortSocket();

    // How much is read from a connection at a time.
    static constexpr qint64 SocketChunkSize = 1 << 20;

    PerfConversionOptions m_options;
    Utils::FilePath m_inputFile;
    QUrl m_inputUrl;
    qint64 m_maxQueuedBytes = 16 << 20;
    std::chrono::milliseconds m_connectTimeout = std::chrono::seconds(10);
    // Which start() a run's queued notifications belong to.
    quint64 m_generation = 0;

    std::shared_ptr<PerfByteQueue> m_queue;
    std::thread m_thread;
    std::atomic_bool m_canceled = false;
    bool m_finished = false;
    QPointer<QTcpSocket> m_socket;

    mutable std::mutex m_mutex; // guards the four below, which the thread writes
    QByteArray m_output;
    bool m_succeeded = false;
    QString m_threadError;
    QStringList m_warnings;

    Result m_result = Result::NotRun;
    QString m_errorString;
};

} // namespace Profiler::Internal
