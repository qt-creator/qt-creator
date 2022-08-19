// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "namevaluesdialog.h"
#include <thread>

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentDialog : public NameValuesDialog
{
    Q_OBJECT
public:
    static Utils::optional<EnvironmentItems> getEnvironmentItems(QWidget *parent = nullptr,
                                                                 const EnvironmentItems &initial = {},
                                                                 const QString &placeholderText = {},
                                                                 Polisher polish = {});
};

} // namespace Utils
