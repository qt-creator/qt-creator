// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace TextEditor::Internal {

class BookmarkFilter : public Core::ILocatorFilter
{
public:
    BookmarkFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
    Core::LocatorFilterEntries match(const QString &input) const;
};

} // TextEditor::Internal
