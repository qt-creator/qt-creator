// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <coreplugin/dialogs/ioptionspage.h>

namespace Beautifier::Internal {

class GeneralSettings : public Core::PagedSettings
{
public:
    GeneralSettings();

    static GeneralSettings *instance();

    QList<Utils::MimeType> allowedMimeTypes() const;

    Utils::BoolAspect autoFormatOnSave;
    Utils::BoolAspect autoFormatOnlyCurrentProject;
    Utils::SelectionAspect autoFormatTools;
    Utils::StringAspect autoFormatMime;
};

} // Beautifier::Internal
