// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "namevaluesdialog.h"

namespace Utils {

QTCREATOR_UTILS_EXPORT std::optional<EnvironmentChanges> runEnvironmentItemsDialog(
    QWidget *parent = nullptr,
    const EnvironmentChanges &initial = {},
    const QString &placeholderText = {},
    NameValuesDialog::Polisher polish = {},
    const QString &dialogTitle = {},
    const FilePath &browseHint = {});

} // namespace Utils
