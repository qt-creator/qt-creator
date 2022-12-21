// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/basefilewizard.h>

namespace Designer {

class FormClassWizardParameters;

namespace Internal {

class FormClassWizardPage;
class FormTemplateWizardPage;

class FormClassWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    typedef QList<QWizardPage *> WizardPageList;

    explicit FormClassWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = nullptr);

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &);

    Designer::FormClassWizardParameters parameters() const;

protected:
    void initializePage(int id) final;

private:
    FormTemplateWizardPage *m_formPage = nullptr;
    FormClassWizardPage *m_classPage = nullptr;
    QString m_rawFormTemplate;
};

} // namespace Internal
} // namespace Designer
