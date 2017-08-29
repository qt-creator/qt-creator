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

#include "qbsprofilessettingspage.h"
#include "ui_qbsprofilessettingswidget.h"

#include "qbsprojectmanager.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagersettings.h"

#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <QCoreApplication>
#include <QHash>
#include <QWidget>

namespace QbsProjectManager {
namespace Internal {

class QbsProfilesSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    QbsProfilesSettingsWidget(QWidget *parent = 0);

    void apply();

private:
    void refreshKitsList();
    void displayCurrentProfile();

    Ui::QbsProfilesSettingsWidget m_ui;
    qbs::SettingsModel m_model;
};

QbsProfilesSettingsPage::QbsProfilesSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
    , m_widget(0)
    , m_useQtcSettingsDirPersistent(QbsProjectManagerSettings::useCreatorSettingsDirForQbs())

{
    setId("Y.QbsProfiles");
    setDisplayName(QCoreApplication::translate("QbsProjectManager", "Qbs"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
       ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *QbsProfilesSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new QbsProfilesSettingsWidget;
    return m_widget;
}

void QbsProfilesSettingsPage::apply()
{
    if (m_widget)
        m_widget->apply();
    m_useQtcSettingsDirPersistent = QbsProjectManagerSettings::useCreatorSettingsDirForQbs();
}

void QbsProfilesSettingsPage::finish()
{
    delete m_widget;
    m_widget = 0;
    QbsProjectManagerSettings::setUseCreatorSettingsDirForQbs(m_useQtcSettingsDirPersistent);
    QbsProjectManagerSettings::writeSettings();
}


QbsProfilesSettingsWidget::QbsProfilesSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_model(QbsProjectManagerSettings::qbsSettingsBaseDir())
{
    m_model.setEditable(false);
    m_ui.setupUi(this);
    m_ui.settingsDirCheckBox->setText(tr("Store profiles in %1 settings directory")
                                      .arg(Core::Constants::IDE_DISPLAY_NAME));
    m_ui.settingsDirCheckBox->setChecked(QbsProjectManagerSettings::useCreatorSettingsDirForQbs());
    m_ui.versionValueLabel->setText(qbs::LanguageInfo::qbsVersion());
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsChanged,
            this, &QbsProfilesSettingsWidget::refreshKitsList);
    connect(m_ui.settingsDirCheckBox, &QCheckBox::stateChanged, [this]() {
        QbsProjectManagerSettings::setUseCreatorSettingsDirForQbs(m_ui.settingsDirCheckBox->isChecked());
        m_model.updateSettingsDir(QbsProjectManagerSettings::qbsSettingsBaseDir());
        displayCurrentProfile();
    });
    connect(m_ui.expandButton, &QAbstractButton::clicked,
            m_ui.propertiesView, &QTreeView::expandAll);
    connect(m_ui.collapseButton, &QAbstractButton::clicked,
            m_ui.propertiesView, &QTreeView::collapseAll);
    refreshKitsList();
}

void QbsProfilesSettingsWidget::apply()
{
    m_model.reload();
    displayCurrentProfile();
}

void QbsProfilesSettingsWidget::refreshKitsList()
{
    m_ui.kitsComboBox->disconnect(this);
    m_ui.propertiesView->setModel(0);
    m_model.reload();
    m_ui.profileValueLabel->clear();
    Core::Id currentId;
    if (m_ui.kitsComboBox->count() > 0)
        currentId = Core::Id::fromSetting(m_ui.kitsComboBox->currentData());
    m_ui.kitsComboBox->clear();
    int newCurrentIndex = -1;
    QList<ProjectExplorer::Kit *> validKits = ProjectExplorer::KitManager::kits();
    Utils::erase(validKits, [](const ProjectExplorer::Kit *k) { return !k->isValid(); });
    const bool hasKits = !validKits.isEmpty();
    foreach (const ProjectExplorer::Kit * const kit, validKits) {
        if (kit->id() == currentId)
            newCurrentIndex = m_ui.kitsComboBox->count();
        m_ui.kitsComboBox->addItem(kit->displayName(), kit->id().toSetting());
    }
    if (newCurrentIndex != -1)
        m_ui.kitsComboBox->setCurrentIndex(newCurrentIndex);
    else if (hasKits)
        m_ui.kitsComboBox->setCurrentIndex(0);
    displayCurrentProfile();
    connect(m_ui.kitsComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &QbsProfilesSettingsWidget::displayCurrentProfile);
}

void QbsProfilesSettingsWidget::displayCurrentProfile()
{
    m_ui.propertiesView->setModel(0);
    if (m_ui.kitsComboBox->currentIndex() == -1)
        return;
    const Core::Id kitId = Core::Id::fromSetting(m_ui.kitsComboBox->currentData());
    const ProjectExplorer::Kit * const kit = ProjectExplorer::KitManager::kit(kitId);
    QTC_ASSERT(kit, return);
    const QString profileName = QbsManager::instance()->profileForKit(kit);
    m_ui.profileValueLabel->setText(profileName);
    for (int i = 0; i < m_model.rowCount(); ++i) {
        const QModelIndex profilesIndex = m_model.index(i, 0);
        if (m_model.data(profilesIndex).toString() != QLatin1String("profiles"))
            continue;
        for (int i = 0; i < m_model.rowCount(profilesIndex); ++i) {
            const QModelIndex currentProfileIndex = m_model.index(i, 0, profilesIndex);
            if (m_model.data(currentProfileIndex).toString() != profileName)
                continue;
            m_ui.propertiesView->setModel(&m_model);
            m_ui.propertiesView->header()->setSectionResizeMode(m_model.keyColumn(),
                                                                QHeaderView::ResizeToContents);
            m_ui.propertiesView->setRootIndex(currentProfileIndex);
            return;
        }
    }
}

} // namespace Internal
} // namespace QbsProjectManager

#include "qbsprofilessettingspage.moc"
