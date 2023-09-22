// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace TextEditor::Internal {

class BookmarkManager;

class BookmarkFilter : public Core::ILocatorFilter
{
public:
    BookmarkFilter(BookmarkManager *manager);

private:
    Core::LocatorMatcherTasks matchers() final;
    Core::LocatorFilterEntries match(const QString &input) const;

    BookmarkManager *m_manager = nullptr; // not owned
};

} // TextEditor::Internal
