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

void Core::Internal::runSearch(QFutureInterface<Core::LocatorFilterEntry> &future,
                               const QList<ILocatorFilter *> &filters, const QString &searchText)
{
    QSet<QString> alreadyAdded;
    const bool checkDuplicates = (filters.size() > 1);
    for (ILocatorFilter *filter : filters) {
        if (future.isCanceled())
            break;

        const QList<LocatorFilterEntry> filterResults = filter->matchesFor(future, searchText);
        QVector<LocatorFilterEntry> uniqueFilterResults;
        uniqueFilterResults.reserve(filterResults.size());
        for (const LocatorFilterEntry &entry : filterResults) {
            if (checkDuplicates) {
                const QString stringData = entry.internalData.toString();
                if (!stringData.isEmpty()) {
                    if (alreadyAdded.contains(stringData))
                        continue;
                    alreadyAdded.insert(stringData);
                }
            }
            uniqueFilterResults.append(entry);
        }
        if (!uniqueFilterResults.isEmpty())
            future.reportResults(uniqueFilterResults);
    }
}
