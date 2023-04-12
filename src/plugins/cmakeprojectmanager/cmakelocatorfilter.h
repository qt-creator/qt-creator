// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace CMakeProjectManager::Internal {

class CMakeTargetLocatorFilter : public Core::ILocatorFilter
{
public:
    CMakeTargetLocatorFilter();

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) final;
protected:
    using BuildAcceptor = std::function<void(const Utils::FilePath &, const QString &)>;
    void setBuildAcceptor(const BuildAcceptor &acceptor) { m_acceptor = acceptor; }

private:
    void projectListUpdated();

    QList<Core::LocatorFilterEntry> m_result;
    BuildAcceptor m_acceptor;
};

class BuildCMakeTargetLocatorFilter : CMakeTargetLocatorFilter
{
public:
    BuildCMakeTargetLocatorFilter();
};

class OpenCMakeTargetLocatorFilter : CMakeTargetLocatorFilter
{
public:
    OpenCMakeTargetLocatorFilter();
};

} // CMakeProjectManager::Internal
