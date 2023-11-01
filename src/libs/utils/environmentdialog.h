// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "namevaluesdialog.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentDialog : public NameValuesDialog
{
public:
    static std::optional<EnvironmentItems> getEnvironmentItems(QWidget *parent = nullptr,
                                                                const EnvironmentItems &initial = {},
                                                                const QString &placeholderText = {},
                                                                Polisher polish = {});
};

} // namespace Utils
