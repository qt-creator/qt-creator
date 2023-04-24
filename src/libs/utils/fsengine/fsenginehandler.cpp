// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fsenginehandler.h"

#include "fixedlistfsengine.h"
#include "fsengine_impl.h"
#include "rootinjectfsengine.h"

#include "fsengine.h"

#include "../algorithm.h"

namespace Utils::Internal {

static FilePath removeDoubleSlash(const QString &fileName)
{
    // Reduce every two or more slashes to a single slash.
    QString result;
    const QChar slash = QChar('/');
    bool lastWasSlash = false;
    for (const QChar &ch : fileName) {
        if (ch == slash) {
            if (!lastWasSlash)
                result.append(ch);
            lastWasSlash = true;
        } else {
            result.append(ch);
            lastWasSlash = false;
        }
    }
    // We use fromString() here to not normalize / clean the path anymore.
    return FilePath::fromString(result);
}

QAbstractFileEngine *FSEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(':'))
        return nullptr;

    static const QString rootPath = FilePath::specialRootPath();
    static const FilePath rootFilePath = FilePath::fromString(rootPath);

    const QString fixedFileName = QDir::cleanPath(fileName);

    if (fixedFileName == rootPath) {
        const FilePaths paths
            = Utils::transform(FSEngine::registeredDeviceSchemes(), [](const QString &scheme) {
                  return rootFilePath.pathAppended(scheme);
              });

        return new FixedListFSEngine(removeDoubleSlash(fileName), paths);
    }

    if (fixedFileName.startsWith(rootPath)) {
        const QStringList deviceSchemes = FSEngine::registeredDeviceSchemes();
        for (const QString &scheme : deviceSchemes) {
            if (fixedFileName == rootFilePath.pathAppended(scheme).toString()) {
                const FilePaths filteredRoots = Utils::filtered(FSEngine::registeredDeviceRoots(),
                                                                [scheme](const FilePath &root) {
                                                                    return root.scheme() == scheme;
                                                                });

                return new FixedListFSEngine(removeDoubleSlash(fileName), filteredRoots);
            }
        }

        FilePath fixedPath = FilePath::fromString(fixedFileName);

        if (fixedPath.needsDevice())
            return new FSEngineImpl(removeDoubleSlash(fileName));
    }

    if (fixedFileName.compare(QDir::rootPath(), Qt::CaseInsensitive) == 0)
        return new RootInjectFSEngine(fileName);

    return nullptr;
}

} // Utils::Internal
