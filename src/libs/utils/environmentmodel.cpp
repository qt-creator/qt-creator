// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentmodel.h"

#include "environment.h"

namespace Utils {

Environment EnvironmentModel::baseEnvironment() const
{
    return Environment(baseNameValueDictionary());
}

void EnvironmentModel::setBaseEnvironment(const Environment &env)
{
    setBaseNameValueDictionary(env.toDictionary());
}

} // namespace Utils
