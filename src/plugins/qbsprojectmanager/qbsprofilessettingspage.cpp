// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsprofilessettingspage.h"

#include "qbsprofilemanager.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QComboBox>
#include <QHash>
#include <QHeaderView>
#include <QTreeView>
#include <QWidget>

using namespace ProjectExplorer;

namespace QbsProjectManager::Internal {

class ProfileTreeItem : public Utils::TypedTreeItem<ProfileTreeItem, ProfileTreeItem>
{
public:
    ProfileTreeItem() = default;
    ProfileTreeItem(const QString &key, const QString &value) : m_key(key), m_value(value) { }

private:
    QVariant data(int column, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        if (column == 0)
            return m_key;
        if (column == 1)
            return m_value;
        return {};
    }

    const QString m_key;
    const QString m_value;
};

class ProfileModel : public Utils::TreeModel<ProfileTreeItem>
{
public:
    ProfileModel() : TreeModel(static_cast<QObject *>(nullptr))
    {
        setHeader(QStringList{Tr::tr("Key"), Tr::tr("Value")});
        reload();
    }

    void reload()
    {
        ProfileTreeItem * const newRoot = new ProfileTreeItem(QString(), QString());
        QHash<QStringList, ProfileTreeItem *> itemMap;
        const QStringList output = QbsProfileManager::runQbsConfig(
                    QbsProfileManager::QbsConfigOp::Get, "profiles").split('\n', Qt::SkipEmptyParts);
        for (QString line : output) {
            line = line.trimmed();
            line = line.mid(QString("profiles.").length());
            const int colonIndex = line.indexOf(':');
            if (colonIndex == -1)
                continue;
            const QStringList key = line.left(colonIndex).trimmed().split('.', Qt::SkipEmptyParts);
            const QString value = line.mid(colonIndex + 1).trimmed();
            QStringList partialKey;
            ProfileTreeItem *parent = newRoot;
            for (const QString &keyComponent : key) {
                partialKey << keyComponent;
                ProfileTreeItem *&item = itemMap[partialKey];
                if (!item) {
                    item = new ProfileTreeItem(keyComponent, partialKey == key ? value : QString());
                    parent->appendChild(item);
                }
                parent = item;
            }
        }
        setRootItem(newRoot);
    }
};

class QbsProfilesSettingsWidget : public Core::IOptionsPageWidget
{
public:
    QbsProfilesSettingsWidget();

private:
    void apply() final {}

    void refreshKitsList();
    void displayCurrentProfile();

    ProfileModel m_model;
    QComboBox *m_kitsComboBox;
    QLabel *m_profileValueLabel;
    QTreeView *m_propertiesView;
};

QbsProfilesSettingsPage::QbsProfilesSettingsPage()
{
    setId("Y.QbsProfiles");
    setDisplayName(Tr::tr("Profiles"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new QbsProfilesSettingsWidget; });
}

QbsProfilesSettingsWidget::QbsProfilesSettingsWidget()
{
    m_kitsComboBox = new QComboBox;
    m_profileValueLabel = new QLabel;
    m_propertiesView = new QTreeView;

    using namespace Layouting;
    Column {
        Form {
            Tr::tr("Kit:"), m_kitsComboBox, br,
            Tr::tr("Associated profile:"), m_profileValueLabel, br,
        },
        hr,
        Tr::tr("Profile properties:"),
        Row {
            m_propertiesView,
            Column {
                PushButton {
                    text(Tr::tr("E&xpand All")),
                    onClicked([this] { m_propertiesView->expandAll(); }),
                },
                PushButton {
                    text(Tr::tr("&Collapse All")),
                    onClicked([this] { m_propertiesView->collapseAll(); }),
                },
                st,
            },
        },
    }.attachTo(this);

    connect(QbsProfileManager::instance(), &QbsProfileManager::qbsProfilesUpdated,
            this, &QbsProfilesSettingsWidget::refreshKitsList);
    refreshKitsList();
}

void QbsProfilesSettingsWidget::refreshKitsList()
{
    m_kitsComboBox->disconnect(this);
    m_propertiesView->setModel(nullptr);
    m_model.reload();
    m_profileValueLabel->clear();
    Utils::Id currentId;
    if (m_kitsComboBox->count() > 0)
        currentId = Utils::Id::fromSetting(m_kitsComboBox->currentData());
    m_kitsComboBox->clear();
    int newCurrentIndex = -1;
    QList<Kit *> validKits = KitManager::kits();
    Utils::erase(validKits, [](const Kit *k) { return !k->isValid(); });
    const bool hasKits = !validKits.isEmpty();
    for (const Kit * const kit : std::as_const(validKits)) {
        if (kit->id() == currentId)
            newCurrentIndex = m_kitsComboBox->count();
        m_kitsComboBox->addItem(kit->displayName(), kit->id().toSetting());
    }
    if (newCurrentIndex != -1)
        m_kitsComboBox->setCurrentIndex(newCurrentIndex);
    else if (hasKits)
        m_kitsComboBox->setCurrentIndex(0);
    displayCurrentProfile();
    connect(m_kitsComboBox, &QComboBox::currentIndexChanged,
            this, &QbsProfilesSettingsWidget::displayCurrentProfile);
}

void QbsProfilesSettingsWidget::displayCurrentProfile()
{
    m_propertiesView->setModel(nullptr);
    if (m_kitsComboBox->currentIndex() == -1)
        return;
    const Utils::Id kitId = Utils::Id::fromSetting(m_kitsComboBox->currentData());
    const Kit * const kit = KitManager::kit(kitId);
    QTC_ASSERT(kit, return);
    const QString profileName = QbsProfileManager::ensureProfileForKit(kit);
    m_profileValueLabel->setText(profileName);
    for (int i = 0; i < m_model.rowCount(); ++i) {
        const QModelIndex currentProfileIndex = m_model.index(i, 0);
        if (m_model.data(currentProfileIndex, Qt::DisplayRole).toString() != profileName)
            continue;
        m_propertiesView->setModel(&m_model);
        m_propertiesView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_propertiesView->setRootIndex(currentProfileIndex);
        return;
    }
}

} // QbsProjectManager::Internal
