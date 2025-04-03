// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljslocatordata.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace QmlJSTools::Internal {

class QmlJSFunctionsFilter final : public Core::ILocatorFilter
{
public:
    QmlJSFunctionsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;

    LocatorData m_data;
};

} // namespace QmlJSTools::Internal
