// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtwizard.h"

namespace QmakeProjectManager::Internal {

class SubdirsProjectWizard final : public QtWizard
{
public:
    SubdirsProjectWizard();

private:
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const final;

    Utils::Result<Core::GeneratedFiles> generateFiles(const QWizard *w) const final;
    Utils::Result<> postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l) const final;
};

} // namespace QmakeProjectManager::Internal
