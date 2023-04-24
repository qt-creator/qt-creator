// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core::Internal {

class OpenDocumentsFilter : public ILocatorFilter
{
public:
    OpenDocumentsFilter();

private:
    LocatorMatcherTasks matchers() final;
};

} // namespace Core::Internal
