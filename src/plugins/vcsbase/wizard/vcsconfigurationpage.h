// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../vcsbase_global.h"

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/wizardpage.h>

namespace Core { class IVersionControl; }

namespace VcsBase {

namespace Internal {
class VcsConfigurationPagePrivate;

class VcsConfigurationPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    VcsConfigurationPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

} // namespace Internal

class VCSBASE_EXPORT VcsConfigurationPage : public Utils::WizardPage
{
public:
    VcsConfigurationPage();
    ~VcsConfigurationPage() override;

    void setVersionControl(const Core::IVersionControl *vc);
    void setVersionControlId(const QString &id);

    void initializePage() override;
    bool isComplete() const override;

private:
    void openConfiguration();

    Internal::VcsConfigurationPagePrivate *const d;
};

} // namespace VcsBase
