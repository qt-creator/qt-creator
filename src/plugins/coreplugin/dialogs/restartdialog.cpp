// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "restartdialog.h"

#include <coreplugin/icore.h>

#include <QPushButton>

namespace Core {

RestartDialog::RestartDialog(QWidget *parent, const QString &text)
    : QMessageBox(parent)
{
    setWindowTitle(tr("Restart Required"));
    setText(text);
    setIcon(QMessageBox::Information);
    addButton(tr("Later"), QMessageBox::NoRole);
    addButton(tr("Restart Now"), QMessageBox::YesRole);

    connect(this, &QDialog::accepted, ICore::instance(), &ICore::restart, Qt::QueuedConnection);
}

} // namespace Core
