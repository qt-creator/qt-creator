// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fsenginehandler.h"

#include "fixedlistfsengine.h"
#include "fsengine_impl.h"
#include "rootinjectfsengine.h"

#include "fsengine.h"

#include "../algorithm.h"
#include "../hostosinfo.h"

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

static bool isRootPath(const QString &fileName)
{
    if (HostOsInfo::isWindowsHost()) {
        static const QChar lowerDriveLetter = HostOsInfo::root().path().front().toUpper();
        static const QChar upperDriveLetter = HostOsInfo::root().path().front().toLower();
        return fileName.size() == 3 && fileName[1] == ':' && fileName[2] == '/'
               && (fileName[0] == lowerDriveLetter || fileName[0] == upperDriveLetter);
     }

     return fileName.size() == 1 && fileName[0] == '/';
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
std::unique_ptr<QAbstractFileEngine>
#else
QAbstractFileEngine *
#endif
FSEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(':'))
        return {};

    static const QString rootPath = FilePath::specialRootPath();
    static const FilePath rootFilePath = FilePath::fromString(rootPath);

    const QString fixedFileName = QDir::cleanPath(fileName);

    if (fixedFileName == rootPath) {
        const FilePaths paths
            = Utils::transform(FSEngine::registeredDeviceSchemes(), [](const QString &scheme) {
                  return rootFilePath.pathAppended(scheme);
              });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName), paths);
#else
        return new FixedListFSEngine(removeDoubleSlash(fileName), paths);
#endif
    }

    if (fixedFileName.startsWith(rootPath)) {
        const QStringList deviceSchemes = FSEngine::registeredDeviceSchemes();
        for (const QString &scheme : deviceSchemes) {
            if (fixedFileName == rootFilePath.pathAppended(scheme).toString()) {
                const FilePaths filteredRoots = Utils::filtered(FSEngine::registeredDeviceRoots(),
                                                                [scheme](const FilePath &root) {
                                                                    return root.scheme() == scheme;
                                                                });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
                return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName),
                                                           filteredRoots);
#else
                return new FixedListFSEngine(removeDoubleSlash(fileName), filteredRoots);
#endif
            }
        }

        FilePath fixedPath = FilePath::fromString(fixedFileName);

        if (fixedPath.needsDevice()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            return std::make_unique<FSEngineImpl>(removeDoubleSlash(fileName));
#else
            return new FSEngineImpl(removeDoubleSlash(fileName));
#endif
        }
    }

    if (isRootPath(fixedFileName)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        return std::make_unique<RootInjectFSEngine>(fileName);
#else
        return new RootInjectFSEngine(fileName);
#endif
    }

    return {};
}

} // Utils::Internal
