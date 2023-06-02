// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

namespace Beautifier::Internal {

class ArtisticStyleSettings : public AbstractSettings
{
public:
    ArtisticStyleSettings();

    Utils::BoolAspect useOtherFiles{this};
    Utils::BoolAspect useSpecificConfigFile{this};
    Utils::FilePathAspect specificConfigFile{this};
    Utils::BoolAspect useHomeFile{this};
    Utils::BoolAspect useCustomStyle{this};
    Utils::StringAspect customStyle{this};

    void createDocumentationFile() const override;
};

class ArtisticStyleOptionsPage final : public Core::IOptionsPage
{
public:
    explicit ArtisticStyleOptionsPage(ArtisticStyleSettings *settings);
};

} // Beautifier::Internal
