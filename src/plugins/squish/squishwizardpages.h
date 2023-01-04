// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizardgeneratorfactory.h>
#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>
#include <utils/wizardpage.h>

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }

namespace Squish {
namespace Internal {

class SquishToolkitsPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    SquishToolkitsPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class SquishToolkitsPage : public Utils::WizardPage
{
    Q_OBJECT
public:
    SquishToolkitsPage();
    ~SquishToolkitsPage() = default;

    void initializePage() override;
    bool isComplete() const override;
    bool handleReject() override;

private:
    void delayedInitialize();
    void fetchServerSettings();

    QButtonGroup *m_buttonGroup = nullptr;
    QLineEdit *m_hiddenLineEdit = nullptr;
    Utils::InfoLabel *m_errorLabel = nullptr;
};

class SquishScriptLanguagePageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    SquishScriptLanguagePageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class SquishScriptLanguagePage : public Utils::WizardPage
{
    Q_OBJECT
public:
    SquishScriptLanguagePage();
    ~SquishScriptLanguagePage() = default;
};

class SquishAUTPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    SquishAUTPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class SquishAUTPage : public Utils::WizardPage
{
    Q_OBJECT
public:
    SquishAUTPage();
    ~SquishAUTPage() = default;

    void initializePage() override;
private:
    QComboBox *m_autCombo = nullptr;
};

class SquishGeneratorFactory : public ProjectExplorer::JsonWizardGeneratorFactory
{
public:
    SquishGeneratorFactory();

    ProjectExplorer::JsonWizardGenerator *create(Utils::Id typeId, const QVariant &data,
                                                 const QString &path, Utils::Id platform,
                                                 const QVariantMap &variables) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class SquishFileGenerator : public ProjectExplorer::JsonWizardGenerator
{
public:
    bool setup(const QVariant &data, QString *errorMessage);
    Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                  const Utils::FilePath &wizardDir,
                                  const Utils::FilePath &projectDir,
                                  QString *errorMessage) override;
    bool writeFile(const ProjectExplorer::JsonWizard *wizard, Core::GeneratedFile *file,
                   QString *errorMessage) override;
    bool allDone(const ProjectExplorer::JsonWizard *wizard, Core::GeneratedFile *file,
                 QString *errorMessage) override;

private:
    QString m_mode;
};

} // namespace Internal
} // namespace Squish
