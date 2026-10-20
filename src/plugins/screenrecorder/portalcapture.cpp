// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "portalcapture.h"

#include "screenrecordertr.h"

#ifdef QTC_SCREENRECORDER_WITH_PORTAL
#include "screenrecorderconstants.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QUuid>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace ScreenRecorder::Internal {

PortalCapture::PortalCapture(QObject *parent)
    : QObject(parent)
{}

PortalCapture::~PortalCapture()
{
    stop();
}

#ifdef QTC_SCREENRECORDER_WITH_PORTAL

// D-Bus service, object path, interfaces and members of xdg-desktop-portal.
// https://flatpak.github.io/xdg-desktop-portal/docs/
static const char kPortalService[] = "org.freedesktop.portal.Desktop";
static const char kPortalPath[] = "/org/freedesktop/portal/desktop";
static const char kScreenCastInterface[] = "org.freedesktop.portal.ScreenCast";
static const char kRequestInterface[] = "org.freedesktop.portal.Request";
static const char kSessionInterface[] = "org.freedesktop.portal.Session";
static const char kResponseSignal[] = "Response";
static const char kCloseMethod[] = "Close";

static const char kFifoName[] = "portalcapture.fifo";

enum PortalCursorMode { CursorHidden = 1, CursorEmbedded = 2 };
// Only monitors: a window source can change its size during the recording, but the raw
// FFmpeg input has a fixed size.
enum PortalSourceType { SourceMonitor = 1 };
enum PortalResponse { ResponseSuccess = 0, ResponseCancelled = 1 };

static QString newToken()
{
    return "qtc" + QUuid::createUuid().toString(QUuid::Id128);
}

void PortalCapture::prepare(const Utils::FilePath &fifoDirectory, int frameRate,
                            bool captureCursor)
{
    stop();
    m_fifo = fifoDirectory / kFifoName;
    m_frameRate = frameRate;

    m_gstLaunch = Utils::Environment::systemEnvironment().searchInPath(
        Constants::GSTLAUNCH_COMMAND);
    if (m_gstLaunch.isEmpty()) {
        emit failed(Tr::tr("Cannot find \"%1\". Install GStreamer and its PipeWire "
                           "plugin, for example with\n\"%2\".")
                        .arg(Constants::GSTLAUNCH_COMMAND, Constants::GSTREAMER_INSTALL_COMMAND));
        return;
    }
    if (!QDBusConnection::sessionBus().isConnected()) {
        emit failed(Tr::tr("Cannot connect to the D-Bus session bus."));
        return;
    }
    callPortal("CreateSession", {}, {{"session_handle_token", newToken()}},
               [this, captureCursor](const QVariantMap &results) {
        m_sessionHandle = results.value("session_handle").toString();
        selectSources(captureCursor);
    });
}

void PortalCapture::callPortal(const QString &method, QVariantList arguments,
                               QVariantMap options, const ResultHandler &onSuccess)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    const QString token = newToken();
    options.insert("handle_token", token);

    // The portal answers asynchronously with a Response signal on a request object whose path
    // is derived from the unique bus name and the token. Subscribe before the call to not miss
    // a fast answer.
    const QString sender = bus.baseService().mid(1).replace('.', '_');
    m_requestPath = QString("%1/request/%2/%3").arg(kPortalPath, sender, token);
    m_onSuccess = onSuccess;
    bus.connect(kPortalService, m_requestPath, kRequestInterface, kResponseSignal, this,
                SLOT(onResponse(uint,QVariantMap)));

    QDBusMessage message = QDBusMessage::createMethodCall(kPortalService, kPortalPath,
                                                          kScreenCastInterface, method);
    arguments.append(options);
    message.setArguments(arguments);
    auto watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        if (w->isError())
            emit failed(Tr::tr("Screen cast portal error: %1").arg(w->error().message()));
        w->deleteLater();
    });
}

void PortalCapture::onResponse(uint code, const QVariantMap &results)
{
    QDBusConnection::sessionBus().disconnect(kPortalService, m_requestPath, kRequestInterface,
                                             kResponseSignal, this,
                                             SLOT(onResponse(uint,QVariantMap)));
    m_requestPath.clear();
    const auto onSuccess = std::exchange(m_onSuccess, {});
    if (code == ResponseCancelled) {
        emit cancelled();
    } else if (code != ResponseSuccess) {
        emit failed(Tr::tr("The screen cast portal request failed."));
    } else if (onSuccess) {
        onSuccess(results);
    }
}

void PortalCapture::selectSources(bool captureCursor)
{
    const QVariantMap options{
        {"types", uint(SourceMonitor)},
        {"multiple", false},
        {"cursor_mode", uint(captureCursor ? CursorEmbedded : CursorHidden)},
    };
    callPortal("SelectSources", {QVariant::fromValue(QDBusObjectPath(m_sessionHandle))}, options,
               [this](const QVariantMap &) { startStream(); });
}

