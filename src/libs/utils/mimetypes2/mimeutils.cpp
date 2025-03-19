// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    if (!filePath.isLocal() && mode != MimeMatchMode::MatchDefaultAndRemote)
        return mdb.mimeTypeForFile(filePath.path(), MimeDatabase::MatchExtension);

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
    QWriteLocker locker(&d->m_initMutex);
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

void visitMimeParents(const MimeType &mimeType,
                      const std::function<bool(const MimeType &mimeType)> &visitor)
{
    // search breadth-first through parent hierarchy, e.g. for hierarchy
    // * application/x-ruby
    //     * application/x-executable
    //         * application/octet-stream
    //     * text/plain
    QList<MimeType> queue;
    QSet<QString> seen;
    queue.append(mimeType);
    seen.insert(mimeType.name());
    while (!queue.isEmpty()) {
        const MimeType mt = queue.takeFirst();
        if (!visitor(mt))
            break;
        // add parent mime types
        const QStringList parentNames = mt.parentMimeTypes();
        for (const QString &parentName : parentNames) {
            const MimeType parent = mimeTypeForName(parentName);
            if (parent.isValid()) {
                int seenSize = seen.size();
                seen.insert(parent.name());
                if (seen.size() != seenSize) // not seen before, so add
                    queue.append(parent);
            }
        }
    }
}

/*!
    The \a init function will be executed once after the MIME database is first initialized.
    It must be thread safe.
*/
void addMimeInitializer(const std::function<void()> &init)
{
    auto d = MimeDatabasePrivate::instance();
    d->addInitializer(init);
}

} // namespace Utils
