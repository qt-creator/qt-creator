// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesselectionwizardpage.h"

#include "genericprojectconstants.h"
#include "genericprojectmanagertr.h"
#include "genericprojectwizard.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/wizard.h>

#include <QVBoxLayout>

namespace GenericProjectManager {
namespace Internal {

FilesSelectionWizardPage::FilesSelectionWizardPage(GenericProjectWizardDialog *genericProjectWizard,
                                                   QWidget *parent) :
    QWizardPage(parent),
    m_genericProjectWizardDialog(genericProjectWizard),
    m_filesWidget(new ProjectExplorer::SelectableFilesWidget(this))
{
    auto layout = new QVBoxLayout(this);

    layout->addWidget(m_filesWidget);
    m_filesWidget->enableFilterHistoryCompletion
            (ProjectExplorer::Constants::ADD_FILES_DIALOG_FILTER_HISTORY_KEY);
    m_filesWidget->setBaseDirEditable(false);
    connect(m_filesWidget, &ProjectExplorer::SelectableFilesWidget::selectedFilesChanged,
            this, &FilesSelectionWizardPage::completeChanged);

    setProperty(Utils::SHORT_TITLE_PROPERTY, Tr::tr("Files"));
}

void FilesSelectionWizardPage::initializePage()
{
    m_filesWidget->resetModel(m_genericProjectWizardDialog->filePath(), Utils::FilePaths());
}

void FilesSelectionWizardPage::cleanupPage()
{
    m_filesWidget->cancelParsing();
}

bool FilesSelectionWizardPage::isComplete() const
{
    return m_filesWidget->hasFilesSelected();
}

Utils::FilePaths FilesSelectionWizardPage::selectedPaths() const
{
    return m_filesWidget->selectedPaths();
}

Utils::FilePaths FilesSelectionWizardPage::selectedFiles() const
{
    return m_filesWidget->selectedFiles();
}

} // namespace Internal
} // namespace GenericProjectManager
