// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace Macros {
namespace Internal {

class MacroLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    MacroLocatorFilter();

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
private:
    Core::LocatorMatcherTasks matchers() final;

    const QIcon m_icon;
};

} // namespace Internal
} // namespace Macros
