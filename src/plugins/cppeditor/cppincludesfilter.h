// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/basefilefilter.h>

namespace CppEditor::Internal {

class CppIncludesFilter : public Core::BaseFileFilter
{
    Q_OBJECT

public:
    CppIncludesFilter();

    // ILocatorFilter interface
public:
    void prepareSearch(const QString &entry) override;
    void refresh(QFutureInterface<void> &future) override;

private:
    void markOutdated();

    bool m_needsUpdate = true;
};

} // namespace CppEditor::Internal
