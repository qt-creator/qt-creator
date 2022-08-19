// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeTargetLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    CMakeTargetLocatorFilter();

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) final;

private:
    void projectListUpdated();

    QList<Core::LocatorFilterEntry> m_result;
};

class BuildCMakeTargetLocatorFilter : CMakeTargetLocatorFilter
{
    Q_OBJECT

public:
    BuildCMakeTargetLocatorFilter();

    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText,
                int *selectionStart,
                int *selectionLength) const final;
};

class OpenCMakeTargetLocatorFilter : CMakeTargetLocatorFilter
{
    Q_OBJECT

public:
    OpenCMakeTargetLocatorFilter();

    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText,
                int *selectionStart,
                int *selectionLength) const final;
};

} // namespace Internal
} // namespace CMakeProjectManager
