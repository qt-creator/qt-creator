// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentdialog.h"

#include "utilstr.h"

namespace Utils {

std::optional<EnvironmentItems> EnvironmentDialog::getEnvironmentItems(
    QWidget *parent, const EnvironmentItems &initial, const QString &placeholderText, Polisher polisher)
{
    return getNameValueItems(
        parent,
        initial,
        placeholderText,
        polisher,
        Tr::tr("Edit Environment"));
}

} // namespace Utils
