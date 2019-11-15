/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewutils.h"
#include "classviewconstants.h"
#include "classviewsymbolinformation.h"

#include <QStandardItem>
#include <QDebug>

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
    foreach (const QVariant &loc, locationsVar) {
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

} // namespace Internal
} // namespace ClassView
