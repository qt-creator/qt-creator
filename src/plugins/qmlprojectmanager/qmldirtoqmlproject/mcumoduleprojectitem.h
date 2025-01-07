// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlprojectmanager_global.h"

#include <utils/filepath.h>

#include <QJsonObject>

#ifndef UNIT_TESTS
#  define MCUMODULEPROJEC_EXPORT QMLPROJECTMANAGER_EXPORT
#else
#  define MCUMODULEPROJEC_EXPORT
#endif

namespace QmlProjectManager {
class MCUMODULEPROJEC_EXPORT McuModuleProjectItem
{
public:
    explicit McuModuleProjectItem(const QJsonObject &project);
    explicit McuModuleProjectItem(const Utils::FilePath &qmlprojectFile);

    static std::optional<McuModuleProjectItem> fromQmldirModule(const Utils::FilePath &qmldirFile);

    bool isValid() const noexcept;

    QString uri() const noexcept;
    void setUri(const QString &moduleUri);

    QStringList qmlFiles() const noexcept;
    void setQmlFiles(const QStringList &files);

    Utils::FilePath qmlProjectPath() const noexcept;
    void setQmlProjectPath(const Utils::FilePath &path);

    QJsonObject project() const noexcept;

    bool saveQmlProjectFile() const;

    bool operator==(const McuModuleProjectItem &other) const noexcept;

private:
    QByteArray jsonToQmlproject() const;

    Utils::FilePath m_qmlProjectFile;
    QJsonObject m_project;
};
} // namespace QmlProjectManager
