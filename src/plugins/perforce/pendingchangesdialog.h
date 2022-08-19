// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include "ui_pendingchangesdialog.h"

namespace Perforce {
namespace Internal {

class PendingChangesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PendingChangesDialog(const QString &data, QWidget *parent = nullptr);
    int changeNumber() const;

private:
    Ui::PendingChangesDialog m_ui;
};

} // namespace Perforce
} // namespace Internal
