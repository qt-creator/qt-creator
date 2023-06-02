// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace QmlJSTools::Internal {

class LocatorData;

class QmlJSFunctionsFilter : public Core::ILocatorFilter
{
public:
    QmlJSFunctionsFilter(LocatorData *data);

private:
    Core::LocatorMatcherTasks matchers() final;

    LocatorData *m_data = nullptr;
};

} // namespace QmlJSTools::Internal
