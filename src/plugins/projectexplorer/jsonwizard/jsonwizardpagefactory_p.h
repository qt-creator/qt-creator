// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonwizardpagefactory.h"

namespace ProjectExplorer {
namespace Internal {

class FieldPageFactory : public JsonWizardPageFactory
{
public:
    FieldPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class FilePageFactory : public JsonWizardPageFactory
{
public:
    FilePageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class KitsPageFactory : public JsonWizardPageFactory
{
public:
    KitsPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class ProjectPageFactory : public JsonWizardPageFactory
{
public:
    ProjectPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class SummaryPageFactory : public JsonWizardPageFactory
{
public:
    SummaryPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

} // namespace Internal
} // namespace ProjectExplorer
