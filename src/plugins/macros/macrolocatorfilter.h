// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

#include <QIcon>

namespace Macros {
namespace Internal {

class MacroLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    MacroLocatorFilter();
    ~MacroLocatorFilter() override;

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

private:
    const QIcon m_icon;
};

} // namespace Internal
} // namespace Macros
