// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

// FIXME: Remove once dowstream is adjusted.
#include "projectmanager.h"

#include <utils/id.h>
#include <utils/persistentsettings.h>

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT SessionManager : public QObject
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

    static Utils::FilePath sessionNameToFileName(const QString &session);

    static bool isDefaultVirgin();
    static bool isDefaultSession(const QString &session);

    // Let other plugins store persistent values within the session file
    static void setValue(const QString &name, const QVariant &value);
    static QVariant value(const QString &name);

    static bool loadingSession();
    static void markSessionFileDirty();

signals:
    void aboutToUnloadSession(QString sessionName);
    void aboutToLoadSession(QString sessionName);
    void sessionLoaded(QString sessionName);
    void aboutToSaveSession();

    void sessionRenamed(const QString &oldName, const QString &newName);
    void sessionRemoved(const QString &name);

private:
    static void saveActiveMode(Utils::Id mode);
};

} // namespace ProjectExplorer
