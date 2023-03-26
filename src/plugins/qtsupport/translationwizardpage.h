// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

namespace QtSupport {
namespace Internal {

class TranslationWizardPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    TranslationWizardPageFactory();

private:
    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id, const QVariant &, QString *) override { return true; }
};

} // namespace Internal
} // namespace QtSupport
