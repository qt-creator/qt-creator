// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

#include <QIcon>
#include <QMultiMap>
#include <QUrl>

#include <atomic>

namespace Help {
namespace Internal {

class HelpIndexFilter final : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    HelpIndexFilter();
    ~HelpIndexFilter() final;

    // ILocatorFilter
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

    QStringList allIndices() const;

signals:
    void linksActivated(const QMultiMap<QString, QUrl> &links, const QString &key) const;

private:
    bool updateCache(QFutureInterface<Core::LocatorFilterEntry> &future,
                     const QStringList &cache, const QString &entry);
    void invalidateCache();
    std::optional<Utils::Tasking::TaskItem> refreshRecipe() override;

    QStringList m_allIndicesCache;
    QStringList m_lastIndicesCache;
    QString m_lastEntry;
    std::atomic_bool m_needsUpdate = true;
    QIcon m_icon;
};

} // namespace Internal
} // namespace Help
