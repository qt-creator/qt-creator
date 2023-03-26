// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentdialog.h"

#include "environment.h"
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
        Tr::tr("Edit Environment"),
        Tr::tr("Enter one environment variable per line.\n"
               "To set or change a variable, use VARIABLE=VALUE.\n"
               "To append to a variable, use VARIABLE+=VALUE.\n"
               "To prepend to a variable, use VARIABLE=+VALUE.\n"
               "Existing variables can be referenced in a VALUE with ${OTHER}.\n"
               "To clear a variable, put its name on a line with nothing else on it.\n"
               "To disable a variable, prefix the line with \"#\"."));
}

} // namespace Utils
