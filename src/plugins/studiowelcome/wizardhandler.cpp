// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wizardhandler.h"
#include "fieldhelper.h"

#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonfieldpage_p.h>
#include <projectexplorer/jsonwizard/jsonprojectpage.h>

#include <utils/qtcassert.h>
#include <utils/wizard.h>

#include <QMessageBox>
#include <QWizardPage>

using namespace StudioWelcome;
using namespace StudioWelcome::FieldHelper;

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
    if (m_wizard) {
        m_wizard->deleteLater();
        m_wizard = nullptr;
    }
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
    if (!m_detailsPage) {
        emit wizardCreationFailed();
        return;
    }
    auto *screenFactorModel = getScreenFactorModel();
    auto *styleModel = getStyleModel();

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

QStandardItemModel *WizardHandler::getScreenFactorModel()
{
    return ComboBoxHelper(m_detailsPage, "ScreenFactor").model();
}

bool WizardHandler::haveStyleModel() const
{
    return m_wizard->hasField("ControlsStyle");
}

QStandardItemModel *WizardHandler::getStyleModel()
{
    return ComboBoxHelper(m_detailsPage, "ControlsStyle").model();
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
    ComboBoxHelper(m_detailsPage, "ScreenFactor").selectIndex(index);
}

QString WizardHandler::screenSizeName(int index) const
{
    return ComboBoxHelper(m_detailsPage, "ScreenFactor").text(index);
}

int WizardHandler::screenSizeIndex() const
{
    return ComboBoxHelper(m_detailsPage, "ScreenFactor").selectedIndex();
}

int WizardHandler::screenSizeIndex(const QString &sizeName) const
{
    return ComboBoxHelper(m_detailsPage, "ScreenFactor").indexOf(sizeName);
}

void WizardHandler::setTargetQtVersionIndex(int index)
{
    ComboBoxHelper(m_detailsPage, "TargetQtVersion").selectIndex(index);
}

bool WizardHandler::haveTargetQtVersion() const
{
    return m_wizard->hasField("TargetQtVersion");
}

QString WizardHandler::targetQtVersionName(int index) const
{
    return ComboBoxHelper(m_detailsPage, "TargetQtVersion").text(index);
}

QStringList WizardHandler::targetQtVersionNames() const
{
    return ComboBoxHelper(m_detailsPage, "TargetQtVersion").allTexts();
}

int WizardHandler::targetQtVersionIndex(const QString &qtVersionName) const
{
    return ComboBoxHelper(m_detailsPage, "TargetQtVersion").indexOf(qtVersionName);
}

int WizardHandler::targetQtVersionIndex() const
{
    return ComboBoxHelper(m_detailsPage, "TargetQtVersion").selectedIndex();
}

void WizardHandler::setStyleIndex(int index)
{
    ComboBoxHelper(m_detailsPage, "ControlsStyle").selectIndex(index);
}

int WizardHandler::styleIndex() const
{
    return ComboBoxHelper(m_detailsPage, "ControlsStyle").selectedIndex();
}

int WizardHandler::styleIndex(const QString &styleName) const
{
    return ComboBoxHelper(m_detailsPage, "ControlsStyle").indexOf(styleName);
}

QString WizardHandler::styleNameAt(int index) const
{
    return ComboBoxHelper(m_detailsPage, "ControlsStyle").text(index);
}

QString WizardHandler::styleName() const
{
    return styleNameAt(styleIndex());
}

void WizardHandler::setUseVirtualKeyboard(bool value)
{
    CheckBoxHelper(m_detailsPage, "UseVirtualKeyboard").setChecked(value);
}

bool WizardHandler::haveVirtualKeyboard() const
{
    return m_wizard->hasField("UseVirtualKeyboard");
}

bool WizardHandler::virtualKeyboardUsed() const
{
    return CheckBoxHelper(m_detailsPage, "UseVirtualKeyboard").isChecked();
}

void WizardHandler::enableCMakeGeneration(bool value)
{
    CheckBoxHelper(m_detailsPage, "EnableCMakeGeneration").setChecked(value);
}

bool WizardHandler::hasCMakeGeneration() const
{
    return m_wizard->hasField("EnableCMakeGeneration");
}

bool WizardHandler::cmakeGenerationEnabled() const
{
    return CheckBoxHelper(m_detailsPage, "EnableCMakeGeneration").isChecked();
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
