// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

#include <utils/fileutils.h>

namespace Beautifier {
namespace Internal {

class ArtisticStyleSettings : public AbstractSettings
{
    Q_OBJECT

public:
    ArtisticStyleSettings();

    bool useOtherFiles() const;
    void setUseOtherFiles(bool useOtherFiles);

    bool useSpecificConfigFile() const;
    void setUseSpecificConfigFile(bool useSpecificConfigFile);

    Utils::FilePath specificConfigFile() const;
    void setSpecificConfigFile(const Utils::FilePath &specificConfigFile);

    bool useHomeFile() const;
    void setUseHomeFile(bool useHomeFile);

    bool useCustomStyle() const;
    void setUseCustomStyle(bool useCustomStyle);

    QString customStyle() const;
    void setCustomStyle(const QString &customStyle);

    QString documentationFilePath() const override;
    void createDocumentationFile() const override;
};

} // namespace Internal
} // namespace Beautifier
