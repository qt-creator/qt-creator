// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

namespace Beautifier::Internal {

class UncrustifySettings : public AbstractSettings
{
public:
    UncrustifySettings();

    void createDocumentationFile() const override;

    Utils::BoolAspect useOtherFiles{this};
    Utils::BoolAspect useHomeFile{this};
    Utils::BoolAspect useCustomStyle{this};

    Utils::StringAspect customStyle{this};
    Utils::BoolAspect formatEntireFileFallback{this};

    Utils::FilePathAspect specificConfigFile{this};
    Utils::BoolAspect useSpecificConfigFile{this};
};

class UncrustifyOptionsPage final : public Core::IOptionsPage
{
public:
    explicit UncrustifyOptionsPage(UncrustifySettings *settings);
};

} // Beautifier::Internal
