// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "namevaluemodel.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentModel : public NameValueModel
{
public:
    Environment baseEnvironment() const;
    void setBaseEnvironment(const Environment &env);
};

} // namespace Utils
