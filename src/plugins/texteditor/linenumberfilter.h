// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace Core { class IEditor; }

namespace TextEditor {
namespace Internal {

class LineNumberFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit LineNumberFilter(QObject *parent = nullptr);

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
private:
    Core::LocatorMatcherTasks matchers() final;
    bool m_hasCurrentEditor = false;
};

} // namespace Internal
} // namespace TextEditor
