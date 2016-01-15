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

#include "locatorsearchutils.h"

#include <QSet>
#include <QString>
#include <QVariant>

namespace Core {

uint qHash(const LocatorFilterEntry &entry)
{
    if (entry.internalData.canConvert(QVariant::String))
        return QT_PREPEND_NAMESPACE(qHash)(entry.internalData.toString());
    return QT_PREPEND_NAMESPACE(qHash)(entry.internalData.constData());
}

} // namespace Core

void Core::Internal::runSearch(QFutureInterface<Core::LocatorFilterEntry> &future,
                               const QList<ILocatorFilter *> &filters, const QString &searchText)
{
    QSet<LocatorFilterEntry> alreadyAdded;
    const bool checkDuplicates = (filters.size() > 1);
    foreach (ILocatorFilter *filter, filters) {
        if (future.isCanceled())
            break;

        QList<LocatorFilterEntry> filterResults = filter->matchesFor(future, searchText);
        QVector<LocatorFilterEntry> uniqueFilterResults;
        uniqueFilterResults.reserve(filterResults.size());
        foreach (const LocatorFilterEntry &entry, filterResults) {
            if (checkDuplicates && alreadyAdded.contains(entry))
                continue;
            uniqueFilterResults.append(entry);
            if (checkDuplicates)
                alreadyAdded.insert(entry);
        }
        if (!uniqueFilterResults.isEmpty())
            future.reportResults(uniqueFilterResults);
    }
}
