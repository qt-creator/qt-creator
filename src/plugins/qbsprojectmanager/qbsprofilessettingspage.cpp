/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsprofilessettingspage.h"
#include "ui_qbsprofilessettingswidget.h"

#include "customqbspropertiesdialog.h"
#include "qbsconstants.h"
#include "qbsprojectmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
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

    void applyCustomProperties();

private slots:
    void refreshKitsList();
    void displayCurrentProfile();
    void editProfile();

private:
    void setupCustomProperties(const ProjectExplorer::Kit *kit);
    void mergeCustomPropertiesIntoModel();

    Ui::QbsProfilesSettingsWidget m_ui;
    qbs::SettingsModel m_model;

    typedef QHash<Core::Id, QVariantMap> CustomProperties;
    CustomProperties m_customProperties;
    bool m_applyingProperties;
};

QbsProfilesSettingsPage::QbsProfilesSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent), m_widget(0)
{
    setId("AA.QbsProfiles");
    setDisplayName(QCoreApplication::translate("QbsProjectManager", "Profiles"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("QbsProjectManager",
                                                   Constants::QBS_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::QBS_SETTINGS_CATEGORY_ICON));
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
        m_widget->applyCustomProperties();
}

void QbsProfilesSettingsPage::finish()
{
    delete m_widget;
    m_widget = 0;
}


QbsProfilesSettingsWidget::QbsProfilesSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_model(Core::ICore::userResourcePath())
    , m_applyingProperties(false)
{
    m_model.setEditable(false);
    m_ui.setupUi(this);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsChanged,
            this, &QbsProfilesSettingsWidget::refreshKitsList);
    connect(m_ui.expandButton, SIGNAL(clicked()), m_ui.propertiesView, SLOT(expandAll()));
    connect(m_ui.collapseButton, SIGNAL(clicked()), m_ui.propertiesView, SLOT(collapseAll()));
    connect(m_ui.editButton, SIGNAL(clicked()), SLOT(editProfile()));
    refreshKitsList();
}

void QbsProfilesSettingsWidget::applyCustomProperties()
{
    QTC_ASSERT(!m_applyingProperties, return);
    m_applyingProperties = true; // The following will cause kitsChanged() to be emitted.
    for (CustomProperties::ConstIterator it = m_customProperties.constBegin();
         it != m_customProperties.constEnd(); ++it) {
        ProjectExplorer::Kit * const kit = ProjectExplorer::KitManager::find(it.key());
        QTC_ASSERT(kit, continue);
        kit->setValue(Core::Id(Constants::QBS_PROPERTIES_KEY_FOR_KITS), it.value());
    }
    m_applyingProperties = false;
    m_model.reload();
    displayCurrentProfile();
}

void QbsProfilesSettingsWidget::refreshKitsList()
{
    if (m_applyingProperties)
        return;

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
    m_customProperties.clear();
    foreach (const ProjectExplorer::Kit * const kit, validKits) {
        if (kit->id() == currentId)
            newCurrentIndex = m_ui.kitsComboBox->count();
        m_ui.kitsComboBox->addItem(kit->displayName(), kit->id().toSetting());
        setupCustomProperties(kit);
    }
    mergeCustomPropertiesIntoModel();
    m_ui.editButton->setEnabled(hasKits);
    if (newCurrentIndex != -1)
        m_ui.kitsComboBox->setCurrentIndex(newCurrentIndex);
    else if (hasKits)
        m_ui.kitsComboBox->setCurrentIndex(0);
    displayCurrentProfile();
    connect(m_ui.kitsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(displayCurrentProfile()));
}

void QbsProfilesSettingsWidget::displayCurrentProfile()
{
    m_ui.propertiesView->setModel(0);
    if (m_ui.kitsComboBox->currentIndex() == -1)
        return;
    const Core::Id kitId = Core::Id::fromSetting(m_ui.kitsComboBox->currentData());
    const ProjectExplorer::Kit * const kit = ProjectExplorer::KitManager::find(kitId);
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

void QbsProfilesSettingsWidget::editProfile()
{
    QTC_ASSERT(m_ui.kitsComboBox->currentIndex() != -1, return);

    const Core::Id kitId = Core::Id::fromSetting(m_ui.kitsComboBox->currentData());
    CustomQbsPropertiesDialog dlg(m_customProperties.value(kitId), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    m_customProperties.insert(kitId, dlg.properties());
    mergeCustomPropertiesIntoModel();
    displayCurrentProfile();
}

void QbsProfilesSettingsWidget::setupCustomProperties(const ProjectExplorer::Kit *kit)
{
    const QVariantMap &properties
            = kit->value(Core::Id(Constants::QBS_PROPERTIES_KEY_FOR_KITS)).toMap();
    m_customProperties.insert(kit->id(), properties);
}

void QbsProfilesSettingsWidget::mergeCustomPropertiesIntoModel()
{
    QVariantMap customProperties;
    for (CustomProperties::ConstIterator it = m_customProperties.constBegin();
         it != m_customProperties.constEnd(); ++it) {
        const Core::Id kitId = it.key();
        const ProjectExplorer::Kit * const kit = ProjectExplorer::KitManager::find(kitId);
        QTC_ASSERT(kit, continue);
        const QString keyPrefix = QLatin1String("profiles.")
                + QbsManager::instance()->profileForKit(kit) + QLatin1Char('.');
        for (QVariantMap::ConstIterator it2 = it.value().constBegin(); it2 != it.value().constEnd();
             ++it2) {
            customProperties.insert(keyPrefix + it2.key(), it2.value());
        }
    }
    m_model.setAdditionalProperties(customProperties);
}

} // namespace Internal
} // namespace QbsProjectManager

#include "qbsprofilessettingspage.moc"
