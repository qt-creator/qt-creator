// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/basefilewizardfactory.h>

namespace ProjectExplorer::Internal {

class SimpleProjectWizard : public Core::BaseFileWizardFactory
{
public:
    SimpleProjectWizard();

private:
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const override;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    Utils::Result<> postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l) const override;
};

} // namespace ProjectExplorer::Internal
