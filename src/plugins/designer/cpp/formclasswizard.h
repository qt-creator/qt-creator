// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/basefilewizardfactory.h>

namespace Designer::Internal {

class FormClassWizard final : public Core::BaseFileWizardFactory
{
public:
    FormClassWizard();

    QString headerSuffix() const;
    QString sourceSuffix() const;
    QString formSuffix() const;

private:
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const final;

    Utils::Result<Core::GeneratedFiles> generateFiles(const QWizard *w) const final;
};

} // namespace Designer::Internal
