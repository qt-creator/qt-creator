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

#include "kitchooser.h"

#include "kitinformation.h"
#include "kitmanager.h"

namespace ProjectExplorer {

KitChooser::KitChooser(QWidget *parent) :
    QComboBox(parent)
{
    connect(this, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));
}

void KitChooser::onCurrentIndexChanged(int index)
{
    if (Kit *kit = kitAt(index))
        setToolTip(kitToolTip(kit));
    else
        setToolTip(QString());
}

bool KitChooser::kitMatches(const Kit *k) const
{
    return k->isValid();
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
    clear();
    foreach (Kit *kit, KitManager::instance()->kits()) {
        if (kitMatches(kit)) {
            addItem(kitText(kit), qVariantFromValue(kit->id()));
            setItemData(count() - 1, kitToolTip(kit), Qt::ToolTipRole);
        }
    }
    setEnabled(count() > 1);
}

Kit *KitChooser::currentKit() const
{
    const int index = currentIndex();
    return index == -1 ? 0 : kitAt(index);
}

void KitChooser::setCurrentKitId(Core::Id id)
{
    for (int i = 0, n = count(); i != n; ++i) {
        if (kitAt(i)->id() == id) {
            setCurrentIndex(i);
            break;
        }
    }
}

Core::Id KitChooser::currentKitId() const
{
    Kit *kit = currentKit();
    return kit ? kit->id() : Core::Id();
}

Kit *KitChooser::kitAt(int index) const
{
    Core::Id id = qvariant_cast<Core::Id>(itemData(index));
    return KitManager::instance()->find(id);
}

} // namespace ProjectExplorer
