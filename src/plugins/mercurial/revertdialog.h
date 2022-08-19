// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_revertdialog.h"

#include <QDialog>

namespace Mercurial {
namespace Internal {

class RevertDialog : public QDialog
{
    Q_OBJECT

public:
    RevertDialog(QWidget *parent = nullptr);
    ~RevertDialog() override;

    QString revision() const;

private:
    Ui::RevertDialog *m_ui;
};

} // namespace Internal
} // namespace Mercurial
