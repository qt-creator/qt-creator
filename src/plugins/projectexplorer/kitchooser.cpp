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

#include "kitconfigwidget.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>

namespace ProjectExplorer {

const char lastKitKey[] = "LastSelectedKit";

KitChooser::KitChooser(QWidget *parent) :
    QWidget(parent),
    m_kitPredicate([](const Kit *k) { return k->isValid(); })
{
    m_chooser = new QComboBox(this);
    m_chooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_manageButton = new QPushButton(KitConfigWidget::msgManage(), this);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    layout->addWidget(m_manageButton);
    setFocusProxy(m_manageButton);

    connect(m_chooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &KitChooser::onCurrentIndexChanged);
    connect(m_chooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &KitChooser::activated);
    connect(m_manageButton, &QAbstractButton::clicked, this, &KitChooser::onManageButtonClicked);
    connect(KitManager::instance(), &KitManager::kitsChanged, this, &KitChooser::populate);
}

void KitChooser::onManageButtonClicked()
{
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, this);
}

void KitChooser::onCurrentIndexChanged(int index)
{
    if (Kit *kit = kitAt(index))
        setToolTip(kitToolTip(kit));
    else
        setToolTip(QString());
    emit currentIndexChanged(index);
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
    foreach (Kit *kit, KitManager::sortKits(KitManager::kits())) {
        if (m_kitPredicate(kit)) {
            m_chooser->addItem(kitText(kit), qVariantFromValue(kit->id()));
            m_chooser->setItemData(m_chooser->count() - 1, kitToolTip(kit), Qt::ToolTipRole);
        }
    }

    const int n = m_chooser->count();
    const int index = Core::ICore::settings()->value(QLatin1String(lastKitKey)).toInt();
    if (0 <= index && index < n)
        m_chooser->setCurrentIndex(index);
    m_chooser->setEnabled(n > 1);

    if (n > 1)
        setFocusProxy(m_chooser);
    else
        setFocusProxy(m_manageButton);

}

Kit *KitChooser::currentKit() const
{
    const int index = m_chooser->currentIndex();
    Core::ICore::settings()->setValue(QLatin1String(lastKitKey), index);
    return index == -1 ? nullptr : kitAt(index);
}

void KitChooser::setCurrentKitId(Core::Id id)
{
    for (int i = 0, n = m_chooser->count(); i != n; ++i) {
        if (kitAt(i)->id() == id) {
            m_chooser->setCurrentIndex(i);
            break;
        }
    }
}

Core::Id KitChooser::currentKitId() const
{
    Kit *kit = currentKit();
    return kit ? kit->id() : Core::Id();
}

void KitChooser::setKitPredicate(const Kit::Predicate &predicate)
{
    m_kitPredicate = predicate;
    populate();
}

Kit *KitChooser::kitAt(int index) const
{
    auto id = qvariant_cast<Core::Id>(m_chooser->itemData(index));
    return KitManager::kit(id);
}

} // namespace ProjectExplorer
