// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfconversion.h"

#include "perfdataparser.h"
#include "perfrecordreader.h"
#include "perfregisters.h"
#include "perftraceconverter.h"
#include "profilertr.h"

#include <QTcpSocket>
#include <QTimer>

using namespace Utils;

namespace Profiler::Internal {

PerfConversion::PerfConversion(QObject *parent)
    : QObject(parent)
{}

PerfConversion::~PerfConversion()
{
    kill();
    waitForFinished();
}

void PerfConversion::start()
{
    // A run still in progress may wait for input only this thread can give.
    if (m_thread.joinable()) {
        kill();
        waitForFinished();
    }

    m_canceled = false;
    m_finished = false;
    m_result = Result::NotRun;
    m_errorString.clear();
    {
        std::lock_guard lock(m_mutex);
        m_output.clear();
        m_succeeded = false;
        m_threadError.clear();
        m_warnings.clear();
    }
    m_queue = std::make_shared<PerfByteQueue>();
    const quint64 generation = ++m_generation;

    if (!m_inputUrl.isEmpty()) {
        auto socket = new QTcpSocket(this);
        m_socket = socket;
        // What the conversion does not keep up with stays with the device,
        // which the full receive window holds back, rather than piling up here.
        socket->setReadBufferSize(SocketChunkSize);
        m_queue->setDrainedCallback(m_maxQueuedBytes, [this] {
            QMetaObject::invokeMethod(this, &PerfConversion::readFromSocket, Qt::QueuedConnection);
        });
        connect(socket, &QTcpSocket::readyRead, this, &PerfConversion::readFromSocket);
        connect(socket, &QTcpSocket::disconnected, this, [socket, queue = m_queue] {
            // Bounded by the read buffer.
            queue->push(socket->readAll());
            queue->close();
            socket->deleteLater();
        });
        connectSocket(socket, m_queue, QDeadlineTimer(m_connectTimeout));
    }

    m_thread = std::thread(&PerfConversion::run, this, m_queue, m_options, m_inputFile,
                           generation);
    QMetaObject::invokeMethod(this, [this, generation] {
        if (generation == m_generation)
            emit started();
    }, Qt::QueuedConnection);
}

void PerfConversion::connectSocket(QTcpSocket *socket,
                                   const std::shared_ptr<PerfByteQueue> &queue,
                                   const QDeadlineTimer &deadline)
{
    // A device starts listening only once the application is launched there,
    // which can well be after this connects.
    connect(socket, &QTcpSocket::errorOccurred, this,
            [this, socket, queue, deadline](QAbstractSocket::SocketError error) {
        if (socket->state() == QAbstractSocket::ConnectedState)
            return;
        const bool notYetListening = error == QAbstractSocket::ConnectionRefusedError
                                     || error == QAbstractSocket::SocketTimeoutError;
        if (notYetListening && !deadline.hasExpired()) {
            QTimer::singleShot(100, socket, [this, socket] {
                socket->connectToHost(m_inputUrl.host(), quint16(m_inputUrl.port()));
            });
            return;
        }
        {
            std::lock_guard lock(m_mutex);
            m_threadError = Tr::tr("Cannot connect to %1: %2")
                                .arg(m_inputUrl.toString(), socket->errorString());
        }
        queue->close();
    });
    socket->connectToHost(m_inputUrl.host(), quint16(m_inputUrl.port()));
}

void PerfConversion::run(std::shared_ptr<PerfByteQueue> queue, PerfConversionOptions options,
                         FilePath inputFile, quint64 generation)
{
    PerfData::PerfDataParser parser;
    PerfTraceConverter converter(parser, [this](const QByteArray &chunk) {
        {
            std::lock_guard lock(m_mutex);
            m_output.append(chunk);
        }
        QMetaObject::invokeMethod(this, &PerfConversion::readyRead, Qt::QueuedConnection);
    });
    converter.setCancelFlag(&m_canceled);
    converter.setWarningHandler([this](const QString &warning) {
        std::lock_guard lock(m_mutex);
        m_warnings.append(warning);
    });
    converter.symbolizer().setBinaryLocations(options.sysroot, options.searchPaths);
    const PerfArchitecture arch = perfArchitectureFromName(options.architecture);
    converter.setArchitecture(arch);
    // A target of another architecture, or with a root of its own, is another
    // machine, whose kernel this one's symbols are not.
    converter.setForeignRecording(!options.sysroot.isEmpty()
                                  || (arch != PerfArchitecture::Unknown
                                      && arch != hostPerfArchitecture()));

    const Utils::Result<> parsed = inputFile.isEmpty() ? parser.parseStream(*queue, converter)
                                                : parser.parseFile(inputFile, converter);
    if (parsed)
        converter.finish();
    {
        std::lock_guard lock(m_mutex);
        m_succeeded = bool(parsed);
        if (!parsed && m_threadError.isEmpty())
            m_threadError = parsed.error();
    }
    // Nobody reads the rest of a stream any more; let whoever writes it
    // finish rather than block.
    queue->close();
    // A start() since may have taken over: this run's completion is not its.
    QMetaObject::invokeMethod(this, [this, generation] {
        if (generation == m_generation)
            finishRun();
    }, Qt::QueuedConnection);
}

void PerfConversion::readFromSocket()
{
    if (!m_socket || !m_queue)
        return;
    while (m_socket->bytesAvailable() > 0 && m_queue->size() < m_maxQueuedBytes)
        m_queue->push(m_socket->read(SocketChunkSize));
}

void PerfConversion::finishRun()
{
    // Nothing converts what the device still sends.
    abortSocket();
    if (m_finished)
        return;
    waitForFinished();
    emit done();
}

void PerfConversion::abortSocket()
{
    if (!m_socket)
        return;
    m_socket->disconnect(this);
    m_socket->abort();
    m_socket->deleteLater();
    m_socket = nullptr;
}

qint64 PerfConversion::write(const QByteArray &data)
{
    if (!m_queue || !isRunning() || !isFed())
        return -1;
    m_queue->push(data);
    return data.size();
}

void PerfConversion::closeWriteChannel()
{
    // A recording from a file or a connection ends where they do.
    if (m_queue && isFed())
        m_queue->close();
}

void PerfConversion::kill()
{
    m_canceled = true;
    abortSocket();
    if (m_queue)
        m_queue->close();
}

void PerfConversion::waitForFinished()
{
    if (!m_thread.joinable())
        return;
    m_thread.join();
    m_finished = true;
    std::lock_guard lock(m_mutex);
    if (m_canceled) {
        m_result = Result::Canceled;
    } else if (m_succeeded && m_threadError.isEmpty()) {
        m_result = Result::Success;
    } else {
        m_result = Result::Failed;
        m_errorString = m_threadError;
    }
}

QStringList PerfConversion::warnings() const
{
    std::lock_guard lock(m_mutex);
    return m_warnings;
}

QByteArray PerfConversion::readAllOutput()
{
    std::lock_guard lock(m_mutex);
    return std::exchange(m_output, {});
}

} // namespace Profiler::Internal
