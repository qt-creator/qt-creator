// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitutils.h"

#include <QInputDialog>
#include <QLineEdit>

namespace Git::Internal {

// Make QInputDialog  play nicely, widen it a bit.
bool inputText(QWidget *parent, const QString &title, const QString &prompt, QString *s)
{
    QInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(prompt);
    dialog.setTextValue(*s);
    // Nasty hack:
    if (auto le = dialog.findChild<QLineEdit*>())
        le->setMinimumWidth(500);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    *s = dialog.textValue();
    return true;
}

} // Git::Internal
