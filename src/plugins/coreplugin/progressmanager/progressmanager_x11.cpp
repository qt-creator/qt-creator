// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressmanager_p.h"

#ifdef QTC_SUPPORT_UNITY_LAUNCHER
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QVariantMap>

namespace {

// State for the Unity LauncherEntry D-Bus API, which is honored by KDE Plasma
// (and formerly Unity) to show progress and a count badge on the task bar icon.
int minProgress = 0;
int totalProgress = 0;
double progressValue = 0.0;
bool progressVisible = false;
qint64 count = 0;
bool countVisible = false;

void sendLauncherUpdate()
{
    const QLatin1String desktopSuffix(".desktop");
    QString desktopFile = QGuiApplication::desktopFileName();
    if (!desktopFile.endsWith(desktopSuffix))
        desktopFile += desktopSuffix;
    const QString appUri = "application://" + desktopFile;

    const QVariantMap properties = {
        {"progress", progressValue},
        {"progress-visible", progressVisible},
        {"count", count},
        {"count-visible", countVisible},
    };

    QDBusMessage message = QDBusMessage::createSignal(
        "/com/canonical/unity/launcherentry/qtcreator",
        "com.canonical.Unity.LauncherEntry",
        "Update");
    message << appUri << properties;
    QDBusConnection::sessionBus().send(message);
}

} // namespace
#endif // QTC_SUPPORT_UNITY_LAUNCHER

void Core::Internal::ProgressManagerPrivate::initInternal()
{
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
#ifdef QTC_SUPPORT_UNITY_LAUNCHER
    progressVisible = false;
    countVisible = false;
    sendLauncherUpdate();
#endif
}

void Core::Internal::ProgressManagerPrivate::updateApplicationLabelNow()
{
#ifdef QTC_SUPPORT_UNITY_LAUNCHER
    bool ok = false;
    const int number = m_appLabelText.toInt(&ok);
    count = ok ? number : 0;
    countVisible = ok && number > 0;
    sendLauncherUpdate();
#endif
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
#ifdef QTC_SUPPORT_UNITY_LAUNCHER
    minProgress = min;
    totalProgress = max - min;
#else
    Q_UNUSED(min)
    Q_UNUSED(max)
#endif
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
#ifdef QTC_SUPPORT_UNITY_LAUNCHER
    progressValue = totalProgress > 0 ? double(value - minProgress) / totalProgress : 0.0;
    sendLauncherUpdate();
#else
    Q_UNUSED(value)
#endif
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
#ifdef QTC_SUPPORT_UNITY_LAUNCHER
    progressVisible = visible;
    sendLauncherUpdate();
#else
    Q_UNUSED(visible)
#endif
}
