// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitchooser.h"

#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "target.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

const char lastKitKey[] = "LastSelectedKit";

KitChooser::KitChooser(QWidget *parent) :
    QWidget(parent),
    m_kitPredicate([](const Kit *k) { return k->isValid(); })
{
    m_chooser = new QComboBox(this);
    m_chooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_manageButton = new QPushButton(KitAspect::msgManage(), this);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    layout->addWidget(m_manageButton);
    setFocusProxy(m_manageButton);

    connect(m_chooser, &QComboBox::currentIndexChanged, this, &KitChooser::onCurrentIndexChanged);
    connect(m_chooser, &QComboBox::activated, this, &KitChooser::onActivated);
    connect(m_manageButton, &QAbstractButton::clicked, this, &KitChooser::onManageButtonClicked);
    connect(KitManager::instance(), &KitManager::kitsChanged, this, &KitChooser::populate);
}

void KitChooser::onManageButtonClicked()
{
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, this);
}

void KitChooser::setShowIcons(bool showIcons)
{
    m_showIcons = showIcons;
}

void KitChooser::onCurrentIndexChanged()
{
    const Id id = Id::fromSetting(m_chooser->currentData());
    Kit *kit = KitManager::kit(id);
    setToolTip(kit ? kitToolTip(kit) : QString());
    emit currentIndexChanged();
}

void KitChooser::onActivated()
{
    // Active user interaction.
    Id id = Id::fromSetting(m_chooser->currentData());
    if (m_hasStartupKit && m_chooser->currentIndex() == 0)
        id = Id(); // Special value to indicate startup kit.
    ICore::settings()->setValueWithDefault(lastKitKey, id.toSetting(), Id().toSetting());
    emit activated();
}

QString KitChooser::kitText(const Kit *k) const
{
    return k->displayName();
}

QString KitChooser::kitToolTip(Kit *k) const
{
    return k->toHtml();
}

void KitChooser::populate()
{
    m_chooser->clear();

    const Id lastKit = Id::fromSetting(ICore::settings()->value(lastKitKey));
    bool didActivate = false;

    if (Target *target = ProjectManager::startupTarget()) {
        Kit *kit = target->kit();
        if (m_kitPredicate(kit)) {
            QString display = Tr::tr("Kit of Active Project: %1").arg(kitText(kit));
            m_chooser->addItem(display, kit->id().toSetting());
            m_chooser->setItemData(0, kitToolTip(kit), Qt::ToolTipRole);
            if (!lastKit.isValid()) {
                m_chooser->setCurrentIndex(0);
                didActivate = true;
            }
            m_chooser->insertSeparator(1);
            m_hasStartupKit = true;
        }
    }
    for (Kit *kit : KitManager::sortedKits()) {
        if (m_kitPredicate(kit)) {
            m_chooser->addItem(kitText(kit), kit->id().toSetting());
            const int pos = m_chooser->count() - 1;
            m_chooser->setItemData(pos, kitToolTip(kit), Qt::ToolTipRole);
            if (m_showIcons)
                m_chooser->setItemData(pos, kit->displayIcon(), Qt::DecorationRole);
            if (!didActivate && kit->id() == lastKit) {
                m_chooser->setCurrentIndex(pos);
                didActivate = true;
            }
        }
    }

    const int n = m_chooser->count();
    m_chooser->setEnabled(n > 1);

    if (n > 1)
        setFocusProxy(m_chooser);
    else
        setFocusProxy(m_manageButton);

}

Kit *KitChooser::currentKit() const
{
    const Id id = Id::fromSetting(m_chooser->currentData());
    return KitManager::kit(id);
}

void KitChooser::setCurrentKitId(Utils::Id id)
{
    QVariant v = id.toSetting();
    for (int i = 0, n = m_chooser->count(); i != n; ++i) {
        if (m_chooser->itemData(i) == v) {
            m_chooser->setCurrentIndex(i);
            break;
        }
    }
}

Utils::Id KitChooser::currentKitId() const
{
    Kit *kit = currentKit();
    return kit ? kit->id() : Utils::Id();
}

void KitChooser::setKitPredicate(const Kit::Predicate &predicate)
{
    m_kitPredicate = predicate;
    populate();
}

} // namespace ProjectExplorer
