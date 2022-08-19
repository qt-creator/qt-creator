// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

#include "mimeutils.h"

#include "mimedatabase.h"
#include "mimedatabase_p.h"
#include "mimemagicrule_p.h"
#include "mimeprovider_p.h"

#include "filepath.h"

using namespace Utils;
using namespace Utils::Internal;

void Utils::addMimeTypes(const QString &fileName, const QByteArray &data)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);

    if (d->m_startupPhase >= MimeDatabase::PluginsDelayedInitializing)
        qWarning("Adding items from %s to MimeDatabase after initialization time",
                 qPrintable(fileName));

    auto xmlProvider = static_cast<MimeXMLProvider *>(d->provider());
    xmlProvider->addData(fileName, data);
}

QMap<int, QList<MimeMagicRule>> Utils::magicRulesForMimeType(const MimeType &mimeType)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    return d->provider()->magicRulesForMimeType(mimeType);
}

void Utils::setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    d->provider()->setGlobPatternsForMimeType(mimeType, patterns);
}

void Utils::setMagicRulesForMimeType(const MimeType &mimeType,
                                     const QMap<int, QList<MimeMagicRule>> &rules)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    d->provider()->setMagicRulesForMimeType(mimeType, rules);
}

void Utils::setMimeStartupPhase(MimeStartupPhase phase)
{
    auto d = MimeDatabasePrivate::instance();
    QMutexLocker locker(&d->mutex);
    if (int(phase) != d->m_startupPhase + 1)
        qWarning("Unexpected jump in MimedDatabase lifetime from %d to %d",
                 d->m_startupPhase,
                 int(phase));
    d->m_startupPhase = int(phase);
}

MimeType Utils::mimeTypeForName(const QString &nameOrAlias)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForName(nameOrAlias);
}

MimeType Utils::mimeTypeForFile(const QString &fileName, MimeMatchMode mode)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForFile(fileName, MimeDatabase::MatchMode(mode));
}

MimeType Utils::mimeTypeForFile(const QFileInfo &fileInfo, MimeMatchMode mode)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForFile(fileInfo, MimeDatabase::MatchMode(mode));
}

MimeType Utils::mimeTypeForFile(const FilePath &filePath, MimeMatchMode mode)
{
    MimeDatabase mdb;
    if (filePath.needsDevice())
        return mdb.mimeTypeForUrl(filePath.toUrl());
    return mdb.mimeTypeForFile(filePath.toString(), MimeDatabase::MatchMode(mode));
}

QList<MimeType> Utils::mimeTypesForFileName(const QString &fileName)
{
    MimeDatabase mdb;
    return mdb.mimeTypesForFileName(fileName);
}

MimeType Utils::mimeTypeForData(const QByteArray &data)
{
    MimeDatabase mdb;
    return mdb.mimeTypeForData(data);
}

QList<MimeType> Utils::allMimeTypes()
{
    MimeDatabase mdb;
    return mdb.allMimeTypes();
}
