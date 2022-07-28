/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    QString fixedFileName = fileName;

    if (fileName.startsWith("//"))
        fixedFileName = fixedFileName.mid(1);

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
    }

    FilePath filePath = FilePath::fromString(fixedFileName);
    if (filePath.needsDevice())
        return new FSEngineImpl(filePath);


    if (fixedFileName.compare(QDir::rootPath(), Qt::CaseInsensitive) == 0)
        return new RootInjectFSEngine(fixedFileName);

    return nullptr;
}

} // namespace Internal

} // namespace Utils
