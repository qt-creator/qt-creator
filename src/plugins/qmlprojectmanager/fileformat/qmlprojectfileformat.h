// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QString>

namespace QmlProjectManager {

class QmlProjectItem;

class QmlProjectFileFormat
{
    Q_DECLARE_TR_FUNCTIONS(QmlProjectManager::QmlProjectFileFormat)

public:
    static std::unique_ptr<QmlProjectItem> parseProjectFile(const Utils::FilePath &fileName,
                                                            QString *errorMessage = nullptr);
};

} // namespace QmlProjectManager
