// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Utils {
class FilePath;
class Key;
} // Utils

namespace Core {

class CORE_EXPORT SessionManager : public QObject
{
    Q_OBJECT

public:
    SessionManager();
    ~SessionManager() override;

    static SessionManager *instance();

    // higher level session management
    static QString activeSession();
    static QString lastSession();
    static QString startupSession();
    static QStringList sessions();
    static QDateTime sessionDateTime(const QString &session);
    static QDateTime lastActiveTime(const QString &session);

    static bool createSession(const QString &session);

    static bool confirmSessionDelete(const QStringList &sessions);
    static bool deleteSession(const QString &session);
    static void deleteSessions(const QStringList &sessions);

    static bool cloneSession(const QString &original, const QString &clone);
    static bool renameSession(const QString &original, const QString &newName);
    static void showSessionManager();

    static Utils::FilePath sessionNameToFileName(const QString &session);

    static bool isDefaultVirgin();
    static bool isDefaultSession(const QString &session);

    // Let other plugins store persistent values within the session file
    // These are settings that are also saved and loaded at startup, and are taken over
    // to the default session when switching from implicit to explicit default session
    static void setValue(const Utils::Key &name, const QVariant &value);
    static QVariant value(const Utils::Key &name);

    // These are settings that are specific to a session and are not loaded
    // at startup and also not taken over to the default session when switching from implicit
    static void setSessionValue(const Utils::Key &name, const QVariant &value);
    static QVariant sessionValue(const Utils::Key &name, const QVariant &defaultValue = {});

    static bool isLoadingSession();
    static void markSessionFileDirty();

    static void sessionLoadingProgress();
    static void addSessionLoadingSteps(int steps);

    static bool loadSession(const QString &session, bool initial = false);
    static bool saveSession();

signals:
    void startupSessionRestored();
    void aboutToUnloadSession(QString sessionName);
    // Sent during session loading, after the values of the session are available via value() and
    // sessionValue. Use to restore values from the new session
    void aboutToLoadSession(QString sessionName);
    void sessionLoaded(QString sessionName);
    void aboutToSaveSession();

    void sessionCreated(const QString &name);
    void sessionRenamed(const QString &oldName, const QString &newName);
    void sessionRemoved(const QString &name);
};

} // namespace Core
