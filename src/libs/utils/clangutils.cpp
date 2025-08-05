// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangutils.h"

#include "filepath.h"
#include "qtcprocess.h"
#include "utilstr.h"

#include <QVersionNumber>

namespace Utils {

static QVersionNumber getClangdVersion(const FilePath &clangdFilePath)
{
    Process clangdProc;
    clangdProc.setCommand({clangdFilePath, {"--version"}});
    clangdProc.runBlocking();
    if (clangdProc.result() != ProcessResult::FinishedWithSuccess)
        return {};
    const QString output = clangdProc.allOutput();
    static const QString versionPrefix = "clangd version ";
    const int prefixOffset = output.indexOf(versionPrefix);
    if (prefixOffset == -1)
        return {};
    return QVersionNumber::fromString(output.mid(prefixOffset + versionPrefix.length()));
}

QVersionNumber clangdVersion(const FilePath &clangd)
{
    static QHash<FilePath, QPair<QDateTime, QVersionNumber>> versionCache;
    const QDateTime timeStamp = clangd.lastModified();
    const auto it = versionCache.find(clangd);
    if (it == versionCache.end()) {
        const QVersionNumber version = getClangdVersion(clangd);
        versionCache.insert(clangd, {timeStamp, version});
        return version;
    }
    if (it->first != timeStamp) {
        it->first = timeStamp;
        it->second = getClangdVersion(clangd);
    }
    return it->second;
}

Result<> checkClangdVersion(const FilePath &clangd)
{
    if (clangd.isEmpty())
        return ResultError(Tr::tr("No clangd executable specified."));

    const QVersionNumber version = clangdVersion(clangd);
    if (version >= minimumClangdVersion())
        return ResultOk;

    return ResultError(version.isNull()
                     ? Tr::tr("Failed to retrieve clangd version: Unexpected clangd output.")
                     : Tr::tr("The clangd version is %1, but %2 or greater is required.")
                           .arg(version.toString())
                           .arg(minimumClangdVersion().majorVersion()));
}

QVersionNumber minimumClangdVersion()
{
    return QVersionNumber(17);
}

} // namespace Utils
