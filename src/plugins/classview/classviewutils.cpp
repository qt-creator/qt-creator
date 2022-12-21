// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewutils.h"

#include "classviewconstants.h"

#include <QStandardItem>

namespace ClassView {
namespace Internal {

/*!
    Converts QVariant location container to internal.
    \a locationsVar contains a list of variant locations from the data of an
    item.
    Returns a set of symbol locations.
 */

QSet<SymbolLocation> roleToLocations(const QList<QVariant> &locationsVar)
{
    QSet<SymbolLocation> locations;
    for (const QVariant &loc : locationsVar) {
        if (loc.canConvert<SymbolLocation>())
            locations.insert(loc.value<SymbolLocation>());
    }

    return locations;
}


/*!
    Returns symbol information for \a item.
*/

SymbolInformation symbolInformationFromItem(const QStandardItem *item)
{
    Q_ASSERT(item);

    if (!item)
        return SymbolInformation();

    const QString &name = item->data(Constants::SymbolNameRole).toString();
    const QString &type = item->data(Constants::SymbolTypeRole).toString();
    int iconType = item->data(Constants::IconTypeRole).toInt();

    return SymbolInformation(name, type, iconType);
}

} // namespace Internal
} // namespace ClassView
