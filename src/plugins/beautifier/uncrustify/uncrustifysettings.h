// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

namespace Beautifier {
namespace Internal {

class UncrustifySettings : public AbstractSettings
{
    Q_OBJECT

public:
    UncrustifySettings();
    ~UncrustifySettings() override;

    bool useOtherFiles() const;
    void setUseOtherFiles(bool useOtherFiles);

    bool useHomeFile() const;
    void setUseHomeFile(bool useHomeFile);

    bool useCustomStyle() const;
    void setUseCustomStyle(bool useCustomStyle);

    QString customStyle() const;
    void setCustomStyle(const QString &customStyle);

    bool formatEntireFileFallback() const;
    void setFormatEntireFileFallback(bool formatEntireFileFallback);

    QString documentationFilePath() const override;
    void createDocumentationFile() const override;

    Utils::FilePath specificConfigFile() const;
    void setSpecificConfigFile(const Utils::FilePath &filePath);

    bool useSpecificConfigFile() const;
    void setUseSpecificConfigFile(bool useConfigFile);
};

} // namespace Internal
} // namespace Beautifier