void PortalCapture::startStream()
{
    callPortal("Start", {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), QString()}, {},
               [this](const QVariantMap &results) {
        const QDBusArgument streams = results.value("streams").value<QDBusArgument>();
        streams.beginArray();
        if (streams.atEnd()) {
            streams.endArray();
            emit failed(Tr::tr("The screen cast portal provided no stream."));
            return;
        }
        // Each stream is a (PipeWire node id, properties). See:
        // https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html#start
        uint nodeId = 0;
        QVariantMap properties;
        streams.beginStructure();
        streams >> nodeId >> properties;
        streams.endStructure();
        streams.endArray();

        // Node ids can be reused. Prefer the serial, which portal version 6 and later provide.
        const quint64 nodeSerial = properties.value("pipewire-serial").toULongLong();
        const QString pipeWireSource = nodeSerial
                                           ? QString("target-object=" + QString::number(nodeSerial))
                                           : QString("path=" + QString::number(nodeId));

        QSize size;
        const QDBusArgument sizeArgument = properties.value("size").value<QDBusArgument>();
        sizeArgument.beginStructure();
        sizeArgument >> size.rwidth() >> size.rheight();
        sizeArgument.endStructure();
        if (size.isEmpty()) {
            emit failed(Tr::tr("The screen cast portal did not report the stream size."));
            return;
        }

        m_fifo.removeFile();
        if (::mkfifo(m_fifo.toFSPathString().toLocal8Bit().constData(), S_IRUSR | S_IWUSR) != 0) {
            emit failed(Tr::tr("Cannot create the pipe %1.").arg(m_fifo.toUserOutput()));
            return;
        }
        m_streamSize = size;
        emit prepared();
        startPipeline(pipeWireSource);
    });
}

void PortalCapture::startPipeline(const QString &pipeWireSource)
{
    const QStringList arguments{
        "-q",
        "pipewiresrc", pipeWireSource,
        "do-timestamp=true",
        // PipeWire delivers frames only when the screen changes. Repeat the last frame in
        // between, so that FFmpeg records until the end and reports its progress.
        QString("keepalive-time=%1").arg(1000 / qMax(1, m_frameRate)),
        "!", "videoconvert",
        // The portal reports the size in compositor coordinates. On scaled screens, the frames
        // are larger. Scale them to the size that FFmpeg reads.
        "!", "videoscale",
        "!", QString("video/x-raw,format=BGRx,width=%1,height=%2")
                 .arg(m_streamSize.width()).arg(m_streamSize.height()),
        "!", "filesink", "location=" + m_fifo.toFSPathString(), "sync=false",
    };
    m_pipeline = new Utils::Process(this);
    m_pipeline->setCommand({m_gstLaunch, arguments});
    connect(m_pipeline, &Utils::Process::done, this, [this] {
        emit failed(Tr::tr("GStreamer stopped unexpectedly:\n%1")
                        .arg(m_pipeline->exitMessage(
                            Utils::Process::FailureMessageFormat::WithStdErr)));
    });
    m_pipeline->start();
}

QStringList PortalCapture::ffmpegGrabArguments() const
{
    // PipeWire delivers frames irregularly. Timestamp them on arrival, and let the fps filter
    // make the frame rate constant.
    return {
        "-use_wallclock_as_timestamps", "1",
        "-f", "rawvideo",
        "-pixel_format", "bgr0",
        "-video_size", QString("%1x%2").arg(m_streamSize.width()).arg(m_streamSize.height()),
        "-i", m_fifo.toFSPathString(),
        "-filter_complex", "fps=" + QString::number(m_frameRate),
    };
}

void PortalCapture::stop()
{
    if (!m_requestPath.isEmpty()) {
        // Closing the request also closes its dialog.
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.disconnect(kPortalService, m_requestPath, kRequestInterface, kResponseSignal, this,
                       SLOT(onResponse(uint,QVariantMap)));
        bus.asyncCall(QDBusMessage::createMethodCall(kPortalService, m_requestPath,
                                                     kRequestInterface, kCloseMethod));
        m_requestPath.clear();
        m_onSuccess = {};
    }
    if (m_pipeline) {
        m_pipeline->disconnect(this);
        if (m_pipeline->isRunning())
            m_pipeline->kill();
        m_pipeline->deleteLater();
        m_pipeline = nullptr;
    }
    if (!m_sessionHandle.isEmpty()) {
        QDBusConnection::sessionBus().asyncCall(
            QDBusMessage::createMethodCall(kPortalService, m_sessionHandle, kSessionInterface,
                                           kCloseMethod));
        m_sessionHandle.clear();
    }
    if (!m_fifo.isEmpty()) {
        // FFmpeg waits in open() until a writer opens the pipe. If GStreamer never opened it,
        // open it once, so that FFmpeg reads the end of the pipe.
        const int fd = ::open(m_fifo.toFSPathString().toLocal8Bit().constData(),
                              O_WRONLY | O_NONBLOCK);
        if (fd >= 0)
            ::close(fd);
        m_fifo.removeFile();
    }
}

#else // QTC_SCREENRECORDER_WITH_PORTAL

void PortalCapture::prepare(const Utils::FilePath &, int, bool)
{
    emit failed(Tr::tr("Screen cast portals are not supported on this platform."));
}

QStringList PortalCapture::ffmpegGrabArguments() const
{
    return {};
}

void PortalCapture::stop() {}

void PortalCapture::onResponse(uint, const QVariantMap &) {}

#endif // QTC_SCREENRECORDER_WITH_PORTAL

} // namespace ScreenRecorder::Internal
