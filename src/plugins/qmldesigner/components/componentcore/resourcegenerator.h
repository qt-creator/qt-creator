// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <qmldesignercomponents_global.h>

namespace QmlDesigner::ResourceGenerator {

QMLDESIGNERCOMPONENTS_EXPORT void generateMenuEntry(QObject *parent);
QMLDESIGNERCOMPONENTS_EXPORT QStringList getProjectFileList();
QMLDESIGNERCOMPONENTS_EXPORT bool createQrcFile(const Utils::FilePath &qrcFilePath);
QMLDESIGNERCOMPONENTS_EXPORT bool createQmlrcFile(const Utils::FilePath &qmlrcFilePath);

} // namespace QmlDesigner::ResourceGenerator
