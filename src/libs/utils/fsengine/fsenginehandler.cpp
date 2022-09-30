// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "fsenginehandler.h"

#include "fixedlistfsengine.h"
#include "fsengine_impl.h"
#include "rootinjectfsengine.h"

#include "fsengine.h"

#include "../algorithm.h"

namespace Utils {

namespace Internal {

QAbstractFileEngine *FSEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(':'))
        return nullptr;

    static const QString rootPath =
        FilePath::specialPath(FilePath::SpecialPathComponent::RootPath);
    static const FilePath rootFilePath =
        FilePath::specialFilePath(FilePath::SpecialPathComponent::RootPath);

    const QString fixedFileName = QDir::cleanPath(fileName);

    if (fixedFileName == rootPath) {
        const FilePaths paths
            = Utils::transform(FSEngine::registeredDeviceSchemes(), [](const QString &scheme) {
                  return rootFilePath.pathAppended(scheme);
              });

        return new FixedListFSEngine(rootFilePath, paths);
    }

    if (fixedFileName.startsWith(rootPath)) {
        const QStringList deviceSchemes = FSEngine::registeredDeviceSchemes();
        for (const QString &scheme : deviceSchemes) {
            if (fixedFileName == rootFilePath.pathAppended(scheme).toString()) {
                const FilePaths filteredRoots = Utils::filtered(FSEngine::deviceRoots(),
                                                                [scheme](const FilePath &root) {
                                                                    return root.scheme() == scheme;
                                                                });

                return new FixedListFSEngine(rootFilePath.pathAppended(scheme), filteredRoots);
            }
        }

        FilePath filePath = FilePath::fromString(fixedFileName);
        if (filePath.needsDevice())
            return new FSEngineImpl(filePath);
    }

    if (fixedFileName.compare(QDir::rootPath(), Qt::CaseInsensitive) == 0)
        return new RootInjectFSEngine(fixedFileName);

    return nullptr;
}

} // namespace Internal

} // namespace Utils
