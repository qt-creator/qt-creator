// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentdialog.h"

#include "utilstr.h"

namespace Utils {

std::optional<EnvironmentChanges> runEnvironmentItemsDialog(
    QWidget *parent,
    const EnvironmentChanges &initial,
    const QString &placeholderText,
    NameValuesDialog::Polisher polisher,
    const QString &dialogTitle,
    const FilePath &browseHint)
{
    return NameValuesDialog::getNameValueItems(
        parent,
        initial,
        placeholderText,
        polisher,
        dialogTitle.isEmpty() ? Tr::tr("Edit Environment") : dialogTitle,
        browseHint);
}

} // namespace Utils
