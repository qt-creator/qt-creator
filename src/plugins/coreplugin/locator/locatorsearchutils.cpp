// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorsearchutils.h"

#include <utils/link.h>

#include <unordered_set>

void Core::Internal::runSearch(QFutureInterface<Core::LocatorFilterEntry> &future,
                               const QList<ILocatorFilter *> &filters, const QString &searchText)
{
    std::unordered_set<Utils::FilePath> addedCache;
    const bool checkDuplicates = (filters.size() > 1);
    const auto duplicatesRemoved = [&](const QList<LocatorFilterEntry> &entries) {
        if (!checkDuplicates)
            return entries;
        QList<LocatorFilterEntry> results;
        results.reserve(entries.size());
        for (const LocatorFilterEntry &entry : entries) {
            const auto &link = entry.linkForEditor;
            if (!link || addedCache.emplace(link->targetFilePath).second)
                results.append(entry);
        }
        return results;
    };

    for (ILocatorFilter *filter : filters) {
        if (future.isCanceled())
            break;
        const auto results = duplicatesRemoved(filter->matchesFor(future, searchText));
        if (!results.isEmpty())
            future.reportResults(results);
    }
}
