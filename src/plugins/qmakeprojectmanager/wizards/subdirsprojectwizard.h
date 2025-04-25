// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

class SubdirsProjectWizard : public QtWizard
{
public:
    SubdirsProjectWizard();

private:
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    Utils::Result<> postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l) const override;
};

} // namespace Internal
} // namespace QmakeProjectManager
