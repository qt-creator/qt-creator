// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Docker::Internal {

class ContainerToolSettings : public Utils::AspectContainer
{
public:
    ContainerToolSettings(Utils::Id typeId,
                          const QString &scheme,
                          const QString &displayType,
                          const Utils::FilePaths &additionalBinaryPaths);

    Utils::FilePathAspect binaryPath{this};

    Utils::Id typeId() const { return m_typeId; }
    QString scheme() const { return m_scheme; }
    QString displayType() const { return m_displayType; }

private:
    Utils::Id m_typeId;
    QString m_scheme;
    QString m_displayType;
};

ContainerToolSettings &dockerSettings();
ContainerToolSettings &podmanSettings();

} // namespace Docker::Internal
