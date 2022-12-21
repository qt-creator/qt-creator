// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QWizardPage>

namespace Utils { class FilePath; }

namespace Designer {

class FormClassWizardParameters;
class FormClassWizardGenerationParameters;

namespace Internal {

class NewClassWidget;

class FormClassWizardPage : public QWizardPage
{
public:
    FormClassWizardPage();
    ~FormClassWizardPage() override;

    bool isComplete () const override;
    bool validatePage() override;

    void setClassName(const QString &suggestedClassName);

    void setFilePath(const Utils::FilePath &);
    Utils::FilePath filePath() const;

    // Fill out applicable parameters
    void getParameters(FormClassWizardParameters *) const;

    FormClassWizardGenerationParameters generationParameters() const;
    void setGenerationParameters(const FormClassWizardGenerationParameters &gp);

    static bool lowercaseHeaderFiles();

private:
    void slotValidChanged();

    bool m_isValid = false;
    Designer::Internal::NewClassWidget *m_newClassWidget;
};

} // namespace Internal
} // namespace Designer
