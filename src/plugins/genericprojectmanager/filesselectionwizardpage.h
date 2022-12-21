// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QWizardPage>

namespace ProjectExplorer { class SelectableFilesWidget; }

namespace GenericProjectManager {
namespace Internal {

class GenericProjectWizardDialog;

class FilesSelectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    FilesSelectionWizardPage(GenericProjectWizardDialog *genericProjectWizard, QWidget *parent = nullptr);
    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;
    Utils::FilePaths selectedFiles() const;
    Utils::FilePaths selectedPaths() const;

private:
    GenericProjectWizardDialog *m_genericProjectWizardDialog;
    ProjectExplorer::SelectableFilesWidget *m_filesWidget;
};

} // namespace Internal
} // namespace GenericProjectManager
