// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "formclasswizardparameters.h"

#include <coreplugin/basefilewizardfactory.h>

namespace Designer {
namespace Internal {

class FormClassWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    FormClassWizard();

    QString headerSuffix() const;
    QString sourceSuffix() const;
    QString formSuffix() const;

private:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const final;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const final;
};

} // namespace Internal
} // namespace Designer
