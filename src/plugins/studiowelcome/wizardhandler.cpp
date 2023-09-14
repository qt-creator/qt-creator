// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QWizardPage>
#include <QMessageBox>

#include "wizardhandler.h"

#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonfieldpage_p.h>

#include <projectexplorer/jsonwizard/jsonprojectpage.h>

#include <utils/qtcassert.h>
#include <utils/wizard.h>

using namespace StudioWelcome;

void WizardHandler::reset(const std::shared_ptr<PresetItem> &presetInfo, int presetSelection)
{
    m_preset = presetInfo;
    m_selectedPreset = presetSelection;

    if (!m_wizard) {
        setupWizard();
    } else {
        QObject::connect(m_wizard, &QObject::destroyed, this, &WizardHandler::onWizardResetting);

        // DON'T SET `m_selectedPreset = -1` --- we are switching now to a separate preset.
        emit deletingWizard();

        m_wizard->deleteLater();
    }
}

void WizardHandler::destroyWizard()
{
    emit deletingWizard();

    m_selectedPreset = -1;
    m_wizard->deleteLater();
    m_wizard = nullptr;
    m_detailsPage = nullptr;
}

void WizardHandler::setupWizard()
{
    m_wizard = m_preset->create(m_projectLocation);
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

    QObject::connect(jpp, &ProjectExplorer::JsonProjectPage::statusMessageChanged,
                     this, &WizardHandler::statusMessageChanged);
    QObject::connect(jpp, &ProjectExplorer::JsonProjectPage::completeChanged,
                     this, [this, jpp] { emit projectCanBeCreated(jpp->isComplete()); });
}

void WizardHandler::initializeFieldsPage(QWizardPage *page)
{
    auto fieldsPage = dynamic_cast<ProjectExplorer::JsonFieldPage *>(page); // required for page->jsonField
    QTC_ASSERT(fieldsPage, return);
    m_detailsPage = fieldsPage;

    fieldsPage->initializePage();
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

bool WizardHandler::haveStyleModel() const
{
    return m_wizard->hasField("ControlsStyle");
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
    // if m_selectedPreset != -1 => the wizard was destroyed as a result of reset to a different preset type
    if (m_selectedPreset > -1)
        setupWizard();
}

void WizardHandler::setScreenSizeIndex(int index)
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return);

    cbfield->selectRow(index);
}

QString WizardHandler::screenSizeName(int index) const
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return "");

    QStandardItemModel *model = cbfield->model();
    if (index < 0 || index >= model->rowCount())
        return {};

    QString text = model->item(index)->text();
    return text;
}

int WizardHandler::screenSizeIndex() const
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    return cbfield->selectedRow();
}

int WizardHandler::screenSizeIndex(const QString &sizeName) const
{
    auto *field = m_detailsPage->jsonField("ScreenFactor");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return false);

    const QStandardItemModel *model = cbfield->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QStandardItem *item = model->item(i, 0);
        const QString text = item->text();

        if (text == sizeName)
            return i;
    }

    return -1;
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

QString WizardHandler::targetQtVersionName(int index) const
{
    auto *field = m_detailsPage->jsonField("TargetQtVersion");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return "");

    QStandardItemModel *model = cbfield->model();
    if (index < 0 || index >= model->rowCount())
        return {};

    QString text = model->item(index)->text();
    return text;
}

QStringList WizardHandler::targetQtVersionNames() const
{
    auto *field = m_detailsPage->jsonField("TargetQtVersion");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return {});

    QStandardItemModel *model = cbfield->model();
    QStringList targetVersions;

    for (int i = 0; i < model->rowCount(); ++i)
        targetVersions.append(model->item(i)->text());

    return targetVersions;
}

int WizardHandler::targetQtVersionIndex(const QString &qtVersionName) const
{
    auto *field = m_detailsPage->jsonField("TargetQtVersion");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    const QStandardItemModel *model = cbfield->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QStandardItem *item = model->item(i, 0);
        const QString text = item->text();

        if (text == qtVersionName)
            return i;
    }

    return -1;
}

int WizardHandler::targetQtVersionIndex() const
{
    auto *field = m_detailsPage->jsonField("TargetQtVersion");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    return cbfield->selectedRow();
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

int WizardHandler::styleIndex(const QString &styleName) const
{
    auto *field = m_detailsPage->jsonField("ControlsStyle");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return -1);

    const QStandardItemModel *model = cbfield->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QStandardItem *item = model->item(i, 0);
        const QString text = item->text();

        if (text == styleName)
            return i;
    }

    return -1;
}

QString WizardHandler::styleName(int index) const
{
    auto *field = m_detailsPage->jsonField("ControlsStyle");
    auto *cbfield = dynamic_cast<ProjectExplorer::ComboBoxField *>(field);
    QTC_ASSERT(cbfield, return "");

    QStandardItemModel *model = cbfield->model();
    if (index < 0 || index >= model->rowCount())
        return {};

    QString text = model->item(index)->text();
    return text;
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

    m_selectedPreset = -1;

    // Note: don't call `emit deletingWizard()` here.

    // Note: QWizard::accept calls QObject::deleteLater on the wizard
    m_wizard->accept();
}
