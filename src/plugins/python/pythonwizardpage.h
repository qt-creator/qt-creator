// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    PythonWizardPage();
    void initializePage() override;
    bool validatePage() override;

    void addPySideVersions(const QString &name, const QVariant &data);
    void setDefaultPySideVersions(int index);

private:
    void setupProject(const ProjectExplorer::JsonWizard::GeneratorFiles &files);
    void updateInterpreters();

    ProjectExplorer::InterpreterAspect m_interpreter;
    Utils::SelectionAspect m_pySideVersion;
};

} // namespace Python::Internal
