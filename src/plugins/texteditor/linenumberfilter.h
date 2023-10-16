// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace TextEditor::Internal {

class LineNumberFilter : public Core::ILocatorFilter
{
public:
    LineNumberFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

} // namespace TextEditor::Internal
