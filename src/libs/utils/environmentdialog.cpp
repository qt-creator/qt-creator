// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentdialog.h"

#include "utilstr.h"

namespace Utils {

std::optional<EnvironmentItems> runEnvironmentItemsDialog(
    QWidget *parent,
    const EnvironmentItems &initial,
    const QString &placeholderText,
    NameValuesDialog::Polisher polisher,
    const QString &dialogTitle)
{
    return NameValuesDialog::getNameValueItems(
        parent,
        initial,
        placeholderText,
        polisher,
        dialogTitle.isEmpty() ? Tr::tr("Edit Environment") : dialogTitle);
}

} // namespace Utils
