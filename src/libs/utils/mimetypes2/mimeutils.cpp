// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

#include "mimeutils.h"

#include "mimedatabase.h"
#include "mimedatabase_p.h"
#include "mimemagicrule_p.h"

#include "filepath.h"

#include <QUrl>

namespace Utils {

MimeType mimeTypeForName(const QString &nameOrAlias)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForName(nameOrAlias);
}

MimeType mimeTypeForFile(const QString &fileName, MimeMatchMode mode)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForFile(fileName, MimeDatabase::MatchMode(mode));
}

MimeType mimeTypeForFile(const FilePath &filePath, MimeMatchMode mode)
{
    MimeDatabase mdb;
    if (filePath.needsDevice() && mode != MimeMatchMode::MatchDefaultAndRemote)
        return mdb.mimeTypeForUrl(filePath.toUrl());
    if (mode == MimeMatchMode::MatchDefaultAndRemote) {
        mode = MimeMatchMode::MatchDefault;
    }
    return mdb.mimeTypeForFile(filePath.toFSPathString(), MimeDatabase::MatchMode(mode));
}

QList<MimeType> mimeTypesForFileName(const QString &fileName)
{
    MimeDatabase mdb;
    return mdb.mimeTypesForFileName(fileName);
}

MimeType mimeTypeForData(const QByteArray &data)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForData(data);
}

QList<MimeType> allMimeTypes()
{
    MimeDatabase mdb;
    return mdb.allMimeTypes();
}

void setMimeStartupPhase(MimeStartupPhase phase)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    if (int(phase) != d->m_startupPhase + 1) {
        qWarning("Unexpected jump in MimedDatabase lifetime from %d to %d",
                 d->m_startupPhase,
                 int(phase));
    }
    d->m_startupPhase = int(phase);
}

void addMimeTypes(const QString &id, const QByteArray &data)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase >= int(MimeStartupPhase::PluginsDelayedInitializing)) {
        qWarning("Adding items for ID \"%s\" to MimeDatabase after initialization time",
                 qPrintable(id));
    }

    d->addMimeData(id, data);
}

QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(const MimeType &mimeType)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    return d->magicRulesForMimeType(mimeType);
}

void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    d->setGlobPatternsForMimeType(mimeType, patterns);
}

void setMagicRulesForMimeType(const MimeType &mimeType, const QMap<int, QList<MimeMagicRule>> &rules)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    d->setMagicRulesForMimeType(mimeType, rules);
}

} // namespace Utils
