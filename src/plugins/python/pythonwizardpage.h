// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/aspects.h>
#include <utils/wizardpage.h>

namespace Python::Internal {

class PythonWizardPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    PythonWizardPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard,
                              Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class PythonWizardPage : public Utils::WizardPage
{
public:
    PythonWizardPage(const QList<QPair<QString, QVariant>> &pySideAndData, const int defaultPyside);
    bool validatePage() override;

private:
    Utils::SelectionAspect m_pySideVersion;
};

} // namespace Python::Internal
