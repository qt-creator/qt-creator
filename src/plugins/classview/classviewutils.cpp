/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

#include "classviewutils.h"
#include "classviewconstants.h"
#include "classviewsymbolinformation.h"

// needed for the correct sorting order
#include <cplusplus/Icons.h>

#include <QStandardItem>
#include <QDebug>

namespace ClassView {
namespace Constants {

//! Default icon sort order
const int IconSortOrder[] = {
    CPlusPlus::Icons::NamespaceIconType,
    CPlusPlus::Icons::EnumIconType,
    CPlusPlus::Icons::ClassIconType,
    CPlusPlus::Icons::FuncPublicIconType,
    CPlusPlus::Icons::FuncProtectedIconType,
    CPlusPlus::Icons::FuncPrivateIconType,
    CPlusPlus::Icons::SignalIconType,
    CPlusPlus::Icons::SlotPublicIconType,
    CPlusPlus::Icons::SlotProtectedIconType,
    CPlusPlus::Icons::SlotPrivateIconType,
    CPlusPlus::Icons::VarPublicIconType,
    CPlusPlus::Icons::VarProtectedIconType,
    CPlusPlus::Icons::VarPrivateIconType,
    CPlusPlus::Icons::EnumeratorIconType,
    CPlusPlus::Icons::KeywordIconType,
    CPlusPlus::Icons::MacroIconType,
    CPlusPlus::Icons::UnknownIconType
};

} // namespace Constants

namespace Internal {

Utils::Utils()
{
}

QList<QVariant> Utils::locationsToRole(const QSet<SymbolLocation> &locations)
{
    QList<QVariant> locationsVar;
    foreach (const SymbolLocation &loc, locations)
        locationsVar.append(QVariant::fromValue(loc));

    return locationsVar;
}

QSet<SymbolLocation> Utils::roleToLocations(const QList<QVariant> &locationsVar)
{
    QSet<SymbolLocation> locations;
    foreach (const QVariant &loc, locationsVar) {
        if (loc.canConvert<SymbolLocation>())
            locations.insert(loc.value<SymbolLocation>());
    }

    return locations;
}

int Utils::iconTypeSortOrder(int icon)
{
    static QHash<int, int> sortOrder;

    // initialization
    if (sortOrder.count() == 0) {
        for (unsigned i = 0 ;
             i < sizeof(Constants::IconSortOrder) / sizeof(Constants::IconSortOrder[0]) ; ++i)
            sortOrder.insert(Constants::IconSortOrder[i], sortOrder.count());
    }

    // if it is missing - return the same value
    if (!sortOrder.contains(icon))
        return icon;

    return sortOrder[icon];
}

QStandardItem *Utils::setSymbolInformationToItem(const SymbolInformation &information,
                                                 QStandardItem *item)
{
    Q_ASSERT(item);

    item->setData(information.name(), Constants::SymbolNameRole);
    item->setData(information.type(), Constants::SymbolTypeRole);
    item->setData(information.iconType(), Constants::IconTypeRole);

    return item;
}

SymbolInformation Utils::symbolInformationFromItem(const QStandardItem *item)
{
    Q_ASSERT(item);

    if (!item)
        return SymbolInformation();

    const QString &name = item->data(Constants::SymbolNameRole).toString();
    const QString &type = item->data(Constants::SymbolTypeRole).toString();
    int iconType = 0;

    QVariant var = item->data(Constants::IconTypeRole);
    bool ok = false;
    int value;
    if (var.isValid())
        value = var.toInt(&ok);
    if (ok)
        iconType = value;

    return SymbolInformation(name, type, iconType);
}

void Utils::fetchItemToTarget(QStandardItem *item, const QStandardItem *target)
{
    if (!item || !target)
        return;

    int itemIndex = 0;
    int targetIndex = 0;
    int itemRows = item->rowCount();
    int targetRows = target->rowCount();

    while (itemIndex < itemRows && targetIndex < targetRows) {
        const QStandardItem *itemChild = item->child(itemIndex);
        const QStandardItem *targetChild = target->child(targetIndex);

        const SymbolInformation &itemInf = symbolInformationFromItem(itemChild);
        const SymbolInformation &targetInf = symbolInformationFromItem(targetChild);

        if (itemInf < targetInf) {
            ++itemIndex;
        } else if (itemInf == targetInf) {
            ++itemIndex;
            ++targetIndex;
        } else {
            item->insertRow(itemIndex, targetChild->clone());
            ++itemIndex;
            ++itemRows;
            ++targetIndex;
        }
    }

    // append
    while (targetIndex < targetRows) {
        item->appendRow(target->child(targetIndex)->clone());
        ++targetIndex;
    }
}

void Utils::moveItemToTarget(QStandardItem *item, const QStandardItem *target)
{
    if (!item || !target)
        return;

    int itemIndex = 0;
    int targetIndex = 0;
    int itemRows = item->rowCount();
    int targetRows = target->rowCount();

    while (itemIndex < itemRows && targetIndex < targetRows) {
        QStandardItem *itemChild = item->child(itemIndex);
        const QStandardItem *targetChild = target->child(targetIndex);

        const SymbolInformation &itemInf = Utils::symbolInformationFromItem(itemChild);
        const SymbolInformation &targetInf = Utils::symbolInformationFromItem(targetChild);

        if (itemInf < targetInf) {
            item->removeRow(itemIndex);
            --itemRows;
        } else if (itemInf == targetInf) {
            moveItemToTarget(itemChild, targetChild);
            ++itemIndex;
            ++targetIndex;
        } else {
            item->insertRow(itemIndex, targetChild->clone());
            moveItemToTarget(item->child(itemIndex), targetChild);
            ++itemIndex;
            ++itemRows;
            ++targetIndex;
        }
    }

    // append
    while (targetIndex < targetRows) {
        item->appendRow(target->child(targetIndex)->clone());
        moveItemToTarget(item->child(itemIndex), target->child(targetIndex));
        ++itemIndex;
        ++itemRows;
        ++targetIndex;
    }

    // remove end of item
    while (itemIndex < itemRows) {
        item->removeRow(itemIndex);
        --itemRows;
    }
}

} // namespace Internal
} // namespace ClassView
