// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/fileutils.h>
#include <utils/wizard.h>

namespace Utils { class FileWizardPage; }

namespace GenericProjectManager {
namespace Internal {

class FilesSelectionWizardPage;

class GenericProjectWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit GenericProjectWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = nullptr);

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &path);
    Utils::FilePaths selectedFiles() const;
    Utils::FilePaths selectedPaths() const;

    QString projectName() const;

    Utils::FileWizardPage *m_firstPage;
    FilesSelectionWizardPage *m_secondPage;
};

class GenericProjectWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    GenericProjectWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;
};

} // namespace Internal
} // namespace GenericProjectManager
