// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmlprojectmanager/qmlprojectmanager_global.h>

#include <utils/filepath.h>

namespace QmlProjectManager {

QMLPROJECTMANAGER_EXPORT QStringList imageFiles(
    const std::function<QString(QString)> &transformer = nullptr);

QMLPROJECTMANAGER_EXPORT bool isQmlFile(const Utils::FilePath &path);
QMLPROJECTMANAGER_EXPORT bool isAssetFile(const Utils::FilePath &path);
QMLPROJECTMANAGER_EXPORT bool isResource(const Utils::FilePath &path);

} // namespace QmlProjectManager.
