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

#include "kitchooser.h"

#include "kitinformation.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "session.h"
#include "target.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>

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
    m_manageButton = new QPushButton(KitAspectWidget::msgManage(), this);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    layout->addWidget(m_manageButton);
    setFocusProxy(m_manageButton);

    connect(m_chooser, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KitChooser::onCurrentIndexChanged);
    connect(m_chooser, QOverload<int>::of(&QComboBox::activated),
            this, &KitChooser::onActivated);
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
    ICore::settings()->setValue(lastKitKey, id.toSetting());
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

    if (Target *target = SessionManager::startupTarget()) {
        Kit *kit = target->kit();
        if (m_kitPredicate(kit)) {
            QString display = tr("Kit of Active Project: %1").arg(kitText(kit));
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

    foreach (Kit *kit, KitManager::sortKits(KitManager::kits())) {
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
