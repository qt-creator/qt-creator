// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizardgeneratorfactory.h>
#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>
#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Core;

namespace QmlProjectManager {


class QmlModuleWizardFieldPage final : public JsonFieldPage
{
public:
    QmlModuleWizardFieldPage(Utils::MacroExpander *expander, QWidget *parent = nullptr);
    void initializePage() override;
};


class QmlModuleWizardFieldPageFactory final : public JsonWizardPageFactory
{
public:
    QmlModuleWizardFieldPageFactory();

    WizardPage *create(JsonWizard *wizard, Id typeId, const QVariant &data) final;
    Utils::Result<> validateData(Id typeId, const QVariant &data) final;
};


class QmlModuleWizardGenerator final : public JsonWizardGenerator
{
public:
    Utils::Result<> setup(const QVariant &data);

    Core::GeneratedFiles fileList(
        Utils::MacroExpander *expander,
        const Utils::FilePath &wizardDir, const Utils::FilePath &projectDir,
        QString *errorMessage) override;

    Utils::Result<> writeFile(const JsonWizard *wizard, Core::GeneratedFile *file) override;
    Utils::Result<> allDone(const JsonWizard *wizard, Core::GeneratedFile *file) override;

private:
    Utils::FilePath m_source;
    Utils::FilePath m_target;
};


void setupQmlModuleWizard();

} // namespace QmlProjectManager
