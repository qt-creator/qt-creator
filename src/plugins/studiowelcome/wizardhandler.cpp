/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QWizardPage>
#include <QMessageBox>

#include "wizardhandler.h"

#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonfieldpage_p.h>

#include <projectexplorer/jsonwizard/jsonprojectpage.h>

#include "utils/wizard.h"
#include <utils/qtcassert.h>

using namespace StudioWelcome;

void WizardHandler::reset(const ProjectItem &projectInfo, int projectSelection, const Utils::FilePath &location)
{
    m_projectItem = projectInfo;
    m_projectLocation = location;
    m_selectedProject = projectSelection;

    if (!m_wizard) {
        setupWizard();
    } else {
        QObject::connect(m_wizard, &QObject::destroyed, this, &WizardHandler::onWizardResetting);

        // DON'T SET `m_selectedProject = -1` --- we are switching now to a separate project.
        emit deletingWizard();

        m_wizard->deleteLater();
    }
}

void WizardHandler::destroyWizard()
{
    emit deletingWizard();

    m_selectedProject = -1;
    m_wizard->deleteLater();
    m_wizard = nullptr;
    m_detailsPage = nullptr;
}

void WizardHandler::setupWizard()
{
    m_wizard = m_projectItem.create(m_projectLocation);
    if (!m_wizard) {
        emit wizardCreationFailed();
        return;
    }

    initializeProjectPage(m_wizard->page(0));
    initializeFieldsPage(m_wizard->page(1));

    auto *screenFactorModel = getScreenFactorModel(m_detailsPage);
    auto *styleModel = getStyleModel(m_detailsPage);

    emit wizardCreated(screenFactorModel, styleModel);
}

void WizardHandler::setProjectName(const QString &name)
{
    QTC_ASSERT(m_wizard, return);

    QWizardPage *projectPage = m_wizard->page(0);
    auto *jpp = dynamic_cast<ProjectExplorer::JsonProjectPage *>(projectPage);
    QTC_ASSERT(jpp, return);

    jpp->setProjectName(name);
}

void WizardHandler::setProjectLocation(const Utils::FilePath &location)
{
    QTC_ASSERT(m_wizard, return);

    QWizardPage *projectPage = m_wizard->page(0);
    auto *jpp = dynamic_cast<ProjectExplorer::JsonProjectPage *>(projectPage);
    QTC_ASSERT(jpp, return);

    jpp->setFilePath(location);
}

void WizardHandler::initializeProjectPage(QWizardPage *page)
{
    auto *jpp = dynamic_cast<ProjectExplorer::JsonProjectPage *>(page);
    QTC_ASSERT(jpp, return);

    QObject::connect(jpp, &ProjectExplorer::JsonProjectPage::statusMessageChanged, this, &WizardHandler::statusMessageChanged);
    QObject::connect(jpp, &ProjectExplorer::JsonProjectPage::completeChanged, this, &WizardHandler::onProjectIntroCompleteChanged);
}

void WizardHandler::initializeFieldsPage(QWizardPage *page)
{
    auto fieldsPage = dynamic_cast<ProjectExplorer::JsonFieldPage *>(page); // required for page->jsonField
    QTC_ASSERT(fieldsPage, return);
    m_detailsPage = fieldsPage;

    fieldsPage->initializePage();
}

void WizardHandler::onProjectIntroCompleteChanged()
{
    auto *page = dynamic_cast<ProjectExplorer::JsonProjectPage *>(QObject::sender());
    QTC_ASSERT(page, return);

    emit projectCanBeCreated(page->isComplete());
}

QStandardItemModel *WizardHandler::getScreenFactorModel(ProjectExplorer::JsonFieldPage *page)
{
    auto *field = page->jsonField("ScreenFactor");
    if (!field)
        return nullptr;

    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return nullptr);

    return cbfield->model();
}

QStandardItemModel *WizardHandler::getStyleModel(ProjectExplorer::JsonFieldPage *page)
{
    auto *field = page->jsonField("ControlsStyle");
    if (!field)
        return nullptr;

    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField*>(field);
    QTC_ASSERT(cbfield, return nullptr);

    return cbfield->model();
}

void WizardHandler::onWizardResetting()
{
    m_wizard = nullptr;

    // if have a wizard request pending => create new wizard
    // note: we always have a wizard request pending here, unless the dialogbox was requested to be destroyed.
    // if m_selectedProject != -1 => the wizard was destroyed as a result of reset to a different project type
    if (m_selectedProject > -1)
        setupWizard();
}

void WizardHandler::setScreenSizeIndex(int index)
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return);

    cbfield->selectRow(index);
}

int WizardHandler::screenSizeIndex() const
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    return cbfield->selectedRow();
}

void WizardHandler::setTargetQtVersionIndex(int index)
{
    auto *field = m_detailsPage->jsonField("TargetQtVersion");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return);

    cbfield->selectRow(index);
}

bool WizardHandler::haveTargetQtVersion() const
{
    return m_wizard->hasField("TargetQtVersion");
}

void WizardHandler::setStyleIndex(int index)
{
    auto *field = m_detailsPage->jsonField("ControlsStyle");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return);

    cbfield->selectRow(index);
}

int WizardHandler::styleIndex() const
{
    auto *field = m_detailsPage->jsonField("ControlsStyle");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    return cbfield->selectedRow();
}

void WizardHandler::setUseVirtualKeyboard(bool value)
{
    auto *field = m_detailsPage->jsonField("UseVirtualKeyboard");
    auto *cbfield = dynamic_cast<ProjectExplorer::CheckBoxField *>(field);
    QTC_ASSERT(cbfield, return);

    cbfield->setChecked(value);
}

bool WizardHandler::haveVirtualKeyboard() const
{
    return m_wizard->hasField("UseVirtualKeyboard");
}

void WizardHandler::run(const std::function<void(QWizardPage *)> &processPage)
{
    m_wizard->restart();

    int nextId = 0;
    do {
        QWizardPage *page = m_wizard->currentPage();
        QTC_ASSERT(page, return);

        processPage(page);

        if (!page->validatePage() || !page->isComplete()) {
            QMessageBox::warning(m_wizard, "New project", "Could not create the project because fields are invalid");
            return;
        }

        nextId = m_wizard->nextId();
        m_wizard->next();
    } while (-1 != nextId);

    m_selectedProject = -1;

    // Note: don't call `emit deletingWizard()` here.

    // Note: QWizard::accept calls QObject::deleteLater on the wizard
    m_wizard->accept();
}
