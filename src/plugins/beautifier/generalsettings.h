// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/mimeutils.h>

namespace Beautifier::Internal {

class GeneralSettings final : public Utils::AspectContainer
{
public:
    GeneralSettings();

    QList<Utils::MimeType> allowedMimeTypes() const;

    Utils::BoolAspect autoFormatOnSave{this};
    Utils::BoolAspect autoFormatOnlyCurrentProject{this};
    Utils::SelectionAspect autoFormatTools{this};
    Utils::StringAspect autoFormatMime{this};
};

GeneralSettings &generalSettings();

} // Beautifier::Internal
