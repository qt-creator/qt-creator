/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "filesselectionwizardpage.h"

#include "genericprojectwizard.h"
#include "genericprojectconstants.h"

#include <coreplugin/icore.h>
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
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(m_filesWidget);
    m_filesWidget->setBaseDirEditable(false);
    connect(m_filesWidget, &ProjectExplorer::SelectableFilesWidget::selectedFilesChanged,
            this, &FilesSelectionWizardPage::completeChanged);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Files"));
}

void FilesSelectionWizardPage::initializePage()
{
    m_filesWidget->resetModel(Utils::FileName::fromString(m_genericProjectWizardDialog->path()),
                              Utils::FileNameList());
}

void FilesSelectionWizardPage::cleanupPage()
{
    m_filesWidget->cancelParsing();
}

bool FilesSelectionWizardPage::isComplete() const
{
    return m_filesWidget->hasFilesSelected();
}

Utils::FileNameList FilesSelectionWizardPage::selectedPaths() const
{
    return m_filesWidget->selectedPaths();
}

Utils::FileNameList FilesSelectionWizardPage::selectedFiles() const
{
    return m_filesWidget->selectedFiles();
}

} // namespace Internal
} // namespace GenericProjectManager
