// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtsingleapplication.h"
#include "qtlocalpeer.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QLockFile>
#include <QPointer>
#include <QSharedMemory>
#include <QWidget>

namespace SharedTools {

static const int instancesSize = 1024;

static QString instancesLockFilename(const QString &appSessionId)
{
    const QChar slash(QLatin1Char('/'));
    QString res = QDir::tempPath();
    if (!res.endsWith(slash))
        res += slash;
    return res + appSessionId + QLatin1String("-instances");
}

QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv),
      firstPeer(-1),
      pidPeer(0)
{
    this->appId = appId;

    const QString appSessionId = QtLocalPeer::appSessionId(appId);

    // This shared memory holds a zero-terminated array of active (or crashed) instances
    instances = new QSharedMemory(appSessionId, this);
    actWin = 0;
    block = false;

    // First instance creates the shared memory, later instances attach to it
    const bool created = instances->create(instancesSize);
    if (!created) {
        if (!instances->attach()) {
            qWarning() << "Failed to initialize instances shared memory: "
                       << instances->errorString();
            delete instances;
            instances = 0;
            return;
        }
    }

    // QLockFile is used to workaround QTBUG-10364
    QLockFile lockfile(instancesLockFilename(appSessionId));

    lockfile.lock();
    qint64 *pids = static_cast<qint64 *>(instances->data());
    if (!created) {
        // Find the first instance that it still running
        // The whole list needs to be iterated in order to append to it
        for (; *pids; ++pids) {
            if (firstPeer == -1 && isRunning(*pids))
                firstPeer = *pids;
        }
    }
    // Add current pid to list and terminate it
    *pids++ = QCoreApplication::applicationPid();
    *pids = 0;
    pidPeer = new QtLocalPeer(this, appId + QLatin1Char('-') +
                              QString::number(QCoreApplication::applicationPid()));
    connect(pidPeer, &QtLocalPeer::messageReceived, this, &QtSingleApplication::messageReceived);
    pidPeer->isClient();
    lockfile.unlock();
}

QtSingleApplication::~QtSingleApplication()
{
    if (!instances)
        return;
    const qint64 appPid = QCoreApplication::applicationPid();
    QLockFile lockfile(instancesLockFilename(QtLocalPeer::appSessionId(appId)));
    lockfile.lock();
    // Rewrite array, removing current pid and previously crashed ones
    qint64 *pids = static_cast<qint64 *>(instances->data());
    qint64 *newpids = pids;
    for (; *pids; ++pids) {
        if (*pids != appPid && isRunning(*pids))
            *newpids++ = *pids;
    }
    *newpids = 0;
    lockfile.unlock();
}

bool QtSingleApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(event);
        emit fileOpenRequest(foe->file());
        return true;
    }
    return QApplication::event(event);
}

bool QtSingleApplication::isRunning(qint64 pid)
{
    if (pid == -1) {
        pid = firstPeer;
        if (pid == -1)
            return false;
    }

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.isClient();
}

bool QtSingleApplication::sendMessage(const QString &message, int timeout, qint64 pid)
{
    if (pid == -1) {
        pid = firstPeer;
        if (pid == -1)
            return false;
    }

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.sendMessage(message, timeout, block);
}

QString QtSingleApplication::applicationId() const
{
    return appId;
}

void QtSingleApplication::setBlock(bool value)
{
    block = value;
}

void QtSingleApplication::setActivationWindow(QWidget *aw, bool activateOnMessage)
{
    actWin = aw;
    if (!pidPeer)
        return;
    if (activateOnMessage)
        connect(pidPeer, &QtLocalPeer::messageReceived, this, &QtSingleApplication::activateWindow);
    else
        disconnect(pidPeer, &QtLocalPeer::messageReceived, this, &QtSingleApplication::activateWindow);
}

QWidget* QtSingleApplication::activationWindow() const
{
    return actWin;
}

void QtSingleApplication::activateWindow()
{
    if (actWin) {
        actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
        actWin->raise();
        actWin->activateWindow();
    }
}

static const char s_freezeDetector[] = "QTC_FREEZE_DETECTOR";

static std::optional<int> isUsingFreezeDetector()
{
    if (!qEnvironmentVariableIsSet(s_freezeDetector))
        return {};

    bool ok = false;
    const int threshold = qEnvironmentVariableIntValue(s_freezeDetector, &ok);
    return ok ? threshold : 100; // default value 100ms
}

class ApplicationWithFreezerDetector : public SharedTools::QtSingleApplication
{
public:
    ApplicationWithFreezerDetector(const QString &id, int &argc, char **argv)
        : QtSingleApplication(id, argc, argv)
        , m_align(21, QChar::Space)
    {}
    void setFreezeTreshold(std::chrono::milliseconds freezeAbove) { m_threshold = freezeAbove; }

    bool notify(QObject *receiver, QEvent *event) override {
        using namespace std::chrono;
        const auto start = system_clock::now();
        const QPointer<QObject> p(receiver);
        const QString className = QLatin1String(receiver->metaObject()->className());
        const QString name = receiver->objectName();

        const bool ret = QtSingleApplication::notify(receiver, event);

        const auto end = system_clock::now();
        const auto freeze = duration_cast<milliseconds>(end - start);
        if (freeze > m_threshold) {
            const QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
            qDebug().noquote() << QString("FREEZE [%1]").arg(time)
                               << "of" << freeze.count() << "ms, on:" << event;
            const QString receiverMessage = name.isEmpty()
                ? QString("receiver class: %1").arg(className)
                : QString("receiver class: %1, object name: %2").arg(className, name);
            qDebug().noquote() << m_align << receiverMessage;
            if (!p)
                qDebug().noquote() << m_align << "THE RECEIVER GOT DELETED inside the event filter!";
        }
        return ret;
    }

private:
    const QString m_align;
    std::chrono::milliseconds m_threshold = std::chrono::milliseconds(100);
};

QtSingleApplication *createApplication(const QString &id, int &argc, char **argv)
{
    const std::optional<int> freezeDetector = isUsingFreezeDetector();
    if (!freezeDetector)
        return new SharedTools::QtSingleApplication(id, argc, argv);

    qDebug() << s_freezeDetector << "evn var is set. The freezes of main thread, above"
             << *freezeDetector << "ms, will be reported.";
    qDebug() << "Change the freeze detection threshold by setting the" << s_freezeDetector
             << "env var to a different numeric value (in ms).";
    ApplicationWithFreezerDetector *app = new ApplicationWithFreezerDetector(id, argc, argv);
    app->setFreezeTreshold(std::chrono::milliseconds(*freezeDetector));
    return app;
}

} // namespace SharedTools
