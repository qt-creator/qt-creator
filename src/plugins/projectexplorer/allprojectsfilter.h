// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/basefilefilter.h>

namespace ProjectExplorer {
namespace Internal {

class AllProjectsFilter : public Core::BaseFileFilter
{
    Q_OBJECT

public:
    AllProjectsFilter();
    void prepareSearch(const QString &entry) override;

private:
    void invalidateCache();
};

} // namespace Internal
} // namespace ProjectExplorer
