// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../wizards/qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

class CustomWidgetWizard : public QtWizard
{
    Q_OBJECT

public:
    CustomWidgetWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent,
                                 const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
};

} // namespace Internal
} // namespace QmakeProjectManager
