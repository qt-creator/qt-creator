// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
