// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "mimedatabase.h"
#include "mimetype.h"

#include "mimeglobpattern_p.h"
#include "mimemagicrule_p.h"
#include "mimetype_p.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>

#include <QReadWriteLock>

#include <atomic>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QIODevice;
QT_END_NAMESPACE

namespace Utils {

class MimeDatabase;
class MimeProviderBase;

class MimeDatabasePrivate
{
public:
    Q_DISABLE_COPY_MOVE(MimeDatabasePrivate)

    MimeDatabasePrivate();
    ~MimeDatabasePrivate();

    static MimeDatabasePrivate *instance();

    const QString &defaultMimeType() const { return m_defaultMimeType; }

    bool inherits(const QString &mime, const QString &parent);

    QList<MimeType> allMimeTypes();

    QString resolveAlias(const QString &nameOrAlias);
    QStringList parents(const QString &mimeName);
    MimeType mimeTypeForName(const QString &nameOrAlias);
    MimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device);
    MimeType mimeTypeForFileExtension(const QString &fileName);
    MimeType mimeTypeForData(QIODevice *device);
    MimeType mimeTypeForFile(const QString &fileName, const QFileInfo &fileInfo, MimeDatabase::MatchMode mode);
    MimeType findByData(const QByteArray &data, int *priorityPtr);
    QStringList mimeTypeForFileName(const QString &fileName);
    MimeGlobMatchResult findByFileName(const QString &fileName);

    // API for MimeType. Takes care of locking the mutex.
    MimeTypePrivate::LocaleHash localeComments(const QString &name);
    QStringList globPatterns(const QString &name);
    QString genericIcon(const QString &name);
    QString icon(const QString &name);
    QStringList mimeParents(const QString &mimeName);
    QStringList listAliases(const QString &mimeName);
    bool mimeInherits(const QString &mime, const QString &parent);

    // added for Qt Creator
    void addMimeData(const QString &id, const QByteArray &data);
    QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(const MimeType &mimeType);
    void setMagicRulesForMimeType(const MimeType &mimeType,
                                  const QMap<int, QList<MimeMagicRule>> &rules);
    void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns);
    void setPreferredSuffix(const QString &mimeName, const QString &suffix);
    void checkInitPhase(const QString &info);
    void addInitializer(const std::function<void()> &init);

private:
    using Providers = std::vector<std::unique_ptr<MimeProviderBase>>;
    const Providers &providers();
    bool shouldCheck();
    void loadProviders();
    QString fallbackParent(const QString &mimeTypeName) const;

    const QString m_defaultMimeType;
    mutable Providers m_providers; // most local first, most global last
    QElapsedTimer m_lastCheck;

    // added for Qt Creator
    QList<std::function<void()>> m_initializers;
    QHash<QString, QByteArray> m_additionalData; // id -> data
    bool m_forceLoad = true;

public:
    QMutex mutex;

    // added for Qt Creator
    QReadWriteLock m_initMutex;
    std::atomic_bool m_initialized = false;
    int m_startupPhase = 0;
    QHash<QString, QString> m_preferredSuffix; // MIME name -> suffix
};

} // namespace Utils
