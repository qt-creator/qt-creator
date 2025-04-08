// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressdialog.h"

#include "guiutils.h"
#include "utilstr.h"

namespace Utils {

QProgressDialog *createProgressDialog(int maxValue, const QString &windowTitle,
                                      const QString &labelText)
{
    QProgressDialog *progressDialog = new QProgressDialog(labelText, Tr::tr("Cancel"),
                                                          0, maxValue, dialogParent());
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->setMinimumDuration(INT_MAX); // see QTBUG-135797
    progressDialog->setWindowTitle(windowTitle);
    progressDialog->setFixedSize(progressDialog->sizeHint());
    progressDialog->setAutoClose(false);
    progressDialog->show();
    return progressDialog;
}

} // namespace Utils
