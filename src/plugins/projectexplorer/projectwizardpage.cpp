/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectwizardpage.h"
#include "ui_projectwizardpage.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QDir>
#include <QTextStream>

/*!
    \class ProjectExplorer::Internal::ProjectWizardPage

    \brief Wizard page showing projects and version control to add new files to.

    \sa ProjectExplorer::Internal::ProjectFileWizardExtension
*/

using namespace ProjectExplorer;
using namespace Internal;

ProjectWizardPage::ProjectWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::WizardPage)
{
    m_ui->setupUi(this);
    connect(m_ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotProjectChanged(int)));
    connect(m_ui->vcsManageButton, SIGNAL(clicked()), this, SLOT(slotManageVcs()));
    setProperty("shortTitle", tr("Summary"));
}

ProjectWizardPage::~ProjectWizardPage()
{
    delete m_ui;
}

void ProjectWizardPage::setProjects(const QStringList &p)
{
    m_ui->projectComboBox->clear();
    m_ui->projectComboBox->addItems(p);
    m_ui->projectComboBox->setEnabled(p.size() > 1);
    m_ui->projectLabel->setEnabled(p.size() > 1);
}

void ProjectWizardPage::setProjectToolTips(const QStringList &t)
{
    m_projectToolTips = t;
}

void ProjectWizardPage::setAddingSubProject(bool addingSubProject)
{
    m_ui->projectLabel->setText(addingSubProject ?
                                    tr("Add as a subproject to project:")
                                  : tr("Add to &project:"));
}

void ProjectWizardPage::setProjectComoBoxVisible(bool visible)
{
    m_ui->projectComboBox->setVisible(visible);
    m_ui->projectLabel->setVisible(visible);
}

int ProjectWizardPage::currentProjectIndex() const
{
    return m_ui->projectComboBox->currentIndex();
}

void ProjectWizardPage::setCurrentProjectIndex(int idx)
{
    m_ui->projectComboBox->setCurrentIndex(idx);
}

void ProjectWizardPage::setNoneLabel(const QString &label)
{
    m_ui->projectComboBox->setItemText(0, label);
}

void ProjectWizardPage::setAdditionalInfo(const QString &text)
{
    m_ui->additionalInfo->setText(text);
    m_ui->additionalInfo->setVisible(!text.isEmpty());
}

void ProjectWizardPage::setVersionControls(const QStringList &vcs)
{
    m_ui->addToVersionControlComboBox->clear();
    m_ui->addToVersionControlComboBox->addItems(vcs);
}

int ProjectWizardPage::versionControlIndex() const
{
    return m_ui->addToVersionControlComboBox->currentIndex();
}

void ProjectWizardPage::setVersionControlIndex(int idx)
{
    m_ui->addToVersionControlComboBox->setCurrentIndex(idx);
}

void ProjectWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

// Alphabetically, and files in sub-directories first
static bool generatedFilePathLessThan(const QString &filePath1, const QString &filePath2)
{
    const bool filePath1HasDir = filePath1.contains(QLatin1Char('/'));
    const bool filePath2HasDir = filePath2.contains(QLatin1Char('/'));

    if (filePath1HasDir == filePath2HasDir)
        return Utils::FileName::fromString(filePath1) < Utils::FileName::fromString(filePath2);
    else
        return filePath1HasDir;
}

void ProjectWizardPage::setFilesDisplay(const QString &commonPath, const QStringList &files)
{
    QString fileMessage;
    {
        QTextStream str(&fileMessage);
        str << "<qt>"
            << (commonPath.isEmpty() ? tr("Files to be added:") : tr("Files to be added in"))
            << "<pre>";

        QStringList formattedFiles;
        if (commonPath.isEmpty()) {
            formattedFiles = files;
        } else {
            str << QDir::toNativeSeparators(commonPath) << ":\n\n";
            const int prefixSize = commonPath.size() + 1;
            foreach (const QString &f, files)
                formattedFiles.append(f.right(f.size() - prefixSize));
        }
        qSort(formattedFiles.begin(), formattedFiles.end(), generatedFilePathLessThan);

        foreach (const QString &f, formattedFiles)
            str << QDir::toNativeSeparators(f) << '\n';

        str << "</pre>";
    }
    m_ui->filesLabel->setText(fileMessage);
}

void ProjectWizardPage::setProjectToolTip(const QString &tt)
{
    m_ui->projectComboBox->setToolTip(tt);
    m_ui->projectLabel->setToolTip(tt);
}

void ProjectWizardPage::slotProjectChanged(int index)
{
    setProjectToolTip(index >= 0 && index < m_projectToolTips.size() ?
                      m_projectToolTips.at(index) : QString());
}

void ProjectWizardPage::slotManageVcs()
{
    Core::ICore::showOptionsDialog(VcsBase::Constants::VCS_SETTINGS_CATEGORY,
                                   VcsBase::Constants::VCS_COMMON_SETTINGS_ID);
}
